// OPeNDAPTokenizer.h

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

#ifndef OPeNDAPTokenizer_h_
#define OPeNDAPTokenizer_h_ 1

#include <vector>
#include <string>

using std::vector ;
using std::string ;

/** @brief tokenizer for the OPeNDAP request command string

    OPeNDAPTokenizer tokenizes an OPeNDAP request command string, such as a get
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
class OPeNDAPTokenizer
{
private:
    vector <string>		tokens ;
    vector <string>::iterator	tokens_iterator ;
    int				_counter ;
    int				_number_tokens ;

public:
    				OPeNDAPTokenizer() ;
    				~OPeNDAPTokenizer();

    void			tokenize( const char *p ) ;
    string &			get_first_token() ;
    string &			get_current_token() ;
    string &			get_next_token() ;
    void			parse_error( const string &s = "" ) ;
    string			parse_container_name( const string &s,
                                                      unsigned int &type ) ;
    string			remove_quotes( const string &s ) ;

    void			dump_tokens() ;
} ;

#endif // OPeNDAPTokenizer_h_

// $Log: OPeNDAPTokenizer.h,v $
// Revision 1.3  2005/03/15 19:58:35  pwest
// using OPeNDAPTokenizer to get first and next tokens
//
// Revision 1.2  2005/02/10 19:27:45  pwest
// beginning quotes were being removed when tokenizing
//
// Revision 1.1  2005/02/01 17:48:17  pwest
//
// integration of ESG into opendap
//
