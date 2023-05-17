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
#define ROOT_DIRECTORY "BES.Catalog.catalog.RootDirectory"

/**
 * @brief Recreate the command invocation given argv and argc.
 * @param argc
 * @param argv
 * @return The command
 */
static string cmdln(int argc, char *argv[])
{
    stringstream ss;
    for(int i=0; i<argc; i++) {
        if (i > 0)
            ss << " ";
        ss << argv[i];
    }
    return ss.str();
}

/**
 * @brief Write information to the the DMR++ about its provenance.
 * @param argc Used to determine how this DMR++ was built
 * @param argv Used to determine how this DMR++ was built
 * @param dmrpp Add provenance information to this instance of DMRpp
 * @note The DMRpp instance will free all memory allocated by this method.
 */
void inject_version_and_configuration(int argc, char **argv, DMRpp *dmrpp)
{
    dmrpp->set_version(CVER);

    // Build the version attributes for the DMR++
    auto version = new D4Attribute("build_dmrpp_metadata", StringToD4AttributeType("container"));

    auto build_dmrpp_version = new D4Attribute("build_dmrpp", StringToD4AttributeType("string"));
    build_dmrpp_version->add_value(CVER);
    version->attributes()->add_attribute_nocopy(build_dmrpp_version);

    auto bes_version = new D4Attribute("bes", StringToD4AttributeType("string"));
    bes_version->add_value(CVER);
    version->attributes()->add_attribute_nocopy(bes_version);

    stringstream ldv;
    ldv << libdap_name() << "-" << libdap_version();
    auto libdap4_version =  new D4Attribute("libdap", StringToD4AttributeType("string"));
    libdap4_version->add_value(ldv.str());
    version->attributes()->add_attribute_nocopy(libdap4_version);

    if(!TheBESKeys::ConfigFile.empty()) {
        // What is the BES configuration in play?
        auto config = new D4Attribute("configuration", StringToD4AttributeType("string"));
        config->add_value(TheBESKeys::TheKeys()->get_as_config());
        version->attributes()->add_attribute_nocopy(config);
    }

    // How was build_dmrpp invoked?
    auto invoke = new D4Attribute("invocation", StringToD4AttributeType("string"));
    invoke->add_value(cmdln(argc, argv));
    version->attributes()->add_attribute_nocopy(invoke);

    // Inject version and configuration attributes into DMR here.
    dmrpp->root()->attributes()->add_attribute_nocopy(version);
}

