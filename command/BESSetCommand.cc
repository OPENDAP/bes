// BESSetCommand.cc

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

#include "BESSetCommand.h"
#include "BESTokenizer.h"
#include "BESResponseHandlerList.h"
#include "BESDataNames.h"

string
BESSetCommand::parse_options( BESTokenizer &tokens,
			          BESDataHandlerInterface &dhi )
{
    string my_token = tokens.get_next_token() ;
    if( my_token == "silently" || my_token == "silent" )
    {
	dhi.data[SILENT] = "yes" ;
	my_token = tokens.get_next_token() ;
    }
    else
    {
	dhi.data[SILENT] = "no" ;
    }
    return my_token ;
}

/** @brief parses the request to create a new container or replace an already
 * existing container given a symbolic name, a real name, and a data type.
 *
 * The syntax for a request handled by this response handler is:
 *
 * set container values * &lt;sym_name&gt;,&lt;real_name&gt;,&lt;data_type&gt;;
 *
 * The request must end with a semicolon and must contain the symbolic name,
 * the real name (in most cases a file name), and the type of data represented
 * by this container (e.g. cedar, netcdf, cdf, hdf, etc...).
 *
 * @param tokenizer holds on to the list of tokens to be parsed
 * @param dhi structure that holds request and response information
 * @throws BESParserException if there is a problem parsing the request
 * @see BESTokenizer
 * @see _BESDataHandlerInterface
 */
BESResponseHandler *
BESSetCommand::parse_request( BESTokenizer &tokenizer,
                                  BESDataHandlerInterface &dhi )
{
    string my_token = parse_options( tokenizer, dhi ) ;

    /* First we will make sure that the developer has not over-written this
     * command to work with a sub command. In other words, they have a new
     * command called "set something". Look up set.something
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
    tokenizer.parse_error( my_token + " not expected\n" ) ;

    return NULL ;
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance
 *
 * @param strm C++ i/o stream to dump the information to
 */
void
BESSetCommand::dump( ostream &strm ) const
{
    strm << BESIndent::LMarg << "BESSetCommand::dump - ("
			     << (void *)this << ")" << endl ;
    BESIndent::Indent() ;
    BESCommand::dump( strm ) ;
    BESIndent::UnIndent() ;
}

