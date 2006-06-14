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
#include "BESHTMLInfo.h"
#include "cgi_util.h"
#include "BESRequestHandlerList.h"
#include "BESRequestHandler.h"

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
    BESInfo *info = 0 ;
    if( dhi.transmit_protocol == "HTTP" )
	info = new BESHTMLInfo( dhi.transmit_protocol == "HTTP" ) ;
    else
	info = new BESInfo( dhi.transmit_protocol == "HTTP" ) ;
    _response = info ;

    // if http then prepare header information for html output
    if( dhi.transmit_protocol == "HTTP" )
    {
	info->add_data( "<HTML>\n" ) ;
	info->add_data( "<HEAD>\n" ) ;
	info->add_data( "<TITLE>OPeNDAP General Help</TITLE>\n" ) ;
	info->add_data( "</HEAD>\n" ) ;
	info->add_data( "<BODY>\n" ) ;
    }

    // get the list of registered servers and the responses that they handle
    info->add_data( "Registered data request handlers:\n" ) ;
    BESRequestHandlerList::Handler_citer i =
	BESRequestHandlerList::TheList()->get_first_handler() ;
    BESRequestHandlerList::Handler_citer ie =
	BESRequestHandlerList::TheList()->get_last_handler() ;
    for( ; i != ie; i++ ) 
    {
	if( dhi.transmit_protocol == "HTTP" )
	{
	    info->add_data( "<BR />&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;\n" ) ;
	}
	else
	{
	    info->add_data( "    " ) ;
	}
	BESRequestHandler *rh = (*i).second ;
	string name = rh->get_name() ;
	string resp = rh->get_handler_names() ;
	info->add_data( name + ": handles " + resp + "\n" ) ;
    }

    string key ;
    if( dhi.transmit_protocol == "HTTP" )
	key = (string)"OPeNDAP.Help." + dhi.transmit_protocol ;
    else
	key = "OPeNDAP.Help.TXT" ;
    info->add_data_from_file( key, "general help" ) ;

    // execute help for each registered request server
    if( dhi.transmit_protocol == "HTTP" )
    {
	info->add_data( "<BR /><BR />\n" ) ;
    }
    else
    {
	info->add_data( "\n\n" ) ;
    }

    BESRequestHandlerList::TheList()->execute_all( dhi ) ;

    // if http then close out the html format
    if( dhi.transmit_protocol == "HTTP" )
    {
	info->add_data( "</BODY>\n" ) ;
	info->add_data( "</HTML>\n" ) ;
    }
    else
    {
	info->add_data( "\n" ) ;
    }
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
	if( dhi.transmit_protocol == "HTTP" )
	    transmitter->send_html( *info, dhi ) ;
	else
	    transmitter->send_text( *info, dhi ) ;
    }
}

BESResponseHandler *
BESHelpResponseHandler::HelpResponseBuilder( string handler_name )
{
    return new BESHelpResponseHandler( handler_name ) ;
}

