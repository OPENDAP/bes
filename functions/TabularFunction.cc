
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
#include <climits>

#include <sstream>
#include <memory>
#include <algorithm>

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
 * Simple function to read the values of an Array. Can be called on its
 * own or with for_each().
 *
 * @param a Call read() for this variable. Also sets the read_p property
 */
static void read_array_values(Array *a)
{
    a->read();
    a->set_read_p(true);
}

/**
 * Build a vector of array dimension sizes used to compare with
 * other arrays to determine compatibility for the tabular function.
 *
 * @note Not a static function so it can be tested in the unit tests
 * @param a Read the size and number of dimensions from this array
 * @return The shape array
 */
vector<unsigned long>
TabularFunction::array_shape(Array *a)
{
    vector<unsigned long> shape;

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
TabularFunction::shape_matches(Array *a, vector<unsigned long> shape)
{
    // Same number of dims
    if (shape.size() != a->dimensions())
        return false;

    // Each dim the same size
    Array::Dim_iter i = a->dim_begin(), e = a->dim_end();
    vector<unsigned long>::iterator si = shape.begin(), se = shape.end();
    while (i != e && si != se) {
        assert(a->dimension_size(i) >= 0);
        if (*si != (unsigned long)a->dimension_size(i))
            return false;
        ++i; ++si;
    }

    return true;
}

unsigned long
TabularFunction::number_of_values(vector<unsigned long> shape)
{
    unsigned long size = 1;
    vector<unsigned long>::iterator si = shape.begin(), se = shape.end();
    while (si != se) {
        size *= *si++;
    }
    return size;
}

/**
 * @brief Add the BaseTypes for the Sequence's columns.
 *
 * This function is passed the arguments of the tabular() server function
 * one at a time, performs some simple testing, and adds them to a vector
 * of Arrays. This vector will be used to build the columns of the resulting
 * Sequence (or D4Sequence) and to read and build up the internal
 * store of values.
 *
 * The first time this function is called, it records the shape of the
 * DAP Array passed in using the BaseType*. That shape is returned as a
 * value-result parameter and forms the baseline for testing the subsequent
 * arrays' shape, which must match
 *
 * @param n Column number
 * @param btp Pointer to the Basetype; must be an Array
 * @param the_arrays Value-result parameter for the resulting BaseTypes
 * @param shape The array shape. Computer for the first array, subsequent
 * arrays must match it.
 */
void
TabularFunction::build_columns(unsigned long n, BaseType* btp, vector<Array*>& the_arrays, vector<unsigned long> &shape)
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

    read_array_values(a);

    the_arrays.at(n) = a;       // small number of Arrays; use the safe method
}

/**
 * For each Array in the vector of Arrays, read its values into
 * internal memory and set the read_p property.
 *
 * @param arrays For this vector<Array*>, read the data for each array.
 */
void TabularFunction::read_values(vector<Array*> arrays)
{
    // NB: read_array_values is defined at the very top of this file
    for_each(arrays.begin(), arrays.end(), read_array_values);
}

/**
 * @brief Load the values into a vector of a vector of BaseType pointers.
 *
 * Given a vector of the arrays that will supply values for this Sequence,
 * build up the values and load them into the SequenceValues/D4SeqValues
 * object that is the value-result parameter.
 *
 * @note This code depends on each Array in 'arrays' having already read its
 * values. It will throw Error if that is not the case.
 *
 * @param the_arrays Extract data from these arrays
 * @param num_values The number of values (i.e, rows)
 * @param sv The destination object; a value-result parameter, passed
 * by reference. Note that DAP2's SequenceValues and DAP4's D4SeqValues
 * are both typedefs to a vector of vectors of BaseType pointers, so
 * both D2 and D4 objects can use this code.
 */
void
TabularFunction::build_sequence_values(vector<Array*> the_arrays, SequenceValues &sv)
{
    // This can be optimized for most cases because we're storing objects for Byte, Int32, ...
    // values where we could be storing native types. But this is DAP2 code... jhrg 2/3/15
    //
    // NB: SequenceValues == vector< vector<BaseType*> *>, and
    // D4SeqRow, BaseTypeRow == vector<BaseType*>
    for (SequenceValues::size_type i = 0; i < sv.size(); ++i) {

        BaseTypeRow *row = new BaseTypeRow(the_arrays.size());

        for (BaseTypeRow::size_type j = 0; j < the_arrays.size(); ++j) {
            DBG(cerr << "the_arrays.at(" << j << ") " << the_arrays.at(j) << endl);
            // i == row number; j == column (or array) number
            (*row)[j]/*->at(j)*/ = the_arrays[j]/*.at(j)*/->var(i)->ptr_duplicate();

            (*row)[j]->set_send_p(true);
            (*row)[j]->set_read_p(true);
        }

        sv[i]/*.at(i)*/ = row;
    }
}

