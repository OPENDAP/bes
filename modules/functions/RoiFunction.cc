
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

#include <libdap/BaseType.h>
#include <libdap/Int32.h>
#include <libdap/Str.h>
#include <libdap/Array.h>
#include <libdap/Structure.h>

#include <libdap/D4RValue.h>
#include <libdap/Error.h>
#include <libdap/debug.h>
#include <libdap/util.h>
#include <libdap/ServerFunctionsList.h>

#include <BESDebug.h>
#include <roi_util.h>

#include "RoiFunction.h"
#include "functions_util.h"

using namespace std;
using namespace libdap;

namespace functions {

/**
 * @brief Subset the N arrays using index slicing information
 *
 * This function should be called with a series of array variables,
 * each of which are N-dimensions or greater, where the N common
 * dimensions should all be the same size. The intent of this function
 * is that a N-dimensional bounding box, provided in indicial space,
 * will be used to subset each of the arrays. There are other functions
 * that can be used to build these bounding boxes using values of
 * dataset variables - see bbox() and bbox_union(). Taken together,
 * the roi(), bbox() and bbox_union() functions can be used to subset
 * a collection of Arrays where some arrays are taken to be dependent
 * variables and others independent variables. The result is a subset
 * of the 'discrete coverage' defined by the collection of independent
 * and dependent variables.
 *
 * @param argc Argument count
 * @param argv Argument vector - variable in the current DDS
 * @param dds The current DDS
 * @param btpp Value-result parameter for the resulting Array of Structure
 */
void
function_dap2_roi(int argc, BaseType *argv[], DDS &, BaseType **btpp)
{
    const string wrong_args = "Wrong number of arguments to roi(). Expected one or more Arrays and bounding box";

    // This is the rank of the Array of Slices, not the N-1 arrays to be sliced
    int rank = 0;

    switch (argc) {
    case 0:
    case 1:
        // Must have 2 or more arguments
        throw Error(malformed_expr, wrong_args);
    default:
        rank = roi_valid_bbox(argv[argc-1]); // throws if slice is not valid
    }

    unique_ptr<Structure> response(new Structure("roi_subset_unwrap"));

    auto bbox = static_cast<Array*>(argv[argc-1]);
    // Loop over arguments
    for (int i = 0; i < argc-1; ++i) {
        // cast is safe given the above
        auto the_array = static_cast<Array*>(argv[i]);

        // For each dimension of the array, apply the slice constraint.
        // Assume Array[]...[][X][Y] where the slice has dims X and Y
        // So find the last <rank> dimensions and check that their names
        // match those of the slice. This loops 'walks backward' over
        // both the N bbox slices and the right-most N dimensions of
        // the arrays.

        // Loop over dimensions in BBox
        for (int i = rank-1; i >= 0; --i) {
            int start, stop;
            string name;
            // start, stop, name are value-result parameters
            roi_bbox_get_slice_data(bbox, i, start, stop, name);
            // Loop over dimensions in argument
            for (auto iter = the_array->dim_begin(); iter< the_array->dim_end(); iter++){
                string cname = the_array->dimension_name(iter);
                if (the_array->dimension_name(iter) != name) continue;
                the_array->add_constraint(iter, start, 1 /*stride*/, stop);
            }
        }

        // Add the array to the Structure returned by the function
        the_array->set_send_p(true);    // include it

        // TODO Why do we have to force this read? The libdap::BaseType::serialize()
        // code should take care of it, but in the debugger the read_p property is
        // showing  up as true. jhrg 2/26/15 Hack and move on...
        //if (!the_array->read_p())
        the_array->set_read_p(false);
        the_array->read();
        the_array->set_read_p(true);

        response->add_var(the_array);
    }

    response->set_send_p(true);
    response->set_read_p(true);

    *btpp = response.release();
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
}

} // namesspace functions
