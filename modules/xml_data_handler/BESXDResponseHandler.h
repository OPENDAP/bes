// BESXDResponseHandler.h

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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//
// You can contact University Corporation for Atmospheric Research at
// 3080 Center Green Drive, Boulder, CO 80301
 
// Authors:
//      pwest       Patrick West <pwest@ucar.edu>
//      jgarcia     Jose Garcia <jgarcia@ucar.edu>

#ifndef I_BESXDResponseHandler_h
#define I_BESXDResponseHandler_h 1

#include <BESResponseHandler.h>

/** @brief response handler that builds an OPeNDAP ASCII response object
 *
 * A request 'get xml_data for &lt;def_name&gt;;' will be handled by this
 * response handler. Given a definition name it determines what containers
 * are to be used to build the OPeNDAP ASCII response object. It then
 * transmits the DataDDS object as xml_data values.
 *
 * @see DataDDS
 * @see BESContainer
 * @see BESTransmitter
 */
class BESXDResponseHandler : public BESResponseHandler {
public:
				BESXDResponseHandler( const string &name ) ;
    virtual			~BESXDResponseHandler( void ) ;

    virtual void		execute( BESDataHandlerInterface &dhi ) ;
    virtual void		transmit( BESTransmitter *transmitter,
                                          BESDataHandlerInterface &dhi ) ;

    static BESResponseHandler *XDResponseBuilder( const string &name ) ;
};

#endif // I_BESXDResponseHandler_h

