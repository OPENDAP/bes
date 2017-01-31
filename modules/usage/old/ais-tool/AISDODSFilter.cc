
// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of libdap, A C++ implementation of the OPeNDAP Data
// Access Protocol.

// Copyright (c) 2003 OPeNDAP, Inc.
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
 
#ifdef __GNUG__
#pragma implementation
#endif

#include "config.h"

static char rcsid[] not_used = {"$Id$"};

#include <GetOpt.h>

#include "../../../usage/old/ais-tool/AISDODSFilter.h"
#include "debug.h"

AISDODSFilter::AISDODSFilter(int argc, char *argv[]) 
{
    initialize(argc, argv);
}

/** This method is used to process command line options feed into the filter.
    This is a virtual method and is called by DODSFilter's constructor. This
    version uses the code in the DODSFilter version plus some new code. This
    is probably \i not the best way to handle this sort of thing, but
    splitting up the option processing is hard to do with GetOpt. */
int
AISDODSFilter::process_options(int argc, char *argv[]) throw(Error)
{
    DBG(cerr << "Entering process_options... ");

    d_ais_db = "";

    DBG(cerr << "Entering process_options... ");

    int option_char;
    GetOpt getopt (argc, argv, "B:ce:v:d:f:r:l:o:u:t:");

    while ((option_char = getopt()) != EOF) {
        switch (option_char) {
          // This is the one new option.
          case 'B': d_ais_db = getopt.optarg; break;                
                
          case 'c': d_comp = true; break;
          case 'e': set_ce(getopt.optarg); break;
          case 'v': set_cgi_version(getopt.optarg); break;
          case 'd': d_anc_dir = getopt.optarg; break;
          case 'f': d_anc_file = getopt.optarg; break;
          case 'r': d_cache_dir = getopt.optarg; break;
          case 'o': set_response(getopt.optarg); break;
          case 'u': set_URL(getopt.optarg); break;
          case 't': d_timeout = atoi(getopt.optarg); break;
          case 'l': 
            d_conditional_request = true;
            d_if_modified_since 
                = static_cast<time_t>(strtol(getopt.optarg, NULL, 10));
            break;
          default: print_usage(); // Throws Error
        }
    }

    DBGN(cerr << "exiting." << endl);

    return getopt.optind;       // return the index of the next argument
}
