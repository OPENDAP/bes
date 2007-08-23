// encodeT.C

#include <iostream>
#include <fstream>

using std::cerr ;
using std::cout ;
using std::endl ;
using std::ifstream ;

#include "encodeT.h"
#include "BESProcessEncodedString.h"
#include "test_config.h"

int
encodeT::run(void)
{
    cout << endl << "*****************************************" << endl;
    cout << "Entered encodeT::run" << endl;
    int retVal = 0;

    string teststr = "request=%22This%20is%20a%20test%3B%22&username=pwest" ;
    BESProcessEncodedString pes( teststr.c_str() ) ;
    string request = pes.get_key( "request" ) ;
    cout << "request = " << request << endl ;
    if( request != "\"This is a test;\"" )
    {
	cerr << "Resulting request incorrect" << endl ;
	return 1 ;
    }
    else
    {
	cout << "Resulting request correct" << endl ;
    }
    string username = pes.get_key( "username" ) ;
    cout << "username = " << username << endl ;
    if( username != "pwest" )
    {
	cerr << "Resulting username incorrect" << endl ;
	return 1 ;
    }
    else
    {
	cout << "Resulting username correct" << endl ;
    }

    cout << endl << "*****************************************" << endl;
    cout << "Returning from encodeT::run" << endl;

    return retVal;
}

int
main(int argC, char **argV) {
    string env_var = (string)"BES_CONF=" + TEST_SRC_DIR + "/bes.conf" ;
    putenv( (char *)env_var.c_str() ) ;
    Application *app = new encodeT();
    return app->main(argC, argV);
}

