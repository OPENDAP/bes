// BESHelpResponseHandler.cc

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

#include "BESHelpResponseHandler.h"
#include "BESInfoList.h"
#include "BESInfo.h"
#include "BESRequestHandlerList.h"
#include "BESRequestHandler.h"
#include "BESResponseNames.h"

BESHelpResponseHandler::BESHelpResponseHandler( string name )
    : BESResponseHandler( name )
{
}

BESHelpResponseHandler::~BESHelpResponseHandler( )
{
}

/** @brief executes the command 'show help;' by returning general help
 * information as well as help information for all of the data request
 * handlers registered.
 *
 * The BESHelpResponseHandler first retreives general help information from help
 * files located in the file pointed to by either the key OPeNDAP.Help.TXT if
 * the client is a basic text client or OPeNDAP.Help.HTTP if the client is
 * HTML based. It then lists each of the data types registered to handle
 * requests (such as NetCDF, HDF, Cedar, etc...). Then for all data request
 * handlers registered with BESRequestHandlerList help information can be
 * added to the informational object.
 *
 * The response object BESHTMLInfo is created to store the help information.
 *
 * @param dhi structure that holds request and response information
 * @throws BESHandlerException if there is a problem building the
 * response object
 * @throws BESResponseException upon fatal error building the response
 * object
 * @see _BESDataHandlerInterface
 * @see BESHTMLInfo
 * @see BESRequestHandlerList
 */
void
BESHelpResponseHandler::execute( BESDataHandlerInterface &dhi )
{
    BESInfo *info = BESInfoList::TheList()->build_info() ;
    _response = info ;

    info->begin_response( HELP_RESPONSE_STR ) ;
    dhi.action_name = HELP_RESPONSE_STR ;

    info->begin_tag( "BES" ) ;
    info->add_data_from_file( "BES.Help", "OPeNDAP BES Help" ) ;
    info->end_tag( "BES" ) ;

    // execute help for each registered request server
    info->add_break( 2 ) ;

    info->begin_tag( "Handlers" ) ;
    BESRequestHandlerList::TheList()->execute_all( dhi ) ;
    info->end_tag( "Handlers" ) ;

    info->end_response() ;
}

/** @brief transmit the response object built by the execute command
 * using the specified transmitter object
 *
 * If a response object was built then transmit it as text or html, depending
 * on whether the client making the request can handle HTML information.
 *
 * @param transmitter object that knows how to transmit specific basic types
 * @param dhi structure that holds the request and response information
 * @see BESHTMLInfo
 * @see BESTransmitter
 * @see _BESDataHandlerInterface
 */
void
BESHelpResponseHandler::transmit( BESTransmitter *transmitter,
                               BESDataHandlerInterface &dhi )
{
    if( _response )
    {
	BESInfo *info = dynamic_cast<BESInfo *>(_response) ;
	info->transmit( transmitter, dhi ) ;
    }
}

BESResponseHandler *
BESHelpResponseHandler::HelpResponseBuilder( string handler_name )
{
    return new BESHelpResponseHandler( handler_name ) ;
}

