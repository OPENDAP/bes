
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
    unsigned int rank = 0;

    switch (argc) {
    case 0:
        // Must have 1 or more arguments
        throw Error("No help yet");

    default:
        // Vet the input: All bbox variables must be the same shape
        rank = roi_valid_bbox(argv[0]); // throws if bbox is not valid

        // Actually, we could us names to form the unions - they don't
        // really have to be the same shape, but this will do for now.
        for (int i = 1; i < argc; ++i)
            if (roi_valid_bbox(argv[0]) != rank)
                throw Error("In function bbox_union(): All bounding boxes must be the same shape to form their union.");
        break;
    }

    // For each BBox, for each dimension, update the union,
    // using the first BBox as a starting point.

    // Initialize a local data structure - used because it's much
    // easier to read and write this than the DAP variables.
    struct slice {
    	int start, stop;
    	string name;
    };
    vector<slice> result(rank);

    for (unsigned int i = 0; i < rank; ++i) {
        int start, stop;
        string name;
        // start, stop, name are value-result parameters; we know they are Array*
        // because of the roi_valid_bbox() test.
        roi_bbox_get_slice_data(static_cast<Array*>(argv[0]), i, start, stop, name);

        Structure *slice = roi_bbox_build_slice(start, stop, name);

        //response->set_vec_nocopy(i, slice);
        result.at(i).start = start;
        result.at(i).stop = stop;
        result.at(i).name = name;
    }

    // For each BBox, for each dimension...
    for (int i = 1; i < argc; ++i) {
        // cast is safe given the tests above
        Array *bbox = static_cast<Array*>(argv[i]);

        for (int i = 0; i < rank; ++i) {
            int start, stop;
            string name;
            // start, stop, name are value-result parameters
            roi_bbox_get_slice_data(bbox, i, start, stop, name);

            if (result.at(i).name != name)
            	throw Error("In function bbox_union(): named dimensions must match in teh bounding boxes");

            result.at(i).start = min(result.at(i).start, start);
            result.at(i).stop = max(result.at(i).stop, stop);
        }
    }

    // Build the response
    auto_ptr<Array> response = roi_bbox_build_empty_bbox(rank, "indices");
    for (int i = 0; i < rank; ++i) {
    	Structure *slice = roi_bbox_build_slice(result.at(i).start, result.at(i).stop, result.at(i).name);
    	response->set_vec_nocopy(i, slice);
    }

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
