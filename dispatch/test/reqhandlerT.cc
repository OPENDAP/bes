// reqhandlerT.C

#include <iostream>

using std::cerr ;
using std::cout ;
using std::endl ;

#include "reqhandlerT.h"
#include "TestRequestHandler.h"

int reqhandlerT::
run(void) {
    cout << endl << "*****************************************" << endl;
    cout << "Entered reqhandlerT::run" << endl;

    TestRequestHandler trh( "test" ) ;
    int retVal = trh.test() ;

    cout << endl << "*****************************************" << endl;
    cout << "Returning from reqhandlerT::run" << endl;

    return retVal;
}

int
main(int argC, char **argV) {
    Application *app = new reqhandlerT();
    return app->main(argC, argV);
}

