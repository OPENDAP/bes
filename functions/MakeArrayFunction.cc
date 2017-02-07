// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of libdap, A C++ implementation of the OPeNDAP Data
// Access Protocol.

// Copyright (c) 2013 OPeNDAP, Inc.
// Authors: Nathan Potter <npotter@opendap.org>
//         James Gallagher <jgallagher@opendap.org>
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

#include "MakeArrayFunction.h"
#include "functions_util.h"

using namespace libdap;

namespace functions {

string make_array_info =
        string("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n")
                + "<function name=\"make_array\" version=\"1.0\" href=\"http://docs.opendap.org/index.php/Server_Side_Processing_Functions#make_array\">\n"
                + "</function>";

bool isValidTypeMatch(Type requestedType, Type argType)
{
    bool typematch_status = false;
    switch (requestedType) {
    case dods_byte_c:
    case dods_int16_c:
    case dods_uint16_c:
    case dods_int32_c:
    case dods_uint32_c: {
        // All integer values are stored in Int32 DAP variables by the stock argument parser
        // except values too large; those are stored in a UInt32 these return the same size value
        switch (argType) {
        case dods_int32_c:
        case dods_uint32_c: {
            typematch_status = true;
            break;
        }
        default:
            break;
        }
        break;
    }

    case dods_float32_c:
    case dods_float64_c: {
        // All floating point values are stored as Float64 by the stock argument parser
        switch (argType) {
        case dods_float64_c: {
            typematch_status = true;
            break;
        }
        default:
            break;
        }
        break;
    }

    case dods_str_c:
    case dods_url_c: {
        // Strings and Urls, like Int32 and UInt32 are pretty much the same
        switch (argType) {
        case dods_str_c:
        case dods_url_c: {
            typematch_status = true;
            break;
        }
        default:
            break;
        }
        break;
    }

    default:
        throw InternalErr(__FILE__, __LINE__, "Unknown type error");
    }

    return typematch_status;
}

template<class DAP_Primitive, class DAP_BaseType>
static void read_values(int argc, BaseType *argv[], Array *dest)
{
    vector<DAP_Primitive> values;
    values.reserve(argc - 2); 	// The number of values/elements to read

    string requestedTypeName = extract_string_argument(argv[0]);
    Type requestedType = libdap::get_type(requestedTypeName.c_str());
    BESDEBUG("functions", "make_dap2_array() - Requested array type: " << requestedTypeName<< endl);

    // read argv[2]...argv[2+N-1] elements, convert them to type an load them in the Array.
    for (int i = 2; i < argc; ++i) {
        BESDEBUG("functions", "make_dap2_array() - Adding value of type " << argv[i]->type_name() << endl);
        if (!isValidTypeMatch(requestedType, argv[i]->type())) {
            throw Error(malformed_expr,
                    "make_array(): Expected values to be of type " + requestedTypeName + " but argument "
                            + long_to_string(i) + " evaluated into a type " + argv[i]->type_name() + " instead.");
        }
        BESDEBUG("functions", "make_dap2_array() - Adding value: " << static_cast<DAP_BaseType*>(argv[i])->value() << endl);
        values.push_back(static_cast<DAP_BaseType*>(argv[i])->value());
    }

    BESDEBUG("functions", "make_dap2_array() - values size: " << values.size() << endl);

    // copy the values to the DAP Array
    dest->set_value(values, values.size());
}

template<class DAP_Primitive, class DAP_BaseType>
static void read_values(D4RValueList *args, DMR &dmr, Array *dest)
{
    vector<DAP_Primitive> values;
    values.reserve(args->size() - 2); 	// The number of values/elements to read

    string requestedTypeName = extract_string_argument(args->get_rvalue(0)->value(dmr));
    Type requestedType = libdap::get_type(requestedTypeName.c_str());
    BESDEBUG("functions", "make_dap2_array() - Requested array type: " << requestedTypeName<< endl);

    // read argv[2]...argv[2+N-1] elements, convert them to type an load them in the Array.
    for (unsigned int i = 2; i < args->size(); ++i) {

        BESDEBUG("functions", "Adding value of type " << args->get_rvalue(i)->value(dmr)->type_name() << endl);
        if (!isValidTypeMatch(requestedType, args->get_rvalue(i)->value(dmr)->type())) {
            throw Error(malformed_expr,
                    "make_array(): Expected values to be of type " + requestedTypeName + " but argument "
                            + long_to_string(i) + " evaluated into a type "
                            + args->get_rvalue(i)->value(dmr)->type_name() + " instead.");
        }

        BESDEBUG("functions",
                "Adding value: " << static_cast<DAP_BaseType*>(args->get_rvalue(i)->value(dmr))->value() <<endl);
        values.push_back(static_cast<DAP_BaseType*>(args->get_rvalue(i)->value(dmr))->value());
    }

    BESDEBUG("functions", "values size: " << values.size() << endl);

    // copy the values to the DAP Array
    dest->set_value(values, values.size());
}

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
void function_make_dap2_array(int argc, BaseType * argv[], DDS &dds, BaseType **btpp)
{
    if (argc == 0) {
        Str *response = new Str("info");
        response->set_value(make_array_info);
        *btpp = response;
        return;
    }

    BESDEBUG("functions", "function_make_dap2_array() -  argc: " << long_to_string(argc) << endl);

    // Check for two args or more. The first two must be strings.
    if (argc < 2) throw Error(malformed_expr, "make_array(type,shape,[value0,...]) requires at least two arguments.");

    string requested_type_name = extract_string_argument(argv[0]);
    string shape = extract_string_argument(argv[1]);

    BESDEBUG("functions", "function_make_dap2_array() -  type: " << requested_type_name << endl);
    BESDEBUG("functions", "function_make_dap2_array() - shape: " << shape << endl);

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
    } while (dds.var(name));

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
    // check that argc + 2 is N
    if (number_of_elements + 2 != (unsigned long) argc)
        throw Error(malformed_expr,
                "make_array(): Expected " + long_to_string(number_of_elements) + " parameters but found "
                        + long_to_string(argc - 2) + " instead.");

