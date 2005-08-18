// TheDODSKeys.cc

// 2004 Copyright University Corporation for Atmospheric Research

#include "TheDODSKeys.h"
#include "DODSKeysException.h"
#include "DODSInitList.h"
#include "DODSInitOrder.h"

#define DODS_INI_FILE getenv("DODS_INI")

DODSKeys *TheDODSKeys = 0;

static bool
buildDODSKeys(int, char**) {
    char *dods_ini = DODS_INI_FILE ;
    if( !dods_ini )
    {
	throw DODSKeysException( "Can not load environment variable DODS_INI" ) ;
    }
    TheDODSKeys = new DODSKeys( dods_ini ) ;
    return true ;
}

static bool
destroyDODSKeys(void) {
    if( TheDODSKeys )
    {
	delete (DODSKeys *)TheDODSKeys ;
    }
    TheDODSKeys = 0 ;
    return true ;
}

FUNINITQUIT( buildDODSKeys, destroyDODSKeys, DODSKEYS_INIT ) ;

// $Log: TheDODSKeys.cc,v $
// Revision 1.2  2004/09/09 17:17:12  pwest
// Added copywrite information
//
// Revision 1.1  2004/06/30 20:16:24  pwest
// dods dispatch code, can be used for apache modules or simple cgi script
// invocation or opendap daemon. Built during cedar server development.
//
