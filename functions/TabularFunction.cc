
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
#include <D4RValue.h>
#include <Error.h>
#include <debug.h>
#include <util.h>
#include <ServerFunctionsList.h>

#include "TabularSequence.h"
#include "TabularFunction.h"

using namespace std;

namespace libdap {

/**
 * Build a vector of array dimension sizes used to compare with
 * other arrays to determine compatibility for the tabular function.
 *
 * @note Not a static function so it can be tested in the unit tests
 * @param a Read the size and number of dimensions from this array
 * @return The shape array
 */
vector<long long>
array_shape(Array *a)
{
    vector<long long> shape;

    for (Array::Dim_iter i = a->dim_begin(), e = a->dim_end(); i != e; ++i) {
        shape.push_back(a->dimension_size(i));
    }

    return shape;
}

/**
 * Does the given array match the shape of the vector? The vector
 * 'shape' is built at the start of tabular's run.
 *
 * @note Not a static function so it can be tested in the unit tests
 * @param a Test this array
 * @param shape The reference shape information
 * @return True if the shapes match, False otherwise.
 */
bool
shape_matches(Array *a, vector<long long> shape)
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
        ++i; ++si;
    }

    return true;
}

static long long
shape_values(vector<long long> shape)
{
    long long size = 1;
    vector<long long>::iterator si = shape.begin(), se = shape.end();
    while (si != se) {
        size *= *si++;
    }
    return size;
}

/** @brief Add the BaseTypes for the Sequence's columns.
 *
 * This function is passed the arguments of the function one at a time,
 * performs some simple testing, and adds them to a vector of Arrays.
 * This vector will be used to build the columns of the resulting
 * Sequence (or D4Sequence) and to read and build up the internal
 * store of values.
 *
 * @param n Column number
 * @param btp Pointer to the Basetype; must be an array
 * @param the_arrays Value-result parameter for the resulting BaseTypes
 * @param shape The array shape. Computer for the first array, subsequent
 * arrays must match it.
 */
static void
build_columns(unsigned long n, BaseType* btp, vector<Array*>& the_arrays, vector<long long> &shape)
{
    if (btp->type() != dods_array_c)
        throw Error( "In tabular(): Expected argument '" + btp->name() + "' to be an Array.");

    // We know it's an Array; cast, test, save and read the values
    Array *a = static_cast<Array*>(btp);
    // For the first array, record the number of dims and their sizes
    // for all subsequent arrays, test for a match
    if (n == 0)
        shape = array_shape(a);
    else if (!shape_matches(a, shape))
        throw Error("In tabular: Array '" + a->name() + "' does not match the shape of the initial Array.");

    a->read();
    a->set_read_p(true);

    the_arrays.at(n) = a;
}

/** @brief Load the values into a vector of a vector of BaseType pointers.
 *
 * Given a vector of the arrays that will supply values for this Sequence,
 * build up the values and load them into the SequenceValues/D4SeqValues
 * object that is the value-result parameter.
 *
 * @param the_arrays Extract data from these arrays
 * @param num_values The number of values (i.e, rows)
 * @param sv The destination object; a value-result parameter, passed
 * by reference. Note that DAP2's SequenceValues and DAP4's D4SeqValues
 * are both typedefs to a vector of vectors of BaseType pointers, so
 * both D2 and D4 objects can use this code.
 */
static void
get_sequence_values(vector<Array*> the_arrays, long long num_values, vector< vector<BaseType*>* > &sv)
{
    // This can be optimized for most cases because we're storing objects for Byte, Int32, ...
    // values where we could be storing native types. But this is DAP2 code... jhrg 2/3/15
    for (int i = 0; i < num_values; ++i) {
        // D4SeqRow, BaseTypeRow == vector<BaseType*>
        vector<BaseType*> *row = new vector<BaseType*>(the_arrays.size());

        for (unsigned long j = 0; j < the_arrays.size(); ++j) {
            DBG(cerr << "the_arrays.at(" << j << ") " << the_arrays.at(j) << endl);
            // i == row number; j == column (or array) number
            row->at(j) = the_arrays.at(j)->var(i)->ptr_duplicate();
            row->at(j)->set_send_p(true);
            row->at(j)->set_read_p(true);
        }

        sv.at(i) = row;
    }
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
 * @todo Write version for differing dimensions
 *
 * @param argc Argument count
 * @param argv Argument vector - variable in the current DDS
 * @param dds The current DDS
 * @param btpp Value-result parameter for the resulting Sequence
 */
void
function_dap2_tabular(int argc, BaseType *argv[], DDS &, BaseType **btpp)
{
    // jhrg 2/5/15 auto_ptr<Sequence> response(new Sequence("table"));
    unique_ptr<TabularSequence> response(new TabularSequence("table"));

    int num_arrays = argc;              // Might pass in other stuff...
    vector<long long> shape;            // Holds shape info; used to test array sizes for uniformity
    vector<Array*>the_arrays(num_arrays);

    for (int n = 0; n < num_arrays; ++n) {
        build_columns(n, argv[n], the_arrays, shape);
    }

    DBG(cerr << "the_arrays.size(): " << the_arrays.size() << endl);

    for (unsigned long n = 0; n < the_arrays.size(); ++n) {
        response->add_var(the_arrays[n]->var());
    }

    long long num_values = shape_values(shape);
    SequenceValues sv(num_values);
    // sv is a value-result parameter
    get_sequence_values(the_arrays, num_values, sv);

    response->set_value(sv);
    response->set_read_p(true);

    *btpp = response.release();
    return;
}

/**
 * This takes N Arrays where each is the same shape and returns a
 * DAP4 Sequence with the values of those arrays enumerated as a
 * table.
 *
 * @note The main difference between this function and the DAP2
 * version is to use args->size() in place of argc and
 * args->get_rvalue(n)->value(dmr) in place of argv[n].
 *
 * @see function_dap2_tabular
 */
BaseType *function_dap4_tabular(D4RValueList *args, DMR &dmr)
{
    unique_ptr<D4Sequence> response(new D4Sequence("table"));

    int num_arrays = args->size();              // Might pass in other stuff...
    vector<long long> shape;            // Holds shape info; used to test array sizes for uniformity
    vector<Array*>the_arrays(num_arrays);

    for (int n = 0; n < num_arrays; ++n) {
        build_columns(n, args->get_rvalue(n)->value(dmr), the_arrays, shape);
    }

    DBG(cerr << "the_arrays.size(): " << the_arrays.size() << endl);

    for (unsigned long n = 0; n < the_arrays.size(); ++n) {
        response->add_var(the_arrays[n]->var());
    }

    long long num_values = shape_values(shape);
    D4SeqValues sv(num_values);
    // sv is a value-result parameter
    get_sequence_values(the_arrays, num_values, sv);

    response->set_value(sv);
    response->set_read_p(true);

    return response.release();
}

} // namesspace libdap