    switch (requested_type) {
    // All integer values are stored in Int32 DAP variables by the stock argument parser
    // except values too large; those are stored in a UInt32
    case dods_byte_c:
        read_values<dods_byte, Int32>(argc, argv, dest);
        break;

    case dods_int16_c:
        read_values<dods_int16, Int32>(argc, argv, dest);
        break;

    case dods_uint16_c:
        read_values<dods_uint16, Int32>(argc, argv, dest);
        break;

    case dods_int32_c:
        read_values<dods_int32, Int32>(argc, argv, dest);
        break;

    case dods_uint32_c:
        // FIXME Should be UInt32 but the parser uses Int32 unless a value is too big.
        read_values<dods_uint32, Int32>(argc, argv, dest);
        break;

    case dods_float32_c:
        read_values<dods_float32, Float64>(argc, argv, dest);
        break;

    case dods_float64_c:
        read_values<dods_float64, Float64>(argc, argv, dest);
        break;

    case dods_str_c:
        read_values<string, Str>(argc, argv, dest);
        break;

    case dods_url_c:
        read_values<string, Url>(argc, argv, dest);
        break;

    default:
        throw InternalErr(__FILE__, __LINE__, "Unknown type error");
    }

    dest->set_send_p(true);
    dest->set_read_p(true);

    // return the array
    *btpp = dest;
    return;
}

BaseType *function_make_dap4_array(D4RValueList *args, DMR &dmr)
{
    // DAP4 function porting information: in place of 'argc' use 'args.size()'
    if (args == 0 || args->size() == 0) {
        Str *response = new Str("info");
        response->set_value(make_array_info);
        // DAP4 function porting: return a BaseType* instead of using the value-result parameter
        return response;
    }

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

}

} // namesspace functions
