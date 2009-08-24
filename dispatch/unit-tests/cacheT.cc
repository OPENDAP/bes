// cacheT.C

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

#include <unistd.h>  // for sleep
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>  // for closedir opendir

#include <iostream>
#include <sstream>
#include <cstdlib>

using std::cerr ;
using std::cout ;
using std::endl ;
using std::ostringstream ;

#include "BESCache.h"
#include "TheBESKeys.h"
#include "BESError.h"
#include <test_config.h>

class connT: public TestFixture {
private:

public:
    connT() {}
    ~connT() {}

    void setUp()
    {
	string bes_conf = (string)TEST_SRC_DIR + "/cache_test.ini" ;
	TheBESKeys::ConfigFile = bes_conf ;
    } 

    void tearDown()
    {
    }

    /** @brief Set up the cache.
	Add to the cache a set of eight test files, with names that are easy to
	work with and each with an access time two seconds later than the
	preceding one.

	@param cache_dir Directory that holds the cached files.
    */
    void
    init_cache( const string &cache_dir )
    {
	string chmod = (string)"chmod a+w " + TEST_SRC_DIR + "/cache" ;
	system( chmod.c_str() ) ;

	string t_file = cache_dir + "/template.txt" ;
	for( int i = 1; i < 9; i++ )
	{
	    ostringstream s ;
	    s << "cp -f " << t_file << " " << TEST_SRC_DIR << "/cache/bes_cache#usr#local#data#template0" << i << ".txt" ;
	    cout << s.str() << endl ;
	    system( s.str().c_str() );

	    ostringstream m ;
	    m << "chmod a+w " << TEST_SRC_DIR << "/cache/bes_cache#usr#local#data#template0" << i << ".txt" ;
	    cout << m.str() << endl ;
	    system( m.str().c_str() ) ;
	}

	char *touchers[8] = { "7", "6", "4", "2", "8", "5", "3", "1" } ;
	for( int i = 0; i < 8; i++ )
	{
	    sleep(1);
	    string cmd = (string)"cat " + TEST_SRC_DIR
			 + "/cache/bes_cache#usr#local#data#template0"
			 + touchers[i]
			 + ".txt > /dev/null" ;
	    cout << cmd << endl ;
	    system( cmd.c_str() );
	}
    }

    void
    check_cache( const string &cache_dir, map<string,string> &should_be )
    {
	map<string,string> contents ;
	string match_prefix = "bes_cache#" ;
	DIR *dip = opendir( cache_dir.c_str() ) ;
	CPPUNIT_ASSERT( dip ) ;
	struct dirent *dit;
	while( ( dit = readdir( dip ) ) != NULL )
	{
	    string dirEntry = dit->d_name ;
	    if( dirEntry.compare( 0, match_prefix.length(), match_prefix ) == 0)
		contents[dirEntry] = dirEntry ;
	}
	closedir( dip ) ;

	CPPUNIT_ASSERT( should_be.size() == contents.size() ) ;
	map<string,string>::const_iterator ci = contents.begin() ;
	map<string,string>::const_iterator ce = contents.end() ;
	map<string,string>::const_iterator si = should_be.begin() ;
	map<string,string>::const_iterator se = should_be.end() ;
	bool good = true ;
	for( ; ci != ce; ci++, si++ )
	{
	    if( (*ci).first != (*si).first )
	    {
		cerr << "contents: " << (*ci).first
		     << " - should be: " << (*si).first << endl ;
		good = false ;
	    }
	    CPPUNIT_ASSERT( (*ci).first == (*si).first ) ;
	}
    }

    CPPUNIT_TEST_SUITE( connT ) ;

    CPPUNIT_TEST( do_test ) ;

    CPPUNIT_TEST_SUITE_END() ;

