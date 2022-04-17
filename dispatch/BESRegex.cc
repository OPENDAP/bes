
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


//#define DODS_DEBUG

#include "config.h"

#if 0
#ifndef WIN32
#include <alloca.h>
#endif
#include <stdlib.h>
 
#include <sys/types.h>
#include <regex.h>

#include <new>
#include <string>
#include <vector>
#include <stdexcept>
#endif

#include <string>
#include <vector>

#include <regex.h>

//#include <libdap/Error.h>
#include <libdap/debug.h>
#include <libdap/util.h>

#include "BESError.h"
#include "BESRegex.h"

#if 0
#include "util.h"
#include "debug.h"
#endif

using namespace std;

void
BESRegex::init(const char *t)
{
#if !USE_CPP_11_REGEX
    d_preg = static_cast<void*>(new regex_t);

    int result = regcomp(static_cast<regex_t*>(d_preg), t, REG_EXTENDED);
    if  (result != 0) {
        size_t msg_len = regerror(result, static_cast<regex_t*>(d_preg),
                                  static_cast<char*>(nullptr),
                                  static_cast<size_t>(0));

        vector<char> msg(msg_len+1);
        regerror(result, static_cast<regex_t*>(d_preg), &msg[0], msg_len);
        string err = string("BESRegex error: ") + string(&msg[0], msg_len);
        throw BESError(err, BES_SYNTAX_USER_ERROR, __FILE__, __LINE__);
    }
#else
    d_exp = regex(t);
#endif
}

#if 0
void
BESRegex::init(const string &t)
{
    d_exp = regex(t);
}
#endif

#if !USE_CPP_11_REGEX
BESRegex::~BESRegex()
{
    regfree(static_cast<regex_t*>(d_preg));
    delete static_cast<regex_t*>(d_preg); d_preg = 0;
}
#endif

#if 0
/** Initialize a POSIX regular expression (using the 'extended' features).

    @param t The regular expression pattern. */
BESRegex::BESRegex(const char* t)
{
    init(t);
}

/** Compatability ctor.
    @see BESRegex::BESRegex(const char* t) */
BESRegex::BESRegex(const char* t, int)
{
    init(t);
}
#endif

/** Does the regular expression match the string? 

    @param s The string
    @param len The length of string to consider
    @param pos Start looking at this position in the string
    @return The number of characters that match, -1 if there's no match. */
int 
BESRegex::match(const char *s, int len, int pos) const
{
#if !USE_CPP_11_REGEX
    if (len > 32766)	// Integer overflow protection
    	return -1;
    	
    regmatch_t *pmatch = new regmatch_t[len+1];
    string ss = s;

    int result = regexec(static_cast<regex_t*>(d_preg), 
                         ss.substr(pos, len-pos).c_str(), len, pmatch, 0);
	int matchnum;
    if (result == REG_NOMATCH)
        matchnum = -1;
	else
		matchnum = pmatch[0].rm_eo - pmatch[0].rm_so;
		
	delete[] pmatch; pmatch = 0;

    return matchnum;
#else
    if (pos > len)
        throw Error("Position exceed length in BESRegex::match()");

    smatch match;
    auto target = string(s+pos, len-pos);
    bool found = regex_search(target, match, d_exp);
    if (found)
        return (int)match.length();
    else
        return -1;
#endif
}

/**
 * @brief Search for a match to the regex
 * @param s The target for the search
 * @return The length of the matching substring, or -1 if no match was found
 */
int
BESRegex::match(const string &s) const
{
#if USE_CPP_11_REGEX
    smatch match;
    bool found = regex_search(s, match, d_exp);
    if (found)
        return (int)match.length();
    else
        return -1;
#else
    return match(s.c_str(), s.length(), 0);
#endif
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
#if !USE_CPP_11_REGEX
    // sanitize allocation
    if (!libdap::size_ok(sizeof(regmatch_t), len+1))
    	return -1;
    	
    // alloc space for len matches, which is theoretical max.
    // Problem: If somehow 'len' is very large - say the size of a 32-bit int,
    // then len+1 is a an integer overflow and this might be exploited by
    // an attacker. It's not likely there will be more than a handful of
    // matches, so I am going to limit this value to 32766. jhrg 3/4/09
    if (len > 32766)
    	return -1;

    regmatch_t *pmatch = new regmatch_t[len+1];
    string ss = s;
     
    int result = regexec(static_cast<regex_t*>(d_preg),
                         ss.substr(pos, len-pos).c_str(), len, pmatch, 0);
    if (result == REG_NOMATCH) {
        delete[] pmatch; pmatch = 0;
        return -1;
    }

    // Match found, find the first one (pmatch lists the longest first)
    int m = 0;
    for (int i = 1; i < len; ++i)
        if (pmatch[i].rm_so != -1 && pmatch[i].rm_so < pmatch[m].rm_so)
            m = i;
            
    matchlen = pmatch[m].rm_eo - pmatch[m].rm_so;
    int matchpos = pmatch[m].rm_so;
    
    delete[] pmatch; pmatch = 0;
    return matchpos;
#else
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
#endif
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
#if USE_CPP_11_REGEX
    smatch match;
    bool found = regex_search(s, match, d_exp);
    matchlen = (int)match.length();
    if (found)
        return (int)match.position();
    else
        return -1;
#else
    // search(const char *s, int len, int& matchlen, int pos) const
    return search(s.c_str(), s.length(), matchlen, 0);
#endif
}

