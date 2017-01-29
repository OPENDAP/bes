
// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of asciival, software which can return an ASCII
// representation of the data read from a DAP server.

// Copyright (c) 2002,2003 OPeNDAP, Inc.
// Author: James Gallagher <jgallagher@opendap.org>
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

// (c) COPYRIGHT URI/MIT 1998,2000
// Please read the full copyright statement in the file COPYRIGHT_URI.
//
// Authors:
//      jhrg,jimg       James Gallagher <jgallagher@gso.uri.edu>

// Implementation for AsciiArray. See AsciiByte.cc
//
// 3/12/98 jhrg

#include "config.h"

#include <iostream>
#include <string>
#include <algorithm>

using namespace std;

// #define DODS_DEBUG

#include "InternalErr.h"
#include "debug.h"

#include "AsciiArray.h"
#include "util.h"
#include "get_ascii.h"

using namespace dap_asciival;

BaseType *
AsciiArray::ptr_duplicate()
{
    return new AsciiArray(*this);
}

AsciiArray::AsciiArray(const string &n, BaseType *v) : Array(n, v)
{
}

AsciiArray::AsciiArray( Array *bt )
    : Array(bt->name(), 0), AsciiOutput( bt )
{
    // By calling var() without any parameters we get back the template
    // itself, then we can add it to this Array as the template. By doing
    // this we set the parent as well, which is what we need.
    BaseType *abt = basetype_to_asciitype( bt->var() ) ;
    add_var( abt ) ;
    // add_var makes a copy of the base type passed, so delete the original
    delete abt ;

    // Copy the dimensions
    Dim_iter p = bt->dim_begin();
    while ( p != bt->dim_end() ) {
        append_dim(bt->dimension_size(p, true), bt->dimension_name(p));
        ++p;
    }

    // I'm not particularly happy with this constructor; we should be able to
    // use the libdap ctors like BaseType::BaseType(const BaseType &copy_from)
    // using that via the Array copy ctor won't let us use the
    // basetype_to_asciitype() factory class. jhrg 5/19/09
    set_send_p(bt->send_p());
}

AsciiArray::~AsciiArray()
{
}

void AsciiArray::print_ascii(ostream &strm, bool print_name) throw(InternalErr)
{
    Array *bt = dynamic_cast < Array * >(_redirect);
    if (!bt) {
        bt = this;
    }

    if (bt->var()->is_simple_type()) {
        if (/*bt->*/dimensions(true) > 1) {
            print_array(strm, print_name);
        } else {
            print_vector(strm, print_name);
        }
    } else {
        print_complex_array(strm, print_name);
    }
}

// Print out a values for a vector (one dimensional array) of simple types.
void AsciiArray::print_vector(ostream &strm, bool print_name)
{
    Array *bt = dynamic_cast < Array * >(_redirect);
    if (!bt) {
        bt = this;
    }

    if (print_name)
        strm << dynamic_cast<AsciiOutput*>(this)->get_full_name() << ", " ;

    // only one dimension
    // Added the 'if (dimension_size...' in support of zero-length arrays.
    // jhrg 2/2/16
    if (dimension_size(dim_begin(), true) > 0) {
        int end = /*bt->*/dimension_size(/*bt->*/dim_begin(), true) - 1;

        for (int i = 0; i < end; ++i) {
            BaseType *curr_var = basetype_to_asciitype(bt->var(i));
            dynamic_cast<AsciiOutput &>(*curr_var).print_ascii(strm, false);
            strm << ", ";
            // we're not saving curr_var for future use, so delete it here
            delete curr_var;
        }
        BaseType *curr_var = basetype_to_asciitype(bt->var(end));
        dynamic_cast<AsciiOutput &>(*curr_var).print_ascii(strm, false);
        // we're not saving curr_var for future use, so delete it here
        delete curr_var;
    }
}

/** Print a single row of values for a N-dimensional array. Since we store
    N-dim arrays in vectors, #index# gives the starting point in that vector
    for this row and #number# is the number of values to print. The counter
    #index# is returned.

    @param os Write to stream os.
    @param index Print values starting from this point.
    @param number Print this many values.
    @return One past the last value printed (i.e., the index of the next
    row's first value).
    @see print\_array */
int AsciiArray::print_row(ostream &strm, int index, int number)
{
    Array *bt = dynamic_cast < Array * >(_redirect);
    if (!bt) {
        bt = this;
    }

    // Added 'if (number > 0)' to support zero-length arrays. jhrg 2/2/16
    // Changed to >= 0 to catch the edge case where the rightmost dimension
    // is constrained to be just one element. jhrg 6/9/16 (See Hyrax-225)
    if (number >= 0) {
        for (int i = 0; i < number; ++i) {
            BaseType *curr_var = basetype_to_asciitype(bt->var(index++));
            dynamic_cast<AsciiOutput &>(*curr_var).print_ascii(strm, false);
            strm << ", ";
            // we're not saving curr_var for future use, so delete it here
            delete curr_var;
        }
        BaseType *curr_var = basetype_to_asciitype(bt->var(index++));
        dynamic_cast<AsciiOutput &>(*curr_var).print_ascii(strm, false);
        // we're not saving curr_var for future use, so delete it here
        delete curr_var;
    }

    return index;
}

// Given a vector of indices, return the corresponding index.

