// ContainersResponseHandler.cc

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2004,2005 University Corporation for Atmospheric Research
// Author: Patrick West <pwest@ucar.org>
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

#include "ContainersResponseHandler.h"
#include "DODSTokenizer.h"
#include "DODSTextInfo.h"
#include "DODSContainerPersistenceList.h"

ContainersResponseHandler::ContainersResponseHandler( string name )
    : DODSResponseHandler( name )
{
}

ContainersResponseHandler::~ContainersResponseHandler( )
{
}

/** @brief executes the command 'show containers;' by returning the list of
 * currently defined containers
 *
 * This response handler knows how to retrieve the list of containers
 * currently defined in the server. It simply asks the container persistence
 * list, that all containers have registered with, to show all containers
 * given the DODSTextInfo object created here.
 *
 * @param dhi structure that holds request and response information
 * @throws DODSResponseException if there is a problem building the
 * response object
 * @see _DODSDataHandlerInterface
 * @see DODSTextInfo
 */
void
ContainersResponseHandler::execute( DODSDataHandlerInterface &dhi )
{
    DODSTextInfo *info = new DODSTextInfo( dhi.transmit_protocol == "HTTP" ) ;
    _response = info ;
    DODSContainerPersistenceList::TheList()->show_containers( *info ) ;
}

/** @brief transmit the response object built by the execute command
 * using the specified transmitter object
 *
 * If a response object was built then transmit it as text.
 *
 * @param transmitter object that knows how to transmit specific basic types
 * @param dhi structure that holds the request and response information
 * @see DODSTextInfo
 * @see DODSTransmitter
 * @see _DODSDataHandlerInterface
 */
void
ContainersResponseHandler::transmit( DODSTransmitter *transmitter,
                               DODSDataHandlerInterface &dhi )
{
    if( _response )
    {
	DODSTextInfo *info = dynamic_cast<DODSTextInfo *>(_response) ;
	transmitter->send_text( *info, dhi );
    }
}

DODSResponseHandler *
ContainersResponseHandler::ContainersResponseBuilder( string handler_name )
{
    return new ContainersResponseHandler( handler_name ) ;
}

// $Log: ContainersResponseHandler.cc,v $
// Revision 1.1  2005/03/15 20:06:20  pwest
// show definitions and show containers response handler
//
