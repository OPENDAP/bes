
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

#include "BBoxUnionFunction.h"
#include "roi_utils.h"

using namespace std;

namespace libdap {

/**
 * @brief Combine several bounding boxes, forming their union.
 *
 * @note There are both DAP2 and DAP4 versions of this function.
 *
 * @param argc Argument count
 * @param argv Argument vector - variable in the current DDS
 * @param dds The current DDS
 * @param btpp Value-result parameter for the resulting Array of Structure
 */
void
function_dap2_bbox_union(int argc, BaseType *argv[], DDS &, BaseType **btpp)
{
#if 0
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
    response->set_send_p(true);

    *btpp = response.release();
#endif

    unsigned int rank = 0;

    switch (argc) {
    case 0:
        // Must have 1 or more arguments
        throw Error("No help yet");

    default:
        // Vet the input: All bbox variables must be the same shape
        rank = roi_valid_bbox(argv[0]); // throws if bbox is not valid

        // Actually, we could us names to for the unions - they don't
        // really have to be the same shape, but this will do for now.
        for (int i = 1; i < argc; ++i)
            if (roi_valid_bbox(argv[0]) != rank)
                throw Error("In function bbox_union(): All bounding boxes must be the same shape to form their union.");
        break;
    }

    // Build the response
    auto_ptr<Array> response = roi_bbox_build_empty_bbox(rank, "indices");

#if 0
    // Build a new BBox variable to return
    auto_ptr<Array> response = roi_bbox_build_empty_bbox();
    response->set_length(rank);
#endif
    // For each BBox, for each dimension, update the union,
    // using the first BBox as a starting point

    // Initialize...
    for (unsigned int i = 0; i < rank; ++i) {
        int start, stop;
        string name;
        // start, stop, name are value-result parameters; we know they are Array*
        // because of the roi_valid_bbox() test.
        roi_bbox_get_slice_data(static_cast<Array*>(argv[0]), i, start, stop, name);

        Structure *slice = roi_bbox_build_slice(start, stop, name);

        response->set_vec_nocopy(i, slice);
    }

    // Foreach BBox, for each dimension...

    // Return the result
    *btpp = response.release();
    return;
}

/**
 * @brief Combine several bounding boxes, forming their union.
 *
 * @note The main difference between this function and the DAP2
 * version is to use args->size() in place of argc and
 * args->get_rvalue(n)->value(dmr) in place of argv[n].
 *
 * @note Not yet implemented.
 *
 * @see function_dap2_bbox
 */
BaseType *function_dap4_bbox_union(D4RValueList *args, DMR &)
{
    auto_ptr<Array> response(new Array("bbox", new Structure("bbox")));

    throw Error(malformed_expr, "Not yet implemented for DAP4 functions.");

    return response.release();
}

} // namesspace libdap
