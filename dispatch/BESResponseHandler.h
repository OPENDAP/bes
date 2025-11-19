// BESResponseHandler.h

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

#ifndef I_BESResponseHandler_h
#define I_BESResponseHandler_h 1

#include <string>

#include "BESObj.h"

class BESResponseObject;
class BESDataHandlerInterface;
class BESTransmitter;

/** @brief handler object that knows how to create a specific response object
 *
 * A response handler is something that knows how to create a response
 * object, such as a DAS, DDS, DDX, informational response object. It knows
 * how to construct the requested response object but does not necessarily
 * fill in the response object. It does know, however, how to have other
 * objects fill in the response object. For example, a BESDASResponseHandler
 * knows that it needs to go to each of the request handlers (data handlers)
 * for each of the containers requested so that those request handlers can
 * fill in the response object. Another example is the BESHelpResponseHandler,
 * which knows to construct an informational response object and then pass
 * that informational response object to each registered request handler (data
 * handler) so that each of the request handlers has an opportunity to add any
 * help information it needs to add.
 *
 * Response handlers such as the BESStatusResponseHandler (and others) are able
 * to create the informational response object and fill it in without handing
 * off the response object to any other object. But usually,
 * the response handler passes the response object to another object to have
 * it fill in the response object.
 *
 * A response handler object also knows how to transmit the response object
 * using a BESTransmitter object.
 *
 * This is an abstract base class for response handlers. Derived classes
 * implement the methods execute and transmit.
 *
 * @see BESResponseObject
 * @see BESContainer
 * @see BESRequestHandler
 * @see BESResponseHandlerList
 * @see BESTransmitter
 */
class BESResponseHandler : public BESObj {
protected:
    std::string d_response_name;
    BESResponseObject *d_response_object = nullptr;

    std::string d_annotation_service_url; //< If not null, include this in the DAS/DMR

    friend class resplistT;

public:
    explicit BESResponseHandler(std::string name);

    ~BESResponseHandler() override;

    /** @brief return the current response object
     *
     * Returns the current response object, null if one has not yet been
     * created. The response handler maintains ownership of the response
     * object.
     *
     * @return current response object
     * @see BESResponseObject
     */
    virtual BESResponseObject *get_response_object();

    /** @brief replaces the current response object with the specified one,
     * returning the current response object
     *
     * This method is used to replace the response object with a new one, for
     * example if during aggregation a new response object is built from the
     * current response object.
     *
     * The caller of set_response_object now owns the returned response
     * object. The new response object is now owned by the response object.
     *
     * @param o new response object used to replace the current one
     * @return the response object being replaced
     * @see BESResponseObject
     */
    virtual BESResponseObject *set_response_object(BESResponseObject *o);

    /** @brief knows how to build a requested response object
     *
     * Derived instances of this abstract base class know how to create a
     * specific response object and what objects (including itself) to pass
     * that response object to for it to be filled in.
     *
     * @param dhi structure that holds request and response information
     * @throws BESHandlerException if there is a problem building the
     * response object
     * @throws BESResponseException upon fatal error building the response
     * object
     * @see BESDataHandlerInterface
     * @see BESResponseObject
     */
    virtual void execute(BESDataHandlerInterface &dhi) = 0;

    /** @brief transmit the response object built by the execute command
     * using the specified transmitter object
     *
     * @param transmitter object that knows how to transmit specific basic types
     * @param dhi structure that holds the request and response information
     * @throws BESTransmitException if problem transmitting the response obj
     * @see BESResponseObject
     * @see BESTransmitter
     * @see BESDataHandlerInterface
     * @see BESTransmitException
     */
    virtual void transmit(BESTransmitter *transmitter, BESDataHandlerInterface &dhi) = 0;

    void dump(std::ostream &strm) const override;
};

#endif // I_BESResponseHandler_h
