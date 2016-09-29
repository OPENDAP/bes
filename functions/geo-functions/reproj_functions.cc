// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of libdap, A C++ implementation of the OPeNDAP Data
// Access Protocol.

// Copyright (c) 2013 OPeNDAP, Inc.
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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.

#include "config.h"

#include <limits.h>

#include <iostream>

#define DODS_DEBUG

#include "BaseType.h"

#include "Str.h"
#include "Array.h"
#include "Grid.h"

#include "Error.h"
#include "debug.h"

#include "DAP_Dataset.h"
#include "reproj_functions.h"

using namespace std;

namespace libdap {

/**
 * @todo The lat and lon arrays are passed in, but there's an assumption that the
 * source data array and the two lat and lon arrays are the same shape. But the
 * code does not actually test that.
 *
 * @todo Enable multiple bands paired with just the two lat/lon arrays? Not sure
 * if that is a good idea...
 */
void function_swath2array(int argc, BaseType * argv[], DDS &, BaseType **btpp)
{
    DBG(cerr << "function_swath2array() - BEGIN" << endl);

    // Use the same documentation for both swath2array and swath2grid
    string info =
        string("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n")
            + "<function name=\"swath2array\" version=\"1.0\" href=\"http://docs.opendap.org/index.php/Server_Side_Processing_Functions#swath2grid\">\n"
            + "</function>\n";

    if (argc == 0) {
        Str *response = new Str("info");
        response->set_value(info);
        *btpp = response;
        DBG(cerr << "function_swath2array() - END (no args)" << endl);
        return;
    }

    // TODO Add optional fourth arg that lets the caller say which datum to use;
    // default to WGS84
    if (argc != 3)
        throw Error(
            "The function swath2array() requires three arguments. See http://docs.opendap.org/index.php/Server_Side_Processing_Functions#swath2grid");

    Array *src = dynamic_cast<Array*>(argv[0]);
    if (!src)
        throw Error(
            "The first argument to swath2array() must be a data array. See http://docs.opendap.org/index.php/Server_Side_Processing_Functions#swath2grid");

    Array *lat = dynamic_cast<Array*>(argv[1]);
    if (!lat)
        throw Error(
            "The second argument to swath2array() must be a latitude array. See http://docs.opendap.org/index.php/Server_Side_Processing_Functions#swath2grid");

    Array *lon = dynamic_cast<Array*>(argv[2]);
    if (!lon)
        throw Error(
            "The third argument to swath2array() must be a longitude array. See http://docs.opendap.org/index.php/Server_Side_Processing_Functions#swath2grid");

    // The args passed into the function using argv[] are deleted after the call.

    DAP_Dataset ds(src, lat, lon);

    try {
        ds.InitializeDataset(0);

        *btpp = ds.GetDAPArray();
    }
    catch (Error &e) {
        DBG(cerr << "function_swath2array() - Encountered libdap::Error   msg: '" << e.get_error_message() << "'" << endl);
        throw e;
    }
    catch (...) {
        DBG(cerr << "function_swath2array() - Encountered unknown exception." << endl);
        throw;
    }

    DBG(cerr << "function_swath2array() - END" << endl);

    return;
}

/**
 * @todo The lat and lon arrays are passed in, but there's an assumption that the
 * source data array and the two lat and lon arrays are the same shape. But the
 * code does not actually test that.
 *
 * @todo Enable multiple bands paired with just the two lat/lon arrays? Not sure
 * if that is a good idea...
 */
void function_swath2grid(int argc, BaseType * argv[], DDS &, BaseType **btpp)
{
    DBG(cerr << "function_swath2grid() - BEGIN" << endl);

    string info =
        string("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n")
            + "<function name=\"swath2grid\" version=\"1.0\" href=\"http://docs.opendap.org/index.php/Server_Side_Processing_Functions#swath2grid\">\n"
            + "</function>\n";

    if (argc == 0) {
        Str *response = new Str("info");
        response->set_value(info);
        *btpp = response;
        return;
    }

    // TODO Add optional fourth arg that lets the caller say which datum to use;
    // default to WGS84
    if (argc != 3)
        throw Error(
            "The function swath2grid() requires three arguments. See http://docs.opendap.org/index.php/Server_Side_Processing_Functions#swath2grid");

    Array *src = dynamic_cast<Array*>(argv[0]);
    if (!src)
        throw Error(
            "The first argument to swath2grid() must be a data array. See http://docs.opendap.org/index.php/Server_Side_Processing_Functions#swath2grid");

    Array *lat = dynamic_cast<Array*>(argv[1]);
    if (!lat)
        throw Error(
            "The second argument to swath2grid() must be a latitude array. See http://docs.opendap.org/index.php/Server_Side_Processing_Functions#swath2grid");

    Array *lon = dynamic_cast<Array*>(argv[2]);
    if (!lon)
        throw Error(
            "The third argument to swath2grid() must be a longitude array. See http://docs.opendap.org/index.php/Server_Side_Processing_Functions#swath2grid");

    // The args passed into the function using argv[] are deleted after the call.

    DAP_Dataset ds(src, lat, lon);

    try {
        DBG(cerr << "function_swath2grid() - Calling DAP_Dataset::InitialDataset()" << endl);
        ds.InitializeDataset(0);

        DBG(cerr << "function_swath2grid() - Calling DAP_Dataset::GetDAPGrid()" << endl);
        *btpp = ds.GetDAPGrid();

    }
    catch (libdap::Error &e) {
        DBG(cerr << "function_swath2grid() - Caught libdap::Error  msg:" << e.get_error_message() << endl);
        throw e;
    }
    catch (...) {
        DBG(cerr << "function_swath2grid() - Caught unknown exception." << endl);
        throw;
    }

    DBG(cerr << "function_swath2grid() - END" << endl);

    return;
}

#if 0
/**
 * @todo The lat and lon arrays are passed in, but there's an assumption that the
 * source data array and the two lat and lon arrays are the same shape. But the
 * code does not actually test that.
 *
 * @todo Enable multiple bands paired with just the two lat/lon arrays? Not sure
 * if that is a good idea...
 */
void function_changeCRS(int argc, BaseType * argv[], DDS &, BaseType **btpp)
{
    DBG(cerr << "function_changeCRS() - BEGIN" << endl);

    string functionName = "crs";

    string info = string("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n")
    + "<function name=\""+functionName+"\" version=\"1.0\" href=\"http://docs.opendap.org/index.php/Server_Side_Processing_Functions#crs\">\n"
    + "</function>\n";

    if (argc == 0) {
        Str *response = new Str("info");
        response->set_value(info);
        *btpp = response;
        return;
    }

    if (argc != 5)
    throw Error("The function "+functionName+"() requires five arguments. See http://docs.opendap.org/index.php/Server_Side_Processing_Functions#"+functionName);

    Array *src = dynamic_cast<Array*>(argv[0]);
    if (!src)
    throw Error("The first argument to "+functionName+"() must be a data array. See http://docs.opendap.org/index.php/Server_Side_Processing_Functions#"+functionName);

    Array *lat = dynamic_cast<Array*>(argv[1]);
    if (!lat)
    throw Error("The second argument to "+functionName+"() must be a latitude array. See http://docs.opendap.org/index.php/Server_Side_Processing_Functions#"+functionName);

    Array *lon = dynamic_cast<Array*>(argv[2]);
    if (!lon)
    throw Error("The third argument to "+functionName+"() must be a longitude array. See http://docs.opendap.org/index.php/Server_Side_Processing_Functions#"+functionName);

    Str *nativeCrsName = dynamic_cast<Str*>(argv[3]);
    if (!src)
    throw Error("The fourth argument to "+functionName+"() must be a string identifying the native CRS of the data array. See http://docs.opendap.org/index.php/Server_Side_Processing_Functions#"+functionName);

    Str *targetCrsName = dynamic_cast<Str*>(argv[4]);
    if (!src)
    throw Error("The fifth argument to "+functionName+"() must be a string identifying the target CRS. See http://docs.opendap.org/index.php/Server_Side_Processing_Functions#"+functionName);

    // The args passed into the function using argv[] are deleted after the call.

    DAP_Dataset ds(src, lat, lon);

    try {
        ds.InitializeDataset(0);

        *btpp = ds.GetDAPGrid();
    }
    catch(Error &e) {
        DBG(cerr << "caught Error: " << e.get_error_message() << endl);
        throw e;
    }
    catch(...) {
        DBG(cerr << "caught unknown exception" << endl);
        throw;
    }

    DBG(cerr << "function_changeCRS() - END" << endl);

    return;
}
#endif

}
 // namespace libdap
