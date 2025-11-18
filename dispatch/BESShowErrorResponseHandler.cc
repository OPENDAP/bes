// BESShowErrorResponseHandler.cc

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

using std::endl;
using std::istringstream;
using std::ostream;
using std::string;

#include "BESDataHandlerInterface.h"
#include "BESDataNames.h"
#include "BESForbiddenError.h"
#include "BESInternalError.h"
#include "BESInternalFatalError.h"
#include "BESNotFoundError.h"
#include "BESShowErrorResponseHandler.h"
#include "BESSyntaxUserError.h"

BESShowErrorResponseHandler::BESShowErrorResponseHandler(const string &name) : BESResponseHandler(name) {}

BESShowErrorResponseHandler::~BESShowErrorResponseHandler() = default;

/** @brief throws a specific exception to test error handling in clients
 *
 * Where error_type_num is one of the following
 * 1. Internal Error - the error is internal to the BES Server
 * 2. Internal Fatal Error - error is fatal, can not continue
 * 3. Syntax User Error - the requester has a syntax error in request or config
 * 4. Forbidden Error - the requester is forbidden to see the resource
 * 5. Not Found Error - the resource can not be found
 *
 * @param dhi structure that holds request and response information
 * @see BESDataHandlerInterface
 * @see BESInfo
 * @see BESContextManager
 */
void BESShowErrorResponseHandler::execute(BESDataHandlerInterface &dhi) {
    string etype_s = dhi.data[SHOW_ERROR_TYPE];
    if (etype_s.empty()) {
        string err = dhi.action + " error type missing";
        throw BESSyntaxUserError(err, __FILE__, __LINE__);
    }
    istringstream strm(etype_s);
    unsigned int etype = 0;
    strm >> etype;
    if (!etype || etype > 5) {
        string err = dhi.action + " invalid error type, should be 1-5";
        throw BESSyntaxUserError(err, __FILE__, __LINE__);
    }

    switch (etype) {
    case BES_INTERNAL_ERROR: {
        string err = dhi.action + " Internal Error";
        throw BESInternalError(err, __FILE__, __LINE__);
    }
    case BES_INTERNAL_FATAL_ERROR: {
        string err = dhi.action + " Internal Fatal Error";
        throw BESInternalFatalError(err, __FILE__, __LINE__);
    }
    case BES_SYNTAX_USER_ERROR: {
        string err = dhi.action + " Syntax User Error";
        throw BESSyntaxUserError(err, __FILE__, __LINE__);
    }
    case BES_FORBIDDEN_ERROR: {
        string err = dhi.action + " Forbidden Error";
        throw BESForbiddenError(err, __FILE__, __LINE__);
    }
    case BES_NOT_FOUND_ERROR: {
        string err = dhi.action + " Not Found Error";
        throw BESNotFoundError(err, __FILE__, __LINE__);
    }
    }
}

/** @brief transmit the response object built by the execute command
 * using the specified transmitter object
 *
 * If a response object was built then transmit it as text using the specified
 * transmitter object.
 *
 * @param transmitter object that knows how to transmit specific basic types
 * @param dhi structure that holds the request and response information
 * @see BESInfo
 * @see BESTransmitter
 * @see BESDataHandlerInterface
 */
void BESShowErrorResponseHandler::transmit(BESTransmitter * /*transmitter*/, BESDataHandlerInterface & /*dhi*/) {
    string err = "An exception should have been thrown, nothing to transmit";
    throw BESInternalError(err, __FILE__, __LINE__);
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance
 *
 * @param strm C++ i/o stream to dump the information to
 */
void BESShowErrorResponseHandler::dump(ostream &strm) const {
    strm << BESIndent::LMarg << "BESShowErrorResponseHandler::dump - (" << (void *)this << ")" << endl;
    BESIndent::Indent();
    BESResponseHandler::dump(strm);
    BESIndent::UnIndent();
}

BESResponseHandler *BESShowErrorResponseHandler::ResponseBuilder(const string &name) {
    return new BESShowErrorResponseHandler(name);
}
