// BESUtil.cc

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
 
// (c) COPYRIGHT University Corporation for Atmospheric Research 2004-2005
// Please read the full copyright statement in the file COPYRIGHT_UCAR.
//
// Authors:
//      pwest       Patrick West <pwest@ucar.edu>
//      jgarcia     Jose Garcia <jgarcia@ucar.edu>

#include "config.h"

#include <sys/types.h>
#include <sys/stat.h>

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <cerrno>
#include <cstring>
#include <cstdlib>
#include <sstream>
#include <iostream>

using std::istringstream ;
using std::cout ;
using std::endl ;

#include "BESUtil.h"
#include "BESForbiddenError.h"
#include "BESNotFoundError.h"

#define CRLF "\r\n"

/** @brief Generate an HTTP 1.0 response header for a text document.

    @param strm Write the MIME header to this ostream.
 */
void
BESUtil::set_mime_text( ostream &strm )
{
    strm << "HTTP/1.0 200 OK" << CRLF ;
    strm << "XBES-Server: " << PACKAGE_STRING << CRLF ;

    const time_t t = time(0);
    strm << "Date: " << rfc822_date(t).c_str() << CRLF ;
    strm << "Last-Modified: " << rfc822_date(t).c_str() << CRLF ;

    strm << "Content-Type: text/plain" << CRLF ;
    // Note that Content-Description is from RFC 2045 (MIME, pt 1), not 2616.
    strm << "Content-Description: unknown" << CRLF ;
    strm << CRLF ;
}

/** @brief Generate an HTTP 1.0 response header for a html document.

    @param strm Write the MIME header to this ostream.
 */
void
BESUtil::set_mime_html( ostream &strm )
{
    strm << "HTTP/1.0 200 OK" << CRLF ;
    strm << "XBES-Server: " << PACKAGE_STRING << CRLF ;

    const time_t t = time(0);
    strm << "Date: " << rfc822_date(t).c_str() << CRLF ;
    strm << "Last-Modified: " << rfc822_date(t).c_str() << CRLF ;

    strm << "Content-type: text/html" << CRLF ;
    // Note that Content-Description is from RFC 2045 (MIME, pt 1), not 2616.
    strm << "Content-Description: unknown" << CRLF ;
    strm << CRLF ;
}

// Return a MIME rfc-822 date. The grammar for this is:
//       date-time   =  [ day "," ] date time        ; dd mm yy
//                                                   ;  hh:mm:ss zzz
//
//       day         =  "Mon"  / "Tue" /  "Wed"  / "Thu"
//                   /  "Fri"  / "Sat" /  "Sun"
//
//       date        =  1*2DIGIT month 2DIGIT        ; day month year
//                                                   ;  e.g. 20 Jun 82
//                   NB: year is 4 digit; see RFC 1123. 11/30/99 jhrg
//
//       month       =  "Jan"  /  "Feb" /  "Mar"  /  "Apr"
//                   /  "May"  /  "Jun" /  "Jul"  /  "Aug"
//                   /  "Sep"  /  "Oct" /  "Nov"  /  "Dec"
//
//       time        =  hour zone                    ; ANSI and Military
//
//       hour        =  2DIGIT ":" 2DIGIT [":" 2DIGIT]
//                                                   ; 00:00:00 - 23:59:59
//
//       zone        =  "UT"  / "GMT"                ; Universal Time
//                                                   ; North American : UT
//                   /  "EST" / "EDT"                ;  Eastern:  - 5/ - 4
//                   /  "CST" / "CDT"                ;  Central:  - 6/ - 5
//                   /  "MST" / "MDT"                ;  Mountain: - 7/ - 6
//                   /  "PST" / "PDT"                ;  Pacific:  - 8/ - 7
//                   /  1ALPHA                       ; Military: Z = UT;
//                                                   ;  A:-1; (J not used)
//                                                   ;  M:-12; N:+1; Y:+12
//                   / ( ("+" / "-") 4DIGIT )        ; Local differential
//                                                   ;  hours+min. (HHMM)

