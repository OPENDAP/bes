// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of libdap, A C++ implementation of the OPeNDAP Data
// Access Protocol.

// Copyright (c) 2011 OPeNDAP, Inc.
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

#include <signal.h>
#include <unistd.h>
#include <sys/stat.h>

#ifdef HAVE_UUID_UUID_H
#include <uuid/uuid.h>  // used to build CID header value for data ddx
#elif defined(HAVE_UUID_H)
#include <uuid.h>
#else
#error "Could not find UUID library header"
#endif


#ifndef WIN32
#include <sys/wait.h>
#else
#include <io.h>
#include <fcntl.h>
#include <process.h>
#endif

#include <iostream>
#include <string>
#include <sstream>
#include <fstream>

#include <cstring>
#include <ctime>

//#define DODS_DEBUG
#define CLEAR_LOCAL_DATA
#undef USE_LOCAL_TIMEOUT_SCHEME

#include <libdap/DAS.h>
#include <libdap/DDS.h>
#include <libdap/Structure.h>
#include <libdap/ConstraintEvaluator.h>
#include <libdap/DDXParserSAX2.h>
#include <libdap/Ancillary.h>
#include <libdap/XDRStreamMarshaller.h>
#include <libdap/XDRFileUnMarshaller.h>

#include <libdap/DMR.h>
#include <libdap/D4Group.h>
#include <libdap/D4Attributes.h>
#include <libdap/XMLWriter.h>
#include <libdap/D4AsyncUtil.h>
#include <libdap/D4StreamMarshaller.h>
#include <libdap/chunked_ostream.h>
#include <libdap/chunked_istream.h>
#include <libdap/D4ConstraintEvaluator.h>
#include <libdap/D4FunctionEvaluator.h>
#include <libdap/D4BaseTypeFactory.h>

#include <libdap/ServerFunctionsList.h>

#include <libdap/mime_util.h>	// for last_modified_time() and rfc_822_date()
#include <libdap/escaping.h>
#include <libdap/util.h>

#include "DapUtils.h"

#if USE_LOCAL_TIMEOUT_SCHEME
#ifndef WIN32
#include <libdap/SignalHandler.h>
#include <libdap/EventHandler.h>
#include <libdap/AlarmHandler.h>
#endif
#endif

#include "TheBESKeys.h"
#include "BESDapResponseBuilder.h"
#include "BESContextManager.h"
#include "BESDapFunctionResponseCache.h"
#include "BESStoredDapResultCache.h"


#include "BESResponseObject.h"
#include "BESDDSResponse.h"
#include "BESDataDDSResponse.h"
#include "BESDMRResponse.h"
#include "BESDataHandlerInterface.h"
#include "BESInternalFatalError.h"
#include "BESSyntaxUserError.h"
#include "BESDataNames.h"

#include "BESRequestHandler.h"
#include "BESRequestHandlerList.h"
#include "BESNotFoundError.h"

#include "BESUtil.h"
#include "BESDebug.h"
#include "BESLog.h"
#include "BESStopWatch.h"
#include "DapFunctionUtils.h"
#include "RequestServiceTimer.h"
#include "../dispatch/BESContextManager.h"

using namespace std;
using namespace libdap;

// @TODO make this std::endl (Is that hard?)
const string CRLF = "\r\n";             // Change here, expr-test.cc
const string BES_KEY_TIMEOUT_CANCEL = "BES.CancelTimeoutOnSend";

#define MODULE "dap"
#define prolog std::string("BESDapResponseBuilder::").append(__func__).append("() - ")

BESDapResponseBuilder::~BESDapResponseBuilder()
{
#if USE_LOCAL_TIMEOUT_SCHEME
    // If an alarm was registered, delete it. The register code in SignalHandler
    // always deletes the old alarm handler object, so only the one returned by
    // remove_handler needs to be deleted at this point.
    delete dynamic_cast<AlarmHandler*>(SignalHandler::instance()->remove_handler(SIGALRM));
#endif
}

/** Return the entire DAP2 constraint expression in a string.  This
 includes both the projection and selection clauses, but not the
 question mark.

 @brief Get the constraint expression.
 @return A string object that contains the constraint expression. */
string BESDapResponseBuilder::get_ce() const
{
    return d_dap2ce;
}

/** Set the DAP2 constraint expression. This will filter the CE text removing
 * any 'WWW' escape characters except space. Spaces are left in the CE
 * because the CE parser uses whitespace to delimit tokens while some
 * datasets have identifiers that contain spaces. It's possible to use
 * double quotes around identifiers too, but most client software doesn't
 * know about that.
 *
 * @@brief Set the CE
 * @param _ce The constraint expression
 */
void BESDapResponseBuilder::set_ce(string _ce)
{
    d_dap2ce = www2id(_ce, "%", "%20");
}

/** Return the entire DAP4 constraint expression in a string.
 @brief Get the DAP4 constraint expression.
 @return A string object that contains the constraint expression. */
string BESDapResponseBuilder::get_dap4ce() const
{
    return d_dap4ce;
}

/** Set the DAP4 constraint expression. This will filter the DAP4 CE text removing
 * any 'WWW' escape characters except space. Spaces are left in the CE
 * because the CE parser uses whitespace to delimit tokens while some
 * datasets have identifiers that contain spaces. It's possible to use
 * double quotes around identifiers too, but most client software doesn't
 * know about that.
 *
 * @@brief Set the CE
 * @param _ce The constraint expression
 */
void BESDapResponseBuilder::set_dap4ce(const string &_ce)
{
    d_dap4ce = www2id(_ce, "%", "%20");
}

/** Return the entire DAP4 server side function expression in a string.
 @brief Get the DAP4 server side function expression.
 @return A string object that contains the constraint expression. */
string BESDapResponseBuilder::get_dap4function() const
{
    return d_dap4function;
}

/** Set the DAP4 Server Side Function expression. This will filter the
 * function expression text removing
 * any 'WWW' escape characters except space. Spaces are left in the CE
 * because the CE parser uses whitespace to delimit tokens while some
 * datasets have identifiers that contain spaces. It's possible to use
 * double quotes around identifiers too, but most client software doesn't
 * know about that.
 *
 * @@brief Set the CE
 * @param _ce The constraint expression
 */
void BESDapResponseBuilder::set_dap4function(const string &_func)
{
    d_dap4function = www2id(_func, "%", "%20");
}

std::string BESDapResponseBuilder::get_store_result() const
{
    return d_store_result;
}

void BESDapResponseBuilder::set_store_result(const std::string &_sr)
{
    d_store_result = _sr;
    BESDEBUG(MODULE, prolog << "store_result: " << _sr << endl);
}

std::string BESDapResponseBuilder::get_async_accepted() const
{
    return d_async_accepted;
}

void BESDapResponseBuilder::set_async_accepted(const std::string &_aa)
{
    d_async_accepted = _aa;
    BESDEBUG(MODULE, prolog << "set_async_accepted() - async_accepted: " << _aa << endl);
}

/** The ``dataset name'' is the filename or other string that the
 filter program will use to access the data. In some cases this
 will indicate a disk file containing the data.  In others, it
 may represent a database query or some other exotic data
 access method.

 @brief Get the dataset name.
 @return A string object that contains the name of the dataset. */
string BESDapResponseBuilder::get_dataset_name() const
{
    return d_dataset;
}

/** Set the dataset name, which is a string used to access the dataset
 * on the machine running the server. That is, this is typically a pathname
 * to a data file, although it doesn't have to be. This is not
 * echoed in error messages (because that would reveal server
 * storage patterns that data providers might want to hide). All WWW-style
 * escapes are replaced except for spaces.
 *
 * @brief Set the dataset pathname.
 * @param ds The pathname (or equivalent) to the dataset.
 */
void BESDapResponseBuilder::set_dataset_name(const string &ds)
{
    d_dataset = www2id(ds, "%", "%20");
}

/**
 * Starting at pos, look for the next closing (right) parenthesis. This code
 * will count opening (left) parens and find the closing paren that maches
 * the first opening paren found. Examples: "0123)56" --> 4; "0123(5)" --> 6;
 * "01(3(5)7)9" --> 8.
 *
 * This function is intended to help split up a constraint expression so that
 * the server functions can be processed separately from the projection and
 * selection parts of the CE.
 *
 * @param ce The constraint to look in
 * @param pos Start looking at this position; zero-based indexing
 * @return The position in the string where the closing paren was found
 */
