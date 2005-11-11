// LeavesResponseHandler.cc

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

#include "LeavesResponseHandler.h"
#include "DODSTextInfo.h"
#include "cgi_util.h"
#include "DODSRequestHandlerList.h"
#include "DODSRequestHandler.h"
#include "DODSParserException.h"
#include "DODSTokenizer.h"
#include "OPeNDAPDataNames.h"

LeavesResponseHandler::LeavesResponseHandler( string name )
    : DODSResponseHandler( name )
{
}

LeavesResponseHandler::~LeavesResponseHandler( )
{
}

/** @brief executes the command 'show leaves for &lt;node&gt;;' by returning
 * the leaves at the specified node.
 *
 * The LeavesResponseHandler first determines what data type is being
 * requested then passes off the request to that request handler.
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
LeavesResponseHandler::execute( DODSDataHandlerInterface &dhi )
{
    DODSTextInfo *info = new DODSTextInfo( dhi.transmit_protocol == "HTTP" ) ;
    _response = info ;

    string node = dhi.data[NODE] ;
    if( node == "" )
    {
	// if no node is specified then nothing is returned.
	cerr << "No node specified in the show leaves request" << endl ;
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
LeavesResponseHandler::transmit( DODSTransmitter *transmitter,
                               DODSDataHandlerInterface &dhi )
{
    if( _response )
    {
	DODSTextInfo *info = dynamic_cast<DODSTextInfo *>(_response) ;
	transmitter->send_text( *info, dhi ) ;
    }
}

DODSResponseHandler *
LeavesResponseHandler::LeavesResponseBuilder( string handler_name )
{
    return new LeavesResponseHandler( handler_name ) ;
}

// $Log: LeavesResponseHandler.cc,v $
