// keysT.C

#include <cstdlib>
#include <iostream>

using std::cerr ;
using std::cout ;
using std::endl ;

#include "keysT.h"
#include "TheBESKeys.h"
#include "BESError.h"
#include <test_config.h>

int keysT::
initialize( int argC, char **argV )
{
    if( argC == 2 )
    {
	_keyFile = argV[1] ;
    }

    return baseApp::initialize( argC, argV ) ;
}

int keysT::
run(void)
{
    cout << endl << "*****************************************" << endl;
    cout << "Entered keysT::run" << endl;
    int retVal = 0;

    if( _keyFile != "" )
    {
	char envVal[256] ;
	sprintf( envVal, "BES_CONF=%s", _keyFile.c_str() ) ;
	putenv( envVal ) ;
	try
	{
	    TheBESKeys::TheKeys()->dump( cout ) ;
	}
	catch( BESError &e )
	{
	    cout << "unable to create BESKeys:" << endl ;
	    cout << e.get_message() << endl ;
	}
	catch( ... )
	{
	    cout << "unable to create BESKeys: unkown exception caught"
	         << endl ;
	}

	return 0 ;
    }

    cout << endl << "*****************************************" << endl;
    cout << "no file set" << endl;
    putenv( "BES_CONF=" ) ;
    try
    {
	TheBESKeys::TheKeys() ;
	cerr << "created, should have not been created" << endl ;
	return 1 ;
    }
    catch( BESError &e )
    {
	cout << "unable to create BESKeys, good, because:" << endl ;
	// have to comment this out because the error includes the current
	// working directory of where it is looking for the config file.
	//cout << e.get_message() << endl ;
    }

    cout << endl << "*****************************************" << endl;
    cout << "notfound file set" << endl;
    putenv( "BES_CONF=notfound.ini" ) ;
    try
    {
	TheBESKeys::TheKeys() ;
	cerr << "created, should have not been created" << endl ;
	return 1 ;
    }
    catch( BESError &e )
    {
	cout << "unable to create BESKeys, good, because:" << endl ;
	// have to comment this out because the error includes the current
	// working directory of where it is looking for the config file.
	//cout << e.get_message() << endl ;
    }

    cout << endl << "*****************************************" << endl;
    cout << "bad keys, not enough equal signs" << endl;
    char *pwd = getenv( "PWD" ) ;
    string pwd_s ;
    if( !pwd )
	pwd_s = "." ;
    else
	pwd_s = pwd ;
    string env_s = (string)"BES_CONF=" + TEST_SRC_DIR + "/bad_keys1.ini" ;
    char env1[1024] ;
    sprintf( env1, "%s", env_s.c_str() ) ;
    putenv( env1 ) ;
    try
    {
	TheBESKeys::TheKeys() ;
	cerr << "created, should have not been created" << endl ;
	return 1 ;
    }
    catch( BESError &e )
    {
	cout << "unable to create BESKeys, good, because:" << endl ;
	cout << e.get_message() << endl ;
    }

    cout << endl << "*****************************************" << endl;
    cout << "bad keys, too many equal signs" << endl;
    env_s = (string)"BES_CONF=" + TEST_SRC_DIR + "/bad_keys2.ini" ;
    char env2[1024] ;
    sprintf( env2, "%s", env_s.c_str() ) ;
    putenv( env2 ) ;
    try
    {
	TheBESKeys::TheKeys() ;
	cerr << "created, should have not been created" << endl ;
	return 1 ;
    }
    catch( BESError &e )
    {
	cout << "unable to create BESKeys, good, because:" << endl ;
	cout << e.get_message() << endl ;
    }

    cout << endl << "*****************************************" << endl;
    cout << "good keys file, should load" << endl;
    env_s = (string)"BES_CONF=" + TEST_SRC_DIR + "/keys_test.ini" ;
    char env3[1024] ;
    sprintf( env3, "%s", env_s.c_str() ) ;
    putenv( env3 ) ;
    try
    {
	TheBESKeys::TheKeys() ;
	cout << "created, good" << endl ;
    }
    catch( BESError &e )
    {
	cerr << "unable to create BESKeys, because:" << endl ;
	cerr << e.get_message() << endl ;
	return 1 ;
    }

    cout << endl << "*****************************************" << endl;
    cout << "get keys" << endl;
    bool found = false ;
    string ret = "" ;
    for( int i = 1; i < 4; i++ )
    {
	char key[32] ;
	sprintf( key, "BES.KEY%d", i ) ;
	char val[32] ;
	sprintf( val, "val%d", i ) ;
	cout << "looking for " << key << endl ;
	ret = "" ;
	ret = TheBESKeys::TheKeys()->get_key( key, found ) ;
	if( found == false )
	{
	    cerr << key << " not found" << endl ;
	    return 1 ;
	}
	if( ret == "" )
	{
	    cerr << key << " set to \"\"" << endl ;
	    return 1 ;
	}
	if( ret != val )
	{
	    cerr << key << " = " << ret << ", but should = " << val << endl ;
	    return 1 ;
	}
	else
	{
	    cout << key << " = " << ret << endl ;
	}
    }

    cout << endl << "*****************************************" << endl;
    cout << "look for non existant key" << endl;
    ret = TheBESKeys::TheKeys()->get_key( "BES.NOTFOUND", found ) ;
    if( found == true )
    {
	cerr << "found BES.NOTFOUND = \"" << ret << "\"" << endl ;
	return 1 ;
    }
    else
    {
	cout << "did not find BES.NOTFOUND" << endl ;
    }

    cout << endl << "*****************************************" << endl;
    cout << "look for key with empty value" << endl;
    ret = TheBESKeys::TheKeys()->get_key( "BES.KEY4", found ) ;
    if( found == true )
    {
	if( ret == "" )
	{
	    cout << "found and is empty" << endl ;
	}
	else
	{
	    cerr << "found BES.NOTFOUND = \"" << ret << "\"" << endl ;
	    return 1 ;
	}
    }
    else
    {
	cerr << "did not find BES.KEY4" << endl ;
	return 1 ;
    }

    cout << endl << "*****************************************" << endl;
    cout << "set bad key, 0 = characters" << endl;
    try
    {
	ret = TheBESKeys::TheKeys()->set_key( "BES.NOEQS" ) ;
	cerr << "set_key successful with value \"" << ret << "\"" << endl ;
	return 1 ;
    }
    catch( BESError &e )
    {
	cout << "unable to set the key, good, because:" << endl ;
	cout << e.get_message() ;
    }

    cout << endl << "*****************************************" << endl;
    cout << "set bad key, 2 = characters" << endl;
    try
    {
	ret = TheBESKeys::TheKeys()->set_key( "BES.2EQS=val1=val2" ) ;
	cerr << "set_key successful with value \"" << ret << "\"" << endl ;
	return 1 ;
    }
    catch( BESError &e )
    {
	cout << "unable to set the key, good, because:" << endl ;
	cout << e.get_message() ;
    }

    cout << endl << "*****************************************" << endl;
    cout << "set BES.KEY5 to val5" << endl;
    try
    {
	ret = TheBESKeys::TheKeys()->set_key( "BES.KEY5=val5" ) ;
	if( ret == "val5" )
	{
	    cout << "set_key successful" << endl ;
	}
	else
	{
	    cerr << "set successfully, but incorrect val = \""
		 << ret << "\"" << endl ;
	    return 1 ;
	}
    }
    catch( BESError &e )
    {
	cerr << "unable to set the key, because:" << endl ;
	cerr << e.get_message() ;
	return 1 ;
    }

    cout << endl << "*****************************************" << endl;
    cout << "set BES.KEY6 to val6" << endl;
    try
    {
	ret = TheBESKeys::TheKeys()->set_key( "BES.KEY6", "val6" ) ;
	if( ret == "val6" )
	{
	    cout << "set_key successful" << endl ;
	}
	else
	{
	    cerr << "set successfully, but incorrect val = \""
		 << ret << "\"" << endl ;
	    return 1 ;
	}
    }
    catch( BESError &e )
    {
	cerr << "unable to set the key, because:" << endl ;
	cerr << e.get_message() ;
	return 1 ;
    }

    cout << endl << "*****************************************" << endl;
    cout << "get keys" << endl;
    for( int i = 1; i < 7; i++ )
    {
	char key[32] ;
	sprintf( key, "BES.KEY%d", i ) ;
	char val[32] ;
	if( i == 4 ) sprintf( val, "" ) ;
	else sprintf( val, "val%d", i ) ;
	cout << "looking for " << key << endl ;
	ret = "" ;
	ret = TheBESKeys::TheKeys()->get_key( key, found ) ;
	if( found == false )
	{
	    cerr << key << " not found" << endl ;
	    return 1 ;
	}
	if( ret != val )
	{
	    cerr << key << " = " << ret << ", but should = " << val << endl ;
	    return 1 ;
	}
	else
	{
	    cout << key << " = " << ret << endl ;
	}
    }

    cout << endl << "*****************************************" << endl;
    cout << "Returning from keysT::run" << endl;

    return retVal;
}

int
main(int argC, char **argV) {
    Application *app = new keysT();
    return app->main(argC, argV);
}

