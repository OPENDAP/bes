// SocketUtilities.cc

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

#include "config.h"

#include <cstdlib>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "SocketUtilities.h"

using std::string;

char *
SocketUtilities::ltoa( long val, char *buf, int base)
{
    ldiv_t r ;

    if( base > 36 || base < 2 ) // no conversion if wrong base
    {
	*buf = '\0' ;
	return buf ;
    }
    if(val < 0 )
    *buf++ = '-' ;
    r = ldiv( labs( val ), base ) ;

    /* output digits of val/base first */
    if( r.quot > 0 )
	buf = ltoa( r.quot, buf, base ) ;

    /* output last digit */
    *buf++ = "0123456789abcdefghijklmnopqrstuvwxyz"[(int)r.rem] ;
    *buf = '\0' ;
    return buf ;
}

string
SocketUtilities::create_temp_name()
{
    char tempbuf1[50] ;
    SocketUtilities::ltoa( getpid(), tempbuf1, 10 ) ;
    string s = tempbuf1 + (string)"_" ;
    char tempbuf0[50] ;
    unsigned int t = time( NULL ) - 1000000000 ;
    SocketUtilities::ltoa( t, tempbuf0, 10 ) ;
    s += tempbuf0 ;
    return s ;
}

