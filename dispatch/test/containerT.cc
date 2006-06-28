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
	string key = (string)"OPeNDAP.Container.Persistence.File.TheFile=" +
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
    TheBESKeys::TheKeys()->set_key( "OPeNDAP.Container.Persistence=strict" ) ;

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
    TheBESKeys::TheKeys()->set_key( "OPeNDAP.Container.Persistence=nice" ) ;

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

