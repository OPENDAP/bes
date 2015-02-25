
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
#include <Int32.h>
#include <Str.h>
#include <Array.h>
#include <Structure.h>

#include <D4RValue.h>
#include <Error.h>
#include <debug.h>
#include <util.h>
#include <ServerFunctionsList.h>

#include "BBoxFunction.h"

using namespace std;

namespace libdap {

#if 0
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
#endif

/**
 * @brief Return the bounding box for an array
 *
 * Given an N-dimensional Array on simple types and a set of
 * minimum and maximum values, return the indices of a N-dimensional
 * bounding box. The indices are returned using an Array of
 * Structure, where each element of the array holds the name,
 * minimum index and maximum index in fields with those names.
 *
 * It is up to the caller to make use of the returned values; the
 * array is not modified in any way other than to read in it's
 * values (and set the variable's read_p property).
 *
 * @note There are both DAP2 and DAP4 versions of this function.
 *
 * @param argc Argument count
 * @param argv Argument vector - variable in the current DDS
 * @param dds The current DDS
 * @param btpp Value-result parameter for the resulting Array of Structure
 */
void
function_dap2_bbox(int argc, BaseType *argv[], DDS &, BaseType **btpp)
{
    // Build the Structure and load it with the needed fields. The
    // Array instances will have the same fields, but each instance
    // will also be loaded with values.
    Structure *proto = new Structure("bbox");
    proto->add_var_nocopy(new Int32("start"));
    proto->add_var_nocopy(new Int32("stop"));
    proto->add_var_nocopy(new Str("name"));
    // Using auto_ptr and not unique_ptr because of OS/X 10.7. jhrg 2/24/15
    auto_ptr<Array> response(new Array("bbox", proto));

    switch (argc) {
    case 0:
        throw Error("No help yet");
    case 3:
        // correct number of args
        break;
    default:
        throw Error(malformed_expr, "Wrong number of args to bbox()");
    }

    if (argv[0] && argv[0]->type() != dods_array_c)
        throw Error("In function bbox(): Expected argument 1 to be an Array.");
    if (!argv[0]->var()->is_simple_type() || argv[0]->var()->type() == dods_str_c || argv[0]->var()->type() == dods_url_c)
        throw Error("In function bbox(): Expected argument 1 to be an Array of numeric types.");

    // cast is safe given the above
    Array *the_array = static_cast<Array*>(argv[0]);

    // Read the variable into memory
    the_array->read();
    the_array->set_read_p(true);

    // Get the values as doubles
    vector<double> the_values;
    extract_double_array(the_array, the_values); // This function sets the size of dest

    double min_value = extract_double_value(argv[1]);
    double max_value = extract_double_value(argv[2]);

    // Before loading the values, set the length of the response array
    // This is a one-dimensional array (vector) with a set of start, stop
    // and name tuples for each dimension of the first argument
    response->append_dim(the_array->dimensions(), "indices");

    int i = 0;
    for(Array::Dim_iter di = the_array->dim_begin(), de = the_array->dim_end(); di != de; ++di) {
        Structure *slice = new Structure("slice");

        Int32 *start = new Int32("start");
        // FIXME hack code for now to see end-to-end operation. jhrg 2/25/15
        start->set_value(10);
        slice->add_var_nocopy(start);

        Int32 *stop = new Int32("stop");
        stop->set_value(20);
        slice->add_var_nocopy(stop);

        Str *name = new Str("name");
        name->set_value(the_array->dimension_name(di));
        slice->add_var_nocopy(name);

        slice->set_read_p(true);        // Sets all children too, as does set_send_p()
        slice->set_send_p(true);        // Not sure this is needed, but it cannot hurt

        response->set_vec_nocopy(i++, slice);
    }

    response->set_length(i);
    response->set_read_p(true);
    response->set_read_p(true);

    *btpp = response.release();
    return;
}

/**
 * @brief Return the bounding box for an array
 *
 * @note The main difference between this function and the DAP2
 * version is to use args->size() in place of argc and
 * args->get_rvalue(n)->value(dmr) in place of argv[n].
 *
 * @see function_dap2_bbox
 */
BaseType *function_dap4_bbox(D4RValueList *args, DMR &)
{
    auto_ptr<Array> response(new Array("bbox", new Structure("bbox")));

    throw Error(malformed_expr, "Not yet implemented for DAP4 functions.");

    return response.release();
}

} // namesspace libdap
