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
#include <uuid/uuid.h>  // used to build CID header value for data ddx

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

#include <DAS.h>
#include <DDS.h>
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

// #include <debug.h>
#include <mime_util.h>	// for last_modified_time() and rfc_822_date()
#include <escaping.h>
#include <util.h>
#ifndef WIN32
#include <SignalHandler.h>
#include <EventHandler.h>
#include <AlarmHandler.h>
#endif

#include "TheBESKeys.h"
#include "BESDapResponseBuilder.h"
#include "BESContextManager.h"
#include "BESDapResponseCache.h"
#include "BESStoredDapResultCache.h"
#include "BESDebug.h"

#define DAP_PROTOCOL_VERSION "3.2"

const std::string CRLF = "\r\n";             // Change here, expr-test.cc

using namespace std;
using namespace libdap;

/** Called when initializing a ResponseBuilder that's not going to be passed
 command line arguments. */
void BESDapResponseBuilder::initialize()
{
    // Set default values. Don't use the C++ constructor initialization so
    // that a subclass can have more control over this process.
    d_dataset = "";
    d_dap2ce = "";
    d_btp_func_ce = "";
    d_timeout = 0;

    d_default_protocol = DAP_PROTOCOL_VERSION;

    d_response_cache = 0;

    d_dap4ce = "";
    d_dap4function = "";
    d_store_result = "";
    d_async_accepted = "";
}

/** Lazy getter for the ResponseCache. */
BESDapResponseCache *
BESDapResponseBuilder::responseCache()
{
	if (!d_response_cache)
		d_response_cache =  BESDapResponseCache::get_instance();

	return d_response_cache;
}