    void do_test()
    {
	cout << "*****************************************" << endl;
	cout << "Entered cacheT::run" << endl;

	string cache_dir = (string)TEST_SRC_DIR + "/cache" ;

	init_cache(cache_dir);

	BESKeys *keys = TheBESKeys::TheKeys() ;

	string target ;
	bool is_it = false ;

	cout << "*****************************************" << endl;
	cout << "creating cache with empty directory name" << endl ;
	try
	{
	    BESCache cache( "", "", 0 ) ;
	    CPPUNIT_ASSERT( !"Created cache with empty dir" ) ;
	}
	catch( BESError &e )
	{
	    cout << "Failed to create cache with empty dir, good" << endl ;
	    cout << e.get_message() << endl ;
	}

	cout << "*****************************************" << endl;
	cout << "creating cache with non-existant directory" << endl ;
	try
	{
	    BESCache cache( "/dummy", "", 0 ) ;
	    CPPUNIT_ASSERT( !"Created cache with bad dir" ) ;
	}
	catch( BESError &e )
	{
	    cout << "Failed to create cache with bad dir, good" << endl ;
	    cout << e.get_message() << endl ;
	}

	cout << "*****************************************" << endl;
	cout << "creating cache with empty prefix name" << endl ;
	try
	{
	    BESCache cache( cache_dir, "", 0 ) ;
	    CPPUNIT_ASSERT( !"Created cache with empty prefix" ) ;
	}
	catch( BESError &e )
	{
	    cout << "Failed to create cache with empty prefix, good" << endl ;
	    cout << e.get_message() << endl ;
	}

	cout << "*****************************************" << endl;
	cout << "creating cache with 0 size" << endl ;
	try
	{
	    BESCache cache( cache_dir, "bes_cache", 0 ) ;
	    CPPUNIT_ASSERT( !"Created cache with 0 size" ) ;
	}
	catch( BESError &e )
	{
	    cout << "Failed to create cache with 0 size, good" << endl ;
	    cout << e.get_message() << endl ;
	}

	cout << "*****************************************" << endl;
	cout << "creating good cache" << endl ;
	try
	{
	    BESCache cache( cache_dir, "bes_cache", 1 ) ;
	}
	catch( BESError &e )
	{
	    cerr << e.get_message() << endl ;
	    CPPUNIT_ASSERT( !"Failed to create cache" ) ;
	}

	cout << "*****************************************" << endl;
	cout << "creating cache with empty dir key" << endl ;
	try
	{
	    BESCache cache( *keys, "", "", "" ) ;
	    CPPUNIT_ASSERT( !"Created cache with empty dir key" ) ;
	}
	catch( BESError &e )
	{
	    cout << "Failed to create cache with empty dir key, good" << endl ;
	    cout << e.get_message() << endl ;
	}

	cout << "*****************************************" << endl;
	cout << "creating cache with non-exist dir key" << endl ;
	try
	{
	    BESCache cache( *keys, "BES.CacheDir", "", "" ) ;
	    CPPUNIT_ASSERT( !"Created cache with non-exist dir key" ) ;
	}
	catch( BESError &e )
	{
	    cout << "Failed to create cache with non-exist dir key, good"
		 << endl ;
	    cout << e.get_message() << endl ;
	}

	keys->set_key( "BES.CacheDir", "/dummy" ) ;
	cout << "*****************************************" << endl;
	cout << "creating cache with bad dir in conf" << endl ;
	try
	{
	    BESCache cache( *keys, "BES.CacheDir", "", "" ) ;
	    CPPUNIT_ASSERT( !"Created cache with bad dir in conf" ) ;
	}
	catch( BESError &e )
	{
	    cout << "Failed to create cache with bad dir in conf, good"
		 << endl ;
	    cout << e.get_message() << endl ;
	}

	keys->set_key( "BES.CacheDir", cache_dir ) ;
	cout << "*****************************************" << endl;
	cout << "creating cache with empty prefix key" << endl ;
	try
	{
	    BESCache cache( *keys, "BES.CacheDir", "", "" ) ;
	    CPPUNIT_ASSERT( !"Created cache with empty prefix key" ) ;
	}
	catch( BESError &e )
	{
	    cout << "Failed to create cache with empty prefix key, good" << endl ;
	    cout << e.get_message() << endl ;
	}

	cout << "*****************************************" << endl ;
	cout << "creating cache with non-exist prefix key" << endl ;
	try
	{
	    BESCache cache( *keys, "BES.CacheDir", "BES.CachePrefix", "" ) ;
	    CPPUNIT_ASSERT( !"Created cache with non-exist prefix key" ) ;
	}
	catch( BESError &e )
	{
	    cout << "Failed to create cache with non-exist prefix key, good"
		 << endl ;
	    cout << e.get_message() << endl ;
	}

	keys->set_key( "BES.CachePrefix", "" ) ;
	cout << "*****************************************" << endl;
	cout << "creating cache with empty prefix key in conf" << endl ;
	try
	{
	    BESCache cache( *keys, "BES.CacheDir", "BES.CachePrefix", "" ) ;
	    CPPUNIT_ASSERT( !"Created cache with empty prefix in conf" ) ;
	}
	catch( BESError &e )
	{
	    cout << "Failed to create cache with empty prefix in conf, good"
		 << endl ;
	    cout << e.get_message() << endl ;
	}

	keys->set_key( "BES.CachePrefix", "bes_cache" ) ;
	cout << "*****************************************" << endl;
	cout << "creating cache with empty size key" << endl ;
	try
	{
	    BESCache cache( *keys, "BES.CacheDir", "BES.CachePrefix", "" ) ;
	    CPPUNIT_ASSERT( !"Created cache with empty size key" ) ;
	}
	catch( BESError &e )
	{
	    cout << "Failed to create cache with empty size key, good"
		 << endl ;
	    cout << e.get_message() << endl ;
	}

	cout << "*****************************************" << endl;
	cout << "creating cache with non-exist size key" << endl ;
	try
	{
	    BESCache cache( *keys, "BES.CacheDir", "BES.CachePrefix", "BES.CacheSize" ) ;
	    CPPUNIT_ASSERT( !"Created cache with non-exist size key" ) ;
	}
	catch( BESError &e )
	{
	    cout << "Failed to create cache with non-exist size key, good"
		 << endl ;
	    cout << e.get_message() << endl ;
	}

	keys->set_key( "BES.CacheSize", "dummy" ) ;
	cout << "*****************************************" << endl;
	cout << "creating cache with bad size in conf" << endl ;
	try
	{
	    BESCache cache( *keys, "BES.CacheDir", "BES.CachePrefix", "BES.CacheSize" ) ;
	    CPPUNIT_ASSERT( !"Created cache with bad size in conf" ) ;
	}
	catch( BESError &e )
	{
	    cout << "Failed to create cache with bad size in conf, good"
		 << endl ;
	    cout << e.get_message() << endl ;
	}

	keys->set_key( "BES.CacheSize", "0" ) ;
	cout << "*****************************************" << endl;
	cout << "creating cache with 0 size in conf" << endl ;
	try
	{
	    BESCache cache( *keys, "BES.CacheDir", "BES.CachePrefix", "BES.CacheSize" ) ;
	    CPPUNIT_ASSERT( !"Created cache with 0 size in conf" ) ;
	}
	catch( BESError &e )
	{
	    cout << "Failed to create cache with 0 size in conf, good"
		 << endl ;
	    cout << e.get_message() << endl ;
	}

	keys->set_key( "BES.CacheSize", "1" ) ;
	cout << "*****************************************" << endl;
	cout << "creating good cache from config" << endl ;
	try
	{
	    BESCache cache( *keys, "BES.CacheDir", "BES.CachePrefix", "BES.CacheSize" ) ;
	}
	catch( BESError &e )
	{
	    cerr << e.get_message() << endl ;
	    CPPUNIT_ASSERT( !"Failed to create cache with good keys" ) ;
	}

	BESCache cache( *keys, "BES.CacheDir", "BES.CachePrefix", "BES.CacheSize" );

	cout << "*****************************************" << endl;
	cout << "checking the cache for non-exist compressed file" << endl ;
	try
	{
	    CPPUNIT_ASSERT( cache.is_cached( "/dummy/dummy/dummy.nc.gz",
					     target ) == false ) ;
	}
	catch( BESError &e )
	{
	    cerr << e.get_message() << endl ;
	    CPPUNIT_ASSERT( !"Error checking if non-exist file cached" ) ;
	}

	cout << "*****************************************" << endl;
	try
	{
	    CPPUNIT_ASSERT( cache.is_cached( "dummy", target ) == false ) ;
	}
	catch( BESError &e )
	{
	    cerr << e.get_message() << endl ;
	    CPPUNIT_ASSERT( !"Error checking if non-exist file cached" ) ;
	}

	cout << "*****************************************" << endl;
	cout << "find existing cached file" << endl ;
	try
	{
	    string should_be = cache_dir
			       + "/bes_cache#usr#local#data#template01.txt" ;
	    is_it = cache.is_cached( "/usr/local/data/template01.txt.gz", target ) ;
	    CPPUNIT_ASSERT( is_it == true ) ;
	    CPPUNIT_ASSERT( target == should_be ) ;
	}
	catch( BESError &e )
	{
	    cerr << e.get_message() << endl ;
	    CPPUNIT_ASSERT( !"Error checking if good file cached" ) ;
	}

	map<string,string> should_be ;
	should_be["bes_cache#usr#local#data#template01.txt"] = "bes_cache#usr#local#data#template01.txt" ;
	should_be["bes_cache#usr#local#data#template03.txt"] = "bes_cache#usr#local#data#template02.txt" ;
	should_be["bes_cache#usr#local#data#template05.txt"] = "bes_cache#usr#local#data#template03.txt" ;
	should_be["bes_cache#usr#local#data#template08.txt"] = "bes_cache#usr#local#data#template04.txt" ;

	cout << "*****************************************" << endl;
	cout << "Test purge" << endl;
	try
	{
	    cache.purge() ;
	    check_cache( cache_dir, should_be ) ;
	}
	catch( BESError &e )
	{
	    cerr << e.get_message() << endl ;
	    CPPUNIT_ASSERT( !"purge failed" ) ;
	}

	cout << "*****************************************" << endl;
	cout << "Test purge, should not remove any" << endl;
	try
	{
	    cache.purge() ;
	    check_cache( cache_dir, should_be ) ;
	}
	catch( BESError &e )
	{
	    cerr << e.get_message() << endl ;
	    CPPUNIT_ASSERT( !"purge failed" ) ;
	}

	cout << "*****************************************" << endl;
	cout << "Returning from cacheT::run" << endl;
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

