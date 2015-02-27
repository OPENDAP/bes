
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

#include "RoiFunction.h"
#include "roi_utils.h"

using namespace std;

namespace libdap {

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
    auto_ptr<Structure> response(new Structure("roi_subset"));

    // This is the rank of the Array of Slices, not the N-1 arrays to be sliced
    int rank = 0;

    switch (argc) {
    case 0:
    case 1:
        // Must have 2 or more arguments
        throw Error("No help yet");
    default:
        rank = roi_valid_bbox(argv[argc-1]); // throws if slice is not valid

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
        // match those of the slice
        unsigned int num_dims = the_array->dimensions();
        int d = num_dims-1;
        for (int i = rank-1; i >= 0; --i) {
            int start, stop;
            string name;
            // start, stop, name are value-result parameters
            roi_bbox_get_slice_data(slices, i, start, stop, name);

            // Hack, should use reverse iterators, but Array does not have them
            Array::Dim_iter iter = the_array->dim_begin() + d;

            // TODO Make this an option (i.e., turn of the test)?
            // TODO Make a second option that will match names instead of position
            if (the_array->dimension_name(iter) != name)
                throw Error("In function roi(): Dimension name (" + the_array->dimension_name(iter) + ") and slice name (" + name + ") don't match");

            // TODO Add stride option?
            the_array->add_constraint(iter, start, 1 /*stride*/, stop);
            --d;
        }

        // Add the array to the Structure returned by the function
        the_array->set_send_p(true);    // include it

        // TODO Why do we have to force this read? The libdap::BaseType::serialize()
        // code should take care of it, but in the debugger the read_p property is
        // showing  up as true. jhrg 2/26/15 Hack and move on...
        if (!the_array->read_p())
            the_array->read();
        the_array->set_read_p(true);

        response->add_var(the_array);
    }

    response->set_send_p(true);
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
BaseType *function_dap4_roi(D4RValueList *, DMR &)
{
    throw Error(malformed_expr, "Not yet implemented for DAP4 functions.");

    return 0;
}

} // namesspace libdap
