// OPENDAP_RESPONSEResponseHandler.h

// Copyright (c) 2013 OPeNDAP, Inc. Author: James Gallagher
// <jgallagher@opendap.org>, Patrick West <pwest@opendap.org>
// Nathan Potter <npotter@opendap.org>
//                                                                            
// modify it under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation; either version 2.1 of
// the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// Lesser General Public License for more details.
//
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
// 02110-1301 U\ SA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI.
// 02874-0112.
#ifndef I_OPENDAP_RESPONSEResponseHandler_h
#define I_OPENDAP_RESPONSEResponseHandler_h 1

#include "BESResponseHandler.h"

class OPENDAP_RESPONSEResponseHandler : public BESResponseHandler {
public:
				OPENDAP_RESPONSEResponseHandler( const string &name ) ;
    virtual			~OPENDAP_RESPONSEResponseHandler( void ) ;

    virtual void		execute( BESDataHandlerInterface &dhi ) ;
    virtual void		transmit( BESTransmitter *transmitter,
                                          BESDataHandlerInterface &dhi ) ;

    virtual void		dump( ostream &strm ) const ;

    static BESResponseHandler *OPENDAP_RESPONSEResponseBuilder( const string &name ) ;
};

#endif // I_OPENDAP_RESPONSEResponseHandler_h

