// scrubT.C

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
#include <cstring>
#include <limits.h>

using std::cerr ;
using std::cout ;
using std::endl ;

#include "scrubT.h"
#include "BESScrub.h"
#include "BESError.h"

int scrubT::
run(void)
{
    cout << endl << "*****************************************" << endl;
    cout << "Entered scrubT::run" << endl;
    int retVal = 0;

    try
    {
	cout << endl << "*****************************************" << endl;
	cout << "Test command line length over 255 characters" << endl;
	char arg[512] ;
	memset( arg, 'a', 300 ) ;
	arg[300] = '\0' ;
	if( BESScrub::command_line_arg_ok( arg ) )
	{
	    cerr << "command line ok, shouldn't be" << endl ;
	    return 1 ;
	}
	else
	{
	    cout << "command line not ok, good" << endl ;
	}
    }
    catch( BESError &e )
    {
	cerr << "caught exception: " << e.get_message() << endl ;
	return 1 ;
    }
    catch( ... )
    {
	cerr << "caught unknown exception" << endl ;
	return 1 ;
    }

    try
    {
	cout << endl << "*****************************************" << endl;
	cout << "Test command line length ok" << endl;
	string arg = "anarg" ;
	if( BESScrub::command_line_arg_ok( arg ) )
	{
	    cout << "command line ok, good" << endl ;
	}
	else
	{
	    cerr << "command line not ok, should be" << endl ;
	    return 1 ;
	}
    }
    catch( BESError &e )
    {
	cerr << "caught exception: " << e.get_message() << endl ;
	return 1 ;
    }
    catch( ... )
    {
	cerr << "caught unknown exception" << endl ;
	return 1 ;
    }

    try
    {
	cout << endl << "*****************************************" << endl;
	cout << "Test path name length over 255 characters" << endl;
	char path_name[512] ;
	memset( path_name, 'a', 300 ) ;
	path_name[300] = '\0' ;
	if( BESScrub::pathname_ok( path_name, true ) )
	{
	    cerr << "path name ok, shouldn't be" << endl ;
	    return 1 ;
	}
	else
	{
	    cout << "path name not ok, good" << endl ;
	}
    }
    catch( BESError &e )
    {
	cerr << "caught exception: " << e.get_message() << endl ;
	return 1 ;
    }
    catch( ... )
    {
	cerr << "caught unknown exception" << endl ;
	return 1 ;
    }

    try
    {
	cout << endl << "*****************************************" << endl;
	cout << "Test path name good" << endl;
	if( BESScrub::pathname_ok( "/usr/local/something_goes_here-and-is-ok.txt", true ) )
	{
	    cout << "path name ok, good" << endl ;
	}
	else
	{
	    cerr << "path name not ok, should be" << endl ;
	    return 1 ;
	}
    }
    catch( BESError &e )
    {
	cerr << "caught exception: " << e.get_message() << endl ;
	return 1 ;
    }
    catch( ... )
    {
	cerr << "caught unknown exception" << endl ;
	return 1 ;
    }

    try
    {
	cout << endl << "*****************************************" << endl;
	cout << "Test path name bad characters strict" << endl;
	if( BESScrub::pathname_ok( "*$^&;@/user/local/bin/ls", true ) )
	{
	    cerr << "path name ok, shouldn't be" << endl ;
	    return 1 ;
	}
	else
	{
	    cout << "path name not ok, good" << endl ;
	}
    }
    catch( BESError &e )
    {
	cerr << "caught exception: " << e.get_message() << endl ;
	return 1 ;
    }
    catch( ... )
    {
	cerr << "caught unknown exception" << endl ;
	return 1 ;
    }

    try
    {
	cout << endl << "*****************************************" << endl;
	cout << "Test array size too big" << endl;
	if( BESScrub::size_ok( 4, UINT_MAX ) )
	{
	    cerr << "array size ok, shouldn't be" << endl ;
	    return 1 ;
	}
	else
	{
	    cout << "array size not ok, good" << endl ;
	}
    }
    catch( BESError &e )
    {
	cerr << "caught exception: " << e.get_message() << endl ;
	return 1 ;
    }
    catch( ... )
    {
	cerr << "caught unknown exception" << endl ;
	return 1 ;
    }

    try
    {
	cout << endl << "*****************************************" << endl;
	cout << "Test array size ok" << endl;
	if( BESScrub::size_ok( 4, 32 ) )
	{
	    cout << "array size ok, good" << endl ;
	}
	else
	{
	    cerr << "array size not ok, should be" << endl ;
	    return 1 ;
	}
    }
    catch( BESError &e )
    {
	cerr << "caught exception: " << e.get_message() << endl ;
	return 1 ;
    }
    catch( ... )
    {
	cerr << "caught unknown exception" << endl ;
	return 1 ;
    }

    cout << endl << "*****************************************" << endl;
    cout << "Returning from scrubT::run" << endl;

    return retVal;
}

int
main(int argC, char **argV) {
    Application *app = new scrubT();
    return app->main(argC, argV);
}

