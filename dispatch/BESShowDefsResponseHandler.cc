// BESShowDefsResponseHandler.cc

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

#include "BESShowDefsResponseHandler.h"
#include "BESInfoList.h"
#include "BESInfo.h"
#include "BESDefinitionStorageList.h"
#include "BESResponseNames.h"

BESShowDefsResponseHandler::BESShowDefsResponseHandler( string name )
    : BESResponseHandler( name )
{
}

BESShowDefsResponseHandler::~BESShowDefsResponseHandler( )
{
}

/** @brief executes the command 'show definitions;' by returning the list of
 * currently defined definitions
 *
 * This response handler knows how to retrieve the list of definitions
 * currently defined in the server. It simply asks the definition list
 * to show all definitions given the BESInfo object created here.
 *
 * @param dhi structure that holds request and response information
 * @throws BESHandlerException if there is a problem building the
 * response object
 * @throws BESResponseException upon fatal error building the response
 * object
 * @see _BESDataHandlerInterface
 * @see BESInfo
 * @see BESDefinitionStorageList
 */
void
BESShowDefsResponseHandler::execute( BESDataHandlerInterface &dhi )
{
    BESInfo *info = BESInfoList::TheList()->build_info() ;
    _response = info ;
    dhi.action_name = SHOWDEFS_RESPONSE_STR ;
    info->begin_response( SHOWDEFS_RESPONSE_STR ) ;
    BESDefinitionStorageList::TheList()->show_definitions( *info ) ;
    info->end_response() ;
}

/** @brief transmit the response object built by the execute command
 * using the specified transmitter object
 *
 * If a response object was built then transmit it as text using the specified
 * transmitter.
 *
 * @param transmitter object that knows how to transmit specific basic types
 * @param dhi structure that holds the request and response information
 * @see BESInfo
 * @see BESTransmitter
 * @see _BESDataHandlerInterface
 */
void
BESShowDefsResponseHandler::transmit( BESTransmitter *transmitter,
                               BESDataHandlerInterface &dhi )
{
    if( _response )
    {
	BESInfo *info = dynamic_cast<BESInfo *>(_response) ;
	info->transmit( transmitter, dhi ) ;
    }
}

BESResponseHandler *
BESShowDefsResponseHandler::ShowDefsResponseBuilder( string handler_name )
{
    return new BESShowDefsResponseHandler( handler_name ) ;
}

