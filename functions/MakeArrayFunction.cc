
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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.

#include "config.h"

#include <cassert>

#include <sstream>
#include <vector>

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

#include <debug.h>
#include <util.h>

#include <BaseTypeFactory.h>

#include <BESDebug.h>

#include "MakeArrayFunction.h"

namespace libdap {

/** Parse the shape 'expression'. The form of the expression is '[' size ']'
 * @return A vector of ints
 */
static vector<int>
parse_dims(const string &shape)
{
	vector<int> dims;
	istringstream iss(shape);
	string::size_type pos = 0;
	do {
		char brace;
		iss >> brace;
		++pos;
		// EOF is only found by reading past the last character
		// TODO Replace with a real parser? Find a better scanner/parser tool than flex/bison
		// or use the C++ versions?
		if (iss.eof())
			return dims;

		if (brace != '[' || iss.fail())
			throw Error(malformed_expr, "make_array(): Expected a left brace at position " + long_to_string(pos) + " in shape expression: " + shape);

		int size = 0;
		iss >> size;
		++pos;
		if (size == 0 || iss.fail())
			throw Error(malformed_expr, "make_array(): Expected an integer at position " + long_to_string(pos) + " in shape expression: " + shape);
		dims.push_back(size);

		iss >> brace;
		++pos;
		if (brace != ']' || iss.fail())
			throw Error(malformed_expr, "make_array(): Expected a right brace at position " + long_to_string(pos) + " in shape expression: " + shape);
	} while (!iss.eof());

	return dims;
}

template<class DAP_Primitive, class DAP_BaseType>
static void
read_values(int argc, BaseType *argv[], Array *dest)
{
    vector<DAP_Primitive> values;
    values.reserve(argc-2); 	// The number of values/elements to read

    // read argv[2]...argv[2+N-1] elements, convert them to type an load them in the Array.
    for (int i = 2; i < argc; ++i) {
    	BESDEBUG("functions", "Adding value: " << static_cast<DAP_BaseType*>(argv[i])->value() <<endl);
    	values.push_back(static_cast<DAP_BaseType*>(argv[i])->value());
    }

    BESDEBUG("functions", "values size: " << values.size() << endl);

    // copy the values to the DAP Array
    dest->set_value(values, values.size());
}

/** Given a BaseType, scale it using 'y = mx + b'. Either provide the
 constants 'm' and 'b' or the function will look for the COARDS attributes
 'scale_factor' and 'add_offset'.

 @param argc A count of the arguments
 @param argv An array of pointers to each argument, wrapped in a child of BaseType
 @param btpp A pointer to the return value; caller must delete.

 @return The scaled variable, represented using Float64
 @exception Error Thrown if scale_factor is not given and the COARDS
 attributes cannot be found OR if the source variable is not a
 numeric scalar, Array or Grid. */
void
function_make_array(int argc, BaseType * argv[], DDS &, BaseType **btpp)
{
    string info =
    string("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n") +
    "<function name=\"make_array\" version=\"1.0\" href=\"http://docs.opendap.org/index.php/Server_Side_Processing_Functions#make_array\">\n" +
    "</function>";

    if (argc == 0) {
        Str *response = new Str("info");
        response->set_value(info);
        *btpp = response;
        return;
    }

    // Check for two args or more. The first two must be strings.
    if (argc < 2)
    	throw Error(malformed_expr, "make_array(type,shape,[value0,...]) requires at least two arguments.");

    string type_name = extract_string_argument(argv[0]);
    string shape = extract_string_argument(argv[1]);

    BESDEBUG("functions", "type: " << type_name << endl);
    BESDEBUG("functions", "shape: " << shape << endl);

    // get the DAP type; NB: In DAP4 this will include Url4 and Enum
    Type type = libdap::get_type(type_name.c_str());
    if (!is_simple_type(type))
    	throw Error(malformed_expr, "make_array() can only build arrays of simple types (integers, floats and strings).");

    // parse the shape information. The shape expression form is [size0][size1]...[sizeN]
    // count [ and ] and the numbers should match (low budget invariant) and that's N
    // use an istringstream to read the integer sizes and build an Array
    vector<int> dims = parse_dims(shape);	// throws on parse error

    static int counter = 1;
    string name = "anon" + long_to_string(counter++);

    Array *dest = new Array(name, 0);	// The ctor for Array copies the prototype pointer...
    BaseTypeFactory btf;
    dest->add_var_nocopy(btf.NewVariable(type));	// ... so use add_var_nocopy() to add it instead

    unsigned long number_of_elements = 1;
    vector<int>::iterator i = dims.begin();
    while (i != dims.end()) {
    	number_of_elements *= *i;
    	dest->append_dim(*i++);
    }

    // Get the total element number
    // check that argc + 2 is N
    if (number_of_elements + 2 != (unsigned long)argc)
    	throw Error(malformed_expr, "make_array(): Expected " + long_to_string(number_of_elements) + " but found " + long_to_string(argc-2) + " instead.");

    switch (type) {
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

} // namesspace libdap
