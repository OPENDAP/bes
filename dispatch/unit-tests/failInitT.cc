// failInitT.C

#include <iostream>

using std::cerr ;
using std::cout ;
using std::endl ;

#include "failInitT.h"
#include "BESError.h"

int failInitT::
run(void) {
    cout << endl << "*****************************************" << endl;
    cout << "Entered failInitT::run" << endl;
    int retVal = 0;

    /* Your code here */

    cout << endl << "*****************************************" << endl;
    cout << "Returning from failInitT::run" << endl;
    return retVal;
}

int
main(int argC, char **argV) {
    Application *app = new failInitT();
    int ret = 0 ;
    try
    {
	ret = app->main(argC, argV);
    }
    catch( BESError &e )
    {
	cout << "initialization failed" << endl ;
    }
    return ret ;
}

