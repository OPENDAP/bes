// NullResponseHandler.h

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2018 OPeNDAP, Inc
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

#ifndef NullResponseHandler_h
#define NullResponseHandler_h 1

#include "BESResponseHandler.h"

namespace bes {

/** @brief A ResponseHandler that does nothing
 *
 * Many commands don't send a response back to the BES's client. Instead, they
 * modify the BES's current state. This ResponseHandler is useful when the
 * entire command's action (like setContexts) can be performed during the
 * XMLInterface::build_data_request_plan() phase of command evaluation. This
 * ResponseHandler will do only bookkeeping during its execute() method and
 * nothing at all during transmit().
 *
 * This ResponseHandler instance will not transmit anything back to the BES's
 * client unless there is an error.
 *
 * @see BESResponseObject
 */
class NullResponseHandler: public BESResponseHandler {
public:

    NullResponseHandler(const std::string &name): BESResponseHandler(name) { }
    virtual ~NullResponseHandler(void) { }

    /** @brief Minimal execution
     *
     * Set the ResponseObject for this ResponseHandler to null.
     *
     * @param dhi structure that holds request and response information
     * @throws BESSyntaxUserError if no context name was given object
     */
    virtual void execute(BESDataHandlerInterface &/*dhi*/) {
        // This would be used in the transmit() method below to send a response back to the
        // BES's client, if this command returned data. Since it does not, this can be NULL
        // and the transmit() method can be a no-op. jhrg 2/8/18
        d_response_object = 0;
    }

    /** @brief This is a no-op
     *
     * The NullResponseHandler does not transmit a response. Errors are returned using
     * the DHI's error object field and are returned to the BES's client with ExceptionHandler.
     *
     * @param transmitter object that knows how to transmit specific basic types
     * @param dhi structure that holds the request and response information
     */
    virtual void transmit(BESTransmitter */*transmitter*/, BESDataHandlerInterface &/*dhi*/) { }

    virtual void dump(std::ostream &strm) const {
        strm << BESIndent::LMarg << "NullResponseHandler::dump - (" << (void *) this << ")" << std::endl;
        BESIndent::Indent();
        BESResponseHandler::dump(strm);
        BESIndent::UnIndent();
    }


    // Factory method, used by the DefaultModule to add this to the list of
    // ResponseHandlers for a given 'action'
    static BESResponseHandler *NullResponseBuilder(const std::string &name) {
        return new NullResponseHandler(name);
    }
};

}   // namespace bes

#endif // NullResponseHandler_h

