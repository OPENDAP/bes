// DODSTokenizer.cc

// 2004 Copyright University Corporation for Atmospheric Research

#include <iostream>

using std::cout ;
using std::endl ;

#include "DODSTokenizer.h"
#include "DODSParserException.h"
#include "DODSMemoryException.h"

DODSTokenizer::DODSTokenizer( )
    : _counter( -1 )
{
}

DODSTokenizer::~DODSTokenizer()
{
}

/** @brief throws an exception giving the tokens up to the point of the
 * problem
 *
 * Throws an excpetion using the passed error string, and adding to it the
 * tokens that have been read leading up to the point that this method is
 * called.
 *
 * @param s error string passed by caller to be display with list of
 * tokens
 * @throws DODSException with the passed error string as well as all tokens
 * leading up to the error.
 */
void
DODSTokenizer::parse_error( const string &s )
{
    string error = "Parse error." ;
    string where = "" ;
    if( _counter >= 0 )
    {
	for( int w = 0; w < _counter+1; w++ )
	    where += tokens[w] + " " ;
	where += "<----HERE IS THE ERROR" ;
	error += "\n" + where ;
    }
    if( s != "" )
	error += "\n" + s ;
    DODSParserException pe ;
    pe.set_error_description( error ) ;
    throw pe ;
}

/** @brief returns the first token from the token list
 *
 * This method will return the first token from the token list. Whether the
 * caller has accessed tokens already or not, the caller is returned to the
 * beginning of the list.
 *
 * @returns the first token in the token list
 */
string &
DODSTokenizer::get_first_token()
{
    _counter = 0 ;
    return tokens[_counter] ;
}

/** @brief returns the next token from the token list
 *
 * Returns the next token from the token list and returns it to the caller.
 *
 * @returns next token
 * @throws DODSException if the first token has not yet been retrieved or if
 * the caller is attempting to access more tokens than are available.
 * @see DODSException
 */
string &
DODSTokenizer::get_next_token()
{
    if( _counter == -1 )
    {
	parse_error( "incomplete expression!" ) ;
    }

    if( _counter >= _number_tokens-1 )
    {
	parse_error( "incomplete expression!" ) ;
    }

    return tokens[++_counter] ;
}

/** @brief tokenize the OPeNDAP request/command string
 *
 *  DODSTokenizer tokenizes an OPeNDAP request command string, such as a get
 *  request, a define command, a set command, etc... Tokens are separated by
 *  the following characters:
 *
 *  '"'
 *  ' '
 *  '\n'
 *  0x0D
 *  0x0A
 *
 *  When the tokenizer sees a double quote it then finds the next double
 *  quote and includes all test between the quotes, and including the
 *  quotes, as a single token.
 *
 * @param p request command string
 * @throws DODSException if quoted text is missing an end quote, if the
 * request string is not terminated by a semiclon, if the number of tokens
 * is less than 2.
 * @see DODSException
 */
void
DODSTokenizer::tokenize( const char *p )
{
    int len = strlen( p ) ;
    string s = "" ;
    bool passing_raw = false ;
    for( int j = 0; j < len; j++ )
    {
	if( p[j] == '\"' )
	{
	    if( s != "" )
	    {
		if( passing_raw )
		{
		    s += "\"" ;
		    tokens.push_back( s ) ;
		    s = "" ;
		}
		else
		{
		    tokens.push_back( s ) ;
		    s = "\"" ;
		}
	    }
	    else
	    {
		s += "\"" ;
	    }
	    passing_raw =! passing_raw ;
	}
	else if( passing_raw )
	{
	    s += p[j] ;
	}
	else
	{
	    if( ( p[j] == ' ' ) ||
	        ( p[j] == '\n' ) ||
		( p[j] == 0x0D ) ||
		( p[j] == 0x0A ) )
	    {
		if( s != "" )
		{
		    tokens.push_back( s ) ;
		    s = "" ;
		}
	    }
	    else if( ( p[j] == ',' ) || ( p[j] == ';' ) )
	    {
		if( s!= "" )
		{
		    tokens.push_back( s ) ;
		    s = "" ;
		}
		switch( p[j] )
		{
		    case ',':
			tokens.push_back( "," ) ;
			break;
		    case ';':
			tokens.push_back( ";" ) ;
			break;
		}
	    }
	    else
	    s += p[j] ;
	}
    }

    if( s != "" )
	tokens.push_back( s ) ;
    _number_tokens = tokens.size() ;
    if( passing_raw )
	parse_error( "Unclose quote found.(\")" ) ;
    if( _number_tokens < 2 )
	parse_error( "Unknown command: '" + (string)p + (string)"'") ;
    if( tokens[_number_tokens - 1] != ";" )
	parse_error( "The request must be terminated by a semicolon (;)" ) ;
}

