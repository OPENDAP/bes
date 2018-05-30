// BESDapError.cc

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2004-2009 University Corporation for Atmospheric Research
// Author: Patrick West <pwest@ucar.edu> and Jose Garcia <jgarcia@ucar.edu>
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
// You can contact University Corporation for Atmospheric Research at
// 3080 Center Green Drive, Boulder, CO 80301

// (c) COPYRIGHT University Corporation for Atmospheric Research 2004-2005
// Please read the full copyright statement in the file COPYRIGHT_UCAR.
//
// Authors:
//      pwest       Patrick West <pwest@ucar.edu>
//      jgarcia     Jose Garcia <jgarcia@ucar.edu>

#include <sstream>
#include <iostream>

using std::ostringstream;

#include "BESLog.h"

#include "BESDapError.h"
#include "BESContextManager.h"
#include "BESDapErrorInfo.h"
#include "BESInfoList.h"
#include "TheBESKeys.h"

#define DEFAULT_ADMINISTRATOR "support@opendap.org"

BESDapError *BESDapError::_instance = 0;


/** @brief converts the libdap error code to the bes error type
 *
 * This functions converts the libdap error codes in Error to the proper BES
 * error type.
 *
 *    undefined_error   1000 -> BES_INTERNAL_ERROR
 *    unknown_error     1001 -> BES_INTERNAL_ERROR
 *    internal_error    1002 -> BES_INTERNAL_FATAL_ERROR
 *    no_such_file      1003 -> BES_NOT_FOUND_ERROR
 *    no_such_variable  1004 -> BES_SYNTAX_USER_ERROR
 *    malformed_expr    1005 -> BES_SYNTAX_USER_ERROR
 *    no_authorization  1006 -> BES_FORBIDDEN_ERROR
 *    cannot_read_file  1007 -> BES_FORBIDDEN_ERROR
 *    dummy_message     1008 -> BES_FORBIDDEN_ERROR
 *
 * If the error type is already set to BES_INTERNAL_FATAL_ERROR it is not
 * changed.
 *
 * @param error_code The libdap error code to convert
 * @param current_error_type The current error type of the exception
 * @returns BES error type used in any error response
 */
int BESDapError::convert_error_code(int error_code, int current_error_type)
{
	if (current_error_type == BES_INTERNAL_FATAL_ERROR) return current_error_type;
	switch (error_code) {
	case undefined_error:
	case unknown_error: {
		return BES_INTERNAL_ERROR;
		break;
	}
	case internal_error: {
		return BES_INTERNAL_FATAL_ERROR;
		break;
	}
	case no_such_file: {
		return BES_NOT_FOUND_ERROR;
		break;
	}
	case no_such_variable:
	case malformed_expr: {
		return BES_SYNTAX_USER_ERROR;
		break;
	}
	case no_authorization:
	case cannot_read_file:
	case dummy_message: {
		return BES_FORBIDDEN_ERROR;
		break;
	}
	default: {
		return BES_INTERNAL_ERROR;
		break;
	}
	}
	return BES_INTERNAL_ERROR;
}

void log_error(BESError &e)
{
    string error_name = "";
    // TODO This should be configurable; I'm changing the values below to always log all errors.
    // I'm also confused about the actual intention. jhrg 11/14/17
    bool only_log_to_verbose = false;
    switch (e.get_error_type()) {
    case BES_INTERNAL_FATAL_ERROR:
        error_name = "BES Internal Fatal Error";
        break;

    case BES_INTERNAL_ERROR:
        error_name = "BES Internal Error";
        break;

    case BES_SYNTAX_USER_ERROR:
        error_name = "BES User Syntax Error";
        only_log_to_verbose = false; // TODO Was 'true.' jhrg 11/14/17
        break;

    case BES_FORBIDDEN_ERROR:
        error_name = "BES Forbidden Error";
        break;

    case BES_NOT_FOUND_ERROR:
        error_name = "BES Not Found Error";
        only_log_to_verbose = false; // TODO was 'true.' jhrg 11/14/17
        break;

    default:
        error_name = "Unrecognized BES Error";
        break;
    }

    if (only_log_to_verbose) {
            VERBOSE("ERROR: " << error_name << ", type: " << e.get_error_type() << ", file: " << e.get_file() << ":"
                    << e.get_line()  << ", message: " << e.get_message() << endl);

    }
	else {
		LOG("ERROR: " << error_name << ": " << e.get_message() << endl);
	}
}


