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

//#define DODS_DEBUG 1

#include <cassert>

#include <sstream>
#include <vector>
#include <algorithm>

#include <Type.h>
#include <BaseType.h>
#include <Byte.h>
#include <Str.h>

#include <Array.h>
#include <Error.h>
#include <DDS.h>

#if 0
// No DAP4 support yet...
#include <DMR.h>
#include <D4Group.h>
#include <D4RValue.h>
#endif

#include <debug.h>
#include <util.h>

#include <BESDebug.h>

#include "MakeMaskFunction.h"
#include "Odometer.h"
#include "functions_util.h"

using namespace libdap;
using namespace std;

namespace functions {

vector<int> parse_dims(const string &shape); // defined in MakeArrayFunction.cc

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
        if (double_eq(*i, value, 0.1)) {        // FIXME Hack: 0.1 epsilon is a hack. jhrg 5/25/15
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

/**
 * Given a vector of Grid maps (effectively domain variables) and a 'list'
 * of tuples where N tuples and M maps means N x M values n the list, build
 * a mask that can be used to filter an array of N dimensions selecting only
 * locations (range values) that match those domain values. The list of M tuples
 * is organized so that the first 0, ..., N-1 (the first N values) are M0, the
 * next N values are M1, and so on.
 *
 * @param dims Maps from a Grid that supply the domain values' indices
 * @param tuples The domain values
 * @param mask The resulting mask for an N dimensional array, where N is the number
 * of arrays in the Vector<> dims
 */
template<typename T>
void make_mask_helper(const vector<Array*> dims, Array *tuples, vector<dods_byte> &mask)
{
    vector< vector<double> > dim_value_vecs(dims.size());
    int i = 0;  // index the dim_value_vecs vector of vectors;
    for (vector<Array*>::const_iterator d = dims.begin(), e = dims.end(); d != e; ++d) {
        // This version of extract...() takes the vector<double> by reference:
        // In util.cc/h: void extract_double_array(Array *a, vector<double> &dest)
        extract_double_array(*d, dim_value_vecs.at(i++));
    }

    // Construct and Odometer used to calculate offsets
    Odometer::shape shape(dims.size());

    int j = 0;  // index the shape vector for an Odometer;
    for (vector<Array*>::const_iterator d = dims.begin(), e = dims.end(); d != e; ++d) {
	shape[j++] = (*d)->length();
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

        DBG(cerr << "tuple: ");
        DBGN(copy(tuple.begin(), tuple.end(), ostream_iterator<int>(std::cerr, " ")));
        DBGN(cerr << endl);

        // find indices for tuple-values in the specified
	// target-grid dimensions
	vector<int> indices = find_value_indices(tuple, dim_value_vecs);
	DBG(cerr << "indices: ");
        DBGN(copy(indices.begin(), indices.end(), ostream_iterator<int>(std::cerr, " ")));
        DBGN(cerr << endl);

        // if all of the indices are >= 0, then add this point to the mask
        if (all_indices_valid(indices)) {

	    // Pass identified indices to Odometer, it will automatically
	    // calculate offset within defined 'shape' using those index values.
	    // Result of set_indices() will update d_offset value, accessible
	    // using the Odometer::offset() accessor.
	    odometer.set_indices(indices);
	    DBG(cerr << "odometer.offset(): " << odometer.offset() << endl);
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

    // Check for three args or more. The first two must be strings.
    DBG(cerr << "argc = " << argc << endl);
    if (argc < 3)
        throw Error(malformed_expr,
                "make_mask(shape_string,[dim1,...],$TYPE(dim1_value0,dim2_value0,...)) requires at least four arguments.");

    if (argv[0]->type() != dods_str_c)
        throw Error(malformed_expr, "make_mask(): first argument must point to a string variable.");

    string shape_str = extract_string_argument(argv[0]);
    vector<int> shape = parse_dims(shape_str);

    // Create the 'mask' array using the shape of the target grid variable's array.
    int length = 1;
    for (vector<int>::iterator i = shape.begin(), e = shape.end(); i != e; ++i)
        length *= *i;
    vector<dods_byte> mask(length, 0);  // Create 'mask', initialized with zero's
    unsigned int nDims = shape.size();

    // read argv[1] -> argv[1+numberOfDims]; the grid dimensions where we will find the values
    // of the mask tuples. Also note that the number of dims (shape.size() above) should be the
    // same as argc-2.
    assert(nDims == (unsigned int)argc-2);

    vector<Array*> dims;
    for (unsigned int i = 0; i < nDims; i++) {

        BaseType *btp = argv[1 + i];
        if (btp->type() != dods_array_c) {
            throw Error(malformed_expr,
                    "make_mask(): dimension-name arguments must point to Grid variable dimensions.");
        }

        Array *a = static_cast<Array*>(btp);

        // Check that each map size matches the 'shape' info passed in the first arg.
        // This might not be the case for DAP4 (or if we change this to support level
        // 2 swath data).
        assert(a->dimension_size(a->dim_begin()) == shape.at(i));

        a->read();
        a->set_read_p(true);
        dims.push_back(a);
    }

    BaseType *btp = argv[argc - 1];
    if (btp->type() != dods_array_c) {
        throw Error(malformed_expr, "make_mask(): last argument must be an array.");
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
    case dods_url_c:
    default:
        throw InternalErr(__FILE__, __LINE__,
                "make_mask(): Expect an array of mask points (numbers) but found something else instead.");
    }

    Array *dest = new Array("mask", 0);	// The ctor for Array copies the prototype pointer...
    BaseTypeFactory btf;
    dest->add_var_nocopy(new Byte("mask"));	// ... so use add_var_nocopy() to add it instead

    for (vector<int>::iterator i = shape.begin(), e = shape.end(); i != e; ++i)
        dest->append_dim(*i);

    dest->set_value(mask, length);

    dest->set_read_p(true);

    *btpp = dest;
}

} // namespace functions
