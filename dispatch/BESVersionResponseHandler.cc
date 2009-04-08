// BESVersionResponseHandler.cc

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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// You can contact University Corporation for Atmospheric Research at
// 3080 Center Green Drive, Boulder, CO 80301
 
// (c) COPYRIGHT University Corporation for Atmospheric Research 2004-2005
// Please read the full copyright statement in the file COPYRIGHT_UCAR.
//
// Authors:
//      pwest       Patrick West <pwest@ucar.edu>
//      jgarcia     Jose Garcia <jgarcia@ucar.edu>

#include "config.h"

#include "BESVersionResponseHandler.h"
#include "BESVersionInfo.h"
#include "BESRequestHandlerList.h"
#include "BESResponseNames.h"

BESVersionResponseHandler::BESVersionResponseHandler( const string &name )
    : BESResponseHandler( name )
{
}

BESVersionResponseHandler::~BESVersionResponseHandler( )
{
}

/** @brief executes the command 'show version;' by returning the version of
 * the BES and the version of all registered data request
 * handlers.
 *
 * This response handler knows how to retrieve the version of the BES
 * server. It adds this information to a BESVersionInfo informational response
 * object. It also forwards the request to all registered data request
 * handlers to add their version information.
 *
 * @param dhi structure that holds request and response information
 * @see BESDataHandlerInterface
 * @see BESVersionInfo
 * @see BESRequestHandlerList
 */
void
BESVersionResponseHandler::execute( BESDataHandlerInterface &dhi )
{
    BESVersionInfo *info = new BESVersionInfo() ;
    _response = info ;
    dhi.action_name = VERS_RESPONSE_STR ;
    info->begin_response( VERS_RESPONSE_STR, dhi ) ;

    info->add_library( PACKAGE_NAME, PACKAGE_VERSION ) ;

    BESRequestHandlerList::TheList()->execute_all( dhi ) ;

    info->end_response() ;
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
 * @see BESDataHandlerInterface
 */
void
BESVersionResponseHandler::transmit( BESTransmitter *transmitter,
                                  BESDataHandlerInterface &dhi )
{
    if( _response )
    {
	BESVersionInfo *info = dynamic_cast<BESVersionInfo *>(_response) ;
	if( !info )
	    throw BESInternalError( "cast error", __FILE__, __LINE__ ) ;
	info->transmit( transmitter, dhi ) ;
    }
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance
 *
 * @param strm C++ i/o stream to dump the information to
 */
void
BESVersionResponseHandler::dump( ostream &strm ) const
{
    strm << BESIndent::LMarg << "BESVersionResponseHandler::dump - ("
			     << (void *)this << ")" << endl ;
    BESIndent::Indent() ;
    BESResponseHandler::dump( strm ) ;
    BESIndent::UnIndent() ;
}

BESResponseHandler *
BESVersionResponseHandler::VersionResponseBuilder( const string &name )
{
    return new BESVersionResponseHandler( name ) ;
}

