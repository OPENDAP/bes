// -*- mode: c++; c-basic-offset:4 -*-
//
// W10nJsonTransmitter.cc
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

#include <libdap/DataDDS.h>
#include <libdap/BaseType.h>
#include <libdap/escaping.h>
#include <libdap/ConstraintEvaluator.h>

#include <BESUtil.h>
#include <BESInternalError.h>
#include <BESDapError.h>
#include <BESDapError.h>
#include <TheBESKeys.h>
#include <BESContextManager.h>
#include <BESDataDDSResponse.h>
#include <BESDDSResponse.h>
#include <BESDapError.h>
#include <BESDapNames.h>
#include <BESDataNames.h>
#include <BESDebug.h>
#include <BESStopWatch.h>
#include <BESSyntaxUserError.h>
#include <BESDapResponseBuilder.h>
#include <RequestServiceTimer.h>

#include "W10nJsonTransmitter.h"

#include "W10nJsonTransform.h"
#include "W10NNames.h"
#include "w10n_utils.h"

using namespace ::libdap;

#define MODULE "w10n"
#define prolog string("W10nJsonTransmitter::").append(__func__).append("() - ")

#define W10N_JSON_TEMP_DIR "/tmp"

string W10nJsonTransmitter::temp_dir;

/**
 * @brief Construct the W10nJsonTransmitter
 *
 * The transmitter is created to add the ability to return OPeNDAP data
 * objects (DataDDS) as abstract object representation JSON documents.
 *
 * The OPeNDAP data object is written to a JSON file locally in a
 * temporary directory specified by the BES configuration parameter
 * FoJson.Tempdir. If this variable is not found or is not set then it
 * defaults to the macro definition FO_JSON_TEMP_DIR.
 */
W10nJsonTransmitter::W10nJsonTransmitter() :
    BESTransmitter()
{
    add_method(DATA_SERVICE, W10nJsonTransmitter::send_data);
    add_method(DDX_SERVICE, W10nJsonTransmitter::send_metadata);

    if (W10nJsonTransmitter::temp_dir.empty()) {
        // Where is the temp directory for creating these files
        bool found = false;
        string key = "W10nJson.Tempdir";
        TheBESKeys::TheKeys()->get_value(key, W10nJsonTransmitter::temp_dir, found);
        if (!found || W10nJsonTransmitter::temp_dir.empty()) {
            W10nJsonTransmitter::temp_dir = W10N_JSON_TEMP_DIR;
        }
        string::size_type len = W10nJsonTransmitter::temp_dir.size();
        if (W10nJsonTransmitter::temp_dir[len - 1] == '/') {
            W10nJsonTransmitter::temp_dir = W10nJsonTransmitter::temp_dir.substr(0, len - 1);
        }
    }
}

/**
 * Checks the supplied constraint expression by first removing any selection clauses and then
 * verifies that the projection clause contains only a single variable reference.
 */
void W10nJsonTransmitter::checkConstraintForW10nCompatibility(const string &ce)
{
    BESDEBUG(W10N_DEBUG_KEY, "W10nJsonTransmitter::checkConstraintForW10nCompatibility() - BEGIN. ce:  "<< ce << endl);

    string projectionClause = getProjectionClause(ce);
    int firstComma = projectionClause.find(",");

    if (firstComma != -1) {
        string msg = "The w10n protocol only allows one variable to be selected at a time. ";
        msg += "The constraint expression '" + ce + "' requests more than one.";
        BESDEBUG(W10N_DEBUG_KEY, "W10nJsonTransmitter::checkConstraintForW10nCompatibility() - ERROR! "<< msg << endl);
        throw BESSyntaxUserError(msg, __FILE__, __LINE__);
    }

    BESDEBUG(W10N_DEBUG_KEY, "W10nJsonTransmitter::checkConstraintForW10nCompatibility() - END:  " << endl);
}

/**
 * Strips any selection clauses from the passed constraint expression and returns only the projection clause part.
 */
string W10nJsonTransmitter::getProjectionClause(const string &constraintExpression)
{
    string projectionClause = constraintExpression;
    BESDEBUG(W10N_DEBUG_KEY,
        "W10nJsonTransmitter::getProjectionClause() - constraintExpression: "<< constraintExpression << endl);

    int firstAmpersand = constraintExpression.find("&");
    BESDEBUG(W10N_DEBUG_KEY, "W10nJsonTransmitter::getProjectionClause() - firstAmpersand: "<< firstAmpersand << endl);
    if (firstAmpersand >= 0) projectionClause = constraintExpression.substr(0, firstAmpersand);

    BESDEBUG(W10N_DEBUG_KEY,
        "W10nJsonTransmitter::getProjectionClause() - CE projection clause: "<< projectionClause << endl);

    return projectionClause;
}

/**
 * Strips any selection clauses from the passed constraint expression and returns only the projection clause part.
 */
string W10nJsonTransmitter::getProjectedVariableName(const string &constraintExpression)
{
    string varName = getProjectionClause(constraintExpression);

    int firstSquareBracket = varName.find("[");
    if (firstSquareBracket != -1) {
        varName = varName.substr(0, firstSquareBracket);
    }

    return varName;
}

struct ContextCleanup {
    ContextCleanup() {}
    ~ContextCleanup() {
    	BESDEBUG(W10N_DEBUG_KEY, "Cleanup w10n contexts" << endl);
    	W10nJsonTransmitter::cleanupW10nContexts();
    }
};

