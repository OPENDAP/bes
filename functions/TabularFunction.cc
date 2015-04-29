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
#include <UInt32.h>
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
using namespace libdap;

namespace functions {

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
TabularFunction::Shape TabularFunction::array_shape(Array *a)
{
    Shape shape;

    for (Array::Dim_iter i = a->dim_begin(), e = a->dim_end(); i != e; ++i) {
        shape.push_back(a->dimension_size(i));
    }

    return shape;
}

/**
 * Does the given array match the shape of the vector? The vector
 * 'shape' is built at the start of tabular's run.
 *
 * @param a Test this array
 * @param shape The reference shape information
 * @return True if the shapes match, False otherwise.
 */
bool TabularFunction::shape_matches(Array *a, const Shape &shape)
{
    // Same number of dims
    if (shape.size() != a->dimensions()) return false;

    // Each dim the same size
    Array::Dim_iter i = a->dim_begin(), e = a->dim_end();
    Shape::const_iterator si = shape.begin(), se = shape.end();
    while (i != e && si != se) {
        assert(a->dimension_size(i) >= 0);
        if (*si != (unsigned long) a->dimension_size(i)) return false;
        ++i;
        ++si;
    }

    return true;
}

/**
 * Compare the shape of the dependent variables to the independent
 * variables' shape. Do they meet the requirements of this function?
 * The independent variables' shape must match that of the right-most
 * N dimensions of the dependent variables given that those have rank
 * N + 1. This also means that the dependent variables may have at most
 * one extra dimension in addition to those of the independent variables.
 *
 * @param dep_shape
 * @param indep_shape
 * @return True if the two groups of variables can be processed by this
 * function, false otherwise.
 */
bool TabularFunction::dep_indep_match(const Shape &dep_shape, const Shape &indep_shape)
{
    // Each of the indep vars dims must match the corresponding dep var dims
    // Start the comparison with the right-most dims (rbegin())
    Shape::const_reverse_iterator di = dep_shape.rbegin();
    for (Shape::const_reverse_iterator i = indep_shape.rbegin(), e = indep_shape.rend(); i != e; ++i) {
        assert(di != dep_shape.rend());
        if (*i != *di++) return false;
    }

    return true;
}

/**
 * Given a shape vector, how many elements are there in an Array that
 * matches those dimension sizes?
 *
 * @param shape The Array shape
 * @return The number of elements
 */
unsigned long TabularFunction::number_of_values(const Shape &shape)
{
    unsigned long size = 1;
    Shape::const_iterator si = shape.begin(), se = shape.end();
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
void TabularFunction::build_columns(unsigned long n, BaseType* btp, vector<Array*>& the_arrays,
        Shape &shape)
{
    if (btp->type() != dods_array_c)
        throw Error("In tabular(): Expected argument '" + btp->name() + "' to be an Array.");

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
 * Note that the vector<> is const but the pointers it contains
 * reference objects that are modified.
 */
void TabularFunction::read_values(const vector<Array*> &arrays)
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
 * @param sv The destination object; a value-result parameter, passed
 * by reference. Note that DAP2's SequenceValues and DAP4's D4SeqValues
 * are both typedefs to a vector of vectors of BaseType pointers, so
 * both D2 and D4 objects can use this code.
 */
void TabularFunction::build_sequence_values(const vector<Array*> &the_arrays, SequenceValues &sv)
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
            (*row)[j]/*->at(j)*/= the_arrays[j]/*.at(j)*/->var(i)->ptr_duplicate();

            (*row)[j]->set_send_p(true);
            (*row)[j]->set_read_p(true);
        }

        sv[i]/*.at(i)*/= row;
    }
}

/**
 * Combine the two SequenceValues vectors to form one table. The dep
 * SequenceValues are the left hand columns and the indep are the right
 * hand ones. Because there can be more dep values than indep values
 * (but dep == indep * N, where N is and integer > 0), and because
 * std::vector append operations are faster than inserts, this code loops
 * over dep and inserts elements from indep, returning deps as the function
 * result.
 *
 * @note The notions of dependent and independent variables are somewhat
 * phony. In general, indep will hold the variables like Lat and Lon that
 * are [x][y] while dep will hold variables like temp which may be either
 * [x][y] _or_ [b][x][y]. In the former case, this code will actually
 * never be called and in the latter case the size of 'b' is N in the
 * above description.
 *
 * @param dep The 'dependent' variables; A value-result parameter
 * @param indep The 'independent' variables
 */
void TabularFunction::combine_sequence_values(SequenceValues &dep, const SequenceValues &indep)
{
    SequenceValues::const_iterator ii = indep.begin(), ie = indep.end();
    for (SequenceValues::iterator i = dep.begin(), e = dep.end(); i != e; ++i) {
        // When we get to the end of the indep variables, start over
        // This test is at the start of the loop so that we can test ii == ie on exit
        if (ii == ie) ii = indep.begin();
        // This call to insert() copies the pointers, but that will lead to duplicate
        // calls to delete when Sequence deletes the SequenceValues object. Could be
        // replaced with reference counted pointers??? jhrg 3/13/15
        // (*i)->insert((*i)->end(), (*ii)->begin(), (*ii)->end());
        for (BaseTypeRow::iterator btr_i = (*ii)->begin(), btr_e = (*ii)->end(); btr_i != btr_e; ++btr_i) {
            (*i)->push_back((*btr_i)->ptr_duplicate());
        }
        ++ii;
    }

    assert(ii == ie);
}