/** @brief Register an exception handler with the manager

 Signature of the function is as follows:

 int function_name( BESError &e, BESDataHandlerInterface &dhi ) ;

 If the handler does not handle the exception then it should return
 0. Otherwise, return a status code. Pre-defined status
 codes can be found in BESError.h

 @param ehm exception handler function
 @see BESError
 */
void BESDapError::add_ehm_callback(ptr_bes_ehm ehm)
{
    _ehm_list.push_back(ehm);
}

int BESDapError::handleBESError(BESError &e, BESDataHandlerInterface &dhi)
{
	// Let's see if any of these exception callbacks can handle the
	// exception. The first callback that can handle the exception wins
	for (ehm_iter i = _ehm_list.begin(), ei = _ehm_list.end(); i != ei; ++i) {
		ptr_bes_ehm p = *i;
		int handled = p(e, dhi);
		if (handled) {
			return handled;
		}
	}

	dhi.error_info = BESInfoList::TheList()->build_info();
	string action_name = dhi.action_name;
	if (action_name.empty()) action_name = "BES";
	dhi.error_info->begin_response(action_name, dhi);

	string administrator = "";
	try {
		bool found = false;
		vector<string> vals;
		string key = "BES.ServerAdministrator";
		TheBESKeys::TheKeys()->get_value(key, administrator, found);
	}
	catch (...) {
		administrator = DEFAULT_ADMINISTRATOR;
	}
	if (administrator.empty()) {
		administrator = DEFAULT_ADMINISTRATOR;
	}
	dhi.error_info->add_exception(e, administrator);
	dhi.error_info->end_response();

	// Write a message in the log file about this error...
	log_error(e);

	return e.get_error_type();
}


/** @brief handles exceptions if the error context is set to dap2
 *
 * If the error context from the BESContextManager is set to dap2 then
 * handle all exceptions by returning transmitting them as dap2 error
 * messages.
 *
 * @param e exception to be handled
 * @param dhi structure that holds request and response information
 */
int BESDapError::handleException(BESError &e, BESDataHandlerInterface &dhi)
{
	// If we are handling errors in a dap2 context, then create a
	// DapErrorInfo object to transmit/print the error as a dap2
	// response.
	bool found = false;
	// I changed 'dap_format' to 'errors' in the following line. jhrg 10/6/08
	string context = BESContextManager::TheManager()->get_context("errors", found);
	if (context == "dap2" | context == "dap") {
		ErrorCode ec = unknown_error;
		BESDapError *de = dynamic_cast<BESDapError*>(&e);
		if (de) {
			ec = de->get_error_code();
		}
		e.set_error_type(convert_error_code(ec, e.get_error_type()));
		dhi.error_info = new BESDapErrorInfo(ec, e.get_message());
#if 0
//TODO Either remove this or find a way to keep it and remove the one from handleBESError() kln 05/25/18
		dhi.error_info = BESInfoList::TheList()->build_info();
		string action_name = dhi.action_name;
		if (action_name.empty()) action_name = "BES";
		dhi.error_info->begin_response(action_name, dhi);

		string administrator = "";
		try {
			bool found = false;
			vector<string> vals;
			string key = "BES.ServerAdministrator";
			TheBESKeys::TheKeys()->get_value(key, administrator, found);
		}
		catch (...) {
			administrator = DEFAULT_ADMINISTRATOR;
		}
		if (administrator.empty()) {
			administrator = DEFAULT_ADMINISTRATOR;
		}
		dhi.error_info->add_exception(e, administrator);
		dhi.error_info->end_response();
#endif

		return e.get_error_type();
	}
	else {
		// If we are not in a dap2 context and the exception is a dap
		// handler exception, then convert the error message to include the
		// error code. If it is or is not a dap exception, we simply return
		// that the exception was not handled.
		BESError *e_p = &e;
		BESDapError *de = dynamic_cast<BESDapError*>(e_p);
		if (de) {
			ostringstream s;
			s << "libdap exception building response: error_code = " << de->get_error_code() << ": "
					<< de->get_message();
			e.set_message(s.str());
			e.set_error_type(convert_error_code(de->get_error_code(), e.get_error_type()));
		}
	}
	return 0;
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance and the stored error code
 *
 * @param strm C++ i/o stream to dump the information to
 */
void BESDapError::dump(ostream &strm) const
{
	strm << BESIndent::LMarg << "BESDapError::dump - (" << (void *) this << ")" << endl;
	BESIndent::Indent();
	strm << BESIndent::LMarg << "error code = " << get_error_code() << endl;
	BESError::dump(strm);
	BESIndent::UnIndent();
}

BESDapError *
BESDapError::TheDapHandler()
{
    if (_instance == 0) {
        _instance = new BESDapError();
    }
    return _instance;
}

