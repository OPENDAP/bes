// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of libdap, A C++ implementation of the OPeNDAP Data
// Access Protocol.

// Copyright (c) 2013 OPeNDAP, Inc.
// Authors: Nathan Potter <npotter@opendap.org>
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


/** Build a new DAP Array variable. Read the type, shape and values from the
 * arg list. The variable will be named anon<number> and is guaranteed not
 * to shadow the name of an existing variable in the DDS.
 *
 * @see function_bind_name
 *
 * @param argc A count of the arguments
 * @param argv An array of pointers to each argument, wrapped in a child of BaseType
 * @param btpp A pointer to the return value; caller must delete.
 * @return The new DAP Array variable.
 * @exception Error Thrown for a variety of errors.
 */
void function_dilate_dap2_array(int argc, BaseType * argv[], DDS &dds, BaseType **btpp)
{
    if (argc == 0) {
        Str *response = new Str("info");
        response->set_value(dilate_array_info);
        *btpp = response;
        return;
    }

    BESDEBUG("functions", "function_dilate_dap2_array() -  argc: " << long_to_string(argc) << endl);

    BaseType *btp = argv[0];

    if (btp->type() != dods_array_c) {
        throw Error(malformed_expr, "dilate_array(first argument must point to a DAP2 Array variable.");
    }

    Array *mask = static_cast<Array*>(btp);

    // Create the 'dilated' array using the shape of the input 'mask' array variable.

    Array *dest = new Array("dilated_mask", 0);	// The ctor for Array copies the prototype pointer...
    
    BaseTypeFactory btf;
    dest->add_var_nocopy(btf.NewVariable(dods_byte_c));	// ... so use add_var_nocopy() to add it instead

    for (Array::Dim_iter itr = mask->dim_begin(); itr != mask->dim_end(); ++itr) {
	dest->append_dim(mask->dimension_size(itr));
    }

    // Copy 'mask' array to initialize dilated array.

    dest->set_value(mask, mask->length());

    // read argv[1], the number of dilation operations (size of the 'hot-dog') to perform.

    if (!is_integer_type(argv[1]->type()))
        throw Error(malformed_expr, "dilate_array(): Expected an integer for the second argument.");

    unsigned int dSize = extract_uint_value(argv[1]);

    Array::Dim_iter itr = dest->dim_begin();
    int maxI = dest->dimension_size(itr);
    
    itr = dest->dim_end();
    int maxJ = dest->dimension_size(itr);

    //int maxI = 100; 
    //int maxJ = 100;
    
    // Dilate each mask location 'dSize' elements around it.

    // NB: currently not handling mask edge.

    for (unsigned int i=dSize; i<maxI-dSize; i++) {
	for (unsigned int j=dSize; j<maxJ-dSize; j++ ) {
	    if ( mask[i][j] == 1 ) {
		for (unsigned int x=i-dSize; x<i+dSize; x++) {
		    for (unsigned int y=j-dSize; y<j+dSize; y++) {
			dest.value[x][y] = 1;
		    }
		}
	    }
	}
    }



    dest->set_send_p(true);
    dest->set_read_p(true);

    // return the array
    *btpp = dest;
    return;
}


} // namesspace functions
