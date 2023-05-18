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
#include <memory>
#include <iterator>

#include <unistd.h>
#include <cstdlib>
#include <libgen.h>

#include <libdap/Array.h>
#include <libdap/util.h>
#include <libdap/D4Attributes.h>
#include <libdap/D4ParserSax2.h>

#include <TheBESKeys.h>
#include <BESUtil.h>
#include <BESDebug.h>
#include <BESError.h>
#include <BESInternalError.h>
#include <BESInternalFatalError.h>

#include "DMRpp.h"
#include "DmrppTypeFactory.h"
#include "DmrppD4Group.h"
#include "DmrppMetadataStore.h"

#include "build_dmrpp_util.h"

using namespace std;
using namespace libdap;
using namespace dmrpp;
using namespace build_dmrpp_util;

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
    string h5_file_name;
    string h5_dset_path;
    string dmr_name;
    string url_name;
    bool add_production_metadata = false;

    int option_char;
    while ((option_char = getopt(argc, argv, "c:f:r:u:dhvVM")) != -1) {
        switch (option_char) {
            case 'V':
                cerr << basename(argv[0]) << "-" << CVER << " (bes-"<< CVER << ", " << libdap_name() << "-"
                    << libdap_version() << ")" << endl;
                return 0;

            case 'v':
                build_dmrpp_util::verbose = true; // verbose hdf5 errors
                break;

            case 'd':
                BESDebug::SetUp(string("cerr,").append(DEBUG_KEY));
                break;

            case 'f':
                h5_file_name = optarg;
                break;

            case 'r':
                dmr_name = optarg;
                break;

            case 'u':
                url_name = optarg;
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

        qc_input_file(h5_file_name);

        if (!dmr_name.empty()) {
            // Build the dmr++ from an existing DMR file.
            build_dmrpp_from_dmr( url_name,  dmr_name,  h5_file_name,  add_production_metadata,  argc,  argv);
        }
        else {
            stringstream msg;
            msg << "A DMR fully file for the granule '" << h5_file_name << " must also be provided." << endl;
            throw BESInternalFatalError(msg.str(), __FILE__, __LINE__);
        }
    }
    catch (const BESError &e) {
        cerr << "Error: " << e.get_message() << endl;
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
