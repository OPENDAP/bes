// TheRequestHandlerList.cc

// 2004 Copyright University Corporation for Atmospheric Research

#include "TheRequestHandlerList.h"
#include "DODSInitList.h"
#include "DODSInitOrder.h"

DODSRequestHandlerList *TheRequestHandlerList = 0;

static bool
buildRequestHandlerList(int, char**) {
    TheRequestHandlerList = new DODSRequestHandlerList ;
    return true ;
}

static bool
destroyRequestHandlerList(void) {
    if(TheRequestHandlerList)
    {
	delete (DODSRequestHandlerList *)TheRequestHandlerList;
    }
    TheRequestHandlerList = 0 ;
    return true;
}

FUNINITQUIT(buildRequestHandlerList, destroyRequestHandlerList, REQUESTHANDLERLIST_INIT);

// $Log: TheRequestHandlerList.cc,v $
// Revision 1.2  2004/09/09 17:17:12  pwest
// Added copywrite information
//
// Revision 1.1  2004/06/30 20:16:24  pwest
// dods dispatch code, can be used for apache modules or simple cgi script
// invocation or opendap daemon. Built during cedar server development.
//
