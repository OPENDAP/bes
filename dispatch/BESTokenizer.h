// BESTokenizer.h

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2004-2009 University Corporation for Atmospheric Research
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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//
// You can contact University Corporation for Atmospheric Research at
// 3080 Center Green Drive, Boulder, CO 80301

// (c) COPYRIGHT University Corporation for Atmospheric Research 2004-2005
// Please read the full copyright statement in the file COPYRIGHT_UCAR.
//
// Authors:
//      pwest       Patrick West <pwest@ucar.edu>
//      jgarcia     Jose Garcia <jgarcia@ucar.edu>

#ifndef BESTokenizer_h_
#define BESTokenizer_h_ 1

#include <string>
#include <vector>

#include "BESObj.h"

/** @brief tokenizer for the BES request command string

 BESTokenizer tokenizes an BES request command string, such as a get
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
 the first token is accessed, a BESExcpetion is thrown.

 If the user of the tokenizer attempts to access more tokens than were
 read in, an exception is thrown.
 */
class BESTokenizer : public BESObj {
private:
    std::vector<std::string> tokens;
    typedef std::vector<std::string>::iterator tokens_iterator;
    typedef std::vector<std::string>::const_iterator tokens_citerator;
    int _counter;
    unsigned int _number_tokens;

public:
    BESTokenizer();
    ~BESTokenizer() override;

    void tokenize(const char *p);
    std::string &get_first_token();
    std::string &get_current_token();
    std::string &get_next_token();
    void parse_error(const std::string &s = "");
    std::string parse_container_name(const std::string &s, unsigned int &type);
    std::string remove_quotes(const std::string &s);

    void dump_tokens();

    void dump(std::ostream &strm) const override;
};

#endif // BESTokenizer_h_
