// BESResponseHandler.h

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2004,2005 University Corporation for Atmospheric Research
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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// You can contact University Corporation for Atmospheric Research at
// 3080 Center Green Drive, Boulder, CO 80301
 
// (c) COPYRIGHT University Corporation for Atmostpheric Research 2004-2005
// Please read the full copyright statement in the file COPYRIGHT_UCAR.
//
// Authors:
//      pwest       Patrick West <pwest@ucar.edu>
//      jgarcia     Jose Garcia <jgarcia@ucar.edu>

#ifndef I_BESResponseHandler_h
#define I_BESResponseHandler_h 1

#include <string>

#include <DODSResponseObject.h>

#include "BESDataHandlerInterface.h"
#include "BESTransmitter.h"

using std::string ;
#if 0
class DODSResponseObject ;
#endif

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
 * help information in needs to add.
 *
 * Response handlers such as the BESStatusResponseHandler (and others) are able
 * to create the informational response object and fill it in. But usually,
 * the response handler passes the response object to another object to have
 * it fill in the response object.
 *
 * A response handler object also knows how to transmit the response object
 * using a BESTransmitter object.
 *
 * This is an abstract base class for response handlers. Derived classes
 * implement the methods execute and transmit.
 *
 * @see DODSResponseObject
 * @see BESContainer
 * @see BESRequestHandler
 * @see BESResponseHandlerList
 * @see BESTransmitter
 */
class BESResponseHandler {
protected:
    string			_response_name ;
    DODSResponseObject		*_response ;
    
				BESResponseHandler( string name ) ;
public:
    virtual			~BESResponseHandler(void) ;
    
    /** @brief return the current response object
     *
     * Returns the current response object, null if one has not yet been
     * created. The response handler maintains ownership of the response
     * object.
     *
     * @return current response object
     * @see DODSResponseObject
     */
    virtual DODSResponseObject  *get_response_object() ;

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
     * @see DODSResponseObject
     */
    virtual DODSResponseObject	*set_response_object( DODSResponseObject *o ) ;

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
     * @see _BESDataHandlerInterface
     * @see DODSResponseObject
     */
    virtual void		execute( BESDataHandlerInterface &dhi ) = 0 ;

    /** @brief transmit the respobse object built by the execute command
     * using the specified transmitter object
     *
     * @param transmitter object that knows how to transmit specific basic types
     * @param dhi structure that holds the request and response information
     * @throws BESTransmitException if problem transmitting the response obj
     * @see DODSResponseObject
     * @see BESTransmitter
     * @see _BESDataHandlerInterface
     * @see BESTransmitException
     */
    virtual void		transmit( BESTransmitter *transmitter,
                                          BESDataHandlerInterface &dhi ) = 0 ;

    /** @brief return the name of this response object
     *
     * This name is used to determine which response handler can handle a
     * requested responose, such as das, dds, ddx, tab, info, version, help,
     * etc...
     *
     * @return response name
     */
    virtual string		get_name( ) { return _response_name ; }
};

#endif // I_BESResponseHandler_h

