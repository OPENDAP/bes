
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

#include "BBoxFunction.h"

using namespace std;

namespace libdap {

/**
 * @brief Return the bounding box for an array
 *
 * Given an N-dimensional Array of simple types and two
 * minimum and maximum values, return the indices of a N-dimensional
 * bounding box. The indices are returned using an Array of
 * Structure, where each element of the array holds the name,
 * start index and stop index in fields with those names.
 *
 * It is up to the caller to make use of the returned values; the
 * array is not modified in any way other than to read in it's
 * values (and set the variable's read_p property).
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
BaseType *function_dap4_bbox(D4RValueList *args, DMR &)
{
    auto_ptr<Array> response(new Array("bbox", new Structure("bbox")));

    throw Error(malformed_expr, "Not yet implemented for DAP4 functions.");

    return response.release();
}

} // namesspace libdap
