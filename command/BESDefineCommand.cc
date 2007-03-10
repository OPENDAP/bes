// BESDefineCommand.cc

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

#include "BESDefineCommand.h"
#include "BESTokenizer.h"
#include "BESContainerStorageList.h"
#include "BESResponseHandlerList.h"
#include "BESParserException.h"
#include "BESDataNames.h"
#include "BESUtil.h"

string
BESDefineCommand::parse_options( BESTokenizer &tokens,
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

/** @brief parses the request to build a definition that can be used in other
 * requests, such as get commands.
 *
 * A request looks like:
 *
 * define &lt;def_name&gt; as &lt;container_list&gt;
 * <BR />
 * &nbsp;&nbsp;[where &lt;container_x&gt;.constraint="&lt;constraint&gt;"]
 * <BR />
 * &nbsp;&nbsp;[,&lt;container_x&gt;.attributes="&lt;attrs&gt;"]
 * <BR />
 * &nbsp;&nbsp;[aggregate by "&lt;aggregation_command&gt;"];
 *
 * where container_list is a list of containers representing points of data,
 * such as a file. For each container in the container_list the user can
 * specify a constraint and a list of attributes. You need not specify a
 * constraint for a given container or a list of attributes. If just
 * specifying a constraint then leave out the attributes. If just specifying a
 * list of attributes then leave out the constraint. For example:
 *
 * define d1 as container_1,container_2
 * <BR />
 * &nbsp;&nbsp;where container_1.constraint="constraint1"
 * <BR />
 * &nbsp;&nbsp;,container_2.constraint="constraint2"
 * <BR />
 * &nbsp;&nbsp;,container_2.attributes="attr1,attr2";
 *
 * @param tokenizer holds on to the list of tokens to be parsed
 * @param dhi structure that holds request and response information
 * @throws BESParserException if this method is called, as the request string
 * should have already been parsed.
 * @see BESTokenizer
 * @see _BESDataHandlerInterface
 */
BESResponseHandler *
BESDefineCommand::parse_request( BESTokenizer &tokenizer,
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

    /* No sub-command to define, so the define command looks like:
     * define name as sym1,sym2,...,symn with ... aggregate by ...
     * 
     * no return as
     */

    /* Look for the response handler that knows how to build the response
     * object for a define command.
     */
    dhi.action = _cmd ;
    BESResponseHandler *retResponse =
	BESResponseHandlerList::TheList()->find_handler( _cmd ) ;
    if( !retResponse )
    {
	string s = (string)"No response handler for command " + _cmd ;
	throw BESParserException( s, __FILE__, __LINE__ ) ;
    }

    bool with_aggregation = false ;

    dhi.data[DEF_NAME] = my_token ;

    my_token = tokenizer.get_next_token() ;
    if( my_token == "in" )
    {
	string store_name = tokenizer.get_next_token() ;
	if( store_name == ";" || store_name == "" )
	{
	    tokenizer.parse_error( my_token + " not expected, expecting definition store name" ) ;
	}
	dhi.data[STORE_NAME] = store_name ;
	my_token = tokenizer.get_next_token() ;
    }

    if( my_token != "as" )
    {
	tokenizer.parse_error( my_token + " not expected, expecting \"as\"" ) ;
    }
    else
    {
	my_token = tokenizer.get_next_token() ;
	bool expecting_comma = false ;
	bool with_proyection = false ;
	if( my_token == ";" )
	    tokenizer.parse_error( my_token + " not expected, expecting list of symbolic names\n" ) ;
	while( ( my_token != "with" ) && ( my_token!=";" ) )
	{
	    if( ( my_token == "," ) && ( !expecting_comma ) )
		tokenizer.parse_error( my_token + " not expected\n" ) ;
	    else if( ( my_token == "," ) && ( expecting_comma ) )
		expecting_comma = false ;
	    else if( ( my_token != "," ) && ( expecting_comma ) )
		tokenizer.parse_error( my_token + " not expected\n" ) ;
	    else
	    {
		BESContainer d( my_token ) ;
		BESContainerStorageList::TheList()->look_for( d ) ;
		dhi.containers.push_back( d ) ;
		expecting_comma = true ;
	    }
	    my_token = tokenizer.get_next_token() ;
	    if( my_token == "with" )
		with_proyection = true ;
	}
	if( !expecting_comma )
	    tokenizer.parse_error( my_token + " not expected\n" ) ;
	else
	    expecting_comma = false ;
	if( with_proyection )
	{
	    my_token = tokenizer.get_next_token() ;
	    if( my_token == ";" )
		tokenizer.parse_error( my_token + " not expected\n" ) ;
	    else
	    {
		int rat = 0 ;
		bool need_constraint = false ;
		int where_in_list = 0 ;
		bool found = false ;
		unsigned int my_type = 0 ;
		while( my_token != "aggregate" && my_token != ";" )
		{
		    if( ( my_token == "," ) && ( !expecting_comma ) )
			tokenizer.parse_error( my_token + " not expected\n" ) ;
		    else if( ( my_token == "," ) && ( expecting_comma ) )
			expecting_comma = false ;
		    else if( ( my_token != "," ) && ( expecting_comma ) )
			tokenizer.parse_error( my_token + " not expected\n" ) ;
		    else
		    {
			rat++ ;
			switch( rat )
			{
			    case 1:
			    {
				my_type = 0 ;
				string ds = tokenizer.parse_container_name( my_token, my_type ) ;
				found = false ;
				dhi.first_container() ;
				where_in_list = 0 ;
				while( dhi.container && !found )
				{ 
				    if( ds == dhi.container->get_symbolic_name() )
				    {
					found = true ;
				    }
				    dhi.next_container() ;
				    where_in_list++ ;
				}
				if( !found )
				    tokenizer.parse_error( "Container " + ds + " is in the proyection but is not in the selection." ) ;
				need_constraint = true ;
				break ;
			    }
			    case 2:
			    {
				expecting_comma = true ;
				rat = 0 ;
				need_constraint = false ;
				dhi.first_container() ;
				for( int w = 0; w < where_in_list-1 ; w++ )
				{
				    dhi.next_container() ;
				}
				if( my_type == 1 )
				{
				    dhi.container->set_constraint( BESUtil::www2id( tokenizer.remove_quotes( my_token ) ) ) ;
				}
				else if( my_type == 2 )
				{
				    dhi.container->set_attributes( tokenizer.remove_quotes( my_token ) ) ;
				}
				else
				{
				    tokenizer.parse_error( "Unknown property type for container" + dhi.container->get_symbolic_name() ) ;
				}
				break;
			    }
			}
		    }
		    my_token = tokenizer.get_next_token() ;
		    if( my_token == "aggregate" )
			with_aggregation = true ;
		}
		if( need_constraint )
		    tokenizer.parse_error( "; not expected" ) ;
	    }
	}
	if( with_aggregation == true )
	{
	    my_token = tokenizer.get_next_token() ;
	    if( my_token != "using" )
	    {
		tokenizer.parse_error( my_token + " not expected" ) ;
	    }

	    my_token = tokenizer.get_next_token() ;
	    if( my_token == ";" )
	    {
		tokenizer.parse_error( my_token + " not expected" ) ;
	    }
	    dhi.data[AGG_HANDLER] = my_token ;

	    my_token = tokenizer.get_next_token() ;
	    if( my_token != "by" )
	    {
		tokenizer.parse_error( my_token + " not expected" ) ;
	    }

	    my_token = tokenizer.get_next_token() ;
	    if( my_token == ";" )
	    {
		tokenizer.parse_error( my_token + " not expected" ) ;
	    }
	    dhi.data[AGG_CMD] =
		tokenizer.remove_quotes( my_token ) ;

	    my_token = tokenizer.get_next_token() ;
	}
	if( my_token != ";" )
	{
	    tokenizer.parse_error( my_token + " not expected" ) ;
	}
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
BESDefineCommand::dump( ostream &strm ) const
{
    strm << BESIndent::LMarg << "BESDefineCommand::dump - ("
			     << (void *)this << ")" << endl ;
    BESIndent::Indent() ;
    BESCommand::dump( strm ) ;
    BESIndent::UnIndent() ;
}

