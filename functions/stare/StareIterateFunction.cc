// This file is part of libdap, A C++ implementation of the OPeNDAP Data
// Access Protocol.

// Copyright (c) 2013 OPeNDAP, Inc.
// Authors: Kodi Neumiller <kneumiller@opendap.org>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.

#include "config.h"
#include "STARE.h"

#include <sstream>

#include <BaseType.h>
#include <Float64.h>
#include <Str.h>
#include <Array.h>
#include <Grid.h>
#include "D4RValue.h"
#include "hdf5.h"

#include "Byte.h"
#include "Int16.h"
#include "Int32.h"
#include "UInt16.h"
#include "UInt32.h"
#include "Float32.h"
#include "Int64.h"
#include "UInt64.h"
#include "Int8.h"

#include <TheBESKeys.h>

#include <Error.h>
#include <DDS.h>

#include <debug.h>
#include <util.h>
#include <unordered_map>

#include "BESDebug.h"

using namespace libdap;

struct coords {
	int x;
	int y;
};

struct returnVal {
//TODO: These should probably be arrays
	int x;
	int y;
	uint64 stareVal;
	BaseType *dataVal;
};

namespace functions {

//May need to be moved to libdap/util
uint64 extract_uint64_value(BaseType *arg) {
	assert(arg);

	// Simple types are Byte, ..., Float64, String and Url.
	if (!arg->is_simple_type() || arg->type() == dods_str_c || arg->type() == dods_url_c)
		throw Error(malformed_expr, "The function requires a numeric-type argument.");

	if (!arg->read_p())
		throw InternalErr(__FILE__, __LINE__,
				"The Evaluator built an argument list where some constants held no values.");

	// The types of arguments that the CE Parser will build for numeric
	// constants are limited to Uint32, Int32 and Float64. See ce_expr.y.
	// Expanded to work for any numeric type so it can be used for more than
	// just arguments.
	switch (arg->type()) {
	case dods_byte_c:
		return (uint64) (static_cast<Byte*>(arg)->value());
	case dods_uint16_c:
		return (uint64) (static_cast<UInt16*>(arg)->value());
	case dods_int16_c:
		return (uint64) (static_cast<Int16*>(arg)->value());
	case dods_uint32_c:
		return (uint64) (static_cast<UInt32*>(arg)->value());
	case dods_int32_c:
		return (uint64) (static_cast<Int32*>(arg)->value());
	case dods_float32_c:
		return (uint64) (static_cast<Float32*>(arg)->value());
	case dods_float64_c:
		return (uint64) (static_cast<Float64*>(arg)->value());

		// Support for DAP4 types.
	case dods_uint8_c:
		return (uint64) (static_cast<Byte*>(arg)->value());
	case dods_int8_c:
		return (uint64) (static_cast<Int8*>(arg)->value());
	case dods_uint64_c:
		return static_cast<UInt64*>(arg)->value();
	case dods_int64_c:
		return (uint64) (static_cast<Int64*>(arg)->value());

	default:
		throw InternalErr(__FILE__, __LINE__,
				"The argument list built by the parser contained an unsupported numeric type.");
	}
}

//May need to be moved to libdap/util
// This helper function assumes 'var' is the correct size.
uint64 *extract_uint64_array(Array *var)
{
    assert(var);

    int length = var->length();

    vector<uint64> newVar(length);
    var->value(&newVar[0]);    // Extract the values of 'var' to 'newVar'

    uint64 *dest = new uint64[length];
    for (int i = 0; i < length; ++i)
        dest[i] = (uint64) newVar[i];

    return dest;
}

/*
 * Use the passed in Stare Index to see if the sidecar file contains
 *  the provided Stare Index.
 * @param *bt - The variable array
 * @param stareIndices - A vector of stare values
 */
bool hasValue(uint64 stareVal, BaseType *stareIndices) {
	uint64 *stareData;

	Array &stareSrc = dynamic_cast<Array&>(*stareIndices);

	stareSrc.read();

	stareData = extract_uint64_array(&stareSrc);

	for (int i = 0; i < stareSrc.length(); i++) {
		if (stareData[i] == stareVal)
			return true;
	}

	return false;
}

/*
 * Count the number of times a stare index matches in the
 *  variable array
 *
 */
int count(BaseType *bt, BaseType *stareIndices) {
	int counter = 0;

	uint64 *data;
	uint64 *stareData;

	Array &source = dynamic_cast<Array&>(*bt);
	Array &stareSrc = dynamic_cast<Array&>(*stareIndices);

	source.read();
	stareSrc.read();

	data = extract_uint64_array(&source);
	stareData = extract_uint64_array(&source);

	for (int i = 0; i < source.length(); i++) {
		//for (vector<uint64>::iterator j = stareIndices.begin(), e = stareIndices.end(); j != e; j++)
		for (int j = 0; j < stareSrc.length(); j++)
			if (stareData[j] == data[i])
				counter++;
	}

	return counter;
}

BaseType *stare_dap4_function(D4RValueList *args, DMR &dmr) {
/*	size_t granulePos = dmr.find_last_of("/");
	string granuleName = dmr.substr(granulePos + 1);
	size_t findDot = granuleName.find_last_of(".");
	string fileName = granuleName.substr(0, findDot) + "_sidecar.h5";

	string pathName = dmr.filename();

	string fullPath = BESUtil::pathConcat(pathName, fileName);
*/
	hsize_t dims[1];
	hid_t file;
	hid_t xFilespace, yFilespace, stareFilespace;
	hid_t xMemspace, yMemspace, stareMemspace;
	hid_t xDataset, yDataset, stareDataset;

	vector<int> xArray;
	vector<int> yArray;
	vector<BaseType> stareArray;

	const char *fileName ="test";
	file = H5Fopen(fileName, H5F_ACC_RDONLY, H5P_DEFAULT);
	xDataset = H5Dopen(file, "X", H5P_DEFAULT);
	yDataset = H5Dopen(file, "Y", H5P_DEFAULT);
	stareDataset = H5Dopen(file, "Stare Index", H5P_DEFAULT);

	xFilespace = H5Dget_space(xDataset);
	yFilespace = H5Dget_space(yDataset);
	stareFilespace = H5Dget_space(stareDataset);

	xMemspace = H5Screate_simple(1,dims,NULL);
	yMemspace = H5Screate_simple(1,dims,NULL);
	stareMemspace = H5Screate_simple(1,dims,NULL);

	H5Dread(xDataset, H5T_NATIVE_INT, xMemspace, xFilespace, H5P_DEFAULT, &xArray[0]);
	H5Dread(yDataset, H5T_NATIVE_INT, yMemspace, yFilespace, H5P_DEFAULT, &yArray[0]);
	H5Dread(stareDataset, H5T_NATIVE_INT, stareMemspace, stareFilespace, H5P_DEFAULT, &stareArray[0]);

#if 0
	H5File file(fullPath, H5F_ACC_RDONLY);
	DataSet x = file.openDataSet("X");
	DataSet y = file.openDataSet("Y");
	DataSet stareIndex = file.openDataSet("Stare Index");

	DataSpace xDataspace = x.getSpace();
	DataSpace yDataspace = y.getSpace();
	DataSpace stareDataspace = stareIndex.getSpace();


	DataSpace memspace (1, dims);

	x.read(xArray, PredType::NATIVE_INT, memspace, xDataspace); //TODO: Finish
	y.read(yArray, PredType::NATIVE_INT, memspace, yDataspace); //TODO: Finish
	stareIndex.read(stareArray, PredType::NATIVE_UINT64, memspace, stareDataspace); //TODO: Finish
#endif


//TODO: Refactor to work with BaseType. May be able to put this in a seperate function
/*	std::unordered_map<uint64, struct coords> stareMap;

	vector<int>::iterator xIter = xArray.begin();
	vector<int>::iterator yIter = yArray.begin();
	for (vector<uint64>::iterator i = stareArray.begin(), e = stareArray.end(); i != e; i++, xIter++, yIter++) {
		stareMap[*i].x = *xIter;
		stareMap[*i].y = *yIter;
	}
*/
	BaseType *result = 0;

	BaseType *varCheck = args->get_rvalue(0)->value(dmr);
	uint64 stareVal = extract_uint64_value(args->get_rvalue(1)->value(dmr));

	if (!hasValue(stareVal, &stareArray[0]))
		throw Error(malformed_expr, "Could not find any STARE values in the provided variable array");

	//TODO: Should return a BaseType struct that contains arrays of: x, y, stare index, and data value
	return result;
}

}
