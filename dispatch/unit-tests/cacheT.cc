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

#include "cacheT.h"
#include "BESCache.h"
#include "TheBESKeys.h"
#include "BESError.h"
#include <test_config.h>

void
cacheT::check_cache( const string &cache_dir, map<string,string> &should_be )
{
    map<string,string> contents ;
    string match_prefix = "bes_cache#" ;
    DIR *dip = opendir( cache_dir.c_str() ) ;
    if( dip != NULL )
    {
	struct dirent *dit;
	while( ( dit = readdir( dip ) ) != NULL )
	{
	    string dirEntry = dit->d_name ;
	    if( dirEntry.compare( 0, match_prefix.length(), match_prefix ) == 0)
		contents[dirEntry] = dirEntry ;
	}
    }
    closedir( dip ) ;

    if( should_be.size() != contents.size() )
    {
	cerr << "actual number of files is " << contents.size()
	     << " should be " << should_be.size() << endl ;
    }
    else
    {
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
	}
	if( good )
	{
	    cout << "contents matches what should be there" << endl ;
	}
    }
}

/** @brief Set up the cache.
    Add to the cache a set of eight test files, with names that are easy to
    work with and each with an access time two seconds later than the
    preceding one.

    @param cache_dir Directory that holds the cached files.*/
void
cacheT::init_cache( const string &cache_dir )
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

