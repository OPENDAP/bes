// cacheT.C

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdio.h>
#include <errno.h>

#include <iostream>

using std::cerr ;
using std::cout ;
using std::endl ;

#include "cacheT.h"
#include "BESCache.h"
#include "TheBESKeys.h"
#include "BESException.h"

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
    system("cp -f cache/template.txt cache/bes_cache#usr#local#data#template01.txt");
    system("cp -f cache/template.txt cache/bes_cache#usr#local#data#template02.txt");
    system("cp -f cache/template.txt cache/bes_cache#usr#local#data#template03.txt");
    system("cp -f cache/template.txt cache/bes_cache#usr#local#data#template04.txt");
    system("cp -f cache/template.txt cache/bes_cache#usr#local#data#template05.txt");
    system("cp -f cache/template.txt cache/bes_cache#usr#local#data#template06.txt");
    system("cp -f cache/template.txt cache/bes_cache#usr#local#data#template07.txt");
    system("cp -f cache/template.txt cache/bes_cache#usr#local#data#template08.txt");

    sleep(1);
    system("cat cache/bes_cache#usr#local#data#template08.txt > /dev/null");
    sleep(1);
    system("cat cache/bes_cache#usr#local#data#template07.txt > /dev/null");
    sleep(1);
    system("cat cache/bes_cache#usr#local#data#template06.txt > /dev/null");
    sleep(1);
    system("cat cache/bes_cache#usr#local#data#template05.txt > /dev/null");
    sleep(1);
    system("cat cache/bes_cache#usr#local#data#template04.txt > /dev/null");
    sleep(1);
    system("cat cache/bes_cache#usr#local#data#template03.txt > /dev/null");
    sleep(1);
    system("cat cache/bes_cache#usr#local#data#template02.txt > /dev/null");
    sleep(1);
    system("cat cache/bes_cache#usr#local#data#template01.txt > /dev/null");

#if 0
    catch "exec /bin/echo \"update\" >> cache/bes_cache#usr#local#data#template05.txt"
    catch "exec sleep 2"
    catch "exec /bin/echo \"update\" >> cache/bes_cache#usr#local#data#template03.txt"
    catch "exec sleep 2"
    catch "exec /bin/echo \"update\" >> cache/bes_cache#usr#local#data#template06.txt"
    catch "exec sleep 2"
    catch "exec /bin/echo \"update\" >> cache/bes_cache#usr#local#data#template02.txt"
    catch "exec sleep 2"
    catch "exec /bin/echo \"update\" >> cache/bes_cache#usr#local#data#template04.txt"
    catch "exec sleep 2"
    catch "exec /bin/echo \"update\" >> cache/bes_cache#usr#local#data#template01.txt"
    catch "exec sleep 2"
    catch "exec /bin/echo \"update\" >> cache/bes_cache#usr#local#data#template08.txt"
#endif
}

int
cacheT::run(void)
{
    cout << endl << "*****************************************" << endl;
    cout << "Entered cacheT::run" << endl;
    int retVal = 0;

    char cur_dir[4096] ;
    getcwd( cur_dir, 4096 ) ;
    string cache_dir = (string)cur_dir + "/cache" ;

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
    catch( BESException &e )
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
    catch( BESException &e )
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
    catch( BESException &e )
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
    catch( BESException &e )
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
    catch( BESException &e )
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
    catch( BESException &e )
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
    catch( BESException &e )
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
    catch( BESException &e )
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
    catch( BESException &e )
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
    catch( BESException &e )
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
    catch( BESException &e )
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
    catch( BESException &e )
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
    catch( BESException &e )
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
    catch( BESException &e )
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
    catch( BESException &e )
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
    catch( BESException &e )
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
    catch( BESException &e )
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
    catch( BESException &e )
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
    catch( BESException &e )
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
    should_be["bes_cache#usr#local#data#template02.txt"] = "bes_cache#usr#local#data#template02.txt" ;
    should_be["bes_cache#usr#local#data#template03.txt"] = "bes_cache#usr#local#data#template03.txt" ;
    should_be["bes_cache#usr#local#data#template04.txt"] = "bes_cache#usr#local#data#template04.txt" ;

    cout << endl << "*****************************************" << endl;
    cout << "Test purge, should remove a few" << endl;
    try
    {
	cache.purge() ;
	cout << "purge returned with success ... checking" << endl ;
	check_cache( cache_dir, should_be ) ;
    }
    catch( BESException &e )
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
    catch( BESException &e )
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
    putenv( "BES_CONF=./cache_test.ini" ) ;
    Application *app = new cacheT();
    return app->main(argC, argV);
}

