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

#include <DAS.h>
#include <DDS.h>
#include <Structure.h>
#include <ConstraintEvaluator.h>
#include <DDXParserSAX2.h>
#include <Ancillary.h>
#include <XDRStreamMarshaller.h>
#include <XDRFileUnMarshaller.h>

#include <DMR.h>
#include <D4Group.h>
#include <XMLWriter.h>
#include <D4AsyncUtil.h>
#include <D4StreamMarshaller.h>
#include <chunked_ostream.h>
#include <chunked_istream.h>
#include <D4ConstraintEvaluator.h>
#include <D4FunctionEvaluator.h>
#include <D4BaseTypeFactory.h>

#include <ServerFunctionsList.h>

#include <mime_util.h>	// for last_modified_time() and rfc_822_date()
#include <escaping.h>
#include <util.h>
#if USE_LOCAL_TIMEOUT_SCHEME
#ifndef WIN32
#include <SignalHandler.h>
#include <EventHandler.h>
#include <AlarmHandler.h>
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
#include "BESDataNames.h"

#include "BESRequestHandler.h"
#include "BESRequestHandlerList.h"
#include "BESNotFoundError.h"

#include "BESUtil.h"
#include "BESDebug.h"
#include "BESStopWatch.h"
#include "DapFunctionUtils.h"

using namespace std;
using namespace libdap;

const string CRLF = "\r\n";             // Change here, expr-test.cc
const string BES_KEY_TIMEOUT_CANCEL = "BES.CancelTimeoutOnSend";

/**
 * Look up the BES Keys (parameters in the bes.conf file) that this class
 * uses.
 */
void BESDapResponseBuilder::initialize()
{
    bool found = false;
    string cancel_timeout_on_send = "";
    TheBESKeys::TheKeys()->get_value(BES_KEY_TIMEOUT_CANCEL, cancel_timeout_on_send, found);
    if (found && !cancel_timeout_on_send.empty()) {
        // The default value is false.
        downcase(cancel_timeout_on_send);
        if (cancel_timeout_on_send == "yes" || cancel_timeout_on_send == "true")
            d_cancel_timeout_on_send = true;
    }
}

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
void BESDapResponseBuilder::set_dap4ce(string _ce)
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
void BESDapResponseBuilder::set_dap4function(string _func)
{
    d_dap4function = www2id(_func, "%", "%20");
}

std::string BESDapResponseBuilder::get_store_result() const
{
    return d_store_result;
}

void BESDapResponseBuilder::set_store_result(std::string _sr)
{
    d_store_result = _sr;
    BESDEBUG("dap", "BESDapResponseBuilder::set_store_result() - store_result: " << _sr << endl);
}

std::string BESDapResponseBuilder::get_async_accepted() const
{
    return d_async_accepted;
}