static string::size_type find_closing_paren(const string &ce, string::size_type pos)
{
    // Iterate over the string finding all ( or ) characters until the matching ) is found.
    // For each ( found, increment count. When a ) is found and count is zero, it is the
    // matching closing paren, otherwise, decrement count and keep looking.
    int count = 1;
    do {
        pos = ce.find_first_of("()", pos + 1);
        if (pos == string::npos){
            stringstream msg;
            msg << "Expected to find a matching closing parenthesis in: " << ce;
            throw BESSyntaxUserError(msg.str(),__FILE__,__LINE__);
        }

        if (ce[pos] == '(')
            ++count;
        else
            --count;	// must be ')'

    } while (count > 0);

    return pos;
}

/**
 *  Split the CE so that the server functions that compute new values are
 *  separated into their own string and can be evaluated separately from
 *  the rest of the CE (which can contain simple and slicing projection
 *  as well as other types of function calls).
 */
void BESDapResponseBuilder::split_ce(ConstraintEvaluator &eval, const string &expr)
{
    BESDEBUG(MODULE, prolog << "source expression: " << expr << endl);

    string ce;
    if (!expr.empty())
        ce = expr;
    else
        ce = d_dap2ce;

    string btp_function_ce;
    string::size_type pos = 0;

    // This hack assumes that the functions are listed first. Look for the first
    // open paren and the last closing paren to accommodate nested function calls
    string::size_type first_paren = ce.find("(", pos);
    string::size_type closing_paren = string::npos;
    if (first_paren != string::npos) closing_paren = find_closing_paren(ce, first_paren); //ce.find(")", pos);

    while (first_paren != string::npos && closing_paren != string::npos) {
        // Maybe a BTP function; get the name of the potential function
        string name = ce.substr(pos, first_paren - pos);

        // is this a BTP function
        btp_func f;
        if (eval.find_function(name, &f)) {
            // Found a BTP function
            if (!btp_function_ce.empty()) btp_function_ce += ",";
            btp_function_ce += ce.substr(pos, closing_paren + 1 - pos);
            ce.erase(pos, closing_paren + 1 - pos);
            if (ce[pos] == ',') ce.erase(pos, 1);
        }
        else {
            pos = closing_paren + 1;
            // exception?
            if (pos < ce.size() && ce.at(pos) == ',') ++pos;
        }

        first_paren = ce.find("(", pos);
        closing_paren = ce.find(")", pos);
    }

    d_dap2ce = ce;
    d_btp_func_ce = btp_function_ce;

    BESDEBUG(MODULE, prolog << "Modified constraint: " << d_dap2ce << endl);
    BESDEBUG(MODULE, prolog << "BTP Function part: " << btp_function_ce << endl);
    BESDEBUG(MODULE, prolog << "END" << endl);
}



/** This function formats and prints an ASCII representation of a
 DAS on stdout.  This has the effect of sending the DAS object
 back to the client program.

 @note This is the DAP2 attribute response.

 @brief Send a DAS.

 @param out The output stream to which the DAS is to be sent.
 @param das The DAS object to be sent.
 @param with_mime_headers If true (the default) send MIME headers.
 @return void
 @see DAS
 @deprecated */
void BESDapResponseBuilder::send_das(ostream &out, DAS &das, bool with_mime_headers) const
{
    if (with_mime_headers) set_mime_text(out, dods_das, x_plain, last_modified_time(d_dataset), "2.0");

    dap_utils::throw_for_dap4_typed_attrs(&das, __FILE__, __LINE__);

    das.print(out);

    out << flush;
}

/** Send the DAP2 DAS response to the given stream. This version of
 * send_das() uses the DDS object, assuming that it contains attribute
 * information. If there is a constraint expression associated with this
 * instance of ResponseBuilder, then it will be applied. This means
 * that CEs that contain server functions will populate the response cache
 * even if the server's initial request is for a DAS. This is different
 * from the older behavior of libdap where CEs were never evaluated for
 * the DAS response. This does not actually change the resulting DAS,
 * just the behavior 'under the covers'.
 *
 * @param out Send the response to this ostream
 * @param dds Use this DDS object
 * @param eval A Constraint Evaluator to use for any CE bound to this
 * ResponseBuilder instance
 * @param constrained Should the result be constrained
 * @param with_mime_headers Should MIME headers be sent to out?
 */
void BESDapResponseBuilder::send_das(ostream &out, DDS **dds, ConstraintEvaluator &eval, bool constrained,
    bool with_mime_headers)
{
#if USE_LOCAL_TIMEOUT_SCHEME
    // Set up the alarm.
    establish_timeout(out);
    dds.set_timeout(d_timeout);
#endif

    // Verify the request hasn't exceeded bes_timeout.
    RequestServiceTimer::TheTimer()->throw_if_timeout_expired(prolog +" ERROR: bes-timeout expired before transmit", __FILE__, __LINE__);

    if (!constrained) {
        if (with_mime_headers) set_mime_text(out, dods_das, x_plain, last_modified_time(d_dataset), "2.0");

        BESUtil::conditional_timeout_cancel();
        (*dds)->mark_all(true);
        dap_utils::throw_for_dap4_typed_vars_or_attrs(*dds, __FILE__, __LINE__);

        (*dds)->print_das(out);
        out << flush;

        return;
    }

    split_ce(eval);

    // If there are functions, parse them and eval.
    // Use that DDS and parse the non-function ce
    // Serialize using the second ce and the second dds
    if (!d_btp_func_ce.empty()) {
        ConstraintEvaluator func_eval;
        BESDapFunctionResponseCache *responseCache = BESDapFunctionResponseCache::get_instance();

        DDS *fdds = 0; // nullptr
        if (responseCache && responseCache->can_be_cached(*dds, get_btp_func_ce())) {
            fdds = responseCache->get_or_cache_dataset(*dds, get_btp_func_ce());
        }
        else {
            func_eval.parse_constraint(get_btp_func_ce(), **dds);
            fdds = func_eval.eval_function_clauses(**dds);
        }

        delete *dds; *dds = 0;
        *dds = fdds;

        if (with_mime_headers)
            set_mime_text(out, dods_das, x_plain, last_modified_time(d_dataset), (*dds)->get_dap_version());

        RequestServiceTimer::TheTimer()->throw_if_timeout_expired(prolog +"ERROR: bes-timeout expired before transmit", __FILE__, __LINE__);
        BESUtil::conditional_timeout_cancel();

        (*dds)->print_das(out);
    }
    else {
        eval.parse_constraint(d_dap2ce, **dds); // Throws Error if the ce doesn't parse.

        dap_utils::throw_for_dap4_typed_vars_or_attrs(*dds, __FILE__, __LINE__);

        if (with_mime_headers)
            set_mime_text(out, dods_das, x_plain, last_modified_time(d_dataset), (*dds)->get_dap_version());

        RequestServiceTimer::TheTimer()->throw_if_timeout_expired(prolog +"ERROR: bes-timeout expired before transmit", __FILE__, __LINE__);
        BESUtil::conditional_timeout_cancel();

        (*dds)->print_das(out);
    }

    out << flush;
}


/** This function formats and prints an ASCII representation of a
 DDS on stdout. Either an entire DDS or a constrained DDS may be sent.
 This function looks in the local cache and uses a DDS object there
 if it's valid. Otherwise, if the request CE contains server functions
 that build data for the response, the resulting DDS will be cached.

 @brief Transmit a DDS.
 @param out The output stream to which the DAS is to be sent.
 @param dds The DDS to send back to a client.
 @param eval A reference to the ConstraintEvaluator to use.
 @param constrained If this argument is true, evaluate the
 current constraint expression and send the `constrained DDS'
 back to the client.
 @param constrained If true, apply the constraint bound to this instance
 of ResponseBuilder
 @param with_mime_headers If true (default) send MIME headers.
 @return void
 @see DDS */
