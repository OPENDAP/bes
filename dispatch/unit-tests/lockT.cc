// lockT.C

#include <iostream>

using std::cerr ;
using std::cout ;
using std::endl ;

#include "lockT.h"
#include "BESCache.h"
#include "BESError.h"
#include <test_config.h>

int
lockT::run(void)
{
    cout << endl << "*****************************************" << endl;
    cout << "Entered lockT::run" << endl;
    int retVal = 0;

    try
    {
	string cache_dir = (string)TEST_SRC_DIR + "/cache" ;
	BESCache cache( cache_dir, "lock_test", 1 ) ;

	cout << endl << "*****************************************" << endl;
	cout << "lock, then try to lock again" << endl;
	try
	{
	    cout << "get first lock" << endl ;
	    if( cache.lock( 2, 10 ) == false )
	    {
		cerr << "failed to lock the cache" << endl ;
		return 1 ;
	    }
	}
	catch( BESError &e )
	{
	    cerr << "locking test failed" << endl ;
	    cerr << e.get_message() << endl ;
	    cache.unlock() ;
	    return 1 ;
	}
	catch( ... )
	{
	    cerr << "locking test failed" << endl ;
	    cerr << "Unknown error" << endl ;
	    cache.unlock() ;
	    return 1 ;
	}

	try
	{
	    cout << "try to lock again" << endl ;
	    if( cache.lock( 2, 10 ) == true )
	    {
		cerr << "successfully got the lock, should not have" << endl ;
		cache.unlock() ;
		return 1 ;
	    }
	}
	catch( BESError &e )
	{
	    cout << "failed to get lock, good" << endl ;
	    cout << e.get_message() << endl ;
	}
	catch( ... )
	{
	    cerr << "failed to get lock, unkown exception" << endl ;
	    cache.unlock() ;
	    return 1 ;
	}

	cout << endl << "*****************************************" << endl;
	cout << "unlock" << endl;
	if( cache.unlock() == false )
	{
	    cerr << "failed to release the lock" << endl ;
	    return 1 ;
	}

	cout << endl << "*****************************************" << endl;
	cout << "lock the cache, create another cache and try to lock" << endl;
	try
	{
	    cout << "locking first" << endl;
	    if( cache.lock( 2, 10 ) == false )
	    {
		cerr << "failed to lock the cache" << endl ;
		return 1 ;
	    }
	}
	catch( BESError &e )
	{
	    cerr << "2 cache locking failed" << endl ;
	    cerr << e.get_message() << endl ;
	    cache.unlock() ;
	    return 1 ;
	}
	catch( ... )
	{
	    cerr << "2 cache locking failed" << endl ;
	    cerr << "Unknown error" << endl ;
	    cache.unlock() ;
	    return 1 ;
	}

	cout << "creating second" << endl;
	BESCache cache2( cache_dir, "lock_test", 1 ) ;
	try
	{
	    cout << "locking second" << endl;
	    if( cache2.lock( 2, 10 ) == false )
	    {
		cout << "failed to lock the cache, good" << endl ;
	    }
	}
	catch( BESError &e )
	{
	    cerr << "2 cache locking failed" << endl ;
	    cerr << e.get_message() << endl ;
	    cache.unlock() ;
	    return 1 ;
	}
	catch( ... )
	{
	    cerr << "2 cache locking failed" << endl ;
	    cerr << "Unknown error" << endl ;
	    cache.unlock() ;
	    return 1 ;
	}

	cout << endl << "*****************************************" << endl;
	cout << "unlock the first cache" << endl;
	if( cache.unlock() == false )
	{
	    cerr << "failed to release the lock" << endl ;
	    return 1 ;
	}

	cout << endl << "*****************************************" << endl;
	cout << "lock the second cache" << endl;
	try
	{
	    if( cache2.lock( 2, 10 ) == true )
	    {
		cout << "got the lock, good" << endl ;
	    }
	}
	catch( BESError &e )
	{
	    cerr << "locking second cache failed" << endl ;
	    cerr << e.get_message() << endl ;
	    cache.unlock() ;
	    return 1 ;
	}
	catch( ... )
	{
	    cerr << "locking second cache failed" << endl ;
	    cerr << "Unknown error" << endl ;
	    cache.unlock() ;
	    return 1 ;
	}

	cout << endl << "*****************************************" << endl;
	cout << "unlock the second cache" << endl;
	if( cache2.unlock() == false )
	{
	    cerr << "failed to release the lock" << endl ;
	    return 1 ;
	}
    }
    catch( BESError &e )
    {
	cerr << "Failed to use the cache" << endl ;
	cerr << e.get_message() << endl ;
	return 1 ;
    }
    catch( ... )
    {
	cerr << "Failed to use the cache" << endl ;
	cerr << "Unknown error" << endl ;
	return 1 ;
    }

    cout << endl << "*****************************************" << endl;
    cout << "Returning from lockT::run" << endl;

    return retVal;
}

int
main(int argC, char **argV) {
    string env_var = (string)"BES_CONF=" + TEST_SRC_DIR + "/bes.conf" ;
    putenv( (char *)env_var.c_str() ) ;
    Application *app = new lockT();
    return app->main(argC, argV);
}

