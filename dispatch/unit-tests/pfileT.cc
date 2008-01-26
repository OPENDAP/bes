// pfileT.C

#include <iostream>

using std::cerr ;
using std::cout ;
using std::endl ;

#include "pfileT.h"
#include "BESContainerStorageFile.h"
#include "BESContainer.h"
#include "BESError.h"
#include "BESTextInfo.h"
#include "TheBESKeys.h"
#include <test_config.h>

int pfileT::
run(void)
{
    BESKeys *keys = TheBESKeys::TheKeys() ;
    keys->set_key( (string)"BES.Container.Persistence.File.FileTooMany=" + TEST_SRC_DIR + "/persistence_file3.txt" ) ;
    keys->set_key( (string)"BES.Container.Persistence.File.FileTooFew=" + TEST_SRC_DIR + "/persistence_file4.txt" ) ;
    keys->set_key( (string)"BES.Container.Persistence.File.File1=" + TEST_SRC_DIR + "/persistence_file1.txt" ) ;
    keys->set_key( (string)"BES.Container.Persistence.File.File2=" + TEST_SRC_DIR + "/persistence_file2.txt" ) ;

    cout << endl << "*****************************************" << endl;
    cout << "Entered pfileT::run" << endl;
    int retVal = 0;

    cout << endl << "*****************************************" << endl;
    cout << "Try to get one called File" << endl;
    try
    {
	BESContainerStorageFile cpf( "File" ) ;
	cerr << "opened file File, shouldn't have" << endl ;
	return 1 ;
    }
    catch( BESError &ex )
    {
	cout << "couldn't get File, good, because" << endl ;
	cout << ex.get_message() << endl ;
    }

    cout << endl << "*****************************************" << endl;
    cout << "Try to get one called FileNot" << endl;
    try
    {
	BESContainerStorageFile cpf( "FileNot" ) ;
	cerr << "opened file FileNot, shouldn't have" << endl ;
	return 1 ;
    }
    catch( BESError &ex )
    {
	cout << "couldn't get FileNot, good, because" << endl ;
	cout << ex.get_message() << endl ;
    }

    cout << endl << "*****************************************" << endl;
    cout << "Try to get one called FileTooMany" << endl;
    try
    {
	BESContainerStorageFile cpf( "FileTooMany" ) ;
	cerr << "opened file FileTooMany, shouldn't have" << endl ;
	return 1 ;
    }
    catch( BESError &ex )
    {
	cout << "couldn't get FileTooMany, good, because" << endl ;
	cout << ex.get_message() << endl ;
    }

    cout << endl << "*****************************************" << endl;
    cout << "Try to get one called FileTooFew" << endl;
    try
    {
	BESContainerStorageFile cpf( "FileTooFew" ) ;
	cerr << "opened file FileTooFew, shouldn't have" << endl ;
	return 1 ;
    }
    catch( BESError &ex )
    {
	cout << "couldn't get FileTooFew, good, because" << endl ;
	cout << ex.get_message() << endl ;
    }

    cout << endl << "*****************************************" << endl;
    cout << "Get one called File1" << endl;
    try
    {
	BESContainerStorageFile cpf( "File1" ) ;
	cout << "opened file File1, good" << endl ;
    }
    catch( BESError &ex )
    {
	cerr << "couldn't get File1 because" << endl ;
	cerr << ex.get_message() << endl ;
	return 1 ;
    }

    BESContainerStorageFile cpf( "File1" ) ;
    char s[10] ;
    char r[10] ;
    char c[10] ;
    for( int i = 1; i < 6; i++ )
    {
	sprintf( s, "sym%d", i ) ;
	sprintf( r, "real%d", i ) ;
	sprintf( c, "type%d", i ) ;
	cout << endl << "*****************************************" << endl;
	cout << "Looking for " << s << endl;
	BESContainer *d = cpf.look_for( s ) ;
	if( d )
	{
	    if( d->get_real_name() == r && d->get_container_type() == c )
	    {
		cout << "found " << s << endl ;
	    }
	    else
	    {
		cerr << "found " << s << " but real = " << r
		     << " and container = " << c << endl ;
		return 1 ;
	    }
	}
	else
	{
	    cerr << "couldn't find " << s << endl ;
	    return 1 ;
	}
    }

    cout << endl << "*****************************************" << endl;
    cout << "Looking for thingy" << endl;
    BESContainer *d = cpf.look_for( "thingy" ) ;
    if( d )
    {
	cerr << "found thingy" << endl ;
	return 1 ;
    }
    else
    {
	cout << "didn't find thingy, good" << endl ;
    }

    cout << endl << "*****************************************" << endl;
    cout << "show containers" << endl;
    BESTextInfo info ;
    cpf.show_containers( info ) ;
    info.print( cout ) ;

    cout << endl << "*****************************************" << endl;
    cout << "Returning from pfileT::run" << endl;

    return retVal;
}

int
main(int argC, char **argV) {
    string env_var = (string)"BES_CONF=" + TEST_SRC_DIR
                     + "/persistence_file_test.ini" ;
    putenv( (char *)env_var.c_str() ) ;
    Application *app = new pfileT();
    return app->main(argC, argV);
}

