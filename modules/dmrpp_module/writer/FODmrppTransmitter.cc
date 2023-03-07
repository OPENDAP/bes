// -*- mode: c++; c-basic-offset:4 -*-
//
// FoDapJsonTransmitter.cc
//
// This file is part of BES JSON File Out Module
//
// Copyright (c) 2014 OPeNDAP, Inc.
// Author: Nathan Potter <ndp@opendap.org>
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
// (c) COPYRIGHT URI/MIT 1995-1999
// Please read the full copyright statement in the file COPYRIGHT_URI.
//

#include "config.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <exception>
#include <sstream>      // std::stringstream
#include <thread>
#include <future>

#include <libdap/util.h>
#include <libdap/D4Attributes.h>
#include <libdap/D4ParserSax2.h>

#include <BESContextManager.h>
#include <BESDataDDSResponse.h>
#include <BESDapNames.h>
#include <BESDataNames.h>
#include <BESDebug.h>
#include <BESUtil.h>
#include <TempFile.h>
#include <RequestServiceTimer.h>

#include <BESLog.h>
#include <BESError.h>
#include <BESInternalFatalError.h>
#include <BESDapError.h>
#include "BESDMRResponse.h"

#include "DMRpp.h"
#include "DmrppTypeFactory.h"
#include "DmrppD4Group.h"

#include "FODmrppTransmitter.h"

using namespace libdap;
using namespace std;

#define MODULE "dmrpp"
#define prolog string("FODmrppTransmitter::").append(__func__).append("() - ")

/** @brief Construct the FoW10nJsonTransmitter
 *
 *
 * The transmitter is created to add the ability to return OPeNDAP data
 * objects (DataDDS) as abstract object representation JSON documents.
 *
 * The OPeNDAP data object is written to a JSON file locally in a
 * temporary directory specified by the BES configuration parameter
 * FoJson.Tempdir. If this variable is not found or is not set then it
 * defaults to the macro definition FO_JSON_TEMP_DIR.
 */
FODmrppTransmitter::FODmrppTransmitter() :
    BESTransmitter()
{
    add_method(DATA_SERVICE, FODmrppTransmitter::send_dmrpp);

    /*if (FODmrppTransmitter::temp_dir.empty()) {
        // Where is the temp directory for creating these files
        bool found = false;
        string key = "FoDmrpp.Tempdir";
        TheBESKeys::TheKeys()->get_value(key, FODmrppTransmitter::temp_dir, found);
        if (!found || FODmrppTransmitter::temp_dir.empty()) {
        	FODmrppTransmitter::temp_dir = FO_DMRPP_TEMP_DIR;
        }
        string::size_type len = FODmrppTransmitter::temp_dir.size();
        if (FODmrppTransmitter::temp_dir[len - 1] == '/') {
        	FODmrppTransmitter::temp_dir = FODmrppTransmitter::temp_dir.substr(0, len - 1);
        }
    }*/
}

/**
 * Follow the send_data()
 * @brief The static method registered to transmit OPeNDAP data objects as
 * a netcdf file.
 *
 * This function takes the OPeNDAP DMR object, reads in the data (can be
 * used with any data handler), transforms the data into a netcdf file, and
 * streams back that netcdf file back to the requester using the stream
 * specified in the BESDataHandlerInterface.
 *
 * @param obj The BESResponseObject containing the OPeNDAP DataDDS object
 * @param dhi BESDataHandlerInterface containing information about the
 * request and response
 * @throws BESInternalError if the response is not an OPeNDAP DMR or if
 * there are any problems reading the data, writing to a netcdf file, or
 * streaming the netcdf file
 */
void FODmrppTransmitter::send_dmrpp(BESResponseObject *obj, BESDataHandlerInterface &dhi)
{
    BESDEBUG(MODULE,  prolog << "BEGIN" << endl);

    auto bdmr = dynamic_cast<BESDMRResponse *>(obj);
    if (!bdmr) throw BESInternalFatalError("Expected a BESDMRResponse instance", __FILE__, __LINE__);
    auto dmr = bdmr->get_dmr();

    //string base_name = dmr->filename().substr(dmr->filename().find_last_of("/\\") + 1);

    // This object closes the file when it goes out of scope.
    /*bes::TempFile temp_file;
    string temp_file_name = temp_file.create(FODmrppRequestHandler::temp_dir,  "dmrpp_"+base_name);
*/
//    BESDEBUG(MODULE,  prolog << "Building response file " << temp_file_name << endl);

    try {
        dmrpp::DMRpp dmrpp;
        dmrpp::DmrppTypeFactory dtf;
        dmrpp.set_factory(&dtf);

        D4ParserSax2 parser;
        /*parser.intern(in, &dmrpp, false);

        add_chunk_information(h5_file_name, &dmrpp);

        if (add_production_metadata) {
            inject_version_and_configuration(argc, argv, &dmrpp);
        }

        XMLWriter writer;
        dmrpp.print_dmrpp(writer, url_name);
        cout << writer.get_doc();*/

        ostream &strm = dhi.get_output_stream();

#if !NDEBUG
        stringstream msg;
        msg << prolog << "Using ostream: " << (void *) &strm << endl;
        BESDEBUG(MODULE, msg.str());
        INFO_LOG(msg.str());
#endif

        if (!strm) throw BESInternalError("Output stream is not set, can not return as", __FILE__, __LINE__);

        // Verify the request hasn't exceeded bes_timeout, and disable timeout if allowed.
        RequestServiceTimer::TheTimer()->throw_if_timeout_expired("ERROR: bes-timeout expired before transmit",
                                                                  __FILE__, __LINE__);
        BESUtil::conditional_timeout_cancel();

//        BESDEBUG(MODULE,  prolog << "Transmitting temp file " << temp_file_name << endl);

        // FONcTransmitter::write_temp_file_to_stream(temp_file.get_fd(), strm); //, loaded_dds->filename(), ncVersion);
//        BESUtil::file_to_stream(temp_file_name,strm);
    }
    catch (Error &e) {
        throw BESDapError("Failed to read data: " + e.get_error_message(), false, e.get_error_code(), __FILE__, __LINE__);
    }
    catch (BESError &e) {
        throw;
    }
    catch (std::exception &e) {
        throw BESInternalError("Failed to read data: STL Error: " + string(e.what()), __FILE__, __LINE__);
    }
    catch (...) {
        throw BESInternalError("Failed to get read data: Unknown exception caught", __FILE__, __LINE__);
    }

    BESDEBUG(MODULE,  prolog << "END  Transmitted DMRPP XML" << endl);
}
