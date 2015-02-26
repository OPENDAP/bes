
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

#include "RoiFunction.h"

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
 * Test for acceptable array types for the N-1 arguments of roi().
 * Throw Error if the array is not valid for this function
 *
 * @param btp Test this variable.
 * @exception Error thrown if the array is not valid
 */
static void check_number_type_array(BaseType *btp, unsigned int rank)
{
    if (!btp)
        throw InternalErr(__FILE__, __LINE__, "roi() function called with null variable.");

    if (btp->type() != dods_array_c)
        throw Error("In function roi(): Expected argument '" + btp->name() + "' to be an Array.");

    Array *a = static_cast<Array *>(btp);
    if (!a->var()->is_simple_type() || a->var()->type() == dods_str_c || a->var()->type() == dods_url_c)
        throw Error("In function roi(): Expected argument '" + btp->name() + "' to be an Array of numeric types.");

    if (a->dimensions() < rank)
        throw Error("In function roi(): Expected the array '" + a->name() +"' to be rank " + long_to_string(rank) + " or greater.");
}

static void check_valid_slice(BaseType *btp)
{
    // we know it's a Structure * and it has one element because the test above passed
    if (btp->type() != dods_structure_c)
        throw Error("In function roi(): Expected an Array of Structures for the slice information.");

    Structure *slice = static_cast<Structure*>(btp);

    Constructor::Vars_iter i = slice->var_begin();
    if (i == slice->var_end() || (*i)->name() != "start" || (*i)->type() != dods_int32_c)
        throw Error("In function roi(): Could not find valid 'start' field in slice information");

    ++i;
    if (i == slice->var_end() || (*i)->name() != "stop" || (*i)->type() != dods_int32_c)
        throw Error("In function roi(): Could not find valid 'stop' field in slice information");

    ++i;
    if (i == slice->var_end() || (*i)->name() != "name" || (*i)->type() != dods_str_c)
        throw Error("In function roi(): Could not find valid 'name' field in slice information");
}

/**
 * Is the slice array (an array of structures) correct? Throw Error
 * if not.
 *
 * @param btp Pointer to the Array of Structure that holds the slice information
 * @return The number of slices in the slice array
 * @exception Error Thrown if the array si not valid.
 */
static unsigned int valid_slice_array(BaseType *btp)
{
    if (!btp)
        throw InternalErr(__FILE__, __LINE__, "roi() function called with null slice array.");

    if (btp->type() != dods_array_c)
        throw Error("In function roi(): Expected last argument to be an Array of slices.");

    Array *slices = static_cast<Array*>(btp);
    if (slices->dimensions() != 1)
        throw Error("In function roi(): Expected last argument to be a one dimensional Array of slices.");

    int rank = slices->dimension_size(slices->dim_begin());
    for (int i = 0; i < rank; ++i) {
        check_valid_slice(slices->var(i));
    }

    return rank;
}

/**
 * This method extracts values from one element of the slices Array of Structures.
 * It assumes that the input has been validated.
 *
 * @param slices
 * @param i
 * @param start
 * @param stop
 * @param name
 */
static void get_slice_data(Array *slices, unsigned int i, int &start, int &stop, string &name)
{
    BaseType *btp = slices->var(i);

    Structure *slice = static_cast<Structure*>(btp);
    Constructor::Vars_iter vi = slice->var_begin();

    start = static_cast<Int32*>(*vi++)->value();
    stop = static_cast<Int32*>(*vi++)->value();
    name = static_cast<Str*>(*vi++)->value();
}

/**
 * @brief Subset the N arrays using index slicing information
 *
 *
 * @param argc Argument count
 * @param argv Argument vector - variable in the current DDS
 * @param dds The current DDS
 * @param btpp Value-result parameter for the resulting Array of Structure
 */
void
function_dap2_roi(int argc, BaseType *argv[], DDS &, BaseType **btpp)
{
    auto_ptr<Structure> response(new Structure("Arrays"));

    // This is the rank of the Array of Slices, not the N-1 arrays to be sliced
    int rank = 0;

    switch (argc) {
    case 0:
    case 1:
        // Must have 2 or more arguments
        throw Error("No help yet");
    default:
        rank = valid_slice_array(argv[argc-1]); // throws if slice is not valid

        for (int i = 0; i < argc-1; ++i)
            check_number_type_array(argv[i], rank);      // throws if array is not valid
        break;
    }

    Array *slices = static_cast<Array*>(argv[argc-1]);

    for (int i = 0; i < argc-1; ++i) {
        // cast is safe given the above
        Array *the_array = static_cast<Array*>(argv[i]);

        // foreach dimension of the array, apply the slice constraint.
        // Assume Array[]...[][X][Y] where the slice has dims X and Y
        // So find the last <rank> dimensions and check that their names
        // match those of the slice (optionally ignore the name check?)
        unsigned int num_dims = the_array->dimensions();
        int d = num_dims-1;
        for (int i = rank-1; i >= 0; --i) {
            int start, stop;
            string name;
            // start, stop, name are value-result parameters
            get_slice_data(slices, i, start, stop, name);

            // Hack, should use reverse iterators, but Array does not have them
            Array::Dim_iter iter = the_array->dim_begin() + d;

            if (the_array->dimension_name(iter) != name)
                throw Error("In function roi(): Dimension name (" + the_array->dimension_name(iter) + ") and slice name (" + name + ") don't match");

            // TODO Add stride option?
            the_array->add_constraint(iter, start, 1 /*stride*/, stop);
            --d;
        }

        // Add the array to the Structure returned by the function
        the_array->set_send_p(true);    // include it
        the_array->set_read_p(false);   // re-read the values
        response->add_var(the_array);
    }

    response->set_send_p(true);

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
BaseType *function_dap4_roi(D4RValueList *, DMR &)
{
    throw Error(malformed_expr, "Not yet implemented for DAP4 functions.");

    return 0;
}

} // namesspace libdap
