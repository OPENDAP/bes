
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
#include "roi_util.h"

using namespace std;
using namespace libdap;

namespace functions {

/**
 * @brief Combine several bounding boxes, forming their union.
 *
 * This combines N BBox variables (Array of Structure) forming
 * either their union or intersection, depending on the last
 * parameter's value ("union" or "inter[section]").
 *
 * If the function is passed bboxes that have no intersection,
 * an exception is thrown. This is so that callers will know
 * why no data were returned. Otherwise, an empty response, while
 * correct, could be baffling to the client.
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
    const string wrong_args = "Wrong number of arguments to bbox_union(). Expected one or more bounding boxes and a string naming the operation (2+ arguments)";

    unsigned int rank = 0;
    string operation = "";

    switch (argc) {
    case 0:
    case 1:
        // Must have 2 or more arguments
        throw Error(malformed_expr, wrong_args);

    default:
        // Vet the input: All bbox variables must be the same shape
        rank = roi_valid_bbox(argv[0]); // throws if bbox is not valid

        // Actually, we could us names to form the unions - they don't
        // really have to be the same shape, but this will do for now.
        for (int i = 1; i < argc-1; ++i)
            if (roi_valid_bbox(argv[0]) != rank)
                throw Error(malformed_expr, "In function bbox_union(): All bounding boxes must be the same shape to form their union.");

        operation = extract_string_argument(argv[argc-1]);
        downcase(operation);
        break;
    }

    // For each BBox, for each dimension, update the union,
    // using the first BBox as a starting point.

    // Initialize a local data structure - used because it's much
    // easier to read and write this than the DAP variables.
    vector<slice> result(rank);		// struct slice is defined in roi_utils.h

    for (unsigned int i = 0; i < rank; ++i) {
        int start, stop;
        string name;
        // start, stop, name are value-result parameters; we know they are Array*
        // because of the roi_valid_bbox() test.
        roi_bbox_get_slice_data(static_cast<Array*>(argv[0]), i, start, stop, name);

        result.at(i).start = start;
        result.at(i).stop = stop;
        result.at(i).name = name;
    }

    // For each BBox, for each dimension...
    for (int i = 1; i < argc-1; ++i) {
        // cast is safe given the tests above
        Array *bbox = static_cast<Array*>(argv[i]);

        for (unsigned int i = 0; i < rank; ++i) {
            int start, stop;
            string name;
            // start, stop, name are value-result parameters
            roi_bbox_get_slice_data(bbox, i, start, stop, name);

            if (result.at(i).name != name)
            	throw Error("In function bbox_union(): named dimensions must match in the bounding boxes");

            if (operation == "union") {
                result.at(i).start = min(result.at(i).start, start);
                result.at(i).stop = max(result.at(i).stop, stop);
            }
            else if (operation == "inter" || operation == "intersection") {
                result.at(i).start = max(result.at(i).start, start);
                result.at(i).stop = min(result.at(i).stop, stop);

                if (result.at(i).stop < result.at(i).start)
                	throw Error("In bbox_union(): The intersection of the bounding boxes is empty (dimension " + long_to_string(i) + ").");
            }
            else {
            	throw Error(malformed_expr, "In bbox_union(): Unknown operator '" + operation + "'; expected 'union', 'intersection' or 'inter'.");
            }
        }
    }

    // Build the response; name the result after the operation
    unique_ptr<Array> response = roi_bbox_build_empty_bbox(rank, operation);
    for (unsigned int i = 0; i < rank; ++i) {
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
BaseType *function_dap4_bbox_union(D4RValueList *, DMR &)
{
    //auto_ptr<Array> response(new Array("bbox", new Structure("bbox")));

    throw Error(malformed_expr, "Not yet implemented for DAP4 functions.");

    return 0; //response.release();
}

} // namesspace functions
