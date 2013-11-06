// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of libdap, A C++ implementation of the OPeNDAP Data
// Access Protocol.

// Copyright (c) 2002,2003 OPeNDAP, Inc.
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

// (c) COPYRIGHT URI/MIT 1999
// Please read the full copyright statement in the file COPYRIGHT_URI.
//
// Authors:
//      jhrg,jimg       James Gallagher <jgallagher@gso.uri.edu>


// These functions are used by the CE evaluator
//
// 1/15/99 jhrg

#include "config.h"

#include <limits.h>

#if 0
#include <cstdlib>      // used by strtod()
#include <cerrno>
#include <cmath>
#endif
#include <iostream>
#if 0
#include <vector>
#include <algorithm>
#endif

// #include <gdal.h>
// #include <gdal_priv.h>

#define DODS_DEBUG

#include "BaseType.h"

#include "Str.h"
#include "Array.h"
#include "Grid.h"

#include "Error.h"
#include "debug.h"

#include "DAP_Dataset.h"
#include "reproj_functions.h"

//  We wrapped VC++ 6.x strtod() to account for a short coming
//  in that function in regards to "NaN".  I don't know if this
//  still applies in more recent versions of that product.
//  ROM - 12/2007
#ifdef WIN32
#include <limits>
double w32strtod(const char *, char **);
#endif

using namespace std;
//using namespace libdap;

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
    DBG(cerr << "Entering function_swath2array..." << endl);

    // Use the same documentation for both swath2array and swath2grid
    string info = string("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n")
    		+ "<function name=\"swath2array\" version=\"1.0\" href=\"http://docs.opendap.org/index.php/Server_Side_Processing_Functions#swath2grid\">\n"
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
    	throw Error("The function swath2array() requires three arguments. See http://docs.opendap.org/index.php/Server_Side_Processing_Functions#swath2grid");

    Array *src = dynamic_cast<Array*>(argv[0]);
    if (!src)
    	throw Error("The first argument to swath2array() must be a data array. See http://docs.opendap.org/index.php/Server_Side_Processing_Functions#swath2grid");

    Array *lat = dynamic_cast<Array*>(argv[1]);
    if (!lat)
    	throw Error("The second argument to swath2array() must be a latitude array. See http://docs.opendap.org/index.php/Server_Side_Processing_Functions#swath2grid");

    Array *lon = dynamic_cast<Array*>(argv[2]);
    if (!lon)
    	throw Error("The third argument to swath2array() must be a longitude array. See http://docs.opendap.org/index.php/Server_Side_Processing_Functions#swath2grid");

    // The args passed into the function using argv[] are deleted after the call.

    DAP_Dataset ds(src, lat, lon);

    try {
        ds.InitialDataset(0);

        *btpp = ds.GetDAPArray();
    }
    catch (Error &e) {
        DBG(cerr << "caught Error: " << e.get_error_message() << endl);
        throw e;
    }
    catch(...) {
        DBG(cerr << "caught unknown exception" << endl);
        throw;
    }

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
    DBG(cerr << "Entering function_swath2grid..." << endl);

    string info = string("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n")
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
    	throw Error("The function swath2grid() requires three arguments. See http://docs.opendap.org/index.php/Server_Side_Processing_Functions#swath2grid");

    Array *src = dynamic_cast<Array*>(argv[0]);
    if (!src)
    	throw Error("The first argument to swath2grid() must be a data array. See http://docs.opendap.org/index.php/Server_Side_Processing_Functions#swath2grid");

    Array *lat = dynamic_cast<Array*>(argv[1]);
    if (!lat)
    	throw Error("The second argument to swath2grid() must be a latitude array. See http://docs.opendap.org/index.php/Server_Side_Processing_Functions#swath2grid");

    Array *lon = dynamic_cast<Array*>(argv[2]);
    if (!lon)
    	throw Error("The third argument to swath2grid() must be a longitude array. See http://docs.opendap.org/index.php/Server_Side_Processing_Functions#swath2grid");

    // The args passed into the function using argv[] are deleted after the call.

    DAP_Dataset ds(src, lat, lon);

    try {
        ds.InitialDataset(0);

        *btpp = ds.GetDAPGrid();
    }
    catch (Error &e) {
        DBG(cerr << "caught Error: " << e.get_error_message() << endl);
        throw e;
    }
    catch(...) {
        DBG(cerr << "caught unknown exception" << endl);
        throw;
    }

    return;
}





} // namespace libdap
