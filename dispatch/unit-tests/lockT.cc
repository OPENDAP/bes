// lockT.C

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
#include <cstdlib>

using std::cerr ;
using std::cout ;
using std::endl ;

#include "BESCache.h"
#include "BESError.h"
#include "TheBESKeys.h"
#include <test_config.h>

class lockT: public TestFixture {
private:

public:
    lockT() {}
    ~lockT() {}

    void setUp()
    {
	string bes_conf = (string)TEST_SRC_DIR + "/bes.conf" ;
	TheBESKeys::ConfigFile = bes_conf ;
    } 

    void tearDown()
    {
    }

    CPPUNIT_TEST_SUITE( lockT ) ;

    CPPUNIT_TEST( do_test ) ;

    CPPUNIT_TEST_SUITE_END() ;

    void do_test()
    {
	cout << "*****************************************" << endl;
	cout << "Entered lockT::run" << endl;

	try
	{
	    string cache_dir = (string)TEST_SRC_DIR + "/cache" ;
	    BESCache cache( cache_dir, "lock_test", 1 ) ;

	    cout << "*****************************************" << endl;
	    cout << "lock, then try to lock again" << endl;
	    try
	    {
		cout << "    get first lock" << endl ;
		CPPUNIT_ASSERT( cache.lock( 2, 10 ) ) ;
	    }
	    catch( BESError &e )
	    {
		cerr << e.get_message() << endl ;
		cache.unlock() ;
		CPPUNIT_ASSERT( !"locking test failed" ) ;
	    }

	    try
	    {
		cout << "    try to lock again" << endl ;
		CPPUNIT_ASSERT( cache.lock( 2, 10 ) == false ) ;
	    }
	    catch( BESError &e )
	    {
		cout << e.get_message() << endl ;
		cout << "failed to get lock, good" << endl ;
	    }

	    cout << "*****************************************" << endl;
	    cout << "unlock" << endl;
	    CPPUNIT_ASSERT( cache.unlock() ) ;

	    cout << "*****************************************" << endl;
	    cout << "lock the cache, create another cache, try to lock" << endl;
	    try
	    {
		cout << "    locking first" << endl;
		CPPUNIT_ASSERT( cache.lock( 2, 10 ) ) ;
	    }
	    catch( BESError &e )
	    {
		cerr << e.get_message() << endl ;
		cache.unlock() ;
		CPPUNIT_ASSERT( !"2 cache locking failed" ) ;
	    }

	    cout << "    creating second" << endl;
	    BESCache cache2( cache_dir, "lock_test", 1 ) ;
	    try
	    {
		CPPUNIT_ASSERT( cache2.lock( 2, 10 ) == false ) ;
	    }
	    catch( BESError &e )
	    {
		cerr << e.get_message() << endl ;
		cache.unlock() ;
		CPPUNIT_ASSERT( !"cache 2 locking failed" ) ;
	    }

	    cout << "*****************************************" << endl;
	    cout << "unlock the first cache" << endl;
	    CPPUNIT_ASSERT( cache.unlock() ) ;

	    cout << "*****************************************" << endl;
	    cout << "lock the second cache" << endl;
	    try
	    {
		CPPUNIT_ASSERT( cache2.lock( 2, 10 ) ) ;
	    }
	    catch( BESError &e )
	    {
		cerr << e.get_message() << endl ;
		cache.unlock() ;
		CPPUNIT_ASSERT( !"locking second cache failed" ) ;
	    }

	    cout << "*****************************************" << endl;
	    cout << "unlock the second cache" << endl;
	    CPPUNIT_ASSERT( cache2.unlock() ) ;
	}
	catch( BESError &e )
	{
	    cerr << e.get_message() << endl ;
	    CPPUNIT_ASSERT( !"Failed to use the cache" ) ;
	}

	cout << "*****************************************" << endl;
	cout << "Returning from lockT::run" << endl;
    }
} ;

CPPUNIT_TEST_SUITE_REGISTRATION( lockT ) ;

int 
main( int, char** )
{
    CppUnit::TextTestRunner runner ;
    runner.addTest( CppUnit::TestFactoryRegistry::getRegistry().makeTest() ) ;

    bool wasSuccessful = runner.run( "", false )  ;

    return wasSuccessful ? 0 : 1 ;
}

