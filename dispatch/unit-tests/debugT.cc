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

#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>

using namespace CppUnit ;

#include <unistd.h>  // for access
#include <iostream>
#include <sstream>
#include <cstdlib>

using std::cerr ;
using std::cout ;
using std::endl ;
using std::ostringstream ;

#include "BESDebug.h"
#include "BESError.h"
#include "BESUtil.h"
#include "TheBESKeys.h"
#include <test_config.h>

string DebugArgs ;

class debugT: public TestFixture {
private:

public:
    debugT() {}
    ~debugT() {}

    void setUp()
    {
	string bes_conf = (string)TEST_SRC_DIR + "/bes.conf" ;
	TheBESKeys::ConfigFile = bes_conf ;
    } 

    void tearDown()
    {
    }

    CPPUNIT_TEST_SUITE( debugT ) ;

    CPPUNIT_TEST( do_test ) ;

    CPPUNIT_TEST_SUITE_END() ;

    void compare_debug( string result, string expected )
    {
	if( !expected.empty() )
	{
	    string::size_type lb = result.find( "[" ) ;
	    CPPUNIT_ASSERT( lb != string::npos ) ;
	    string::size_type rb = result.rfind( "]" ) ;
	    CPPUNIT_ASSERT( rb != string::npos ) ;
	    result = result.substr( rb+2 ) ;
	}
	CPPUNIT_ASSERT( result == expected ) ;
    }

    void do_test()
    {
	cout << "*****************************************" << endl;
	cout << "Entered debugT::run" << endl;

	char mypid[12] ;
	BESUtil::fastpidconverter( mypid, 10 ) ;

	if( !DebugArgs.empty() )
	{
	    cout << "*****************************************" << endl;
	    cout << "trying " << DebugArgs << endl;
	    try
	    {
		BESDebug::SetUp( DebugArgs ) ;
	    }
	    catch( BESError &e )
	    {
		cerr << e.get_message() << endl ;
		CPPUNIT_ASSERT( !"Failed to set up Debug" ) ;
	    }
	}
	else
	{
	    try
	    {
		cout << "*****************************************" << endl;
		cout << "Setting up with bad file name /bad/dir/debug" << endl;
		BESDebug::SetUp( "/bad/dir/debug,nc" ) ;
		CPPUNIT_ASSERT( !"Successfully set up with bad file" ) ;
	    }
	    catch( BESError &e )
	    {
		cout << "Unable to set up debug ... good" << endl ;
		cout << e.get_message() << endl ;
	    }

	    try
	    {
		cout << "*****************************************" << endl;
		cout << "Setting up myfile.debug,nc,cdf,hdf4" << endl;
		BESDebug::SetUp( "myfile.debug,nc,cdf,hdf4" ) ;
		int result = access( "myfile.debug", W_OK|R_OK ) ;
		CPPUNIT_ASSERT( result != -1 ) ;
		CPPUNIT_ASSERT( BESDebug::IsSet( "nc" ) ) ;
		CPPUNIT_ASSERT( BESDebug::IsSet( "cdf" ) ) ;
		CPPUNIT_ASSERT( BESDebug::IsSet( "hdf4" ) ) ;

		BESDebug::SetStrm( 0, false ) ;
		result = remove( "myfile.debug" ) ;
		CPPUNIT_ASSERT( result != -1 ) ;
	    }
	    catch( BESError &e )
	    {
		CPPUNIT_ASSERT( !"Unable to set up debug" ) ;
	    }

	    try
	    {
		cout << "*****************************************" << endl;
		cout << "Setting up cerr,ff,-cdf" << endl;
		BESDebug::SetUp( "cerr,ff,-cdf" ) ;
		CPPUNIT_ASSERT( BESDebug::IsSet( "ff" ) ) ;
		CPPUNIT_ASSERT( !BESDebug::IsSet( "cdf" ) ) ;
	    }
	    catch( BESError &e )
	    {
		CPPUNIT_ASSERT( !"Unable to set up debug" ) ;
	    }

	    cout << "*****************************************" << endl;
	    cout << "try debugging to nc" << endl;
	    ostringstream nc ;
	    BESDebug::SetStrm( &nc, false ) ;
	    string debug_str = "Testing nc debug" ;
	    BESDEBUG( "nc", debug_str ) ;
	    compare_debug( nc.str(), debug_str ) ;

	    cout << "*****************************************" << endl;
	    cout << "try debugging to hdf4" << endl;
	    ostringstream hdf4 ;
	    BESDebug::SetStrm( &hdf4, false ) ;
	    debug_str = "Testing hdf4 debug" ;
	    BESDEBUG( "hdf4", debug_str ) ;
	    compare_debug( hdf4.str(), debug_str ) ;

	    cout << "*****************************************" << endl;
	    cout << "try debugging to ff" << endl;
	    ostringstream ff ;
	    BESDebug::SetStrm( &ff, false ) ;
	    debug_str = "Testing ff debug" ;
	    BESDEBUG( "ff", debug_str ) ;
	    compare_debug( ff.str(), debug_str ) ;

	    cout << "*****************************************" << endl;
	    cout << "turn off ff and try debugging to ff again" << endl;
	    BESDebug::Set( "ff", false ) ;
	    CPPUNIT_ASSERT( !BESDebug::IsSet( "ff" ) ) ;
	    ostringstream ff2 ;
	    BESDebug::SetStrm( &ff2, false ) ;
	    debug_str = "" ;
	    BESDEBUG( "ff", debug_str ) ;
	    compare_debug( ff2.str(), debug_str ) ;

	    cout << "*****************************************" << endl;
	    cout << "try debugging to cdf" << endl;
	    ostringstream cdf ;
	    BESDebug::SetStrm( &cdf, false ) ;
	    debug_str = "" ;
	    BESDEBUG( "cdf", debug_str ) ;
	    compare_debug( cdf.str(), debug_str ) ;
	}

	cout << "*****************************************" << endl;
	cout << "display debug help" << endl;
	BESDebug::Help( cout ) ;

	cout << "*****************************************" << endl;
	cout << "Returning from debugT::run" << endl;
    }
} ;

CPPUNIT_TEST_SUITE_REGISTRATION( debugT ) ;

int 
main( int argC, char **argV )
{
    int c = 0 ;
    while( ( c = getopt( argC, argV, "d:" ) ) != EOF )
    {
	switch( c )
	{
	    case 'd':
		DebugArgs = optarg ;
		break ;
	    default:
		cerr << "bad option to debugT" << endl ;
		cerr << "debugT -d string" << endl ;
		return 1 ;
		break ;
	}
    }

    CppUnit::TextTestRunner runner ;
    runner.addTest( CppUnit::TestFactoryRegistry::getRegistry().makeTest() ) ;

    bool wasSuccessful = runner.run( "", false )  ;

    return wasSuccessful ? 0 : 1 ;
}