void BESDapResponseBuilder::set_async_accepted(std::string _aa)
{
    d_async_accepted = _aa;
    BESDEBUG("dap", "BESDapResponseBuilder::set_async_accepted() - async_accepted: " << _aa << endl);
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
void BESDapResponseBuilder::set_dataset_name(const string ds)
{
    d_dataset = www2id(ds, "%", "%20");
}

/** Set the server's timeout value. A value of zero (the default) means no
 timeout.

 @see To establish a timeout, call establish_timeout(ostream &)
 @param t Server timeout in seconds. Default is zero (no timeout). */
void BESDapResponseBuilder::set_timeout(int t)
{
    d_timeout = t;
}

/** Get the server's timeout value. */
int BESDapResponseBuilder::get_timeout() const
{
    return d_timeout;
}

/**
 * Turn on the alarm. The code will timeout after d_timeout
 * seconds unless timeout_off() is called first.
 *
 * @deprecated
 */
void
BESDapResponseBuilder::timeout_on() const
{
#if USE_LOCAL_TIMEOUT_SCHEME
#ifndef WIN32
    alarm(d_timeout);
#endif
#endif
}

/**
 * Turn off the timeout.
 *
 * @deprecated
 */
void
BESDapResponseBuilder::timeout_off()
{
#if USE_LOCAL_TIMEOUT_SCHEME
#ifndef WIN32
    alarm(0);
#endif
#endif
}

/**
 * If the value of the BES Key BES.CancelTimeoutOnSend is true, cancel the
 * timeout. The intent of this is to stop the timeout counter once the
 * BES starts sending data back since, the network link used by a remote
 * client may be low-bandwidth and data providers might want to ensure those
 * users get their data (and don't submit second, third, ..., requests when/if
 * the first one fails). The timeout is initiated in the BES framework when it
 * first processes the request.
 *
 * @note The BES timeout is set/controlled in bes/dispatch/BESInterface
 * in the 'int BESInterface::execute_request(const string &from)' method.
 *
 * @see BESInterface::execute_request(const string &from)
 */
void BESDapResponseBuilder::conditional_timeout_cancel()
{
    if (d_cancel_timeout_on_send)
        alarm(0);
}

/**
 * Configure a signal handler for the SIGALRM. The signal handler
 * will throw a DAP Error object if the alarm signal is triggered.
 *
 * @deprecated
 *
 * @return void
 */
void BESDapResponseBuilder::register_timeout() const
{
#if USE_LOCAL_TIMEOUT_SCHEME
#ifndef WIN32
    SignalHandler *sh = SignalHandler::instance();
    EventHandler *old_eh = sh->register_handler(SIGALRM, new AlarmHandler());
    delete old_eh;
#endif
#endif
}


/** Use values of this instance to establish a timeout alarm for the server.
 * If the timeout value is zero, do nothing.
 *
 * @deprecated
 */
void BESDapResponseBuilder::establish_timeout(ostream &) const
{
#if USE_LOCAL_TIMEOUT_SCHEME
#ifndef WIN32
    if (d_timeout > 0) {
        SignalHandler *sh = SignalHandler::instance();
        EventHandler *old_eh = sh->register_handler(SIGALRM, new AlarmHandler());
        delete old_eh;
        alarm(d_timeout);
    }
#endif
#endif
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
        if (pos == string::npos)
            throw Error(malformed_expr, "Expected to find a matching closing parenthesis in " + ce);

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
    BESDEBUG("dap", "BESDapResponseBuilder::split_ce() - source expression: " << expr << endl);

    string ce;
    if (!expr.empty())
        ce = expr;
    else
        ce = d_dap2ce;

    string btp_function_ce = "";
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
            if (pos < ce.length() && ce.at(pos) == ',') ++pos;
        }

        first_paren = ce.find("(", pos);
        closing_paren = ce.find(")", pos);
    }

    d_dap2ce = ce;
    d_btp_func_ce = btp_function_ce;

    BESDEBUG("dap", "BESDapResponseBuilder::split_ce() - Modified constraint: " << d_dap2ce << endl);
    BESDEBUG("dap", "BESDapResponseBuilder::split_ce() - BTP Function part: " << btp_function_ce << endl);
    BESDEBUG("dap", "BESDapResponseBuilder::split_ce() - END" << endl);
}

/**
 * @brief convenience function for the response limit test.
 * The DDS stores the response size limit in Bytes even though the context
 * param uses KB. The DMR uses KB throughout.
 * @param dds
 */
