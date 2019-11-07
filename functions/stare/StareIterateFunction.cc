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

#include <sstream>
#include <unordered_map>

#include <hdf5.h>
#include <STARE.h>

#include <BaseType.h>
#include <Float64.h>
#include <Str.h>
#include <Array.h>
#include <Grid.h>
#include <DDS.h>

#include "DMR.h"
#include "D4RValue.h"

#include "Byte.h"
#include "Int16.h"
#include "Int32.h"
#include "UInt16.h"
#include "UInt32.h"
#include "Float32.h"
#include "Int64.h"
#include "UInt64.h"
#include "Int8.h"

#include <Error.h>

#include <debug.h>
#include <util.h>

#include "TheBESKeys.h"
#include "BESDebug.h"
#include "BESUtil.h"
#include "BESInternalError.h"
#include "BESSyntaxUserError.h"

#include "StareIterateFunction.h"

using namespace libdap;

namespace functions {

struct coords {
    int x;
    int y;
};

//TODO: Make into Template
struct returnVal {
    //TODO: These should probably be arrays/vectors
    vector<int> x;
    vector<int> y;
    vector<uint64> stareVal;
    vector<BaseType *> dataVal;
};

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
            return (uint64) (static_cast<Byte *>(arg)->value());
        case dods_uint16_c:
            return (uint64) (static_cast<UInt16 *>(arg)->value());
        case dods_int16_c:
            return (uint64) (static_cast<Int16 *>(arg)->value());
        case dods_uint32_c:
            return (uint64) (static_cast<UInt32 *>(arg)->value());
        case dods_int32_c:
            return (uint64) (static_cast<Int32 *>(arg)->value());
        case dods_float32_c:
            return (uint64) (static_cast<Float32 *>(arg)->value());
        case dods_float64_c:
            return (uint64) (static_cast<Float64 *>(arg)->value());

            // Support for DAP4 types.
        case dods_uint8_c:
            return (uint64) (static_cast<Byte *>(arg)->value());
        case dods_int8_c:
            return (uint64) (static_cast<Int8 *>(arg)->value());
        case dods_uint64_c:
            return static_cast<UInt64 *>(arg)->value();
        case dods_int64_c:
            return (uint64) (static_cast<Int64 *>(arg)->value());

        default:
            throw InternalErr(__FILE__, __LINE__,
                              "The argument list built by the parser contained an unsupported numeric type.");
    }
}

// May need to be moved to libdap/util
// This helper function assumes 'var' is the correct size.
vector<dods_uint64> *extract_uint64_array(Array *var) {
    assert(var);

    int length = var->length();

    vector<dods_uint64> *newVar = new vector<dods_uint64>;
    newVar->resize(length);
    var->value(&(*newVar)[0]);    // Extract the values of 'var' to 'newVar'

    return newVar;
}

/*
 * Use the passed in Stare Index to see if the sidecar file contains
 *  the provided Stare Index.
 * @param stareVal - stare values from given dataset
 * @param stareIndices - stare values being compared, retrieved from the sidecar file
 */
bool hasValue(vector<dods_uint64> *stareVal, vector<dods_uint64> *stareIndices) {
#if 0
    //Originally took the stareIndices in as a BaseType.
    //However, the stare indices can be taken from the provided h5 sidecar file and
    // the vector containing those values can be passed in directly instead of having
    // to be converted from BaseType to a uint64 array. -kln 10/22/19
    vector<dods_uint64> *stareData;

    Array &stareSrc = dynamic_cast<Array&>(*stareIndices);
    stareSrc.read();
    stareData = extract_uint64_array(&stareSrc);
#endif
    // Changes to the range-for loop, fixed the type (was unsigned long long
    // which works on OSX but not CentOS7). jhrg 11/5/19
    for (dods_uint64 &i : *stareIndices) {
        for (dods_uint64 &j : *stareVal)
            //TODO: Check to see if the index is within the stare index being compared
            if (i == j)
                return true;
    }

    return false;
}

/*
 * Count the number of times a Stare index matches in the
 *  sidecar
 * @param stareVal - stare values from given dataset
 * @param stareIndices - stare values being compared, retrieved from the sidecar file
 */
unsigned int count(vector<dods_uint64> *stareVal, vector<dods_uint64> *stareIndices) {
#if 0
    Array &stareSrc = dynamic_cast<Array&>(*stareIndices);

    stareSrc.read();

    vector<dods_uint64> *stareData = extract_uint64_array(&stareSrc);
#endif

    unsigned int counter = 0;
    for (dods_uint64 &stareIndex : *stareIndices) {
        for (dods_uint64 &j : *stareVal)
            if (stareIndex == j)
                counter++;
    }

    return counter;
}

