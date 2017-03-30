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

#include <stdio.h>
#include <stdlib.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <sys/types.h>                  // For umask
#include <sys/stat.h>

#include <iostream>
#include <fstream>

#include <DataDDS.h>
#include <BaseType.h>
#include <escaping.h>
#include <ConstraintEvaluator.h>

#include <BESInternalError.h>
#include <BESDapError.h>
#include <TheBESKeys.h>
#include <BESContextManager.h>
#include <BESDataDDSResponse.h>
#include <BESDDSResponse.h>
#include <BESDapNames.h>
#include <BESDataNames.h>
#include <BESDapResponseBuilder.h>
#include <BESDebug.h>
#include <DapFunctionUtils.h>

#include "FoDapJsonTransmitter.h"
#include "FoDapJsonTransform.h"

using namespace ::libdap;

#define FO_JSON_TEMP_DIR "/tmp"

string FoDapJsonTransmitter::temp_dir;

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
FoDapJsonTransmitter::FoDapJsonTransmitter() : BESBasicTransmitter()
{
    add_method(DATA_SERVICE, FoDapJsonTransmitter::send_data);
    add_method(DDX_SERVICE,  FoDapJsonTransmitter::send_metadata);

    if (FoDapJsonTransmitter::temp_dir.empty()) {
        // Where is the temp directory for creating these files
        bool found = false;
        string key = "FoJson.Tempdir";
        TheBESKeys::TheKeys()->get_value(key, FoDapJsonTransmitter::temp_dir, found);
        if (!found || FoDapJsonTransmitter::temp_dir.empty()) {
        	FoDapJsonTransmitter::temp_dir = FO_JSON_TEMP_DIR;
        }
        string::size_type len = FoDapJsonTransmitter::temp_dir.length();
        if (FoDapJsonTransmitter::temp_dir[len - 1] == '/') {
        	FoDapJsonTransmitter::temp_dir = FoDapJsonTransmitter::temp_dir.substr(0, len - 1);
        }
    }
}

/** @brief The static method registered to transmit OPeNDAP data objects as
 * a JSON file.
 *
 * This function takes the OPeNDAP DataDDS object, reads in the data (can be
 * used with any data handler), transforms the data into a JSON file, and
 * streams back that JSON file back to the requester using the stream
 * specified in the BESDataHandlerInterface.
 *
 * @param obj The BESResponseObject containing the OPeNDAP DataDDS object
 * @param dhi BESDataHandlerInterface containing information about the
 * request and response
 * @throws BESInternalError if the response is not an OPeNDAP DataDDS or if
 * there are any problems reading the data, writing to a JSON file, or
 * streaming the JSON file
 */
void FoDapJsonTransmitter::send_data(BESResponseObject *obj, BESDataHandlerInterface &dhi)
{
    BESDEBUG("fojson", "FoDapJsonTransmitter::send_data - BEGIN" << endl);

    try {
        BESDapResponseBuilder responseBuilder;

        BESDEBUG("fojson", "FoJsonTransmitter::send_data - Reading data into DataDDS" << endl);

        // The response object will manage loaded_dds
        // Use the DDS from the ResponseObject along with the parameters
        // from the DataHandlerInterface to load the DDS with values.
        // Note that the BESResponseObject will manage the loaded_dds object's
        // memory. Make this a shared_ptr<>. jhrg 9/6/16
        DDS *loaded_dds = responseBuilder.intern_dap2_data(obj, dhi);

        ostream &o_strm = dhi.get_output_stream();
        if (!o_strm)
            throw BESInternalError("Output stream is not set, can not return as JSON", __FILE__, __LINE__);

        FoDapJsonTransform ft(loaded_dds);

        // Now that we are ready to start building the response data we
        // cancel any pending timeout alarm according to the configuration.
        conditional_timeout_cancel();

        ft.transform(o_strm, true /* send data */);
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

    BESDEBUG("fojson", "FoDapJsonTransmitter::send_data - done transmitting JSON" << endl);
}

/** @brief The static method registered to transmit OPeNDAP data objects as
 * a JSON file.
 *
 * This function takes the OPeNDAP DataDDS object, reads in the data (can be
 * used with any data handler), transforms the data into a JSON file, and
 * streams back that JSON file back to the requester using the stream
 * specified in the BESDataHandlerInterface.
 *
 * @param obj The BESResponseObject containing the OPeNDAP DataDDS object
 * @param dhi BESDataHandlerInterface containing information about the
 * request and response
 * @throws BESInternalError if the response is not an OPeNDAP DataDDS or if
 * there are any problems reading the data, writing to a JSON file, or
 * streaming the JSON file
 */
void FoDapJsonTransmitter::send_metadata(BESResponseObject *obj, BESDataHandlerInterface &dhi)
{
    BESDEBUG("fojson", "FoDapJsonTransmitter::send_data - BEGIN transmitting JSON" << endl);

    try {
        BESDapResponseBuilder responseBuilder;

        // processed_dds managed by response builder
        DDS *processed_dds = responseBuilder.process_dap2_dds(obj, dhi);

        ostream &o_strm = dhi.get_output_stream();
        if (!o_strm)
            throw BESInternalError("Output stream is not set, can not return as JSON", __FILE__, __LINE__);

        FoDapJsonTransform ft(processed_dds);

        // Now that we are ready to start building the response data we
        // cancel any pending timeout alarm according to the configuration.
        conditional_timeout_cancel();

        ft.transform(o_strm, false /* do not send data */);
    }
    catch (Error &e) {
        throw BESDapError("Failed to transform data to JSON: " + e.get_error_message(), false, e.get_error_code(),
            __FILE__, __LINE__);
    }
    catch (BESError &e) {
        throw;
    }
    catch (...) {
        throw BESInternalError("Failed to transform to JSON: Unknown exception caught", __FILE__, __LINE__);
    }

    BESDEBUG("fojson", "FoDapJsonTransmitter::send_data - done transmitting JSON" << endl);
}
