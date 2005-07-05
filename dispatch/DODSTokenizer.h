// DODSTokenizer.h

// 2004 Copyright University Corporation for Atmospheric Research

#ifndef DODSTokenizer_h_
#define DODSTokenizer_h_ 1

#include <vector>
#include <string>

using std::vector ;
using std::string ;

/** @brief tokenizer for the OPeNDAP request command string

    DODSTokenizer tokenizes an OPeNDAP request command string, such as a get
    request, a define command, a set command, etc... Tokens are separated by
    the following characters:

    '"'
    ' '
    '\n'
    0x0D
    0x0A

    When the tokenizer sees a double quote it then finds the next double
    quote and includes all test between the quotes, and including the
    quotes, as a single token.

    If there is any problem in the syntax of the request or command then you
    can use the method parse_error, which will display all of the tokens up
    to the point of the error.

    If the user of the tokenizer attempts to access the next token before
    the first token is accessed, a DODSExcpetion is thrown.

    If the user of the tokenizer attempts to access more tokens than were
    read in, a DODSException is thrown.

    @see DODSException
 */
class DODSTokenizer
{
private:
    vector <string>		tokens ;
    vector <string>::iterator	tokens_iterator ;
    int				_counter ;
    int				_number_tokens ;

public:
    				DODSTokenizer() ;
    				~DODSTokenizer();

    void			tokenize( const char *p ) ;
    string &			get_first_token() ;
    string &			get_next_token() ;
    void			parse_error( const string &s = "" ) ;
    string			parse_container_name( const string &s,
                                                      unsigned int &type ) ;
    string			remove_quotes( const string &s ) ;

    void			dump_tokens() ;
} ;

#endif // DODSTokenizer_h_

// $Log: DODSTokenizer.h,v $
// Revision 1.3  2005/03/15 19:58:35  pwest
// using DODSTokenizer to get first and next tokens
//
// Revision 1.2  2005/02/10 19:27:45  pwest
// beginning quotes were being removed when tokenizing
//
// Revision 1.1  2005/02/01 17:48:17  pwest
//
// integration of ESG into opendap
//
