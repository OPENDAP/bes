// BESStatusResponseHandler.cc

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

#include "BESStatusResponseHandler.h"
#include "BESInfoList.h"
#include "BESInfo.h"
#include "BESStatus.h"
#include "BESResponseNames.h"

BESStatusResponseHandler::BESStatusResponseHandler( string name )
    : BESResponseHandler( name )
{
}

BESStatusResponseHandler::~BESStatusResponseHandler( )
{
}

/** @brief executes the command 'show status;' by returning the status of
 * the server process
 *
 * This response handler knows how to retrieve the status for the server
 * process handing this clients requests from BESStatus and stores it in a
 * BESInfo informational response object.
 *
 * @param dhi structure that holds request and response information
 * @throws BESHandlerException if there is a problem building the
 * response object
 * @throws BESResponseException upon fatal error building the response
 * object
 * @see _BESDataHandlerInterface
 * @see BESInfo
 * @see BESStatus
 */
void
BESStatusResponseHandler::execute( BESDataHandlerInterface &dhi )
{
    BESInfo *info = BESInfoList::TheList()->build_info() ;
    _response = info ;
    BESStatus s ;
    dhi.action_name = STATUS_RESPONSE_STR ;
    info->begin_response( STATUS_RESPONSE_STR ) ;
    info->add_tag( "status", s.get_status() ) ;
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
 * @see DODSResponseObject
 * @see BESTransmitter
 * @see _BESDataHandlerInterface
 */
void
BESStatusResponseHandler::transmit( BESTransmitter *transmitter,
                                  BESDataHandlerInterface &dhi )
{
    if( _response )
    {
	BESInfo *info = dynamic_cast<BESInfo *>(_response) ;
	info->transmit( transmitter, dhi ) ;
    }
}

BESResponseHandler *
BESStatusResponseHandler::StatusResponseBuilder( string handler_name )
{
    return new BESStatusResponseHandler( handler_name ) ;
}