/**
 *  @brief The static method registered to transmit OPeNDAP data objects as
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
void W10nJsonTransmitter::send_data(BESResponseObject *obj, BESDataHandlerInterface &dhi)
{
#ifndef NDEBUG
    BESStopWatch sw;
    if (BESDebug::IsSet(TIMING_LOG_KEY)) sw.start("W10nJsonTransmitter::send_data", &dhi);
#endif

    BESDEBUG(W10N_DEBUG_KEY, "W10nJsonTransmitter::send_data() - BEGIN." << endl);

    // When 'cleanup' goes out of scope, cleanup the w10n contexts - incl. exceptions.
    ContextCleanup cleanup;

    try {
        BESDapResponseBuilder responseBuilder;

        BESDEBUG(W10N_DEBUG_KEY, "W10nJsonTransmitter::send_data() - reading data into DataDDS" << endl);

        DDS *loaded_dds = responseBuilder.intern_dap2_data(obj, dhi);

        checkConstraintForW10nCompatibility(dhi.data[POST_CONSTRAINT]);
        w10n::checkConstrainedDDSForW10nDataCompatibility(loaded_dds);

        ostream &o_strm = dhi.get_output_stream();
        if (!o_strm) throw BESInternalError("Output stream is not set, can not return as JSON", __FILE__, __LINE__);

        W10nJsonTransform ft(loaded_dds, dhi, &o_strm);

        string varName = getProjectedVariableName(dhi.data[POST_CONSTRAINT]);

        // Verify the request hasn't exceeded bes_timeout.
        RequestServiceTimer::TheTimer()->throw_if_timeout_expired("ERROR: bes-timeout expired before transmit", __FILE__, __LINE__);

        // Now that we are ready to start building the response data we
        // cancel any pending timeout alarm according to the configuration.
        BESUtil::conditional_timeout_cancel();

        BESDEBUG(W10N_DEBUG_KEY,
            "W10nJsonTransmitter::send_data() - Sending w10n data response for variable " << varName << endl);

        ft.sendW10nDataForVariable(varName);
    }
    catch (Error &e) {
        throw BESDapError("Failed to read data! Msg: " + e.get_error_message(), false, e.get_error_code(),
        __FILE__, __LINE__);
    }
    catch (BESError &e) {
        throw;
    }
    catch (...) {
        throw BESInternalError("Failed to read data: Unknown exception caught", __FILE__, __LINE__);
    }

    // cleanupW10nContexts(); See above where an instance

    BESDEBUG(W10N_DEBUG_KEY, "W10nJsonTransmitter::send_data() - END. Done transmitting JSON" << endl);
}

/**
 * @brief The static method registered to transmit OPeNDAP data objects as
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
void W10nJsonTransmitter::send_metadata(BESResponseObject *obj, BESDataHandlerInterface &dhi)
{
#ifndef NDEBUG
    BESStopWatch sw;
    if (BESDebug::IsSet(TIMING_LOG_KEY)) sw.start("W10nJsonTransmitter::send_metadata", &dhi);
#endif

    ContextCleanup cleanup;

    BESDDSResponse *bdds = dynamic_cast<BESDDSResponse *>(obj);
    if (!bdds) throw BESInternalError("cast error", __FILE__, __LINE__);

    DDS *dds = bdds->get_dds();
    if (!dds) throw BESInternalError("No DDS has been created for transmit", __FILE__, __LINE__);

    ConstraintEvaluator &eval = bdds->get_ce();

    ostream &o_strm = dhi.get_output_stream();
    if (!o_strm) throw BESInternalError("Output stream is not set, can not return as JSON", __FILE__, __LINE__);

    // ticket 1248 jhrg 2/23/09
    string ce = www2id(dhi.data[POST_CONSTRAINT], "%", "%20%26");

    checkConstraintForW10nCompatibility(ce);

    try {
        eval.parse_constraint(ce, *dds);
    }
    catch (Error &e) {
        throw BESDapError("Failed to parse the constraint expression: " + e.get_error_message(), false,
            e.get_error_code(), __FILE__, __LINE__);
    }
    catch (...) {
        throw BESInternalError("Failed to parse the constraint expression: Unknown exception caught", __FILE__,
            __LINE__);
    }

    W10nJsonTransform ft(dds, dhi, &o_strm);

    string varName = getProjectedVariableName(ce);

    if (varName.size() == 0) {
        BESDEBUG(W10N_DEBUG_KEY, "W10nJsonTransmitter::send_metadata() - Sending w10n meta response for DDS" << endl);
        ft.sendW10nMetaForDDS();
    }
    else {
        BESDEBUG(W10N_DEBUG_KEY,
            "W10nJsonTransmitter::send_metadata() - Sending w10n meta response for variable " << varName << endl);
        ft.sendW10nMetaForVariable(varName, true);
    }

    BESDEBUG(W10N_DEBUG_KEY, "W10nJsonTransmitter::send_metadata() - done transmitting JSON" << endl);
}

/**
 * Cleanup the special contexts set by w10n. If left around, these will interfere with other commands.
 */
void W10nJsonTransmitter::cleanupW10nContexts()
{
    BESDEBUG(W10N_DEBUG_KEY, "W10nJsonTransmitter::cleanupW10nContexts() - Removing contexts" << endl);

    BESContextManager::TheManager()->unset_context(W10N_META_OBJECT_KEY);

    BESContextManager::TheManager()->unset_context(W10N_CALLBACK_KEY);

    BESContextManager::TheManager()->unset_context(W10N_FLATTEN_KEY);

    BESContextManager::TheManager()->unset_context(W10N_TRAVERSE_KEY);
}
