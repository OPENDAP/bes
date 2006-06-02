// ShowContainersResponseHandler.cc

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

#include "ShowContainersResponseHandler.h"
#include "DODSInfo.h"
#include "ContainerStorageList.h"

ShowContainersResponseHandler::ShowContainersResponseHandler( string name )
    : DODSResponseHandler( name )
{
}

ShowContainersResponseHandler::~ShowContainersResponseHandler( )
{
}

/** @brief executes the command 'show containers;' by returning the list of
 * currently defined containers in all container stores
 *
 * This response handler knows how to retrieve information about all
 * containers stored with any container storage objects. This response handler
 * creates a DODSInfo response object to store the container information.
 *
 * @param dhi structure that holds request and response information
 * @throws DODSHandlerException if there is a problem building the response
 * @throws DODSResponseException if a fatal error occurs building the response
 * @see _DODSDataHandlerInterface
 * @see DODSInfo
 * @see ContainerStorageList
 * @see ContainerStorage
 * @see DODSContainer
 */
void
ShowContainersResponseHandler::execute( DODSDataHandlerInterface &dhi )
{
    DODSInfo *info = new DODSInfo( dhi.transmit_protocol == "HTTP" ) ;
    _response = info ;
    ContainerStorageList::TheList()->show_containers( *info ) ;
}

/** @brief transmit the response object built by the execute command
 * using the specified transmitter object
 *
 * If a response object was built (DODSInfo) then transmit it as text.
 *
 * @param transmitter object that knows how to transmit specific basic types
 * @param dhi structure that holds the request and response information
 * @see DODSInfo
 * @see DODSTransmitter
 * @see _DODSDataHandlerInterface
 */
void
ShowContainersResponseHandler::transmit( DODSTransmitter *transmitter,
                                     DODSDataHandlerInterface &dhi )
{
    if( _response )
    {
	DODSInfo *info = dynamic_cast<DODSInfo *>(_response) ;
	transmitter->send_text( *info, dhi );
    }
}

DODSResponseHandler *
ShowContainersResponseHandler::ShowContainersResponseBuilder( string handler_name )
{
    return new ShowContainersResponseHandler( handler_name ) ;
}

