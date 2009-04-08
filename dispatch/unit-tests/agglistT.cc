// agglistT.C

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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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
#include <cstdlib>

using std::cerr ;
using std::cout ;
using std::endl ;

#include "agglistT.h"
#include "BESAggFactory.h"
#include "BESTextInfo.h"
#include "BESError.h"
#include "TestAggServer.h"

int agglistT::
run(void)
{
    cout << endl << "*****************************************" << endl;
    cout << "Entered agglistT::run" << endl;
    int retVal = 0;

    cout << endl << "*****************************************" << endl;
    cout << "Adding three handlers to the list" << endl ;
    try
    {
	BESAggFactory::TheFactory()->add_handler( "h1", agglistT::h1 ) ;
	BESAggFactory::TheFactory()->add_handler( "h2", agglistT::h2 ) ;
	BESAggFactory::TheFactory()->add_handler( "h3", agglistT::h3 ) ;
	cout << "Successfully added three handlers" << endl ;
    }
    catch( BESError &e )
    {
	cerr << "Failed to add aggregation servers to list" << endl ;
	cerr << e.get_message() << endl ;
	return 1 ;
    }

    cout << endl << "*****************************************" << endl;
    cout << "Find the three handlers" << endl ;
    try
    {
	BESAggregationServer *s = 0 ;
	s = BESAggFactory::TheFactory()->find_handler( "h1" ) ;
	if( !s )
	{
	    cerr << "Failed to find handler h1" << endl ;
	    return 1 ;
	}
	else
	{
	    cout << "Successfully found handler h1" << endl ;
	}
	s = BESAggFactory::TheFactory()->find_handler( "h2" ) ;
	if( !s )
	{
	    cerr << "Failed to find handler h2" << endl ;
	    return 1 ;
	}
	else
	{
	    cout << "Successfully found handler h2" << endl ;
	}
	s = BESAggFactory::TheFactory()->find_handler( "h3" ) ;
	if( !s )
	{
	    cerr << "Failed to find handler h3" << endl ;
	    return 1 ;
	}
	else
	{
	    cout << "Successfully found handler h3" << endl ;
	}
    }
    catch( BESError &e )
    {
	cerr << "Failed to find aggregation servers" << endl ;
	cerr << e.get_message() << endl ;
    }

    cout << endl << "*****************************************" << endl;
    cout << "Remove handler h2" << endl ;
    try
    {
	bool removed = BESAggFactory::TheFactory()->remove_handler( "h2" ) ;
	if( removed )
	{
	    cout << "Successfully removed handler h2" << endl ;
	}
	else
	{
	    cerr << "Failed to remove handler h2" << endl ;
	    return 1 ;
	}
    }
    catch( BESError &e )
    {
	cerr << "Failed to remove aggregation server h2" << endl ;
	cerr << e.get_message() << endl ;
	return 1 ;
    }

    cout << endl << "*****************************************" << endl;
    cout << "Find the two handlers" << endl ;
    try
    {
	BESAggregationServer *s = 0 ;
	s = BESAggFactory::TheFactory()->find_handler( "h1" ) ;
	if( !s )
	{
	    cerr << "Failed to find handler h1" << endl ;
	    return 1 ;
	}
	else
	{
	    cout << "Successfully found handler h1" << endl ;
	}
	s = BESAggFactory::TheFactory()->find_handler( "h2" ) ;
	if( !s )
	{
	    cout << "Failed to find handler h2, good" << endl ;
	}
	else
	{
	    cout << "Successfully found handler h2, should not have" << endl ;
	    return 1 ;
	}
	s = BESAggFactory::TheFactory()->find_handler( "h3" ) ;
	if( !s )
	{
	    cerr << "Failed to find handler h3" << endl ;
	    return 1 ;
	}
	else
	{
	    cout << "Successfully found handler h3" << endl ;
	}
    }
    catch( BESError &e )
    {
	cerr << "Failed to find aggregation servers" << endl ;
	cerr << e.get_message() << endl ;
    }

    cout << endl << "*****************************************" << endl;
    cout << "Show handler names registered" << endl;
    cout << BESAggFactory::TheFactory()->get_handler_names() << endl ;

    cout << endl << "*****************************************" << endl;
    cout << "Returning from agglistT::run" << endl;

    return retVal;
}

BESAggregationServer *
agglistT::h1( string name )
{
    return new TestAggServer( name ) ;
}

BESAggregationServer *
agglistT::h2( string name )
{
    return new TestAggServer( name ) ;
}

BESAggregationServer *
agglistT::h3( string name )
{
    return new TestAggServer( name ) ;
}

int
main(int argC, char **argV) {
    Application *app = new agglistT();
    putenv( "BES_CONF=./persistence_file_test.ini" ) ;
    return app->main(argC, argV);
}

