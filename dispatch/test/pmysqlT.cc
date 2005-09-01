// pmysqlT.cc

#include <iostream>

using std::cerr ;
using std::cout ;
using std::endl ;

#include "pmysqlT.h"
#include "DODSContainerPersistenceMySQL.h"
#include "DODSContainer.h"
#include "TheDODSKeys.h"
#include "DODSException.h"
#include "DODSTextInfo.h"

int pmysqlT::
run(void)
{
    cout << endl << "*****************************************" << endl;
    cout << "Entered pmysqlT::run" << endl;
    int retVal = 0;

    cout << endl << "*****************************************" << endl;
    cout << "Try to create without any sql params set" << endl;
    try
    {
	DODSContainerPersistenceMySQL cpm( "MySQL" ) ;
	cerr << "created persistence, shouldn't have" << endl ;
	return 1 ;
    }
    catch( DODSException &ex )
    {
	cout << "couldn't create persistence, good, because" << endl ;
	cout << ex.get_error_description() << endl ;
    }

    cout << endl << "*****************************************" << endl;
    cout << "Set server" << endl;
    TheDODSKeys->set_key( "DODS.Container.Persistence.MySQL.MySQL.server=no_server" ) ;

    cout << endl << "*****************************************" << endl;
    cout << "Try to create with only server" << endl;
    try
    {
	DODSContainerPersistenceMySQL cpm( "MySQL" ) ;
	cerr << "created persistence, shouldn't have" << endl ;
	return 1 ;
    }
    catch( DODSException &ex )
    {
	cout << "couldn't create persistence, good, because" << endl ;
	cout << ex.get_error_description() << endl ;
    }

    cout << endl << "*****************************************" << endl;
    cout << "Set user" << endl;
    TheDODSKeys->set_key( "DODS.Container.Persistence.MySQL.MySQL.user=no_user" ) ;

    cout << endl << "*****************************************" << endl;
    cout << "Try to create with only server and user" << endl;
    try
    {
	DODSContainerPersistenceMySQL cpm( "MySQL" ) ;
	cerr << "created persistence, shouldn't have" << endl ;
	return 1 ;
    }
    catch( DODSException &ex )
    {
	cout << "couldn't create persistence, good, because" << endl ;
	cout << ex.get_error_description() << endl ;
    }

    cout << endl << "*****************************************" << endl;
    cout << "Set password" << endl;
    TheDODSKeys->set_key( "DODS.Container.Persistence.MySQL.MySQL.password=no_pwd" ) ;

    cout << endl << "*****************************************" << endl;
    cout << "Try to create with only server and user and password" << endl;
    try
    {
	DODSContainerPersistenceMySQL cpm( "MySQL" ) ;
	cerr << "created persistence, shouldn't have" << endl ;
	return 1 ;
    }
    catch( DODSException &ex )
    {
	cout << "couldn't create persistence, good, because" << endl ;
	cout << ex.get_error_description() << endl ;
    }

    cout << endl << "*****************************************" << endl;
    cout << "Set incorrect database" << endl;
    TheDODSKeys->set_key( "DODS.Container.Persistence.MySQL.MySQL.database=no_db" ) ;

    cout << endl << "*****************************************" << endl;
    cout << "Should create persistence, but not connect" << endl;
    try
    {
	DODSContainerPersistenceMySQL cpm( "MySQL" ) ;
	cerr << "created persistence, shouldn't have" << endl ;
	return 1 ;
    }
    catch( DODSException &ex )
    {
	cout << "couldn't create persistence, good, because" << endl ;
	cout << ex.get_error_description() << endl ;
    }

    cout << endl << "*****************************************" << endl;
    cout << "Set good server" << endl;
    TheDODSKeys->set_key( "DODS.Container.Persistence.MySQL.MySQL.server=cedar-l" ) ;

    cout << endl << "*****************************************" << endl;
    cout << "Try to create with bad user" << endl;
    try
    {
	DODSContainerPersistenceMySQL cpm( "MySQL" ) ;
	cerr << "created persistence, shouldn't have" << endl ;
	return 1 ;
    }
    catch( DODSException &ex )
    {
	cout << "couldn't create persistence, good, because" << endl ;
	cout << ex.get_error_description() << endl ;
    }

    cout << endl << "*****************************************" << endl;
    cout << "Set good user" << endl;
    TheDODSKeys->set_key( "DODS.Container.Persistence.MySQL.MySQL.user=cedardb" ) ;

    cout << endl << "*****************************************" << endl;
    cout << "Try to create with bad password" << endl;
    try
    {
	DODSContainerPersistenceMySQL cpm( "MySQL" ) ;
	cerr << "created persistence, shouldn't have" << endl ;
	return 1 ;
    }
    catch( DODSException &ex )
    {
	cout << "couldn't create persistence, good, because" << endl ;
	cout << ex.get_error_description() << endl ;
    }

    cout << endl << "*****************************************" << endl;
    cout << "Set good password" << endl;
    TheDODSKeys->set_key( "DODS.Container.Persistence.MySQL.MySQL.password=1001000110110101110101111001101010111101001111010100110010101000" ) ;

    cout << endl << "*****************************************" << endl;
    cout << "Try to create with bad database" << endl;
    try
    {
	DODSContainerPersistenceMySQL cpm( "MySQL" ) ;
	cerr << "created persistence, shouldn't have" << endl ;
	return 1 ;
    }
    catch( DODSException &ex )
    {
	cout << "couldn't create persistence, good, because" << endl ;
	cout << ex.get_error_description() << endl ;
    }

    cout << endl << "*****************************************" << endl;
    cout << "Set empty database" << endl;
    TheDODSKeys->set_key( "DODS.Container.Persistence.MySQL.MySQL.database=" ) ;

    cout << endl << "*****************************************" << endl;
    cout << "Try to create with empty database name" << endl;
    try
    {
	DODSContainerPersistenceMySQL cpm( "MySQL" ) ;
	cout << "created persistence" << endl ;
	try
	{
	    DODSContainer d( "no_sym" ) ;
	    cpm.look_for( d ) ;
	    cerr << "found no_sym, shouldn't have" << endl ;
	    return 1 ;
	}
	catch( DODSException &ex )
	{
	    cout << "connected and couldn't find no_sym" << endl ;
	    cout << ex.get_error_description() << endl ;
	}
    }
    catch( DODSException &ex )
    {
	cerr << "couldn't create persistence" << endl ;
	cerr << ex.get_error_description() << endl ;
	return 1 ;
    }

    cout << endl << "*****************************************" << endl;
    cout << "Set correct database" << endl;
    string val = TheDODSKeys->set_key( "DODS.Container.Persistence.MySQL.MySQL.database=DODS_CONTAINERS" ) ;
    cerr << "set database to " << val << endl ;

    cout << endl << "*****************************************" << endl;
    cout << "Create persistence" << endl;
    try
    {
	DODSContainerPersistenceMySQL cpm( "MySQL" ) ;
	cout << "created persistence" << endl ;
	try
	{
	    {
		cout << endl << "*****************************************" << endl;
		cout << "look for no_sym" << endl;
		DODSContainer d( "no_sym" ) ;
		cpm.look_for( d ) ;
		if( d.is_valid() )
		{
		    cerr << "found no_sym, shouldn't have" << endl ;
		    cerr << "real_name = " << d.get_real_name() << endl ;
		    cerr << "container_type = " << d.get_container_type() << endl;
		    return 1 ;
		}
		else
		{
		    cout << "couldn't find no_sym, good" << endl ;
		}
	    }

	    {
		cout << endl << "*****************************************" << endl;
		cout << "look for cedar1" << endl;
		DODSContainer d( "cedarl" ) ;
		cpm.look_for( d ) ;
		if( d.is_valid() )
		{
		    cout << "found cedar1 with:" << endl ;
		    string real_name = d.get_real_name() ;
		    cout << "  real_name = " << real_name << endl ;
		    string container_type = d.get_container_type() ;
		    cout << "  container_type = " << container_type << endl ;
		}
		else
		{
		    cerr << "couldn't find cedar1" << endl ;
		    return 1 ;
		}
	    }
	}
	catch( DODSException &ex )
	{
	    cerr << "failed to look for no_sym" << endl ;
	    cerr << ex.get_error_description() << endl ;
	    return 1 ;
	}
    }
    catch( DODSException &ex )
    {
	cerr << "couldn't create persistence" << endl ;
	cerr << ex.get_error_description() << endl ;
	return 1 ;
    }

    cout << endl << "*****************************************" << endl;
    cout << "Create persistence" << endl;
    try
    {
	DODSContainerPersistenceMySQL cpm( "MySQL" ) ;
	DODSTextInfo info( false ) ;
	cpm.show_containers( info ) ;
	info.print( stdout ) ;
    }
    catch( DODSException &ex )
    {
	cerr << "couldn't create persistence" << endl ;
	cerr << ex.get_error_description() << endl ;
	return 1 ;
    }

    cout << endl << "*****************************************" << endl;
    cout << "Returning from pmysqlT::run" << endl;

    return retVal;
}

int
main(int argC, char **argV) {
    putenv( "DODS_INI=./persistence_mysql_test.ini" ) ;
    Application *app = new pmysqlT();
    return app->main(argC, argV);
}

