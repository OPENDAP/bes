
#include "BESUtil.h"
#include "config.h"

#define CRLF "\r\n"

/** @brief Generate an HTTP 1.0 response header for a text document.

    @param out Write the MIME header to this FILE pointer.
 */
void
BESUtil::set_mime_text( FILE *out )
{
    fprintf( out, "HTTP/1.0 200 OK%s", CRLF ) ;
    fprintf( out, "XBES-Server: %s%s", PACKAGE_STRING, CRLF ) ;

    const time_t t = time(0);
    fprintf( out, "Date: %s%s", rfc822_date(t).c_str(), CRLF ) ;
    fprintf( out, "Last-Modified: %sw%s", rfc822_date(t).c_str(), CRLF ) ;

    fprintf( out, "Content-Type: text/plain%s", CRLF ) ;
    // Note that Content-Description is from RFC 2045 (MIME, pt 1), not 2616.
    fprintf( out, "Content-Description: unknown%s", CRLF ) ;
    fprintf( out, CRLF ) ;
}

void
BESUtil::set_mime_html( FILE *out )
{
    fprintf( out, "HTTP/1.0 200 OK%s", CRLF ) ;
    fprintf( out, "XBES-Server: %s%s", PACKAGE_STRING, CRLF ) ;

    const time_t t = time(0);
    fprintf( out, "Date: %s%s", rfc822_date(t).c_str(), CRLF ) ;
    fprintf( out, "Last-Modified: %sw%s", rfc822_date(t).c_str(), CRLF ) ;

    fprintf( out, "Content-type: text/html%s", CRLF ) ;
    // Note that Content-Description is from RFC 2045 (MIME, pt 1), not 2616.
    fprintf( out, "Content-Description: unknown%s", CRLF ) ;
    fprintf( out, CRLF ) ;
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
    @param t A const <tt>time_t</tt> pointer. */
string
BESUtil::rfc822_date(const time_t t)
{
    struct tm *stm = gmtime(&t);
    char d[256];

    sprintf(d, "%s, %02d %s %4d %02d:%02d:%02d GMT", days[stm->tm_wday], 
	    stm->tm_mday, months[stm->tm_mon], 
#if 0
	    // On Solaris 2.7 this tm_year is years since 1900. 3/17/2000
	    // jhrg
	    stm->tm_year < 100 ? 1900 + stm->tm_year : stm->tm_year, 
#endif
	    1900 + stm->tm_year,
	    stm->tm_hour, stm->tm_min, stm->tm_sec);
    return string(d);
}

