// BESTokenizer.cc

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

#include <cstring>
#include <iostream>

using std::cout;
using std::endl;
using std::ostream;
using std::string;

#include "BESSyntaxUserError.h"
#include "BESTokenizer.h"

BESTokenizer::BESTokenizer() : _counter(-1), _number_tokens(0) {}

BESTokenizer::~BESTokenizer() {}

/** @brief throws an exception giving the tokens up to the point of the
 * problem
 *
 * Throws an excpetion using the passed error string, and adding to it the
 * tokens that have been read leading up to the point that this method is
 * called.
 *
 * @param s error string passed by caller to be display with list of
 * tokens
 * @throws BESSyntaxUserError with the passed error string as well as all tokens
 * leading up to the error.
 */
void BESTokenizer::parse_error(const string &s) {
    string error = "Parse error.";
    string where = "";
    if (_counter >= 0) {
        for (int w = 0; w < _counter + 1; w++)
            where += tokens[w] + " ";
        where += "<----HERE IS THE ERROR";
        error += "\n" + where;
    }
    if (s != "")
        error += "\n" + s;
    throw BESSyntaxUserError(error, __FILE__, __LINE__);
}

/** @brief returns the first token from the token list
 *
 * This method will return the first token from the token list. Whether the
 * caller has accessed tokens already or not, the caller is returned to the
 * beginning of the list.
 *
 * @returns the first token in the token list
 */
string &BESTokenizer::get_first_token() {
    _counter = 0;
    return tokens[_counter];
}

/** @brief returns the current token from the token list
 *
 * Returns the current token from the token list and returns it to the caller.
 *
 * @returns current token
 * @throws BESError if the first token has not yet been retrieved or if
 * the caller is attempting to access more tokens than are available.
 * @see BESError
 */
string &BESTokenizer::get_current_token() {
    if (_counter < 0 || _counter > (int)_number_tokens - 1) {
        parse_error("incomplete expression!");
    }

    return tokens[_counter];
}

/** @brief returns the next token from the token list
 *
 * Returns the next token from the token list and returns it to the caller.
 *
 * @returns next token
 * @throws BESError if the first token has not yet been retrieved or if
 * the caller is attempting to access more tokens than are available.
 * @see BESError
 */
string &BESTokenizer::get_next_token() {
    if (_counter == -1) {
        parse_error("incomplete expression!");
    }

    if (_counter >= (int)(_number_tokens - 1)) {
        parse_error("incomplete expression!");
    }

    return tokens[++_counter];
}

/** @brief tokenize the BES request/command string
 *
 *  BESTokenizer tokenizes a BES request command string, such as a get
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
 * When a "\" is character is encountered it is treated as an escape character.
 * The "\" is removed from the token and the character following it is added
 * to the token - even if it's a double qoute.
 *
 * @param p request command string
 * @throws BESError if quoted text is missing an end quote, if the
 * request string is not terminated by a semiclon, if the number of tokens
 * is less than 2.
 * @see BESError
 */
void BESTokenizer::tokenize(const char *p) {
    size_t len = strlen(p);
    string s = "";
    bool passing_raw = false;
    bool escaped = false;

    for (unsigned int j = 0; j < len; j++) {

        if (!escaped && p[j] == '\"') {

            if (s != "") {
                if (passing_raw) {
                    s += "\"";
                    tokens.push_back(s);
                    s = "";
                } else {
                    tokens.push_back(s);
                    s = "\"";
                }
            } else {
                s += "\"";
            }
            passing_raw = !passing_raw;

        } else if (passing_raw) {

            if (!escaped && p[j] == '\\') {
                escaped = true;
            } else {
                s += p[j];

                if (escaped)
                    escaped = false;
            }

        } else {
            if ((p[j] == ' ') || (p[j] == '\n') || (p[j] == 0x0D) || (p[j] == 0x0A)) {
                if (s != "") {
                    tokens.push_back(s);
                    s = "";
                }
            } else if ((p[j] == ',') || (p[j] == ';')) {
                if (s != "") {
                    tokens.push_back(s);
                    s = "";
                }
                switch (p[j]) {
                case ',':
                    tokens.push_back(",");
                    break;
                case ';':
                    tokens.push_back(";");
                    break;
                }
            } else
                s += p[j];
        }
    }

    if (s != "")
        tokens.push_back(s);
    _number_tokens = tokens.size();
    if (passing_raw)
        parse_error("Unclose quote found.(\")");
    if (_number_tokens < 1)
        parse_error("Unknown command: '" + (string)p + (string) "'");
    if (tokens[_number_tokens - 1] != ";")
        parse_error("The request must be terminated by a semicolon (;)");
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
 * @throws BESError if the syntax is incorrect
 * @see BESError
 */
string BESTokenizer::parse_container_name(const string &s, unsigned int &type) {
    string::size_type where = s.rfind(".constraint=", s.size());
    if (where == string::npos) {
        where = s.rfind(".attributes=", s.size());
        if (where == string::npos) {
            parse_error("Expected property declaration.");
        } else {
            type = 2;
        }
    } else {
        type = 1;
    }
    string valid = s.substr(where, s.size());
    if ((valid != ".constraint=") && (valid != ".attributes=")) {
        string err = (string) "Invalid container property " + valid + " for container " + s.substr(0, where) +
                     ". constraint expressions and attribute lists " + "must be wrapped in quotes";
        parse_error(err);
    }
    return s.substr(0, where);
}

/** @brief removes quotes from a quoted token
 *
 * Removes quotes from a quoted token. The passed string must begin with a
 * double quote and must end with a double quote.
 *
 * @param s string where the double quotes are removed.
 * @returns the unquoted token
 * @throws BESError if the string does not begin and end with a double
 * quote
 * @see BESError
 */
string BESTokenizer::remove_quotes(const string &s) {
    if ((s[0] != '"') || (s[s.size() - 1] != '"')) {
        parse_error("item " + s + " must be enclosed by quotes");
    }
    return s.substr(1, s.size() - 2);
}

/** @brief dump the tokens that have been tokenized in the order in which
 * they are parsed.
 *
 * Dumps to standard out the tokens that were parsed from the
 * request/command string. If the tokens were quoted, the quotes are kept
 * in. The tokens are all displayed with double quotes around them to show
 * if there are any spaces or special characters in the token.
 */
void BESTokenizer::dump_tokens() {
    tokens_citerator i = tokens.begin();
    tokens_citerator ie = tokens.end();
    for (; i != ie; i++) {
        cout << "\"" << (*i) << "\"" << endl;
    }
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance
 *
 * @param strm C++ i/o stream to dump the information to
 */
void BESTokenizer::dump(ostream &strm) const {
    strm << BESIndent::LMarg << "BESTokenizer::dump - (" << (void *)this << ")" << endl;
    BESIndent::Indent();
    tokens_citerator i = tokens.begin();
    tokens_citerator ie = tokens.end();
    for (; i != ie; i++) {
        strm << BESIndent::LMarg << "\"" << (*i) << "\"" << endl;
    }
    BESIndent::UnIndent();
}
