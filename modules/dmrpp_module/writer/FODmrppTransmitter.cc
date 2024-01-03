// -*- mode: c++; c-basic-offset:4 -*-
//
// FoDapJsonTransmitter.cc
//
// This file is part of BES DMRPP Module
//
// Copyright (c) 2023 OPeNDAP, Inc.
// Author: Nathan Potter <ndp@opendap.org>
// Author: Daniel Holloway <dholloway@opendap.org>
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
#include <BESDapNames.h>
#include <BESDebug.h>
#include <BESUtil.h>
#include <RequestServiceTimer.h>

#include <BESLog.h>
#include <BESError.h>
#include <BESInternalFatalError.h>
#include <BESDapError.h>

#include "BESDMRResponse.h"

#include "DMRpp.h"
#include "DmrppTypeFactory.h"
#include "DmrppD4Group.h"

#include "build_dmrpp_util.h"
#include "FODmrppTransmitter.h"

#include "DapUtils.h"

using namespace libdap;
using namespace std;

#define MODULE "dmrpp"
#define prolog string("FODmrppTransmitter::").append(__func__).append("() - ")

/** @brief BESTransmitter class named "dmrpp" that transmits an OPeNDAP
 * data object as a DMRPP file
 *
 * The FoDapDmrppTransmitter transforms an OPeNDAP DMR object into a
 * DMRPP file and streams the new (temporary) DMRPP file back to the
 * client.
 *
 */
FODmrppTransmitter::FODmrppTransmitter() :
    BESTransmitter()
{
    add_method(DAP4DATA_SERVICE, FODmrppTransmitter::send_dmrpp);
}

/**
 * Follow the send_dmrpp()
 * @brief The static method registered to transmit OPeNDAP DMRPP XML metadata.
 *
 * This function takes the OPeNDAP DMR object, reads in the data (can be
 * used with any data handler), transforms the DMR into a DMRPP, and
 * streams back that DMRPP XML back to the requester using the stream
 * specified in the BESDataHandlerInterface.
 *
 * @param obj The BESResponseObject containing the OPeNDAP DataDDS object
 * @param dhi BESDataHandlerInterface containing information about the
 * request and response
 * @throws BESInternalError if the response is not an OPeNDAP DMR or if
 * there are any problems reading the data, generating DMRPP XML, or
 * streaming the DMRPP XML
 */
void FODmrppTransmitter::send_dmrpp(BESResponseObject *obj, BESDataHandlerInterface &dhi)
{
    BESDEBUG(MODULE,  prolog << "BEGIN" << endl);

    bool add_production_metadata = true;

    auto bdmr = dynamic_cast<BESDMRResponse *>(obj);
    if (!bdmr) throw BESInternalFatalError("Expected a BESDMRResponse instance", __FILE__, __LINE__);
    auto dmr = bdmr->get_dmr();

    try {
        dmrpp::DMRpp dmrpp;
        dmrpp::DmrppTypeFactory dtf;
        dmrpp.set_factory(&dtf);

        XMLWriter dmr_writer;
        dmr->print_dap4(dmr_writer, false);

        // WORKAROUND: Need to write DMR to stringstream to strip HDF5 specific content
        // for subsequent parsing by D4ParserSax3 parser.
        ostringstream oss;
        oss << dmr_writer.get_doc();

        istringstream iss(oss.str());

        D4ParserSax2 parser;
        parser.intern(iss, &dmrpp, false);

        // WORKAROUND: Because this Transmitter is called after the H5 ResponseHandler building
        // the DMR, dhi.container is set to null by BESDap4ResponseHandler::execute_each().
        // BUT we need access to the container::d_real_name so we use an iterator to the dhi.containers
        // to get the first container.
        string dataset_name = (*(dhi.containers.begin()))->access();
        string href_url = (*(dhi.containers.begin()))->get_real_name();

        build_dmrpp_util::add_chunk_information(dataset_name, &dmrpp);

        if (add_production_metadata) {
            build_dmrpp_util::inject_version_and_configuration(&dmrpp);
        }

        XMLWriter dmrpp_writer;
        dmrpp.print_dmrpp(dmrpp_writer, href_url);

        dap_utils::log_response_and_memory_size(prolog, dmrpp_writer);

        auto &strm = dhi.get_output_stream();
        strm << dmrpp_writer.get_doc() << flush;

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