/**
 * Given the shape information about the independent and dependent
 * variables, add an extra variable to the dep_vars vector to hold the
 * values of the extra index.
 *
 * For example, suppose indep_shape is [2][3] and dep_shape is [5][2][3],
 * then this code would add a single integer variable to dep_vars that
 * will hold values ranging from 0 to 4 to represent the additional
 * left-most dimension in dep_shape.
 *
 * This function should only be called if the dependent and independent
 * variable shape meets the criteria outlined above. In a production build
 * (#define NDEBUG) there are no sanity checks made here.
 *
 * @note This function not only adds the new variables to the dep_vars
 * parameter, it also loads those variables with values.
 *
 * @todo Extend this to support more than one additional dimension.
 *
 * @param indep_shape The shape common to all 'independent' variables
 * @param dep_shape The shape common to all 'dependent' variables
 * @param dep_vars The value result parameter that holds the dependent
 * variables. On return this has nominally been extended by one or more
 * new variables. The new variables contain values.
 */
void TabularFunction::add_index_column(const Shape &indep_shape, const Shape &dep_shape,
        vector<Array*> &dep_vars)
{
    assert(dep_vars.size() > 0);
    assert(dep_shape.size() == indep_shape.size() + 1);

    // load a vector with values for the new variable
    unsigned long num_indep_values = number_of_values(indep_shape);
    unsigned long num_dep_values = number_of_values(dep_shape);
    vector<dods_uint32> index_vals(num_dep_values);

    assert(num_dep_values == num_indep_values * dep_shape.at(0));

    // dep_shape.at(0) == the left-most dimension size
    vector<dods_uint32>::iterator iv = index_vals.begin();
    for (Shape::size_type i = 0; i < dep_shape.at(0); ++i) {
        assert(iv != index_vals.end());
        fill(iv, iv + num_indep_values, i);
        iv += num_indep_values;
    }

    // Figure out what to call the new variable/column
    string new_column_name = dep_vars.at(0)->dimension_name(dep_vars.at(0)->dim_begin());
    if (new_column_name.empty())
        new_column_name = "index";

    // Make the new column var
    Array *a = new Array(new_column_name, new UInt32(new_column_name));
    a->append_dim(num_dep_values, new_column_name);
    a->set_value(index_vals, (int)index_vals.size());
    a->set_read_p(true);

    dep_vars.insert(dep_vars.begin(), a);
}

#if 0
/**
 * @brief Transform one or more arrays to a sequence.
 *
 * This function will transform one or more arrays into a sequence,
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
void TabularFunction::function_dap2_tabular_2(int argc, BaseType *argv[], DDS &, BaseType **btpp)
{
    // unique_ptr is not available on gcc 4.2. jhrg 2/11/15
    // unique_ptr<TabularSequence> response(new TabularSequence("table"));
    auto_ptr<TabularSequence> response(new TabularSequence("table"));

    int num_arrays = argc;              // Might pass in other stuff...
    Shape shape;            // Holds shape info; used to test array sizes for uniformity
    vector<Array*> the_arrays(num_arrays);

    // Read each array passed to tabular(), check that its shape matches
    // the first array's shape, store the array in a vector and read in
    // it's values (into the Array object's internal store).
    for (int n = 0; n < num_arrays; ++n) {
        build_columns(n, argv[n], the_arrays, shape);
    }

    DBG(cerr << "the_arrays.size(): " << the_arrays.size() << endl);

    // Now build the response Sequence so it has columns that match the
    // Array element types
    for (unsigned long n = 0; n < the_arrays.size(); ++n) {
        response->add_var(the_arrays[n]->var());
    }

    unsigned long num_values = number_of_values(shape);
    SequenceValues sv(num_values);
    // Transfer the data from the array variables held in the vector of
    // Arrays into the Sequence using the SequenceValues object.
    // sv is a value-result parameter
    build_sequence_values(the_arrays, sv);

    response->set_value(sv);
    response->set_read_p(true);

    *btpp = response.release();
    return;
}
#endif

/**
 * @brief Transform one or more arrays to a sequence.
 *
 * This function will transform one or more arrays into a sequence,
 * where each array becomes a column in the sequence, with one
 * exception. If each array has the same shape, then the number of
 * columns in the resulting table is the same as the number of arrays.
 * If one or more arrays has more dimensions than the others, an
 * extra column is added for each of those extra dimensions. Arrays
 * are enumerated in row-major order (the right-most dimension varies
 * fastest).
 *
 * It's assumed that for each of the arrays, elements (i0, i1, ..., in)
 * are all related. The function makes no test to ensure that, however.
 *
 * @note While this version of tabular() will work when some arrays
 * have more dimensions than others, the collection of arrays must have
 * shapes that 'fit together'.
 *
 * @todo Write better documentation about the differing dimensions
 *
 * @param argc Argument count
 * @param argv Argument vector - variable in the current DDS
 * @param dds The current DDS
 * @param btpp Value-result parameter for the resulting Sequence
 */
