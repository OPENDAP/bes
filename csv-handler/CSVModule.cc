// CSVModule.cc

#include <iostream>

using std::endl ;

#include "CSVModule.h"
#include "BESRequestHandlerList.h"
#include "CSVRequestHandler.h"
#include "BESLog.h"
#include "BESResponseHandlerList.h"
#include "BESResponseNames.h"
#include "BESCommand.h"
#include "CSVResponseNames.h"

void
CSVModule::initialize( const string &modname )
{
    if( BESLog::TheLog()->is_verbose() )
	(*BESLog::TheLog()) << "Initializing CSV Handler:" << endl ;

    if( BESLog::TheLog()->is_verbose() )
	(*BESLog::TheLog()) << "    adding " << modname << " request handler" << endl ;
    BESRequestHandlerList::TheList()->add_handler( modname, new CSVRequestHandler( modname ) ) ;

    // If new commands are needed, then let's declare this once here. If
    // not, then you can remove this line.
    string cmd_name ;

    // INIT_END
}

void
CSVModule::terminate( const string &modname )
{
    if( BESLog::TheLog()->is_verbose() )
	(*BESLog::TheLog()) << "Removing CSV Handlers" << endl;
    BESRequestHandler *rh = BESRequestHandlerList::TheList()->remove_handler( modname ) ;
    if( rh ) delete rh ;
}

extern "C"
{
    BESAbstractModule *maker()
    {
	return new CSVModule ;
    }
}

void
CSVModule::dump( ostream &strm ) const
{
    strm << BESIndent::LMarg << "CSVModule::dump - ("
			     << (void *)this << ")" << endl ;
}

