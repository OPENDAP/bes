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

#include "BBoxCombFunction.h"
#include "roi_util.h"

using namespace std;
using namespace libdap;

namespace functions {

/**
 * @brief to just "Combine two bounding boxes with different shapes"
 *
 * This combines to bounding boxes (Array of Structure) of rank N and M
 * into a single bounding box of rank N+M.
 *
 * @note There is DAP2 versions of this function.
 *
 * @param argc Argument count - must be 2
 * @param argv Argument vector - variable in the current DDS
 * @param dds The current DDS
 * @param btpp Value-result parameter for the resulting Array of Structure
 */

void
function_dap2_bbox_comb(int argc, BaseType *argv[], DDS &, BaseType **btpp)
{
    const string wrong_args = "Wrong number of arguments to bbox_comb(). Expected two bounding boxes";

    unsigned int rank = 0;
    unsigned int rnk1= 0;
    unsigned int rnk2 = 0;

    switch (argc) {
    case 0:
    case 1:
        // Must have 2 arguments
        throw Error(malformed_expr, wrong_args);

    case 2:
        // Rank should be sum of ranks of arguments because names all different,
        rnk1 = roi_valid_bbox(argv[0]);
        rnk2 = roi_valid_bbox(argv[1]);
        break;

    default:
        throw Error(malformed_expr, wrong_args);
        break;
    }

    // Define rank for output result
    rank = rnk1 + rnk2;

    // Initialize a local data structure - used because it's much
    // easier to read and write this than the DAP variables.
    vector<slice> combo(rank);     // struct slice is defined in roi_utils.h

    //first arg
    Array *bbox1 = static_cast<Array*>(argv[0]);

    for (unsigned int i = 0; i < rnk1; ++i) {
        int start, stop;
        string name;
        // start, stop, name are value-result parameters; we know they are Array*
        // because of the roi_valid_bbox() test.
        roi_bbox_get_slice_data(static_cast<Array*>(argv[0]), i, start, stop, name);

        combo.at(i).start = start;
        combo.at(i).stop = stop;
        combo.at(i).name = name;
    }
    //second arg
    Array *bbox2 = static_cast<Array*>(argv[1]);

    for (unsigned int j = 0; j < rnk2; ++j) {
        int start, stop;
        string name;
        // start, stop, name are value-result parameters; we know they are Array*
        // because of the roi_valid_bbox() test.
        roi_bbox_get_slice_data(static_cast<Array*>(argv[1]), j, start, stop, name);
        if(combo.at(j).name != name){
            combo.at(rnk1+j).start = start;
            combo.at(rnk1+j).stop = stop;
            combo.at(rnk1+j).name = name;
        }
    }

    // Build the response; name the result after the operation
    auto_ptr<Array> response = roi_bbox_build_empty_bbox(rank);
    for (unsigned int k = 0; k < rank; ++k) {
        Structure *slice = roi_bbox_build_slice(combo.at(k).start, combo.at(k).stop, combo.at(k).name);
        response->set_vec_nocopy(k, slice);
    }
    // Return the result
    *btpp = response.release();
    return;
}

} /* namespace functions */
