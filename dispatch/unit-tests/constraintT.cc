// constraintT.cc

#include <iostream>

using std::cerr ;
using std::cout ;
using std::endl ;

#include "constraintT.h"
#include "BESContainer.h"
#include "BESDataHandlerInterface.h"
#include "BESConstraintFuncs.h"
#include "BESException.h"
#include "BESDataNames.h"
#include "test_config.h"

int constraintT::
run(void)
{
    cout << endl << "*****************************************" << endl;
    cout << "Entered constraintT::run" << endl;
    int retVal = 0;

    cout << endl << "*****************************************" << endl;
    cout << "Build the data and build the post constraint" << endl ;
    BESDataHandlerInterface dhi ;
    BESContainer d1( "sym1" ) ;
    d1.set_constraint( "var1" ) ;
    dhi.containers.push_back( d1 ) ;

    BESContainer d2( "sym2" ) ;
    d2.set_constraint( "var2" ) ;
    dhi.containers.push_back( d2 ) ;

    dhi.first_container() ;
    BESConstraintFuncs::post_append( dhi ) ;
    dhi.next_container() ;
    BESConstraintFuncs::post_append( dhi ) ;

    string should_be = "sym1.var1,sym2.var2" ;
    if( dhi.data[POST_CONSTRAINT] != should_be )
    {
	cerr << "bad things man" << endl ;
	cerr << "    post constraint: " << dhi.data[POST_CONSTRAINT] << endl;
	cerr << "    should be: " << should_be << endl;
    }
    else
    {
	cout << "good" << endl ;
	cout << "    post constraint: " << dhi.data[POST_CONSTRAINT] << endl;
	cout << "    should be: " << should_be << endl;
    }

    cout << endl << "*****************************************" << endl;
    cout << "Returning from constraintT::run" << endl;

    return retVal;
}

int
main(int argC, char **argV) {
    string env_var = (string)"BES_CONF=" + TEST_SRC_DIR
                     + "/persistence_cgi_test.ini" ;
    putenv( (char *)env_var.c_str() ) ;
    Application *app = new constraintT();
    return app->main(argC, argV);
}

