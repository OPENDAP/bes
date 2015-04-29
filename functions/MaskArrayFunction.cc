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
void mask_array(Array *array, double no_data_value, const vector<dods_byte> &mask)
{
    // Get the data in a vector
    // FIXME Only handles data arrays that are int32s...
    switch (array->var()->type()) {
    case dods_int32_c: {
        array->read();
        array->set_read_p(true);
        unsigned long data_size = array->length();
        vector<dods_int32> data(data_size);
        array->value(&data[0]);

        // Use transform here?
        vector<dods_byte>::const_iterator mi = mask.begin();
        for(vector<dods_int32>::iterator i = data.begin(), e = data.end(); i != e; ++i) {
            if (!*mi++) *i = no_data_value;
        }

        array->set_value(data, data.size());
        break;
    }
    default:
        throw InternalErr(__FILE__, __LINE__, "In mask_array(): Type " + array->type_name() + " not handled.");
    }
}

/**
 * Implementation of the mask_array() function for DAP2.
 *
 * The mask_array() function takes one or more arrays and applys a mask
 * to the array, returning and array that holds the array's data values
 * wherever the mask is has a non-zero value and the supplied no-data
 * value otherwise.
 *
 * @param argc
 * @param argv
 * @param dds
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
    if (argc < 3) throw Error(malformed_expr, "mask_array(Array1, ..., ArrayN, NoData, Mask) requires at least three arguments.");

    // Get the NoData value; it must be a number
    double no_data_value = extract_double_value(argv[argc-2]);

    // Get the mask, which must be a DAP Byte array
    BaseType *mask_var = argv[argc-1];

    check_number_type_array(mask_var);      // Throws Error if not a numeric array
    if (mask_var->var()->type() == dods_byte_c)
        throw Error(malformed_expr, "mask_array: Expected the last argument (the mask) to be a byte array.");

    mask_var->read();
    mask_var->set_read_p(true);
    vector<dods_byte> mask(mask_var->length());
    static_cast<Array*>(mask_var)->value(&mask[0]);     // get the value

    // The Mask and Array(s) must be numerical and match in shape
    // FIXME Add this.

    // Now mask the arrays
    for (int i = 0; i < argc-2; ++i) {
        mask_array(static_cast<Array*>(argv[i]), no_data_value, mask);
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
 * @param args
 * @param dmr
 * @return The masked array or a DAP4 String with usage information
 */
BaseType *function_mask_dap4_array(D4RValueList *args, DMR &)
{
    // DAP4 function porting information: in place of 'argc' use 'args.size()'
    if (args == 0 || args->size() == 0) {
        Str *response = new Str("info");
        response->set_value(mask_array_info);
        // DAP4 function porting: return a BaseType* instead of using the value-result parameter
        return response;
    }

#if 0
    // Check for 2 arguments
    DBG(cerr << "args.size() = " << args.size() << endl);
    if (args->size() < 2)
        throw Error(malformed_expr, "Wrong number of arguments to make_array(). See make_array() for more information");

    string requested_type_name = extract_string_argument(args->get_rvalue(0)->value(dmr));
    string shape = extract_string_argument(args->get_rvalue(1)->value(dmr));

    BESDEBUG("functions", "type: " << requested_type_name << endl);
    BESDEBUG("functions", "shape: " << shape << endl);

    // get the DAP type; NB: In DAP4 this will include Url4 and Enum
    Type requested_type = libdap::get_type(requested_type_name.c_str());
    if (!is_simple_type(requested_type))
        throw Error(malformed_expr,
                "make_array() can only build arrays of simple types (integers, floats and strings).");

    // parse the shape information. The shape expression form is [size0][size1]...[sizeN]
    // count [ and ] and the numbers should match (low budget invariant) and that's N
    // use an istringstream to read the integer sizes and build an Array
    vector<int> dims = parse_dims(shape);	// throws on parse error

    static unsigned long counter = 1;
    string name;
    do {
        name = "g" + long_to_string(counter++);
    } while (dmr.root()->var(name));

    Array *dest = new Array(name, 0);	// The ctor for Array copies the prototype pointer...
    BaseTypeFactory btf;
    dest->add_var_nocopy(btf.NewVariable(requested_type));	// ... so use add_var_nocopy() to add it instead

    unsigned long number_of_elements = 1;
    vector<int>::iterator i = dims.begin();
    while (i != dims.end()) {
        number_of_elements *= *i;
        dest->append_dim(*i++);
    }

    // Get the total element number
    // check that args.size() + 2 is N
    if (number_of_elements + 2 != args->size())
        throw Error(malformed_expr,
                "make_array(): Expected " + long_to_string(number_of_elements) + " parameters but found "
                        + long_to_string(args->size() - 2) + " instead.");

    switch (requested_type) {
    // All integer values are stored in Int32 DAP variables by the stock argument parser
    // except values too large; those are stored in a UInt32
    case dods_byte_c:
        read_values<dods_byte, Int32>(args, dmr, dest);
        break;

    case dods_int16_c:
        read_values<dods_int16, Int32>(args, dmr, dest);
        break;

    case dods_uint16_c:
        read_values<dods_uint16, Int32>(args, dmr, dest);
        break;

    case dods_int32_c:
        read_values<dods_int32, Int32>(args, dmr, dest);
        break;

    case dods_uint32_c:
        // FIXME Should be UInt32 but the parser uses Int32 unless a value is too big.
        read_values<dods_uint32, Int32>(args, dmr, dest);
        break;

    case dods_float32_c:
        read_values<dods_float32, Float64>(args, dmr, dest);
        break;

    case dods_float64_c:
        read_values<dods_float64, Float64>(args, dmr, dest);
        break;

    case dods_str_c:
        read_values<string, Str>(args, dmr, dest);
        break;

    case dods_url_c:
        read_values<string, Url>(args, dmr, dest);
        break;

    default:
        throw InternalErr(__FILE__, __LINE__, "Unknown type error");
    }
    dest->set_send_p(true);
    dest->set_read_p(true);

    return dest;
#endif

    return 0; // null_ptr
}

} // namesspace functions
