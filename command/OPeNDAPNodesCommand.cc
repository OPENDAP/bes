// OPeNDAPNodesCommand.cc

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

#include "OPeNDAPNodesCommand.h"
#include "OPeNDAPTokenizer.h"
#include "DODSResponseHandlerList.h"
#include "OPeNDAPParserException.h"
#include "OPeNDAPDataNames.h"
#include "DODSResponseNames.h"

/** @brief knows how to parse a show request
 *
 * This class knows how to parse a show nodes request, building a sub response
 * handler that actually knows how to build the requested response
 * object.
 *
 * A show request looks like:
 *
 * show nodes [for &lt;node&gt;];
 *
 * where node is a node in the tree that the user wants look into further, a
 * node returned from a previous shoew nodes call.
 *
 * This parse method creates the sub response handler that knows how to create
 * the specified information.
 *
 * @param tokenizer holds on to the list of tokens to be parsed
 * @param dhi structure that holds request and response information
 * @throws OPeNDAPParserException if there is a problem parsing the request
 */
DODSResponseHandler *
OPeNDAPNodesCommand::parse_request( OPeNDAPTokenizer &tokenizer,
                                   DODSDataHandlerInterface &dhi )
{
    string my_token = tokenizer.get_next_token() ;
    if( my_token == "for" )
    {
	string node = tokenizer.get_next_token() ;
	if( node == ";" || node == "" )
	{
	    tokenizer.parse_error( node + " not expected" ) ;
	}
	dhi.data[NODE] = tokenizer.remove_quotes( node ) ;
	my_token = tokenizer.get_next_token() ;
    }
    if( my_token != ";" )
    {
	tokenizer.parse_error( my_token + " not expected" ) ;
    }

    dhi.data[ISLEAVES] = "false" ;
    dhi.action = CATALOG_RESPONSE ;
    DODSResponseHandler *retResponse =
	DODSResponseHandlerList::TheList()->find_handler( CATALOG_RESPONSE ) ;
    if( !retResponse )
    {
	string err( "Command " ) ;
	err += _cmd ;
	err += " does not have a registered response handler" ;
	throw OPeNDAPParserException( err ) ;
    }

    return retResponse ;
}

// $Log: OPeNDAPNodesCommand.cc,v $