static const char *days[]={"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
static const char *months[]={"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", 
			"Aug", "Sep", "Oct", "Nov", "Dec"};

/** Given a constant pointer to a <tt>time_t</tt>, return a RFC
    822/1123 style date.

    This function returns the RFC 822 date with the exception that the RFC
    1123 modification for four-digit years is implemented.

    @return The RFC 822/1123 style date in a C++ string.
    @param t A const <tt>time_t</tt> pointer.
 */
string
BESUtil::rfc822_date(const time_t t)
{
    struct tm *stm = gmtime(&t);
    char d[256];

    snprintf(d, 255, "%s, %02d %s %4d %02d:%02d:%02d GMT", days[stm->tm_wday],
            stm->tm_mday, months[stm->tm_mon],
            1900 + stm->tm_year,
            stm->tm_hour, stm->tm_min, stm->tm_sec);
    d[255] = '\0';
    return string(d);
}

string 
BESUtil::unhexstring( string s ) 
{
    int val;
    istringstream ss( s ) ;
    ss >> std::hex >> val;
    char tmp_str[2];
    tmp_str[0] = static_cast<char>(val);
    tmp_str[1] = '\0';
    return string(tmp_str);
}

string 
BESUtil::www2id(const string &in, const string &escape, const string &except)
{
    string::size_type i = 0;
    string res = in;
    while ((i = res.find_first_of(escape, i)) != string::npos) {
	if (res.substr(i, 3) == except) {
	    i += 3;
	    continue;
	}
	res.replace(i, 3, unhexstring(res.substr(i + 1, 2)));
    }

    return res;
}

string
BESUtil::lowercase( const string &s )
{
    string return_string = s ;
    for( int j = 0; j < return_string.length(); j++ )
    {
	return_string[j] = (char)tolower( return_string[j] ) ;
    }

    return return_string ;
}

string
BESUtil::unescape( const string &s )
{
    bool done = false ;
    string::size_type index = 0 ;
    string::size_type new_index = 0 ;
    string new_str ;
    while( !done )
    {
	string::size_type bs = s.find( '\\', index ) ;
	if( bs == string::npos )
	{
	    new_str += s.substr( index, s.length() - index ) ;
	    done = true ;
	}
	else
	{
	    new_str += s.substr( index, bs - index ) ;
	    new_str += s[bs+1] ;
	    index = bs+2 ;
	}
    }

    return new_str ;
}

/** Check if the specified path is valid
 *
 * Checks to see if the specified path is a valid path or not. The root
 * directory specified is assumed to be valid, so we don't check that
 * part of the path. The path parameter is relative to the root
 * directory.
 *
 * If follow_sym_links is false, then if any part of the specified path
 * is a symbolic link, this function will return false, set the passed
 * has_sym_link parameter. No error message is specified.
 *
 * If there is a problem accessing the specified path then the error
 * string will be filled with whatever system error message is provided.
 *
 * param path path to check
 * param root root directory path, assumed to be valid
 * param follow_sym_links specifies whether allowed to follow symbolic links
 * throws BESForbiddenError if the user is not allowed to traverse the path
 * throws BESNotFoundError if there is a problem accessing the path or the
 * path does not exist.
 **/
void
BESUtil::check_path( const string &path,
		     const string &root,
		     bool follow_sym_links )
{
    // if nothing is passed in path, then the path checks out since root is
    // assumed to be valid.
    if( path == "" )
	return ;

    // make sure there are no ../ in the directory, backing up in any way is
    // not allowed.
    string::size_type dotdot = path.find( ".." ) ;
    if( dotdot != string::npos )
    {
	string s = (string)"You are not allowed to access the node " + path;
	throw BESForbiddenError( s, __FILE__, __LINE__ ) ;
    }

    // What I want to do is to take each part of path and check to see if it
    // is a symbolic link and it is accessible. If everything is ok, add the
    // next part of the path.
    bool done = false ;

    // what is remaining to check
    string rem = path ;
    if( rem[0] == '/' )
	rem = rem.substr( 1, rem.length() - 1 ) ;
    if( rem[rem.length()-1] == '/' )
	rem = rem.substr( 0, rem.length() - 1 ) ;

    // full path of the thing to check
    string fullpath = root ;
    if( fullpath[fullpath.length()-1] == '/' )
    {
	fullpath = fullpath.substr( 0, fullpath.length() - 1 ) ;
    }

    // path checked so far
    string checked ;

    while( !done )
    {
	size_t slash = rem.find( '/' ) ;
	if( slash == string::npos )
	{
	    fullpath = fullpath + "/" + rem ;
	    checked = checked + "/" + rem ;
	    done = true ;
	}
	else
	{
	    fullpath = fullpath + "/" + rem.substr( 0, slash ) ;
	    checked = checked + "/" + rem.substr( 0, slash ) ;
	    rem = rem.substr( slash + 1, rem.length() - slash ) ;
	}

	if( !follow_sym_links )
	{
	    struct stat buf;
	    int statret = lstat( fullpath.c_str(), &buf ) ;
	    if( statret == -1 )
	    {
		int errsv = errno ;
		// stat failed, so not accessible. Get the error string,
		// store in error, and throw exception
		char *s_err = strerror( errsv ) ;
		string error = "Unable to access node " + checked + ": " ;
		if( s_err )
		{
		    error = error + s_err ;
		}
		else
		{
		    error = error + "unknow access error" ;
		}
		// ENOENT means that the node wasn't found. Otherise, access
		// is denied for some reason
		if( errsv == ENOENT )
		{
		    throw BESNotFoundError( error, __FILE__, __LINE__ ) ;
		}
		else
		{
		    throw BESForbiddenError( error, __FILE__, __LINE__ ) ;
		}
	    }
	    else
	    {
		// lstat was successful, now check if sym link
		if( S_ISLNK( buf.st_mode ) )
		{
		    string error = "You do not have permission to access "
		                   + checked ;
		    throw BESForbiddenError( error, __FILE__, __LINE__ ) ;
		}
	    }
	}
	else
	{
	    // just do a stat and see if we can access the thing. If we
	    // can't, get the error information and throw an exception
	    struct stat buf ;
	    int statret = stat( fullpath.c_str(), &buf ) ;
	    if( statret == -1 )
	    {
		int errsv = errno ;
		// stat failed, so not accessible. Get the error string,
		// store in error, and throw exception
		char *s_err = strerror( errsv ) ;
		string error = "Unable to access node " + checked + ": " ;
		if( s_err )
		{
		    error = error + s_err ;
		}
		else
		{
		    error = error + "unknow access error" ;
		}
		// ENOENT means that the node wasn't found. Otherise, access
		// is denied for some reason
		if( errsv == ENOENT )
		{
		    throw BESNotFoundError( error, __FILE__, __LINE__ ) ;
		}
		else
		{
		    throw BESForbiddenError( error, __FILE__, __LINE__ ) ;
		}
	    }
	}
    }
}

char *
BESUtil::fastpidconverter( char *buf, int base )
{
    return fastpidconverter( getpid(), buf, base ) ;
}

char *
BESUtil::fastpidconverter(
      long val,					/* value to be converted */
      char *buf,                                /* output string         */
      int base)                                 /* conversion base       */
{
      ldiv_t r;                                 /* result of val / base  */

      if (base > 36 || base < 2)          /* no conversion if wrong base */
      {
            *buf = '\0';
            return buf;
      }
      if (val < 0)
            *buf++ = '-';
      r = ldiv (labs(val), base);

      /* output digits of val/base first */

      if (r.quot > 0)
            buf = fastpidconverter ( r.quot, buf, base);
      /* output last digit */

      *buf++ = "0123456789abcdefghijklmnopqrstuvwxyz"[(int)r.rem];
      *buf   = '\0';
      return buf;
}

