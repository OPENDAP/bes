
// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of libdap, A C++ implementation of the OPeNDAP Data
// Access Protocol.

// Copyright (c) 2013 OPeNDAP, Inc.
// Authors:  James Gallagher <jgallagher@opendap.org>
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

#include <BaseType.h>
#include <Array.h>
#include <Str.h>

#include <Error.h>
#include <DDS.h>

#include <debug.h>
#include <util.h>

#include <BESDebug.h>

#include "BindNameFunction.h"

namespace libdap {

vector<int> parse_dims(const string &shape); // defined in MakeArrayFunction.cc

/** Bind a shape to a DAP2 Array that is a vector. The product of the dimension
 * sizes must match the number of elements in the vector. This function takes
 * two arguments: A shape expression and a BaseType* to the DAP2 Array that holds
 * the data. In practice, the Array can already have a shape (it's a vector, so
 * that is a shape, e.g.) and this function simply changes that shape. The shape
 * expression is the C bracket notation for array size and is parsed by this
 * function.
 *
 * @param argc A count of the arguments
 * @param argv An array of pointers to each argument, wrapped in a child of BaseType
 * @param btpp A pointer to the return value; caller must delete.
 * @return The newly (re)named variable.
 * @exception Error Thrown for a variety of errors.
 */
void
function_bind_shape(int argc, BaseType * argv[], DDS &, BaseType **btpp)
{
    string info =
    string("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n") +
    "<function name=\"make_array\" version=\"1.0\" href=\"http://docs.opendap.org/index.php/Server_Side_Processing_Functions#bind_shape\">\n" +
    "</function>";

    if (argc == 0) {
        Str *response = new Str("info");
        response->set_value(info);
        *btpp = response;
        return;
    }

    // Check for two args or more. The first two must be strings.
    if (argc != 2)
    	throw Error(malformed_expr, "bind_shape(shape,variable) requires two arguments.");

    // string shape = extract_string_argument(argv[0]);
    vector<int> dims = parse_dims(extract_string_argument(argv[0]));

    Array *array = dynamic_cast<Array*>(argv[1]);
    if (!array)
    	throw Error(malformed_expr, "bind_shape() requires an Array as its second argument.");

    unsigned long vector_size = array->length();

    array->clear_all_dims();

    unsigned long number_of_elements = 1;
    vector<int>::iterator i = dims.begin();
    while (i != dims.end()) {
    	number_of_elements *= *i;
    	array->append_dim(*i++);
    }

    if (number_of_elements != vector_size)
    	throw Error(malformed_expr, "bind_shape(): The product of the new dimensions must match the size of the vector argument.");

    *btpp = argv[1];

    return;
}

} // namesspace libdap