/** @brief Transform one or more arrays to a sequence.
 *
 * This function will transform one of more arrays into a sequence,
 * where each array becomes a column in the sequence. Each
 * array must have the same number of dimensions and is
 * enumerated in row-major order (the right-most dimension varies
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
TabularFunction::function_dap2_tabular(int argc, BaseType *argv[], DDS &, BaseType **btpp)
{
    // unique_ptr is not avialable on gcc 4.2. jhrg 2/11/15
    // unique_ptr<TabularSequence> response(new TabularSequence("table"));
    auto_ptr<TabularSequence> response(new TabularSequence("table"));

    int num_arrays = argc;              // Might pass in other stuff...
    vector<unsigned long> shape;            // Holds shape info; used to test array sizes for uniformity
    vector<Array*>the_arrays(num_arrays);

    // Read each array passed to tabular(), check that its shape matches
    // the first array's shape, store the array in a vector and read in
    // it's values (into the Array object's internal store).
    for (int n = 0; n < num_arrays; ++n) {
        TabularFunction::build_columns(n, argv[n], the_arrays, shape);
    }

    DBG(cerr << "the_arrays.size(): " << the_arrays.size() << endl);

    // Now build the response Sequence so it has columns that match the
    // Array element types
    for (unsigned long n = 0; n < the_arrays.size(); ++n) {
        response->add_var(the_arrays[n]->var());
    }

    unsigned long num_values = TabularFunction::number_of_values(shape);
    SequenceValues sv(num_values);
    // Transfer the data from the array variables held in the vector of
    // Arrays into the Sequence using the SequenceValues object.
    // sv is a value-result parameter
    TabularFunction::build_sequence_values(the_arrays, sv);

    response->set_value(sv);
    response->set_read_p(true);

    *btpp = response.release();
    return;
}

void
TabularFunction::function_dap2_tabular_2(int argc, BaseType *argv[], DDS &, BaseType **btpp)
{
    vector<Array*> the_arrays;
    // collect all of the arrays; separates them from other kinds of parameters
    for (int n = 0; n < argc; ++n) {
        if (argv[n]->type() != dods_array_c)
            throw Error("In function tabular(): Expected an array, but argument " + argv[n]->name() + " is a " + argv[n]->type_name() + ".");
        the_arrays.push_back(static_cast<Array*>(argv[n]));
    }

    // every var with dimension == min_dim_size is considered an 'independent' var
    unsigned long min_dim_size = ULONG_MAX;   // <climits>
    for (vector<Array*>::iterator i = the_arrays.begin(), e = the_arrays.end(); i != e; ++i) {
        min_dim_size = min((unsigned long)(*i)->dimensions(), min_dim_size);
    }

    // collect the independent and dependent variables; size _and_ shape must match
    vector<Array*> indep, dep;
    for (vector<Array*>::iterator i = the_arrays.begin(), e = the_arrays.end(); i != e; ++i) {
        if ((*i)->dimensions() == min_dim_size) {
            indep.push_back(*i);
        }
        else {
            dep.push_back(*i);
        }
    }

    vector<unsigned long> indep_shape = array_shape(indep.at(0));
    vector<unsigned long> dep_shape = array_shape(dep.at(0));

    // FIXME Test shapes here

    unsigned long num_indep_values = number_of_values(indep_shape);
    SequenceValues indep_sv(num_indep_values);

    read_values(indep);
    build_sequence_values(indep, indep_sv);

    unsigned long num_dep_values = number_of_values(dep_shape);
    SequenceValues dep_sv(num_dep_values);

    read_values(dep);
    build_sequence_values(dep, dep_sv);

    auto_ptr<TabularSequence> response(new TabularSequence("table"));
    // combine the two SequenceValue tables and set the response to the result

#if 0
    DBG(cerr << "the_arrays.size(): " << the_arrays.size() << endl);

    // Now build the response Sequence so it has columns that match the
    // Array element types
    for (unsigned long n = 0; n < the_arrays.size(); ++n) {
        response->add_var(the_arrays[n]->var());
    }

    unsigned long num_values = TabularFunction::number_of_values(shape);
    SequenceValues sv(num_values);
    // Transfer the data from the array variables held in the vector of
    // Arrays into the Sequence using the SequenceValues object.
    // sv is a value-result parameter
    TabularFunction::build_sequence_values(the_arrays, sv);

    response->set_value(sv);
    response->set_read_p(true);

    *btpp = response.release();
    return;
#endif
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
BaseType *TabularFunction::function_dap4_tabular(D4RValueList *args, DMR &dmr)
{
    // unique_ptr is not avialable on gcc 4.2. jhrg 2/11/15
    //unique_ptr<D4Sequence> response(new D4Sequence("table"));
    auto_ptr<D4Sequence> response(new D4Sequence("table"));

    int num_arrays = args->size();              // Might pass in other stuff...
    vector<unsigned long> shape;            // Holds shape info; used to test array sizes for uniformity
    vector<Array*>the_arrays(num_arrays);

    for (int n = 0; n < num_arrays; ++n) {
        TabularFunction::build_columns(n, args->get_rvalue(n)->value(dmr), the_arrays, shape);
    }

    DBG(cerr << "the_arrays.size(): " << the_arrays.size() << endl);

    for (unsigned long n = 0; n < the_arrays.size(); ++n) {
        response->add_var(the_arrays[n]->var());
    }

    unsigned long num_values = TabularFunction::number_of_values(shape);
    D4SeqValues sv(num_values);
    // sv is a value-result parameter
    TabularFunction::build_sequence_values(the_arrays, sv);

    response->set_value(sv);
    response->set_read_p(true);

    return response.release();
}

} // namesspace libdap
