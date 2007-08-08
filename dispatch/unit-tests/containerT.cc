// containerT.C

#include <iostream>

using std::cerr ;
using std::cout ;
using std::endl ;

#include "containerT.h"
#include "TheBESKeys.h"
#include "BESContainerStorageList.h"
#include "BESFileContainer.h"
#include "BESContainerStorage.h"
#include "BESContainerStorageFile.h"
#include "BESCache.h"
#include "BESException.h"
#include <test_config.h>

int containerT::
run(void)
{
    cout << endl << "*****************************************" << endl;
    cout << "Entered containerT::run" << endl;
    int retVal = 0;

    // test nice, can't find
    // test nice, can find

    try
    {
	string key = (string)"BES.Container.Persistence.File.TheFile=" +
		     TEST_SRC_DIR + "/container01.file" ;
	TheBESKeys::TheKeys()->set_key( key ) ;
	BESContainerStorageList::TheList()->add_persistence( new BESContainerStorageFile( "TheFile" ) ) ;
    }
    catch( BESException &e )
    {
	cerr << "couldn't add storage to storage list:" << endl ;
	cerr << e.get_message() << endl ;
	return 1 ;
    }

    cout << endl << "*****************************************" << endl;
    cout << "try to find symbolic name that doesn't exist, default" << endl;
    try
    {
	BESContainer *c =
	    BESContainerStorageList::TheList()->look_for( "nosym" ) ;
	cerr << "Found nosym, shouldn't have" << endl ;
	if( c )
	    cerr << "container is valid, should not be" << endl ;
	cerr << " real_name = " << c->get_real_name() << endl ;
	cerr << " constraint = " << c->get_constraint() << endl ;
	cerr << " sym_name = " << c->get_symbolic_name() << endl ;
	cerr << " container type = " << c->get_container_type() << endl ;
	return 1 ;
    }
    catch( BESException &e )
    {
	cout << "caught exception, didn't find nosym, good" << endl ;
	cout << e.get_message() << endl ;
    }

    cout << endl << "*****************************************" << endl;
    cout << "try to find symbolic name that does exist, default" << endl;
    try
    {
	BESContainer *c =
	    BESContainerStorageList::TheList()->look_for( "sym1" ) ;
	if( c ) cout << "found sym1" << endl ;
	if( c->get_symbolic_name() != "sym1" )
	{
	    cerr << "symbolic name != sym1, " << c->get_symbolic_name()
		 << endl ; 
	    return 1 ;
	}
	if( c->get_real_name() != "real1" )
	{
	    cerr << "real name != real1, " << c->get_real_name()
		 << endl ; 
	    return 1 ;
	}
	if( c->get_container_type() != "type1" )
	{
	    cerr << "real name != type1, " << c->get_container_type()
		 << endl ; 
	    return 1 ;
	}
	delete c ;
    }
    catch( BESException &e )
    {
	cerr << "didn't find sym1, should have" << endl ;
	return 1 ;
    }

    cout << endl << "*****************************************" << endl;
    cout << "set to strict" << endl;
    TheBESKeys::TheKeys()->set_key( "BES.Container.Persistence=strict" ) ;

    cout << endl << "*****************************************" << endl;
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
	    cerr << " container type = " << c->get_container_type() << endl ;
	}
	else
	{
	    cerr << "look_for returned with null c, should have thrown"
	         << endl ;
	}
	return 1 ;
    }
    catch( BESException &e )
    {
	cout << "caught exception, didn't find nosym, good" << endl ;
	cout << e.get_message() << endl ;
    }

    cout << endl << "*****************************************" << endl;
    cout << "try to find symbolic name that does exist, strict" << endl;
    try
    {
	BESContainer *c = 
	    BESContainerStorageList::TheList()->look_for( "sym1" ) ;
	if( c )
	{
	    cout << "found sym1" << endl ;
	    if( c->get_symbolic_name() != "sym1" )
	    {
		cerr << "symbolic name != sym1, " << c->get_symbolic_name()
		     << endl ; 
		return 1 ;
	    }
	    if( c->get_real_name() != "real1" )
	    {
		cerr << "real name != real1, " << c->get_real_name()
		     << endl ; 
		return 1 ;
	    }
	    if( c->get_container_type() != "type1" )
	    {
		cerr << "real name != type1, " << c->get_container_type()
		     << endl ; 
		return 1 ;
	    }
	}
	else
	{
	    cerr << "returned but not found" << endl ;
	    return 1 ;
	}
    }
    catch( BESException &e )
    {
	cerr << "didn't find sym1, should have" << endl ;
	return 1 ;
    }

    cout << endl << "*****************************************" << endl;
    cout << "set to nice" << endl;
    TheBESKeys::TheKeys()->set_key( "BES.Container.Persistence=nice" ) ;

    cout << endl << "*****************************************" << endl;
    cout << "try to find symbolic name that doesn't exist, nice" << endl;
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
	    cerr << " container type = " << c->get_container_type() << endl ;
	    return 1 ;
	}
	else
	{
	    cout << "didn't find nosym, didn't throw exception, good" << endl ;
	}
    }
    catch( BESException &e )
    {
	cerr << "caught exception, shouldn't have" << endl ;
	cerr << e.get_message() << endl ;
	return 1 ;
    }

    cout << endl << "*****************************************" << endl;
    cout << "try to find symbolic name that does exist, nice" << endl;
    try
    {
	BESContainer *c =
	    BESContainerStorageList::TheList()->look_for( "sym1" ) ;
	if( c )
	{
	    if( c->get_symbolic_name() != "sym1" )
	    {
		cerr << "symbolic name != sym1, " << c->get_symbolic_name()
		     << endl ; 
		return 1 ;
	    }
	    if( c->get_real_name() != "real1" )
	    {
		cerr << "real name != real1, " << c->get_real_name()
		     << endl ; 
		return 1 ;
	    }
	    if( c->get_container_type() != "type1" )
	    {
		cerr << "real name != type1, " << c->get_container_type()
		     << endl ; 
		return 1 ;
	    }
	}
	else
	{
	    cerr << "didn't find sym1" << endl ;
	    return 1 ;
	}
    }
    catch( BESException &e )
    {
	cerr << "didn't find sym1, should have" << endl ;
	return 1 ;
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

    cout << endl << "*****************************************" << endl;
    cout << "access a non compressed file" << endl;
    if( !isdotdot )
    {
	try
	{
	    BESFileContainer c( "sym", src_file, "txt" ) ;

	    string result = c.access() ;
	    if( result != src_file )
	    {
		cerr << "result " << result << " does not match src "
		     << src_file << endl ;
		return 1 ;
	    }
	    else
	    {
		cout << "result matches src" << endl ;
	    }
	}
	catch( BESException &e )
	{
	    cerr << "Failed to access non compressed file" << endl ;
	    cerr << e.get_message() << endl ;
	    return 1 ;
	}
	catch( ... )
	{
	    cerr << "Failed to access non compressed file" << endl ;
	    cerr << "Unknown error" << endl ;
	    return 1 ;
	}
    }
    else
    {
	try
	{
	    BESFileContainer c( "sym", src_file, "txt" ) ;

	    string result = c.access() ;
	    cerr << "Should have failed with ../ in container real name: "
	         << src_file << endl ;
	    return 1 ;
	}
	catch( BESException &e )
	{
	    cout << "Failed to access file with ../ in name, good" << endl ;
	}
	catch( ... )
	{
	    cerr << "Failed to access non compressed file" << endl ;
	    cerr << "Unknown error" << endl ;
	    return 1 ;
	}
    }

    cout << endl << "*****************************************" << endl;
    cout << "access a compressed file" << endl;
    if( !isdotdot )
    {
	try
	{
	    BESCache cache( *(TheBESKeys::TheKeys()),
			    "BES.CacheDir", "BES.CachePrefix", "BES.CacheSize" ) ;
	    string target ;
	    bool is_it = cache.is_cached( com_file, target ) ;
	    if( is_it )
	    {
		if( remove( target.c_str() ) != 0 )
		{
		    cerr << "Unable to remove target file " << target
			 << " , initializing test" << endl ;
		    return 1 ;
		}
	    }

	    BESFileContainer c( "sym", com_file, "txt" ) ;

	    string result = c.access() ;
	    if( result != target )
	    {
		cerr << "result " << result << " does not match target "
		     << target << endl ;
		return 1 ;
	    }
	    else
	    {
		cout << "result matches src" << endl ;
	    }

	    if( cache.is_cached( com_file, target ) )
	    {
		cout << "file is now cached" << endl ;
	    }
	    else
	    {
		cerr << "file should be cached in " << target << endl ;
		return 1 ;
	    }
	}
	catch( BESException &e )
	{
	    cerr << "Failed to access compressed file" << endl ;
	    cerr << e.get_message() << endl ;
	    return 1 ;
	}
	catch( ... )
	{
	    cerr << "Failed to access compressed file" << endl ;
	    cerr << "Unknown error" << endl ;
	    return 1 ;
	}
    }
    else
    {
	try
	{
	    BESCache cache( *(TheBESKeys::TheKeys()),
			    "BES.CacheDir", "BES.CachePrefix", "BES.CacheSize" ) ;
	    string target ;
	    bool is_it = cache.is_cached( com_file, target ) ;
	    if( is_it )
	    {
		if( remove( target.c_str() ) != 0 )
		{
		    cerr << "Unable to remove target file " << target
			 << " , initializing test" << endl ;
		    return 1 ;
		}
	    }

	    BESFileContainer c( "sym", com_file, "txt" ) ;

	    string result = c.access() ;
	    cerr << "Should have failed with ../ in container real name: "
	         << com_file << endl ;
	    return 1 ;
	}
	catch( BESException &e )
	{
	    cout << "Failed to access file with ../ in name, good" << endl ;
	}
	catch( ... )
	{
	    cerr << "Failed to access compressed file" << endl ;
	    cerr << "Unknown error" << endl ;
	    return 1 ;
	}
    }

    cout << endl << "*****************************************" << endl;
    cout << "Returning from containerT::run" << endl;

    return retVal;
}

int
main(int argC, char **argV) {
    Application *app = new containerT();
    string env_var = (string)"BES_CONF=" + TEST_SRC_DIR + "/empty.ini" ;
    putenv( (char *)env_var.c_str() ) ;
    return app->main(argC, argV);
}