int
cacheT::run(void)
{
    cout << endl << "*****************************************" << endl;
    cout << "Entered cacheT::run" << endl;
    int retVal = 0;

    string cache_dir = (string)TEST_SRC_DIR + "/cache" ;

    init_cache(cache_dir);

    BESKeys *keys = TheBESKeys::TheKeys() ;

    string target ;
    bool is_it = false ;

    cout << endl << "*****************************************" << endl;
    try
    {
	BESCache cache( "", "", 0 ) ;
	cerr << "Created cache with empty dir, should not have" << endl ;
	return 1 ;
    }
    catch( BESError &e )
    {
	cout << "Failed to create cache with empty dir, good" << endl ;
	cout << e.get_message() << endl ;
    }
    catch( ... )
    {
	cerr << "Failed to create cache with empty dir, unknown exception"
	     << endl ;
	return 1 ;
    }

    cout << endl << "*****************************************" << endl;
    try
    {
	BESCache cache( "/dummy", "", 0 ) ;
	cerr << "Created cache with bad dir, should not have" << endl ;
	return 1 ;
    }
    catch( BESError &e )
    {
	cout << "Failed to create cache with bad dir, good" << endl ;
	cout << e.get_message() << endl ;
    }
    catch( ... )
    {
	cerr << "Failed to create cache with bad dir, unknown exception"
	     << endl ;
	return 1 ;
    }

    cout << endl << "*****************************************" << endl;
    try
    {
	BESCache cache( cache_dir, "", 0 ) ;
	cerr << "Created cache with empty prefix, should not have" << endl ;
	return 1 ;
    }
    catch( BESError &e )
    {
	cout << "Failed to create cache with empty prefix, good" << endl ;
	cout << e.get_message() << endl ;
    }
    catch( ... )
    {
	cerr << "Failed to create cache with empty prefix, unknown exception"
	     << endl ;
	return 1 ;
    }

    cout << endl << "*****************************************" << endl;
    try
    {
	BESCache cache( cache_dir, "bes_cache", 0 ) ;
	cerr << "Created cache with 0 size, should not have" << endl ;
	return 1 ;
    }
    catch( BESError &e )
    {
	cout << "Failed to create cache with 0 size, good" << endl ;
	cout << e.get_message() << endl ;
    }
    catch( ... )
    {
	cerr << "Failed to create cache with 0 size, unknown exception"
	     << endl ;
	return 1 ;
    }

    cout << endl << "*****************************************" << endl;
    try
    {
	BESCache cache( cache_dir, "bes_cache", 1 ) ;
	cout << "Created cache with good params, good" << endl ;
    }
    catch( BESError &e )
    {
	cerr << "Failed to create cache with good params" << endl ;
	cerr << e.get_message() << endl ;
	return 1 ;
    }
    catch( ... )
    {
	cerr << "Failed to create cache with good params, unknown exception"
	     << endl ;
	return 1 ;
    }

    cout << endl << "*****************************************" << endl;
    try
    {
	BESCache cache( *keys, "", "", "" ) ;
	cerr << "Created cache with empty dir key, should not have" << endl ;
	return 1 ;
    }
    catch( BESError &e )
    {
	cout << "Failed to create cache with empty dir key, good" << endl ;
	cout << e.get_message() << endl ;
    }
    catch( ... )
    {
	cerr << "Failed to create cache with empty dir key, unknown exception"
	     << endl ;
	return 1 ;
    }

    cout << endl << "*****************************************" << endl;
    try
    {
	BESCache cache( *keys, "BES.CacheDir", "", "" ) ;
	cerr << "Created cache with non-exist dir key, should not have" << endl;
	return 1 ;
    }
    catch( BESError &e )
    {
	cout << "Failed to create cache with non-exist dir key, good" << endl ;
	cout << e.get_message() << endl ;
    }
    catch( ... )
    {
	cerr << "Failed to create cache with non-exist dir key, unknown exception"
	     << endl ;
	return 1 ;
    }

    keys->set_key( "BES.CacheDir", "/dummy" ) ;
    cout << endl << "*****************************************" << endl;
    try
    {
	BESCache cache( *keys, "BES.CacheDir", "", "" ) ;
	cerr << "Created cache with bad dir in conf, should not have" << endl;
	return 1 ;
    }
    catch( BESError &e )
    {
	cout << "Failed to create cache with bad dir in conf, good" << endl ;
	cout << e.get_message() << endl ;
    }
    catch( ... )
    {
	cerr << "Failed to create cache with bad dir in conf, unknown exception"
	     << endl ;
	return 1 ;
    }

    keys->set_key( "BES.CacheDir", cache_dir ) ;
    cout << endl << "*****************************************" << endl;
    try
    {
	BESCache cache( *keys, "BES.CacheDir", "", "" ) ;
	cerr << "Created cache with empty prefix key, should not have" << endl;
	return 1 ;
    }
    catch( BESError &e )
    {
	cout << "Failed to create cache with empty prefix key, good" << endl ;
	cout << e.get_message() << endl ;
    }
    catch( ... )
    {
	cerr << "Failed to create cache with empty prefix key, unknown exception"
	     << endl ;
	return 1 ;
    }

    cout << endl << "*****************************************" << endl;
    try
    {
	BESCache cache( *keys, "BES.CacheDir", "BES.CachePrefix", "" ) ;
	cerr << "Created cache with non-exist prefix key, should not have"
	     << endl;
	return 1 ;
    }
    catch( BESError &e )
    {
	cout << "Failed to create cache with non-exist prefix key, good"
	     << endl ;
	cout << e.get_message() << endl ;
    }
    catch( ... )
    {
	cerr << "Failed to create cache with non-exist prefix key, "
	     << "unknown exception" << endl ;
	return 1 ;
    }

    keys->set_key( "BES.CachePrefix", "" ) ;
    cout << endl << "*****************************************" << endl;
    try
    {
	BESCache cache( *keys, "BES.CacheDir", "BES.CachePrefix", "" ) ;
	cerr << "Created cache with empty prefix in conf, should not have"
	     << endl;
	return 1 ;
    }
    catch( BESError &e )
    {
	cout << "Failed to create cache with empty prefix in conf, good"
	     << endl ;
	cout << e.get_message() << endl ;
    }
    catch( ... )
    {
	cerr << "Failed to create cache with empty prefix in conf, "
	     << "unknown exception" << endl ;
	return 1 ;
    }

    keys->set_key( "BES.CachePrefix", "bes_cache" ) ;
    cout << endl << "*****************************************" << endl;
    try
    {
	BESCache cache( *keys, "BES.CacheDir", "BES.CachePrefix", "" ) ;
	cerr << "Created cache with empty size key, should not have"
	     << endl;
	return 1 ;
    }
    catch( BESError &e )
    {
	cout << "Failed to create cache with empty size key, good"
	     << endl ;
	cout << e.get_message() << endl ;
    }
    catch( ... )
    {
	cerr << "Failed to create cache with empty size key, "
	     << "unknown exception" << endl ;
	return 1 ;
    }

    cout << endl << "*****************************************" << endl;
    try
    {
	BESCache cache( *keys, "BES.CacheDir", "BES.CachePrefix", "BES.CacheSize" ) ;
	cerr << "Created cache with non-exist size key, should not have"
	     << endl;
	return 1 ;
    }
    catch( BESError &e )
    {
	cout << "Failed to create cache with non-exist size key, good"
	     << endl ;
	cout << e.get_message() << endl ;
    }
    catch( ... )
    {
	cerr << "Failed to create cache with non-exist size key, "
	     << "unknown exception" << endl ;
	return 1 ;
    }

    keys->set_key( "BES.CacheSize", "dummy" ) ;
    cout << endl << "*****************************************" << endl;
    try
    {
	BESCache cache( *keys, "BES.CacheDir", "BES.CachePrefix", "BES.CacheSize" ) ;
	cerr << "Created cache with bad size in conf, should not have"
	     << endl;
	return 1 ;
    }
    catch( BESError &e )
    {
	cout << "Failed to create cache with bad size in conf, good"
	     << endl ;
	cout << e.get_message() << endl ;
    }
    catch( ... )
    {
	cerr << "Failed to create cache with bad size in conf, "
	     << "unknown exception" << endl ;
	return 1 ;
    }

    keys->set_key( "BES.CacheSize", "0" ) ;
    cout << endl << "*****************************************" << endl;
    try
    {
	BESCache cache( *keys, "BES.CacheDir", "BES.CachePrefix", "BES.CacheSize" ) ;
	cerr << "Created cache with 0 size in conf, should not have"
	     << endl;
	return 1 ;
    }
    catch( BESError &e )
    {
	cout << "Failed to create cache with 0 size in conf, good"
	     << endl ;
	cout << e.get_message() << endl ;
    }
    catch( ... )
    {
	cerr << "Failed to create cache with 0 size in conf, "
	     << "unknown exception" << endl ;
	return 1 ;
    }

    keys->set_key( "BES.CacheSize", "1" ) ;
    cout << endl << "*****************************************" << endl;
    try
    {
	BESCache cache( *keys, "BES.CacheDir", "BES.CachePrefix", "BES.CacheSize" ) ;
	cout << "Created cache with good keys" << endl;
    }
    catch( BESError &e )
    {
	cerr << "Failed to create cache with good keys" << endl ;
	cerr << e.get_message() << endl ;
	return 1 ;
    }
    catch( ... )
    {
	cerr << "Failed to create cache with good keys unknown exception"
	     << endl ;
	return 1 ;
    }

    BESCache cache( *keys, "BES.CacheDir", "BES.CachePrefix", "BES.CacheSize" );

    cout << endl << "*****************************************" << endl;
    try
    {
	is_it = cache.is_cached( "/dummy/dummy/dummy.nc.gz", target ) ;
	if( is_it == true )
	{
	    cerr << "non-exist file is cached" << endl ;
	    return 1 ;
	}
	else
	{
	    cout << "non-exist file is not cached, good" << endl ;
	}
    }
    catch( BESError &e )
    {
	cerr << "Error checking if non-exist file cached" << endl ;
	cerr << e.get_message() << endl ;
	return 1 ;
    }
    catch( ... )
    {
	cerr << "Error checking if non-exist file cached" << endl ;
	cerr << "Unknown exception" << endl ;
	return 1 ;
    }

    cout << endl << "*****************************************" << endl;
    try
    {
	is_it = cache.is_cached( "dummy", target ) ;
	if( is_it == true )
	{
	    cerr << "bad file is cached" << endl ;
	    return 1 ;
	}
	else
	{
	    cout << "bad file is not cached, good" << endl ;
	}
    }
    catch( BESError &e )
    {
	cerr << "Error checking if non-exist file cached" << endl ;
	cerr << e.get_message() << endl ;
	return 1 ;
    }
    catch( ... )
    {
	cerr << "Error checking if non-exist file cached" << endl ;
	cerr << "Unknown exception" << endl ;
	return 1 ;
    }

    cout << endl << "*****************************************" << endl;
    try
    {
	string should_be = cache_dir
			   + "/bes_cache#usr#local#data#template01.txt" ;
	is_it = cache.is_cached( "/usr/local/data/template01.txt.gz", target ) ;
	if( is_it == true )
	{
	    cout << "file is cached, good" << endl ;
	    if( target != should_be )
	    {
		cerr << "target is " << target
		     << ", should be " << should_be << endl ;
		return 1 ;
	    }
	    else
	    {
		cout << "target is good" << endl ;
	    }
	}
	else
	{
	    cerr << "file is not cached, should be" << endl ;
	    cerr << "looking for file " << target << endl ;
	    return 1 ;
	}
    }
    catch( BESError &e )
    {
	cerr << "Error checking if good file cached" << endl ;
	cerr << e.get_message() << endl ;
	return 1 ;
    }
    catch( ... )
    {
	cerr << "Error checking if good file cached" << endl ;
	cerr << "Unknown exception" << endl ;
	return 1 ;
    }

    map<string,string> should_be ;
    should_be["bes_cache#usr#local#data#template01.txt"] = "bes_cache#usr#local#data#template01.txt" ;
    should_be["bes_cache#usr#local#data#template03.txt"] = "bes_cache#usr#local#data#template02.txt" ;
    should_be["bes_cache#usr#local#data#template05.txt"] = "bes_cache#usr#local#data#template03.txt" ;
    should_be["bes_cache#usr#local#data#template08.txt"] = "bes_cache#usr#local#data#template04.txt" ;

    cout << endl << "*****************************************" << endl;
    cout << "Test purge, should remove a few" << endl;
    try
    {
	cache.purge() ;
	cout << "purge returned with success ... checking" << endl ;
	check_cache( cache_dir, should_be ) ;
    }
    catch( BESError &e )
    {
	cerr << "purge failed" << endl ;
	cerr << e.get_message() << endl ;
	return 1 ;
    }
    catch( ... )
    {
	cerr << "purge failed with unknown exception" << endl ;
	return 1 ;
    }

    cout << endl << "*****************************************" << endl;
    cout << "Test purge, should not remove any" << endl;
    try
    {
	cache.purge() ;
	cout << "purge returned with success ... checking" << endl ;
	check_cache( cache_dir, should_be ) ;
    }
    catch( BESError &e )
    {
	cerr << "purge failed" << endl ;
	cerr << e.get_message() << endl ;
	return 1 ;
    }
    catch( ... )
    {
	cerr << "purge failed with unknown exception" << endl ;
	return 1 ;
    }

    cout << endl << "*****************************************" << endl;
    cout << "Returning from cacheT::run" << endl;

    return retVal;
}

int
main(int argC, char **argV) {
    string env_var = (string)"BES_CONF=" + TEST_SRC_DIR
                     + "/cache_test.ini" ;
    putenv( (char *)env_var.c_str() ) ;
    Application *app = new cacheT();
    return app->main(argC, argV);
}

