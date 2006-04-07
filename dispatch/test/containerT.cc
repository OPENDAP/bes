// containerT.C

#include <iostream>

using std::cerr ;
using std::cout ;
using std::endl ;

#include "containerT.h"
#include "TheDODSKeys.h"
#include "ContainerStorageList.h"
#include "DODSContainer.h"
#include "ContainerStorage.h"
#include "ContainerStorageFile.h"
#include "DODSException.h"

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
	TheDODSKeys::TheKeys()->set_key( key ) ;
	ContainerStorageList::TheList()->add_persistence( new ContainerStorageFile( "TheFile" ) ) ;
    }
    catch( DODSException &e )
    {
	cerr << "couldn't add storage to storage list:" << endl ;
	cerr << e.get_error_description() << endl ;
	return 1 ;
    }

    cout << endl << "*****************************************" << endl;
    cout << "try to find symbolic name that doesn't exist, default" << endl;
    try
    {
	DODSContainer c( "nosym" ) ;
	ContainerStorageList::TheList()->look_for( c ) ;
	cerr << "Found nosym, shouldn't have" << endl ;
	if( c.is_valid() == true )
	    cerr << "container is valid, should not be" << endl ;
	cerr << " real_name = " << c.get_real_name() << endl ;
	cerr << " constraint = " << c.get_constraint() << endl ;
	cerr << " sym_name = " << c.get_symbolic_name() << endl ;
	cerr << " container type = " << c.get_container_type() << endl ;
	return 1 ;
    }
    catch( DODSException &e )
    {
	cout << "caught exception, didn't find nosym, good" << endl ;
	cout << e.get_error_description() << endl ;
    }

    cout << endl << "*****************************************" << endl;
    cout << "try to find symbolic name that does exist, default" << endl;
    try
    {
	DODSContainer c( "sym1" ) ;
	ContainerStorageList::TheList()->look_for( c ) ;
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
    catch( DODSException &e )
    {
	cerr << "didn't find sym1, should have" << endl ;
	return 1 ;
    }

    cout << endl << "*****************************************" << endl;
    cout << "set to strict" << endl;
    TheDODSKeys::TheKeys()->set_key( "OPeNDAP.Container.Persistence=strict" ) ;

    cout << endl << "*****************************************" << endl;
    cout << "try to find symbolic name that doesn't exist, strict" << endl;
    try
    {
	DODSContainer c( "nosym" ) ;
	ContainerStorageList::TheList()->look_for( c ) ;
	cerr << "Found nosym, shouldn't have" << endl ;
	if( c.is_valid() == true )
	    cerr << "container is valid, should not be" << endl ;
	cerr << " real_name = " << c.get_real_name() << endl ;
	cerr << " constraint = " << c.get_constraint() << endl ;
	cerr << " sym_name = " << c.get_symbolic_name() << endl ;
	cerr << " container type = " << c.get_container_type() << endl ;
	return 1 ;
    }
    catch( DODSException &e )
    {
	cout << "caught exception, didn't find nosym, good" << endl ;
	cout << e.get_error_description() << endl ;
    }

    cout << endl << "*****************************************" << endl;
    cout << "try to find symbolic name that does exist, strict" << endl;
    try
    {
	DODSContainer c( "sym1" ) ;
	ContainerStorageList::TheList()->look_for( c ) ;
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
    catch( DODSException &e )
    {
	cerr << "didn't find sym1, should have" << endl ;
	return 1 ;
    }

    cout << endl << "*****************************************" << endl;
    cout << "set to nice" << endl;
    TheDODSKeys::TheKeys()->set_key( "OPeNDAP.Container.Persistence=nice" ) ;

    cout << endl << "*****************************************" << endl;
    cout << "try to find symbolic name that doesn't exist, nice" << endl;
    try
    {
	DODSContainer c( "nosym" ) ;
	ContainerStorageList::TheList()->look_for( c ) ;
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
    catch( DODSException &e )
    {
	cerr << "caught exception, shouldn't have" << endl ;
	cerr << e.get_error_description() << endl ;
	return 1 ;
    }

    cout << endl << "*****************************************" << endl;
    cout << "try to find symbolic name that does exist, nice" << endl;
    try
    {
	DODSContainer c( "sym1" ) ;
	ContainerStorageList::TheList()->look_for( c ) ;
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
    catch( DODSException &e )
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
    putenv( "OPENDAP_INI=./empty.ini" ) ;
    return app->main(argC, argV);
}

