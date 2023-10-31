// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of the Hyrax data server.

// Copyright (c) 2018 OPeNDAP, Inc.
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


#include <iostream>
#include <sstream>
#include <iterator>

#include <unistd.h>
#include <cstdlib>
#include <libgen.h>

#include <libdap/util.h>

#include <TheBESKeys.h>
#include <BESDebug.h>
#include <BESError.h>
#include <BESInternalFatalError.h>

#if 0

#include "build_dmrpp_util.h"

#endif

using namespace std;
using namespace libdap;
#if 0
using namespace dmrpp;
using namespace build_dmrpp_util;
#endif

#define DEBUG_KEY "metadata_store,dmrpp_store,dmrpp"

void usage() {
    const char *help = R"(
    build_dmrpp -h: Show this help

    build_dmrpp -V: Show build versions for components that make up the program

    build_dmrpp -f <data file> -r <dmr file> [-u <href url>]: As above, but uses the DMR
       read from the given file.

    Other options:
        -v: Verbose
        -d: Turn on BES software debugging output
        -M: Add information about the build_dmrpp software, incl versions, to the built DMR++)";

    cerr << help << endl;
}

/**
 *
 * @param argc
 * @param argv
 * @return
 */
int main(int argc, char *argv[]) {
    string h4_file_name;
    string h4_dset_path;
    string dmr_filename;
    string dmrpp_href_value;
    string bes_conf_file_used_to_create_dmr;
    bool add_production_metadata = false;

    int option_char;
    while ((option_char = getopt(argc, argv, "c:f:r:u:dhvVM")) != -1) {
        switch (option_char) {
            case 'V':
                cerr << basename(argv[0]) << "-" << CVER << " (bes-"<< CVER << ", " << libdap_name() << "-"
                    << libdap_version() << ")" << endl;
                return 0;

            case 'v':
#if 0
                build_dmrpp_util::verbose = true; // verbose hdf5 errors
#endif
                break;

            case 'd':
                BESDebug::SetUp(string("cerr,").append(DEBUG_KEY));
                break;

            case 'f':
                h4_file_name = optarg;
                break;

            case 'r':
                dmr_filename = optarg;
                break;

            case 'u':
                dmrpp_href_value = optarg;
                break;

            case 'c':
                bes_conf_file_used_to_create_dmr = optarg;
                break;

            case 'M':
                add_production_metadata = true;
                break;

            case 'h':
                usage();
                exit(EXIT_FAILURE);

            default:
                break;
        }
    }

    try {

#if 0
        // Check to see if the file is hdf5 compliant
        qc_input_file(h5_file_name);
#endif

        if (dmr_filename.empty()){
            stringstream msg;
            msg << "A DMR file for the granule '" << h4_file_name << "' must also be provided." << endl;
            throw BESInternalFatalError(msg.str(), __FILE__, __LINE__);
        }

        // Build the dmr++ from an existing DMR file.
#if 0
        build_dmrpp_from_dmr_file(
                dmrpp_href_value,
                dmr_filename,
                h4_file_name,
                add_production_metadata,
                bes_conf_file_used_to_create_dmr,
                argc,  argv);
#endif
    }
    catch (const BESError &e) {
        cerr << "BESError: " << e.get_message() << endl;
        return EXIT_FAILURE;
    }
    catch (const std::exception &e) {
        cerr << "std::exception: " << e.what() << endl;
        return EXIT_FAILURE;
    }
    catch (...) {
        cerr << "Unknown error." << endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
