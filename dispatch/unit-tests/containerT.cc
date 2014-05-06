// containerT.C

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

#include <iostream>
#include <cstdlib>

using std::cerr ;
using std::cout ;
using std::endl ;

#include "TheBESKeys.h"
#include "BESContainerStorageList.h"
#include "BESFileContainer.h"
#include "BESContainerStorage.h"
#include "BESContainerStorageFile.h"
#include "BESFileLockingCache.h"
#include "BESError.h"
#include <test_config.h>

class containerT: public TestFixture {
private:

public:
    containerT() {}
    ~containerT() {}

    void setUp()
    {
	string bes_conf = (string)TEST_SRC_DIR + "/empty.ini" ;
	TheBESKeys::ConfigFile = bes_conf ;
    }

    void tearDown()
    {
    }

    CPPUNIT_TEST_SUITE( containerT ) ;

    CPPUNIT_TEST( do_test ) ;

    CPPUNIT_TEST_SUITE_END() ;

    void do_test()
    {
	cout << "*****************************************" << endl;
	cout << "Entered containerT::run" << endl;

	// test nice, can't find
	// test nice, can find

	try
	{
	    string key = (string)"BES.Container.Persistence.File.TheFile=" +
			 TEST_SRC_DIR + "/container01.file" ;
	    TheBESKeys::TheKeys()->set_key( key ) ;
	    BESContainerStorageList::TheList()->add_persistence( new BESContainerStorageFile( "TheFile" ) ) ;
	}
	catch( BESError &e )
	{
	    cerr << e.get_message() << endl ;
	    CPPUNIT_ASSERT( !"Failed to add storage persistence" ) ;
	}

	cout << "*****************************************" << endl;
	cout << "try to find symbolic name that doesn't exist, default" << endl;
	try
	{
	    BESContainer *c =
		BESContainerStorageList::TheList()->look_for( "nosym" ) ;
	    if( c )
	    {
		cerr << "container is valid, should not be" << endl ;
		cerr << " real_name = " << c->get_real_name() << endl ;
		cerr << " constraint = " << c->get_constraint() << endl ;
		cerr << " sym_name = " << c->get_symbolic_name() << endl ;
		cerr << " container type = " << c->get_container_type() << endl;
	    }
	    CPPUNIT_ASSERT( !"Found nosym, shouldn't have" ) ;
	}
	catch( BESError &e )
	{
	    cout << "caught exception, didn't find nosym, good" << endl ;
	    cout << e.get_message() << endl ;
	}

	cout << "*****************************************" << endl;
	cout << "try to find symbolic name that does exist, default" << endl;
	try
	{
	    BESContainer *c =
		BESContainerStorageList::TheList()->look_for( "sym1" ) ;
	    CPPUNIT_ASSERT( c ) ;
	    CPPUNIT_ASSERT( c->get_symbolic_name() == "sym1" ) ;
	    CPPUNIT_ASSERT( c->get_real_name() == "real1" ) ;
	    CPPUNIT_ASSERT( c->get_container_type() == "type1" ) ;
	    delete c ;
	}
	catch( BESError &e )
	{
	    cerr << e.get_message() << endl ;
	    CPPUNIT_ASSERT( !"Failed to find container sym1" ) ;
	}

	cout << "*****************************************" << endl;
	cout << "set to strict" << endl;
	TheBESKeys::TheKeys()->set_key( "BES.Container.Persistence=strict" ) ;

	cout << "*****************************************" << endl;
	cout << "try to find symbolic name that doesn't exist, strict" << endl;
	try
	{
	    BESContainer *c =
		BESContainerStorageList::TheList()->look_for( "nosym" ) ;
	    if( c )
	    {
		cerr << "Found nosym, shouldn't have" << endl ;
		cerr << " real_name = " << c->get_real_name() << endl ;
		cerr << " constraint = " << c->get_constraint() << endl ;
		cerr << " sym_name = " << c->get_symbolic_name() << endl ;
		cerr << " container type = " << c->get_container_type() << endl;
		CPPUNIT_ASSERT( !"Found nosym, shouldn't have" ) ;
	    }
	    else
	    {
		CPPUNIT_ASSERT( !"look_for returned null, should have thrown" );
	    }
	}
	catch( BESError &e )
	{
	    cout << "caught exception, didn't find nosym, good" << endl ;
	    cout << e.get_message() << endl ;
	}

	cout << "*****************************************" << endl;
	cout << "try to find symbolic name that does exist, strict" << endl;
	try
	{
	    BESContainer *c =
		BESContainerStorageList::TheList()->look_for( "sym1" ) ;
	    CPPUNIT_ASSERT( c ) ;
	    CPPUNIT_ASSERT( c->get_symbolic_name() == "sym1" ) ;
	    CPPUNIT_ASSERT( c->get_real_name() == "real1" ) ;
	    CPPUNIT_ASSERT( c->get_container_type() == "type1" ) ;
	}
	catch( BESError &e )
	{
	    cerr << e.get_message() << endl ;
	    CPPUNIT_ASSERT( !"Failed to find container sym1" ) ;
	}

	cout << "*****************************************" << endl;
	cout << "set to nice" << endl;
	TheBESKeys::TheKeys()->set_key( "BES.Container.Persistence=nice" ) ;

	cout << "*****************************************" << endl;
	cout << "try to find symbolic name that doesn't exist, nice" << endl;
	try
	{
	    BESContainer *c =
		BESContainerStorageList::TheList()->look_for( "nosym" ) ;
	    if( c )
	    {
		cerr << " real_name = " << c->get_real_name() << endl ;
		cerr << " constraint = " << c->get_constraint() << endl ;
		cerr << " sym_name = " << c->get_symbolic_name() << endl ;
		cerr << " container type = " << c->get_container_type() << endl;
		CPPUNIT_ASSERT( !"Found nosym, shouldn't have" ) ;
	    }
	}
	catch( BESError &e )
	{
	    cerr << e.get_message() << endl ;
	    CPPUNIT_ASSERT( !"Exception thrown in nice mode" ) ;
	}

	cout << "*****************************************" << endl;
	cout << "try to find symbolic name that does exist, nice" << endl;
	try
	{
	    BESContainer *c =
		BESContainerStorageList::TheList()->look_for( "sym1" ) ;
	    CPPUNIT_ASSERT( c ) ;
	    CPPUNIT_ASSERT( c->get_symbolic_name() == "sym1" ) ;
	    CPPUNIT_ASSERT( c->get_real_name() == "real1" ) ;
	    CPPUNIT_ASSERT( c->get_container_type() == "type1" ) ;
	}
	catch( BESError &e )
	{
	    cerr << e.get_message() << endl ;
	    CPPUNIT_ASSERT( !"Failed to find container sym1" ) ;
	}

	/* Because of the nature of the build system sometimes the cache
	 * directory will contain ../, which is not allowed for a containers
	 * real name (for files). So this test will be different when just doing
	 * a make check or a make distcheck
	 */
	string cache_dir = (string)TEST_SRC_DIR + "/cache" ;
	bool isdotdot = false ;
	string::size_type dotdot = cache_dir.find( "../" ) ;
	if( dotdot != string::npos )
	    isdotdot = true ;

	string src_file = cache_dir + "/testfile.txt" ;
	string com_file = cache_dir + "/testfile.txt.gz" ;

	TheBESKeys::TheKeys()->set_key( "BES.CacheDir", cache_dir ) ;
	TheBESKeys::TheKeys()->set_key( "BES.CachePrefix", "cont_cache" ) ;
	TheBESKeys::TheKeys()->set_key( "BES.CacheSize", "1" ) ;

	string chmod = (string)"chmod a+w " + TEST_SRC_DIR + "/cache" ;
	system( chmod.c_str() ) ;

	cout << "*****************************************" << endl;
	cout << "access a non compressed file" << endl;
	if( !isdotdot )
	{
	    try
	    {
		BESFileContainer c( "sym", src_file, "txt" ) ;

		string result = c.access() ;
		CPPUNIT_ASSERT( result == src_file ) ;
	    }
	    catch( BESError &e )
	    {
		cerr << e.get_message() << endl ;
		CPPUNIT_ASSERT( !"Failed to access non compressed file" ) ;
	    }
	}
	else
	{
	    try
	    {
		BESFileContainer c( "sym", src_file, "txt" ) ;

		string result = c.access() ;
		CPPUNIT_ASSERT( result != src_file ) ;
	    }
	    catch( BESError &e )
	    {
		cout << "Failed to access file with ../ in name, good" << endl ;
		cout << e.get_message() << endl ;
	    }
	}

	cout << "*****************************************" << endl;
	cout << "access a compressed file" << endl;
	if( !isdotdot )
	{
	    try
	    {
		BESFileLockingCache cache( *(TheBESKeys::TheKeys()),
				"BES.CacheDir", "BES.CachePrefix",
				"BES.CacheSize" ) ;
		string target ;
		bool is_it = cache.is_cached( com_file, target ) ;
		if( is_it )
		{
		    CPPUNIT_ASSERT( remove( target.c_str() ) == 0 ) ;
		}

		BESFileContainer c( "sym", com_file, "txt" ) ;

		string result = c.access() ;
		cout << "    result = " << result << endl ;
		cout << "    target = " << target << endl ;
		CPPUNIT_ASSERT( result == target ) ;

		CPPUNIT_ASSERT( cache.is_cached( com_file, target ) ) ;
	    }
	    catch( BESError &e )
	    {
		cerr << e.get_message() << endl ;
		CPPUNIT_ASSERT( !"Failed to access compressed file" ) ;
	    }
	}
	else
	{
	    try
	    {
		BESFileLockingCache cache( *(TheBESKeys::TheKeys()),
				"BES.CacheDir", "BES.CachePrefix",
				"BES.CacheSize" ) ;
		string target ;
		bool is_it = cache.is_cached( com_file, target ) ;
		if( is_it )
		{
		    CPPUNIT_ASSERT( remove( target.c_str() ) == 0 ) ;
		}

		BESFileContainer c( "sym", com_file, "txt" ) ;

		string result = c.access() ;
		CPPUNIT_ASSERT( result != target ) ;
	    }
	    catch( BESError &e )
	    {
		cout << "Failed to access file with ../ in name, good" << endl ;
		cout << e.get_message() << endl ;
	    }
	}

	cout << "*****************************************" << endl;
	cout << "Returning from containerT::run" << endl;
    }
} ;

CPPUNIT_TEST_SUITE_REGISTRATION( containerT ) ;

int
main( int, char** )
{
    CppUnit::TextTestRunner runner ;
    runner.addTest( CppUnit::TestFactoryRegistry::getRegistry().makeTest() ) ;

    bool wasSuccessful = runner.run( "", false )  ;

    return wasSuccessful ? 0 : 1 ;
}