int AsciiArray::get_index(vector < int >indices) throw(InternalErr)
{
    if (indices.size() != /*bt->*/dimensions(true)) {
        throw InternalErr(__FILE__, __LINE__,
                          "Index vector is the wrong size!");
    }
    // suppose shape is [3][4][5][6] for x,y,z,t. The index is
    // t + z(6) + y(5 * 6) + x(4 * 5 *6).
    // Assume that indices[0] holds x, indices[1] holds y, ...

    // It's hard to work with Pixes
    vector < int >shape = get_shape_vector(indices.size());

    // We want to work from the rightmost index to the left
    reverse(indices.begin(), indices.end());
    reverse(shape.begin(), shape.end());

    vector < int >::iterator indices_iter = indices.begin();
    vector < int >::iterator shape_iter = shape.begin();

    int index = *indices_iter++;        // in the ex. above, this adds `t'
    int multiplier = 1;
    while (indices_iter != indices.end()) {
        multiplier *= *shape_iter++;
        index += multiplier * *indices_iter++;
    }

    return index;
}

// get_shape_vector and get_nth_dim_size are public because that are called
// from Grid. 9/14/2001 jhrg

vector < int > AsciiArray::get_shape_vector(size_t n) throw(InternalErr)
{
    if (n < 1 || n > dimensions(true)) {
        string msg = "Attempt to get ";
        msg += long_to_string(n) + " dimensions from " + name()
            + " which has only " + long_to_string(dimensions(true))
            + "dimensions.";

        throw InternalErr(__FILE__, __LINE__, msg);
    }

    vector < int >shape;
    Array::Dim_iter p = dim_begin();
    for (unsigned i = 0; i < n && p != dim_end(); ++i, ++p) {
        shape.push_back(dimension_size(p, true));
    }

    return shape;
}

/** Get the size of the Nth dimension. The first dimension is N == 0.
    @param n The index. Uses sero-based indexing.
    @return the size of the dimension. */
int AsciiArray::get_nth_dim_size(size_t n) throw(InternalErr)
{
    if (n > /*bt->*/dimensions(true) - 1) {
        string msg = "Attempt to get dimension ";
        msg +=
            long_to_string(n + 1) + " from `" + /*bt->*/name() +
            "' which has " + long_to_string(/*bt->*/dimensions(true)) +
            " dimension(s).";
        throw InternalErr(__FILE__, __LINE__, msg);
    }

    return /*bt->*/dimension_size(/*bt->*/dim_begin() + n, true);
}

void AsciiArray::print_array(ostream &strm, bool /*print_name */ )
{
    DBG(cerr << "Entering AsciiArray::print_array" << endl);

    int dims = /*bt->*/dimensions(true);
    if (dims <= 1)
        throw InternalErr(__FILE__, __LINE__,
            "Dimension count is <= 1 while printing multidimensional array.");

    // shape holds the maximum index value of all but the last dimension of
    // the array (not the size; each value is one less that the size).
    vector < int >shape = get_shape_vector(dims - 1);
    int rightmost_dim_size = get_nth_dim_size(dims - 1);

    // state holds the indexes of the current row being printed. For an N-dim
    // array, there are N-1 dims that are iterated over when printing (the
    // Nth dim is not printed explicitly. Instead it's the number of values
    // on the row.
    vector < int >state(dims - 1, 0);

    bool more_indices;
    int index = 0;
    do {
        // Print indices for all dimensions except the last one.
	strm << dynamic_cast <AsciiOutput *>(this)->get_full_name() ;

        for (int i = 0; i < dims - 1; ++i) {
            strm << "[" << state[i] << "]" ;
        }
        strm << ", " ;

        index = print_row(strm, index, rightmost_dim_size - 1);
        more_indices = increment_state(&state, shape);
        if (more_indices)
            strm << "\n" ;

    } while (more_indices);

    DBG(cerr << "ExitingAsciiArray::print_array" << endl);
}

void AsciiArray::print_complex_array(ostream &strm, bool /*print_name */ )
{
    DBG(cerr << "Entering AsciiArray::print_complex_array" << endl);

    Array *bt = dynamic_cast < Array * >(_redirect);
    if (!bt)
        bt = this;

    int dims = /*bt->*/dimensions(true);
    if (dims < 1)
        throw InternalErr(__FILE__, __LINE__,
            "Dimension count is <= 1 while printing multidimensional array.");

    // shape holds the maximum index value of all but the last dimension of
    // the array (not the size; each value is one less than the size).
    vector < int >shape = get_shape_vector(dims);

    vector < int >state(dims, 0);

    bool more_indices;
    do {
        // Print indices for all dimensions except the last one.
        strm << dynamic_cast <AsciiOutput *>(this)->get_full_name() ;

        for (int i = 0; i < dims; ++i) {
            strm << "[" << state[i] << "]" ;
        }
        strm << "\n" ;

        BaseType *curr_var =
            basetype_to_asciitype(bt->var(get_index(state)));
        dynamic_cast < AsciiOutput & >(*curr_var).print_ascii(strm, true);
        // we are not saving curr_var for future reference, so delete it
        delete curr_var;

        more_indices = increment_state(&state, shape);
        if (more_indices)
            strm << "\n" ;

    } while (more_indices);

    DBG(cerr << "ExitingAsciiArray::print_complex_array" << endl);
}

