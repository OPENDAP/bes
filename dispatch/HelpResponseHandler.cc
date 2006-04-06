// HelpResponseHandler.cc

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

#include "HelpResponseHandler.h"
#include "DODSHTMLInfo.h"
#include "cgi_util.h"
#include "DODSRequestHandlerList.h"
#include "DODSRequestHandler.h"

HelpResponseHandler::HelpResponseHandler( string name )
    : DODSResponseHandler( name )
{
}

HelpResponseHandler::~HelpResponseHandler( )
{
}

/** @brief executes the command 'show help;' by returning general help
 * information as well as help information for all of the data request
 * handlers registered with DODSRequestHandlerList.
 *
 * The HelpResponseHandler first retreives general help information from help
 * files located in the file pointed to by either the key OPeNDAP.Help.TXT if
 * the client is a basic text client or OPeNDAP.Help.HTTP if the client is
 * HTML based. It then lists each of the data types registered to handle
 * requests (such as NetCDF, HDF, Cedar, etc...). Then for all data request
 * handlers registered with DODSRequestHandlerList help information can be
 * added to the informational object.
 *
 * The response object DODSHTMLInfo is created to store the help information.
 *
 * @param dhi structure that holds request and response information
 * @throws DODSResponseException if there is a problem building the
 * response object
 * @see _DODSDataHandlerInterface
 * @see DODSHTMLInfo
 * @see DODSRequestHandlerList
 */
void
HelpResponseHandler::execute( DODSDataHandlerInterface &dhi )
{
    DODSInfo *info = 0 ;
    if( dhi.transmit_protocol == "HTTP" )
	info = new DODSHTMLInfo( dhi.transmit_protocol == "HTTP" ) ;
    else
	info = new DODSInfo( dhi.transmit_protocol == "HTTP" ) ;
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
    DODSRequestHandlerList::Handler_citer i =
	DODSRequestHandlerList::TheList()->get_first_handler() ;
    DODSRequestHandlerList::Handler_citer ie =
	DODSRequestHandlerList::TheList()->get_last_handler() ;
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
	DODSRequestHandler *rh = (*i).second ;
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

    DODSRequestHandlerList::TheList()->execute_all( dhi ) ;

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
 * @see DODSHTMLInfo
 * @see DODSTransmitter
 * @see _DODSDataHandlerInterface
 */
void
HelpResponseHandler::transmit( DODSTransmitter *transmitter,
                               DODSDataHandlerInterface &dhi )
{
    if( _response )
    {
	DODSInfo *info = dynamic_cast<DODSInfo *>(_response) ;
	if( dhi.transmit_protocol == "HTTP" )
	    transmitter->send_html( *info, dhi ) ;
	else
	    transmitter->send_text( *info, dhi ) ;
    }
}

DODSResponseHandler *
HelpResponseHandler::HelpResponseBuilder( string handler_name )
{
    return new HelpResponseHandler( handler_name ) ;
}

// $Log: HelpResponseHandler.cc,v $
// Revision 1.6  2005/04/19 18:00:15  pwest
// added a newline at the end of the help output
//
// Revision 1.5  2005/04/07 19:55:17  pwest
// added add_data_from_file method to allow for information to be added from a file, for example when adding help information from a file
//
// Revision 1.4  2005/03/15 19:58:35  pwest
// using DODSTokenizer to get first and next tokens
//
// Revision 1.3  2005/02/01 17:48:17  pwest
//
// integration of ESG into opendap
//
// Revision 1.2  2004/09/09 17:17:12  pwest
// Added copywrite information
//
// Revision 1.1  2004/06/30 20:16:24  pwest
// dods dispatch code, can be used for apache modules or simple cgi script
// invocation or opendap daemon. Built during cedar server development.
//