void BESDapResponseBuilder::send_dds(ostream &out, DDS **dds, ConstraintEvaluator &eval, bool constrained,
    bool with_mime_headers)
{
    if (!constrained) {
        if (with_mime_headers)
            set_mime_text(out, dods_dds, x_plain, last_modified_time(d_dataset), (*dds)->get_dap_version());

        // Verify the request hasn't exceeded bes_timeout, and disable timeout if allowed.
        RequestServiceTimer::TheTimer()->throw_if_timeout_expired(prolog +"ERROR: bes-timeout expired before transmit", __FILE__, __LINE__);
        BESUtil::conditional_timeout_cancel();

        dap_utils::throw_for_dap4_typed_vars_or_attrs(*dds, __FILE__, __LINE__);

        (*dds)->print(out);
        out << flush;
        return;
    }

#if USE_LOCAL_TIMEOUT_SCHEME
    // Set up the alarm.
    establish_timeout(out);
    dds.set_timeout(d_timeout);
#endif

    // Split constraint into two halves
    split_ce(eval);

    // If there are functions, parse them and eval.
    // Use that DDS and parse the non-function ce
    // Serialize using the second ce and the second dds
    if (!d_btp_func_ce.empty()) {
        BESDEBUG(MODULE,prolog << "Found function(s) in CE: " << get_btp_func_ce() << endl);
        ConstraintEvaluator func_eval;

        BESDapFunctionResponseCache *responseCache = BESDapFunctionResponseCache::get_instance();

        DDS *fdds = nullptr;
        if (responseCache && responseCache->can_be_cached(*dds, get_btp_func_ce())) {
            fdds = responseCache->get_or_cache_dataset(*dds, get_btp_func_ce());
        }
        else {
            func_eval.parse_constraint(get_btp_func_ce(), **dds);
            fdds = func_eval.eval_function_clauses(**dds);
        }

        delete *dds; *dds = nullptr;
        *dds = fdds;

        // Server functions might mark variables to use their read()
        // methods. Clear that so the CE in d_dap2ce will control what is
        // sent. If that is empty (there was only a function call) all
        // the variables in the intermediate DDS (i.e., the function
        // result) will be sent.
        (*dds)->mark_all(false);

        // This next step utilizes a well known static method (so really it's a function;),
        // promote_function_output_structures() to look for
        // one or more top level Structures whose name indicates (by way of ending with
        // "_unwrap") that their contents should be promoted (aka moved) to the top level.
        // This is in support of a hack around the current API where server side functions
        // may only return a single DAP object and not a collection of objects. The name suffix
        // "_unwrap" is used as a signal from the function to the the various response
        // builders and transmitters that the representation needs to be altered before
        // transmission, and that in fact is what happens in our friend
        // promote_function_output_structures()
        promote_function_output_structures(*dds);

        eval.parse_constraint(d_dap2ce, **dds);

        if (with_mime_headers)
            set_mime_text(out, dods_dds, x_plain, last_modified_time(d_dataset), (*dds)->get_dap_version());

        // Verify the request hasn't exceeded bes_timeout, and disable timeout if allowed.
        RequestServiceTimer::TheTimer()->throw_if_timeout_expired(prolog +"ERROR: bes-timeout expired before transmit", __FILE__, __LINE__);
        BESUtil::conditional_timeout_cancel();

        (*dds)->print_constrained(out);
    }
    else {
        BESDEBUG(MODULE, prolog << "Simple constraint: " << d_dap2ce << endl);
        eval.parse_constraint(d_dap2ce, **dds); // Throws Error if the ce doesn't parse.
        dap_utils::throw_for_dap4_typed_vars_or_attrs(*dds, __FILE__, __LINE__); // Throws error if dap4 types will be in the response.

        if (with_mime_headers)
            set_mime_text(out, dods_dds, x_plain, last_modified_time(d_dataset),(*dds)->get_dap_version());

        // Verify the request hasn't exceeded bes_timeout, and disable timeout if allowed.
        RequestServiceTimer::TheTimer()->throw_if_timeout_expired(prolog +"ERROR: bes-timeout expired before transmit", __FILE__, __LINE__);
        BESUtil::conditional_timeout_cancel();

        (*dds)->print_constrained(out);
    }

    out << flush;
}

#ifdef DAP2_STORED_RESULTS
/**
 * Should this result be returned using the asynchronous response mechanism?
 * Look at the 'store_result' property and see if the code should return this
 * using the asynchronous mechanism. If yes, it will try and return the correct
 * response or an error if the server is not configured or the client has not
 * included the correct information in the request.
 *
 * @param out Write information to the client using this stream
 * @param dds The DDS that hold information used to build the response. The
 * response won't be built until some time in the future
 * @param eval Use this to evaluate the CE associated with the response.
 * @return True if the response should/will be returned asynchronously, false
 * otherwise.
 */
bool BESDapResponseBuilder::store_dap2_result(ostream &out, DDS &dds, ConstraintEvaluator &eval)
{
    if (get_store_result().empty()) return false;

    string serviceUrl = get_store_result();

    XMLWriter xmlWrtr;
    D4AsyncUtil d4au;

    // FIXME Keys should be read in initialize(). Also, I think the D4AsyncUtil should
    // be removed from libdap - it is much more about how the BES processes these kinds
    // of operations. Change this when working on the response caching for ODSIP. But...
    // do we really need to put the style sheet in the bes.conf file? Should it be baked
    // into the code (because we don't want people to change it)?
    bool found;
    string *stylesheet_ref = 0, ss_ref_value;
    TheBESKeys::TheKeys()->get_value(D4AsyncUtil::STYLESHEET_REFERENCE_KEY, ss_ref_value, found);
    if (found && !ss_ref_value.empty()) {
        stylesheet_ref = &ss_ref_value;
    }

    BESStoredDapResultCache *resultCache = BESStoredDapResultCache::get_instance();
    if (resultCache == NULL) {

        /**
         * OOPS. Looks like the BES is not configured to use a Stored Result Cache.
         * Looks like need to reject the request and move on.
         *
         */
        string msg = "The Stored Result request cannot be serviced. ";
        msg += "Unable to acquire StoredResultCache instance. ";
        msg += "This is most likely because the StoredResultCache is not (correctly) configured.";

        BESDEBUG(MODULE, prolog << "[WARNING] " << msg << endl);

        d4au.writeD4AsyncResponseRejected(xmlWrtr, UNAVAILABLE, msg, stylesheet_ref);
        out << xmlWrtr.get_doc();
        out << flush;

        BESDEBUG(MODULE,prolog << "Sent AsyncRequestRejected" << endl);
    }
    else if (get_async_accepted().size() != 0) {

        /**
         * Client accepts async responses so, woot! lets store this thing and tell them where to find it.
         */
        BESDEBUG(MODULE, prolog << "serviceUrl="<< serviceUrl << endl);

        BESStoredDapResultCache *resultCache = BESStoredDapResultCache::get_instance();
        string storedResultId = "";
        storedResultId = resultCache->store_dap2_result(dds, get_ce(), this, &eval);

        BESDEBUG(MODULE, prolog << "storedResultId='"<< storedResultId << "'" << endl);

        string targetURL = BESUtil::assemblePath(serviceUrl, storedResultId);
        BESDEBUG(MODULE, prolog << "targetURL='"<< targetURL << "'" << endl);

        XMLWriter xmlWrtr;
        d4au.writeD4AsyncAccepted(xmlWrtr, 0, 0, targetURL, stylesheet_ref);
        out << xmlWrtr.get_doc();
        out << flush;

        BESDEBUG(MODULE, prolog << "Sent DAP4 AsyncAccepted response" << endl);
    }
    else {
        /**
         * Client didn't indicate a willingness to accept an async response
         * So - we tell them that async is required.
         */
        d4au.writeD4AsyncRequired(xmlWrtr, 0, 0, stylesheet_ref);
        out << xmlWrtr.get_doc();
        out << flush;

        BESDEBUG(MODULE, prolog << "Sent DAP4 AsyncRequired  response" << endl);
    }

    return true;

}
#endif

/**
 * Build/return the BLOB part of the DAP2 data response.
 */
