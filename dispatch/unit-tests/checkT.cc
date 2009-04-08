// checkT.C

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

#include "config.h"

#include <sys/types.h>
#include <sys/stat.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <cerrno>
#include <cstdlib>
#include <iostream>
#include <fstream>

using std::cerr ;
using std::cout ;
using std::endl ;
using std::ifstream ;

#include "checkT.h"
#include "BESUtil.h"
#include "BESError.h"
#include "test_config.h"

int
checkT::run(void)
{
    cout << endl << "*****************************************" << endl;
    cout << "Entered checkT::run" << endl;
    int retVal = 0;

    cout << endl << "*****************************************" << endl;
    cout << "create the desired directory structure" << endl;
    // testdir
    // testdir/nc
    // testdir/link_to_nc -> testdir/nc
    // testdir/nc/testfile.nc
    // testdir/nc/link_to_testfile.nc -> testdir/nc/testfile.nc
    int ret = mkdir( "./testdir", S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH ) ;
    if( ret == -1  && errno != EEXIST )
    {
	cerr << "Failed to create the test directory" << endl ;
	return 1 ;
    }
    ret = mkdir( "./testdir/nc", S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH ) ;
    if( ret == -1  && errno != EEXIST )
    {
	cerr << "Failed to create the test directory" << endl ;
	return 1 ;
    }
    FILE *fp = fopen( "./testdir/nc/testfile.nc", "w+" ) ;
    if( !fp )
    {
	cerr << "Failed to create the test file" << endl ;
	return 1 ;
    }
    fprintf( fp, "This is a test file" ) ;
    fclose( fp ) ;
    ret = symlink( "./nc", "./testdir/link_to_nc" ) ;
    if( ret == -1 && errno != EEXIST )
    {
	cerr << "Failed to create symbolic link to nc directory" << endl ;
	return 1 ;
    }
    ret = symlink( "./testfile.nc", "./testdir/nc/link_to_testfile.nc" ) ;
    if( ret == -1 && errno != EEXIST )
    {
	cerr << "Failed to create symbolic link to test file" << endl ;
	return 1 ;
    }

    cout << endl << "*****************************************" << endl;
    cout << "checking /testdir/" << endl;
    try
    {
	BESUtil::check_path( "/testdir/", "./", true ) ;
    }
    catch( BESError &e )
    {
	cerr << "check failed for /testdir/" << endl ;
	cerr << e.get_message() << endl ;
	return 1 ;
    }

    cout << endl << "*****************************************" << endl;
    cout << "checking /testdir/nc/" << endl;
    try
    {
	BESUtil::check_path( "/testdir/nc/", "./", true ) ;
    }
    catch( BESError &e )
    {
	cerr << "check failed for /testdir/nc/" << endl ;
	cerr << e.get_message() << endl ;
	return 1 ;
    }

    cout << endl << "*****************************************" << endl;
    cout << "checking /testdir/nc/testfile.nc" << endl;
    try
    {
	BESUtil::check_path( "/testdir/nc/testfile.nc", "./", true ) ;
    }
    catch( BESError &e )
    {
	cerr << "check failed for /testdir/nc/testfile.nc" << endl ;
	cerr << e.get_message() << endl ;
	return 1 ;
    }

    cout << endl << "*****************************************" << endl;
    cout << "checking /testdir/link_to_nc/" << endl;
    try
    {
	BESUtil::check_path( "/testdir/link_to_nc/", "./", true ) ;
    }
    catch( BESError &e )
    {
	cerr << "check failed for /testdir/link_to_nc/" << endl ;
	cerr << e.get_message() << endl ;
	return 1 ;
    }

    cout << endl << "*****************************************" << endl;
    cout << "checking /testdir/link_to_nc/link_to_testfile.nc" << endl;
    try
    {
	BESUtil::check_path( "/testdir/link_to_nc/link_to_testfile.nc",
			     "./", true ) ;
    }
    catch( BESError &e )
    {
	cerr << "check failed for /testdir/link_to_nc/link_to_testfile.nc"
	     << endl ;
	cerr << e.get_message() << endl ;
	return 1 ;
    }

    cout << endl << "*****************************************" << endl;
    cout << "checking /testdir/, folllow syms = false" << endl;
    try
    {
	BESUtil::check_path( "/testdir/", "./", false ) ;
    }
    catch( BESError &e )
    {
	cerr << "check failed for /testdir/" << endl ;
	cerr << e.get_message() << endl ;
	return 1 ;
    }

    cout << endl << "*****************************************" << endl;
    cout << "checking /testdir/nc/, folllow syms = false" << endl;
    try
    {
	BESUtil::check_path( "/testdir/nc/", "./", false ) ;
    }
    catch( BESError &e )
    {
	cerr << "check failed for /testdir/nc/" << endl ;
	cerr << e.get_message() << endl ;
	return 1 ;
    }

    cout << endl << "*****************************************" << endl;
    cout << "checking /testdir/nc/testfile.nc, folllow syms = false" << endl;
    try
    {
	BESUtil::check_path( "/testdir/nc/testfile.nc", "./", false ) ;
    }
    catch( BESError &e )
    {
	cerr << "check failed for /testdir/nc/testfile.nc" << endl ;
	cerr << e.get_message() << endl ;
	return 1 ;
    }

    cout << endl << "*****************************************" << endl;
    cout << "checking /testdir/link_to_nc/, follow_syms = false" << endl;
    try
    {
	BESUtil::check_path( "/testdir/link_to_nc/", "./", false ) ;
	cerr << "check succeeded, should have failed" << endl ;
	return 1 ;
    }
    catch( BESError &e )
    {
	cout << "err = " << e.get_message() << endl ;
    }

    cout << endl << "*****************************************" << endl;
    cout << "checking /testdir/link_to_nc/link_to_testfile.nc, "
         << "follow syms = false" << endl;
    try
    {
	BESUtil::check_path( "/testdir/link_to_nc/link_to_testfile.nc",
                              "./", false ) ;
	cerr << "check succeeded, show have failed" << endl ;
	return 1 ;
    }
    catch( BESError &e )
    {
	cout << "err = " << e.get_message() << endl ;
    }

    cout << endl << "*****************************************" << endl;
    cout << "checking /testdir/nc/link_to_testfile.nc, "
         << "follow syms = false" << endl;
    try
    {
	BESUtil::check_path( "/testdir/nc/link_to_testfile.nc",
                              "./", false ) ;
	cerr << "check succeeded, show have failed" << endl ;
	return 1 ;
    }
    catch( BESError &e )
    {
	cout << "err = " << e.get_message() << endl ;
    }

    cout << endl << "*****************************************" << endl;
    cout << "checking /nodir/" << endl;
    try
    {
	BESUtil::check_path( "/nodir/", "./", true ) ;
	cerr << "check succeeded, should have failed" << endl ;
	return 1 ;
    }
    catch( BESError &e )
    {
	cout << "err = " << e.get_message() << endl ;
    }

    cout << endl << "*****************************************" << endl;
    cout << "checking /nodir/, follow syms = false" << endl;
    try
    {
	BESUtil::check_path( "/nodir/", "./", false ) ;
	cerr << "check succeeded, should have failed" << endl ;
	return 1 ;
    }
    catch( BESError &e )
    {
	cout << "err = " << e.get_message() << endl ;
    }

    cout << endl << "*****************************************" << endl;
    cout << "checking /testdir/nodir/" << endl;
    try
    {
	BESUtil::check_path( "/testdir/nodir/", "./", true ) ;
	cerr << "check succeeded, should have failed" << endl ;
	return 1 ;
    }
    catch( BESError &e )
    {
	cout << "err = " << e.get_message() << endl ;
    }

    cout << endl << "*****************************************" << endl;
    cout << "checking /testdir/nc/nofile.nc" << endl;
    try
    {
	BESUtil::check_path( "/testdir/nc/nofile.nc", "./", true ) ;
	cerr << "check succeeded, should have failed" << endl ;
	return 1 ;
    }
    catch( BESError &e )
    {
	cout << "err = " << e.get_message() << endl ;
    }

    cout << endl << "*****************************************" << endl;
    cout << "checking /testdir/link_to_nc/nofile.nc" << endl;
    try
    {
	BESUtil::check_path( "/testdir/link_to_nc/nofile.nc", "./", true ) ;
	cerr << "check succeeded, should have failed" << endl ;
	return 1 ;
    }
    catch( BESError &e )
    {
	cout << "err = " << e.get_message() << endl ;
    }

    cout << endl << "*****************************************" << endl;
    cout << "checking /testdir/link_to_nc/nofile.nc, follow syms = false"
         << endl;
    try
    {
	BESUtil::check_path( "/testdir/link_to_nc/nofile.nc", "./", false ) ;
	cerr << "check succeeded, should have failed" << endl ;
	return 1 ;
    }
    catch( BESError &e )
    {
	cout << "err = " << e.get_message() << endl ;
    }

    cout << endl << "*****************************************" << endl;
    cout << "Returning from checkT::run" << endl;

    return retVal;
}

int
main(int argC, char **argV) {
    string env_var = (string)"BES_CONF=" + TEST_SRC_DIR + "/bes.conf" ;
    putenv( (char *)env_var.c_str() ) ;
    Application *app = new checkT();
    return app->main(argC, argV);
}

