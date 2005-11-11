// NodesResponseHandler.cc

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

#include "NodesResponseHandler.h"
#include "DODSTextInfo.h"
#include "cgi_util.h"
#include "DODSRequestHandlerList.h"
#include "DODSRequestHandler.h"
#include "DODSParserException.h"
#include "DODSTokenizer.h"
#include "OPeNDAPDataNames.h"

NodesResponseHandler::NodesResponseHandler( string name )
    : DODSResponseHandler( name )
{
}

NodesResponseHandler::~NodesResponseHandler( )
{
}

/** @brief executes the command 'show nodes [for &lt;node&gt;];' by returning
 * nodes at the top level or at the specified node.
 *
 * If no node is specified then return the data types (request handlers)
 * handled by this server. If a node is specified, then the first level of
 * the node is the data type. Find the request handler for that data type
 * and hand off the request to that request handler.
 *
 * The response object DODSTextInfo is created to store the information.
 *
 * @param dhi structure that holds request and response information
 * @throws DODSResponseException if there is a problem building the
 * response object
 * @see _DODSDataHandlerInterface
 * @see DODSTextInfo
 * @see DODSRequestHandlerList
 */
void
NodesResponseHandler::execute( DODSDataHandlerInterface &dhi )
{
    DODSTextInfo *info = new DODSTextInfo( dhi.transmit_protocol == "HTTP" ) ;
    _response = info ;

    string node = dhi.data[NODE] ;
    if( node == "" )
    {
	// if no node is specified then the nodes are the list of data types
	// handled by this server, the names of the request handlers. Only
	// return the data types that handle nodes and leaves requests.
	DODSRequestHandlerList::Handler_citer i =
	    DODSRequestHandlerList::TheList()->get_first_handler() ;
	DODSRequestHandlerList::Handler_citer ie =
	    DODSRequestHandlerList::TheList()->get_last_handler() ;
	for( ; i != ie; i++ ) 
	{
	    info->add_data( "<showNodes>\n" ) ;
	    info->add_data( "    <response>\n" ) ;
	    DODSRequestHandler *rh = (*i).second ;
	    p_request_handler p = rh->find_handler( get_name() ) ;
	    if( p )
	    {
		info->add_data( "        <node>\n" ) ;
		info->add_data( (string)"            <name>"
		                + rh->get_name()
				+ "</name>\n" ) ;
		info->add_data( "        </node>\n" ) ;
	    }
	    info->add_data( "    </response>\n" ) ;
	    info->add_data( "</showNodes>\n" ) ;
	}
    }
    else
    {
	// if there is a node specified then the first name in the path is
	// the name of the data type being requested. Pass off the request
	// to this request handler.
	string rh_name ;
	std::string::size_type slash = node.find( "/" ) ;
	if( slash != string::npos )
	{
	    rh_name = node.substr( 0, slash-1 ) ;
	}
	else
	{
	    rh_name = node ;
	}
	DODSRequestHandler *rh =
	    DODSRequestHandlerList::TheList()->find_handler( rh_name ) ;
	if( rh )
	{
	    p_request_handler p = rh->find_handler( get_name() ) ;
	    if( p )
	    {
		p( dhi ) ;
	    }
	    else
	    {
		cerr << "Could not find the node " << node << endl ;
	    }
	}
	else
	{
	    cerr << "Could not find the node " << node << endl ;
	}
    }
}

/** @brief transmit the response object built by the execute command
 * using the specified transmitter object
 *
 * If a response object was built then transmit it as text
 *
 * @param transmitter object that knows how to transmit specific basic types
 * @param dhi structure that holds the request and response information
 * @see DODSTextInfo
 * @see DODSTransmitter
 * @see _DODSDataHandlerInterface
 */
void
NodesResponseHandler::transmit( DODSTransmitter *transmitter,
                               DODSDataHandlerInterface &dhi )
{
    if( _response )
    {
	DODSTextInfo *info = dynamic_cast<DODSTextInfo *>(_response) ;
	transmitter->send_text( *info, dhi ) ;
    }
}

DODSResponseHandler *
NodesResponseHandler::NodesResponseBuilder( string handler_name )
{
    return new NodesResponseHandler( handler_name ) ;
}

// $Log: NodesResponseHandler.cc,v $