void usage() {
    const char *help = R"(
    build_dmrpp -h: Show this help

    build_dmrpp -V: Show build versions for components that make up the program

    build_dmrpp -c <bes.conf> -f <data file> [-u <href url>]: Build the DMR++ using the <bes.conf>
       options to initialize the software for the <data file>. Optionally substitute the <href url>.
       Builds the DMR using the HDF5 handler as configured using the options in the <bes.conf>.

    build_dmrpp build_dmrpp -f <data file> -r <dmr file> [-u <href url>]: As above, but uses the DMR
       read from the given file (so it does not run the HDF5 handler code and does not require the
       Metadata Store - MDS - be configured). Note that the get_dmrpp command appears to use this
       option and thus, the configuration options listed in the built DMR++ files lack the MDS setup.

    Other options:
        -v: Verbose
        -d: Turn on BES software debugging output
        -M: Add information about the build_dmrpp software, incl versions, to the built DMR++)";

    cerr << help << endl;
}

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

            case 'c':
                TheBESKeys::ConfigFile = optarg;
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

    if (h5_file_name.empty()) {
        cerr << "HDF5 file name must be given (-f <input>)." << endl;
        return EXIT_FAILURE;
    }

    try {
        // Turn off automatic hdf5 error printing.
        // See: https://support.hdfgroup.org/HDF5/doc1.8/RM/RM_H5E.html#Error-SetAuto2

        // For a given HDF5, get info for all the HDF5 datasets in a DMR or for a
        // given HDF5 dataset
        if (!dmr_name.empty()) {
            // Get dmr:
            DMRpp dmrpp;
            DmrppTypeFactory dtf;
            dmrpp.set_factory(&dtf);

            ifstream in(dmr_name.c_str());
            D4ParserSax2 parser;
            parser.intern(in, &dmrpp, false);

            add_chunk_information(h5_file_name, &dmrpp);

            if (add_production_metadata) {
                inject_version_and_configuration(argc, argv, &dmrpp);
            }

            XMLWriter writer;
            dmrpp.print_dmrpp(writer, url_name);
            cout << writer.get_doc();
        } else {
            string bes_data_root = TheBESKeys::TheKeys()->read_string_key(ROOT_DIRECTORY, "");
            if (bes_data_root.empty()) {
                cerr << "Could not find the data directory." << endl;
                return EXIT_FAILURE;
            }

            // Use the values from the bes.conf file... jhrg 5/21/18
            bes::DmrppMetadataStore *mds = bes::DmrppMetadataStore::get_instance();
            if (!mds) {
                cerr << "The Metadata Store (MDS) must be configured for this command to work (but see the -r option)." << endl;
                return EXIT_FAILURE;
            }

            // Use the full path to open the file, but use the 'name' (which is the
            // path relative to the BES Data Root) with the MDS.
            // Changed this to utilize assemblePath() because simply concatenating the strings
            // is fragile. - ndp 6/6/18
            string h5_file_path = BESUtil::assemblePath(bes_data_root, h5_file_name);

            //Use this ifstream file to run a check on the provided file's signature
            // to see if it is an HDF5 file
            ifstream file(h5_file_path, ios::binary);
            if (!file) {
                cerr << "Error opening file: " << h5_file_path << endl;
                cerr << "Cause of error: " << strerror(errno) << endl;
                return false;
            }

            //HDF5 and NetCDF3 signatures:
            const char hdf5Signature[] = { '\211', 'H', 'D', 'F', '\r', '\n', '\032', '\n' };
            const char netcdf3Signature[] = {'C', 'D', 'F'};

            //Read the first 8 bytes (file signature) from the file
            char signature[8];
            file.read(signature, 8);

            bool isHDF5 = memcmp(signature, hdf5Signature, sizeof(hdf5Signature)) == 0;
            if (!isHDF5) {
                //Reset the file stream to read from the beginning
                file.clear();
                file.seekg(0);

                char newSignature[3];
                file.read(newSignature, 3);

                bool isNetCDF3 = memcmp(newSignature, netcdf3Signature, sizeof(netcdf3Signature)) == 0;
                if (isNetCDF3) {
                    cerr << "The file submitted, " << h5_file_name << ", is a NetCDF-3 classic file and is not "
                            "compatible with dmr++ production at this time." << endl;
                    return EXIT_FAILURE;
                } else {
                    cerr << "The provided file," << h5_file_name << ", is neither an HDF5 or an NetCDF-4 file,"
                            " currently only HDF5 and NetCDF-4 files are supported for dmr++ production" << endl;
                    return EXIT_FAILURE;
                }
            }
            file.close();

            bes::DmrppMetadataStore::MDSReadLock lock = mds->is_dmr_available(h5_file_path, h5_file_name, "h5");
            if (lock()) {
                // parse the DMR into a DMRpp (that uses the DmrppTypes)
                unique_ptr<DMRpp> dmrpp(dynamic_cast<DMRpp *>(mds->get_dmr_object(h5_file_name /*h5_file_path*/)));
                if (!dmrpp) {
                    cerr << "Expected a DMR++ object from the DmrppMetadataStore." << endl;
                    return EXIT_FAILURE;
                }

                add_chunk_information(h5_file_path, dmrpp.get());

                dmrpp->set_href(url_name);

                mds->add_dmrpp_response(dmrpp.get(), h5_file_name /*h5_file_path*/);

                XMLWriter writer;
                dmrpp->set_print_chunks(true);
                dmrpp->print_dap4(writer);

                cout << writer.get_doc();
            } else {
                cerr << "Error: Could not get a lock on the DMR for '" + h5_file_path + "'." << endl;
                return EXIT_FAILURE;
            }
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
