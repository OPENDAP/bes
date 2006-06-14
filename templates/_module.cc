// OPENDAP_TYPE_module.cc

#include <iostream>

using std::endl ;

#include "BESInitList.h"
#include "BESRequestHandlerList.h"
#include "OPENDAP_CLASSRequestHandler.h"
#include "BESLog.h"
#include "BESResponseHandlerList.h"
#include "OPENDAP_CLASSResponseNames.h"

static bool
OPENDAP_CLASSInit(int, char**)
{
    if( BESLog::TheLog()->is_verbose() )
	(*BESLog::TheLog()) << "Initializing OPENDAP_CLASS Handler:" << endl ;

    if( BESLog::TheLog()->is_verbose() )
	(*BESLog::TheLog()) << "    adding " << OPENDAP_CLASS_NAME << " request handler" << endl ;
    BESRequestHandlerList::TheList()->add_handler( OPENDAP_CLASS_NAME, new OPENDAP_CLASSRequestHandler( OPENDAP_CLASS_NAME ) ) ;

    return true ;
}

static bool
OPENDAP_CLASSTerm(void)
{
    if( BESLog::TheLog()->is_verbose() )
	(*BESLog::TheLog()) << "Removing OPENDAP_CLASS Handlers" << endl;
    BESRequestHandler *rh = BESRequestHandlerList::TheList()->remove_handler( OPENDAP_CLASS_NAME ) ;
    if( rh ) delete rh ;
    return true ;
}

FUNINITQUIT( OPENDAP_CLASSInit, OPENDAP_CLASSTerm, 3 ) ;

