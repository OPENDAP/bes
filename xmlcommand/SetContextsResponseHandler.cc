// SetContextsResponseHandler.cc

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2018 OPeNDAP, Inc.
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

#include <cassert>

#include <string>
#include <sstream>

#include "BESContextManager.h"
#include "BESDataNames.h"
#include "BESSyntaxUserError.h"

#include "SetContextsNames.h"
#include "SetContextsResponseHandler.h"

#include "BESDebug.h"

using namespace bes;
using namespace std;

/** @brief Set multiple contexts in the BES's Context Manager
 *
 * @param dhi structure that holds request and response information
 * @throws BESSyntaxUserError if no context name was given object
 * @see BESDataHandlerInterface
 * @see BESInfo
 * @see BESContextManager
 */
void SetContextsResponseHandler::execute(BESDataHandlerInterface &dhi)
{
    // To record the information in the DHI data[] map, use one value to hold a
    // list of all of Context names, then use the names to hold the values. Read
    // this using names = data[CONTEXT_NAMES] and then:
    //
    // for each 'name' in names { value = data['context_'name] }
    //
    string names = dhi.data[CONTEXT_NAMES];
    if (names.empty())
        throw BESSyntaxUserError( "setContexts: No context names found in the data.", __FILE__, __LINE__);

    BESDEBUG("besxml", "dhi.data[CONTEXT_NAMES]: " << names << endl);

    istringstream iss(names);
    string name;
    iss >> name;
    while (iss && !name.empty()) {

         string value = dhi.data[name];

         if (value.empty())
             throw BESSyntaxUserError( "setContexts: Context value was not found in the data.", __FILE__, __LINE__);

         // Remove the CONTEXT_PREFIX before adding the context
         assert(name.find(CONTEXT_PREFIX) != string::npos); // Is the prefix really there?
         name.erase(0, sizeof(CONTEXT_PREFIX)-1);   // The constant includes the two double quotes

         BESDEBUG("besxml", "BESContextManager::TheManager()->set_context(" << name << ", " << value << ")" << endl);

         BESContextManager::TheManager()->set_context(name, value);

         iss >> name;
     }

    // This would be used in the transmit() method below to send a response back to the
    // BES's client, if this command returned data. Since it does not, this can be NULL
    // and the transmit() method can be a no-op. jhrg 2/8/18
    d_response_object = 0;
}

/** @brief For the setContexts command, this is a no-op
 *
 * @param transmitter object that knows how to transmit specific basic types
 * @param dhi structure that holds the request and response information
 */
void SetContextsResponseHandler::transmit(BESTransmitter */*transmitter*/, BESDataHandlerInterface &/*dhi*/)
{
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance
 *
 * @param strm C++ i/o stream to dump the information to
 */
void SetContextsResponseHandler::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << "BESSetContextResponseHandler::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();
    BESResponseHandler::dump(strm);
    BESIndent::UnIndent();
}

BESResponseHandler *
SetContextsResponseHandler::SetContextsResponseBuilder(const string &name)
{
    return new SetContextsResponseHandler(name);
}

