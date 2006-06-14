// BESShowContainersResponseHandler.cc

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

#include "BESShowContainersResponseHandler.h"
#include "BESInfo.h"
#include "BESContainerStorageList.h"

BESShowContainersResponseHandler::BESShowContainersResponseHandler( string name )
    : BESResponseHandler( name )
{
}

BESShowContainersResponseHandler::~BESShowContainersResponseHandler( )
{
}

/** @brief executes the command 'show containers;' by returning the list of
 * currently defined containers in all container stores
 *
 * This response handler knows how to retrieve information about all
 * containers stored with any container storage objects. This response handler
 * creates a BESInfo response object to store the container information.
 *
 * @param dhi structure that holds request and response information
 * @throws BESHandlerException if there is a problem building the response
 * @throws BESResponseException if a fatal error occurs building the response
 * @see _BESDataHandlerInterface
 * @see BESInfo
 * @see BESContainerStorageList
 * @see BESContainerStorage
 * @see BESContainer
 */
void
BESShowContainersResponseHandler::execute( BESDataHandlerInterface &dhi )
{
    BESInfo *info = new BESInfo( dhi.transmit_protocol == "HTTP" ) ;
    _response = info ;
    BESContainerStorageList::TheList()->show_containers( *info ) ;
}

/** @brief transmit the response object built by the execute command
 * using the specified transmitter object
 *
 * If a response object was built (BESInfo) then transmit it as text.
 *
 * @param transmitter object that knows how to transmit specific basic types
 * @param dhi structure that holds the request and response information
 * @see BESInfo
 * @see BESTransmitter
 * @see _BESDataHandlerInterface
 */
void
BESShowContainersResponseHandler::transmit( BESTransmitter *transmitter,
                                     BESDataHandlerInterface &dhi )
{
    if( _response )
    {
	BESInfo *info = dynamic_cast<BESInfo *>(_response) ;
	transmitter->send_text( *info, dhi );
    }
}

BESResponseHandler *
BESShowContainersResponseHandler::ShowContainersResponseBuilder( string handler_name )
{
    return new BESShowContainersResponseHandler( handler_name ) ;
}