void TabularFunction::function_dap2_tabular(int argc, BaseType *argv[], DDS &, BaseType **btpp)
{
    vector<Array*> the_arrays;
    // collect all of the arrays; separates them from other kinds of parameters
    for (int n = 0; n < argc; ++n) {
        if (argv[n]->type() != dods_array_c)
            throw Error("In function tabular(): Expected an array, but argument " + argv[n]->name()
                    + " is a " + argv[n]->type_name() + ".");
        the_arrays.push_back(static_cast<Array*>(argv[n]));
    }

    if (the_arrays.size() < 1)
        throw Error("In function tabular(): Expected at least one Array variable.");

    // every var with dimension == min_dim_size is considered an 'independent' var
    unsigned long min_dim_size = ULONG_MAX;   // <climits>
    for (vector<Array*>::iterator i = the_arrays.begin(), e = the_arrays.end(); i != e; ++i) {
        min_dim_size = min((unsigned long) (*i)->dimensions(), min_dim_size);
    }

    // collect the independent and dependent variables; size _and_ shape must match
    vector<Array*> indep_vars, dep_vars;
    for (vector<Array*>::iterator i = the_arrays.begin(), e = the_arrays.end(); i != e; ++i) {
        if ((*i)->dimensions() == min_dim_size) {
            indep_vars.push_back(*i);
        }
        else {
            dep_vars.push_back(*i);
        }
    }

    Shape indep_shape = array_shape(indep_vars.at(0));
    // Test that all the indep arrays have the same shape
    for (vector<Array*>::iterator i = indep_vars.begin()+1, e = indep_vars.end(); i != e; ++i) {
        if (!shape_matches(*i, indep_shape))
            throw Error("In function tabular(): Expected all of the 'independent' variables to have the same shape.");
    }

    // Read the values and load them into a SequenceValues object
    read_values(indep_vars);
    unsigned long num_indep_values = number_of_values(indep_shape);
    SequenceValues indep_sv(num_indep_values);
    build_sequence_values(indep_vars, indep_sv);

    // Set the reference to the result. If there are any dependent variables,
    // 'result' will be set to 'dep_vars' once that has been hasked and the
    // indep_vars merged in.
    SequenceValues &result = indep_sv;

    // If there are dependent variables, process them
    if (dep_vars.size() > 0) {
        Shape dep_shape = array_shape(dep_vars.at(0));
        // Test that all the dep arrays have the same shape
        for (vector<Array*>::iterator i = dep_vars.begin()+1, e = dep_vars.end(); i != e; ++i) {
            if (!shape_matches(*i, dep_shape))
                throw Error("In function tabular(): Expected all of the 'dependent' variables to have the same shape.");
        }

        // Test shapes here. My code assumes that deps are like dep_vars[7][x][y]
        // and indep_vars are [x][y] - the left-most dim is the 'extra' parameter
        // of the dep_vars.
        if (dep_shape.size() > indep_shape.size() + 1)
            throw Error("In function tabular(): The rank of the dependent variables may be at most one more than the rank of the independent variables");
        if (dep_shape.size() < indep_shape.size())
            throw Error("In function tabular(): The rank of the dependent variables cannot be less than the rank of the independent variables");

        if (!dep_indep_match(dep_shape, indep_shape))
            throw Error("In function tabular(): The 'independent' array shapes must match the right-most dimensions of the 'dependent' variables.");

        read_values(dep_vars);
        unsigned long num_dep_values = number_of_values(dep_shape);
        SequenceValues dep_sv(num_dep_values);

        // Add and extra variable for extra dimension's index
        add_index_column(indep_shape, dep_shape, dep_vars);

        build_sequence_values(dep_vars, dep_sv);

        // Now combine the dependent and independent variables; put the
        // result in the dependent variable vector and assign the 'result'
        // reference to it.
        combine_sequence_values(dep_sv, indep_sv);
        result = dep_sv;
    }

    auto_ptr<TabularSequence> response(new TabularSequence("table"));

    if (dep_vars.size() > 0) {
        // set the columns of the response
        for (SequenceValues::size_type n = 0; n < dep_vars.size(); ++n) {
            response->add_var(dep_vars[n]->var());
        }
    }

    for (SequenceValues::size_type n = 0; n < indep_vars.size(); ++n) {
        response->add_var(indep_vars[n]->var());
    }

    // set the values of the response
    response->set_value(result);
    response->set_read_p(true);

    *btpp = response.release();
    return;
}

#if 0

// Rework this as time permits. jhrg 3/12/15

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
    vector<Array*> the_arrays(num_arrays);

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
#endif

} // namesspace functions
