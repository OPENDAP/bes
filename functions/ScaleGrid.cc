
// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of libdap, A C++ implementation of the OPeNDAP Data
// Access Protocol.

// Copyright (c) 2016 OPeNDAP, Inc.
// Author: James Gallagher <jgallagher@opendap.org>
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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.

#include "ScaleGrid.h"

#include "config.h"

#include <memory>
#include <string>

#include <BaseType.h>
#include <Array.h>
#include <Grid.h>
#include <Str.h>
#include <Error.h>
#include <util.h>
#include <BesDebug.h>

#include "functions_util.h"

#define DEBUG_KEY "geo"

using namespace std;
using namespace libdap;


namespace functions {


/**
 * @brief Scale a grid; uses gdal
 *
 * A server function that will scale a Grid. The function takes three required
 * parameters: A Grid and the new Longitude and Latitude extents (y and x in
 * Cartesian coordinates). It also takes two optional arguments: a CRS for the
 * result and an interpolation method.
 *
 * @param argc Argument count
 * @param argv Argument list
 * @param dds Unused
 * @param btpp Return value (A BaseType that holds the function result)
 */
void function_scale_grid(int argc, BaseType *argv[], DDS &, BaseType **btpp)
{
    string info =
    string("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n") +
    "<function name=\"scale_grid\" version=\"1.0\" href=\"http://docs.opendap.org/index.php/Server_Side_Processing_Functions#scale_grid\">\n" +
    "</function>\n";

    if (argc == 0) {
        auto_ptr<Str> response(new Str("info"));
        response->set_value(info);
        *btpp = response.release();
        return;
    }

    if (argc < 3 || argc > 5) {
        throw Error("The scale_grid() function requires three arguments: a Grid and the new lon, lat extents (got " + long_to_string(argc) + " args).\n\
             See http://docs.opendap.org/index.php/Server_Side_Processing_Functions#scale_grid");
    }

    Grid *src = dynamic_cast < Grid * >(argv[0]);
    if (!src)
        throw Error(malformed_expr,"The first argument to scale_grid() must be a Grid variable!");

    BESDEBUG(DEBUG_KEY,"function_scale_grid() - Evaluating grid '"<< src->name() << "'" << endl);
    unsigned long y = extract_uint_value(argv[1]);
    unsigned long x = extract_uint_value(argv[2]);

    string crs = "WGS84";
    string interp = "nearest";
    if (argc > 3) {
        crs = extract_string_argument(argv[3]);
    }
    if (argc > 4) {
        interp = extract_string_argument(argv[4]);
    }

    BESDEBUG(DEBUG_KEY,"function_scale_grid() - CRS '"<< crs << "'" << endl);
    BESDEBUG(DEBUG_KEY,"function_scale_grid() - Interpolation Method '"<< interp << "'" << endl);

    SizeBox size(y, x);
    *btpp = scale_dap_grid(src, size, crs, interp);
}

//Grid *scale_dap_array(const Array *data, const Array *lon, const Array *lat, const SizeBox &size,
// const string &crs, const string &interp)
/**
 * @brief Scale a grid; uses gdal
 *
 * A server function that will scale a data array. The function takes five required
 * parameters: A data array to scale, two lon/lat arrays that are effectively
 * DAP2 Grid map vectors and the new Longitude and Latitude extents (y and x in
 * Cartesian coordinates). It also takes two optional arguments: a CRS for the
 * result and an interpolation method.
 *
 * @param argc Argument count
 * @param argv Argument list
 * @param dds Unused
 * @param btpp Return value (A BaseType that holds the function result)
 */
void function_scale_array(int argc, BaseType *argv[], DDS &, BaseType **btpp)
{
    string info =
    string("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n") +
    "<function name=\"scale_array\" version=\"1.0\" href=\"http://docs.opendap.org/index.php/Server_Side_Processing_Functions#scale_array\">\n" +
    "</function>\n";

    if (argc == 0) {
        auto_ptr<Str> response(new Str("info"));
        response->set_value(info);
        *btpp = response.release();
        return;
    }

    if (!(argc > 4  && argc < 8)) {
        throw Error("The scale_array() function requires five arguments: three Arrays and the new lat and lon extents.\n\
             See http://docs.opendap.org/index.php/Server_Side_Processing_Functions#scale_array");
    }

    Array *data = dynamic_cast <Array *>(argv[0]);
    if (!data)
        throw Error(malformed_expr,"The first argument to scale_array() must be an Array variable!");
    Array *x = dynamic_cast <Array *>(argv[1]);
    if (!x)
        throw Error(malformed_expr,"The second argument to scale_array() must be an Array variable!");
    Array *y = dynamic_cast <Array *>(argv[2]);
    if (!y)
        throw Error(malformed_expr,"The third argument to scale_array() must be an Array variable!");

    unsigned long new_x = extract_uint_value(argv[3]);
    unsigned long new_y = extract_uint_value(argv[4]);

    string crs = "WGS84";   // FIXME WGS84 assumes a certain order for lat and lon
    string interp = "nearest";
    if (argc > 5) {
        crs = extract_string_argument(argv[5]);
    }

    if (argc > 6) {
        interp = extract_string_argument(argv[6]);
    }

    SizeBox size(new_x, new_y);
    *btpp = scale_dap_array(data, x, y, size, crs, interp);
}

}
