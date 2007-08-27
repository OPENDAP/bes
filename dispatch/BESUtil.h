// BESUtil.h

#include <stdio.h>

#include <string>
#include <iostream>

using std::string ;
using std::ostream ;

#ifndef E_BESUtil_h
#define E_BESUtil_h 1

class BESUtil
{
private:
    static string rfc822_date( const time_t t ) ;

public:
    /** These functions are used to create the MIME headers for a message
	from a server to a client.

	NB: These functions actually write both the response status line
	<i>and</i> the header.

	@name MIME utility functions
	@see DODSFilter
    */
    static void set_mime_text( ostream &strm ) ;
    static void set_mime_html( ostream &strm ) ;

    /** This functions are used to unescape hex characters from strings **/
    static string www2id( const string &in,
                          const string &escape = "%",
		          const string &except = "" ) ;
    static string unhexstring( string s ) ;

    /** Convert a string to all lower case **/
    static string lowercase( const string &s ) ;

    /** Unescape characters with backslash before them **/
    static string unescape( const string &s ) ;
} ;

#endif // E_BESUtil_h

