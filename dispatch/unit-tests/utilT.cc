// utilT.C

#include <iostream>
#include <fstream>
#include <cstdlib>

using std::cerr ;
using std::cout ;
using std::endl ;
using std::ifstream ;

#include "utilT.h"
#include "BESUtil.h"
#include "BESError.h"
#include "test_config.h"

int
utilT::run(void)
{
    cout << endl << "*****************************************" << endl;
    cout << "Entered utilT::run" << endl;
    int retVal = 0;

    cout << endl << "*****************************************" << endl;
    cout << "Remove escaped quotes" << endl;
    string s = BESUtil::unescape( "\\\"This is a test, this is \\\"ONLY\\\" a test\\\"" ) ;
    string result = "\"This is a test, this is \"ONLY\" a test\"" ;
    if( s != result )
    {
	cerr << "resulting string incorrect: " << s << " should be " << result << endl ;
	return 1 ;
    }

    cout << endl << "*****************************************" << endl;
    cout << "Returning from utilT::run" << endl;

    return retVal;
}

int
main(int argC, char **argV) {
    string env_var = (string)"BES_CONF=" + TEST_SRC_DIR + "/bes.conf" ;
    putenv( (char *)env_var.c_str() ) ;
    Application *app = new utilT();
    return app->main(argC, argV);
}

