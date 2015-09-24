// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of libdap, A C++ implementation of the OPeNDAP Data
// Access Protocol.

// Copyright (c) 2015 OPeNDAP, Inc.
// Authors:
//         James Gallagher <jgallagher@opendap.org>
//         Dan Holloway <dholloway@opendap.org>
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

#include <cassert>

#include <sstream>
#include <vector>

#include <Type.h>
#include <BaseType.h>
#include <Byte.h>
#include <Int16.h>
#include <UInt16.h>
#include <Int32.h>
#include <UInt32.h>
#include <Float32.h>
#include <Float64.h>
#include <Str.h>
#include <Url.h>
#include <Array.h>
#include <Error.h>
#include <DDS.h>

#include <DMR.h>
#include <D4Group.h>
#include <D4RValue.h>

#include <debug.h>
#include <util.h>

#include <BaseTypeFactory.h>

#include <BESDebug.h>

#include "DilateArrayFunction.h"
#include "functions_util.h"

using namespace libdap;

namespace functions {

string dilate_array_info =
        string("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n")
                + "<function name=\"dilate_array\" version=\"1.0\" href=\"http://docs.opendap.org/index.php/Server_Side_Processing_Functions#dilate_array\">\n"
                + "</function>";

/**
 * Given a 'mask array,' apply a dilation operation to the set bits in that
 * mask. The mask is the first parameter and must be a DAP Byte array. The
 * size of the dilation in pixels is given by the second argument.
 *
 * @param argc A count of the arguments
 * @param argv An array of pointers to each argument, wrapped in a child of BaseType
 * @param btpp A pointer to the return value; caller must delete.
 * @return The modified Mask Array.
 * @exception Error Thrown for a variety of errors.
 */
void function_dilate_dap2_array(int argc, BaseType * argv[], DDS &/*dds*/, BaseType **btpp)
{
    if (argc == 0) {
        Str *response = new Str("info");
        response->set_value(dilate_array_info);
        *btpp = response;
        return;
    }

    BESDEBUG("functions", "function_dilate_dap2_array() -  argc: " << argc << endl);

    BaseType *btp = argv[0];

    if (btp->type() != dods_array_c)
        throw Error(malformed_expr, "dilate_array(): first argument must point to a Array variable.");

    Array *mask = static_cast<Array*>(btp);
    if (mask->var()->type() != dods_byte_c && mask->dimensions() == 2)
        throw Error(malformed_expr, "dilate_array(): first argument must point to a Two dimensional Byte Array variable.");

    // OK, we've got a valid 2D mask, now get the bytes into a vector.
    vector<dods_byte> mask_values(mask->length());
    mask->value(&mask_values[0]);

    // Now make a vector<> for the result; I'm not sure if we really need this... jhrg 5/26/15
    vector<dods_byte> dest_values(mask->length());

    // read argv[1], the number of dilation operations (size of the 'hot-dog') to perform.
    if (!is_integer_type(argv[1]->type()))
        throw Error(malformed_expr, "dilate_array(): Expected an integer for the second argument.");

    unsigned int dSize = extract_uint_value(argv[1]);

    Array::Dim_iter itr = mask->dim_begin();
    int maxI = mask->dimension_size(itr++);
    int maxJ = mask->dimension_size(itr);

    // Dilate each mask location 'dSize' elements around it.
    // NB: currently not handling mask edge.
    for (unsigned int i=dSize; i<maxI-dSize; i++) {
	for (unsigned int j=dSize; j<maxJ-dSize; j++ ) {
	    int mask_offset = j + i * maxI;
	    if ( mask_values.at(mask_offset) /*mask[i][j]*/ == 1 ) {
	        // I think this could be modified to handle the edge case by expanding the
	        // ranges above to be the whole image and then using max() and min() in the
	        // initialization and loop tests below. Not sure though. jhrg 5/16/15
		for (unsigned int x=i-dSize; x<=i+dSize; x++) {
		    for (unsigned int y=j-dSize; y<=j+dSize; y++) {
		        int dest_offset = y + x * maxI;
			dest_values.at(dest_offset) = 1; //dest.value[x][y] = 1;
		    }
		}
	    }
	}
    }

    // Create the 'dilated' array using the shape of the input 'mask' array variable.
    Array *dest = new Array("dilated_mask", 0); // The ctor for Array copies the prototype pointer...

    BaseTypeFactory btf;
    dest->add_var_nocopy(btf.NewVariable(dods_byte_c)); // ... so use add_var_nocopy() to add it instead

    dest->append_dim(maxI);
    dest->append_dim(maxJ);

    dest->set_value(dest_values, mask->length());
    dest->set_send_p(true);
    dest->set_read_p(true);

    // return the array
    *btpp = dest;

    return;
}


} // namesspace functions
