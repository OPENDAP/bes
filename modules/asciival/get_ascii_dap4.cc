
// -*- mode: c++; c-basic-offset:4 -*-

// Copyright (c) 2006 OPeNDAP, Inc.
// Author: James Gallagher <jgallagher@opendap.org>
//
// This is free software; you can redistribute it and/or modify it under the
// terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 2.1 of the License, or (at your
// option) any later version.
//
// This is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for
// more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.

// This file holds the interface for the 'get data as ascii' function of the
// OPeNDAP/HAO data server. This function is called by the BES when it loads
// this as a module. The functions in the file ascii_val.cc also use this, so
// the same basic processing software can be used both by Hyrax and tie older
// Server3.

#include <iostream>
#include <sstream>
#include <iomanip>

#include <DMR.h>
#include <BaseType.h>
#include <Structure.h>
#include <Array.h>
#include <D4Sequence.h>
#include <D4Enum.h>
#include <D4Opaque.h>
#include <D4Group.h>
#include <crc.h>
#include "InternalErr.h"

#include "get_ascii_dap4.h"

namespace dap_asciival {

using namespace libdap;
using namespace std;

// most of the code here defines functions before they are used; these three
// need to be declared.
static void print_values_as_ascii(BaseType *btp, bool print_name, ostream &strm, Crc32 &checksum);
static void print_sequence_header(D4Sequence *s, ostream &strm);
static void print_val_by_rows(D4Sequence *seq, ostream &strm, Crc32 &checksum);

/**
 * For an Array that holds a vector of scalar values, print it on one
 * line. This code uses Vector::var(<index>) to access the values, so
 * the only extra space required is for a single element. However, in
 * iterating over the entire vector, every value is copied.
 *
 * @param a The Array
 * @param strm The output sink
 * @param print_name Should the FQN be printed?
 */
static void print_array_vector(Array *a, ostream &strm, bool print_name)
{
    if (print_name)
        strm << a->FQN() << ", " ;

    // only one dimension
    // Added to support zero-length arrays. jhrg 2/2/16
    if (a->dimension_size(a->dim_begin(), true) > 0) {
        int end = a->dimension_size(a->dim_begin(), true) - 1;

        for (int i = 0; i < end; ++i) {
            a->var(i)->print_val(strm, "", false /*print_decl*/);
            strm << ", ";
        }
        a->var(end)->print_val(strm, "", false /*print_decl*/);
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
static int print_array_row(Array *a, ostream &strm, int index, int number)
{
    // Added to support zero-length arrays. jhrg 2/2/16
    if (number > 0) {
        for (int i = 0; i < number; ++i) {
            a->var(index++)->print_val(strm, "", false /*print_decl*/);
            strm << ", ";
        }

        a->var(index++)->print_val(strm, "", false /*print_decl*/);
    }
    return index;
}

// This code implements simple modulo arithmetic. The vector shape contains
// This code implements simple modulo arithmetic. The vector shape contains
// the maximum count value for each dimension, state contains the current
// state. For example, if shape holds 10, 20 then when state holds 0, 20
// calling this method will increment state to 1, 0. For this example,
// calling the method with state equal to 10, 20 will reset state to 0, 0 and
// the return value will be false.
static bool increment_state(vector<int> *state, const vector<int> &shape)
{
    vector < int >::reverse_iterator state_riter;
    vector < int >::const_reverse_iterator shape_riter;
    for (state_riter = state->rbegin(), shape_riter = shape.rbegin();
         state_riter < state->rend(); state_riter++, shape_riter++) {
        if (*state_riter == *shape_riter - 1) {
            *state_riter = 0;
        }
        else {
            *state_riter = *state_riter + 1;
            return true;
        }
    }

    return false;
}

static vector <int> get_shape_vector(Array *a, size_t n)
{
    if (n < 1 || n > a->dimensions(true)) {
    	ostringstream oss;
    	oss << "Attempt to get " << n << " dimensions from " << a->name() << " which has " <<  a->dimensions(true)  << " dimensions";
        throw InternalErr(__FILE__, __LINE__, oss.str());
    }

    vector <int>shape;
    Array::Dim_iter p = a->dim_begin();
    for (unsigned i = 0; i < n && p != a->dim_end(); ++i, ++p) {
        shape.push_back(a->dimension_size(p, true));
    }

    return shape;
}

/** Get the size of the Nth dimension. The first dimension is N == 0.
    @param n The index. Uses sero-based indexing.
    @return the size of the dimension. */
static int get_nth_dim_size(Array *a, size_t n)
{
    if (n > a->dimensions(true) - 1) {
    	ostringstream oss;
    	oss << "Attempt to get dimension " << n << " from " << a->name() << " which has " <<  a->dimensions(true)  << " dimensions";
        throw InternalErr(__FILE__, __LINE__, oss.str());
    }

    return a->dimension_size(a->dim_begin() + n, true);
}

static void print_ndim_array(Array *a, ostream &strm, bool /*print_name */ )
{

    int dims = a->dimensions(true);
    if (dims <= 1)
        throw InternalErr(__FILE__, __LINE__, "Dimension count is <= 1 while printing multidimensional array.");

    // shape holds the maximum index value of all but the last dimension of
    // the array (not the size; each value is one less than the size).
    vector<int> shape = get_shape_vector(a, dims - 1);
    int rightmost_dim_size = get_nth_dim_size(a, dims - 1);

    // state holds the indexes of the current row being printed. For an N-dim
    // array, there are N-1 dims that are iterated over when printing (the
    // Nth dim is not printed explicitly. Instead it's the number of values
    // on the row.
    vector<int> state(dims - 1, 0);

    bool more_indices;
    int index = 0;
    do {
        // Print indices for all dimensions except the last one.
    	strm << a->FQN();

        for (int i = 0; i < dims - 1; ++i) {
            strm << "[" << state[i] << "]" ;
        }
        strm << ", " ;

        index = print_array_row(a, strm, index, rightmost_dim_size - 1);
        more_indices = increment_state(&state, shape);
        if (more_indices)
            strm << endl ;

    } while (more_indices);
}

static int get_index(Array *a, vector<int> indices)
{
    if (indices.size() != a->dimensions(true))
        throw InternalErr(__FILE__, __LINE__,  "Index vector is the wrong size!");

    // suppose shape is [3][4][5][6] for x,y,z,t. The index is
    // t + z(6) + y(5 * 6) + x(4 * 5 *6).
    // Assume that indices[0] holds x, indices[1] holds y, ...

    vector < int >shape = get_shape_vector(a, indices.size());

    // We want to work from the rightmost index to the left
    reverse(indices.begin(), indices.end());
    reverse(shape.begin(), shape.end());

    vector<int>::iterator indices_iter = indices.begin();
    vector<int>::iterator shape_iter = shape.begin();

    int index = *indices_iter++;        // in the ex. above, this adds `t'
    int multiplier = 1;
    while (indices_iter != indices.end()) {
        multiplier *= *shape_iter++;
        index += multiplier * *indices_iter++;
    }

    return index;
}

/**
 * This prints Arrays that contain Structures and Sequences. Arrays of simple
 * types (scalars) are printed using the print_array_vector() and print_ndim_array().
 *
 * @param a The Array to print
 * @param strm
 * @param print_name
 * @param checksum
 */
static void print_complex_array(Array *a, ostream &strm, bool print_name, Crc32 &checksum)
{
    int dims = a->dimensions(true);
    if (dims < 1)
        throw InternalErr(__FILE__, __LINE__, "Dimension count is <= 1 while printing multidimensional array.");

    // shape holds the maximum index value of all but the last dimension of
    // the array (not the size; each value is one less that the size).
    vector<int> shape = get_shape_vector(a, dims);

    vector<int> state(dims, 0);

    bool more_indices;
    do {
        // Print indices for all dimensions except the last one.
        strm << a->FQN();

        for (int i = 0; i < dims; ++i) {
            strm << "[" << state[i] << "]" ;
        }
        strm << endl;

    	print_values_as_ascii(a->var(get_index(a, state)), print_name, strm, checksum);

        more_indices = increment_state(&state, shape);

        if (more_indices)
            strm << endl;

    } while (more_indices);
}

/**
 * Print an array. Based on the prototype and number of dimensions,
 * choose a print function.
 *
 * @param a
 * @param print_name
 * @param strm
 * @param checksum
 */
static void print_values_as_ascii(Array *a, bool print_name, ostream &strm, Crc32 &checksum)
{
    if (a->var()->is_simple_type()) {
        if (a->dimensions(true) > 1) {
            print_ndim_array(a, strm, print_name);
        }
        else {
            print_array_vector(a, strm, print_name);
        }
    }
    else {
    	print_complex_array(a, strm, print_name, checksum);
    }
}

static void print_structure_header(Structure *s, ostream &strm)
{
	Constructor::Vars_iter p = s->var_begin(), e = s->var_end();
	bool needs_comma = false;
	while (p != e) {
	    if((*p)->send_p()){
            if ((*p)->is_simple_type())
                strm << (needs_comma?", ":"") <<  (*p)->FQN();
            else if ((*p)->type() == dods_structure_c)
                print_structure_header(static_cast<Structure*>(*p), strm);
            else if ((*p)->type() == dods_sequence_c)
                print_sequence_header(static_cast<D4Sequence*>(*p), strm);
            else
                throw InternalErr(__FILE__, __LINE__, "Unknown or unsupported type.");
            needs_comma = true;
	    }
		++p;
	}
}

static void print_structure_ascii(Structure *s, ostream &strm, bool print_name, Crc32 &checksum)
{
	if (s->is_linear()) {
		if (print_name) {
			print_structure_header(s, strm);
			strm << endl;
		}

		Constructor::Vars_iter p = s->var_begin(), e = s->var_end();
		while (p !=e) {
			// bug: print_name should be false, but will be true because it's not a param here
			if ((*p)->send_p()) print_values_as_ascii(*p, false /*print_name*/, strm, checksum);

			if (++p != e) strm << ", ";
		}
	}
	else {
		for (Constructor::Vars_iter p = s->var_begin(), e = s->var_end(); p != e; ++p) {
			if ((*p)->send_p()) {
				print_values_as_ascii(*p, print_name, strm, checksum);
				// This line outputs an extra endl when print_ascii is called for
				// nested structures because an endl is written for each member
				// and then once for the structure itself. 9/14/2001 jhrg
				strm << endl;
			}
		}
	}
}

static void print_values_as_ascii(Structure *v, bool print_name, ostream &strm, Crc32 &checksum)
{
	print_structure_ascii(v, strm, print_name, checksum);
}

static void print_one_row(D4Sequence *seq, ostream &strm, Crc32 &checksum, int row)
{
	int elements = seq->element_count();
	int j = 0;
	BaseType *btp = 0;
	bool first_val = true;

	while (j < elements) {
		btp = seq->var_value(row, j++);
		if (btp) {  // data
			if (!first_val)
				strm << ", ";
			first_val = false;
			if (btp->type() == dods_sequence_c)
				print_val_by_rows(static_cast<D4Sequence*>(btp), strm, checksum);
			else
				print_values_as_ascii(btp, false, strm, checksum);
		}
	}
}

static void print_val_by_rows(D4Sequence *seq, ostream &strm, Crc32 &checksum)
{
	if (seq->length() != 0) {
		int rows = seq->length() /*- 1*/;	// -1 because the last row is treated specially
		for (int i = 0; i < rows; ++i) {
			print_one_row(seq, strm, checksum, i);
			strm << endl;
		}
	}
}

static void print_sequence_header(D4Sequence *s, ostream &strm)
{
	Constructor::Vars_iter p = s->var_begin(), e = s->var_end();
	bool needs_comma = false;
	while (p != e) {
	    if((*p)->send_p()){
            if((*p)->is_simple_type())
                strm  << (needs_comma?", ":"") << (*p)->FQN();
            else if ((*p)->type() == dods_structure_c)
                print_structure_header(static_cast<Structure*>((*p)), strm);
            else if ((*p)->type() == dods_sequence_c)
                print_sequence_header(static_cast<D4Sequence*>((*p)), strm);
            else
                throw InternalErr(__FILE__, __LINE__, "Unknown or unsupported type.");

            needs_comma = true;
	    }
		++p;
	}
}


static void print_values_as_ascii(D4Sequence *v, bool print_name, ostream &strm, Crc32 &checksum)
{
	if (print_name) {
		print_sequence_header(v, strm);
		strm << endl;
	}

	print_val_by_rows(v, strm, checksum);
}

static void print_values_as_ascii(D4Opaque *v, bool print_name, ostream &strm, Crc32 &/*checksum*/)
{
	if (print_name)
		strm << v->FQN() << ", ";
	strm << v->value().size() << " bytes" << endl;
}

static void print_values_as_ascii(D4Group *group, bool print_name, ostream &strm, Crc32 &checksum)
{
    for (D4Group::groupsIter g = group->grp_begin(), e = group->grp_end(); g != e; ++g)
    	print_values_as_ascii(*g, print_name, strm, checksum);

    // Specialize how the top-level variables in any Group are sent; include
    // a checksum for them. A subset operation might make an interior set of
    // variables, but the parent structure will still be present and the checksum
    // will be computed for that structure. In other words, DAP4 does not try
    // to sort out which variables are the 'real' top-level variables and instead
    // simply computes the CRC for whatever appears as a variable in the root
    // group.
	for (Constructor::Vars_iter i = group->var_begin(), e = group->var_end(); i != e; ++i) {
		// Only send the stuff in the current subset.
		if ((*i)->send_p()) {
			(*i)->intern_data();

			// print the data
			print_values_as_ascii((*i), print_name, strm, checksum);
			strm << endl;
		}
	}
}

/**
 * This is the main function; it controls delegation to all the other functions.
 *
 * @param btp Print this variable
 * @param print_name Print the variable's name, then a comma, then the values if true,
 * otherwise just print the values
 * @param strm Print to this stream
 * @param checksum Use this to hold the state of the checksum computation
 */
static void print_values_as_ascii(BaseType *btp, bool print_name, ostream &strm, Crc32 &checksum)
{
    switch (btp->type()) {
    case dods_null_c:
    	throw InternalErr(__FILE__, __LINE__, "Unknown type");

    case dods_byte_c:
    case dods_char_c:

    case dods_int8_c:
    case dods_uint8_c:

    case dods_int16_c:
    case dods_uint16_c:
    case dods_int32_c:
    case dods_uint32_c:

    case dods_int64_c:
    case dods_uint64_c:

    case dods_float32_c:
    case dods_float64_c:
    case dods_str_c:
    case dods_url_c:
    case dods_enum_c:
    	if (print_name) strm << btp->FQN() << ", ";
        btp->print_val(strm, "" /*leading space*/, false /*print dap2 decl*/);
        break;

    case dods_opaque_c:
    	print_values_as_ascii(static_cast<D4Opaque*>(btp), print_name, strm, checksum);
    	break;

    case dods_array_c:
    	print_values_as_ascii(static_cast<Array*>(btp), print_name, strm, checksum);
    	break;

    case dods_structure_c:
    	print_values_as_ascii(static_cast<Structure*>(btp), print_name, strm, checksum);
    	break;

    case dods_sequence_c:
    	print_values_as_ascii(static_cast<D4Sequence*>(btp), print_name, strm, checksum);
    	break;

    case dods_group_c:
    	print_values_as_ascii(static_cast<D4Group*>(btp), print_name, strm, checksum);
    	break;

    case dods_grid_c:
    default:
    	throw InternalErr(__FILE__, __LINE__, "Unsupported type");
    }
}

/**
 * For each variable in the DMR, write out the CSV/ASCII representation
 * for it's data.
 *
 * @param dmr
 * @param strm
 */
void print_values_as_ascii(DMR *dmr, ostream &strm)
{
	Crc32 checksum;

	strm << "Dataset: " << dmr->name() << endl;

	print_values_as_ascii(dmr->root(), true /*print_name*/, strm, checksum);
}

} // namespace dap_asciival
