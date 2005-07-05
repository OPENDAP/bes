// TheDODSReturnManager.cc

// 2004 Copyright University Corporation for Atmospheric Research

#include "TheDODSReturnManager.h"
#include "DODSInitList.h"
#include "DODSInitOrder.h"

DODSReturnManager *TheDODSReturnManager = 0;

static bool
buildDODSReturnManager(int, char**) {
    TheDODSReturnManager = new DODSReturnManager ;
    return true ;
}

static bool
destroyDODSReturnManager(void) {
    if(TheDODSReturnManager)
    {
	delete (DODSReturnManager *)TheDODSReturnManager;
    }
    TheDODSReturnManager = 0 ;
    return true;
}

FUNINITQUIT(buildDODSReturnManager, destroyDODSReturnManager, DODSRETURNMANAGER_INIT);

// $Log: TheDODSReturnManager.cc,v $
// Revision 1.1  2005/02/01 17:48:17  pwest
//
// integration of ESG into opendap
//
