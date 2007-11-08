
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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.

#ifndef _BESRegex_h
#define _BESRegex_h 1

/** a C++ interface to POSIX regular expression functions.

    @author James Gallagher <jgallagher@opendap.org> */
class BESRegex
{
private:
    // d_preg was a regex_t* but I needed to include both regex.h and config.h
    // to make the gnulib code work. Because this header is installed (and is
    // used by other libraries) it cannot include config.h, so I moved the 
    // regex.h and config.h (among other) includes to the implementation. It
    // would be cleaner to use a special class, but for one field that seems
    // like overkill.
    void *d_preg;
    void init(const char *t);
    
public:
    BESRegex(const char *t);
    BESRegex(const char *t, int dummy);
    ~BESRegex();

    /// Does the pattern match.
    int match(const char* s, int len, int pos = 0);
    /// How much of the string does the pattern matche.
    int search(const char* s, int len, int& matchlen, int pos = 0);
};

#endif
