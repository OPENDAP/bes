// TheCat.C

#include <iostream>

using std::cerr ;
using std::endl ;

#include "TheCat.h"
#include "cat.h"
#include "DODSInitList.h"

Animal *TheCat = 0;

static bool
buildNewCat(int, char**) {
    cerr << "I am building new cat" << endl;
    TheCat = new cat("Muffy");
    return true;
}

static bool
destroyNewCat(void) {
    cerr << "I am destroying the cat" << endl;
    if(TheCat) delete TheCat;
    return true;
}

FUNINITQUIT(buildNewCat, destroyNewCat, 1);

