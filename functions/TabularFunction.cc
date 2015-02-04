
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
#include <sstream>
#include <memory>

#include <BaseType.h>
#include <Array.h>
#include <Sequence.h>
#include <D4Sequence.h>
#include <Error.h>
#include <debug.h>
#include <util.h>
#include <ServerFunctionsList.h>

#include "TabularFunction.h"

using namespace std;

namespace libdap {

/**
 * Build a vector of array dimension sizes used to compare with
 * other arrays to determine compatibility for the tabular function.
 *
 * @param a Read the size and number of dimensions from this array
 * @return
 */
vector<long long>
compute_array_shape(Array *a)
{
    vector<long long>shape(a->dimensions());

    for (Array::Dim_iter i = a->dim_begin(), e = a->dim_end(); i != e; ++i) {
        shape.push_back(a->dimension_size(i));
    }

    return shape;
}

/**
 * Does the given array match the shape of the vector? The vector
 * 'shape' is built at the start of tabular's run.
 * @param a Test this array
 * @param shape The reference shape information
 * @return True if the shapes match, False otherwise.
 */
bool
array_shape_matches(Array *a, vector<long long>shape)
{
    // Same number of dims
    if (shape.size() != a->dimensions())
        return false;

    // Each dim the same size
    Array::Dim_iter i = a->dim_begin(), e = a->dim_end();
    vector<long long>::iterator si = shape.begin(), se = shape.end();
    while (i != e && si != se) {
        if (*si != a->dimension_size(i))
            return false;
    }

    return true;
}

/** @brief Transform one or more arrays to a sequence.
 * This function will transform one of more arrays into a sequence,
 * where each array becomes a column in the sequence. Each
 * array must have the same number of dimensions and is
 * enumerated n row-major order (the right-most dimension varies
 * fastest).
 *
 * It's assumed that for each of the arrays, elements (i0, i1, ..., in)
 * are all related. The function makes no test to ensure that, however.
 *
 * @todo Write version for differing dimensions; independent -vs-
 * dependent vars.
 *
 * @param argc Argument count
 * @param argv Argument vector - variable in the current DDS
 * @param dds The current DDS
 * @param btpp Value-result parameter for the resulting Sequence
 */
void
function_dap2_tabular(int argc, BaseType *argv[], DDS &dds, BaseType **btpp)
{
    auto_ptr<Sequence> response(new Sequence("table"));

    int num_arrays = argc;              // Might pass in other stuff...
    vector<long long>shape;             // Holds shape info; used to test array sizes for uniformity
    vector<Array*>the_arrays(num_arrays);
    Array *a = 0;

    // Add support for N arrays, all of the same size
    for (int n = 0; n < num_arrays; ++n) {
        if (argv[n]->type() != dods_array_c)
            throw Error("In tabular(): Expected argument " + long_to_string(n) + " to be an Array, found a " + argv[n]->type_name() + " instead.");

        // We know it's an Array; cast, test, save and read the values
        a = static_cast<Array*>(argv[n]);

        // For the first array, record the number of dims and their sizes
        // for all subsequent arrays, test for a match
        if (n == 0)
            shape = compute_array_shape(a);
        else if (!array_shape_matches(a, shape))
            throw Error("In tabular: Array '" + a->name() + "' does not match the shape of the initial Array '" + argv[0]->name() + ". Array shapes must match.");
        else
            assert(!"In function_dap2_tabular, should never get here");

        the_arrays.push_back(a);
        a->read();
        a->set_read_p(true);

        // Add the column prototype to the result Sequence
        response->add_var(a->var());
    }

    // Now build the SequenceValues object; SequenceValues == vector<BaseTypeRow*>
    SequenceValues sv;

    // This can be optimized for most cases because we're storing objects for Byte, Int32, ...
    // values where we could be storing native types. But this is DAP2 code... jhrg 2/3/15
    for (int i = 0; i < a->length(); ++i) {
        BaseTypeRow *row = new BaseTypeRow(num_arrays); // BaseTypeRow == vector<BaseType*>

        for (int j = 0; j < num_arrays; ++j) {
            row->at(j) = the_arrays[j]->var(i);      // i == row number; j == column (or array) number
        }

        sv.push_back(row);
    }

    response->set_value(sv);

    *btpp = response.release();
    return;
}

/**
 * @todo Add a comment
 * @see function_dap2_tabular
 */
BaseType *function_dap4_tabular(D4RValueList *args, DMR &dmr)
{
    D4Sequence *response = new D4Sequence("table");

    return response;
}

} // namesspace libdap