/** @brief parses a container name for constraint and attributes
 *
 * This method parses a string that contains either a constraint for a
 * container or attributes for a container. If it is a constraint then the
 * type is set to 1, if it is attributes for the container then type is set
 * to 2. The syntax should look like the following.
 *
 * &lt;container_name&gt;.constraint=
 * or
 * &lt;container_name&gt;.attributes=
 *
 * The equal sign must be present.
 *
 * @param s the string to be parsed to determine if constraint or
 * attributes
 * @param type set to 1 if constraint or 2 if attributes
 * @returns the container name
 * @throws DODSException if the syntax is incorrect
 * @see DODSException
 */
string
DODSTokenizer::parse_container_name( const string &s, unsigned int &type )
{
    int where = s.rfind( ".constraint=", s.size() ) ;
    if( where < 0 )
    {
	where = s.rfind( ".attributes=", s.size() ) ;
	if( where < 0 )
	{
	    parse_error( "Expected property declaration." ) ;
	}
	else
	{
	    type = 2 ;
	}
    }
    else
    {
	type = 1 ;
    }
    string valid = s.substr( where, s.size() ) ;
    if( (valid != ".constraint=") && (valid != ".attributes=") )
    {
	string err = (string)"Invalid container property "
	             + valid
		     + " for container "
		     + s.substr( 0, where ) ;
	parse_error( err ) ;
    }
    return s.substr( 0, where ) ;
}

/** @brief removes quotes from a quoted token
 *
 * Removes quotes from a quoted token. The passed string must begin with a
 * double quote and must end with a double quote.
 *
 * @param s string where the double quotes are removed.
 * @returns the unquoted token
 * @throws DODSException if the string does not begin and end with a double
 * quote
 * @see DODSException
 */
string
DODSTokenizer::remove_quotes( const string &s )
{
    if( (s[0] != '"' ) || (s[s.size() - 1] != '"' ) )
    {
	parse_error( "item " + s + " must be enclosed by quotes" ) ;
    }
    return s.substr( 1, s.size() - 2 ) ;
}

/** @brief dump the tokens that have been tokenized in the order in which
 * they are parsed.
 *
 * Dumps to standard out the tokens that were parsed from the
 * request/command string. If the tokens were quoted, the quotes are kept
 * in. The tokens are all displayed with double quotes around them to show
 * if there are any spaces or special characters in the token.
 */
void
DODSTokenizer::dump_tokens()
{
    for( tokens_iterator = tokens.begin();
	 tokens_iterator != tokens.end();
	 tokens_iterator++ )
    {
	cout << "\"" << (*tokens_iterator) << "\"" << endl ;
    }
}

// $Log: DODSTokenizer.cc,v $
// Revision 1.3  2005/03/15 19:58:35  pwest
// using DODSTokenizer to get first and next tokens
//
// Revision 1.2  2005/02/10 19:27:35  pwest
// beginning quotes were being removed when tokenizing
//
// Revision 1.1  2005/02/01 17:48:17  pwest
//
// integration of ESG into opendap
//
