// BESDelContainersCommand.cc

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

#include "BESDelContainersCommand.h"
#include "BESTokenizer.h"
#include "BESResponseHandlerList.h"
#include "BESContainerStorageList.h"
#include "BESSyntaxUserError.h"
#include "BESDataNames.h"

/** @brief parses the request to delete all containers from a specified
 * persistent store
 *
 * Request handled by this response handler:
 *
 * delete containers [from &lt;store_name&gt;];
 *
 * if the store name is not specified it defaults to volatile.
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
BESDelContainersCommand::parse_request( BESTokenizer &tokenizer,
                                     BESDataHandlerInterface &dhi )
{
    BESResponseHandler *retResponse =
	BESResponseHandlerList::TheList()->find_handler( _cmd ) ;
    if( !retResponse )
    {
	string s = (string)"No response handler for command " + _cmd ;
	throw BESSyntaxUserError( s, __FILE__, __LINE__ ) ;
    }

    dhi.action = _cmd ;
    dhi.data[STORE_NAME] = PERSISTENCE_VOLATILE ;
    string my_token = tokenizer.get_next_token() ;
    if( my_token == "from" )
    {
	dhi.data[STORE_NAME] = tokenizer.get_next_token() ;
	if( dhi.data[STORE_NAME] == ";" )
	{
	    tokenizer.parse_error( my_token + " not expected, expecting persistent name" ) ;
	}
	my_token = tokenizer.get_next_token() ;
    }

    if( my_token != ";" )
    {
	tokenizer.parse_error( my_token + " not expected, expecting semicolon (;)" ) ;
    }

    return retResponse ;
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance
 *
 * @param strm C++ i/o stream to dump the information to
 */
void
BESDelContainersCommand::dump( ostream &strm ) const
{
    strm << BESIndent::LMarg << "BESDelContainersCommand::dump - ("
			     << (void *)this << ")" << endl ;
    BESIndent::Indent() ;
    BESCommand::dump( strm ) ;
    BESIndent::UnIndent() ;
}