/*
 * Return data from the provided data array where the requested stare indices are found
 */
returnVal stare_subset(vector<dods_uint64> *stareVal, vector<dods_uint64> *stareIndices,
                       vector<int> *xArray, vector<int> *yArray) {
    returnVal subset = returnVal();

    auto x = xArray->begin();
    auto y = yArray->begin();
    for (auto i = stareIndices->begin(), end = stareIndices->end(); i != end; i++, x++, y++) {
        for (dods_uint64 &j : *stareVal)
            if (*i == j)
                subset.stareVal.push_back(j);
        subset.x.push_back(*x);
        subset.y.push_back(*y);
        //subset.dataVal.push_back();
    }

    return subset;
}

string
StareIterateFunction::get_sidecar_file_pathname(const string &pathName) {
    size_t granulePos = pathName.find_last_of("/");
    string granuleName = pathName.substr(granulePos + 1);
    size_t findDot = granuleName.find_last_of(".");
    // Added extraction of the extension since the files won't always be *.h5
    // also switched to .append() instead of '+' because the former is faster.
    // jhrg 11/5/19
    string extension = granuleName.substr(findDot); // ext includes the dot
    string newPathName = granuleName.substr(0, findDot).append("_sidecar").append(extension);

    string stareDirectory = TheBESKeys::TheKeys()->read_string_key(STARE_STORAGE_PATH, "/tmp");

    string fullPath = BESUtil::pathConcat(stareDirectory, newPathName);
    return fullPath;
}

/**
 * -- Intersection server function --
 * Checks to see if there are any Stare values provided from the client that can be found in the sidecar file.
 * If there is at least one match, the function will return a 1. Otherwise returns a 0.
 */
