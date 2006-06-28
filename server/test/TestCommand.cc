// TestCommand.cc

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

#include "TestCommand.h"
#include "BESTokenizer.h"
#include "BESResponseHandlerList.h"
#include "BESParserException.h"

/** @brief parses the request to build a test signal response
 *
 * A request looks like:
 *
 * test sig;
 *
 * @param tokenizer holds on to the list of tokens to be parsed
 * @param dhi structure that holds request and response information
 * @throws BESParserException if there is a problem parsing the command
 * @see BESTokenizer
 * @see _BESDataHandlerInterface
 */
BESResponseHandler *
TestCommand::parse_request( BESTokenizer &tokenizer,
			    BESDataHandlerInterface &dhi )
{
    string my_token = parse_options( tokenizer, dhi ) ;

    /* First we will make sure that the developer has not over-written this
     * command to work with a sub command. In other words, they have a new
     * command called "define something". Look up define.something
     */
    string newcmd = _cmd + "." + my_token ;
    BESCommand *cmdobj = BESCommand::find_command( newcmd ) ;
    if( cmdobj && cmdobj != BESCommand::TermCommand )
    {
	return cmdobj->parse_request( tokenizer, dhi ) ;
    }

    /* No sub-command to define, so parse the test sig command.
     */

    /* Look for the response handler that knows how to build the response
     * object for a test sig command.
     */
    dhi.action = my_token ;
    BESResponseHandler *retResponse =
	BESResponseHandlerList::TheList()->find_handler( my_token ) ;
    if( !retResponse )
    {
	string s = (string)"No response handler for command " + my_token ;
	throw BESParserException( s, __FILE__, __LINE__ ) ;
    }

    return retResponse ;
}

