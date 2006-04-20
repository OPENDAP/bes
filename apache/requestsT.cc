// requestsT.cc

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2004,2005 University Corporation for Atmospheric Research
// Author: Patrick West <pwest@ucar.org>
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

#include <iostream>

using std::cout ;
using std::cerr ;
using std::endl ;

#include "DODSApacheRequests.h"
#include "DODSBasicException.h"

void test_this( const string &requests ) ;

int
main( int argc, char **argv )
{
    if( argc != 1 )
    {
	test_this( argv[1] ) ;
    }
    else
    {
	test_this( "show version;" ) ;
	test_this( "show version;show help;" ) ;
    }

    return 0 ;
}

void
test_this( const string &requests )
{
    cout << "testing: " << requests << endl ;
    try
    {
	DODSApacheRequests r( requests ) ;
	DODSApacheRequests::requests_citer c = r.get_first_request() ;
	DODSApacheRequests::requests_citer e = r.get_end_request() ;
	for( ; c != e; c++ )
	{
	    cout << "    request: " << (*c) << endl ;
	}
    }
    catch( DODSBasicException &e )
    {
	cerr << "problem: " << e.get_error_description() << endl ;
    }
}