void BESDapResponseBuilder::serialize_dap2_data_dds(ostream &out, DDS **dds, ConstraintEvaluator &eval, bool ce_eval)
{
    BES_STOPWATCH_START(MODULE, prolog + "Timer");
    // Verify the request hasn't exceeded bes_timeout, and disable timeout if allowed.
    RequestServiceTimer::TheTimer()->throw_if_timeout_expired(prolog +"ERROR: bes-timeout expired before transmit", __FILE__, __LINE__);
    BESUtil::conditional_timeout_cancel();

    BESDEBUG(MODULE, prolog << "BEGIN" << endl);

    (*dds)->print_constrained(out);
    out << "Data:\n";
    out << flush;

    XDRStreamMarshaller m(out);

    // Send all variables in the current projection (send_p())
    for (auto i = (*dds)->var_begin(); i != (*dds)->var_end(); i++) {
        if ((*i)->send_p()) {
            RequestServiceTimer::TheTimer()->throw_if_timeout_expired(prolog +"ERROR: bes-timeout expired before transmit " + (*i)->name(), __FILE__, __LINE__);
            (*i)->serialize(eval, **dds, m, ce_eval);
#ifdef CLEAR_LOCAL_DATA
            (*i)->clear_local_data();
#endif
        }
    }

    BESDEBUG(MODULE, prolog << "END" << endl);
}

#ifdef DAP2_STORED_RESULTS
/**
 * Serialize a DAP3.2 DataDDX to the stream "out".
 * This was originally intended to be used for DAP4, now it is used to
 * store responses for the async response feature as well as response
 * caching for function results.
 *
 * FIXME Remove this until Stored DAP2 Results are working again.
 */
void BESDapResponseBuilder::serialize_dap2_data_ddx(ostream &out, DDS **dds, ConstraintEvaluator &eval,
    const string &boundary, const string &start, bool ce_eval)
{
    BESDEBUG(MODULE, prolog << "BEGIN" << endl);

    // Write the MPM headers for the DDX (text/xml) part of the response
    libdap::set_mime_ddx_boundary(out, boundary, start, dods_ddx, x_plain);

    // Make cid
    uuid_t uu;
    uuid_generate(uu);
    char uuid[37];
    uuid_unparse(uu, uuid.data());
    char domain[256];
    if (getdomainname(domain, 255) != 0 || strlen(domain) == 0) strncpy(domain, "opendap.org", 255);

    string cid = string(uuid.data()) + "@" + string(domain.data());

    // Send constrained DDX with a data blob reference.
    // Note: CID passed but ignored jhrg 10/20/15
    (*dds)->print_xml_writer(out, true, cid);

    // write the data part mime headers here
    set_mime_data_boundary(out, boundary, cid, dods_data_ddx /* old value dap4_data*/, x_plain);

    XDRStreamMarshaller m(out);

    conditional_timeout_cancel();


    // Send all variables in the current projection (send_p()).
    for (DDS::Vars_iter i = (*dds)->var_begin(); i != (*dds)->var_end(); i++) {
        if ((*i)->send_p()) {
            (*i)->serialize(eval, **dds, m, ce_eval);
#ifdef CLEAR_LOCAL_DATA
            (*i)->clear_local_data();
#endif
        }
    }

    BESDEBUG(MODULE, prolog << "END" << endl);
}
#endif

/** Send the data in the DDS object back to the client program. The data is
 encoded using a Marshaller, and enclosed in a MIME document which is all sent
 to \c data_stream.

 @note This is the DAP2 data response.

 @brief Transmit data.
 @param dds A DDS object containing the data to be sent.
 @param eval A reference to the ConstraintEvaluator to use.
 @param data_stream Write the response to this stream.
 @param anc_location A directory to search for ancillary files (in
 addition to the CWD).  This is used in a call to
 get_data_last_modified_time().
 @param with_mime_headers If true, include the MIME headers in the response.
 Defaults to true.
 @return void */
#if 0
void BESDapResponseBuilder::remove_timeout() const
{
#if USE_LOCAL_TIMEOUT_SCHEME
    alarm(0);
#endif
}
#endif

/**
 * @brief Process a DDS (i.e., apply a constraint) for a non-DAP transmitter.
 *
 * This is a companion method to the intern_dap2_data() method. Unlike that,
 * this simply evaluates the CE against the DDS, including any functions.
 *
 * @note If there is no constraint, there's no need to call this.
 *
 * @param obj ResponseObject wrapper
 * @param dhi Various parameters to the handler
 * @return The DDS* after the CE, including functions, has been evaluated. The
 * returned pointer is managed by the ResponseObject
 */
libdap::DDS *
BESDapResponseBuilder::process_dap2_dds(BESResponseObject *obj, BESDataHandlerInterface &dhi)
{
    BESDEBUG(MODULE, prolog << "BEGIN"<< endl);

    dhi.first_container();

    auto bdds = dynamic_cast<BESDDSResponse *>(obj);
    if (!bdds) throw BESInternalFatalError("Expected a BESDDSResponse instance", __FILE__, __LINE__);

    DDS *dds = bdds->get_dds();

    set_dataset_name(dds->filename());
    set_ce(dhi.data[POST_CONSTRAINT]);
    set_async_accepted(dhi.data[ASYNC]);
    set_store_result(dhi.data[STORE_RESULT]);

    ConstraintEvaluator &eval = bdds->get_ce();

    // Split constraint into two halves
    split_ce(eval);

    // If there are functions, parse them and eval.
    // Use that DDS and parse the non-function ce
    // Serialize using the second ce and the second dds
    if (!d_btp_func_ce.empty()) {
        BESDapFunctionResponseCache *responseCache = BESDapFunctionResponseCache::get_instance();

        ConstraintEvaluator func_eval;
        DDS *fdds = nullptr;
        if (responseCache && responseCache->can_be_cached(dds, get_btp_func_ce())) {
            fdds = responseCache->get_or_cache_dataset(dds, get_btp_func_ce());
        }
        else {
            func_eval.parse_constraint(get_btp_func_ce(), *dds);
            fdds = func_eval.eval_function_clauses(*dds);
        }

        delete dds;             // Delete so that we can ...
        bdds->set_dds(fdds);    // Transfer management responsibility
        dds = fdds;

        dds->mark_all(false);

        promote_function_output_structures(dds);
    }

    eval.parse_constraint(d_dap2ce, *dds); // Throws Error if the ce doesn't parse.
    BESDEBUG(MODULE, prolog << "END"<< endl);

    return dds;
}

/**
 * Read data into the ResponseObject (a DAP2 DDS). Instead of serializing the data
 * as with send_dap2_data(), this uses the variables' intern_data() method to load
 * the variables so that a transmitter other than libdap::BaseType::serialize()
 * can be used (e.g., AsciiTransmitter).
 *
 * @note Other versions of the methods here (e.g., send_dap2_data()) optionally
 * print MIME headers because this code was used in the past to build HTTP responses.
 * That's only the case with the CEDAR server and I'm not sure we need to maintain
 * that code. TBD. This method will not be used by CEDAR, so I've not included the
 * 'print_mime_headers' bool.
 *
 * @param obj The BESResponseObject. Holds the DDS for this request.
 * @param dhi The BESDataHandlerInterface. Holds many parameters for this request.
 * @return The DDS* is returned where each variable marked to be sent is loaded with
 * data (as per the current constraint expression).
 */
