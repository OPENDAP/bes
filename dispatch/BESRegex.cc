
// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of libdap, A C++ implementation of the OPeNDAP Data
// Access Protocol.

// Copyright (c) 2005 OPeNDAP, Inc.
// Author: James Gallagher <jgallagher@opendap.org>
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
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.

#include "config.h"

#include "BESRegex.h"
#include "Error.h"

using namespace std;
using namespace libdap;

//namespace dispatch {

/** Does the regular expression match the string? 

    @param s The string
    @param len The length of string to consider
    @param pos Start looking at this position in the string
    @return The number of characters that match, -1 if there's no match. */
int 
BESRegex::match(const char *s, int len, int pos) const
{
    if (pos > len)
        throw Error("Position exceed length in BESRegex::match()");

    smatch match;
    auto target = string(s+pos, len-pos);
    bool found = regex_search(target, match, d_exp);
    if (found)
        return (int)match.length();
    else
        return -1;
}

/**
 * @brief Search for a match to the regex
 * @param s The target for the search
 * @return The length of the matching substring, or -1 if no match was found
 */
int
BESRegex::match(const string &s) const
{
    smatch match;
    bool found = regex_search(s, match, d_exp);
    if (found)
        return (int)match.length();
    else
        return -1;
}

/** Does the regular expression match the string? 

    @param s The string
    @param len The length of string to consider
    @param matchlen Return the length of the matched portion in this 
    value-result parameter.
    @param pos Start looking at this position in the string
    @return The start position of the first match. This is different from 
    POSIX regular expressions, whcih return the start position of the 
    longest match. */
int 
BESRegex::search(const char *s, int len, int& matchlen, int pos) const
{
    smatch match;
    // This is needed because in C++14, the first arg to regex_search() cannot be a
    // temporary string. It seems the C++11 compilers on some linux dists are using
    // regex headers that enforce c++14 rules. jhrg 12/2/21
    auto target = string(s+pos, len-pos);
    bool found = regex_search(target, match, d_exp);
    matchlen = (int)match.length();
    if (found)
        return (int)match.position();
    else
        return -1;
}

/**
 * @brief Search for a match to the regex
 * @param s The target for the search
 * @param matchlen The number of characters that matched
 * @return The starting position of the first set of matching characters
 */
int
BESRegex::search(const string &s, int& matchlen) const
{
    smatch match;
    bool found = regex_search(s, match, d_exp);
    matchlen = (int)match.length();
    if (found)
        return (int)match.position();
    else
        return -1;
}

//} // namespace libdap

