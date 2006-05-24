// OPeNDAPShowCommand.cc

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

#include "OPeNDAPShowCommand.h"
#include "OPeNDAPTokenizer.h"
#include "DODSResponseHandlerList.h"
#include "OPeNDAPParserException.h"

/** @brief knows how to parse a show request
 *
 * This class knows how to parse a show request, building a sub response
 * handler that actually knows how to build the requested response
 * object, such as for show help or show version.
 *
 * A show request looks like:
 *
 * get &lt;info_type&gt;;
 *
 * where info_type is the type of information that the user is requesting,
 * such as help or version
 *
 * This parse method creates the sub response handler that knows how to create
 * the specified information, such as creating HelpResponseHandler.
 *
 * @param tokenizer holds on to the list of tokens to be parsed
 * @param dhi structure that holds request and response information
 * @throws OPeNDAPParserException if there is a problem parsing the request
 */
DODSResponseHandler *
OPeNDAPShowCommand::parse_request( OPeNDAPTokenizer &tokenizer,
                                   DODSDataHandlerInterface &dhi )
{
    DODSResponseHandler *retResponse = 0 ;

    string my_token = parse_options( tokenizer, dhi ) ;

    /* First we will make sure that the developer has not over-written this
     * command to work with a sub command. In other words, they have a new
     * command called "show something". Look up show.something
     */
    string newcmd = _cmd + "." + my_token ;
    OPeNDAPCommand *cmdobj = OPeNDAPCommand::find_command( newcmd ) ;
    if( cmdobj && cmdobj != OPeNDAPCommand::TermCommand )
    {
	retResponse = cmdobj->parse_request( tokenizer, dhi ) ;
    }
    else
    {
	dhi.action = newcmd ;
	retResponse =
	    DODSResponseHandlerList::TheList()->find_handler( newcmd ) ;
	if( !retResponse )
	{
	    string err( "Command " ) ;
	    err += _cmd + " " + my_token ;
	    err += " does not have a registered response handler" ;
	    throw OPeNDAPParserException( err ) ;
	}

	my_token = tokenizer.get_next_token() ;
	if( my_token != ";" )
	{
	    tokenizer.parse_error( my_token + " not expected" ) ;
	}
    }

    return retResponse ;
}

