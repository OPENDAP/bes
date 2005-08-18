// SocketUtilities.cc

// 2005 Copyright University Corporation for Atmospheric Research

#include <unistd.h>

#include "SocketUtilities.h"

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

// $Log: SocketUtilities.cc,v $
