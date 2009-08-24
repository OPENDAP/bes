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

#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>

using namespace CppUnit ;

#include <iostream>
#include <cstring>
#include <limits.h>

using std::cerr ;
using std::cout ;
using std::endl ;

#include "BESScrub.h"
#include "BESError.h"

class connT: public TestFixture {
private:

public:
    connT() {}
    ~connT() {}

    void setUp()
    {
    } 

    void tearDown()
    {
    }

    CPPUNIT_TEST_SUITE( connT ) ;

    CPPUNIT_TEST( do_test ) ;

    CPPUNIT_TEST_SUITE_END() ;

    void do_test()
    {
	cout << "*****************************************" << endl;
	cout << "Entered scrubT::run" << endl;

	try
	{
	    cout << "*****************************************" << endl;
	    cout << "Test command line length over 255 characters" << endl;
	    char arg[512] ;
	    memset( arg, 'a', 300 ) ;
	    arg[300] = '\0' ;
	    CPPUNIT_ASSERT( !BESScrub::command_line_arg_ok( arg ) ) ;
	}
	catch( BESError &e )
	{
	    cerr << e.get_message() << endl ;
	    CPPUNIT_ASSERT( !"scrub failed" ) ;
	}

	try
	{
	    cout << "*****************************************" << endl;
	    cout << "Test command line length ok" << endl;
	    string arg = "anarg" ;
	    CPPUNIT_ASSERT( BESScrub::command_line_arg_ok( arg ) ) ;
	}
	catch( BESError &e )
	{
	    cerr << e.get_message() << endl ;
	    CPPUNIT_ASSERT( !"scrub failed" ) ;
	}

	try
	{
	    cout << "*****************************************" << endl;
	    cout << "Test path name length over 255 characters" << endl;
	    char path_name[512] ;
	    memset( path_name, 'a', 300 ) ;
	    path_name[300] = '\0' ;
	    CPPUNIT_ASSERT( !BESScrub::pathname_ok( path_name, true ) ) ;
	}
	catch( BESError &e )
	{
	    cerr << e.get_message() << endl ;
	    CPPUNIT_ASSERT( !"scrub failed" ) ;
	}

	try
	{
	    cout << "*****************************************" << endl;
	    cout << "Test path name good" << endl;
	    CPPUNIT_ASSERT( BESScrub::pathname_ok( "/usr/local/something_goes_here-and-is-ok.txt", true ) ) ;
	}
	catch( BESError &e )
	{
	    cerr << e.get_message() << endl ;
	    CPPUNIT_ASSERT( !"scrub failed" ) ;
	}

	try
	{
	    cout << "*****************************************" << endl;
	    cout << "Test path name bad characters strict" << endl;
	    CPPUNIT_ASSERT( !BESScrub::pathname_ok( "*$^&;@/user/local/bin/ls", true ) ) ;
	}
	catch( BESError &e )
	{
	    cerr << e.get_message() << endl ;
	    CPPUNIT_ASSERT( !"scrub failed" ) ;
	}

	try
	{
	    cout << "*****************************************" << endl;
	    cout << "Test array size too big" << endl;
	    CPPUNIT_ASSERT( !BESScrub::size_ok( 4, UINT_MAX ) ) ;
	}
	catch( BESError &e )
	{
	    cerr << e.get_message() << endl ;
	    CPPUNIT_ASSERT( !"scrub failed" ) ;
	}

	try
	{
	    cout << "*****************************************" << endl;
	    cout << "Test array size ok" << endl;
	    CPPUNIT_ASSERT( BESScrub::size_ok( 4, 32 ) ) ;
	}
	catch( BESError &e )
	{
	    cerr << e.get_message() << endl ;
	    CPPUNIT_ASSERT( !"scrub failed" ) ;
	}

	cout << "*****************************************" << endl;
	cout << "Returning from scrubT::run" << endl;
    }
} ;

CPPUNIT_TEST_SUITE_REGISTRATION( connT ) ;

int 
main( int, char** )
{
    CppUnit::TextTestRunner runner ;
    runner.addTest( CppUnit::TestFactoryRegistry::getRegistry().makeTest() ) ;

    bool wasSuccessful = runner.run( "", false )  ;

    return wasSuccessful ? 0 : 1 ;
}

