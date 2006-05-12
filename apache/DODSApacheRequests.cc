// DODSApacheRequests.cc

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2004,2005 University Corporation for Atmospheric Research
// Author: Patrick West <pwest@ucar.org> and Jose Garcia <jgarcia@ucar.org>
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
 
// (c) COPYRIGHT University Corporation for Atmostpheric Research 2004-2005
// Please read the full copyright statement in the file COPYRIGHT_UCAR.
//
// Authors:
//      pwest       Patrick West <pwest@ucar.edu>
//      jgarcia     Jose Garcia <jgarcia@ucar.edu>

#include "DODSApacheRequests.h"
#include "DODSBasicException.h"

#include <iostream>

using std::cout ;
using std::endl ;

DODSApacheRequests::DODSApacheRequests( const string &requests )
{
    if( requests != "" )
    {
	unsigned int len = requests.length() ;
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
			    throw DODSBasicException( "ending double quote missing in request string" ) ;
			else
			    throw DODSBasicException( "requests must end with a semicolon (;)" ) ;
		    }
		    done = true ;
		}
	    }
	}
    }
}

DODSApacheRequests::~DODSApacheRequests()
{
}

DODSApacheRequests::requests_citer
DODSApacheRequests::get_first_request()
{
    return _requests.begin() ;
}

DODSApacheRequests::requests_citer
DODSApacheRequests::get_end_request()
{
    return _requests.end() ;
}

