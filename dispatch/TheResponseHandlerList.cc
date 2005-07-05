// TheResponseHandlerList.cc

// 2004 Copyright University Corporation for Atmospheric Research

#include "TheResponseHandlerList.h"
#include "DODSInitList.h"
#include "DODSInitOrder.h"

DODSResponseHandlerList *TheResponseHandlerList = 0;

static bool
buildResponseHandlerList(int, char**) {
    TheResponseHandlerList = new DODSResponseHandlerList ;
    return true ;
}

static bool
destroyResponseHandlerList(void) {
    if(TheResponseHandlerList)
    {
	delete (DODSResponseHandlerList *)TheResponseHandlerList;
    }
    TheResponseHandlerList = 0 ;
    return true;
}

FUNINITQUIT(buildResponseHandlerList, destroyResponseHandlerList, RESPONSEHANDLERLIST_INIT);

// $Log: TheResponseHandlerList.cc,v $
// Revision 1.2  2004/09/09 17:17:12  pwest
// Added copywrite information
//
// Revision 1.1  2004/06/30 20:16:24  pwest
// dods dispatch code, can be used for apache modules or simple cgi script
// invocation or opendap daemon. Built during cedar server development.
//
