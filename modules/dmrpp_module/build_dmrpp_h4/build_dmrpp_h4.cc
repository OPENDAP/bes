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
#include <BESInternalError.h>
#include <BESNotFoundError.h>

#include "build_dmrpp_util_h4.h"

using namespace std;
using namespace libdap;

using namespace dmrpp;
using namespace build_dmrpp_util_h4;


#define DEBUG_KEY "metadata_store,dmrpp_store,dmrpp"

void usage() {
    const char *help = R"(
    build_dmrpp_h4 -h: Show this help

    build_dmrpp_h4 -V: Show build versions for components that make up the program

    build_dmrpp_h4 -f <data file> -r <dmr file> [-u <href url>] [-M] [-D] [-v] [-V] [-d] 

    options:
        -f: HDF4/HDF-EOS2 file to build DMR++ from
        -r: DMR file to build DMR++ from
        -u: The href value to use in the DMR++ for the data file
        -M: Add information about this software, incl versions, to the built DMR++
        -D: Disable the generation of HDF-EOS2/HDF4 missing latitude/longitude
        -h: Show this help
        -v: Verbose information helpful for debugging and understanding the program working flow
        -V: Show build versions for components that make up the program
        -d: Turn on BES software debugging output)";

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
    bool disable_missing_data = false;

    int option_char = -1;
    while ((option_char = getopt(argc, argv, "c:f:r:u:dhvVMD")) != -1) {
        switch (option_char) {
            case 'V':
                cerr << basename(argv[0]) << "-" << CVER << " (bes-"<< CVER << ", " << libdap_name() << "-"
                    << libdap_version() << ")" << endl;
                return 0;

            case 'v':
                build_dmrpp_util_h4::verbose = true; // verbose hdf5 errors
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

            case 'D':
                disable_missing_data = true;
                break;

            case 'h':
                usage();
                exit(EXIT_FAILURE);

            default:
                break;
        }
    }

    // Check to see if the file is hdf4 compliant
    if (h4_file_name.empty()) {
        cerr << endl << "    The HDF4 file name must be provided with -f <h4_file_name> ." << endl;
        usage();
        return EXIT_FAILURE; 

    }
    try {
        qc_input_file(h4_file_name);
    }
    catch (BESInternalFatalError &e) {
         cerr<<"Fatal Error: "<<e.get_message() <<" at "<<e.get_file()<<":" <<e.get_line() << endl;
         return EXIT_FAILURE;
    }
    catch(BESNotFoundError &e) {
         cerr<<e.get_message() <<" at "<<e.get_file()<<":" <<e.get_line() <<endl;
         return EXIT_FAILURE;
    }
    catch(BESInternalError  &e) {
         cerr<<"Internal Error: "<<e.get_message() <<" at "<<e.get_file()<<":" <<e.get_line() << endl;
         return EXIT_FAILURE;
    }
    catch (std::exception &se) {
        cerr << "Caught std::exception! Message: " << se.what() <<"at "<<__FILE__<<":" <<__LINE__<<endl;
        return EXIT_FAILURE;
    }
    catch(...) {
        cerr << "Caught unknown exception" <<"at "<<__FILE__<<":" <<__LINE__<<endl;
        return EXIT_FAILURE;
    }

    if (dmr_filename.empty()){
        cerr << endl <<"    A DMR file for the granule '" << h4_file_name << "' must also be provided with -r <h4_dmrpp_file_name> ." << endl;
        usage();
        return EXIT_FAILURE;
    }

    // Build the dmr++ from an existing DMR file.
    try {
        build_dmrpp_from_dmr_file(
            dmrpp_href_value,
            dmr_filename,
            h4_file_name,
            add_production_metadata,
            disable_missing_data,
            bes_conf_file_used_to_create_dmr,
            argc,  argv);
    }
    catch(BESNotFoundError &e) {
         cerr<<e.get_message() <<" at "<<e.get_file()<<":" <<e.get_line() <<endl;
         return EXIT_FAILURE;
    }
    catch(BESInternalError  &e) {
         cerr<<"Internal Error: "<<e.get_message() <<" at "<<e.get_file()<<":" <<e.get_line() << endl;
         return EXIT_FAILURE;
    }
    catch (BESInternalFatalError &e) {
         cerr<<"Fatal Error: "<<e.get_message() <<" at "<<e.get_file()<<":" <<e.get_line() << endl;
         return EXIT_FAILURE;
    }
    catch (std::exception &se) {
        cerr << "Caught std::exception! Message: " << se.what() <<"at "<<__FILE__<<":" <<__LINE__<<endl;
        return EXIT_FAILURE;
    }
    catch(...) {
        cerr << "Caught unknown exception" <<"at "<<__FILE__<<":" <<__LINE__<<endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
