
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

#include <BESDebug.h>

#include "BBoxFunction.h"
#include "Odometer.h"
#include "roi_util.h"

// Set this to 1 to use special code for arrays of rank 1 and 2.
// set it to 0 (... comment out, etc.) to use the general code for
// all cases. I've run the unit and regression tests both ways.
// jhrg 3/2/15
#define UNWIND_BBOX_CODE 1

using namespace std;
using namespace libdap;

namespace functions {

auto_ptr<Array> bbox_helper(double min_value, double max_value, Array* the_array)
{
    // Get the values as doubles
    vector<double> the_values;
    extract_double_array(the_array, the_values); // This function sets the size of the_values

    // Build the response
    unsigned int rank = the_array->dimensions();
    auto_ptr<Array> response = roi_bbox_build_empty_bbox(rank, the_array->name());

    switch (rank) {
    case 1: {
        unsigned int X = the_array->dimension_size(the_array->dim_begin());
        bool found_start = false;
        unsigned int start = 0;
        for (unsigned int i = 0; i < X && !found_start; ++i) {
            if (the_values[i] >= min_value && the_values[i] <= max_value) {
                start = i;
                found_start = true;
            }
        }
        // ! found_start == error?
        if (!found_start) {
            ostringstream oss("In function bbox(): No values between ", std::ios::ate);
            oss << min_value << " and " << max_value << " were found in the array '" << the_array->name() << "'";
            throw Error(oss.str());
        }
        bool found_stop = false;
        unsigned int stop = X - 1;
        for (int i = X - 1; i >= 0 && !found_stop; --i) {
            if (the_values[i] >= min_value && the_values[i] <= max_value) {
                stop = (unsigned int) (i);
                found_stop = true;
            }
        }
        // ! found_stop == error?
        if (!found_stop) throw InternalErr(__FILE__, __LINE__, "In BBoxFunction: Found start but not stop.");

        Structure* slice = roi_bbox_build_slice(start, stop, the_array->dimension_name(the_array->dim_begin()));
        response->set_vec_nocopy(0, slice);
        break;
    }
    case 2: {
        // quick reminder: rows == y == j; cols == x == i
        Array::Dim_iter rows = the_array->dim_begin(), cols = the_array->dim_begin() + 1;
        unsigned int Y = the_array->dimension_size(rows);
        unsigned int X = the_array->dimension_size(cols);
        unsigned int x_start = X - 1; //= 0;
        unsigned int y_start = 0;
        bool found_y_start = false;
        // Must look at all rows to find the 'left-most' col with value
        for (unsigned int j = 0; j < Y; ++j) {
            bool found_x_start = false;
            for (unsigned int i = 0; i < X && !found_x_start; ++i) {
                unsigned int ind = j * X + i;
                if (the_values[ind] >= min_value && the_values[ind] <= max_value) {
                    x_start = min(i, x_start);
                    found_x_start = true;
                    if (!found_y_start) {
                        y_start = j;
                        found_y_start = true;
                    }
                }
            }
        }
        // ! found_y_start == error?
        if (!found_y_start) {
            ostringstream oss("In function bbox(): No values between ", std::ios::ate);
            oss << min_value << " and " << max_value << " were found in the array '" << the_array->name() << "'";
            throw Error(oss.str());
        }
        unsigned int x_stop = 0;
        unsigned int y_stop = 0;
        bool found_y_stop = false;
        // Must look at all rows to find the 'left-most' col with value
        for (int j = Y - 1; j >= (int) (y_start); --j) {
            bool found_x_stop = false;
            for (int i = X - 1; i >= 0 && !found_x_stop; --i) {
                unsigned int ind = j * X + i;
                if (the_values[ind] >= min_value && the_values[ind] <= max_value) {
                    x_stop = max((unsigned int) (i), x_stop);
                    found_x_stop = true;
                    if (!found_y_stop) {
                        y_stop = j;
                        found_y_stop = true;
                    }
                }
            }
        }
        // ! found_stop == error?
        if (!found_y_stop) throw InternalErr(__FILE__, __LINE__, "In BBoxFunction: Found start but not stop.");

        response->set_vec_nocopy(0, roi_bbox_build_slice(y_start, y_stop, the_array->dimension_name(rows)));
        response->set_vec_nocopy(1, roi_bbox_build_slice(x_start, x_stop, the_array->dimension_name(cols)));
        break;
    }
    default: {
        Odometer::shape shape(rank); // the shape of 'the_array'
        int j = 0;
        for (Array::Dim_iter i = the_array->dim_begin(), e = the_array->dim_end(); i != e; ++i) {
            shape.at(j++) = the_array->dimension_size(i);
        }
        Odometer odometer(shape);
        Odometer::shape indices(rank); // Holds a given index
        Odometer::shape min = shape; // Holds the minimum values for each of rank dimensions
        Odometer::shape max(rank, 0); // ... and the maximum. min and max define the bounding box
        // NB: shape is initialized with the size of the array
        do {
            if (the_values[odometer.offset()] >= min_value && the_values[odometer.offset()] <= max_value) {
                // record this index
                odometer.indices(indices);
                Odometer::shape::iterator m = min.begin();
                Odometer::shape::iterator x = max.begin();
                for (Odometer::shape::iterator i = indices.begin(), e = indices.end(); i != e; ++i, ++m, ++x) {
                    if (*i < *m) *m = *i;

                    if (*i > *x) *x = *i;
                }
            }
        } while (odometer.next() != odometer.end());
        // cheap test for 'did we find any values.' If we did, then the
        // min index will have to be less than the shape (which is the
        // size of the array). We only need to test one of the indices.
        if (min[0] == shape[0]) {
            ostringstream oss("In function bbox(): No values between ", std::ios::ate);
            oss << min_value << " and " << max_value << " were found in the array '" << the_array->name() << "'";
            throw Error(oss.str());
        }
        Odometer::shape::iterator m = min.begin();
        Odometer::shape::iterator x = max.begin();
        Array::Dim_iter d = the_array->dim_begin();
        for (unsigned int i = 0; i < rank; ++i, ++m, ++x, ++d) {
            response->set_vec_nocopy(i, roi_bbox_build_slice(*m, *x, the_array->dimension_name(d)));
        }
        break;
    } // default
    } // switch

    response->set_read_p(true);
    response->set_send_p(true);
    return response;
}

/**
 * @brief Return the bounding box for an array
 *
 * Given an N-dimensional Array of simple numeric types and two
 * minimum and maximum values, return the indices of a N-dimensional
 * bounding box. The indices are returned using an Array of
 * Structure, where each element of the array holds the name,
 * start index and stop index in fields with those names.
 *
 * It is up to the caller to make use of the returned values; the
 * array is not modified in any way other than to read in it's
 * values (and set the variable's read_p property).
 *
 * The returned Structure Array has the same name as the variable
 * it applies to, so that error messages can reference the source
 * variable.
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
    const string wrong_args = "Wrong number of arguments to bbox(). Expected an Array and minimum and maximum values (3 arguments)";

    switch (argc) {
    case 0:
        throw Error(malformed_expr, wrong_args);
    case 3:
        // correct number of args
        break;
    default:
        throw Error(malformed_expr, wrong_args);
    }

    if (argv[0] && argv[0]->type() != dods_array_c)
        throw Error("In function bbox(): Expected argument 1 to be an Array.");
    if (!argv[0]->var()->is_simple_type() || argv[0]->var()->type() == dods_str_c || argv[0]->var()->type() == dods_url_c)
        throw Error("In function bbox(): Expected argument 1 to be an Array of numeric types.");

    // cast is safe given the above
    Array *the_array = static_cast<Array*>(argv[0]);
    BESDEBUG("bbox", "the_array: " << the_array->name() << ": " << (void*)the_array << endl);

    // Read the variable into memory
    the_array->read();
    the_array->set_read_p(true);

    double min_value = extract_double_value(argv[1]);
    double max_value = extract_double_value(argv[2]);

    // Get the values as doubles
    auto_ptr<Array> response = bbox_helper(min_value, max_value, the_array);

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
 * @note Not yet implemented.
 *
 * @see function_dap2_bbox
 */
BaseType *function_dap4_bbox(D4RValueList * /* args */, DMR & /* dmr */)
{
    throw Error(malformed_expr, "Not yet implemented for DAP4 functions.");

    return 0; //response.release();
}

} // namesspace functions
