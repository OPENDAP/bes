// -*- mode: c++; c-basic-offset:4 -*-
//
// FoDapCovJsonTransmitter.cc
//
// This file is part of BES CovJSON File Out Module
//
// Copyright (c) 2018 OPeNDAP, Inc.
// Author: Corey Hemphill <hemphilc@oregonstate.edu>
// Author: River Hendriksen <hendriri@oregonstate.edu>
// Author: Riley Rimer <rrimer@oregonstate.edu>
//
// Adapted from the File Out JSON module implemented by Nathan Potter
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

#include <libdap/DataDDS.h>
#include <libdap/BaseType.h>
#include <libdap/escaping.h>
#include <libdap/ConstraintEvaluator.h>

#include <BESUtil.h>
#include <BESInternalError.h>
#include <BESSyntaxUserError.h>
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

#include "FoDapCovJsonTransmitter.h"
#include "FoDapCovJsonTransform.h"

using namespace ::libdap;

#define FO_COVJSON_TEMP_DIR "/tmp"

string FoDapCovJsonTransmitter::temp_dir;

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
FoDapCovJsonTransmitter::FoDapCovJsonTransmitter() : BESTransmitter()
{
    add_method(DATA_SERVICE, FoDapCovJsonTransmitter::send_data);
    add_method(DDX_SERVICE,  FoDapCovJsonTransmitter::send_metadata);

    add_method(DAP4DATA_SERVICE, FoDapCovJsonTransmitter::send_dap4data);
    add_method(DMR_SERVICE,  FoDapCovJsonTransmitter::send_dap4metadata);


    if (FoDapCovJsonTransmitter::temp_dir.empty()) {
        // Where is the temp directory for creating these files
        bool found = false;
        string key = "FoCovJson.Tempdir";
        TheBESKeys::TheKeys()->get_value(key, FoDapCovJsonTransmitter::temp_dir, found);
        if (!found || FoDapCovJsonTransmitter::temp_dir.empty()) {
            FoDapCovJsonTransmitter::temp_dir = FO_COVJSON_TEMP_DIR;
        }
        string::size_type len = FoDapCovJsonTransmitter::temp_dir.size();
        if (FoDapCovJsonTransmitter::temp_dir[len - 1] == '/') {
            FoDapCovJsonTransmitter::temp_dir = FoDapCovJsonTransmitter::temp_dir.substr(0, len - 1);
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
void FoDapCovJsonTransmitter::send_data(BESResponseObject *obj, BESDataHandlerInterface &dhi)
{
    BESDEBUG("focovjson", "FoDapCovJsonTransmitter::send_data - BEGIN" << endl);

    try {
        BESDapResponseBuilder responseBuilder;

        BESDEBUG("focovjson", "FoCovJsonTransmitter::send_data - Reading data into DataDDS" << endl);

        // The response object will manage loaded_dds
        // Use the DDS from the ResponseObject along with the parameters
        // from the DataHandlerInterface to load the DDS with values.
        // Note that the BESResponseObject will manage the loaded_dds object's
        // memory. Make this a shared_ptr<>. jhrg 9/6/16

        // TODO improve this pattern by enabling reading then transforming one varaible
        // at a time so that the whole dataset is not loaded into memory. jhrg 6/11/20
        DDS *loaded_dds;
        try {
            // Added this try block to debug memory use issues in this handler. jhrg 6/11/20
            loaded_dds = responseBuilder.intern_dap2_data(obj, dhi);
        }
        catch(std::exception &e) {
            throw BESSyntaxUserError(string("Caught a C++ standard exception in responseBuilder.intern_dap2_data. The error was: ").append(e.what()), __FILE__, __LINE__);
        }

        ostream &o_strm = dhi.get_output_stream();
        if (!o_strm)
            throw BESInternalError("Output stream is not set, can not return as COVJSON", __FILE__, __LINE__);

        FoDapCovJsonTransform ft(loaded_dds);
        ft.transform(o_strm, true, false); // Send metadata and data; Test override false
        //ft.transform(o_strm, true, false); // Send metadata and data; Test override false
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

    BESDEBUG("focovjson", "FoDapCovJsonTransmitter::send_data - done transmitting COVJSON" << endl);
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
void FoDapCovJsonTransmitter::send_metadata(BESResponseObject *obj, BESDataHandlerInterface &dhi)
{
    BESDEBUG("focovjson", "FoDapCovJsonTransmitter::send_data - BEGIN transmitting COVJSON" << endl);

    try {
        BESDapResponseBuilder responseBuilder;

        // processed_dds managed by response builder
        DDS *processed_dds = responseBuilder.process_dap2_dds(obj, dhi);

        ostream &o_strm = dhi.get_output_stream();
        if (!o_strm)
            throw BESInternalError("Output stream is not set, can not return as COVJSON", __FILE__, __LINE__);

        FoDapCovJsonTransform ft(processed_dds);

        // Now that we are ready to start building the response data we
        // cancel any pending timeout alarm according to the configuration.
        BESUtil::conditional_timeout_cancel();

        ft.transform(o_strm, false, false); // Send metadata only; Test override false
        //ft.transform(o_strm, false, false); // Send metadata only; Test override false
    }
    catch (Error &e) {
        throw BESDapError("Failed to transform data to COVJSON: " + e.get_error_message(), false, e.get_error_code(),
            __FILE__, __LINE__);
    }
    catch (BESError &e) {
        throw;
    }
    catch (...) {
        throw BESInternalError("Failed to transform to COVJSON: Unknown exception caught", __FILE__, __LINE__);
    }

    BESDEBUG("focovjson", "FoDapCovJsonTransmitter::send_data - done transmitting COVJSON" << endl);
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
void FoDapCovJsonTransmitter::send_dap4data(BESResponseObject *obj, BESDataHandlerInterface &dhi)
{
    BESDEBUG("focovjson", "FoDapCovJsonTransmitter::send_dap4data - BEGIN" << endl);

    try {
        BESDapResponseBuilder responseBuilder;

        BESDEBUG("focovjson", "FoCovJsonTransmitter::send_dap4data - Reading data into DMR" << endl);

        // The response object will manage loaded_dds
        // Use the DDS from the ResponseObject along with the parameters
        // from the DataHandlerInterface to load the DDS with values.
        // Note that the BESResponseObject will manage the loaded_dds object's
        // memory. Make this a shared_ptr<>. jhrg 9/6/16

        // TODO improve this pattern by enabling reading then transforming one varaible
        // at a time so that the whole dataset is not loaded into memory. jhrg 6/11/20
        DMR *loaded_dmr;
        try {
            // Added this try block to debug memory use issues in this handler. jhrg 6/11/20
            loaded_dmr = responseBuilder.intern_dap4_data(obj, dhi);
        }
        catch(std::exception &e) {
            throw BESSyntaxUserError(string("Caught a C++ standard exception in responseBuilder.intern_dap4_data. The error was: ").append(e.what()), __FILE__, __LINE__);
        }

        ostream &o_strm = dhi.get_output_stream();
        if (!o_strm)
            throw BESInternalError("Output stream is not set, can not return as COVJSON", __FILE__, __LINE__);

        FoDapCovJsonTransform ft(loaded_dmr);
        ft.transform_dap4(o_strm, true, false); // Send metadata and data; Test override false
    }
    catch (Error &e) {
        throw BESDapError("Failed to read dap4 data: " + e.get_error_message(), false, e.get_error_code(), __FILE__, __LINE__);
    }
    catch (std::exception &e) {
        throw BESInternalError("Failed to read dap4 data: STL Error: " + string(e.what()), __FILE__, __LINE__);
    }
    catch (...) {
        throw BESInternalError("Failed to read dap4 data: Unknown exception caught", __FILE__, __LINE__);
    }

    BESDEBUG("focovjson", "FoDapCovJsonTransmitter::send_dap4data - done transmitting COVJSON" << endl);
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
void FoDapCovJsonTransmitter::send_dap4metadata(BESResponseObject *obj, BESDataHandlerInterface &dhi)
{
    BESDEBUG("focovjson", "FoDapCovJsonTransmitter::send_dap4metadata - BEGIN transmitting COVJSON" << endl);

    try {
        BESDapResponseBuilder responseBuilder;

        // processed_dds managed by response builder
        DMR *processed_dmr = responseBuilder.process_dap4_dmr(obj, dhi);

        ostream &o_strm = dhi.get_output_stream();
        if (!o_strm)
            throw BESInternalError("Output stream is not set, can not return as COVJSON", __FILE__, __LINE__);

        FoDapCovJsonTransform ft(processed_dmr);

        // Now that we are ready to start building the response data we
        // cancel any pending timeout alarm according to the configuration.
        BESUtil::conditional_timeout_cancel();

        ft.transform_dap4(o_strm, false, false); // Send metadata only; Test override false
    }
    catch (Error &e) {
        throw BESDapError("Failed to transform data to COVJSON: " + e.get_error_message(), false, e.get_error_code(),
            __FILE__, __LINE__);
    }
    catch (...) {
        throw BESInternalError("Failed to transform to COVJSON: Unknown exception caught", __FILE__, __LINE__);
    }

    BESDEBUG("focovjson", "FoDapCovJsonTransmitter::send_dap4metadata - done transmitting COVJSON" << endl);
}