libdap::DDS *
BESDapResponseBuilder::intern_dap2_data(BESResponseObject *obj, BESDataHandlerInterface &dhi)
{
    BES_STOPWATCH_START(MODULE, prolog + "Timer");

    BESDEBUG(MODULE, prolog << "BEGIN"<< endl);

    dhi.first_container();

    auto bdds = dynamic_cast<BESDataDDSResponse *>(obj);
    if (!bdds) throw BESInternalFatalError("Expected a BESDataDDSResponse instance", __FILE__, __LINE__);

    DDS *dds = bdds->get_dds();

    set_dataset_name(dds->filename());
    set_ce(dhi.data[POST_CONSTRAINT]);
    set_async_accepted(dhi.data[ASYNC]);
    set_store_result(dhi.data[STORE_RESULT]);

        
    // This function is used by all fileout modules and they need to include the attributes in data access.
    // So obtain the attributes if necessary. KY 2019-10-30
    if(!bdds->get_ia_flag()) {
        BESRequestHandler *besRH = BESRequestHandlerList::TheList()->find_handler(dhi.container->get_container_type());
        besRH->add_attributes(dhi);
    }

    ConstraintEvaluator &eval = bdds->get_ce();

    // Split constraint into two halves; stores the function and non-function parts in this instance.
    split_ce(eval);

    // If there are functions, parse them and eval.
    // Use that DDS and parse the non-function ce
    // Serialize using the second ce and the second dds
    if (!get_btp_func_ce().empty()) {
        BESDEBUG(MODULE,prolog << "Found function(s) in CE: " << get_btp_func_ce() << endl);

        BESDapFunctionResponseCache *responseCache = BESDapFunctionResponseCache::get_instance();

        ConstraintEvaluator func_eval;
        DDS *fdds = nullptr;
        if (responseCache && responseCache->can_be_cached(dds, get_btp_func_ce())) {
            fdds = responseCache->get_or_cache_dataset(dds, get_btp_func_ce());
        }
        else {
            func_eval.parse_constraint(get_btp_func_ce(), *dds);
            fdds = func_eval.eval_function_clauses(*dds);
        }

        delete dds;             // Delete so that we can ...
        bdds->set_dds(fdds);    // Transfer management responsibility
        dds = fdds;

        // Server functions might mark (i.e. setting send_p) so variables will use their read()
        // methods. Clear that so the CE in d_dap2ce will control what is
        // sent. If that is empty (there was only a function call) all
        // the variables in the intermediate DDS (i.e., the function
        // result) will be sent.
        dds->mark_all(false);

        // Look for one or more top level Structures whose name indicates (by way of ending with
        // "_uwrap") that their contents should be moved to the top level.
        //
        // This is in support of a hack around the current API where server side functions
        // may only return a single DAP object and not a collection of objects. The name suffix
        // "_unwrap" is used as a signal from the function to the the various response
        // builders and transmitters that the representation needs to be altered before
        // transmission, and that in fact is what happens in our friend
        // promote_function_output_structures()
        promote_function_output_structures(dds);
    }

    // evaluate the rest of the CE - the part that follows the function calls.
    eval.parse_constraint(get_ce(), *dds);

    dds->tag_nested_sequences(); // Tag Sequences as Parent or Leaf node.

    dap_utils::throw_if_too_big(*dds, __FILE__, __LINE__);


    // Iterate through the variables in the DataDDS and read
    // in the data if the variable has its send flag set.
    for (auto i = dds->var_begin(), e = dds->var_end(); i != e; ++i) {
        if ((*i)->send_p()) {
            try {
                (*i)->intern_data(eval, *dds);
            }
            catch(std::exception &e) {
                throw BESSyntaxUserError(string("Caught a C++ standard exception while working on '") + (*i)->name() + "' The error was: " + e.what(), __FILE__, __LINE__);
            }
        }
    }

    BESDEBUG(MODULE, prolog << "END"<< endl);

    return dds;
}

/**
 * Build the DAP2 data response.
 *
 * @todo consider changing the arguments to this so that it takes the DHI and the
 * other stuff passed to SendDataDDS::send_internal() (DHI and the ResponseObject
 * wrapper).
 *
 * @param data_stream
 * @param dds
 * @param eval
 * @param with_mime_headers
 */
void BESDapResponseBuilder::send_dap2_data(ostream &data_stream, DDS **dds, ConstraintEvaluator &eval,
    bool with_mime_headers)
{
    BESDEBUG(MODULE, prolog << "BEGIN"<< endl);

#if USE_LOCAL_TIMEOUT_SCHEME
    // Set up the alarm.
    establish_timeout(data_stream);
    dds.set_timeout(d_timeout);
#endif

    // Split constraint into two halves
    split_ce(eval);

    // If there are functions, parse them and eval.
    // Use that DDS and parse the non-function ce
    // Serialize using the second ce and the second dds
    if (!get_btp_func_ce().empty()) {
        BESDEBUG(MODULE,prolog << "Found function(s) in CE: " << get_btp_func_ce() << endl);

        BESDapFunctionResponseCache *response_cache = BESDapFunctionResponseCache::get_instance();

        ConstraintEvaluator func_eval;
        DDS *fdds = nullptr;
        if (response_cache && response_cache->can_be_cached(*dds, get_btp_func_ce())) {
            fdds = response_cache->get_or_cache_dataset(*dds, get_btp_func_ce());
        }
        else {
            func_eval.parse_constraint(get_btp_func_ce(), **dds);
            fdds = func_eval.eval_function_clauses(**dds);
        }

        delete *dds; *dds = nullptr;
        *dds = fdds;

        (*dds)->mark_all(false);

        promote_function_output_structures(*dds);

        // evaluate the rest of the CE - the part that follows the function calls.
        eval.parse_constraint(get_ce(), **dds);

        (*dds)->tag_nested_sequences(); // Tag Sequences as Parent or Leaf node.

        dap_utils::throw_if_too_big(**dds, __FILE__, __LINE__);

        if (with_mime_headers)
            set_mime_binary(data_stream, dods_data, x_plain, last_modified_time(d_dataset), (*dds)->get_dap_version());

#if STORE_DAP2_RESULT_FEATURE
        // This means: if we are not supposed to store the result, then serialize it.
        if (!store_dap2_result(data_stream, **dds, eval)) {
            serialize_dap2_data_dds(data_stream, dds, eval, true /* was 'false'. jhrg 3/10/15 */);
        }
#else
        serialize_dap2_data_dds(data_stream, dds, eval, true /* was 'false'. jhrg 3/10/15 */);
#endif

    }
    else {
        BESDEBUG(MODULE, prolog << "Simple constraint" << endl);

        eval.parse_constraint(get_ce(), **dds); // Throws Error if the ce doesn't parse.
        (*dds)->tag_nested_sequences(); // Tag Sequences as Parent or Leaf node.

        dap_utils::throw_for_dap4_typed_vars_or_attrs(*dds, __FILE__, __LINE__);
        dap_utils::throw_if_too_big(**dds, __FILE__, __LINE__);

        if (with_mime_headers)
            set_mime_binary(data_stream, dods_data, x_plain, last_modified_time(d_dataset), (*dds)->get_dap_version());

#if STORE_DAP2_RESULT_FEATURE
        // This means: if we are not supposed to store the result, then serialize it.
        if (!store_dap2_result(data_stream, **dds, eval)) {
            serialize_dap2_data_dds(data_stream, dds, eval);
        }
#else
        serialize_dap2_data_dds(data_stream, dds, eval);
#endif
    }

    dap_utils::log_response_and_memory_size(prolog, dds);

#if 0
    // This change addresses an issue on OSX where the size returned by getrusage() is in bytes, not kb.
    // The BESUtil::get_current_memory_usage() function sorts the problem. jhrg 4/6/22
    struct rusage usage;
    int usage_val;
    usage_val = getrusage(RUSAGE_SELF, &usage);

    if (usage_val == 0){ //if getrusage() was successful, out put both sizes
        long mem_size = usage.ru_maxrss; // get the max size (man page says it is in kilobytes)
        INFO_LOG(prolog << "request size: "<< req_size << "KB -|- memory used by process: " << mem_size << "KB" << endl);
    }
    else { //if the getrusage() wasn't successful, only output the request size
        INFO_LOG(prolog << "request size: "<< req_size << "KB" << endl );
    }
#endif

    data_stream << flush;

    BESDEBUG(MODULE, prolog << "END"<< endl);

}