static void
throw_if_dap2_response_too_big(DDS *dds)
{
    if (dds->get_response_limit() != 0 && ((dds->get_request_size(true)) > dds->get_response_limit())) {
        string msg = "The Request for " + long_to_string(dds->get_request_size(true) / 1024)
            + "KB is too large; requests on this server are limited to "
            + long_to_string(dds->get_response_limit() /1024) + "KB.";
        throw Error(msg);
    }
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
    if (!constrained) {
        if (with_mime_headers) set_mime_text(out, dods_das, x_plain, last_modified_time(d_dataset), "2.0");

        conditional_timeout_cancel();

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

        DDS *fdds = 0; // nulll_ptr
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

        conditional_timeout_cancel();

        (*dds)->print_das(out);
    }
    else {
        eval.parse_constraint(d_dap2ce, **dds); // Throws Error if the ce doesn't parse.

        if (with_mime_headers)
            set_mime_text(out, dods_das, x_plain, last_modified_time(d_dataset), (*dds)->get_dap_version());

        conditional_timeout_cancel();

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

        conditional_timeout_cancel();

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
        ConstraintEvaluator func_eval;

        BESDapFunctionResponseCache *responseCache = BESDapFunctionResponseCache::get_instance();

        DDS *fdds = 0; // nulll_ptr
        if (responseCache && responseCache->can_be_cached(*dds, get_btp_func_ce())) {
            fdds = responseCache->get_or_cache_dataset(*dds, get_btp_func_ce());
        }
        else {
            func_eval.parse_constraint(get_btp_func_ce(), **dds);
            fdds = func_eval.eval_function_clauses(**dds);
        }

        delete *dds; *dds = 0;
        *dds = fdds;

        // Server functions might mark variables to use their read()
        // methods. Clear that so the CE in d_dap2ce will control what is
        // sent. If that is empty (there was only a function call) all
        // of the variables in the intermediate DDS (i.e., the function
        // result) will be sent.
        (*dds)->mark_all(false);

        // This next step utilizes a well known static method (so really it's a function;),
        // promote_function_output_structures() to look for
        // one or more top level Structures whose name indicates (by way of ending with
        // "_uwrap") that their contents should be promoted (aka moved) to the top level.
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


        conditional_timeout_cancel();

        (*dds)->print_constrained(out);
    }
    else {
        eval.parse_constraint(d_dap2ce, **dds); // Throws Error if the ce doesn't parse.

        if (with_mime_headers)
            set_mime_text(out, dods_dds, x_plain, last_modified_time(d_dataset),(*dds)->get_dap_version());

        conditional_timeout_cancel();

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
    if (found && ss_ref_value.length() > 0) {
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

        BESDEBUG("dap", "[WARNING] " << msg << endl);

        d4au.writeD4AsyncResponseRejected(xmlWrtr, UNAVAILABLE, msg, stylesheet_ref);
        out << xmlWrtr.get_doc();
        out << flush;

        BESDEBUG("dap", "BESDapResponseBuilder::store_dap2_result() - Sent AsyncRequestRejected" << endl);
    }
    else if (get_async_accepted().length() != 0) {

        /**
         * Client accepts async responses so, woot! lets store this thing and tell them where to find it.
         */
        BESDEBUG("dap", "BESDapResponseBuilder::store_dap2_result() - serviceUrl="<< serviceUrl << endl);

        BESStoredDapResultCache *resultCache = BESStoredDapResultCache::get_instance();
        string storedResultId = "";
        storedResultId = resultCache->store_dap2_result(dds, get_ce(), this, &eval);

        BESDEBUG("dap",
            "BESDapResponseBuilder::store_dap2_result() - storedResultId='"<< storedResultId << "'" << endl);

        string targetURL = BESUtil::assemblePath(serviceUrl, storedResultId);
        BESDEBUG("dap", "BESDapResponseBuilder::store_dap2_result() - targetURL='"<< targetURL << "'" << endl);

        XMLWriter xmlWrtr;
        d4au.writeD4AsyncAccepted(xmlWrtr, 0, 0, targetURL, stylesheet_ref);
        out << xmlWrtr.get_doc();
        out << flush;

        BESDEBUG("dap", "BESDapResponseBuilder::store_dap2_result() - sent DAP4 AsyncAccepted response" << endl);
    }
    else {
        /**
         * Client didn't indicate a willingness to accept an async response
         * So - we tell them that async is required.
         */
        d4au.writeD4AsyncRequired(xmlWrtr, 0, 0, stylesheet_ref);
        out << xmlWrtr.get_doc();
        out << flush;

        BESDEBUG("dap", "BESDapResponseBuilder::store_dap2_result() - sent DAP4 AsyncRequired  response" << endl);
    }

    return true;

}
#endif

/**
 * Build/return the BLOB part of the DAP2 data response.
 */
void BESDapResponseBuilder::serialize_dap2_data_dds(ostream &out, DDS **dds, ConstraintEvaluator &eval, bool ce_eval)
{
    BESStopWatch sw;
    if (BESISDEBUG(TIMING_LOG)) sw.start("BESDapResponseBuilder::serialize_dap2_data_dds", "");

    BESDEBUG("dap", "BESDapResponseBuilder::serialize_dap2_data_dds() - BEGIN" << endl);

    (*dds)->print_constrained(out);
    out << "Data:\n";
    out << flush;

    XDRStreamMarshaller m(out);

    // This only has an effect when the timeout in BESInterface::execute_request()
    // is set. Otherwise it does nothing.
    conditional_timeout_cancel();

    // Send all variables in the current projection (send_p())
    for (DDS::Vars_iter i = (*dds)->var_begin(); i != (*dds)->var_end(); i++) {
        if ((*i)->send_p()) {
            (*i)->serialize(eval, **dds, m, ce_eval);
#ifdef CLEAR_LOCAL_DATA
            (*i)->clear_local_data();
#endif
        }
    }

    BESDEBUG("dap", "BESDapResponseBuilder::serialize_dap2_data_dds() - END" << endl);
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
    BESDEBUG("dap", __PRETTY_FUNCTION__ << " BEGIN" << endl);

    // Write the MPM headers for the DDX (text/xml) part of the response
    libdap::set_mime_ddx_boundary(out, boundary, start, dods_ddx, x_plain);

    // Make cid
    uuid_t uu;
    uuid_generate(uu);
    char uuid[37];
    uuid_unparse(uu, &uuid[0]);
    char domain[256];
    if (getdomainname(domain, 255) != 0 || strlen(domain) == 0) strncpy(domain, "opendap.org", 255);

    string cid = string(&uuid[0]) + "@" + string(&domain[0]);

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

    BESDEBUG("dap", __PRETTY_FUNCTION__ << " END" << endl);
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
void BESDapResponseBuilder::remove_timeout() const
{
#if USE_LOCAL_TIMEOUT_SCHEME
    alarm(0);
#endif
}

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
    BESDEBUG("dap", "BESDapResponseBuilder::process_dap2_dds() - BEGIN"<< endl);

    dhi.first_container();

    BESDDSResponse *bdds = dynamic_cast<BESDDSResponse *>(obj);
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
        DDS *fdds = 0; // nulll_ptr
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
    BESDEBUG("dap", "BESDapResponseBuilder::intern_dap2_data() - BEGIN"<< endl);

    dhi.first_container();

    BESDataDDSResponse *bdds = dynamic_cast<BESDataDDSResponse *>(obj);
    if (!bdds) throw BESInternalFatalError("Expected a BESDataDDSResponse instance", __FILE__, __LINE__);

    DDS *dds = bdds->get_dds();

    set_dataset_name(dds->filename());
    set_ce(dhi.data[POST_CONSTRAINT]);
    set_async_accepted(dhi.data[ASYNC]);
    set_store_result(dhi.data[STORE_RESULT]);

        
    // This function is used by all fileout modules and they need to include the attributes in data access.
    // So obtain the attributes if necessary. KY 2019-10-30
    if(bdds->get_ia_flag() == false) {
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
        BESDEBUG("dap",
            "BESDapResponseBuilder::intern_dap2_data() - Found function(s) in CE: " << get_btp_func_ce() << endl);

        BESDapFunctionResponseCache *responseCache = BESDapFunctionResponseCache::get_instance();

        ConstraintEvaluator func_eval;
        DDS *fdds = 0; // nulll_ptr
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
        // of the variables in the intermediate DDS (i.e., the function
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

    throw_if_dap2_response_too_big(dds);

    // Iterate through the variables in the DataDDS and read
    // in the data if the variable has the send flag set.
    for (DDS::Vars_iter i = dds->var_begin(), e = dds->var_end(); i != e; ++i) {
        if ((*i)->send_p()) {
            (*i)->intern_data(eval, *dds);
        }
    }

    BESDEBUG("dap", "BESDapResponseBuilder::intern_dap2_data() - END"<< endl);

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
    BESDEBUG("dap", "BESDapResponseBuilder::send_dap2_data() - BEGIN"<< endl);

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
        BESDEBUG("dap",
            "BESDapResponseBuilder::send_dap2_data() - Found function(s) in CE: " << get_btp_func_ce() << endl);

        BESDapFunctionResponseCache *response_cache = BESDapFunctionResponseCache::get_instance();

        ConstraintEvaluator func_eval;
        DDS *fdds = 0; // nulll_ptr
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

        // evaluate the rest of the CE - the part that follows the function calls.
        eval.parse_constraint(get_ce(), **dds);

        (*dds)->tag_nested_sequences(); // Tag Sequences as Parent or Leaf node.

        throw_if_dap2_response_too_big(*dds);

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
        BESDEBUG("dap", "BESDapResponseBuilder::send_dap2_data() - Simple constraint" << endl);

        eval.parse_constraint(get_ce(), **dds); // Throws Error if the ce doesn't parse.

        (*dds)->tag_nested_sequences(); // Tag Sequences as Parent or Leaf node.

        throw_if_dap2_response_too_big(*dds);

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

    data_stream << flush;

    BESDEBUG("dap", "BESDapResponseBuilder::send_dap2_data() - END"<< endl);

}

void BESDapResponseBuilder::send_dap2_data(BESDataHandlerInterface &dhi, DDS **dds, ConstraintEvaluator &eval,
    bool with_mime_headers)
{
    BESDEBUG("dap", "BESDapResponseBuilder::send_dap2_data() - BEGIN"<< endl);

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
        BESDEBUG("dap",
            "BESDapResponseBuilder::send_dap2_data() - Found function(s) in CE: " << get_btp_func_ce() << endl);

        // Server-side functions need to include the attributes in data access.
        // So obtain the attributes if necessary. KY 2019-10-30
        {
            BESResponseObject *response = dhi.response_handler->get_response_object();
            BESDataDDSResponse *bdds = dynamic_cast<BESDataDDSResponse *> (response);
            if (!bdds)
                throw BESInternalError("cast error", __FILE__, __LINE__);

            if(bdds->get_ia_flag() == false) {
                BESRequestHandler *besRH = BESRequestHandlerList::TheList()->find_handler(dhi.container->get_container_type());
                besRH->add_attributes(dhi);
            }
        }

        BESDapFunctionResponseCache *response_cache = BESDapFunctionResponseCache::get_instance();
        ConstraintEvaluator func_eval;
        DDS *fdds = 0; // nulll_ptr
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

        // evaluate the rest of the CE - the part that follows the function calls.
        eval.parse_constraint(get_ce(), **dds);

        (*dds)->tag_nested_sequences(); // Tag Sequences as Parent or Leaf node.

        throw_if_dap2_response_too_big(*dds);

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
        BESDEBUG("dap", "BESDapResponseBuilder::send_dap2_data() - Simple constraint" << endl);

        eval.parse_constraint(get_ce(), **dds); // Throws Error if the ce doesn't parse.

        (*dds)->tag_nested_sequences(); // Tag Sequences as Parent or Leaf node.

        throw_if_dap2_response_too_big(*dds);

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

    data_stream << flush;

    BESDEBUG("dap", "BESDapResponseBuilder::send_dap2_data() - END"<< endl);

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
        DDS *fdds = 0; // nulll_ptr
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

        conditional_timeout_cancel();

        (*dds)->print_xml_writer(out, true, "");
    }
    else {
        eval.parse_constraint(d_dap2ce, **dds); // Throws Error if the ce doesn't parse.

        if (with_mime_headers)
            set_mime_text(out, dods_ddx, x_plain, last_modified_time(d_dataset), (*dds)->get_dap_version());

        conditional_timeout_cancel();


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

        BESDEBUG("dap", "BESDapResponseBuilder::send_dmr() - Parsing DAP4 constraint: '"<< d_dap4ce << "'"<< endl);

        D4ConstraintEvaluator parser(&dmr);
        bool parse_ok = parser.parse(d_dap4ce);
        if (!parse_ok) throw Error(malformed_expr, "Constraint Expression (" + d_dap4ce + ") failed to parse.");
    }
    // with an empty CE, send everything. Even though print_dap4() and serialize()
    // don't need this, other code may depend on send_p being set. This may change
    // if DAP4 has a separate function evaluation phase. jhrg 11/25/13
    else {
        dmr.root()->set_send_p(true);
    }

    if (with_mime_headers) set_mime_text(out, dap4_dmr, x_plain, last_modified_time(d_dataset), dmr.dap_version());

    conditional_timeout_cancel();


    XMLWriter xml;
    dmr.print_dap4(xml, /*constrained &&*/!d_dap4ce.empty() /* true == constrained */);
    out << xml.get_doc() << flush;
}

void BESDapResponseBuilder::send_dap4_data_using_ce(ostream &out, DMR &dmr, bool with_mime_headers)
{
    if (!d_dap4ce.empty()) {
        D4ConstraintEvaluator parser(&dmr);
        bool parse_ok = parser.parse(d_dap4ce);
        if (!parse_ok) throw Error(malformed_expr, "Constraint Expression (" + d_dap4ce + ") failed to parse.");
    }
    // with an empty CE, send everything. Even though print_dap4() and serialize()
    // don't need this, other code may depend on send_p being set. This may change
    // if DAP4 has a separate function evaluation phase. jhrg 11/25/13
    else {
        dmr.root()->set_send_p(true);
    }

    if (dmr.response_limit() != 0 && (dmr.request_size(true) > dmr.response_limit())) {
        string msg = "The Request for " + long_to_string(dmr.request_size(true))
            + "KB is too large; requests for this server are limited to " + long_to_string(dmr.response_limit())
            + "KB.";
        throw Error(msg);
    }

    if (!store_dap4_result(out, dmr)) {
        serialize_dap4_data(out, dmr, with_mime_headers);
    }
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
        if (!ServerFunctionsList::TheList())
            throw Error(
                "The function expression could not be evaluated because there are no server functions defined on this server");

        D4FunctionEvaluator parser(&dmr, ServerFunctionsList::TheList());
        bool parse_ok = parser.parse(d_dap4function);
        if (!parse_ok) throw Error("Function Expression (" + d_dap4function + ") failed to parse.");

        parser.eval(&function_result);

        // Now use the results of running the functions for the remainder of the
        // send_data operation.
        send_dap4_data_using_ce(out, function_result, with_mime_headers);
    }
    else {
        send_dap4_data_using_ce(out, dmr, with_mime_headers);
    }
}

