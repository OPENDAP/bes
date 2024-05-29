// BESRegex.cc

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2004-2009 University Corporation for Atmospheric Research
// Author: Patrick West <pwest@ucar.edu> and Jose Garcia <jgarcia@ucar.edu>
// and James Gallagher <jgallagher@gso.uri.edu>
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
//      jimg        James Gallagher <jgallagher@gso.uri.edu>

#include <config.h>

#ifndef WIN32
#include <alloca.h>
#endif
 
#include <sys/types.h>
#include <regex.h>

#include <cstdlib>
#include <new>
#include <string>
#include <stdexcept>

#include "BESRegex.h"
#include "BESInternalError.h"
#include "BESScrub.h"

using namespace std;

void
BESRegex::init(const char *t)
{
    d_pattern = t;
    d_preg = static_cast<void*>(new regex_t);
    int result = regcomp(static_cast<regex_t*>(d_preg), t, REG_EXTENDED);

    if (result != 0) {
        size_t msg_len = regerror(result, static_cast<regex_t*>(d_preg), static_cast<char*>(NULL),
            static_cast<size_t>(0));
        char *msg = new char[msg_len + 1];
        regerror(result, static_cast<regex_t*>(d_preg), msg, msg_len);
        string err = string("BESRegex error: ") + string(msg);
        BESInternalError e(err, __FILE__, __LINE__);
        delete[] msg;
        throw e;
    }
}

BESRegex::~BESRegex()
{
    regfree(static_cast<regex_t*>(d_preg));
    delete static_cast<regex_t*>(d_preg); d_preg = 0;

}

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

/** @brief Does the regular expression match the string?

    Warning : this function can be used to match strings of zero length
   	if the regex pattern accepts empty strings. Therefore this function
   	returns -1 if the pattern does not match.

    @param s The string
    @param len The length of string to consider
    @param pos Start looking at this position in the string
    @return The number of characters that match, -1 if there's no match. */
int 
BESRegex::match(const char* s, int len, int pos)
{
    // TODO re-implement using auto_ptr or unique_ptr. jhrg 7/27/18
    regmatch_t *pmatch = new regmatch_t[len+1];
    string ss = s;

    int result = regexec(static_cast<regex_t*>(d_preg), 
                         ss.substr(pos, len-pos).c_str(), len, pmatch, 0);
	int matchnum;
    if (result == REG_NOMATCH)
        matchnum = -1; //returns -1 due to function being able to match strings of 0 length
	else
		matchnum = pmatch[0].rm_eo - pmatch[0].rm_so;
		
	delete[] pmatch; pmatch = 0;

    return matchnum;
}

/** Does the regular expression match the string? 

    @param s The string
    @param len The length of string to consider
    @param matchlen Return the length of the matched portion in this 
    value-result parameter.
    @param pos Start looking at this position in the string
    @return The start position of the first match. This is different from 
    POSIX regular expressions, which return the start position of the
    longest match. */
int 
BESRegex::search(const char* s, int len, int& matchlen, int pos)
{
	// sanitize allocation
    if (!BESScrub::size_ok(sizeof(regmatch_t), len+1))
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
}

