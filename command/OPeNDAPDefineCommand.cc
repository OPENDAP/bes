// OPeNDAPDefineCommand.cc

// 2004 Copyright University Corporation for Atmospheric Research

#include "OPeNDAPDefineCommand.h"
#include "DODSTokenizer.h"
#include "ThePersistenceList.h"
#include "TheResponseHandlerList.h"
#include "DODSParserException.h"
#include "OPeNDAPDataNames.h"

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
 * @throws DODSParserException if this method is called, as the request string
 * should have already been parsed.
 * @see DODSTokenizer
 * @see _DODSDataHandlerInterface
 */
DODSResponseHandler *
OPeNDAPDefineCommand::parse_request( DODSTokenizer &tokenizer,
                                     DODSDataHandlerInterface &dhi )
{
    string my_token = tokenizer.get_next_token() ;

    /* First we will make sure that the developer has not over-written this
     * command to work with a sub command. In other words, they have a new
     * command called "define something". Look up define.something
     */
    string newcmd = _cmd + "." + my_token ;
    OPeNDAPCommand *cmdobj = OPeNDAPCommand::find_command( newcmd ) ;
    if( cmdobj && cmdobj != OPeNDAPCommand::TermCommand )
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
    DODSResponseHandler *retResponse =
	TheResponseHandlerList->find_handler( _cmd ) ;
    if( !retResponse )
    {
	throw DODSParserException( (string)"Improper command " + _cmd );
    }

    bool with_aggregation = false ;

    dhi.data[DEF_NAME] = my_token ;

    my_token = tokenizer.get_next_token() ;
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
		DODSContainer d( my_token ) ;
		ThePersistenceList->look_for( d ) ;
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
				    dhi.container->set_constraint( tokenizer.remove_quotes( my_token ) ) ;
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

// $Log: OPeNDAPDefineCommand.cc,v $