/**
 * Serialize the DAP4 data response to the passed stream
 */
void BESDapResponseBuilder::serialize_dap4_data(std::ostream &out, libdap::DMR &dmr, bool with_mime_headers)
{
    BESDEBUG("dap", "BESDapResponseBuilder::serialize_dap4_data() - BEGIN" << endl);

    if (with_mime_headers) set_mime_binary(out, dap4_data, x_plain, last_modified_time(d_dataset), dmr.dap_version());

    // Write the DMR
    XMLWriter xml;
    dmr.print_dap4(xml, !d_dap4ce.empty());

    // now make the chunked output stream; set the size to be at least chunk_size
    // but make sure that the whole of the xml plus the CRLF can fit in the first
    // chunk. (+2 for the CRLF bytes).
    chunked_ostream cos(out, max((unsigned int) CHUNK_SIZE, xml.get_doc_size() + 2));

    conditional_timeout_cancel();

    // using flush means that the DMR and CRLF are in the first chunk.
    cos << xml.get_doc() << CRLF << flush;

    // Write the data, chunked with checksums
    D4StreamMarshaller m(cos);
    dmr.root()->serialize(m, dmr, !d_dap4ce.empty());
#ifdef CLEAR_LOCAL_DATA
    dmr.root()->clear_local_data();
#endif
    cos << flush;

    BESDEBUG("dap", "BESDapResponseBuilder::serialize_dap4_data() - END" << endl);
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
    if (get_store_result().length() != 0) {
        string serviceUrl = get_store_result();

        D4AsyncUtil d4au;
        XMLWriter xmlWrtr;

        // FIXME See above comment for store dap2 result
        bool found;
        string *stylesheet_ref = 0, ss_ref_value;
        TheBESKeys::TheKeys()->get_value(D4AsyncUtil::STYLESHEET_REFERENCE_KEY, ss_ref_value, found);
        if (found && ss_ref_value.length() > 0) {
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

            BESDEBUG("dap", "[WARNING] " << msg << endl);
            d4au.writeD4AsyncResponseRejected(xmlWrtr, UNAVAILABLE, msg, stylesheet_ref);
            out << xmlWrtr.get_doc();
            out << flush;
            BESDEBUG("dap", "BESDapResponseBuilder::store_dap4_result() - Sent AsyncRequestRejected" << endl);

            return true;
        }

        if (get_async_accepted().length() != 0) {

            /**
             * Client accepts async responses so, woot! lets store this thing and tell them where to find it.
             */
            BESDEBUG("dap", "BESDapResponseBuilder::store_dap4_result() - serviceUrl="<< serviceUrl << endl);

            string storedResultId = "";
            storedResultId = resultCache->store_dap4_result(dmr, get_ce(), this);

            BESDEBUG("dap",
                "BESDapResponseBuilder::store_dap4_result() - storedResultId='"<< storedResultId << "'" << endl);

            string targetURL = BESUtil::assemblePath(serviceUrl, storedResultId);
            BESDEBUG("dap", "BESDapResponseBuilder::store_dap4_result() - targetURL='"<< targetURL << "'" << endl);

            d4au.writeD4AsyncAccepted(xmlWrtr, 0, 0, targetURL, stylesheet_ref);
            out << xmlWrtr.get_doc();
            out << flush;
            BESDEBUG("dap", "BESDapResponseBuilder::store_dap4_result() - sent AsyncAccepted" << endl);

        }
        else {
            /**
             * Client didn't indicate a willingness to accept an async response
             * So - we tell them that async is required.
             */
            d4au.writeD4AsyncRequired(xmlWrtr, 0, 0, stylesheet_ref);
            out << xmlWrtr.get_doc();
            out << flush;
            BESDEBUG("dap", "BESDapResponseBuilder::store_dap4_result() - sent AsyncAccepted" << endl);
        }

        return true;
    }

    return false;
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
libdap::DMR *
BESDapResponseBuilder::intern_dap4_data(BESResponseObject *obj, BESDataHandlerInterface &dhi)
{
    BESDEBUG("dap", "BESDapResponseBuilder::intern_dap4_data() - BEGIN"<< endl);

    dhi.first_container();

#if 0
    BESDataDDSResponse *bdds = dynamic_cast<BESDataDDSResponse *>(obj);
    if (!bdds) throw BESInternalFatalError("Expected a BESDataDDSResponse instance", __FILE__, __LINE__);

    DDS *dds = bdds->get_dds();

    set_dataset_name(dds->filename());
    set_ce(dhi.data[POST_CONSTRAINT]);
    set_async_accepted(dhi.data[ASYNC]);
    set_store_result(dhi.data[STORE_RESULT]);


    ConstraintEvaluator &eval = bdds->get_ce();
#endif
    BESDMRResponse *bdmr = dynamic_cast<BESDMRResponse *>(obj);
    if (!bdmr) throw BESInternalFatalError("Expected a BESDMRResponse instance", __FILE__, __LINE__);

    DMR *dmr = bdmr->get_dmr();
    
    BESDEBUG("dap", "BESDapResponseBuilder::dmr filename - END"<< dmr->filename() <<endl);

    set_dataset_name(dmr->filename());
    set_dap4ce(dhi.data[POST_CONSTRAINT]);
    set_async_accepted(dhi.data[ASYNC]);
    set_store_result(dhi.data[STORE_RESULT]);

    // Handle constraint later.
#if 0
    ConstraintEvaluator &eval = bdmr->get_ce();


    // Split constraint into two halves; stores the function and non-function parts in this instance.
    split_ce(eval);

    // If a function was passed in with this request, evaluate it and use that DMR
    // for the remainder of this request.
    // TODO Add caching for these function invocations
    if (!d_dap4function.empty()) {
        D4BaseTypeFactory d4_factory;
        DMR function_result(&d4_factory, "function_results");

        // Function modules load their functions onto this list. The list is
        // part of libdap, not the BES.
        if (!ServerFunctionsList::TheList())
            throw Error(
                "The function expression could not be evaluated because there are no server functions defined on this server");

        D4FunctionEvaluator parser(&dmr, ServerFunctionsList::TheList());
        bool parse_ok = parser.parse(d_dap4function);
        if (!parse_ok) throw Error("Function Expression (" + d_dap4function + ") failed to parse.");

        parser.eval(&function_result);

        // Now use the results of running the functions for the remainder of the
        // send_data operation.
        //send_dap4_data_using_ce(out, function_result, with_mime_headers);
    }
    else {
        //send_dap4_data_using_ce(out, dmr, with_mime_headers);
    }
#endif
#if 0
    // If there are functions, parse them and eval.
    // Use that DDS and parse the non-function ce
    // Serialize using the second ce and the second dds
    if (!get_btp_func_ce().empty()) {
        BESDEBUG("dap",
            "BESDapResponseBuilder::intern_dap2_data() - Found function(s) in CE: " << get_btp_func_ce() << endl);

        BESDapFunctionResponseCache *responseCache = BESDapFunctionResponseCache::get_instance();

        ConstraintEvaluator func_eval;
        DDS *fdds = 0; // nulll_ptr
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
        // of the variables in the intermediate DDS (i.e., the function
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

    throw_if_dap2_response_too_big(dds);

#endif

#if 0
    // Iterate through the variables in the DataDDS and read
    // in the data if the variable has the send flag set.
    for (DDS::Vars_iter i = dds->var_begin(), e = dds->var_end(); i != e; ++i) {
        if ((*i)->send_p()) {
            (*i)->intern_data(eval, *dds);
        }
    }
#endif

    // Iterate through the variables in the DataDDS and read
    // in the data if the variable has the send flag set.
    D4Group* root_grp = dmr->root();
    root_grp->set_send_p(true);
    //Constructor::Vars_iter v = root_grp->var_begin();
    for (D4Group::Vars_iter i = root_grp->var_begin(), e = root_grp->var_end(); i != e; ++i) {
        BESDEBUG("dap", "BESDapResponseBuilder::intern_dap4_data() - "<< (*i)->name() <<endl);
        if ((*i)->send_p()) {
            BESDEBUG("dap", "BESDapResponseBuilder::intern_dap4_data() Obtain data- "<< (*i)->name() <<endl);

            (*i)->intern_data();
        }
    }

    BESDEBUG("dap", "BESDapResponseBuilder::intern_dap4_data() - END"<< endl);

    return dmr;
}

