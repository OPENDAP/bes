// BESProcessEncodedString.cc

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

#include "BESProcessEncodedString.h"

#include <cstring>
#include <cstdlib>

using std::cerr ;

BESProcessEncodedString::BESProcessEncodedString (const char *s)
{
    if (s)
    {
	string key = "" ;
	string value = "" ;
	bool getting_key_data = true ;
	size_t len = strlen( s ) ;
	for( unsigned int j = 0; j < len; j++ )
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
		    _entries[parseHex( key.c_str(), key.size() )] = parseHex( value.c_str(), value.size() ) ;
		    getting_key_data = true ;
		    key = "" ;
		}
	    }
	}
	if( getting_key_data )
	    cerr << "BESProcessEncodedString: parse error.\n" ;
	else
	{
	    _entries[parseHex( key.c_str(), key.size() )] = parseHex( value.c_str(), value.size() ) ;
	}
    }
    else
    {
	cerr << "BESProcessEncodedString: Passing NULL pointer.\n" ;
	exit( 1 ) ;
    }
}

string
BESProcessEncodedString::parseHex( const char *s, unsigned int len )
{
    if( !s || !len )
	return "" ;
    char *hexstr = new char[len + 1] ;
    if( hexstr == NULL )
	return "" ;

    strncpy( hexstr, s, len ) ;
    hexstr[len] = '\0';	// Must explicitly add null; strncpy might not. jhrg
    if(strlen( hexstr ) == 0 ) 
    {
	delete [] hexstr ;
	return ""; 
    }

    register unsigned int x,y; 
    for( x = 0, y = 0; hexstr[y] && y < len && x < len; x++, y++ ) 
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
BESProcessEncodedString::convertHex( const char* what )
{ 
    //0x4f == 01001111 mask 

    register char digit; 
    digit = (what[0] >= 'A' ? ((what[0] & 0x4F) - 'A')+10 : (what[0] - '0')); 
    digit *= 16; 
    digit += (what[1] >='A' ? ((what[1] & 0x4F) - 'A')+10 : (what[1] - '0')); 

    return (unsigned int)digit; 
} 

string
BESProcessEncodedString::get_key( const string& s ) 
{
    map<string,string>::iterator i ;
    i = _entries.find( s ) ;
    if( i != _entries.end() )
	return (*i).second ;
    else
	return "" ;
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance along with the key:value
 * pairs
 *
 * @param strm C++ i/o stream to dump the information to
 */
void
BESProcessEncodedString::dump( ostream &strm ) const
{
    strm << BESIndent::LMarg << "BESProcessEncodedString::dump - ("
			     << (void *)this << ")" << endl ;
    BESIndent::Indent() ;
    if( _entries.size() )
    {
	strm << BESIndent::LMarg << "key|value pairs:" << endl ;
	BESIndent::Indent() ;
	map<string,string>::const_iterator i ;
	map<string,string>::const_iterator ie = _entries.end() ;
	for( i = _entries.begin(); i != ie; ++i )
	{
	    strm << BESIndent::LMarg << (*i).first << ": "
				     << (*i).second << endl ;
	}
	BESIndent::UnIndent() ;
    }
    else
    {
	strm << BESIndent::LMarg << "key|value pairs: none" << endl ;
    }
    BESIndent::UnIndent() ;
}

