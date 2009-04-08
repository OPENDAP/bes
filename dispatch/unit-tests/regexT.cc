// regexT.C

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

#include "regexT.h"
#include "BESRegex.h"
#include "BESError.h"

int regexT::
run(void)
{
    cout << endl << "*****************************************" << endl;
    cout << "Entered regexT::run" << endl;
    int retVal = 0;

    try
    {
	cout << endl << "*****************************************" << endl;
	cout << "Match reg ex 123456 against string 01234567" << endl;
	BESRegex reg_expr( "123456" ) ;
	string inQuestion = "01234567" ;
	int result = reg_expr.match( inQuestion.c_str(), inQuestion.length() ) ;
	if( result == 6 )
	{
	    cout << "Successfully matched characters" << endl ;
	}
	else
	{
	    cerr << "Regular expression matched " << result
	         << " characets, should have matched 6" << endl ;
	    return 1 ;
	}
    }
    catch( BESError &e )
    {
	cerr << "Failed to match, exception caught" << endl << e.get_message() << endl ;
	return 1 ;
    }
    catch( ... )
    {
	cerr << "Failed to match, unknown exception caught" << endl ;
	return 1 ;
    }

    try
    {
	cout << endl << "*****************************************" << endl;
	cout << "Match reg ex ^123456$ against string 01234567" << endl;
	BESRegex reg_expr( "^123456$" ) ;
	string inQuestion = "01234567" ;
	int result = reg_expr.match( inQuestion.c_str(), inQuestion.length() ) ;
	if( result == -1 )
	{
	    cout << "Does not match, good" << endl ;
	}
	else
	{
	    cerr << "Regular expression matched " << result
	         << " characets, should have matched none" << endl ;
	    return 1 ;
	}
    }
    catch( BESError &e )
    {
	cerr << "Failed to match, exception caught" << endl << e.get_message() << endl ;
	return 1 ;
    }
    catch( ... )
    {
	cerr << "Failed to match, unknown exception caught" << endl ;
	return 1 ;
    }

    try
    {
	cout << endl << "*****************************************" << endl;
	cout << "Match reg ex ^123456$ against string 123456" << endl;
    cout << "    besregtest include \"^123456$;\" 123456 matches all 6 of 6 characters" << endl ;
	BESRegex reg_expr( "^123456$" ) ;
	string inQuestion = "123456" ;
	int result = reg_expr.match( inQuestion.c_str(), inQuestion.length() ) ;
	if( result == 6 )
	{
	    cout << "Successfully matched characters" << endl ;
	}
	else
	{
	    cerr << "Regular expression matched " << result
	         << " characets, should have matched 6" << endl ;
	    return 1 ;
	}
    }
    catch( BESError &e )
    {
	cerr << "Failed to match, exception caught" << endl << e.get_message() << endl ;
	return 1 ;
    }
    catch( ... )
    {
	cerr << "Failed to match, unknown exception caught" << endl ;
	return 1 ;
    }

    try
    {
	cout << endl << "*****************************************" << endl;
	cout << "Match reg ex \".*\\.nc$;\" against string fnoc1.nc" << endl;
	BESRegex reg_expr( ".*\\.nc$" ) ;
	string inQuestion = "fnoc1.nc" ;
	int result = reg_expr.match( inQuestion.c_str(), inQuestion.length() ) ;
	if( result == 8 )
	{
	    cout << "Successfully matched characters" << endl ;
	}
	else
	{
	    cerr << "Regular expression matched " << result
	         << " characets, should have matched 8" << endl ;
	    return 1 ;
	}
    }
    catch( BESError &e )
    {
	cerr << "Failed to match, exception caught" << endl << e.get_message() << endl ;
	return 1 ;
    }
    catch( ... )
    {
	cerr << "Failed to match, unknown exception caught" << endl ;
	return 1 ;
    }

    try
    {
	cout << endl << "*****************************************" << endl;
	cout << "Match reg ex \".*\\.nc$;\" against string fnoc1.ncd" << endl;
	BESRegex reg_expr( ".*\\.nc$" ) ;
	string inQuestion = "fnoc1.ncd" ;
	int result = reg_expr.match( inQuestion.c_str(), inQuestion.length() ) ;
	if( result == -1 )
	{
	    cout << "Successfully did not match characters" << endl ;
	}
	else
	{
	    cerr << "Regular expression matched " << result
	         << " characets, should have matched none" << endl ;
	    return 1 ;
	}
    }
    catch( BESError &e )
    {
	cerr << "Failed to match, exception caught" << endl << e.get_message() << endl ;
	return 1 ;
    }
    catch( ... )
    {
	cerr << "Failed to match, unknown exception caught" << endl ;
	return 1 ;
    }

    try
    {
	cout << endl << "*****************************************" << endl;
	cout << "Match reg ex .*\\.(nc|NC)(\\.gz|\\.bz2|\\.Z)?$ against string fnoc1.nc" << endl;
	BESRegex reg_expr( ".*\\.(nc|NC)(\\.gz|\\.bz2|\\.Z)?$" ) ;
	string inQuestion = "fnoc1.nc" ;
	int result = reg_expr.match( inQuestion.c_str(), inQuestion.length() ) ;
	if( result == 8 )
	{
	    cout << "Successfully matched characters" << endl ;
	}
	else
	{
	    cerr << "Regular expression matched " << result
	         << " characets, should have matched 8" << endl ;
	    return 1 ;
	}
    }
    catch( BESError &e )
    {
	cerr << "Failed to match, exception caught" << endl << e.get_message() << endl ;
	return 1 ;
    }
    catch( ... )
    {
	cerr << "Failed to match, unknown exception caught" << endl ;
	return 1 ;
    }

    try
    {
	cout << endl << "*****************************************" << endl;
	cout << "Match reg ex .*\\.(nc|NC)(\\.gz|\\.bz2|\\.Z)?$ against string fnoc1.nc.gz" << endl;
	BESRegex reg_expr( ".*\\.(nc|NC)(\\.gz|\\.bz2|\\.Z)?$" ) ;
	string inQuestion = "fnoc1.nc.gz" ;
	int result = reg_expr.match( inQuestion.c_str(), inQuestion.length() ) ;
	if( result == 11 )
	{
	    cout << "Successfully matched characters" << endl ;
	}
	else
	{
	    cerr << "Regular expression matched " << result
	         << " characets, should have matched 11" << endl ;
	    return 1 ;
	}
    }
    catch( BESError &e )
    {
	cerr << "Failed to match, exception caught" << endl << e.get_message() << endl ;
	return 1 ;
    }
    catch( ... )
    {
	cerr << "Failed to match, unknown exception caught" << endl ;
	return 1 ;
    }

    cout << endl << "*****************************************" << endl;
    cout << "Returning from regexT::run" << endl;

    return retVal;
}

int
main(int argC, char **argV) {
    Application *app = new regexT();
    return app->main(argC, argV);
}

