// authtestT.cc

#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <iostream>

using std::cerr ;
using std::cout ;
using std::endl ;

#include "authtestT.h"
#include "DODSContainerPersistenceMySQL.h"
#include "TheDODSKeys.h"
#include "DODSMySQLAuthenticate.h"
#include "DODSDataRequestInterface.h"
#include "DODSException.h"

string
get_ip()
{
    char name[256] ;
    gethostname( name, 256 ) ;

    struct hostent *hp = gethostbyname( name ) ;

    char **p = hp->h_addr_list ;
    struct in_addr in;
    (void)memcpy( &in.s_addr, *p, sizeof( in.s_addr ) ) ;
    string s = inet_ntoa( in ) ;

    return s ;
}

int authtestT::
run(void)
{
    cout << endl << "*****************************************" << endl;
    cout << "Entered authtestT::run" << endl;
    int retVal = 0;

    TheDODSKeys->set_key( "DODS.Authenticate.MySQL.server=cedar-l" ) ;
    TheDODSKeys->set_key( "DODS.Authenticate.MySQL.user=cedardb" ) ;
    TheDODSKeys->set_key( "DODS.Authenticate.MySQL.password=1001000110110101110101111001101010111101001111010100110010101000" ) ;
    TheDODSKeys->set_key( "DODS.Authenticate.MySQL.database=CEDARAUTHEN" ) ;
    TheDODSKeys->set_key( "DODS.Authenticate.MySQL.mode=on" ) ;

    try
    {
	cout << "authenticating pwest at 0.0.0.1" << endl ;
	DODSDataHandlerInterface dhi ;
	dhi.user_address = "0.0.0.1" ;
	dhi.user_name = "pwest" ;

	DODSMySQLAuthenticate auth ;
	auth.authenticate( dhi ) ;

	cout << "user authenticated, good" << endl ;
    }
    catch( DODSException &e )
    {
	string err = e.get_error_description() ;
	cerr << err << endl ;
    }
    catch( ... )
    {
	cerr << "unknown exception" << endl ;
    }

    try
    {
	cout << "authenticating pwest at 0.0.0.5" << endl ;
	DODSDataHandlerInterface dhi ;
	dhi.user_address = "0.0.0.5" ;
	dhi.user_name = "pwest" ;

	DODSMySQLAuthenticate auth ;
	auth.authenticate( dhi ) ;

	cout << "user authenticated, good" << endl ;
    }
    catch( DODSException &e )
    {
	string err = e.get_error_description() ;
	cerr << err << endl ;
    }
    catch( ... )
    {
	cerr << "unknown exception" << endl ;
    }

    try
    {
	cout << "authenticating pwest at 0.0.0.6" << endl ;
	DODSDataHandlerInterface dhi ;
	dhi.user_address = "0.0.0.6" ;
	dhi.user_name = "pwest" ;

	DODSMySQLAuthenticate auth ;
	auth.authenticate( dhi ) ;

	cerr << "user authenticated, bad" << endl ;
    }
    catch( DODSException &e )
    {
	cout << "failed to authenticate, good" << endl ;
	string err = e.get_error_description() ;
	cout << err << endl ;
    }
    catch( ... )
    {
	cerr << "unknown exception" << endl ;
    }

    try
    {
	cout << "authenticating nouser at 0.0.0.1" << endl ;
	DODSDataHandlerInterface dhi ;
	dhi.user_address = "0.0.0.1" ;
	dhi.user_name = "nouser" ;

	DODSMySQLAuthenticate auth ;
	auth.authenticate( dhi ) ;

	cerr << "user authenticated, bad" << endl ;
    }
    catch( DODSException &e )
    {
	cout << "failed to authenticate, good" << endl ;
	string err = e.get_error_description() ;
	cout << err << endl ;
    }
    catch( ... )
    {
	cerr << "unknown exception" << endl ;
    }

    try
    {
	cout << "authenticating nouser at 0.0.0.6" << endl ;
	DODSDataHandlerInterface dhi ;
	dhi.user_address = "0.0.0.6" ;
	dhi.user_name = "nouser" ;

	DODSMySQLAuthenticate auth ;
	auth.authenticate( dhi ) ;

	cerr << "user authenticated, bad" << endl ;
    }
    catch( DODSException &e )
    {
	cout << "failed to authenticate, good" << endl ;
	string err = e.get_error_description() ;
	cout << err << endl ;
    }
    catch( ... )
    {
	cerr << "unknown exception" << endl ;
    }

    cout << endl << "*****************************************" << endl;
    cout << "Returning from authtestT::run" << endl;

    return retVal;
}

int
main(int argC, char **argV) {
    putenv( "DODS_INI=./empty.ini" ) ;
    Application *app = new authtestT();
    return app->main(argC, argV);
}

