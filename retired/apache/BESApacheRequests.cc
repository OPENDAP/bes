// BESApacheRequests.cc

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

#include "BESApacheRequests.h"
#include "BESInternalError.h"

#include <iostream>

using std::cout ;
using std::endl ;

BESApacheRequests::BESApacheRequests( const string &requests )
{
    if( requests != "" )
    {
	unsigned int len = requests.size() ;
	const char *request = requests.c_str() ;
	if( request )
	{
	    unsigned int index = 0 ;
	    unsigned int start = 0 ;
	    bool inquotes = false ;
	    bool done = false ;
	    char c ;
	    while( !done )
	    {
		c = request[index] ;
		if( inquotes )
		{
		    if( c == '\"' )
		    {
			inquotes = false ;
		    }
		}
		else
		{
		    if( c == '\"' )
		    {
			inquotes = true ;
		    }
		    else if( c == ';' )
		    {
			string req = requests.substr( start, index-start+1 ) ;
			_requests.push_back( req ) ;
			start = index+1 ;
		    }
		}
		index++ ;
		if( index == len )
		{
		    if( index != start )
		    {
			if( inquotes )
			{
			    string err = "ending double quote missing in request string" ;
			    throw BESInternalError( err, __FILE__, __LINE__ ) ;
			}
			else
			{
			    string err = "requests must end with a semicolon (;)" ;
			    throw BESInternalError( err, __FILE__, __LINE__ ) ;
			}
		    }
		    done = true ;
		}
	    }
	}
    }
}

BESApacheRequests::~BESApacheRequests()
{
}

BESApacheRequests::requests_citer
BESApacheRequests::get_first_request()
{
    return _requests.begin() ;
}

BESApacheRequests::requests_citer
BESApacheRequests::get_end_request()
{
    return _requests.end() ;
}

