// BESCmdParser.cc

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
 
// (c) COPYRIGHT University Corporation for Atmospheric Research 2004-2005
// Please read the full copyright statement in the file COPYRIGHT_UCAR.
//
// Authors:
//      pwest       Patrick West <pwest@ucar.edu>
//      jgarcia     Jose Garcia <jgarcia@ucar.edu>

#include "BESCmdParser.h"
#include "BESTokenizer.h"
#include "BESCommand.h"
#include "BESResponseHandler.h"
#include "BESSyntaxUserError.h"

BESCmdParser::BESCmdParser( )
{
}

BESCmdParser::~BESCmdParser()
{
}

/** @brief parse the request string and build the execution plan for the
 * request.
 *
 * Parse the request string into a list of tokens using the BESTokenizer
 * object and builds the execution plan for the request. This plan includes
 * the type of response object that is being requested.
 *
 * @param request string representing the request from the client
 * @param dhi information needed to build the request and to store request
 * information for the server
 * @throws BESSyntaxUserError thrown if there is an error in syntax
 * @see BESTokenizer
 * @see BESResponseHandler
 * @see BESDataHandlerInterface
 */
void
BESCmdParser::parse( const string &request, BESDataHandlerInterface &dhi )
{
    BESTokenizer t ;
    t.tokenize( request.c_str() ) ;
    string my_token = t.get_first_token() ;
    BESCommand *cmd = BESCommand::find_command( my_token ) ;
    if( cmd )
    {
	dhi.response_handler = cmd->parse_request( t, dhi ) ;
	if( !dhi.response_handler )
	{
	    string err = "Problem parsing the request command" ;
	    throw BESSyntaxUserError( err, __FILE__, __LINE__ ) ;
	}
    }
    else
    {
	string err = "Invalid command " + my_token ;
	throw BESSyntaxUserError( err, __FILE__, __LINE__ ) ;
    }
}

