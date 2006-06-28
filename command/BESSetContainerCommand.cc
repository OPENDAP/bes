// BESSetContainerCommand.cc

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

#include "BESSetContainerCommand.h"
#include "BESTokenizer.h"
#include "BESResponseHandlerList.h"
#include "BESContainerStorageList.h"
#include "BESParserException.h"
#include "BESDataNames.h"
#include "BESResponseNames.h"

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
BESSetContainerCommand::parse_request( BESTokenizer &tokenizer,
                                           BESDataHandlerInterface &dhi )
{
    string my_token ;

    /* No sub command, so proceed with the default set command
     */
    dhi.action = SETCONTAINER ;
    BESResponseHandler *retResponse =
	BESResponseHandlerList::TheList()->find_handler( SETCONTAINER ) ;
    if( !retResponse )
    {
	string s = (string)"No response handler for command " + SETCONTAINER ;
	throw BESParserException( s, __FILE__, __LINE__ ) ;
    }

    dhi.data[STORE_NAME] = PERSISTENCE_VOLATILE ;
    my_token = tokenizer.get_next_token() ;
    if( my_token != "values" )
    {
	if( my_token == "in" )
	{
	    dhi.data[STORE_NAME] = tokenizer.get_next_token() ;
	}
	else
	{
	    tokenizer.parse_error( my_token + " not expected\n" ) ;
	}
	my_token = tokenizer.get_next_token() ;
    }

    if( my_token == "values" )
    {
	dhi.data[SYMBOLIC_NAME] = tokenizer.get_next_token() ;
	my_token = tokenizer.get_next_token() ;
	if( my_token == "," )
	{
	    dhi.data[REAL_NAME] = tokenizer.get_next_token() ; 
	    my_token = tokenizer.get_next_token() ;
	    if( my_token == "," )
	    {
		dhi.data[CONTAINER_TYPE] = tokenizer.get_next_token() ;
		my_token = tokenizer.get_next_token() ;
		if( my_token != ";" )
		{
		    tokenizer.parse_error( my_token + " not expected\n" ) ;
		}
	    }
	    else
	    {
		dhi.data[CONTAINER_TYPE] = "" ;
		if( my_token != ";" )
		{
		    tokenizer.parse_error( my_token + " not expected\n" ) ;
		}
	    }
	}
	else
	{
	    tokenizer.parse_error( my_token + " not expected\n" ) ;
	}
    }
    else
    {
	tokenizer.parse_error( my_token + " not expected\n" ) ;
    }

    return retResponse ;
}

