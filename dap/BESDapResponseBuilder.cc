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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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
#include <debug.h>
#include <mime_util.h>	// for last_modified_time() and rfc_822_date()
#include <escaping.h>
#include <util.h>
#ifndef WIN32
#include <SignalHandler.h>
#include <EventHandler.h>
#include <AlarmHandler.h>
#endif

#include "BESDapResponseCache.h"
#include "BESDapResponseBuilder.h"
#include "BESDebug.h"

#define CRLF "\r\n"             // Change here, expr-test.cc
#define DAP_PROTOCOL_VERSION "3.2"

using namespace std;
using namespace libdap;

/** Called when initializing a ResponseBuilder that's not going to be passed
 command line arguments. */
void BESDapResponseBuilder::initialize()
{
    // Set default values. Don't use the C++ constructor initialization so
    // that a subclass can have more control over this process.
    d_dataset = "";
    d_ce = "";
    d_btp_func_ce = "";
    d_timeout = 0;

    d_default_protocol = DAP_PROTOCOL_VERSION;

    d_response_cache = 0;
}

/** Lazy getter for the ResponseCache. */
BESDapResponseCache *
BESDapResponseBuilder::responseCache()
{
	if (!d_response_cache) d_response_cache = new BESDapResponseCache();
	return d_response_cache->is_available() ? d_response_cache: 0;
}

BESDapResponseBuilder::~BESDapResponseBuilder()
{
	if (d_response_cache) delete d_response_cache;

	// If an alarm was registered, delete it. The register code in SignalHandler
	// always deletes the old alarm handler object, so only the one returned by
	// remove_handler needs to be deleted at this point.
	delete dynamic_cast<AlarmHandler*>(SignalHandler::instance()->remove_handler(SIGALRM));
}

/** Return the entire constraint expression in a string.  This
 includes both the projection and selection clauses, but not the
 question mark.

 @brief Get the constraint expression.
 @return A string object that contains the constraint expression. */
string BESDapResponseBuilder::get_ce() const
{
    return d_ce;
}

/** Set the constraint expression. This will filter the CE text removing
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
    d_ce = www2id(_ce, "%", "%20");
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
	DBG(cerr << "Entering ResponseBuilder::split_ce" << endl);
    string ce;
    if (!expr.empty())
        ce = expr;
    else
        ce = d_ce;

    string btp_function_ce = "";
    string::size_type pos = 0;
    DBG(cerr << "ce: " << ce << endl);

    // This hack assumes that the functions are listed first. Look for the first
    // open paren and the last closing paren to accommodate nested function calls
    string::size_type first_paren = ce.find("(", pos);
    string::size_type closing_paren = string::npos;
    if (first_paren != string::npos)
    	closing_paren = find_closing_paren(ce, first_paren); //ce.find(")", pos);


    while (first_paren != string::npos && closing_paren != string::npos) {
        // Maybe a BTP function; get the name of the potential function
        string name = ce.substr(pos, first_paren-pos);
        DBG(cerr << "name: " << name << endl);
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

    DBG(cerr << "Modified constraint: " << ce << endl);
    DBG(cerr << "BTP Function part: " << btp_function_ce << endl);

    d_ce = ce;
    d_btp_func_ce = btp_function_ce;
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

        if (responseCache()) {
            DBG(cerr << "Using the cache for the server function CE" << endl);
            fdds = responseCache()->cache_dataset(dds, d_btp_func_ce, this, &eval, cache_token);
        }
        else {
            DBG(cerr << "Cache not found; (re)calculating" << endl);
            eval.parse_constraint(d_btp_func_ce, dds);
            fdds = eval.eval_function_clauses(dds);
        }

        if (with_mime_headers)
            set_mime_text(out, dods_das, x_plain, last_modified_time(d_dataset), dds.get_dap_version());

        fdds->print_das(out);

        if (responseCache())
        	responseCache()->unlock_and_close(cache_token);

        delete fdds;
    }
    else {
        DBG(cerr << "Simple constraint" << endl);

        eval.parse_constraint(d_ce, dds); // Throws Error if the ce doesn't parse.

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

        if (responseCache()) {
            DBG(cerr << "Using the cache for the server function CE" << endl);
            fdds = responseCache()->cache_dataset(dds, d_btp_func_ce, this, &eval, cache_token);
        }
        else {
            DBG(cerr << "Cache not found; (re)calculating" << endl);
            eval.parse_constraint(d_btp_func_ce, dds);
            fdds = eval.eval_function_clauses(dds);
        }

        // Server functions might mark variables to use their read()
        // methods. Clear that so the CE in d_ce will control what is
        // sent. If that is empty (there was only a function call) all
        // of the variables in the intermediate DDS (i.e., the function
        // result) will be sent.
        fdds->mark_all(false);

        eval.parse_constraint(d_ce, *fdds);

        if (with_mime_headers)
            set_mime_text(out, dods_dds, x_plain, last_modified_time(d_dataset), dds.get_dap_version());

        fdds->print_constrained(out);

        if (responseCache())
        	responseCache()->unlock_and_close(cache_token);

        delete fdds;
    }
    else {
        DBG(cerr << "Simple constraint" << endl);

        eval.parse_constraint(d_ce, dds); // Throws Error if the ce doesn't parse.

        if (with_mime_headers)
            set_mime_text(out, dods_dds, x_plain, last_modified_time(d_dataset), dds.get_dap_version());

        dds.print_constrained(out);
    }

    out << flush;
}

/**
 * Build/return the BLOB part of the DAP2 data response.
 */
