// SetContextsResponseHandler.h

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

#ifndef SetContextsResponseHandler_h
#define SetContextsResponseHandler_h 1

#include "BESResponseHandler.h"

namespace bes {

/** @brief Set a number of context name-value pairs at once
 *
 * Given that the XMLSetContextsCommand <setContexts> was found, perform that
 * action. A number of BES context name-value pairs are encoded in the DHI.data[]
 * map (a simple associative array that maps one string value to another).
 * Read those name-value pairs and add them to the BES's Context data store.
 *
 * This ResponseHandler instance will not transmit anything back to the BES's
 * client unless there is an error.
 *
 * @note The XMLSetContextsCommand can be built to use the NullResponseHandler
 * instead.
 *
 * @see BESResponseObject
 */
class SetContextsResponseHandler: public BESResponseHandler {
public:

    SetContextsResponseHandler(const std::string &name): BESResponseHandler(name) { }
    virtual ~SetContextsResponseHandler(void) { }

    virtual void execute(BESDataHandlerInterface &dhi);
    virtual void transmit(BESTransmitter *transmitter, BESDataHandlerInterface &dhi);

    void dump(std::ostream &strm) const override;

    // Factory method, used by the DefaultModule to add this to the list of
    // ResponseHandlers for a given 'action'
    static BESResponseHandler *SetContextsResponseBuilder(const std::string &name);
};

}   // namespace bes

#endif // SetContextsResponseHandler_h

