// BESRegex.h

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
    std::string d_pattern;
    void init(const char *t);
    
public:
    BESRegex(const char *t);
    BESRegex(const char *t, int dummy);
    ~BESRegex();

    /// Does the pattern match.
    int match(const char* s, int len, int pos = 0);
    /// How much of the string does the pattern matche.
    int search(const char* s, int len, int& matchlen, int pos = 0);
    std::string pattern(){ return d_pattern; }
};

#endif
