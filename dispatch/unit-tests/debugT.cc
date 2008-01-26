// debugT.C

#include <unistd.h>

#include <iostream>
#include <sstream>

using std::cerr ;
using std::cout ;
using std::endl ;
using std::ostringstream ;

#include "debugT.h"
#include "BESDebug.h"
#include "BESError.h"
#include "test_config.h"

int
debugT::run(void)
{
    cout << endl << "*****************************************" << endl;
    cout << "Entered debugT::run" << endl;
    int retVal = 0;

    if( !_tryme.empty() )
    {
	cout << endl << "*****************************************" << endl;
	cout << "trying " << _tryme << endl;
    }
    else
    {
	try
	{
	    cout << endl << "*****************************************" << endl;
	    cout << "Setting up with bad file name /bad/dir/debug" << endl;
	    BESDebug::SetUp( "/bad/dir/debug,nc" ) ;
	    cerr << "Successfully set up, shouldn't have" << endl ;
	    return 1 ;
	}
	catch( BESError &e )
	{
	    cout << "Unable to set up debug ... good" << endl ;
	}

	try
	{
	    cout << endl << "*****************************************" << endl;
	    cout << "Setting up myfile.debug,nc,cdf,hdf4" << endl;
	    BESDebug::SetUp( "myfile.debug,nc,cdf,hdf4" ) ;
	    cout << "successfully set up" << endl ;
	    int result = access( "myfile.debug", W_OK|R_OK ) ;
	    if( result == -1 )
	    {
		cerr << "File not created" << endl ;
		return 1 ;
	    }
	    BESDebug::SetStrm( 0, false ) ;
	    result = remove( "myfile.debug" ) ;
	    if( result == -1 )
	    {
		cerr << "Unable to remove the debug file" << endl ;
		return 1 ;
	    }
	}
	catch( BESError &e )
	{
	    cerr << "Unable to set up debug ... should have worked" << endl ;
	    return 1 ;
	}

	try
	{
	    cout << endl << "*****************************************" << endl;
	    cout << "Setting up cerr,ff" << endl;
	    BESDebug::SetUp( "cerr,ff,-cdf" ) ;
	    cout << "Successfully set up" << endl ;
	}
	catch( BESError &e )
	{
	    cerr << "Unable to set up debug ... should have worked" << endl ;
	    return 1 ;
	}

	cout << endl << "*****************************************" << endl;
	cout << "try debugging to nc" << endl;
	ostringstream nc ;
	BESDebug::SetStrm( &nc, false ) ;
	string debug_str = "Testing nc debug" ;
	BESDEBUG( "nc", debug_str ) ;
	if( nc.str() != debug_str )
	{
	    cerr << "incorrect debug information: " << nc.str() << endl ;
	    return 1 ;
	}

	cout << endl << "*****************************************" << endl;
	cout << "try debugging to hdf4" << endl;
	ostringstream hdf4 ;
	BESDebug::SetStrm( &hdf4, false ) ;
	debug_str = "Testing hdf4 debug" ;
	BESDEBUG( "hdf4", debug_str ) ;
	if( hdf4.str() != debug_str )
	{
	    cerr << "incorrect debug information: " << hdf4.str() << endl ;
	    return 1 ;
	}

	cout << endl << "*****************************************" << endl;
	cout << "try debugging to ff" << endl;
	ostringstream ff ;
	BESDebug::SetStrm( &ff, false ) ;
	debug_str = "Testing ff debug" ;
	BESDEBUG( "ff", debug_str ) ;
	if( ff.str() != debug_str )
	{
	    cerr << "incorrect debug information: " << ff.str() << endl ;
	    return 1 ;
	}

	cout << endl << "*****************************************" << endl;
	cout << "turn off ff and try debugging to ff again" << endl;
	BESDebug::Set( "ff", false ) ;
	ostringstream ff2 ;
	BESDebug::SetStrm( &ff2, false ) ;
	debug_str = "" ;
	BESDEBUG( "ff", debug_str ) ;
	if( ff2.str() != debug_str )
	{
	    cerr << "incorrect debug information: " << ff2.str() << endl ;
	    return 1 ;
	}

	cout << endl << "*****************************************" << endl;
	cout << "try debugging to cdf" << endl;
	ostringstream cdf ;
	BESDebug::SetStrm( &cdf, false ) ;
	debug_str = "" ;
	BESDEBUG( "cdf", debug_str ) ;
	if( cdf.str() != debug_str )
	{
	    cerr << "incorrect debug information: " << cdf.str() << endl ;
	    return 1 ;
	}
    }

    cout << endl << "*****************************************" << endl;
    cout << "display debug help" << endl;
    BESDebug::Help( cout ) ;

    cout << endl << "*****************************************" << endl;
    cout << "Returning from debugT::run" << endl;

    return retVal;
}

int
main(int argC, char **argV) {
    int c = 0 ;
    string tryme ;
    while( ( c = getopt( argC, argV, "d:" ) ) != EOF )
    {
	switch( c )
	{
	    case 'd':
		tryme = optarg ;
		break ;
	    default:
		cerr << "bad option to debugT" << endl ;
		cerr << "debugT -d string" << endl ;
		return 1 ;
		break ;
	}
    }
    string env_var = (string)"BES_CONF=" + TEST_SRC_DIR + "/bes.conf" ;
    putenv( (char *)env_var.c_str() ) ;

    Application *app = new debugT( tryme );
    return app->main(argC, argV);
}

