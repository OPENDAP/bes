// OPeNDAPDeleteCommand.cc

// 2004 Copyright University Corporation for Atmospheric Research

#include "OPeNDAPDeleteCommand.h"
#include "DODSTokenizer.h"
#include "DODSResponseHandlerList.h"
#include "DODSContainerPersistenceList.h"
#include "DODSParserException.h"
#include "OPeNDAPDataNames.h"

/** @brief parses the request to delete a container, a definition or all
 * definitions.
 *
 * Possible requests handled by this response handler are:
 *
 * delete container &lt;container_name&gt;;
 * <BR />
 * delete definition &lt;def_name&gt;;
 * <BR />
 * delete definitions;
 *
 * And remember to terminate all commands with a semicolon (;)
 *
 * @param tokenizer holds on to the list of tokens to be parsed
 * @param dhi structure that holds request and response information
 * @throws DODSParserException if there is a problem parsing the request
 * @see DODSTokenizer
 * @see _DODSDataHandlerInterface
 */
DODSResponseHandler *
OPeNDAPDeleteCommand::parse_request( DODSTokenizer &tokenizer,
                                     DODSDataHandlerInterface &dhi )
{
    string my_token = tokenizer.get_next_token() ;
    /* First we will make sure that the developer has not over-written this
     * command to work with a sub command. In other words, they have a new
     * command called "delete something". Look up delete.something
     */
    string newcmd = _cmd + "." + my_token ;
    OPeNDAPCommand *cmdobj = OPeNDAPCommand::find_command( newcmd ) ;
    if( cmdobj && cmdobj != OPeNDAPCommand::TermCommand )
    {
	return cmdobj->parse_request( tokenizer, dhi ) ;
    }

    /* If not a sub command then we have either a delete container, delete
     * definition or delete definitions. Continue...
     */
    dhi.action = _cmd ;
    DODSResponseHandler *retResponse =
	DODSResponseHandlerList::TheList()->find_handler( _cmd ) ;
    if( !retResponse )
    {
	throw DODSParserException( (string)"Improper command " + _cmd );
    }

    if( my_token == "container" )
    {
	dhi.data[CONTAINER_NAME] = tokenizer.get_next_token() ;
	if( dhi.data[CONTAINER_NAME] == ";" )
	{
	    tokenizer.parse_error( my_token + " not expected, expecting container name" ) ;
	}
	dhi.data[STORE_NAME] = PERSISTENCE_VOLATILE ;
	my_token = tokenizer.get_next_token() ;
	if( my_token == "from" )
	{
	    dhi.data[STORE_NAME] = tokenizer.get_next_token() ;
	    if( dhi.data[STORE_NAME] == ";" )
	    {
		tokenizer.parse_error( my_token + " not expected, expecting persistent name" ) ;
	    }
	    my_token = tokenizer.get_next_token() ;
	}
    }
    else if( my_token == "definition" )
    {
	dhi.data[DEF_NAME] = tokenizer.get_next_token() ;
	if( dhi.data[DEF_NAME] == ";" )
	{
	    tokenizer.parse_error( my_token + " not expected, expecting definition name" ) ;
	}
	my_token = tokenizer.get_next_token() ;
    }
    else if( my_token == "definitions" )
    {
	dhi.data[DEFINITIONS] = "true" ;
	my_token = tokenizer.get_next_token() ;
    }
    else
    {
	tokenizer.parse_error( my_token + ": invalid delete command" ) ;
    }

    if( my_token != ";" )
    {
	tokenizer.parse_error( my_token + " not expected, expecting semicolon (;)" ) ;
    }

    return retResponse ;
}

// $Log: OPeNDAPDeleteCommand.cc,v $
