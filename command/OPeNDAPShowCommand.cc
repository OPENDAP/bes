// OPeNDAPShowCommand.cc

// 2004 Copyright University Corporation for Atmospheric Research

#include "OPeNDAPShowCommand.h"
#include "DODSTokenizer.h"
#include "TheResponseHandlerList.h"
#include "DODSParserException.h"

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
 * @throws DODSParserException if there is a problem parsing the request
 */
DODSResponseHandler *
OPeNDAPShowCommand::parse_request( DODSTokenizer &tokenizer,
                                   DODSDataHandlerInterface &dhi )
{
    string my_token = tokenizer.get_next_token() ;

    /* First we will make sure that the developer has not over-written this
     * command to work with a sub command. In other words, they have a new
     * command called "show something". Look up show.something
     */
    string newcmd = _cmd + "." + my_token ;
    OPeNDAPCommand *cmdobj = OPeNDAPCommand::find_command( newcmd ) ;
    if( cmdobj && cmdobj != OPeNDAPCommand::TermCommand )
    {
	cmdobj->parse_request( tokenizer, dhi ) ;
    }

    dhi.action = my_token ;
    DODSResponseHandler *retResponse =
	TheResponseHandlerList->find_handler( my_token ) ;
    if( !retResponse )
    {
	string err( "Command " ) ;
	err += _cmd + " " + my_token ;
	err += " does not have a registered response handler" ;
	throw DODSParserException( err ) ;
    }

    my_token = tokenizer.get_next_token() ;
    if( my_token != ";" )
    {
	tokenizer.parse_error( my_token + " not expected" ) ;
    }

    return retResponse ;
}

// $Log: OPeNDAPShowCommand.cc,v $
