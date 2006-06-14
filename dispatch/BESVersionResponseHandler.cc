// BESVersionResponseHandler.cc

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

#include "config.h"

#include "BESVersionResponseHandler.h"
#include "BESVersionInfo.h"
#include "cgi_util.h"
#include "util.h"
#include "dispatch_version.h"
#include "BESRequestHandlerList.h"

BESVersionResponseHandler::BESVersionResponseHandler( string name )
    : BESResponseHandler( name )
{
}

BESVersionResponseHandler::~BESVersionResponseHandler( )
{
}

/** @brief executes the command 'show version;' by returning the version of
 * the OPeNDAP server and the version of all registered data request
 * handlers.
 *
 * This response handler knows how to retrieve the version of the OPeNDAP
 * server. It adds this information to a BESVersionInfo informational response
 * object. It also forwards the request to all registered data request
 * handlers to add their version information.
 *
 * @param dhi structure that holds request and response information
 * @throws BESHandlerException if there is a problem building the
 * response object
 * @throws BESResponseException upon fatal error building the response
 * object
 * @see _BESDataHandlerInterface
 * @see BESVersionInfo
 * @see BESRequestHandlerList
 */
void
BESVersionResponseHandler::execute( BESDataHandlerInterface &dhi )
{
    BESVersionInfo *info =
	new BESVersionInfo( dhi.transmit_protocol == "HTTP" ) ;
    _response = info ;
    info->addDAPVersion( "2.0" ) ;
    info->addDAPVersion( "3.0" ) ;
    info->addDAPVersion( "3.2" ) ;
    info->addBESVersion( libdap_name(), libdap_version() ) ;
    info->addBESVersion( bes_name(), bes_version() ) ;
    BESRequestHandlerList::TheList()->execute_all( dhi ) ;
}

/** @brief transmit the response object built by the execute command
 * using the specified transmitter object
 *
 * If a response object was built then transmit it as text using the specified
 * transmitter object.
 *
 * @param transmitter object that knows how to transmit specific basic types
 * @param dhi structure that holds the request and response information
 * @see BESResponseObject
 * @see BESTransmitter
 * @see _BESDataHandlerInterface
 */
void
BESVersionResponseHandler::transmit( BESTransmitter *transmitter,
                                  BESDataHandlerInterface &dhi )
{
    if( _response )
    {
	BESVersionInfo *info = dynamic_cast<BESVersionInfo *>(_response) ;
	transmitter->send_text( *info, dhi );
    }
}

BESResponseHandler *
BESVersionResponseHandler::VersionResponseBuilder( string handler_name )
{
    return new BESVersionResponseHandler( handler_name ) ;
}

