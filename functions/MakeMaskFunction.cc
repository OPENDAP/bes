// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of libdap, A C++ implementation of the OPeNDAP Data
// Access Protocol.

// Copyright (c) 2015 OPeNDAP, Inc.
// Authors: Dan Holloway <dholloway@opendap.org>
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
#include <algorithm>
#include <functional>

#include <Type.h>
#include <BaseType.h>
#include <Byte.h>
#include <Int16.h>
#include <UInt16.h>
#include <Int32.h>
#include <UInt32.h>
#include <Float32.h>
#include <Float64.h>
#include <Str.h>
#include <Url.h>
#include <Array.h>
#include <Grid.h>
#include <Error.h>
#include <DDS.h>

#include <DMR.h>
#include <D4Group.h>
#include <D4RValue.h>

#include <debug.h>
#include <util.h>

#include <BaseTypeFactory.h>

#include <BESDebug.h>

#include "MakeMaskFunction.h"
#include "Odometer.h"
#include "functions_util.h"

using namespace libdap;
using namespace std;

namespace functions {

string make_mask_info =
        string("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n")
                + "<function name=\"make_array\" version=\"1.0\" href=\"http://docs.opendap.org/index.php/Server_Side_Processing_Functions#make_mask\">\n"
                + "</function>";

/**
 * Scan the given map and return the first index of value
 * or -1 if the value is not found.
 * @param value
 * @param map
 * @return The index of value in map
 */
int
find_value_index(double value, const vector<double> &map)
{
    // If C++ hadn't borked passing functions to stl algorithms, we could use...
    //vector<double>::iterator loc = find_if(map.begin(), map.end(), bind2nd(ptr_fun(double_eq), value));

    for (vector<double>::const_iterator i = map.begin(), e = map.end(); i != e; ++i) {
        if (double_eq(*i, value)) {
            return i - map.begin(); // there's an official iterator diff function somewhere...
        }
    }

    return -1;
}

/**
 * Given a vector of values and a matching number of maps,
 * find the set of indices for value_0 in map_0, value_1 in
 * map_1, ... and return those in a vector of integers. If
 * any of the values cannot be found, return -1 for it's index.
 *
 * @param values Look for these values in the corresponding maps
 * @param maps The maps
 * @return A vector of index values; -1 indicates the corresponding
 * value was not found.
 */
vector<int>
find_value_indices(const vector<double> &values, const vector< vector<double> > &maps)
{
    assert(values.size() == maps.size());

    vector<int> indices;
    vector <vector<double> >::const_iterator m = maps.begin();
    for (vector<double>::const_iterator d = values.begin(), e = values.end(); d != e; ++d) {
        indices.push_back(find_value_index(*d, *m++));
    }

    return indices;
}

/**
 * Are any of the idex values -1? if so, this is not a valid index list
 * @param indices vector of integer indices
 * @return True if all of the index values are positive, false otherwise
 */
bool all_indices_valid(vector<int> indices)
{
    return find(indices.begin(), indices.end(), -1) == indices.end();
}

// Dan: In this function I made the vector<dods_byte> a reference so that changes to
// it will be accessible to the caller without having to return the vector<> mask
// (it could be large). This also means that it won't be passed on the stack
template<typename T>
void make_mask_helper(const vector<Array*> dims, Array *tuples, vector<dods_byte> &mask)
{
    vector< vector<double> > dim_value_vecs(dims.size());
    
    int i = 0;  // index the dim_value_vecs vector of vectors;
    for (vector<Array*>::const_iterator d = dims.begin(), e = dims.end(); d != e; ++d) {
        // Dan: My mistake. There is a second extract_double_array() that does
        // just what we want that I forgot about. The version I put in the email
        // returns a C array of doubles that you'll have to deallocate using delete[].
        //
        // dim_value_vecs.at(i++) = extract_double_array(*d);

        // This version of extract...() takes the vector<double> by reference:
        // In util.cc/h: void extract_double_array(Array *a, vector<double> &dest)
        extract_double_array(*d, dim_value_vecs.at(i++));
    }

    // Construct and Odometer used to calculate offsets
    int rank = dims.size();
    Odometer::shape shape(rank);

    int j = 0;  // index the shape vector for an Odometer;

    for (vector<Array*>::const_iterator d = dims.begin(), e = dims.end(); d != e; ++d) {
        // Changed d-> to (*d)-> ... So this is one thing about iterators that
        // is less than optimal. The iterator dereferences to the thing in the
        // container, which is this case is a pointer. But you have to explicitly
        // dereference the iterator. Also, Array::length(), not size() (I
        // confuse those two also).
	shape.at(j++) = (*d)->length();
    }

    Odometer odometer(shape);

    // Copy the 'tuple' data to a simple vector<T>
    vector<T> data(tuples->length());
    tuples->value(&data[0]);

    // Iterate over the tuples, searching the dimensions for their values
    int nDims = dims.size();
    int nTuples = data.size() / nDims;

    // NB: 'data' holds the tuple values

    // unsigned int tuple_offset = 0;       // an optimization...
    for (int n = 0; n < nTuples; ++n) {
        vector<double> tuple(nDims);
        // Build the next tuple
        for (int dim = 0; dim < nDims; ++dim) {
            // could replace 'tuple * nDims' with 'tuple_offset'
            tuple[dim] = data[n * nDims + dim];
        }

        // find indices for tuple-values in the specified
	// target-grid dimensions
	vector<int> indices = find_value_indices(tuple, dim_value_vecs);

        // if all of the indices are >= 0, then add this point to the mask
        if (all_indices_valid(indices)) {

	    // Pass identified indices to Odometer, it will automatically
	    // calculate offset within defined 'shape' using those index values.
	    // Result of set_indices() will update d_offset value, accessible
	    // using the Odometer::offset() accessor.
	    odometer.set_indices(indices);  
	    mask[odometer.offset()] = 1;
        }
    }
}

/** Given a ...

 @param argc A count of the arguments
 @param argv An array of pointers to each argument, wrapped in a child of BaseType
 @param btpp A pointer to the return value; caller must delete.

 @return The mask variable, represented using Byte
 @exception Error Thrown if target variable is not a DAP2 Grid
 **/
void function_dap2_make_mask(int argc, BaseType * argv[], DDS &, BaseType **btpp)
{
    if (argc == 0) {
        Str *response = new Str("info");
        response->set_value(make_mask_info);
        *btpp = response;
        return;
    }

    // Check for two args or more. The first two must be strings.
    DBG(cerr << "argc = " << argc << endl);
    if (argc < 4)
        throw Error(malformed_expr,
                "make_mask(target,nDims,[dim1,...],$TYPE(dim1_value0,dim2_value0,...)) requires at least four arguments.");

    string requestedTargetName = extract_string_argument(argv[0]);
    BESDEBUG("functions", "Requested target variable: " << requestedTargetName << endl);

    BaseType *btp = argv[0];

    if (btp->type() != dods_grid_c) {
        throw Error(malformed_expr, "make_mask(first argument must point to a DAP2 Grid variable.");
    }

    Grid *g = static_cast<Grid*>(btp);
    Array *a = g->get_array();

    // Create the 'mask' array using the shape of the target grid variable's array.

    vector<dods_byte> mask(a->length(),0);  // Create 'mask', initialized with zero's

    // read argv[1], the number[N] of dimension variables represented in tuples

    unsigned int nDims = extract_uint_value(argv[1]);
#if 0
    unsigned int nDims =2; //FIXME
#endif

    // read argv[2] -> argv[2+numberOfDims]; the grid dimensions comprising mask tuples

    vector<Array*> dims;

    for (unsigned int i = 0; i < nDims; i++) {

        btp = argv[2 + i];
        if (btp->type() != dods_array_c) {
            throw Error(malformed_expr,
                    "make_mask(dimension-name arguments must point to a DAP2 Grid variable dimensions.");
        }

        int dSize;
        Array *a = static_cast<Array*>(btp);
        for (Array::Dim_iter itr = a->dim_begin(); itr != a->dim_end(); ++itr) {
            dSize = a->dimension_size(itr);
            cerr << "dim[" << i << "] = " << a->name() << " size=" << dSize << endl;
        }

        dims.push_back(a);

    }

    BESDEBUG("functions", "number of dimensions: " << dims.size() << endl);

    btp = argv[argc - 1];

    if (btp->type() != dods_array_c) {
        throw Error(malformed_expr, "make_mask(last argument must be a special-form array..");
    }

    check_number_type_array(btp);  // Throws an exception if not a numeric type.

    Array *tuples = static_cast<Array*>(btp);

    switch (tuples->var()->type()) {
    // All mask values are stored in Byte DAP variables by the stock argument parser
    // except values too large; those are stored in a UInt32
    case dods_byte_c:
        make_mask_helper<dods_byte>(dims, tuples, mask);
        break;

    case dods_int16_c:
        make_mask_helper<dods_int16>(dims, tuples, mask);
        break;

    case dods_uint16_c:
        make_mask_helper<dods_uint16>(dims, tuples, mask);
        break;

    case dods_int32_c:
        make_mask_helper<dods_int32>(dims, tuples, mask);
        break;

    case dods_uint32_c:
        make_mask_helper<dods_uint32>(dims, tuples, mask);
        break;

    case dods_float32_c:
        make_mask_helper<dods_float32>(dims, tuples, mask);
        break;

    case dods_float64_c:
        make_mask_helper<dods_float64>(dims, tuples, mask);
        break;

    case dods_str_c:
        //make_mask_helper<dods_str>(dims, tuples, mask);
        throw InternalErr(__FILE__, __LINE__, "make_mask function non-numeric dimension type error");
        break;

    case dods_url_c:
        //make_mask_helper<dods_url>(dims, tuples, mask);
        throw InternalErr(__FILE__, __LINE__, "make_mask function non-numeric dimension type error");
        break;

    default:
        throw InternalErr(__FILE__, __LINE__, "Unknown type error");
    }

    BESDEBUG("function", "function_dap2_make_mask() -target " << requestedTargetName << " -nDims " << nDims << endl);

    //*btpp = function_linear_scale_worker(argv[0], m, b, missing, use_missing);

}

} // namespace functions
