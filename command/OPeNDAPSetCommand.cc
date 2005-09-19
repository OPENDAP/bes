// OPeNDAPSetCommand.cc

// 2004 Copyright University Corporation for Atmospheric Research

#include "OPeNDAPSetCommand.h"
#include "DODSTokenizer.h"
#include "DODSResponseHandlerList.h"
#include "DODSContainerPersistenceList.h"
#include "DODSParserException.h"
#include "OPeNDAPDataNames.h"

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
 * @throws DODSParserException if there is a problem parsing the request
 * @see DODSTokenizer
 * @see _DODSDataHandlerInterface
 */
DODSResponseHandler *
OPeNDAPSetCommand::parse_request( DODSTokenizer &tokenizer,
                                  DODSDataHandlerInterface &dhi )
{
    string my_token = tokenizer.get_next_token() ;

    /* First we will make sure that the developer has not over-written this
     * command to work with a sub command. In other words, they have a new
     * command called "set something". Look up set.something
     */
    string newcmd = _cmd + "." + my_token ;
    OPeNDAPCommand *cmdobj = OPeNDAPCommand::find_command( newcmd ) ;
    if( cmdobj && cmdobj != OPeNDAPCommand::TermCommand )
    {
	return cmdobj->parse_request( tokenizer, dhi ) ;
    }

    /* No sub command, so proceed with the default set command
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
		    tokenizer.parse_error( my_token + " not expected\n" ) ;
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
    }
    else
    {
	tokenizer.parse_error( my_token + " not expected\n" ) ;
    }

    return retResponse ;
}

// $Log: OPeNDAPSetCommand.cc,v $
