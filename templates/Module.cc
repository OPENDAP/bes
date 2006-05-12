// OPENDAP_CLASSModule.cc

#include <iostream>

using std::endl ;

#include "OPENDAP_CLASSModule.h"
#include "DODSRequestHandlerList.h"
#include "OPENDAP_CLASSRequestHandler.h"
#include "DODSLog.h"
#include "DODSResponseHandlerList.h"
#include "DODSResponseNames.h"
#include "OPeNDAPCommand.h"
#include "OPENDAP_CLASSResponseNames.h"

void
OPENDAP_CLASSModule::initialize()
{
    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "Initializing OPENDAP_CLASS Handler:" << endl ;

    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "    adding " << OPENDAP_CLASS_NAME << " request handler" << endl ;
    DODSRequestHandlerList::TheList()->add_handler( OPENDAP_CLASS_NAME, new OPENDAP_CLASSRequestHandler( OPENDAP_CLASS_NAME ) ) ;

    // If new commands are needed, then let's declare this once here. If
    // not, then you can remove this line.
    string cmd_name ;

    // INIT_END
}

void
OPENDAP_CLASSModule::terminate()
{
    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "Removing OPENDAP_CLASS Handlers" << endl;
    DODSRequestHandler *rh = DODSRequestHandlerList::TheList()->remove_handler( OPENDAP_CLASS_NAME ) ;
    if( rh ) delete rh ;
}

extern "C"
{
    OPeNDAPAbstractModule *maker()
    {
	return new OPENDAP_CLASSModule ;
    }
}

