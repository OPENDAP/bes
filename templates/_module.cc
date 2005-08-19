// OPENDAP_TYPE_module.cc

#include <iostream>

using std::endl ;

#include "DODSInitList.h"
#include "TheRequestHandlerList.h"
#include "OPENDAP_CLASSRequestHandler.h"
#include "TheDODSLog.h"
#include "TheResponseHandlerList.h"
#include "OPENDAP_CLASSResponseNames.h"

static bool
OPENDAP_CLASSInit(int, char**)
{
    if( TheDODSLog->is_verbose() )
	(*TheDODSLog) << "Initializing OPENDAP_CLASS Handler:" << endl ;

    if( TheDODSLog->is_verbose() )
	(*TheDODSLog) << "    adding " << OPENDAP_CLASS_NAME << " request handler" << endl ;
    TheRequestHandlerList->add_handler( OPENDAP_CLASS_NAME, new OPENDAP_CLASSRequestHandler( OPENDAP_CLASS_NAME ) ) ;

    return true ;
}

static bool
OPENDAP_CLASSTerm(void)
{
    if( TheDODSLog->is_verbose() )
	(*TheDODSLog) << "Removing OPENDAP_CLASS Handlers" << endl;
    DODSRequestHandler *rh = TheRequestHandlerList->remove_handler( OPENDAP_CLASS_NAME ) ;
    if( rh ) delete rh ;
    return true ;
}

FUNINITQUIT( OPENDAP_CLASSInit, OPENDAP_CLASSTerm, 3 ) ;

