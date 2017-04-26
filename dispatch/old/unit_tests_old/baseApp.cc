// baseApp.C

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

using std::cerr ;
using std::endl ;

#include "../unit_tests_old/baseApp.h"
#include "BESGlobalIQ.h"
#include "BESError.h"

Application *Application::_theApplication = 0 ;

baseApp::
baseApp(void)
{
    Application::_theApplication = this ;
}

baseApp::
~baseApp(void)
{
    Application::_theApplication = 0 ;
}

int baseApp::
main( int argC, char **argV )
{
    int retVal = 0 ;
    if((retVal = initialize(argC, argV)) != 0)
    {
	cerr << "baseApp::initialize - failed" << endl ;
    }
    else
    {
	retVal = run() ;
	retVal = terminate(retVal) ;
    }
    return retVal ;
}

int baseApp::
initialize( int argC, char **argV )
{
    int retVal = 0 ;

    // set the locale for the applications, currently only C and POSIX are
    // supported
    setlocale( LC_ALL, "" ) ;

    // initialize application information
    try
    {
	if( BESGlobalIQ::BESGlobalInit( argC, argV ) != true )
	{
	    retVal = 1 ;
	}
    }
    catch( BESError &e )
    {
	cerr << "Global Initialization failed" << endl ;
	cerr << e.get_message() << endl ;
    }

    return retVal ;
}

int baseApp::
run( void )
{
    return 1 ;
}

int baseApp::
terminate( int sig )
{
    if( sig )
    {
	cerr << "baseApp::terminating with value " << sig << endl ;
    }
    BESGlobalIQ::BESGlobalQuit() ;

    return sig ;
}

