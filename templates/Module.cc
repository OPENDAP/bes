// OPENDAP_CLASSModule.cc

#include <iostream>

using std::endl ;

#include "OPENDAP_CLASSModule.h"
#include "BESRequestHandlerList.h"
#include "OPENDAP_CLASSRequestHandler.h"
#include "BESLog.h"
#include "BESResponseHandlerList.h"
#include "BESResponseNames.h"
#include "BESCommand.h"
#include "OPENDAP_CLASSResponseNames.h"

void
OPENDAP_CLASSModule::initialize( const string &modname )
{
    if( BESLog::TheLog()->is_verbose() )
	(*BESLog::TheLog()) << "Initializing OPENDAP_CLASS Handler:" << endl ;

    if( BESLog::TheLog()->is_verbose() )
	(*BESLog::TheLog()) << "    adding " << modname << " request handler" << endl ;
    BESRequestHandlerList::TheList()->add_handler( modname, new OPENDAP_CLASSRequestHandler( modname ) ) ;

    // If new commands are needed, then let's declare this once here. If
    // not, then you can remove this line.
    string cmd_name ;

    // INIT_END
}

void
OPENDAP_CLASSModule::terminate( const string &modname )
{
    if( BESLog::TheLog()->is_verbose() )
	(*BESLog::TheLog()) << "Removing OPENDAP_CLASS Handlers" << endl;
    BESRequestHandler *rh = BESRequestHandlerList::TheList()->remove_handler( modname ) ;
    if( rh ) delete rh ;
}

extern "C"
{
    BESAbstractModule *maker()
    {
	return new OPENDAP_CLASSModule ;
    }
}

