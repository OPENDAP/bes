// OPENDAP_TYPE_module.cc

#include <iostream>

using std::endl ;

#include "DODSInitList.h"
#include "DODSRequestHandlerList.h"
#include "OPENDAP_CLASSRequestHandler.h"
#include "DODSLog.h"
#include "DODSResponseHandlerList.h"
#include "OPENDAP_CLASSResponseNames.h"

static bool
OPENDAP_CLASSInit(int, char**)
{
    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "Initializing OPENDAP_CLASS Handler:" << endl ;

    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "    adding " << OPENDAP_CLASS_NAME << " request handler" << endl ;
    DODSRequestHandlerList::TheList()->add_handler( OPENDAP_CLASS_NAME, new OPENDAP_CLASSRequestHandler( OPENDAP_CLASS_NAME ) ) ;

    return true ;
}

static bool
OPENDAP_CLASSTerm(void)
{
    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "Removing OPENDAP_CLASS Handlers" << endl;
    DODSRequestHandler *rh = DODSRequestHandlerList::TheList()->remove_handler( OPENDAP_CLASS_NAME ) ;
    if( rh ) delete rh ;
    return true ;
}

FUNINITQUIT( OPENDAP_CLASSInit, OPENDAP_CLASSTerm, 3 ) ;

