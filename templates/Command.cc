// OPENDAP_CLASSOPENDAP_COMMANDCommand.cc

#include "OPENDAP_CLASSOPENDAP_COMMANDCommand.h"
#include "BESTokenizer.h"
#include "BESResponseHandlerList.h"
#include "BESParserException.h"
#include "OPENDAP_CLASSResponseNames.h"

BESResponseHandler *
OPENDAP_CLASSOPENDAP_COMMANDCommand::parse_request( BESTokenizer &tokenizer,
                                           BESDataHandlerInterface &dhi )
{
    string my_token ;

    /* No sub command, so proceed with the default command
     */
    dhi.action = OPENDAP_COMMAND_RESPONSE ;
    BESResponseHandler *retResponse =
	BESResponseHandlerList::TheList()->find_handler( OPENDAP_COMMAND_RESPONSE ) ;
    if( !retResponse )
    {
	string s = (string)"No response handler for command " + OPENDAP_COMMAND_RESPONSE ;
	throw BESParserException( s, __FILE__, __LINE__ ) ;
    }

    my_token = tokenizer.get_next_token() ;
    if( my_token == ";" )
    {
	tokenizer.parse_error( my_token + " not expected\n" ) ;
    }

    // Here is where your code would parse the tokens

    // Last token should be the terminating semicolon (;)
    my_token = tokenizer.get_next_token() ;
    if( my_token != ";" )
    {
	tokenizer.parse_error( my_token + " not expected\n" ) ;
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
OPENDAP_CLASSOPENDAP_COMMANDCommand::dump( ostream &strm ) const
{
    strm << BESIndent::LMarg << "OPENDAP_CLASSOPENDAP_COMMANDCommand::dump - ("
			     << (void *)this << ")" << endl ;
    BESIndent::Indent() ;
    BESCommand::dump( strm ) ;
    BESIndent::UnIndent() ;
}

