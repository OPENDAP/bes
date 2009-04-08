// resplistT.C

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

using std::cerr ;
using std::cout ;
using std::endl ;

#include "resplistT.h"
#include "BESResponseHandlerList.h"
#include "TestResponseHandler.h"

int resplistT::
run(void) {
    cout << endl << "*****************************************" << endl;
    cout << "Entered resplistT::run" << endl;
    int retVal = 0;

    cout << endl << "*****************************************" << endl;
    cout << "add the 5 response handlers" << endl ;
    BESResponseHandlerList *rhl = BESResponseHandlerList::TheList() ;
    char num[10] ;
    for( int i = 0; i < 5; i++ )
    {
	sprintf( num, "resp%d", i ) ;
	if( rhl->add_handler( num, TestResponseHandler::TestResponseBuilder ) == true )
	{
	    cout << "successfully added " << num << endl ;
	}
	else
	{
	    cerr << "failed to add " << num << endl ;
	    return 1 ;
	}
    }

    cout << endl << "*****************************************" << endl;
    cout << "try to add resp3 again" << endl ;
    if( rhl->add_handler( "resp3", TestResponseHandler::TestResponseBuilder ) == true )
    {
	cerr << "successfully added resp3 again" << endl ;
	return 1 ;
    }
    else
    {
	cout << "failed to add resp3 again, good" << endl ;
    }

    cout << endl << "*****************************************" << endl;
    cout << "finding the handlers" << endl ;
    for( int i = 4; i >= 0; i-- )
    {
	sprintf( num, "resp%d", i ) ;
	BESResponseHandler *rh = rhl->find_handler( num ) ;
	if( rh )
	{
	    if( rh->get_name() == num )
	    {
		cout << "found " << num << endl ;
		delete rh ;
	    }
	    else
	    {
		cerr << "looking for " << num
		     << ", found " << rh->get_name() << endl ;
		delete rh ;
		return 1 ;
	    }
	}
	else
	{
	    cerr << "coundn't find " << num << endl ;
	    return 1 ;
	}
    }

    cout << endl << "*****************************************" << endl;
    cout << "removing resp2" << endl ;
    if( rhl->remove_handler( "resp2" ) == true )
    {
	BESResponseHandler *rh = rhl->find_handler( "resp2" ) ;
	if( rh )
	{
	    if( rh->get_name() == "resp2" )
	    {
		cerr << "remove successful, but found resp2" << endl ;
		delete rh ;
		return 1 ;
	    }
	    else
	    {
		cerr << "remove successful, but found not resp2 but "
		     << rh->get_name() << endl ;
		delete rh ;
		return 1 ;
	    }
	}
	else
	{
	    cout << "successfully removed resp2" << endl ;
	}
    }
    else
    {
	cerr << "failed to remove resp2" << endl ;
	return 1 ;
    }

    if( rhl->add_handler( "resp2", TestResponseHandler::TestResponseBuilder ) == true )
    {
	cout << "successfully added resp2 back" << endl ;
    }
    else
    {
	cerr << "failed to add resp2 back" << endl ;
	return 1 ;
    }

    BESResponseHandler *rh = rhl->find_handler( "resp2" ) ;
    if( rh )
    {
	if( rh->get_name() == "resp2" )
	{
	    cout << "found resp2" << endl ;
	    delete rh ;
	}
	else
	{
	    cerr << "looking for resp2, found " << rh->get_name() << endl ;
	    delete rh ;
	    return 1 ;
	}
    }
    else
    {
	cerr << "coundn't find resp2" << endl ;
	return 1 ;
    }

    cout << endl << "*****************************************" << endl;
    cout << "Returning from resplistT::run" << endl;

    return retVal;
}

int
main(int argC, char **argV) {
    Application *app = new resplistT();
    return app->main(argC, argV);
}

