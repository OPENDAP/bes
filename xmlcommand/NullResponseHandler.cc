// NullResponseHandler.cc

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2018 University Corporation for Atmospheric Research
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
// You can contact University Corporation for Atmospheric Research at
// 3080 Center Green Drive, Boulder, CO 80301

#include "config.h"

#include <cassert>

#include <string>
#include <sstream>

#include "BESContextManager.h"
#include "BESDataNames.h"
#include "BESSyntaxUserError.h"

#include "SetContextsNames.h"
#include "NullResponseHandler.h"

#include "BESDebug.h"

using namespace bes;
using namespace std;

/** @brief Minimal execution
 *
 * Set the ResponseObject for this ResponseHandler to null.
 *
 * @param dhi structure that holds request and response information
 * @throws BESSyntaxUserError if no context name was given object
 */
void NullResponseHandler::execute(BESDataHandlerInterface &dhi)
{
    // This would be used in the transmit() method below to send a response back to the
    // BES's client, if this command returned data. Since it does not, this can be NULL
    // and the transmit() method can be a no-op. jhrg 2/8/18
    _response = 0;
}

/** @brief This is a no-op
 *
 * The NullResponseHandler does not transmit a response. Errors are returned using
 * the DHI's error object field and are returned to the BES's client with ExceptionHandler.
 *
 * @param transmitter object that knows how to transmit specific basic types
 * @param dhi structure that holds the request and response information
 */
void NullResponseHandler::transmit(BESTransmitter */*transmitter*/, BESDataHandlerInterface &/*dhi*/)
{
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance
 *
 * @param strm C++ i/o stream to dump the information to
 */
void NullResponseHandler::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << "NullResponseHandler::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();
    BESResponseHandler::dump(strm);
    BESIndent::UnIndent();
}

BESResponseHandler *
NullResponseHandler::NullResponseBuilder(const string &name)
{
    return new NullResponseHandler(name);
}