BESDapResponseBuilder::~BESDapResponseBuilder()
{
	if (d_response_cache) delete d_response_cache;

	// If an alarm was registered, delete it. The register code in SignalHandler
	// always deletes the old alarm handler object, so only the one returned by
	// remove_handler needs to be deleted at this point.
	delete dynamic_cast<AlarmHandler*>(SignalHandler::instance()->remove_handler(SIGALRM));
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

/** Set the DAP4 Server Side Fucntion expression. This will filter the
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

/** Use values of this instance to establish a timeout alarm for the server.
 If the timeout value is zero, do nothing.
*/
void BESDapResponseBuilder::establish_timeout(ostream &stream) const
{
#ifndef WIN32
    if (d_timeout > 0) {
        SignalHandler *sh = SignalHandler::instance();
        EventHandler *old_eh = sh->register_handler(SIGALRM, new AlarmHandler(stream));
        delete old_eh;
        alarm(d_timeout);
    }
#endif
}

void BESDapResponseBuilder::remove_timeout() const
{
	alarm(0);
}

static string::size_type
find_closing_paren(const string &ce, string::size_type pos)
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
void
BESDapResponseBuilder::split_ce(ConstraintEvaluator &eval, const string &expr)
{
	BESDEBUG("dap", "Entering ResponseBuilder::split_ce" << endl);
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
    if (first_paren != string::npos)
    	closing_paren = find_closing_paren(ce, first_paren); //ce.find(")", pos);


    while (first_paren != string::npos && closing_paren != string::npos) {
        // Maybe a BTP function; get the name of the potential function
        string name = ce.substr(pos, first_paren-pos);

        // is this a BTP function
        btp_func f;
        if (eval.find_function(name, &f)) {
            // Found a BTP function
            if (!btp_function_ce.empty())
                btp_function_ce += ",";
            btp_function_ce += ce.substr(pos, closing_paren+1-pos);
            ce.erase(pos, closing_paren+1-pos);
            if (ce[pos] == ',')
                ce.erase(pos, 1);
        }
        else {
            pos = closing_paren + 1;
            // exception?
            if (pos < ce.length() && ce.at(pos) == ',')
                ++pos;
        }

        first_paren = ce.find("(", pos);
        closing_paren = ce.find(")", pos);
    }

    d_dap2ce = ce;
    d_btp_func_ce = btp_function_ce;

    BESDEBUG("dap", "Modified constraint: " << d_dap2ce << endl);
    BESDEBUG("dap", "BTP Function part: " << btp_function_ce << endl);
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
    if (with_mime_headers)
        set_mime_text(out, dods_das, x_plain, last_modified_time(d_dataset), "2.0");

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
void BESDapResponseBuilder::send_das(ostream &out, DDS &dds, ConstraintEvaluator &eval, bool constrained, bool with_mime_headers)
{
    // Set up the alarm.
    establish_timeout(out);
    dds.set_timeout(d_timeout);

    if (!constrained) {
        if (with_mime_headers)
            set_mime_text(out, dods_das, x_plain, last_modified_time(d_dataset), "2.0");

        dds.print_das(out);
        out << flush;

        return;
    }

    split_ce(eval);

    // If there are functions, parse them and eval.
    // Use that DDS and parse the non-function ce
    // Serialize using the second ce and the second dds
    if (!d_btp_func_ce.empty()) {
        DDS *fdds = 0;
        string cache_token = "";
        ConstraintEvaluator func_eval;

        if (responseCache()) {
            fdds = responseCache()->cache_dataset(dds, d_btp_func_ce, this, &func_eval, cache_token);
        }
        else {
        	func_eval.parse_constraint(d_btp_func_ce, dds);
            fdds = func_eval.eval_function_clauses(dds);
        }

        if (with_mime_headers)
            set_mime_text(out, dods_das, x_plain, last_modified_time(d_dataset), dds.get_dap_version());

        fdds->print_das(out);

        if (responseCache())
        	responseCache()->unlock_and_close(cache_token);

        delete fdds;
    }
    else {
        eval.parse_constraint(d_dap2ce, dds); // Throws Error if the ce doesn't parse.

        if (with_mime_headers)
            set_mime_text(out, dods_das, x_plain, last_modified_time(d_dataset), dds.get_dap_version());

        dds.print_das(out);
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
void BESDapResponseBuilder::send_dds(ostream &out, DDS &dds, ConstraintEvaluator &eval, bool constrained,
        bool with_mime_headers)
{
    if (!constrained) {
        if (with_mime_headers)
            set_mime_text(out, dods_dds, x_plain, last_modified_time(d_dataset), dds.get_dap_version());

        dds.print(out);
        out << flush;
        return;
    }

    // Set up the alarm.
    establish_timeout(out);
    dds.set_timeout(d_timeout);

    // Split constraint into two halves
    split_ce(eval);

    // If there are functions, parse them and eval.
    // Use that DDS and parse the non-function ce
    // Serialize using the second ce and the second dds
    if (!d_btp_func_ce.empty()) {
        string cache_token = "";
        DDS *fdds = 0;
        ConstraintEvaluator func_eval;

        if (responseCache()) {
            fdds = responseCache()->cache_dataset(dds, d_btp_func_ce, this, &func_eval, cache_token);
        }
        else {
            func_eval.parse_constraint(d_btp_func_ce, dds);
            fdds = func_eval.eval_function_clauses(dds);
        }

        // Server functions might mark variables to use their read()
        // methods. Clear that so the CE in d_dap2ce will control what is
        // sent. If that is empty (there was only a function call) all
        // of the variables in the intermediate DDS (i.e., the function
        // result) will be sent.
        fdds->mark_all(false);

        eval.parse_constraint(d_dap2ce, *fdds);

        if (with_mime_headers)
            set_mime_text(out, dods_dds, x_plain, last_modified_time(d_dataset), dds.get_dap_version());

        fdds->print_constrained(out);

        if (responseCache())
        	responseCache()->unlock_and_close(cache_token);

        delete fdds;
    }
    else {
        eval.parse_constraint(d_dap2ce, dds); // Throws Error if the ce doesn't parse.

        if (with_mime_headers)
            set_mime_text(out, dods_dds, x_plain, last_modified_time(d_dataset), dds.get_dap_version());

        dds.print_constrained(out);
    }

    out << flush;
}

/**
 * Should this result be returned using the asynchronous response mechanism?
 * Look at the 'store_result' property and see if the code should return this
 * using the asynchronous mechanism. If yes, it will try and return the correct
 * response or an error if the serveris not configured or the client has not
 * included the correct information in the request.
 *
 * @param out Write information to the client using this stream
 * @param dds The DDS that hold information used to build the response. The
 * response won't be built until some time in the future
 * @param eval Use this to evaluate the CE associated with the response.
 * @return True if the response should/will be returned asynchronously, false
 * otherwise.
 */
bool BESDapResponseBuilder::store_dap2_result(ostream &out, DDS &dds, ConstraintEvaluator &eval) {

	if(get_store_result().length()!=0){
		string serviceUrl = get_store_result();

		XMLWriter xmlWrtr;
		D4AsyncUtil d4au;

		bool found;
		string *stylesheet_ref=0, ss_ref_value;
	    TheBESKeys::TheKeys()->get_value( D4AsyncUtil::STYLESHEET_REFERENCE_KEY, ss_ref_value, found ) ;
	    if( found && ss_ref_value.length()>0) {
	    	stylesheet_ref = &ss_ref_value;
	    }

		BESStoredDapResultCache *resultCache = BESStoredDapResultCache::get_instance();
		if(resultCache == NULL){

			/**
			 * OOPS. Looks like the BES is not configured to use a Stored Result Cache.
			 * Looks like need to reject the request and move on.
			 *
			 */
			string msg =  "The Stored Result request cannot be serviced. ";
			msg += "Unable to acquire StoredResultCache instance. ";
			msg += "This is most likely because the StoredResultCache is not (correctly) configured.";

			BESDEBUG("dap", "[WARNING] " << msg << endl);
			d4au.writeD4AsyncResponseRejected(xmlWrtr, UNAVAILABLE, msg, stylesheet_ref);
			out << xmlWrtr.get_doc();
			out << flush;
			BESDEBUG("dap", "BESDapResponseBuilder::store_dap2_result() - Sent AsyncRequestRejected" << endl);
			return true;
		}


		if(get_async_accepted().length() != 0){

			/**
			 * Client accepts async responses so, woot! lets store this thing and tell them where to find it.
			 */
			BESDEBUG("dap", "BESDapResponseBuilder::store_dap2_result() - serviceUrl="<< serviceUrl << endl);

			BESStoredDapResultCache *resultCache = BESStoredDapResultCache::get_instance();
			string storedResultId="";
			storedResultId = resultCache->store_dap2_result(dds, get_ce(), this, &eval);

			BESDEBUG("dap", "BESDapResponseBuilder::store_dap2_result() - storedResultId='"<< storedResultId << "'" << endl);

			string targetURL = resultCache->assemblePath(serviceUrl,storedResultId);
			BESDEBUG("dap", "BESDapResponseBuilder::store_dap2_result() - targetURL='"<< targetURL << "'" << endl);

			XMLWriter xmlWrtr;
			d4au.writeD4AsyncAccepted(xmlWrtr, 0, 0, targetURL,stylesheet_ref);
			out << xmlWrtr.get_doc();
			out << flush;
			BESDEBUG("dap", "BESDapResponseBuilder::store_dap2_result() - sent DAP4 AsyncAccepted response" << endl);

		}
		else {
			/**
			 * Client didn't indicate a willingness to accept an async response
			 * So - we tell them that async is required.
			 */
			d4au.writeD4AsyncRequired(xmlWrtr, 0, 0,stylesheet_ref);
			out << xmlWrtr.get_doc();
			out << flush;
			BESDEBUG("dap", "BESDapResponseBuilder::store_dap2_result() - sent DAP4 AsyncRequired  response" << endl);
		}


		return true;

	}
	return false;
}

/**
 * Build/return the BLOB part of the DAP2 data response.
 */
void BESDapResponseBuilder::serialize_dap2_data_dds(ostream &out, DDS &dds, ConstraintEvaluator &eval, bool ce_eval)
{
	BESDEBUG("dap", "BESDapResponseBuilder::serialize_dap2_data_dds() - BEGIN" << endl);

	dds.print_constrained(out);
	out << "Data:\n";
	out << flush;

	XDRStreamMarshaller m(out);

	// Send all variables in the current projection (send_p())
	for (DDS::Vars_iter i = dds.var_begin(); i != dds.var_end(); i++){
		if ((*i)->send_p()) {
			(*i)->serialize(eval, dds, m, ce_eval);
		}
	}

	BESDEBUG("dap", "BESDapResponseBuilder::serialize_dap2_data_dds() - END" << endl);
}

/**
 * Serialize a DAP3.2 DataDDX to the stream "out".
 * This was originally intended to be used for DAP4.
 */
void BESDapResponseBuilder::serialize_dap2_data_ddx(ostream &out, DDS &dds, ConstraintEvaluator &eval,
        const string &boundary, const string &start, bool ce_eval)
{
	BESDEBUG("dap", "BESDapResponseBuilder::serialize_dap2_data_ddx() - BEGIN" << endl);

	// Write the MPM headers for the DDX (text/xml) part of the response
	libdap::set_mime_ddx_boundary(out, boundary, start, dods_ddx, x_plain);

	// Make cid
	uuid_t uu;
	uuid_generate(uu);
	char uuid[37];
	uuid_unparse(uu, &uuid[0]);
	char domain[256];
	if (getdomainname(domain, 255) != 0 || strlen(domain) == 0)
		strncpy(domain, "opendap.org", 255);

	string cid = string(&uuid[0]) + "@" + string(&domain[0]);
	// Send constrained DDX with a data blob reference
	dds.print_xml_writer(out, true, cid);

	// write the data part mime headers here
	set_mime_data_boundary(out, boundary, cid, dods_data_ddx /* old value dap4_data*/, x_plain);

	XDRStreamMarshaller m(out);

	// Send all variables in the current projection (send_p()). In DAP4,
	// all of the top-level variables are serialized with their checksums.
	// Internal variables are not.
	for (DDS::Vars_iter i = dds.var_begin(); i != dds.var_end(); i++) {
		if ((*i)->send_p()) {
			(*i)->serialize(eval, dds, m, ce_eval);
		}
	}

	BESDEBUG("dap", "BESDapResponseBuilder::serialize_dap2_data_ddx() - END" << endl);
}

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
void BESDapResponseBuilder::send_dap2_data(ostream &data_stream, DDS &dds, ConstraintEvaluator &eval, bool with_mime_headers)
{
	BESDEBUG("dap", "BESDapResponseBuilder::send_dap2_data() - BEGIN"<< endl);

	// Set up the alarm.
    establish_timeout(data_stream);
    dds.set_timeout(d_timeout);

    // Split constraint into two halves
    split_ce(eval);

    // If there are functions, parse them and eval.
    // Use that DDS and parse the non-function ce
    // Serialize using the second ce and the second dds
    if (!d_btp_func_ce.empty()) {
        BESDEBUG("dap", "BESDapResponseBuilder::send_dap2_data() - Found function(s) in CE: " << d_btp_func_ce << endl);
        string cache_token = "";
        DDS *fdds = 0;
        // Define a local ce evaluator so that the clause from the function parse
        // won't get treated like selection clauses later on when serialize is called
        // on the DDS (fdds)
        ConstraintEvaluator func_eval;
        if (responseCache()) {
        	BESDEBUG("dap", "BESDapResponseBuilder::send_dap2_data() - Using the cache for the server function CE" << endl);
            fdds = responseCache()->cache_dataset(dds, d_btp_func_ce, this, &func_eval, cache_token);
        }
        else {
        	BESDEBUG("dap", "BESDapResponseBuilder::send_dap2_data() - Cache not found; (re)calculating" << endl);
        	func_eval.parse_constraint(d_btp_func_ce, dds);
            fdds = func_eval.eval_function_clauses(dds);
        }
        BESDEBUG("dap", "constrained DDS: " << endl; fdds->print_constrained(cerr));

        // Server functions might mark variables to use their read()
        // methods. Clear that so the CE in d_dap2ce will control what is
        // sent. If that is empty (there was only a function call) all
        // of the variables in the intermediate DDS (i.e., the function
        // result) will be sent.
        fdds->mark_all(false);

        eval.parse_constraint(d_dap2ce, *fdds);

        fdds->tag_nested_sequences(); // Tag Sequences as Parent or Leaf node.

        if (fdds->get_response_limit() != 0 && fdds->get_request_size(true) > fdds->get_response_limit()) {
            string msg = "The Request for " + long_to_string(dds.get_request_size(true) / 1024)
                    + "KB is too large; requests for this user are limited to "
                    + long_to_string(dds.get_response_limit() / 1024) + "KB.";
            throw Error(msg);
        }

        if (with_mime_headers)
            set_mime_binary(data_stream, dods_data, x_plain, last_modified_time(d_dataset), dds.get_dap_version());

        BESDEBUG("dap", cerr << "BESDapResponseBuilder::send_dap2_data() - About to call dataset_constraint" << endl);
    	if(!store_dap2_result(data_stream,dds,eval)){
			serialize_dap2_data_dds(data_stream, *fdds, eval, false);
    	}

        if (responseCache())
        	responseCache()->unlock_and_close(cache_token);

        delete fdds;
    }
    else {
    	BESDEBUG("dap", "BESDapResponseBuilder::send_dap2_data() - Simple constraint" << endl);

        eval.parse_constraint(d_dap2ce, dds); // Throws Error if the ce doesn't parse.

        dds.tag_nested_sequences(); // Tag Sequences as Parent or Leaf node.

        if (dds.get_response_limit() != 0 && dds.get_request_size(true) > dds.get_response_limit()) {
            string msg = "The Request for " + long_to_string(dds.get_request_size(true) / 1024)
                    + "KB is too large; requests for this user are limited to "
                    + long_to_string(dds.get_response_limit() / 1024) + "KB.";
            throw Error(msg);
        }

        if (with_mime_headers)
            set_mime_binary(data_stream, dods_data, x_plain, last_modified_time(d_dataset), dds.get_dap_version());

		if (!store_dap2_result(data_stream, dds, eval)) {
			serialize_dap2_data_dds(data_stream, dds, eval);
		}
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
void BESDapResponseBuilder::send_ddx(ostream &out, DDS &dds, ConstraintEvaluator &eval, bool with_mime_headers)
{
    if (d_dap2ce.empty()) {
        if (with_mime_headers)
            set_mime_text(out, dods_ddx, x_plain, last_modified_time(d_dataset), dds.get_dap_version());

        dds.print_xml_writer(out, false /*constrained */, "");
        //dds.print(out);
        out << flush;
        return;
    }

    // Set up the alarm.
    establish_timeout(out);
    dds.set_timeout(d_timeout);

    // Split constraint into two halves
    split_ce(eval);

    // If there are functions, parse them and eval.
    // Use that DDS and parse the non-function ce
    // Serialize using the second ce and the second dds
    if (!d_btp_func_ce.empty()) {
        string cache_token = "";
        DDS *fdds = 0;
        ConstraintEvaluator func_eval;

        if (responseCache()) {
            fdds = responseCache()->cache_dataset(dds, d_btp_func_ce, this, &func_eval, cache_token);
        }
        else {
            func_eval.parse_constraint(d_btp_func_ce, dds);
            fdds = func_eval.eval_function_clauses(dds);
        }

        // Server functions might mark variables to use their read()
        // methods. Clear that so the CE in d_dap2ce will control what is
        // sent. If that is empty (there was only a function call) all
        // of the variables in the intermediate DDS (i.e., the function
        // result) will be sent.
        fdds->mark_all(false);

        eval.parse_constraint(d_dap2ce, *fdds);

        if (with_mime_headers)
            set_mime_text(out, dods_ddx, x_plain, last_modified_time(d_dataset), dds.get_dap_version());

        fdds->print_constrained(out);

        if (responseCache())
        	responseCache()->unlock_and_close(cache_token);

        delete fdds;
    }
    else {
        eval.parse_constraint(d_dap2ce, dds); // Throws Error if the ce doesn't parse.

        if (with_mime_headers)
            set_mime_text(out, dods_ddx, x_plain, last_modified_time(d_dataset), dds.get_dap_version());

        // dds.print_constrained(out);
        dds.print_xml_writer(out, true, "");
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
        if (!parse_ok)
            throw Error("Constraint Expression failed to parse.");
    }
    // with an empty CE, send everything. Even though print_dap4() and serialize()
    // don't need this, other code may depend on send_p being set. This may change
    // if DAP4 has a separate function evaluation phase. jhrg 11/25/13
    else {
        dmr.root()->set_send_p(true);
    }

    if (with_mime_headers) set_mime_text(out, dap4_dmr, x_plain, last_modified_time(d_dataset), dmr.dap_version());

    XMLWriter xml;
    dmr.print_dap4(xml, /*constrained &&*/ !d_dap4ce.empty() /* true == constrained */);
    out << xml.get_doc() << flush;

    out << flush;
}

void BESDapResponseBuilder::send_dap4_data_using_ce(ostream &out, DMR &dmr, bool with_mime_headers)
{
    if (!d_dap4ce.empty()) {
    	D4ConstraintEvaluator parser(&dmr);
        bool parse_ok = parser.parse(d_dap4ce);
        if (!parse_ok)
            throw Error("Constraint Expression (" + d_dap4ce + ") failed to parse.");
    }
    // with an empty CE, send everything. Even though print_dap4() and serialize()
    // don't need this, other code may depend on send_p being set. This may change
    // if DAP4 has a separate function evaluation phase. jhrg 11/25/13
    else {
        dmr.root()->set_send_p(true);
    }

    if (dmr.response_limit() != 0 && dmr.request_size(true) > dmr.response_limit()) {
        string msg = "The Request for " + long_to_string(dmr.request_size(true) / 1024)
                + "MB is too large; requests for this user are limited to "
                + long_to_string(dmr.response_limit() / 1024) + "MB.";
        throw Error(msg);
    }

	if (!store_dap4_result(out, dmr)) {
        serialize_dap4_data(out, dmr, with_mime_headers);
	}
}

void BESDapResponseBuilder::send_dap4_data(ostream &out, DMR &dmr, bool with_mime_headers)
{
	try {
        // Set up the alarm.
        establish_timeout(out);

        // If a function was passed in with this request, evaluate it and use that DMR
        // for the remainder of this request.
        // TODO Add caching for these function invocations
        if (!d_dap4function.empty()) {
            D4BaseTypeFactory d4_factory;
            DMR function_result(&d4_factory, "function_results");

            // Function modules load their functions onto this list. The list is
            // part of libdap, not the BES.
            if (!ServerFunctionsList::TheList())
            	throw Error("The function expression could not be evaluated because there are no server functions defined on this server");

            D4FunctionEvaluator parser(&dmr, ServerFunctionsList::TheList());
    		bool parse_ok = parser.parse(d_dap4function);
    		if (!parse_ok)
    			throw Error("Function Expression (" + d_dap4function + ") failed to parse.");

			parser.eval(&function_result);

			// Now use the results of running the functions for the remainder of the
			// send_data operation.
			send_dap4_data_using_ce(out, function_result, with_mime_headers);
        }

    	send_dap4_data_using_ce(out, dmr, with_mime_headers);

    	remove_timeout();
    }
    catch (...) {
        remove_timeout();
        throw;
    }
}

/**
 * Serialize the DAP4 data response to the passed stream
 */
void BESDapResponseBuilder::serialize_dap4_data(std::ostream &out, libdap::DMR &dmr, bool with_mime_headers)
{
	BESDEBUG("dap", "BESDapResponseBuilder::serialize_dap4_data() - BEGIN" << endl);

    if (with_mime_headers)
        set_mime_binary(out, dap4_data, x_plain, last_modified_time(d_dataset), dmr.dap_version());

    // Write the DMR
    XMLWriter xml;
    dmr.print_dap4(xml, !d_dap4ce.empty());

    // now make the chunked output stream; set the size to be at least chunk_size
    // but make sure that the whole of the xml plus the CRLF can fit in the first
    // chunk. (+2 for the CRLF bytes).
    chunked_ostream cos(out, max((unsigned int)CHUNK_SIZE, xml.get_doc_size()+2));

    // using flush means that the DMR and CRLF are in the first chunk.
    cos << xml.get_doc() << CRLF << flush;

    // Write the data, chunked with checksums
    D4StreamMarshaller m(cos);
    dmr.root()->serialize(m, dmr, !d_dap4ce.empty());

    out << flush;

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

			string targetURL = resultCache->assemblePath(serviceUrl, storedResultId);
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

