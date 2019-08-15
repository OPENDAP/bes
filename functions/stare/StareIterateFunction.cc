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

#include <Error.h>
#include <DDS.h>

#include <debug.h>
#include <util.h>

#include "BESDebug.h"

using namespace libdap;

namespace functions {

//Probably should be moved to libdap/util
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
 * Check to see if the passed in variable array contains any of
 *  the stare indices.
 * @param *bt - The variable array
 * @param stareIndices - A vector of stare values
 */
bool hasValue(BaseType *bt, BaseType *stareIndices) {
	uint64 *data;
	uint64 *stareData;

	Array &source = dynamic_cast<Array&>(*bt);
	Array &stareSrc = dynamic_cast<Array&>(*stareIndices);

	source.read();
	stareSrc.read();

	data = extract_uint64_array(&source);
	stareData = extract_uint64_array(&source);

	for (int i = 0; i < source.length(); i++) {
		for (int j = 0; j < stareSrc.length(); j++)
			if (stareData[j] == data[i])
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
	BaseType *varCheck = args->get_rvalue(0)->value(dmr);
	BaseType *stareVals = args->get_rvalue(1)->value(dmr);

	if (!hasValue(varCheck, stareVals))
		throw Error(malformed_expr, "Could not find any STARE values in the provided variable array");

}

}
