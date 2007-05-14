// initT.C

#include <iostream>

using std::cerr ;
using std::cout ;
using std::endl ;

#include "initT.h"
#include "TheCat.h"
#include "TheDog.h"

int initT::
run(void) {
    cout << endl << "*****************************************" << endl;
    cout << "Entered initT::run" << endl;
    int retVal = 0;

    cout << endl << "*****************************************" << endl;
    cout << "Using TheCat and TheDog" << endl;
    if(!TheCat)
    {
	cerr << "TheCat was not created" << endl;
	return 1;
    }

    if(!TheDog)
    {
	cerr << "TheDog was not created" << endl;
	return 1;
    }

    cout << endl << "*************************************" << endl;
    string name = TheCat->get_name() ;
    if( name == "Muffy" )
    {
	cout << "correct cat" << endl ;
    }
    else
    {
	cerr << "incorrect cat" << endl ;
	retVal = 1 ;
    }

    cout << endl << "*************************************" << endl;
    name = TheDog->get_name() ;
    if( name == "Killer" )
    {
	cout << "correct dog" << endl ;
    }
    else
    {
	cerr << "incorrect dog" << endl ;
	retVal = 1 ;
    }

    cout << endl << "*****************************************" << endl;
    cout << "Returning from initT::run" << endl;

    return retVal;
}

int
main(int argC, char **argV) {
    Application *app = new initT();
    return app->main(argC, argV);
}

