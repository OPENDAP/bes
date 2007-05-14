// gzT.C

#include <iostream>
#include <fstream>

using std::cerr ;
using std::cout ;
using std::endl ;
using std::ifstream ;

#include "gzT.h"
#include "BESUncompressGZ.h"
#include "BESCache.h"
#include "BESException.h"

#define BES_CACHE_CHAR '#' 

int
gzT::run(void)
{
    cout << endl << "*****************************************" << endl;
    cout << "Entered gzT::run" << endl;
    int retVal = 0;

    char cur_dir[4096] ;
    getcwd( cur_dir, 4096 ) ;
    string cache_dir = (string)cur_dir + "/cache" ;
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
	    if( remove( target.c_str() ) != 0 )
	    {
		cerr << "Unable to remove target file " << target
		     << " , initializing test" << endl ;
		return 1 ;
	    }
	}

	cout << endl << "*****************************************" << endl;
	cout << "uncompress a test file" << endl;
	try
	{
	    string result = BESUncompressGZ::uncompress( src_file, target ) ;
	    cout << "Uncompression succeeded" << endl ;
	    if( result == target )
	    {
		cout << "result is correct" << endl ;
	    }
	    else
	    {
		cerr << "Resulting file " << result << " is not correct, "
		     << "should be " << target << endl ;
		return 1 ;
	    }
	    ifstream strm( target.c_str() ) ;
	    if( !strm )
	    {
		cerr << "Resulting file " << result << " doesn't exist" << endl;
		return 1 ;
	    }
	    char line[80] ;
	    strm.getline( (char *)line, 80 ) ;
	    string sline = line ;
	    if( sline != "This is a test of a compression method." )
	    {
		cerr << "Contents of file not correct" << endl ;
		cerr << "Actual: " << sline << endl ;
		cerr << "Should be: This is a test of a compression method."
		     << endl ;
		return 1 ;
	    }
	    else
	    {
		cout << "Contents of file correct" << endl ;
	    }
	}
	catch( BESException &e )
	{
	    cerr << "Failed to uncompress the file" << endl ;
	    cerr << e.get_message() << endl ;
	    return 1 ;
	}
	catch( ... )
	{
	    cerr << "Failed to uncompress the file" << endl ;
	    cerr << "Unknown exception thrown" << endl ;
	    return 1 ;
	}

	string tmp ;
	if( cache.is_cached( src_file, tmp ) )
	{
	    cout << "File is now cached" << endl ;
	}
	else
	{
	    cerr << "File should be cached" << endl ;
	    return 1 ;
	}

	cout << endl << "*****************************************" << endl;
	cout << "uncompress a test file, should be cached" << endl;
	try
	{
	    string result = BESUncompressGZ::uncompress( src_file, target ) ;
	    cout << "Uncompression succeeded" << endl ;
	    if( result == target )
	    {
		cout << "result is correct" << endl ;
	    }
	    else
	    {
		cerr << "Resulting file " << result << " is not correct, "
		     << "should be " << target << endl ;
		return 1 ;
	    }
	    ifstream strm( target.c_str() ) ;
	    if( !strm )
	    {
		cerr << "Resulting file " << result << " doesn't exist" << endl;
		return 1 ;
	    }
	    char line[80] ;
	    strm.getline( (char *)line, 80 ) ;
	    string sline = line ;
	    if( sline != "This is a test of a compression method." )
	    {
		cerr << "Contents of file not correct" << endl ;
		cerr << "Actual: " << sline << endl ;
		cerr << "Should be: This is a test of a compression method."
		     << endl ;
		return 1 ;
	    }
	    else
	    {
		cout << "Contents of file correct" << endl ;
	    }
	}
	catch( BESException &e )
	{
	    cerr << "Failed to uncompress the file" << endl ;
	    cerr << e.get_message() << endl ;
	    return 1 ;
	}
	catch( ... )
	{
	    cerr << "Failed to uncompress the file" << endl ;
	    cerr << "Unknown exception thrown" << endl ;
	    return 1 ;
	}

	if( cache.is_cached( src_file, tmp ) )
	{
	    cout << "File is still cached" << endl ;
	}
	else
	{
	    cerr << "File should be cached" << endl ;
	    return 1 ;
	}

    }
    catch( BESException &e )
    {
	cerr << "Unable to create the cache object" << endl ;
	cerr << e.get_message() << endl ;
	return 1 ;
    }
    catch( ... )
    {
	cerr << "Unable to create the cache object" << endl ;
	cerr << "Unknown exception thrown" << endl ;
	return 1 ;
    }

    cout << endl << "*****************************************" << endl;
    cout << "Returning from gzT::run" << endl;

    return retVal;
}

int
main(int argC, char **argV) {
    Application *app = new gzT();
    return app->main(argC, argV);
}

