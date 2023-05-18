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

/**
 *
 * @param file_name
 * @return
 */
string qc_input_file(const string &file_name){
    //Use this ifstream file to run a check on the provided file's signature
    // to see if it is an HDF5 file. - kln 5/18/23

    if (file_name.empty()) {
        stringstream msg;
        msg << "HDF5 input file name must be given (-f <input>)." << endl;
        throw BESInternalFatalError(msg.str(), __FILE__, __LINE__);
    }

    string bes_data_root = TheBESKeys::TheKeys()->read_string_key(ROOT_DIRECTORY, "");
    if (bes_data_root.empty()) {
        stringstream msg;
        cerr << "Could not locate the data directory." << endl;
        throw BESInternalFatalError(msg.str(), __FILE__, __LINE__);
    }

    string file_fqn = BESUtil::assemblePath(bes_data_root, file_name);


    std::ifstream file(file_fqn, ios::binary);
    auto errnum = errno;
    if (!file)  // This is same as if(file.fail()){...}
    {

        stringstream msg;
        msg << "Encountered a Read/writing error when attempting to open the file" << file_fqn << endl;
        msg << "*          failbit: " << (((file.rdstate() & std::ifstream::failbit) != 0) ? "true" : "false") << endl;
        msg << "*           badbit: " << (((file.rdstate() & std::ifstream::badbit) != 0) ? "true" : "false") << endl;
        msg << "*            errno: " << errnum << endl;
        msg << "*  strerror(errno): " << strerror(errnum) << endl;
        msg << "Things to check:" << endl;
        msg << "* Does the file exist at expected location?" << endl;
        msg << "* Does your user have permission to read the file?" << endl;
        throw BESInternalFatalError(msg.str(), __FILE__, __LINE__);
    }

    //HDF5 and NetCDF3 signatures:
    const char hdf5Signature[] = {'\211', 'H', 'D', 'F', '\r', '\n', '\032', '\n'};
    const char netcdf3Signature[] = {'C', 'D', 'F'};

    //Read the first 8 bytes (file signature) from the file
    char signature[8];
    file.read(signature, 8);

    //First check if file is NOT an HDF5 file, then, if it is not, check if it is netcdf3
    bool isHDF5 = memcmp(signature, hdf5Signature, sizeof(hdf5Signature)) == 0;
    if (!isHDF5) {
        //Reset the file stream to read from the beginning
        file.clear();
        file.seekg(0);

        char newSignature[3];
        file.read(newSignature, 3);

        bool isNetCDF3 = memcmp(newSignature, netcdf3Signature, sizeof(netcdf3Signature)) == 0;
        if (isNetCDF3) {
            stringstream msg;
            msg << "The file submitted, " << file_fqn << ", ";
            msg << "is a NetCDF-3 classic file and is not compatible with dmr++ production at this time." << endl;
            throw BESInternalFatalError(msg.str(), __FILE__, __LINE__);
        }
        else {
            stringstream msg;
            msg << "The provided file," << file_fqn << ", ";
            msg << "is neither an HDF5 or an NetCDF-4 file, currently only HDF5 and NetCDF-4 files ";
            msg << "are supported for dmr++ production" << endl;
            throw BESInternalFatalError(msg.str(), __FILE__, __LINE__);
        }
    }
    return file_fqn;
}

/**
 * Use the full path to open the file, but use the 'name' (which is the
 * path relative to the BES Data Root) with the MDS.
 * Changed this to utilize assemblePath() because simply concatenating the strings
 * is fragile. - ndp 6/6/18
 * @param url_name
 * @param h5_file_path
 * @param h5_file_name
 */
void check_mds(const string &url_name, const string &h5_file_path, const string &h5_file_name)
{
    // Use the values from the bes.conf file... jhrg 5/21/18
    bes::DmrppMetadataStore *mds = bes::DmrppMetadataStore::get_instance();
    if (!mds) {
        stringstream msg;
        msg << "The Metadata Store (MDS) must be configured for this command to work (but see the -r option)." << endl;
        throw BESInternalFatalError(msg.str(), __FILE__, __LINE__);
    }

    bes::DmrppMetadataStore::MDSReadLock lock = mds->is_dmr_available(h5_file_path, h5_file_name, "h5");
    if (lock()) {
        // parse the DMR into a DMRpp (that uses the DmrppTypes)
        unique_ptr<DMRpp> dmrpp(dynamic_cast<DMRpp *>(mds->get_dmr_object(h5_file_name /*h5_file_path*/)));
        if (!dmrpp) {
            stringstream msg;
            msg << "Expected a DMR++ object from the DmrppMetadataStore." << endl;
            throw BESInternalFatalError(msg.str(), __FILE__, __LINE__);
        }

        add_chunk_information(h5_file_path, dmrpp.get());

        dmrpp->set_href(url_name);

        mds->add_dmrpp_response(dmrpp.get(), h5_file_name /*h5_file_path*/);

        XMLWriter writer;
        dmrpp->set_print_chunks(true);
        dmrpp->print_dap4(writer);

        cout << writer.get_doc();
    } else {
        stringstream msg;
        msg << "Error: Could not get a lock on the DMR for '" + h5_file_path + "'." << endl;
        throw BESInternalFatalError(msg.str(), __FILE__, __LINE__);
    }
}

/**
 *
 * @param url_name
 * @param dmr_name
 * @param h5_file_name
 * @param add_production_metadata
 * @param argc
 * @param argv
 */
void build_dmrpp_from_dmr(
        const string &url_name,
        const string &dmr_name,
        const string &h5_file_name,
        bool add_production_metadata,
        int argc, char *argv[])
{
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

    try {

        string h5_file_path = qc_input_file(h5_file_name);

        if (!dmr_name.empty()) {
            // Build the dmr++ from an exisiting DMR file.
            build_dmrpp_from_dmr( url_name,  dmr_name,  h5_file_name,  add_production_metadata,  argc,  argv);
        }
        else {
            // No existing DMR file? Check the MDS!
            check_mds(url_name, h5_file_path, h5_file_name);
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
