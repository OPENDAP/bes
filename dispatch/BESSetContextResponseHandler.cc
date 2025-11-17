// BESSetContextResponseHandler.cc

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

#include "BESSetContextResponseHandler.h"

#if 0
#include "BESSilentInfo.h"
#endif

#include "BESContextManager.h"
#include "BESDataHandlerInterface.h"
#include "BESDataNames.h"
#include "BESResponseNames.h"
#include "BESSyntaxUserError.h"

using std::endl;
using std::ostream;
using std::string;

BESSetContextResponseHandler::BESSetContextResponseHandler(const string &name) : BESResponseHandler(name) {}

BESSetContextResponseHandler::~BESSetContextResponseHandler() {}

/** @brief executes the command to set context within the BES
 *
 * Using a context name and a context value, set that context in the context
 * manager.
 *
 * The response object is silent, i.e. nothing is returned to the client
 * unless there is an exception condition.
 *
 * @param dhi structure that holds request and response information
 * @throws BESSyntaxUserError if no context name was given
 * object
 * @see BESDataHandlerInterface
 * @see BESInfo
 * @see BESContextManager
 */
void BESSetContextResponseHandler::execute(BESDataHandlerInterface &dhi) {
#if 0
    dhi.action_name = SET_CONTEXT_STR;
    BESInfo *info = new BESSilentInfo();
    d_response_object = info;
#endif

    // the name string cannot be the empty string. No other restrictions
    // apply.
    string name = dhi.data[CONTEXT_NAME];
    if (name.empty()) {
        string e = "No context name was specified in set context command";
        throw BESSyntaxUserError(e, __FILE__, __LINE__);
    }

    string value = dhi.data[CONTEXT_VALUE];

    BESContextManager::TheManager()->set_context(name, value);
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
void BESSetContextResponseHandler::transmit(BESTransmitter * /*transmitter*/, BESDataHandlerInterface & /*dhi*/) {
#if 0
    if (d_response_object) {
        BESInfo *info = dynamic_cast<BESInfo *>(d_response_object);
        if (!info) throw BESInternalError("Expected an Info object.", __FILE__, __LINE__);
        info->transmit(transmitter, dhi);
    }
#endif
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance
 *
 * @param strm C++ i/o stream to dump the information to
 */
void BESSetContextResponseHandler::dump(ostream &strm) const {
    strm << BESIndent::LMarg << "BESSetContextResponseHandler::dump - (" << (void *)this << ")" << endl;
    BESIndent::Indent();
    BESResponseHandler::dump(strm);
    BESIndent::UnIndent();
}

BESResponseHandler *BESSetContextResponseHandler::SetContextResponseBuilder(const string &name) {
    return new BESSetContextResponseHandler(name);
}