void BESDapResponseBuilder::send_dap2_data(BESDataHandlerInterface &dhi, DDS **dds, ConstraintEvaluator &eval,
    bool with_mime_headers)
{
    BESDEBUG(MODULE, prolog << "BEGIN"<< endl);

    ostream & data_stream = dhi.get_output_stream();
#if USE_LOCAL_TIMEOUT_SCHEME
    // Set up the alarm.
    establish_timeout(data_stream);
    dds.set_timeout(d_timeout);
#endif

    // Split constraint into two halves
    split_ce(eval);

    // If there are functions, parse them and eval.
    // Use that DDS and parse the non-function ce
    // Serialize using the second ce and the second dds
    if (!get_btp_func_ce().empty()) {
        BESDEBUG(MODULE, prolog << "Found function(s) in CE: " << get_btp_func_ce() << endl);

        // Server-side functions need to include the attributes in data access.
        // So obtain the attributes if necessary. KY 2019-10-30
        {
            BESResponseObject *response = dhi.response_handler->get_response_object();
            auto *bdds = dynamic_cast<BESDataDDSResponse *> (response);
            if (!bdds)
                throw BESInternalError("cast error", __FILE__, __LINE__);

            if(!bdds->get_ia_flag()) {
                BESRequestHandler *besRH = BESRequestHandlerList::TheList()->find_handler(dhi.container->get_container_type());
                besRH->add_attributes(dhi);
            }
        }

        BESDapFunctionResponseCache *response_cache = BESDapFunctionResponseCache::get_instance();
        ConstraintEvaluator func_eval;
        DDS *fdds = nullptr;
        if (response_cache && response_cache->can_be_cached(*dds, get_btp_func_ce())) {
            fdds = response_cache->get_or_cache_dataset(*dds, get_btp_func_ce());
        }
        else {
            func_eval.parse_constraint(get_btp_func_ce(), **dds);
            fdds = func_eval.eval_function_clauses(**dds);
        }

        delete *dds;
        *dds = nullptr;
        *dds = fdds;

        (*dds)->mark_all(false);

        promote_function_output_structures(*dds);

        // evaluate the rest of the CE - the part that follows the function calls.
        eval.parse_constraint(get_ce(), **dds);

        (*dds)->tag_nested_sequences(); // Tag Sequences as Parent or Leaf node.

        dap_utils::throw_if_too_big(**dds, __FILE__, __LINE__);

        if (with_mime_headers)
            set_mime_binary(data_stream, dods_data, x_plain, last_modified_time(d_dataset), (*dds)->get_dap_version());

#if STORE_DAP2_RESULT_FEATURE
        // This means: if we are not supposed to store the result, then serialize it.
        if (!store_dap2_result(data_stream, **dds, eval)) {
            serialize_dap2_data_dds(data_stream, dds, eval, true /* was 'false'. jhrg 3/10/15 */);
        }
#else
        serialize_dap2_data_dds(data_stream, dds, eval, true /* was 'false'. jhrg 3/10/15 */);
#endif

    }
    else {
        BESDEBUG(MODULE, prolog << "Simple constraint" << endl);

        eval.parse_constraint(get_ce(), **dds); // Throws Error if the ce doesn't parse.
        (*dds)->tag_nested_sequences(); // Tag Sequences as Parent or Leaf node.

        dap_utils::throw_for_dap4_typed_vars_or_attrs(*dds, __FILE__, __LINE__);
        dap_utils::throw_if_too_big(**dds, __FILE__, __LINE__);

        if (with_mime_headers)
            set_mime_binary(data_stream, dods_data, x_plain, last_modified_time(d_dataset), (*dds)->get_dap_version());

#if STORE_DAP2_RESULT_FEATURE
        // This means: if we are not supposed to store the result, then serialize it.
        if (!store_dap2_result(data_stream, **dds, eval)) {
            serialize_dap2_data_dds(data_stream, dds, eval);
        }
#else
        serialize_dap2_data_dds(data_stream, dds, eval);
#endif
    }

    dap_utils::log_response_and_memory_size(prolog,dds);

#if 0
    struct rusage usage;
    int usage_val;
    usage_val = getrusage(RUSAGE_SELF, &usage);

    if (usage_val == 0){ //if getrusage() was successful, out put both sizes
        long mem_size = usage.ru_maxrss; // get the max size (man page says it is in kilobytes)
        INFO_LOG(prolog << "request size: "<< req_size << "KB -|- memory used by process: " << mem_size << "KB" << endl );
    }
    else { //if the getrusage() wasn't successful, only output the request size
        INFO_LOG(prolog << "request size: "<< req_size << "KB" << endl );
    }
#endif

    data_stream << flush;

    BESDEBUG(MODULE, prolog << "END"<< endl);

}
/** Send the DDX response. The DDX never contains data, instead it holds a
 reference to a Blob response which is used to get the data values. The
 DDS and DAS objects are built using code that already exists in the
 servers.

 @note This is the DAP3.x metadata response; it is supported by most DAP2
 servers as well. The DAP4 DDX will contain types not present in DAP2 or 3.x

 @param dds The dataset's DDS \e with attributes in the variables.
 @param eval A reference to the ConstraintEvaluator to use.
 @param out Destination
 @param with_mime_headers If true, include the MIME headers in the response.
 Defaults to true. */
void BESDapResponseBuilder::send_ddx(ostream &out, DDS **dds, ConstraintEvaluator &eval, bool with_mime_headers)
{
    // Verify the request hasn't exceeded bes_timeout, and disable timeout if allowed.
    RequestServiceTimer::TheTimer()->throw_if_timeout_expired(prolog +"ERROR: bes-timeout expired before transmit", __FILE__, __LINE__);
    BESUtil::conditional_timeout_cancel();

    if (d_dap2ce.empty()) {
        if (with_mime_headers)
            set_mime_text(out, dods_ddx, x_plain, last_modified_time(d_dataset), (*dds)->get_dap_version());

        (*dds)->print_xml_writer(out, false /*constrained */, "");
        //dds.print(out);
        out << flush;
        return;
    }

#if USE_LOCAL_TIMEOUT_SCHEME
    // Set up the alarm.
    establish_timeout(out);
    dds.set_timeout(d_timeout);
#endif

    // Split constraint into two halves
    split_ce(eval);

    // If there are functions, parse them and eval.
    // Use that DDS and parse the non-function ce
    // Serialize using the second ce and the second dds
    if (!d_btp_func_ce.empty()) {
        BESDapFunctionResponseCache *response_cache = BESDapFunctionResponseCache::get_instance();

        ConstraintEvaluator func_eval;
        DDS *fdds = nullptr;
        if (response_cache && response_cache->can_be_cached(*dds, get_btp_func_ce())) {
            fdds = response_cache->get_or_cache_dataset(*dds, get_btp_func_ce());
        }
        else {
            func_eval.parse_constraint(get_btp_func_ce(), **dds);
            fdds = func_eval.eval_function_clauses(**dds);
        }

        delete *dds; *dds = 0;
        *dds = fdds;

        (*dds)->mark_all(false);

        promote_function_output_structures(*dds);

        eval.parse_constraint(d_dap2ce, **dds);

        if (with_mime_headers)
            set_mime_text(out, dods_ddx, x_plain, last_modified_time(d_dataset), (*dds)->get_dap_version());

        // Verify the request hasn't exceeded bes_timeout, and disable timeout if allowed.
        RequestServiceTimer::TheTimer()->throw_if_timeout_expired(prolog +"ERROR: bes-timeout expired before transmit", __FILE__, __LINE__);
        BESUtil::conditional_timeout_cancel();

        (*dds)->print_xml_writer(out, true, "");
    }
    else {
        eval.parse_constraint(d_dap2ce, **dds); // Throws Error if the ce doesn't parse.

        if (with_mime_headers)
            set_mime_text(out, dods_ddx, x_plain, last_modified_time(d_dataset), (*dds)->get_dap_version());

        // Verify the request hasn't exceeded bes_timeout, and disable timeout if allowed.
        RequestServiceTimer::TheTimer()->throw_if_timeout_expired(prolog +"ERROR: bes-timeout expired before transmit", __FILE__, __LINE__);
        BESUtil::conditional_timeout_cancel();

        // dds.print_constrained(out);
        (*dds)->print_xml_writer(out, true, "");
    }

    out << flush;
}

void BESDapResponseBuilder::send_dmr(ostream &out, DMR &dmr, bool with_mime_headers)
{
    // If the CE is not empty, parse it. The projections, etc., are set as a side effect.
    // If the parser returns false, the expression did not parse. The parser may also
    // throw Error
    if (!d_dap4ce.empty()) {

        BESDEBUG(MODULE, prolog << "Parsing DAP4 constraint: '"<< d_dap4ce << "'"<< endl);

        D4ConstraintEvaluator parser(&dmr);
        bool parse_ok = parser.parse(d_dap4ce);
        if (!parse_ok){
            stringstream msg;
            msg << "Failed to parse the provided DAP4 server-side function expression: " << d_dap4function;
            throw BESSyntaxUserError(msg.str(),__FILE__,__LINE__);
        }
    }
    // with an empty CE, send everything. Even though print_dap4() and serialize()
    // don't need this, other code may depend on send_p being set. This may change
    // if DAP4 has a separate function evaluation phase. jhrg 11/25/13
    else {
        dmr.root()->set_send_p(true);
    }

    if (with_mime_headers) set_mime_text(out, dap4_dmr, x_plain, last_modified_time(d_dataset), dmr.dap_version());

    // Verify the request hasn't exceeded bes_timeout, and disable timeout if allowed.
    RequestServiceTimer::TheTimer()->throw_if_timeout_expired(prolog +"ERROR: bes-timeout expired before transmit", __FILE__, __LINE__);
    BESUtil::conditional_timeout_cancel();

    BESDEBUG(MODULE, prolog << "dmr.request_xml_base(): '"<< dmr.request_xml_base() << "' (dmr: " << (void *) &dmr << ")" << endl);

    if (dmr.get_utf8_xml_encoding()) {
        auto xml = XMLWriter("    ","UTF-8");
        dmr.print_dap4(xml, /*constrained &&*/!d_dap4ce.empty() /* true == constrained */);
        out << xml.get_doc() << flush;
    }
    else {
        XMLWriter xml;
        dmr.print_dap4(xml, /*constrained &&*/!d_dap4ce.empty() /* true == constrained */);
        out << xml.get_doc() << flush;
    }
}

