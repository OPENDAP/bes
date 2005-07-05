// TheDog.C

#include <iostream>

using std::cerr ;
using std::endl ;

#include "TheDog.h"
#include "dog.h"
#include "DODSInitList.h"

Animal *TheDog = 0;

static bool
buildNewDog(int, char**) {
    cerr << "I am building new dog, using FUNINIT with no termination" << endl;
    TheDog = new dog("Killer");
    return true;
}

static bool
destroyNewDog(void) {
    cerr << "I am destroying the dog" << endl;
    if(TheDog) delete TheDog;
    return true;
}

FUNINIT(buildNewDog, 1);

