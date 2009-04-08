// BESDeleteCommand.cc

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2004-2009 University Corporation for Atmospheric Research
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

#include <iostream>

using std::cout ;
using std::endl ;

#include "BESDeleteCommand.h"
#include "BESTokenizer.h"
#include "BESDataNames.h"

/** @brief parses the request to delete something
 *
 * This command requires a sub command to complete the parsing of the
 * command. You cannot just have a command to delete. For example, the
 * command 'delete container &lt;container_name&gt;;' looks for a sub
 * command with the name 'delete.container' to complete parsing the command.
 *
 * And remember to terminate all commands with a semicolon (;)
 *
 * @param tokenizer holds on to the list of tokens to be parsed
 * @param dhi structure that holds request and response information
 * @throws BESSyntaxUserError if there is a problem parsing the request
 * @see BESTokenizer
 * @see BESDataHandlerInterface
 */
BESResponseHandler *
BESDeleteCommand::parse_request( BESTokenizer &tokenizer,
                                     BESDataHandlerInterface &dhi )
{
    string my_token = parse_options( tokenizer, dhi ) ;
    /* First we will make sure that the developer has not over-written this
     * command to work with a sub command. In other words, they have a new
     * command called "delete something". Look up delete.something
     */
    string newcmd = _cmd + "." + my_token ;
    BESCommand *cmdobj = BESCommand::find_command( newcmd ) ;
    if( cmdobj && cmdobj != BESCommand::TermCommand )
    {
	return cmdobj->parse_request( tokenizer, dhi ) ;
    }

    /* No sub command, throw an exception. There should be a sub command for
     * set.
     */
    tokenizer.parse_error( my_token + " not expected, expecting sub command\n" ) ;

    return NULL ;
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance
 *
 * @param strm C++ i/o stream to dump the information to
 */
void
BESDeleteCommand::dump( ostream &strm ) const
{
    strm << BESIndent::LMarg << "BESDeleteCommand::dump - ("
			     << (void *)this << ")" << endl ;
    BESIndent::Indent() ;
    BESCommand::dump( strm ) ;
    BESIndent::UnIndent() ;
}