void BESDapResponseBuilder::send_dap4_data_using_ce(ostream &out, DMR &dmr, bool with_mime_headers)
{
    if (!d_dap4ce.empty()) {
        BESDEBUG(MODULE , "BESDapResponseBuilder::send_dap4_data_using_ce() - expression constraint is not empty. " <<endl);
        D4ConstraintEvaluator parser(&dmr);
        bool parse_ok = parser.parse(d_dap4ce);
        if (!parse_ok){
            stringstream msg;
            msg << "Failed to parse the provided DAP4 server-side function expression: " << d_dap4function;
            throw BESSyntaxUserError(msg.str(),__FILE__,__LINE__);
        }
    }
    // with an empty CE, send everything. Even though print_dap4() and serialize()
    // don't need this, other code may depend on send_p being set. This may change
    // if DAP4 has a separate function evaluation phase. jhrg 11/25/13
    else {
        dmr.set_ce_empty(true);
        dmr.root()->set_send_p(true);
    }

    dap_utils::log_response_and_memory_size(prolog, dmr);
    dap_utils::throw_if_too_big(dmr, __FILE__, __LINE__);

    // The following block is for debugging purpose. KY 05/13/2020
#if !NDEBUG
    for (auto i = dmr.root()->var_begin(), e = dmr.root()->var_end(); i != e; ++i) {
        BESDEBUG(MODULE , prolog << (*i)->name() << endl);
        if ((*i)->send_p()) {
            BESDEBUG(MODULE , prolog << "Obtain data- " << (*i)->name() << endl);
            D4Attributes *d4_attrs = (*i)->attributes();
            BESDEBUG(MODULE , prolog << "Number of attributes " << d4_attrs << endl);
            for (auto ii = d4_attrs->attribute_begin(), ee = d4_attrs->attribute_end(); ii != ee; ++ii) {
                BESDEBUG(MODULE ,prolog << "Attribute name is " << (*ii)->name() << endl);
            }
        }
    }
#endif

    if (!store_dap4_result(out, dmr)) {
        serialize_dap4_data(out, dmr, with_mime_headers);
    }
}

/**
 * @brief Parse the DAP4 CE and throw if the request is too large
 *
 * The CE parsed is the one set in this BESDapREsponseBuilder
 * instance and is parsed using the DMR passed to this method.
 *
 * @param dmr Parse the CE using this DMR
 */
void BESDapResponseBuilder::dap4_process_ce_for_intern_data(DMR &dmr)
{
    BES_STOPWATCH_START(MODULE, prolog + "Timer");
    if (!d_dap4ce.empty()) {
        BESDEBUG(MODULE , prolog << "Expression constraint is not empty. " <<endl);
        D4ConstraintEvaluator parser(&dmr);
        bool parse_ok = parser.parse(d_dap4ce);
        if (!parse_ok){
            stringstream msg;
            msg << "Failed to parse the provided DAP4 server-side function expression: " << d_dap4function;
            throw BESSyntaxUserError(msg.str(),__FILE__,__LINE__);
        }
    }
    // with an empty CE, send everything. Even though print_dap4() and serialize()
    // don't need this, other code may depend on send_p being set. This may change
    // if DAP4 has a separate function evaluation phase. jhrg 11/25/13
    else {
        dmr.set_ce_empty(true);
        dmr.root()->set_send_p(true);
    }
    dap_utils::throw_if_too_big(dmr, __FILE__, __LINE__);
}

void BESDapResponseBuilder::send_dap4_data(ostream &out, DMR &dmr, bool with_mime_headers)
{
    // If a function was passed in with this request, evaluate it and use that DMR
    // for the remainder of this request.
    // TODO Add caching for these function invocations
    if (!d_dap4function.empty()) {
        D4BaseTypeFactory d4_factory;
        DMR function_result(&d4_factory, "function_results");

        // Function modules load their functions onto this list. The list is
        // part of libdap, not the BES.
        if (!ServerFunctionsList::TheList()) {
            stringstream msg;
            msg << "The function expression could not be evaluated because ";
            msg << "there are no server-side functions defined on this server.";
            throw BESSyntaxUserError(msg.str(),__FILE__,__LINE__);
        }

        D4FunctionEvaluator parser(&dmr, ServerFunctionsList::TheList());
        bool parse_ok = parser.parse(d_dap4function);
        if (!parse_ok){
            stringstream msg;
            msg << "Failed to parse the provided DAP4 server-side function expression: " << d_dap4function;
            throw BESSyntaxUserError(msg.str(),__FILE__,__LINE__);
        }
        parser.eval(&function_result);

        // Now use the results of running the functions for the remainder of the
        // send_data operation.
        send_dap4_data_using_ce(out, function_result, with_mime_headers);
    }
    else {
        send_dap4_data_using_ce(out, dmr, with_mime_headers);
    }
}

const auto DAP4_CHECKSUMS_KEY="dap4_checksums";

bool use_dap4_checksums() {
    bool found;
    string state = "unset";
    state = BESContextManager::TheManager()->get_context(DAP4_CHECKSUMS_KEY, found);
    if (!found) {
        state="false";
    }
    BESDEBUG(MODULE, prolog << DAP4_CHECKSUMS_KEY << ": " << state << "\n");
    return found && (BESUtil::lowercase(state) == "true");
}

/**
 * Serialize the DAP4 data response to the passed stream
 */
void BESDapResponseBuilder::serialize_dap4_data(std::ostream &out, libdap::DMR &dmr, bool with_mime_headers)
{
    BES_STOPWATCH_START(MODULE, prolog + "Timer");
    BESDEBUG(MODULE, prolog << "BEGIN" << endl);

    bool ucs = use_dap4_checksums();
    BESDEBUG(MODULE, prolog << "use_dap4_checksums: " << (ucs?"true":"false") << "\n");
    dmr.use_checksums(ucs);

    if (with_mime_headers) set_mime_binary(out, dap4_data, x_plain, last_modified_time(d_dataset), dmr.dap_version());

    BESDEBUG(MODULE, prolog << "dmr.request_xml_base(): \"" << dmr.request_xml_base() << "\""<< endl);

    // Write the DMR
    XMLWriter xml;
    if (dmr.get_utf8_xml_encoding()) 
        xml = XMLWriter("    ","UTF-8");
    dmr.print_dap4(xml, !d_dap4ce.empty());

    // now make the chunked output stream; set the size to be at least chunk_size
    // but make sure that the whole of the xml plus the CRLF can fit in the first
    // chunk. (+2 for the CRLF bytes).
    chunked_ostream cos(out, max((unsigned int) CHUNK_SIZE, xml.get_doc_size() + 2));

    // Verify the request hasn't exceeded bes_timeout, and disable timeout if allowed.
    RequestServiceTimer::TheTimer()->throw_if_timeout_expired(prolog +"ERROR: bes-timeout expired before transmit", __FILE__, __LINE__);
    BESUtil::conditional_timeout_cancel();

    // using flush means that the DMR and CRLF are in the first chunk.
    cos << xml.get_doc() << CRLF << flush;

    // Write the data, chunked with checksums
    D4StreamMarshaller m(cos);
    dmr.root()->serialize(m, dmr, !d_dap4ce.empty());
#ifdef CLEAR_LOCAL_DATA
    dmr.root()->clear_local_data();
#endif
    cos << flush;

    BESDEBUG(MODULE, prolog << "END" << endl);
}

/**
 * Should this DAP4 result be stored and the client sent an Asynchronous response?
 * This code looks at the 'store_result' property to determine if the response should
 * be asynchronous and then, if so, churns through the options, sorting out if the
 * client sent the correct information indicating it knows how to process them. If
 * the 'store_result' property is not set, then this method simply returns false,
 * indicating that the response should be returned using the normal synchronous
 * response pattern.
 *
 * @param out Write information about the response here
 * @param dmr This DMR will be serialized to build the response, at some point in time
 * @return True if the response will be Asynchronous, false if the DMR should be sent
 * right away.
 */
