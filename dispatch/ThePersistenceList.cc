// ThePersistenceList.cc

// 2004 Copyright University Corporation for Atmospheric Research

#include "ThePersistenceList.h"
#include "DODSInitList.h"
#include "DODSInitOrder.h"
#include "DODSContainerPersistenceVolatile.h"

DODSContainerPersistenceList *ThePersistenceList = 0;

static bool
buildPersistenceList(int, char**) {
    ThePersistenceList = new DODSContainerPersistenceList ;
    ThePersistenceList->add_persistence( new DODSContainerPersistenceVolatile( PERSISTENCE_VOLATILE ) ) ;
    return true ;
}

static bool
destroyPersistenceList(void) {
    if(ThePersistenceList)
    {
	delete (DODSContainerPersistenceList *)ThePersistenceList;
    }
    ThePersistenceList = 0 ;
    return true;
}

FUNINITQUIT(buildPersistenceList, destroyPersistenceList, PERSISTENCELIST_INIT);

// $Log: ThePersistenceList.cc,v $
// Revision 1.4  2005/03/15 20:01:01  pwest
// using VOLATILE macro instead of hard codes string
//
// Revision 1.3  2005/02/01 17:48:17  pwest
//
// integration of ESG into opendap
//
// Revision 1.2  2004/09/09 17:17:12  pwest
// Added copywrite information
//
// Revision 1.1  2004/06/30 20:16:24  pwest
// dods dispatch code, can be used for apache modules or simple cgi script
// invocation or opendap daemon. Built during cedar server development.
//
