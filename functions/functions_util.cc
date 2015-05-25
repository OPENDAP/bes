
// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of bes, A C++ implementation of the OPeNDAP
// Hyrax data server

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
#include <string>
#include <sstream>
#include <vector>

#include "BaseType.h"
#include "Byte.h"
#include "Int16.h"
#include "Int32.h"
#include "UInt16.h"
#include "UInt32.h"
#include "Int64.h"
#include "UInt64.h"
#include "Int8.h"
#include "Float32.h"
#include "Float64.h"
#include "Str.h"
#include <Array.h>
#include <Error.h>
#include <util.h>

using namespace std;
using namespace libdap;

namespace functions {

/** Parse the shape 'expression'. The form of the expression is '[' size ']'
 * @note Also used by bind_shape()
 * @return A vector of ints
 */
vector<int> parse_dims(const string &shape)
{
    vector<int> dims;
    istringstream iss(shape);
    string::size_type pos = 0;
    do {
        char brace;
        iss >> brace;
        ++pos;
        // EOF is only found by reading past the last character
        if (iss.eof()) return dims;

        if (brace != '[' || iss.fail())
            throw Error(malformed_expr,
                    "make_array(): Expected a left brace at position " + long_to_string(pos) + " in shape expression: "
                            + shape);

        int size = 0;
        iss >> size;
        ++pos;
        if (size == 0 || iss.fail())
            throw Error(malformed_expr,
                    "make_array(): Expected an integer at position " + long_to_string(pos) + " in shape expression: "
                            + shape);
        dims.push_back(size);

        iss >> brace;
        ++pos;
        if (brace != ']' || iss.fail())
            throw Error(malformed_expr,
                    "make_array(): Expected a right brace at position " + long_to_string(pos) + " in shape expression: "
                            + shape);
    } while (!iss.eof());

    return dims;
}

/**
 * Test for acceptable array types for the N-1 arguments of roi().
 * Throw Error if the array is not valid for this function
 *
 * @param btp Test this variable.
 * @param rank If given and not zero, the Array must have rank equal to
 * or at most one greater than the value of this parameter.
 * @exception Error thrown if the array is not valid
 */
void check_number_type_array(BaseType *btp, unsigned int rank /* = 0 */)
{
    if (!btp)
        throw InternalErr(__FILE__, __LINE__, "roi() function called with null variable.");

    if (btp->type() != dods_array_c)
        throw Error("In function roi(): Expected argument '" + btp->name() + "' to be an Array.");

    Array *a = static_cast<Array *>(btp);
    if (!a->var()->is_simple_type() || a->var()->type() == dods_str_c || a->var()->type() == dods_url_c)
        throw Error("In function roi(): Expected argument '" + btp->name() + "' to be an Array of numeric types.");

    if (rank && !(a->dimensions() == rank || a->dimensions() == rank+1))
        throw Error("In function roi(): Expected the array '" + a->name() +"' to be rank " + long_to_string(rank) + " or " + long_to_string(rank+1) + ".");
}

/** Given a BaseType pointer, extract the numeric value it contains and return
 it in a C++ integer.

 @note Support for DAP4 types added.

 @param arg The BaseType pointer
 @return A C++ unsigned integer
 @exception Error thrown if the referenced BaseType object does not contain
 a DAP numeric value. */
unsigned int extract_uint_value(BaseType *arg)
{
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
      return (unsigned int) (static_cast<Byte*>(arg)->value());
    case dods_uint16_c:
      return (unsigned int) (static_cast<UInt16*>(arg)->value());
    case dods_int16_c:
      return (unsigned int) (static_cast<Int16*>(arg)->value());
    case dods_uint32_c:
      return (unsigned int) (static_cast<UInt32*>(arg)->value());
    case dods_int32_c:
      return (unsigned int) (static_cast<Int32*>(arg)->value());
    case dods_float32_c:
      return (unsigned int) (static_cast<Float32*>(arg)->value());
    case dods_float64_c:
      return (unsigned int)static_cast<Float64*>(arg)->value();

        // Support for DAP4 types.
    case dods_uint8_c:
      return (unsigned int) (static_cast<Byte*>(arg)->value());
    case dods_int8_c:
      return (unsigned int) (static_cast<Int8*>(arg)->value());
    case dods_uint64_c:
      return (unsigned int) (static_cast<UInt64*>(arg)->value());
    case dods_int64_c:
      return (unsigned int) (static_cast<Int64*>(arg)->value());

    default:
      throw InternalErr(__FILE__, __LINE__,
			"The argument list built by the parser contained an unsupported numeric type.");
    }
}

} // namespace functions
