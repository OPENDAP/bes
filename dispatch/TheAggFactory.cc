// TheResponseHandlerList.cc

// 2004 Copyright University Corporation for Atmospheric Research

#include "TheAggFactory.h"
#include "DODSInitList.h"
#include "DODSInitOrder.h"

OPeNDAPAggFactory *TheAggFactory = 0;

static bool
buildAggFactory(int, char**) {
    TheAggFactory = new OPeNDAPAggFactory ;
    return true ;
}

static bool
destroyAggFactory(void) {
    if( TheAggFactory )
    {
	delete (OPeNDAPAggFactory *)TheAggFactory ;
    }
    TheAggFactory = 0 ;
    return true;
}

FUNINITQUIT(buildAggFactory, destroyAggFactory, AGGFACTORY_INIT);

// $Log: TheResponseHandlerList.cc,v $
// Revision 1.2  2004/09/09 17:17:12  pwest
// Added copywrite information
//
// Revision 1.1  2004/06/30 20:16:24  pwest
// dods dispatch code, can be used for apache modules or simple cgi script
// invocation or opendap daemon. Built during cedar server development.
//
