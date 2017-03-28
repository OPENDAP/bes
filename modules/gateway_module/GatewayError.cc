// GatewayError.cc

// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of gateway_module, A C++ module that can be loaded in to
// the OPeNDAP Back-End Server (BES) and is able to handle remote requests.

// Copyright (c) 2002,2003 OPeNDAP, Inc.
// Author: Patrick West <pwest@ucar.edu>
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

// (c) COPYRIGHT URI/MIT 1994-1999
// Please read the full copyright statement in the file COPYRIGHT_URI.
//
// Authors:
//      pcw       Patrick West <pwest@ucar.edu>

#include <cstdio>
#include <cstdlib>

#include "GatewayError.h"

/** @brief read the target response file that contains textual error
 * information
 *
 * The contents of the file are read directly into the error message.
 *
 * @param filename target response file name with the error information
 * @param err return the error message in this variable
 * @param url the remote request URL
 */
void GatewayError::read_error(const string &filename, string &err, const string &url)
{
    err = "Remote Request failed for url: " + url + " with error: ";

    // The temporary file will contain the error information that we need to
    // report.
    FILE *f = fopen(filename.c_str(), "r");
    if (!f) {
        err = err + "Could not open the error file " + filename;
        return;
    }

    // read from the file until there is no more to read
    bool done = false;
    const unsigned int bufsize = 1025;
    while (!done) {
        char buff[bufsize];
        size_t bytes_read = fread(buff, 1, bufsize-1, f);
        if (0 == bytes_read) {
            done = true;
        }
        else {
            if (bytes_read < bufsize)
                buff[bytes_read] = '\0';
            else
                buff[bufsize-1] = '\0';

            err = err + buff;
        }
    }

    fclose(f);
}

