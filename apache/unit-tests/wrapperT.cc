// wrapperT.cc

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

#include <iostream>

using std::cout ;
using std::cerr ;
using std::endl ;

#include "BESApacheWrapper.h"
#include "BESDataRequestInterface.h"
#include "BESError.h"
#include "BESGlobalIQ.h"
#include "BESDefaultModule.h"
#include "BESDefaultCommands.h"
#include "BESDapModule.h"
#include "DAPCommandModule.h"
#include "BESDebug.h"

int
main( int argc, char **argv )
{
    string bes_conf = "BES_CONF=./opendap.ini" ;
    putenv( (char *)bes_conf.c_str() ) ;

    BESDebug::SetUp( "cerr,all" ) ;

    BESDataRequestInterface rq;

    // BEGIN Initialize all data request elements correctly to a null pointer 
    rq.server_name=0;
    rq.server_address=0;
    rq.server_protocol=0;
    rq.server_port=0;
    rq.script_name=0;
    rq.user_address=0;
    rq.user_agent=0;
    rq.request=0;
    // END Initialize all the data request elements correctly to a null pointer

    rq.server_name="cedar-l" ;
    rq.server_address="jose" ;
    rq.server_protocol="TXT" ;
    rq.server_port="8081" ;
    rq.script_name="someting" ;
    rq.user_address="0.0.0.0" ;
    rq.user_agent = "Patrick" ;

    try
    {
	BESApacheWrapper wrapper ;
	rq.cookie=wrapper.process_user( "username=pwest" ) ;
	rq.token="token" ;
	wrapper.process_request( "request=show+status;show+version;" ) ;
	rq.request = wrapper.get_first_request() ;
	while( rq.request )
	{
	    int status = wrapper.call_BES(rq);
	    if( status == 0 )
		rq.request = wrapper.get_next_request() ;
	    else
		return 1 ;
	}

    }
    catch( BESError &e )
    {
	cerr << "problem: " << e.get_message() << endl ;
	return 1 ;
    }
    catch( ... )
    {
	cerr << "unknown problem:" << endl ;
	return 1 ;
    }

    BESGlobalIQ::BESGlobalQuit() ;

    return 0 ;
}

