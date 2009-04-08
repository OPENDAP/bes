// debugT.C

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

#include <unistd.h>  // for access
#include <iostream>
#include <sstream>
#include <cstdlib>

using std::cerr ;
using std::cout ;
using std::endl ;
using std::ostringstream ;

#include "debugT.h"
#include "BESDebug.h"
#include "BESError.h"
#include "BESUtil.h"
#include "test_config.h"

int
debugT::run(void)
{
    cout << endl << "*****************************************" << endl;
    cout << "Entered debugT::run" << endl;
    int retVal = 0;

    char mypid[12] ;
    BESUtil::fastpidconverter( mypid, 10 ) ;
    string pid_str = (string)"[" + mypid + "] " ;

    if( !_tryme.empty() )
    {
	cout << endl << "*****************************************" << endl;
	cout << "trying " << _tryme << endl;
    }
    else
    {
	try
	{
	    cout << endl << "*****************************************" << endl;
	    cout << "Setting up with bad file name /bad/dir/debug" << endl;
	    BESDebug::SetUp( "/bad/dir/debug,nc" ) ;
	    cerr << "Successfully set up, shouldn't have" << endl ;
	    return 1 ;
	}
	catch( BESError &e )
	{
	    cout << "Unable to set up debug ... good" << endl ;
	}

	try
	{
	    cout << endl << "*****************************************" << endl;
	    cout << "Setting up myfile.debug,nc,cdf,hdf4" << endl;
	    BESDebug::SetUp( "myfile.debug,nc,cdf,hdf4" ) ;
	    cout << "successfully set up" << endl ;
	    int result = access( "myfile.debug", W_OK|R_OK ) ;
	    if( result == -1 )
	    {
		cerr << "File not created" << endl ;
		return 1 ;
	    }
	    BESDebug::SetStrm( 0, false ) ;
	    result = remove( "myfile.debug" ) ;
	    if( result == -1 )
	    {
		cerr << "Unable to remove the debug file" << endl ;
		return 1 ;
	    }
	}
	catch( BESError &e )
	{
	    cerr << "Unable to set up debug ... should have worked" << endl ;
	    return 1 ;
	}

	try
	{
	    cout << endl << "*****************************************" << endl;
	    cout << "Setting up cerr,ff" << endl;
	    BESDebug::SetUp( "cerr,ff,-cdf" ) ;
	    cout << "Successfully set up" << endl ;
	}
	catch( BESError &e )
	{
	    cerr << "Unable to set up debug ... should have worked" << endl ;
	    return 1 ;
	}

	cout << endl << "*****************************************" << endl;
	cout << "try debugging to nc" << endl;
	ostringstream nc ;
	BESDebug::SetStrm( &nc, false ) ;
	string debug_str = "Testing nc debug" ;
	string result_str = pid_str + debug_str ;
	BESDEBUG( "nc", debug_str ) ;
	if( nc.str() != result_str )
	{
	    cerr << "incorrect debug information: " << nc.str() << endl ;
	    cerr << "should be: " << result_str << endl ;
	    return 1 ;
	}

	cout << endl << "*****************************************" << endl;
	cout << "try debugging to hdf4" << endl;
	ostringstream hdf4 ;
	BESDebug::SetStrm( &hdf4, false ) ;
	debug_str = "Testing hdf4 debug" ;
	result_str = pid_str + debug_str ;
	BESDEBUG( "hdf4", debug_str ) ;
	if( hdf4.str() != result_str )
	{
	    cerr << "incorrect debug information: " << hdf4.str() << endl ;
	    cerr << "should be: " << result_str << endl ;
	    return 1 ;
	}

	cout << endl << "*****************************************" << endl;
	cout << "try debugging to ff" << endl;
	ostringstream ff ;
	BESDebug::SetStrm( &ff, false ) ;
	debug_str = pid_str + "Testing ff debug" ;
	result_str = pid_str + debug_str ;
	BESDEBUG( "ff", debug_str ) ;
	if( ff.str() != result_str )
	{
	    cerr << "incorrect debug information: " << ff.str() << endl ;
	    cerr << "should be: " << result_str << endl ;
	    return 1 ;
	}

	cout << endl << "*****************************************" << endl;
	cout << "turn off ff and try debugging to ff again" << endl;
	BESDebug::Set( "ff", false ) ;
	ostringstream ff2 ;
	BESDebug::SetStrm( &ff2, false ) ;
	debug_str = "" ;
	BESDEBUG( "ff", debug_str ) ;
	if( ff2.str() != debug_str )
	{
	    cerr << "incorrect debug information: " << ff2.str() << endl ;
	    return 1 ;
	}

	cout << endl << "*****************************************" << endl;
	cout << "try debugging to cdf" << endl;
	ostringstream cdf ;
	BESDebug::SetStrm( &cdf, false ) ;
	debug_str = "" ;
	BESDEBUG( "cdf", debug_str ) ;
	if( cdf.str() != debug_str )
	{
	    cerr << "incorrect debug information: " << cdf.str() << endl ;
	    return 1 ;
	}
    }

    cout << endl << "*****************************************" << endl;
    cout << "display debug help" << endl;
    BESDebug::Help( cout ) ;

    cout << endl << "*****************************************" << endl;
    cout << "Returning from debugT::run" << endl;

    return retVal;
}

int
main(int argC, char **argV) {
    int c = 0 ;
    string tryme ;
    while( ( c = getopt( argC, argV, "d:" ) ) != EOF )
    {
	switch( c )
	{
	    case 'd':
		tryme = optarg ;
		break ;
	    default:
		cerr << "bad option to debugT" << endl ;
		cerr << "debugT -d string" << endl ;
		return 1 ;
		break ;
	}
    }
    string env_var = (string)"BES_CONF=" + TEST_SRC_DIR + "/bes.conf" ;
    putenv( (char *)env_var.c_str() ) ;

    Application *app = new debugT( tryme );
    return app->main(argC, argV);
}

