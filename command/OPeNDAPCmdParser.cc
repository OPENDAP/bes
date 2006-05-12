// OPeNDAPCmdParser.cc

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2004,2005 University Corporation for Atmospheric Research
// Author: Patrick West <pwest@ucar.org> and Jose Garcia <jgarcia@ucar.org>
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

#include "OPeNDAPCmdParser.h"
#include "OPeNDAPTokenizer.h"
#include "OPeNDAPCommand.h"
#include "DODSResponseHandler.h"
#include "OPeNDAPParserException.h"

OPeNDAPCmdParser::OPeNDAPCmdParser( )
{
}

OPeNDAPCmdParser::~OPeNDAPCmdParser()
{
}

/** @brief parse the request string and build the execution plan for the
 * request.
 *
 * Parse the request string into a list of tokens using the OPeNDAPTokenizer
 * object and builds the execution plan for the request. This plan includes
 * the type of response object that is being requested.
 *
 * @param request string representing the request from the client
 * @param dhi information needed to build the request and to store request
 * information for the server
 * @throws OPeNDAPParserException thrown if there is an error in syntax
 * @see OPeNDAPTokenizer
 * @see DODSResponseHandler
 * @see _DODSDataHandlerInterface
 */
void
OPeNDAPCmdParser::parse( const string &request, DODSDataHandlerInterface &dhi )
{
    OPeNDAPTokenizer t ;
    t.tokenize( request.c_str() ) ;
    string my_token = t.get_first_token() ;
    OPeNDAPCommand *cmd = OPeNDAPCommand::find_command( my_token ) ;
    if( cmd )
    {
	dhi.response_handler = cmd->parse_request( t, dhi ) ;
	if( !dhi.response_handler )
	{
	    throw OPeNDAPParserException( (string)"Unable to build command" ) ;
	}
    }
    else
    {
	throw OPeNDAPParserException( (string)"Invalid command " + my_token ) ;
    }
}

// $Log: OPeNDAPCmdParser.cc,v $
// Revision 1.5  2005/03/15 19:58:35  pwest
// using OPeNDAPTokenizer to get first and next tokens
//
// Revision 1.4  2005/02/01 17:48:17  pwest
//
// integration of ESG into opendap
//
// Revision 1.3  2004/12/15 17:36:01  pwest
//
// Changed the way in which the parser retrieves container information, going
// instead to ThePersistenceList, which goes through the list of container
// persistence instances it has.
//
// Revision 1.2  2004/09/09 17:17:12  pwest
// Added copywrite information
//
// Revision 1.1  2004/06/30 20:16:24  pwest
// dods dispatch code, can be used for apache modules or simple cgi script
// invocation or opendap daemon. Built during cedar server development.
//