BaseType *stare_intersection_dap4_function(D4RValueList *args, DMR &dmr) {

    if (args->size() != 1) {
        ostringstream oss;
        oss << "stare_intersection(): Expected a single argument, but got " << args->size();
        throw BESSyntaxUserError(oss.str(), __FILE__, __LINE__);
    }

    //Find the filename from the dmr
    string pathName = dmr.filename();
    string fullPath = StareIterateFunction::get_sidecar_file_pathname(pathName);

    //Read the file and store the datasets
    hid_t file = H5Fopen(fullPath.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
    if (file < 0)
        throw BESInternalError("Could not open file " + fullPath, __FILE__, __LINE__);

    hid_t xDataset = H5Dopen(file, "X", H5P_DEFAULT);
    if (xDataset < 0)
        throw BESInternalError("Could not open X dataset " + fullPath, __FILE__, __LINE__);

    hid_t yDataset = H5Dopen(file, "Y", H5P_DEFAULT);
    if (yDataset < 0)
        throw BESInternalError("Could not open Y dataset " + fullPath, __FILE__, __LINE__);

    hid_t stareDataset = H5Dopen(file, "Stare Index", H5P_DEFAULT);
    if (stareDataset < 0)
        throw BESInternalError("Could not open STARE dataset " + fullPath, __FILE__, __LINE__);

    // TODO Extract three functions/methods from this (one for XArray, ...)
    //Get the number of dimensions
    hid_t dspace = H5Dget_space(xDataset);
    const int ndims = H5Sget_simple_extent_ndims(dspace);
    hsize_t dims[ndims];

    //Get the size of the dimension so that we know how big to make the memory space
    //Each of the dataspaces should be the same size, if in the future they are different
    // sizes then the size of each dataspace will need to be calculated.
    H5Sget_simple_extent_dims(dspace, dims, NULL);

    //We need to get the filespace and memspace before reading the values from each dataset
    hid_t xFilespace = H5Dget_space(xDataset);
    hid_t yFilespace = H5Dget_space(yDataset);
    hid_t stareFilespace = H5Dget_space(stareDataset);

    hid_t xMemspace = H5Screate_simple(ndims, dims, NULL);
    hid_t yMemspace = H5Screate_simple(ndims, dims, NULL);
    hid_t stareMemspace = H5Screate_simple(ndims, dims, NULL);

    //Get the number of elements in the dataspace and use that to appropriate the proper size of the vectors
    hssize_t xSize = H5Sget_select_npoints(xFilespace);
    hssize_t ySize = H5Sget_select_npoints(yFilespace);
    hssize_t stareSize = H5Sget_select_npoints(stareFilespace);

    vector<int> xArray(xSize);
    vector<int> yArray(ySize);
    vector<dods_uint64> stareArray(stareSize);

    //Read the data file and store the values of each dataset into an array
    H5Dread(xDataset, H5T_NATIVE_INT, xMemspace, xFilespace, H5P_DEFAULT, &xArray[0]);
    H5Dread(yDataset, H5T_NATIVE_INT, yMemspace, yFilespace, H5P_DEFAULT, &yArray[0]);
    H5Dread(stareDataset, H5T_NATIVE_INT, stareMemspace, stareFilespace, H5P_DEFAULT, &stareArray[0]);

    BaseType *stareVal = args->get_rvalue(0)->value(dmr);
    Array *stareSrc = dynamic_cast<Array *>(stareVal);
    if (stareSrc == nullptr)
        throw BESSyntaxUserError("stare_intersection(): Expected an Array but found a " + stareVal->type_name(), __FILE__, __LINE__);

    if (stareSrc->var()->type() != dods_uint64_c)
        throw BESSyntaxUserError("Expected an Array of UInt64 values but found an Array of  " + stareSrc->var()->type_name(), __FILE__, __LINE__);

    stareSrc->read();

    vector<dods_uint64> *stareData = extract_uint64_array(stareSrc);

    bool status = hasValue(stareData, &stareArray);

    Int32 *result = new Int32("result");
    if (status) {
        result->set_value(1);
    }
    else {
        result->set_value(0);
    }

    return result;
}

#if 0
/**
 * -- Count server function --
 * Counts how many times the provided Stare values are found within the provided sidecar file.
 * Returns the number of matches found.
 */
BaseType *stare_count_dap4_function(D4RValueList *args, DMR &dmr) {
    string pathName = dmr.filename();

    //Find the filename from the dmr
    size_t granulePos = pathName.find_last_of("/");
    string granuleName = pathName.substr(granulePos + 1);
    size_t findDot = granuleName.find_last_of(".");
    string newPathName = granuleName.substr(0, findDot) + "_sidecar.h5";

    string rootDirectory = TheBESKeys::TheKeys()->read_string_key(STARE_STORAGE_PATH, "/tmp");

    string fullPath = BESUtil::pathConcat(rootDirectory, newPathName);

    //TODO: just use fullPath.c_str()
    //The H5Fopen function needs to read in a char *
    int n = fullPath.length();
    char fullPathChar[n + 1];
    strcpy(fullPathChar, fullPath.c_str());

    //Initialize the various variables for the datasets' info
    hid_t file;
    hid_t stareFilespace;
    hid_t stareMemspace;
    hid_t stareDataset;
    hssize_t stareSize;

    vector<uint64> stareArray;

    //Read the file and store the datasets
    file = H5Fopen(fullPathChar, H5F_ACC_RDONLY, H5P_DEFAULT);
    stareDataset = H5Dopen(file, "Stare Index", H5P_DEFAULT);

    //Get the number of dimensions
    hid_t dspace = H5Dget_space(stareDataset);
    const int ndims = H5Sget_simple_extent_ndims(dspace);
    hsize_t dims[ndims];

    //Get the size of the dimension so that we know how big to make the memory space
    H5Sget_simple_extent_dims(dspace, dims, NULL);

    //We need to get the filespace and memspace before reading the values from each dataset
    stareFilespace = H5Dget_space(stareDataset);

    stareMemspace = H5Screate_simple(ndims, dims, NULL);

    //Get the number of elements in the dataspace and use that to appropriate the proper size of the vectors
    stareSize = H5Sget_select_npoints(stareFilespace);
    stareArray.resize(stareSize);

    //Read the data file and store the values of each dataset into an array
    H5Dread(stareDataset, H5T_NATIVE_INT, stareMemspace, stareFilespace, H5P_DEFAULT, &stareArray[0]);

    BaseType *stareVal = args->get_rvalue(0)->value(dmr);
    Array *stareSrc = dynamic_cast<Array *>(stareVal);
    if (stareSrc == nullptr)
        throw BESSyntaxUserError("stare_intersection(): Expected an Array but found a " + stareVal->type_name(), __FILE__,
                               __LINE__);

    stareSrc->read();

    vector<dods_uint64> *stareData = extract_uint64_array(stareSrc);

    unsigned int numMatches = count(stareData, &stareArray);

    auto *result = new Int32("result");
    result->set_value(numMatches);

    return result;
}
#endif

} // namespace functions
