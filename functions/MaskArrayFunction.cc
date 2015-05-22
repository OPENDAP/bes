// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of libdap, A C++ implementation of the OPeNDAP Data
// Access Protocol.

// Copyright (c) 2015 OPeNDAP, Inc.
// Authors: James Gallagher <jgallagher@opendap.org>
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
#include <algorithm>

#include <Type.h>
#include <BaseType.h>
#include <Str.h>
#include <Array.h>
#include <Structure.h>
#include <Error.h>
#include <DDS.h>

#include <DMR.h>
#include <D4Group.h>
#include <D4RValue.h>

#include <debug.h>
#include <util.h>

#include <BaseTypeFactory.h>

#include <BESDebug.h>

#include "MakeArrayFunction.h"
#include "functions_util.h"

using namespace libdap;

namespace functions {

string mask_array_info =
        string("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n")
                + "<function name=\"mask_array\" version=\"1.0\" href=\"http://docs.opendap.org/index.php/Server_Side_Processing_Functions#mask_array\">\n"
                + "</function>";

/**
 * Helper for the DAP2 and DAP4 server functions.
 *
 * After the server functions have done their QC, apply the mask to the
 * data array, altering its values in place.
 *
 * @note Assume the array, mask and no_data_value have been QC'd and are valid.
 *
 * @param array The data array
 * @param no_data_value Use this value to mark locations that are not set in the mask.
 * @param mask The mask. This is a binary mask, where 1 is 'set' and 0 is 'not set'.
 */
template <typename T>
void mask_array_helper(Array *array, double no_data_value, const vector<dods_byte> &mask)
{
    // Read the data array's data
    array->read();
    array->set_read_p(true);
    vector<T> data(array->length());
    array->value(&data[0]);

    assert(data.size() == mask.size());

    // mask the data array
    vector<dods_byte>::const_iterator mi = mask.begin();
    for (typename vector<T>::iterator i = data.begin(), e = data.end(); i != e; ++i) {
        if (!*mi++) *i = no_data_value;
    }

    // reset the values
    array->set_value(data, data.size());
}

/**
 * Implementation of the mask_array() function for DAP2.
 *
 * The mask_array() function takes one or more arrays and applies a mask
 * to the array, returning and array that holds the array's data values
 * wherever the mask is has a non-zero value and the supplied no-data
 * value otherwise.
 *
 * @param argc Three or more args
 * @param argv One or more arrays, a no data value and a mask array
 * @param dds Not used
 * @param btpp Value-result parameter that holds either the masked array
 * or a DAP2 String variable with usage information.
 */
void function_mask_dap2_array(int argc, BaseType * argv[], DDS &, BaseType **btpp)
{
    // Called with no args? Return usage information.
    if (argc == 0) {
        Str *response = new Str("info");
        response->set_value(mask_array_info);
        *btpp = response;
        return;
    }

    BESDEBUG("functions", "function_mask_dap2_array() -  argc: " << argc << endl);

    // QC args: must have a mask, ND value and 1+ array.
    if (argc < 3) throw Error(malformed_expr, "In mask_array(Array1, ..., ArrayN, NoData, Mask) requires at least three arguments.");

    // Get the NoData value; it must be a number
    double no_data_value = extract_double_value(argv[argc-2]);

    // Get the mask, which must be a DAP Byte array
    check_number_type_array(argv[argc-1]);      // Throws Error if not a numeric array
    Array *mask_var = static_cast<Array*>(argv[argc-1]);
    if (mask_var->var()->type() != dods_byte_c)
        throw Error(malformed_expr, "In mask_array(): Expected the last argument (the mask) to be a byte array.");

    mask_var->read();
    mask_var->set_read_p(true);
    vector<dods_byte> mask(mask_var->length());
    mask_var->value(&mask[0]);     // get the value

    // Now mask the arrays
    for (int i = 0; i < argc-2; ++i) {
        check_number_type_array (argv[i]);
        Array *array = static_cast<Array*>(argv[i]);
        // The Mask and Array(s) should match in shape, but to simplify use, we test
        // only that they have the same number of elements.
        if ((vector<dods_byte>::size_type)array->length() != mask.size())
            throw Error(malformed_expr, "In make_array(): The array '" + array->name() + "' and the mask do not match in size.");

        switch (array->var()->type()) {
        case dods_byte_c:
            mask_array_helper<dods_byte>(array, no_data_value, mask);
            break;
        case dods_int16_c:
            mask_array_helper<dods_int16>(array, no_data_value, mask);
            break;
        case dods_uint16_c:
            mask_array_helper<dods_uint16>(array, no_data_value, mask);
            break;
        case dods_int32_c:
            mask_array_helper<dods_int32>(array, no_data_value, mask);
            break;
        case dods_uint32_c:
            mask_array_helper<dods_uint32>(array, no_data_value, mask);
            break;
        case dods_float32_c:
            mask_array_helper<dods_float32>(array, no_data_value, mask);
            break;
        case dods_float64_c:
            mask_array_helper<dods_float64>(array, no_data_value, mask);
            break;
        default:
            throw InternalErr(__FILE__, __LINE__, "In mask_array(): Type " + array->type_name() + " not handled.");
        }
    }

    // Build the return value(s) - this means make copies of the masked arrays
    BaseType *dest = 0; // null_ptr
    if (argc == 3)
        dest = argv[0]->ptr_duplicate();
    else {
        dest = new Structure("masked_arays");
        for (int i = 0; i < argc-2; ++i) {
            dest->add_var(argv[i]);     //add_var() copies its arg
        }
    }

    dest->set_send_p(true);
    dest->set_read_p(true);

    // Return the array or structure containing the arrays
    *btpp = dest;

    return;
}

/**
 * Implementation of the mask_array() function for DAP4.
 *
 * @note I've duplicated a fair amount of code from teh DAP2 version; I could
 * code on in terms of the other, but I'll need to think about the type overlap
 * more before I do that. jhrg 4/30/15
 *
 * @param args One or more numeric arrays (int or float), a NoData value and
 * a mask (a byte array).
 * @param dmr
 * @return The masked array or a DAP4 String with usage information
 */
BaseType *function_mask_dap4_array(D4RValueList *args, DMR &dmr)
{
    // DAP4 function porting information: in place of 'argc' use 'args.size()'
    if (args == 0 || args->size() == 0) {
        Str *response = new Str("info");
        response->set_value(mask_array_info);
        // DAP4 function porting: return a BaseType* instead of using the value-result parameter
        return response;
    }

    // Check for 3+ arguments
    if (args->size() < 3) throw Error(malformed_expr, "In mask_array(Array1, ..., ArrayN, NoData, Mask) requires at least three arguments.");

    // Get the NoData value (second to last last); it must be a number
    double no_data_value = extract_double_value(args->get_rvalue(args->size()-2)->value(dmr));

    // Get the mask (last arg), which must be a DAP Byte array
    BaseType *mask_btp = args->get_rvalue(args->size()-1)->value(dmr);
    check_number_type_array (mask_btp);      // Throws Error if not a numeric array
    Array *mask_var = static_cast<Array*>(mask_btp);
    if (mask_var->var()->type() != dods_byte_c)
        throw Error(malformed_expr, "In mask_array(): Expected the last argument (the mask) to be a byte array.");

    mask_var->read();
    mask_var->set_read_p(true);
    vector<dods_byte> mask(mask_var->length());
    mask_var->value(&mask[0]);     // get the value

    // Now mask the arrays
    for (unsigned int i = 0; i < args->size() - 2; ++i) {
        BaseType *array_btp = args->get_rvalue(i)->value(dmr);
        check_number_type_array (array_btp);
        Array *array = static_cast<Array*>(array_btp);
        // The Mask and Array(s) should match in shape, but to simplify use, we test
        // only that they have the same number of elements.
        if ((vector<dods_byte>::size_type) array->length() != mask.size())
            throw Error(malformed_expr,
                    "In make_array(): The array '" + array->name() + "' and the mask do not match in size.");

        switch (array->var()->type()) {
        case dods_byte_c:
            mask_array_helper<dods_byte>(array, no_data_value, mask);
            break;
        case dods_int16_c:
            mask_array_helper<dods_int16>(array, no_data_value, mask);
            break;
        case dods_uint16_c:
            mask_array_helper<dods_uint16>(array, no_data_value, mask);
            break;
        case dods_int32_c:
            mask_array_helper<dods_int32>(array, no_data_value, mask);
            break;
        case dods_uint32_c:
            mask_array_helper<dods_uint32>(array, no_data_value, mask);
            break;
        case dods_float32_c:
            mask_array_helper<dods_float32>(array, no_data_value, mask);
            break;
        case dods_float64_c:
            mask_array_helper<dods_float64>(array, no_data_value, mask);
            break;
        default:
            throw InternalErr(__FILE__, __LINE__, "In mask_array(): Type " + array->type_name() + " not handled.");
        }
    }

    // Build the return value(s) - this means make copies of the masked arrays
    BaseType *dest = 0; // null_ptr
    if (args->size() == 3)
        dest = args->get_rvalue(0)->value(dmr)->ptr_duplicate();
    else {
        dest = new Structure("masked_arays");
        for (unsigned int i = 0; i < args->size() - 2; ++i) {
            dest->add_var(args->get_rvalue(i)->value(dmr));     //add_var() copies its arg
        }
    }

    dest->set_send_p(true);
    dest->set_read_p(true);

    return dest;
}

} // namesspace functions