void BESDapResponseBuilder::dataset_constraint(ostream &out, DDS & dds, ConstraintEvaluator & eval, bool ce_eval)
{
    // send constrained DDS
    DBG(cerr << "Inside dataset_constraint" << endl);

    dds.print_constrained(out);
    out << "Data:\n";
    out << flush;

    XDRStreamMarshaller m(out);

    try {
        // Send all variables in the current projection (send_p())
        for (DDS::Vars_iter i = dds.var_begin(); i != dds.var_end(); i++)
            if ((*i)->send_p()) {
                (*i)->serialize(eval, dds, m, ce_eval);
            }
    }
    catch (Error & e) {
        throw;
    }
}

/**
 * Build/return the DDX and the BLOB part of the DAP3.x data response.
 * This was never actually used by any server or client, but it has
 * been used to cache responses for some of the OPULS unstructured
 * grid work. It was originally intended to be used for DAP4.
 */
void BESDapResponseBuilder::dataset_constraint_ddx(ostream &out, DDS &dds, ConstraintEvaluator &eval,
        const string &boundary, const string &start, bool ce_eval)
{
    // Write the MPM headers for the DDX (text/xml) part of the response
    libdap::set_mime_ddx_boundary(out, boundary, start, dap4_ddx, x_plain);

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
    set_mime_data_boundary(out, boundary, cid, dap4_data, x_plain);

    XDRStreamMarshaller m(out);

    // Send all variables in the current projection (send_p()). In DAP4,
    // all of the top-level variables are serialized with their checksums.
    // Internal variables are not.
    for (DDS::Vars_iter i = dds.var_begin(); i != dds.var_end(); i++) {
        if ((*i)->send_p()) {
            (*i)->serialize(eval, dds, m, ce_eval);
        }
    }
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
void BESDapResponseBuilder::send_data(ostream &data_stream, DDS &dds, ConstraintEvaluator &eval, bool with_mime_headers)
{
    // Set up the alarm.
    establish_timeout(data_stream);
    dds.set_timeout(d_timeout);

    // Split constraint into two halves
    split_ce(eval);

    // If there are functions, parse them and eval.
    // Use that DDS and parse the non-function ce
    // Serialize using the second ce and the second dds
    if (!d_btp_func_ce.empty()) {
        DBG(cerr << "Found function(s) in CE: " << d_btp_func_ce << endl);
        string cache_token = "";
        DDS *fdds = 0;

        if (responseCache()) {
            DBG(cerr << "Using the cache for the server function CE" << endl);
            fdds = responseCache()->cache_dataset(dds, d_btp_func_ce, this, &eval, cache_token);
        }
        else {
            DBG(cerr << "Cache not found; (re)calculating" << endl);
            eval.parse_constraint(d_btp_func_ce, dds);
            fdds = eval.eval_function_clauses(dds);
        }

        DBG(fdds->print_constrained(cerr));

        // Server functions might mark variables to use their read()
        // methods. Clear that so the CE in d_ce will control what is
        // sent. If that is empty (there was only a function call) all
        // of the variables in the intermediate DDS (i.e., the function
        // result) will be sent.
        fdds->mark_all(false);

        eval.parse_constraint(d_ce, *fdds);

        fdds->tag_nested_sequences(); // Tag Sequences as Parent or Leaf node.

        if (fdds->get_response_limit() != 0 && fdds->get_request_size(true) > fdds->get_response_limit()) {
            string msg = "The Request for " + long_to_string(dds.get_request_size(true) / 1024)
                    + "KB is too large; requests for this user are limited to "
                    + long_to_string(dds.get_response_limit() / 1024) + "KB.";
            throw Error(msg);
        }

        if (with_mime_headers)
            set_mime_binary(data_stream, dods_data, x_plain, last_modified_time(d_dataset), dds.get_dap_version());

        DBG(cerr << "About to call dataset_constraint" << endl);
        dataset_constraint(data_stream, *fdds, eval, false);

        if (responseCache())
        	responseCache()->unlock_and_close(cache_token);

        delete fdds;
    }
    else {
        DBG(cerr << "Simple constraint" << endl);

        eval.parse_constraint(d_ce, dds); // Throws Error if the ce doesn't parse.

        dds.tag_nested_sequences(); // Tag Sequences as Parent or Leaf node.

        if (dds.get_response_limit() != 0 && dds.get_request_size(true) > dds.get_response_limit()) {
            string msg = "The Request for " + long_to_string(dds.get_request_size(true) / 1024)
                    + "KB is too large; requests for this user are limited to "
                    + long_to_string(dds.get_response_limit() / 1024) + "KB.";
            throw Error(msg);
        }

        if (with_mime_headers)
            set_mime_binary(data_stream, dods_data, x_plain, last_modified_time(d_dataset), dds.get_dap_version());

        dataset_constraint(data_stream, dds, eval);
    }

    data_stream << flush;
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
    if (d_ce.empty()) {
        if (with_mime_headers)
            set_mime_text(out, dap4_ddx, x_plain, last_modified_time(d_dataset), dds.get_dap_version());

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

        if (responseCache()) {
            DBG(cerr << "Using the cache for the server function CE" << endl);
            fdds = responseCache()->cache_dataset(dds, d_btp_func_ce, this, &eval, cache_token);
        }
        else {
            DBG(cerr << "Cache not found; (re)calculating" << endl);
            eval.parse_constraint(d_btp_func_ce, dds);
            fdds = eval.eval_function_clauses(dds);
        }

        // Server functions might mark variables to use their read()
        // methods. Clear that so the CE in d_ce will control what is
        // sent. If that is empty (there was only a function call) all
        // of the variables in the intermediate DDS (i.e., the function
        // result) will be sent.
        fdds->mark_all(false);

        eval.parse_constraint(d_ce, *fdds);

        if (with_mime_headers)
            set_mime_text(out, dap4_ddx, x_plain, last_modified_time(d_dataset), dds.get_dap_version());

        fdds->print_constrained(out);

        if (responseCache())
        	responseCache()->unlock_and_close(cache_token);

        delete fdds;
    }
    else {
        DBG(cerr << "Simple constraint" << endl);

        eval.parse_constraint(d_ce, dds); // Throws Error if the ce doesn't parse.

        if (with_mime_headers)
            set_mime_text(out, dap4_ddx, x_plain, last_modified_time(d_dataset), dds.get_dap_version());

        //dds.print_constrained(out);
        dds.print_xml_writer(out, true, "");
    }

    out << flush;
}

/** Send the data in the DDS object back to the client program. The data is
 encoded using a Marshaller, and enclosed in a MIME document which is all sent
 to \c data_stream.

 @note This is not the DAP4 data response, although that was the original
 intent.

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
void BESDapResponseBuilder::send_data_ddx(ostream & data_stream, DDS & dds, ConstraintEvaluator & eval, const string &start,
        const string &boundary, bool with_mime_headers)
{
	// Set up the alarm.
    establish_timeout(data_stream);
    dds.set_timeout(d_timeout);

    eval.parse_constraint(d_ce, dds); // Throws Error if the ce doesn't parse.

    if (dds.get_response_limit() != 0 && dds.get_request_size(true) > dds.get_response_limit()) {
        string msg = "The Request for " + long_to_string(dds.get_request_size(true) / 1024)
                + "KB is too large; requests for this user are limited to "
                + long_to_string(dds.get_response_limit() / 1024) + "KB.";
        throw Error(msg);
    }

    dds.tag_nested_sequences(); // Tag Sequences as Parent or Leaf node.

    // Start sending the response...

    // Handle *functional* constraint expressions specially
    if (eval.function_clauses()) {
        // We could unique_ptr<DDS> here to avoid memory leaks if
        // dataset_constraint_ddx() throws an exception.
        DDS *fdds = eval.eval_function_clauses(dds);
        try {
            if (with_mime_headers)
                set_mime_multipart(data_stream, boundary, start, dap4_data_ddx, x_plain, last_modified_time(d_dataset));
            data_stream << flush;
            dataset_constraint_ddx(data_stream, *fdds, eval, boundary, start);
        }
        catch (...) {
            delete fdds;
            throw;
        }
        delete fdds;
    }
    else {
        if (with_mime_headers)
            set_mime_multipart(data_stream, boundary, start, dap4_data_ddx, x_plain, last_modified_time(d_dataset));
        data_stream << flush;
        dataset_constraint_ddx(data_stream, dds, eval, boundary, start);
    }

    data_stream << flush;

    if (with_mime_headers)
        data_stream << CRLF << "--" << boundary << "--" << CRLF;
}

