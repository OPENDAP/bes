// TheReporterList.cc

// 2004 Copyright University Corporation for Atmospheric Research

#include "TheReporterList.h"
#include "DODSInitList.h"
#include "DODSInitOrder.h"

DODSReporterList *TheReporterList = 0;

static bool
buildReporterList(int, char**) {
    TheReporterList = new DODSReporterList ;
    return true ;
}

static bool
destroyReporterList(void) {
    if( TheReporterList )
    {
	delete (DODSReporterList *)TheReporterList ;
    }
    TheReporterList = 0 ;
    return true;
}

FUNINITQUIT(buildReporterList, destroyReporterList, REPORTERLIST_INIT);

// $Log: TheReporterList.cc,v $
// Revision 1.2  2004/09/09 17:17:12  pwest
// Added copywrite information
//
// Revision 1.1  2004/06/30 20:16:24  pwest
// dods dispatch code, can be used for apache modules or simple cgi script
// invocation or opendap daemon. Built during cedar server development.
//
