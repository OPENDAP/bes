// DODSProcessEncodedString.cc

// 2004 Copyright University Corporation for Atmospheric Research

#include "DODSProcessEncodedString.h"

using std::cerr ;

DODSProcessEncodedString::DODSProcessEncodedString (const char *s)
{
    if (s)
    {
	string key = "" ;
	string value = "" ;
	bool getting_key_data = true ;
	int len = strlen( s ) ;
	for( int j = 0; j < len; j++ )
	{
	    if( getting_key_data )
	    {
		if( s[j] != '=' )
		{
		    key += s[j] ;
		}
		else
		{
		    getting_key_data = false ;
		    value = "" ;
		}
	    }
	    else
	    {
		if( s[j] != '&' )
		{
		    value += s[j] ;
		}
		else
		{
		    _entries[parseHex( key.c_str() )] = parseHex( value.c_str() ) ;
		    getting_key_data = true ;
		    key = "" ;
		}
	    }
	}
	if( getting_key_data )
	    cerr << "DODSProcessEncodedString: parse error.\n" ;
	else
	{
	    _entries[parseHex( key.c_str() )] = parseHex( value.c_str() ) ;
	}
    }
    else
    {
	cerr << "DODSProcessEncodedString: Passing NULL pointer.\n" ;
	exit( 1 ) ;
    }
}

string
DODSProcessEncodedString::parseHex( const char *s )
{ 
    char *hexstr = new char[strlen( s ) + 1] ;
    strcpy( hexstr, s ) ;

    if(hexstr == NULL || strlen( hexstr ) == 0 ) 
	return ""; 

    register unsigned int x,y; 
    for( x = 0, y = 0; hexstr[y]; x++, y++ ) 
    { 
	if( ( hexstr[x] = hexstr[y] ) == '+' ) 
	{ 
	    hexstr[x] = ' ' ;
	    continue ;
	}
	else if( hexstr[x] == '%' ) 
	{ 
	    hexstr[x] = convertHex( &hexstr[y+1] ) ; 
	    y += 2 ; 
	} 
    } 
    hexstr[x] = '\0';
    string w = hexstr ;
    delete [] hexstr ;
    return w ; 
} 

const unsigned int
DODSProcessEncodedString::convertHex( const char* what )
{ 
    //0x4f == 01001111 mask 

    register char digit; 
    digit = (what[0] >= 'A' ? ((what[0] & 0x4F) - 'A')+10 : (what[0] - '0')); 
    digit *= 16; 
    digit += (what[1] >='A' ? ((what[1] & 0x4F) - 'A')+10 : (what[1] - '0')); 

    return digit; 
} 

string
DODSProcessEncodedString::get_key( const string& s ) 
{
    map<string,string,less<string> >::iterator i ;
    i = _entries.find( s ) ;
    if( i != _entries.end() )
	return (*i).second ;
    else
	return "" ;
}

// $Log: DODSProcessEncodedString.cc,v $
// Revision 1.2  2004/09/09 17:17:12  pwest
// Added copywrite information
//
// Revision 1.1  2004/06/30 20:16:24  pwest
// dods dispatch code, can be used for apache modules or simple cgi script
// invocation or opendap daemon. Built during cedar server development.
//
