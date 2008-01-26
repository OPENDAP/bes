// SampleSayCommand.cc

#include "SampleSayCommand.h"
#include "BESTokenizer.h"
#include "BESResponseHandlerList.h"
#include "BESSyntaxUserError.h"
#include "SampleResponseNames.h"

BESResponseHandler *
SampleSayCommand::parse_request( BESTokenizer &tokenizer,
                                           BESDataHandlerInterface &dhi )
{
    string my_token ;

    /* No sub command, so proceed with the default command
     */
    dhi.action = SAY_RESPONSE ;
    BESResponseHandler *retResponse =
	BESResponseHandlerList::TheList()->find_handler( SAY_RESPONSE ) ;
    if( !retResponse )
    {
	string s = (string)"No response handler for command " + SAY_RESPONSE ;
	throw BESSyntaxUserError( s, __FILE__, __LINE__ ) ;
    }

    my_token = tokenizer.get_next_token() ;
    if( my_token == ";" )
    {
	tokenizer.parse_error( my_token + " not expected\n" ) ;
    }

    // Here is where your code would parse the tokens
    dhi.data[SAY_WHAT] = my_token ;

    // Next token should be the token "to"
    my_token = tokenizer.get_next_token() ;
    if( my_token != "to" )
    {
	tokenizer.parse_error( my_token + " not expected\n" ) ;
    }

    // Next token should be what is being said
    my_token = tokenizer.get_next_token() ;
    if( my_token == ";" )
    {
	tokenizer.parse_error( my_token + " not expected\n" ) ;
    }
    dhi.data[SAY_TO] = my_token ;

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
SampleSayCommand::dump( ostream &strm ) const
{
    strm << BESIndent::LMarg << "SampleSayCommand::dump - ("
			     << (void *)this << ")" << endl ;
    BESIndent::Indent() ;
    BESCommand::dump( strm ) ;
    BESIndent::UnIndent() ;
}

