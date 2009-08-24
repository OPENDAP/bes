// uncompressT.C

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
#include <fstream>
#include <cstdlib>

using std::cerr ;
using std::cout ;
using std::endl ;
using std::ifstream ;

#include "config.h"
#include "BESUncompressManager.h"
#include "BESCache.h"
#include "BESError.h"
#include "TheBESKeys.h"
#include <test_config.h>

#define BES_CACHE_CHAR '#' 

class uncompressT: public TestFixture {
private:

public:
    uncompressT() {}
    ~uncompressT() {}

    void setUp()
    {
	string bes_conf = (string)TEST_SRC_DIR + "/bes.conf" ;
	TheBESKeys::ConfigFile = bes_conf ;
    } 

    void tearDown()
    {
    }

    CPPUNIT_TEST_SUITE( uncompressT ) ;

    CPPUNIT_TEST( do_test ) ;

    CPPUNIT_TEST_SUITE_END() ;

    void do_test()
    {
	cout << "*****************************************" << endl;
	cout << "Entered uncompressT::run" << endl;

	string cache_dir = (string)TEST_SRC_DIR + "/cache" ;
	string src_file = cache_dir + "/testfile.txt.gz" ;

	// we're not testing the caching mechanism, so just create it, but make
	// sure it gets created.
	string target ;
	try
	{
	    BESCache cache( cache_dir, "gz_cache", 1 ) ;
	    // get the target name and make sure the target file doesn't exist
	    if( cache.is_cached( src_file, target ) )
	    {
		CPPUNIT_ASSERT( remove( target.c_str() ) == 0 ) ;
	    }

	    cout << "*****************************************" << endl;
	    cout << "uncompress a test gz file" << endl;
	    try
	    {
		string result ;
		bool cached = BESUncompressManager::TheManager()->uncompress( src_file, result, cache ) ;
		CPPUNIT_ASSERT( cached ) ;
		CPPUNIT_ASSERT( result == target ) ;

		ifstream strm( target.c_str() ) ;
		CPPUNIT_ASSERT( strm ) ;

		char line[80] ;
		strm.getline( (char *)line, 80 ) ;
		string sline = line ;
		string should_be = "This is a test of a compression method." ;
		cout << "    contents = " << sline << endl ;
		cout << "    expected = " << should_be << endl ;
		CPPUNIT_ASSERT( sline == should_be ) ;
	    }
	    catch( BESError &e )
	    {
		cerr << e.get_message() << endl ;
		CPPUNIT_ASSERT( !"Failed to uncompress the gz file" ) ;
	    }

	    string tmp ;
	    CPPUNIT_ASSERT( cache.is_cached( src_file, tmp ) ) ;

	    cout << "*****************************************" << endl;
	    cout << "uncompress a test gz file, should be cached" << endl;
	    try
	    {
		string result ;
		bool cached = BESUncompressManager::TheManager()->uncompress( src_file, result, cache ) ;
		CPPUNIT_ASSERT( cached ) ;
		CPPUNIT_ASSERT( result == target ) ;

		ifstream strm( target.c_str() ) ;
		CPPUNIT_ASSERT( strm ) ;

		char line[80] ;
		strm.getline( (char *)line, 80 ) ;
		string sline = line ;
		string should_be = "This is a test of a compression method." ;
		cout << "    contents = " << sline << endl ;
		cout << "    expected = " << should_be << endl ;
		CPPUNIT_ASSERT( sline == should_be ) ;
	    }
	    catch( BESError &e )
	    {
		cerr << e.get_message() << endl ;
		CPPUNIT_ASSERT( !"Failed to uncompress the file" ) ;
	    }

	    CPPUNIT_ASSERT( cache.is_cached( src_file, tmp ) ) ;
	}
	catch( BESError &e )
	{
	    cerr << e.get_message() << endl ;
	    CPPUNIT_ASSERT( !"Unable to create the gz cache object" ) ;
	}

#ifdef HAVE_LIBBZ2
	src_file = cache_dir + "/testfile.txt.bz2" ;
	try
	{
	    BESCache cache( cache_dir, "bz2_cache", 1 ) ;
	    // get the target name and make sure the target file doesn't exist
	    if( cache.is_cached( src_file, target ) )
	    {
		CPPUNIT_ASSERT( remove( target.c_str() ) == 0 ) ;
	    }

	    cout << "*****************************************" << endl;
	    cout << "uncompress a test bz2 file" << endl;
	    try
	    {
		string result ;
		bool cached = BESUncompressManager::TheManager()->uncompress( src_file, result, cache ) ;
		CPPUNIT_ASSERT( cached ) ;
		CPPUNIT_ASSERT( result == target ) ;

		ifstream strm( target.c_str() ) ;
		CPPUNIT_ASSERT( strm ) ;

		char line[80] ;
		strm.getline( (char *)line, 80 ) ;
		string sline = line ;
		string should_be = "This is a test of a compression method." ;
		cout << "    contents = " << sline << endl ;
		cout << "    expected = " << should_be << endl ;
		CPPUNIT_ASSERT( sline == should_be ) ;
	    }
	    catch( BESError &e )
	    {
		cerr << e.get_message() << endl ;
		CPPUNIT_ASSERT( !"Failed to uncompress the file" ) ;
	    }

	    string tmp ;
	    CPPUNIT_ASSERT( cache.is_cached( src_file, tmp ) ) ;

	    cout << "*****************************************" << endl;
	    cout << "uncompress a test bz2 file, should be cached" << endl;
	    try
	    {
		string result ;
		bool cached = BESUncompressManager::TheManager()->uncompress( src_file, result, cache ) ;
		CPPUNIT_ASSERT( cached ) ;
		CPPUNIT_ASSERT( result == target ) ;

		ifstream strm( target.c_str() ) ;
		CPPUNIT_ASSERT( strm ) ;

		char line[80] ;
		strm.getline( (char *)line, 80 ) ;
		string sline = line ;
		string should_be = "This is a test of a compression method." ;
		cout << "    contents = " << sline << endl ;
		cout << "    expected = " << should_be << endl ;
		CPPUNIT_ASSERT( sline == should_be ) ;
	    }
	    catch( BESError &e )
	    {
		cerr << e.get_message() << endl ;
		CPPUNIT_ASSERT( !"Failed to uncompress the file" ) ;
	    }
	}
	catch( BESError &e )
	{
	    cerr << e.get_message() << endl ;
	    CPPUNIT_ASSERT( !"Unable to create the bz2 cache object" ) ;
	}
#endif

	src_file = cache_dir + "/testfile.txt.Z" ;
	try
	{
	    BESCache cache( cache_dir, "z_cache", 1 ) ;
	    // get the target name and make sure the target file doesn't exist
	    if( cache.is_cached( src_file, target ) )
	    {
		CPPUNIT_ASSERT( remove( target.c_str() ) == 0 ) ;
	    }

	    cout << "*****************************************" << endl;
	    cout << "uncompress a test z file" << endl;
	    try
	    {
		string result ;
		bool cached = BESUncompressManager::TheManager()->uncompress( src_file, result, cache ) ;
		CPPUNIT_ASSERT( cached ) ;
		CPPUNIT_ASSERT( result == target ) ;

		ifstream strm( target.c_str() ) ;
		CPPUNIT_ASSERT( strm ) ;

		char line[80] ;
		strm.getline( (char *)line, 80 ) ;
		string sline = line ;
		string should_be = "This is a test of a compression method." ;
		cout << "    contents = " << sline << endl ;
		cout << "    expected = " << should_be << endl ;
		CPPUNIT_ASSERT( sline == should_be ) ;
	    }
	    catch( BESError &e )
	    {
		cerr << e.get_message() << endl ;
		CPPUNIT_ASSERT( !"Failed to uncompress the file" ) ;
	    }

	    string tmp ;
	    CPPUNIT_ASSERT( cache.is_cached( src_file, tmp ) ) ;

	    cout << "*****************************************" << endl;
	    cout << "uncompress a test z file, should be cached" << endl;
	    try
	    {
		string result ;
		bool cached = BESUncompressManager::TheManager()->uncompress( src_file, result, cache ) ;
		cout << "Uncompression succeeded" << endl ;
		CPPUNIT_ASSERT( cached ) ;
		CPPUNIT_ASSERT( result == target ) ;

		ifstream strm( target.c_str() ) ;
		CPPUNIT_ASSERT( strm ) ;

		char line[80] ;
		strm.getline( (char *)line, 80 ) ;
		string sline = line ;
		string should_be = "This is a test of a compression method." ;
		cout << "    contents = " << sline << endl ;
		cout << "    expected = " << should_be << endl ;
		CPPUNIT_ASSERT( sline == should_be ) ;
	    }
	    catch( BESError &e )
	    {
		cerr << e.get_message() << endl ;
		CPPUNIT_ASSERT( !"Failed to uncompress the file" ) ;
	    }
	}
	catch( BESError &e )
	{
	    cerr << e.get_message() << endl ;
	    CPPUNIT_ASSERT( !"Unable to create the z cache object" ) ;
	}

	src_file = cache_dir + "/testfile.nc" ;
	try
	{
	    cout << "*****************************************" << endl;
	    cout << "uncompress a file that is not compressed" << endl;

	    BESCache cache( cache_dir, "z_cache", 1 ) ;
	    string result ;
	    bool cached = BESUncompressManager::TheManager()->uncompress( src_file, result, cache ) ;
	    CPPUNIT_ASSERT( !cached ) ;
	    CPPUNIT_ASSERT( result.empty() ) ;
	}
	catch( BESError &e )
	{
	    cerr << e.get_message() << endl ;
	    CPPUNIT_ASSERT( !"Failed to uncompress the file" ) ;
	}

	cout << "*****************************************" << endl;
	cout << "Returning from uncompressT::run" << endl;
    }
} ;

CPPUNIT_TEST_SUITE_REGISTRATION( uncompressT ) ;

int 
main( int, char** )
{
    CppUnit::TextTestRunner runner ;
    runner.addTest( CppUnit::TestFactoryRegistry::getRegistry().makeTest() ) ;

    bool wasSuccessful = runner.run( "", false )  ;

    return wasSuccessful ? 0 : 1 ;
}

