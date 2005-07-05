// TheDODSLog.cc

// 2004 Copyright University Corporation for Atmospheric Research

#include "TheDODSLog.h"
#include "DODSInitList.h"
#include "DODSInitOrder.h"

DODSLog *TheDODSLog = 0;

static bool
buildDODSLog(int, char**) {
    TheDODSLog = new DODSLog ;
    return true ;
}

static bool
destroyDODSLog(void) {
    if( TheDODSLog )
    {
	delete (DODSLog *)TheDODSLog ;
    }
    TheDODSLog = 0 ;
    return true ;
}

FUNINITQUIT( buildDODSLog, destroyDODSLog, DODSLOG_INIT ) ;

// $Log: TheDODSLog.cc,v $
// Revision 1.2  2004/09/09 17:17:12  pwest
// Added copywrite information
//
// Revision 1.1  2004/06/30 20:16:24  pwest
// dods dispatch code, can be used for apache modules or simple cgi script
// invocation or opendap daemon. Built during cedar server development.
//
