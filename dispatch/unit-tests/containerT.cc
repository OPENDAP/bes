// containerT.C

#include <iostream>

using std::cerr ;
using std::cout ;
using std::endl ;

#include "containerT.h"
#include "TheBESKeys.h"
#include "BESContainerStorageList.h"
#include "BESContainer.h"
#include "BESContainerStorage.h"
#include "BESContainerStorageFile.h"
#include "BESCache.h"
#include "BESException.h"

int containerT::
run(void)
{
    cout << endl << "*****************************************" << endl;
    cout << "Entered containerT::run" << endl;
    int retVal = 0;

    char *pwd = getenv( "PWD" ) ;
    string pwd_s ;
    if( !pwd )
	pwd_s = "." ;
    else
	pwd_s = pwd ;

    // test nice, can't find
    // test nice, can find

    try
    {
	string key = (string)"BES.Container.Persistence.File.TheFile=" +
		     pwd_s + "/container01.file" ;
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
	BESContainer c( "nosym" ) ;
	BESContainerStorageList::TheList()->look_for( c ) ;
	cerr << "Found nosym, shouldn't have" << endl ;
	if( c.is_valid() == true )
	    cerr << "container is valid, should not be" << endl ;
	cerr << " real_name = " << c.get_real_name() << endl ;
	cerr << " constraint = " << c.get_constraint() << endl ;
	cerr << " sym_name = " << c.get_symbolic_name() << endl ;
	cerr << " container type = " << c.get_container_type() << endl ;
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
	BESContainer c( "sym1" ) ;
	BESContainerStorageList::TheList()->look_for( c ) ;
	cout << "found sym1" << endl ;
	if( c.is_valid() == false )
	{
	    cerr << "is not valid though" << endl ;
	    return 1 ;
	}
	if( c.get_symbolic_name() != "sym1" )
	{
	    cerr << "symbolic name != sym1, " << c.get_symbolic_name()
		 << endl ; 
	    return 1 ;
	}
	if( c.get_real_name() != "real1" )
	{
	    cerr << "real name != real1, " << c.get_real_name()
		 << endl ; 
	    return 1 ;
	}
	if( c.get_container_type() != "type1" )
	{
	    cerr << "real name != type1, " << c.get_container_type()
		 << endl ; 
	    return 1 ;
	}
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
	BESContainer c( "nosym" ) ;
	BESContainerStorageList::TheList()->look_for( c ) ;
	cerr << "Found nosym, shouldn't have" << endl ;
	if( c.is_valid() == true )
	    cerr << "container is valid, should not be" << endl ;
	cerr << " real_name = " << c.get_real_name() << endl ;
	cerr << " constraint = " << c.get_constraint() << endl ;
	cerr << " sym_name = " << c.get_symbolic_name() << endl ;
	cerr << " container type = " << c.get_container_type() << endl ;
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
	BESContainer c( "sym1" ) ;
	BESContainerStorageList::TheList()->look_for( c ) ;
	cout << "found sym1" << endl ;
	if( c.is_valid() == false )
	{
	    cerr << "is not valid though" << endl ;
	    return 1 ;
	}
	if( c.get_symbolic_name() != "sym1" )
	{
	    cerr << "symbolic name != sym1, " << c.get_symbolic_name()
		 << endl ; 
	    return 1 ;
	}
	if( c.get_real_name() != "real1" )
	{
	    cerr << "real name != real1, " << c.get_real_name()
		 << endl ; 
	    return 1 ;
	}
	if( c.get_container_type() != "type1" )
	{
	    cerr << "real name != type1, " << c.get_container_type()
		 << endl ; 
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
	BESContainer c( "nosym" ) ;
	BESContainerStorageList::TheList()->look_for( c ) ;
	if( c.is_valid() == true )
	{
	    cerr << "Found nosym, shouldn't have" << endl ;
	    cerr << " real_name = " << c.get_real_name() << endl ;
	    cerr << " constraint = " << c.get_constraint() << endl ;
	    cerr << " sym_name = " << c.get_symbolic_name() << endl ;
	    cerr << " container type = " << c.get_container_type() << endl ;
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
	BESContainer c( "sym1" ) ;
	BESContainerStorageList::TheList()->look_for( c ) ;
	if( c.is_valid() == false )
	{
	    cerr << "is not valid though" << endl ;
	    return 1 ;
	}
	if( c.get_symbolic_name() != "sym1" )
	{
	    cerr << "symbolic name != sym1, " << c.get_symbolic_name()
		 << endl ; 
	    return 1 ;
	}
	if( c.get_real_name() != "real1" )
	{
	    cerr << "real name != real1, " << c.get_real_name()
		 << endl ; 
	    return 1 ;
	}
	if( c.get_container_type() != "type1" )
	{
	    cerr << "real name != type1, " << c.get_container_type()
		 << endl ; 
	    return 1 ;
	}
    }
    catch( BESException &e )
    {
	cerr << "didn't find sym1, should have" << endl ;
	return 1 ;
    }

    char cur_dir[4096] ;
    getcwd( cur_dir, 4096 ) ;
    string cache_dir = (string)cur_dir + "/cache" ;
    string src_file = cache_dir + "/testfile.txt" ;
    string com_file = cache_dir + "/testfile.txt.gz" ;

    TheBESKeys::TheKeys()->set_key( "BES.CacheDir", cache_dir ) ;
    TheBESKeys::TheKeys()->set_key( "BES.CachePrefix", "cont_cache" ) ;
    TheBESKeys::TheKeys()->set_key( "BES.CacheSize", "1" ) ;

    cout << endl << "*****************************************" << endl;
    cout << "access a non compressed file" << endl;
    try
    {
	BESContainer c( "sym" ) ;
	c.set_real_name( src_file ) ;
	c.set_container_type( "txt" ) ;

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

    cout << endl << "*****************************************" << endl;
    cout << "access a compressed file" << endl;
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

	BESContainer c( "sym" ) ;
	c.set_real_name( com_file ) ;
	c.set_container_type( "txt" ) ;

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

    cout << endl << "*****************************************" << endl;
    cout << "Returning from containerT::run" << endl;

    return retVal;
}

int
main(int argC, char **argV) {
    Application *app = new containerT();
    putenv( "BES_CONF=./empty.ini" ) ;
    return app->main(argC, argV);
}

