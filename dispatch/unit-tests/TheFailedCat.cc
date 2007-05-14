// TheFailedCat.C

#include <iostream>

using std::cerr ;
using std::endl ;

#include "TheFailedCat.h"
#include "BESInitList.h"

Animal *TheFailedCat = 0;

static bool
buildNewCat(int, char**) {
    cerr << "I am building new failed cat" << endl;
    return false;
}

static bool
destroyNewCat(void) {
    cerr << "I am destroying the failed cat" << endl;
    return false;
}

FUNINITQUIT(buildNewCat, destroyNewCat, 1);

