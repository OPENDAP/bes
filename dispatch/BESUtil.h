// BESUtil.h

#include <stdio.h>

#include <string>

using std::string ;

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
    static void set_mime_text( FILE *out ) ;
    static void set_mime_html( FILE *out ) ;

    /** This functions are used to unescape hex characters from strings **/
    static string www2id( const string &in,
                          const string &escape = "%",
		          const string &except = "" ) ;
    static string unhexstring( string s ) ;
} ;

#endif // E_BESUtil_h