bool BESDapResponseBuilder::store_dap4_result(ostream &out, libdap::DMR &dmr)
{
    if (!get_store_result().empty()) {
        string serviceUrl = get_store_result();

        D4AsyncUtil d4au;
        XMLWriter xmlWrtr;
        if (dmr.get_utf8_xml_encoding()) 
            xmlWrtr = XMLWriter("    ","UTF-8");
 
        // FIXME See above comment for store dap2 result
        bool found;
        string *stylesheet_ref = nullptr, ss_ref_value;
        TheBESKeys::TheKeys()->get_value(D4AsyncUtil::STYLESHEET_REFERENCE_KEY, ss_ref_value, found);
        if (found && !ss_ref_value.empty()) {
            stylesheet_ref = &ss_ref_value;
        }

        BESStoredDapResultCache *resultCache = BESStoredDapResultCache::get_instance();
        if (resultCache == nullptr) {

            /**
             * OOPS. Looks like the BES is not configured to use a Stored Result Cache.
             * Looks like need to reject the request and move on.
             *
             */
            string msg = "The Stored Result request cannot be serviced. ";
            msg += "Unable to acquire StoredResultCache instance. ";
            msg += "This is most likely because the StoredResultCache is not (correctly) configured.";

            BESDEBUG(MODULE, prolog << "[WARNING] " << msg << endl);
            d4au.writeD4AsyncResponseRejected(xmlWrtr, UNAVAILABLE, msg, stylesheet_ref);
            out << xmlWrtr.get_doc();
            out << flush;
            BESDEBUG(MODULE, prolog << "Sent AsyncRequestRejected" << endl);

            return true;
        }

        if (!get_async_accepted().empty()) {

            /**
             * Client accepts async responses so, woot! lets store this thing and tell them where to find it.
             */
            BESDEBUG(MODULE, prolog << "serviceUrl="<< serviceUrl << endl);

            string storedResultId;
            storedResultId = resultCache->store_dap4_result(dmr, get_ce(), this);

            BESDEBUG(MODULE,prolog << "storedResultId='"<< storedResultId << "'" << endl);

            string targetURL = BESUtil::assemblePath(serviceUrl, storedResultId);
            BESDEBUG(MODULE, prolog << "targetURL='"<< targetURL << "'" << endl);

            d4au.writeD4AsyncAccepted(xmlWrtr, 0, 0, targetURL, stylesheet_ref);
            out << xmlWrtr.get_doc();
            out << flush;
            BESDEBUG(MODULE, prolog << "Sent AsyncAccepted" << endl);

        }
        else {
            /**
             * Client didn't indicate a willingness to accept an async response
             * So - we tell them that async is required.
             */
            d4au.writeD4AsyncRequired(xmlWrtr, 0, 0, stylesheet_ref);
            out << xmlWrtr.get_doc();
            out << flush;
            BESDEBUG(MODULE, prolog << "Sent AsyncAccepted" << endl);
        }

        return true;
    }

    return false;
}

/**
 * The description is mostly cloned from the function intern_dap2_data().
 * Read data into the ResponseObject (a DAP4 DMR). Instead of serializing the data
 * as with send_dap4_data(), this uses the variables' intern_dap4_data() method to load
 * the variables so that a transmitter other than libdap::BaseType::serialize()
 * can be used (e.g., AsciiTransmitter).
 *
 * @note Other versions of the methods here (e.g., send_dap4_data()) optionally
 * print MIME headers because this code was used in the past to build HTTP responses.
 * That's only the case with the CEDAR server and I'm not sure we need to maintain
 * that code. TBD. This method will not be used by CEDAR, so I've not included the
 * 'print_mime_headers' bool.
 *
 * @param obj The BESResponseObject. Holds the DMR for this request.
 * @param dhi The BESDataHandlerInterface. Holds many parameters for this request.
 * @return The DMR* is returned where each variable marked to be sent is loaded with
 * data (as per the current constraint expression).
 */
libdap::DMR *
BESDapResponseBuilder::intern_dap4_data(BESResponseObject *obj, BESDataHandlerInterface &dhi)
{
    BES_STOPWATCH_START(MODULE, prolog + "Timer");
    BESDEBUG(MODULE , prolog << "BEGIN" << endl);

    unique_ptr<DMR> dmr = setup_dap4_intern_data(obj, dhi);

    intern_dap4_data_grp(dmr->root());

    return dmr.release();
}

libdap::DMR *
BESDapResponseBuilder::process_dap4_dmr(BESResponseObject *obj, BESDataHandlerInterface &dhi)
{
    BES_STOPWATCH_START(MODULE, prolog + "Timer");
    BESDEBUG(MODULE , prolog << "BEGIN" << endl);

    unique_ptr<DMR> dmr = setup_dap4_intern_data(obj, dhi);

    return dmr.release();
}

unique_ptr<DMR>
BESDapResponseBuilder::setup_dap4_intern_data(BESResponseObject *obj, BESDataHandlerInterface &dhi)
{
    dhi.first_container();

    auto bdmr = dynamic_cast<BESDMRResponse *>(obj);
    if (!bdmr) throw BESInternalFatalError("Expected a BESDMRResponse instance", __FILE__, __LINE__);

    unique_ptr<DMR> dmr(bdmr->get_dmr());
    // TL;DR Set the DMR managed by the BESResponseObject to nullptr to avoid calling ~DMR
    // twice on the same object.
    bdmr->set_dmr(nullptr);
    // Why this is here: In the past we designed the BESResponseObject class hierarchy to
    // manage the response object, which effectively means delete it when the BES is done
    // processing the request. We pass nullptr to set_dmr so that the BESResponseObject
    // does not call ~DMR since unique_ptr<> will do that for us.

    // Set the correct context by following intern_dap2_data()
    set_dataset_name(dmr->filename());
    set_dap4ce(dhi.data[DAP4_CONSTRAINT]);
    set_dap4function(dhi.data[DAP4_FUNCTION]);
    set_async_accepted(dhi.data[ASYNC]);
    set_store_result(dhi.data[STORE_RESULT]);

    if (!d_dap4function.empty()) {
        D4BaseTypeFactory d4_factory;
        unique_ptr<DMR> function_result(new DMR(&d4_factory, "function_results"));

        // Function modules load their functions onto this list. The list is
        // part of libdap, not the BES.
        if (!ServerFunctionsList::TheList()) {
            stringstream msg;
            msg << "The function expression could not be evaluated because ";
            msg << "there are no server-side functions defined on this server.";
            throw BESSyntaxUserError(msg.str(),__FILE__,__LINE__);
        }

        D4FunctionEvaluator parser(dmr.get(), ServerFunctionsList::TheList());
        bool parse_ok = parser.parse(d_dap4function);
        if (!parse_ok){
            stringstream msg;
            msg << "Failed to parse the provided DAP4 server-side function expression: " << d_dap4function;
            throw BESSyntaxUserError(msg.str(),__FILE__,__LINE__);
        }

        parser.eval(function_result.get());

        // Now use the results of running the functions for the remainder of the
        // send_data operation.
        dap4_process_ce_for_intern_data(*function_result);

        return function_result;
    }
    else {
        BESDEBUG(MODULE , prolog << "Processing the constraint expression. " << endl);
        dap4_process_ce_for_intern_data(*dmr);
        return dmr;
    }
}

void BESDapResponseBuilder::intern_dap4_data_grp(libdap::D4Group* grp) {
    for (auto i = grp->var_begin(), e = grp->var_end(); i != e; ++i) {
        BESDEBUG(MODULE , "BESDapResponseBuilder::intern_dap4_data() - "<< (*i)->name() <<endl);
        if ((*i)->send_p()) {
            BESDEBUG(MODULE , "BESDapResponseBuilder::intern_dap4_data() Obtain data- "<< (*i)->name() <<endl);
            (*i)->intern_data();
        }
    }

    for (auto gi = grp->grp_begin(), ge = grp->grp_end(); gi != ge; ++gi) {
        BESDEBUG(MODULE , "BESDapResponseBuilder::intern_dap4_data() group- "<< (*gi)->name() <<endl);
        intern_dap4_data_grp(*gi);
    }
}
