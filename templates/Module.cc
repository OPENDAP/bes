// OPENDAP_CLASSModule.cc

#include <iostream>

using std::endl ;

#include "OPENDAP_CLASSModule.h"
#include "BESRequestHandlerList.h"
#include "OPENDAP_CLASSRequestHandler.h"
#include "BESDebug.h"
#include "BESResponseHandlerList.h"
#include "BESResponseNames.h"
#include "BESCommand.h"
#include "OPENDAP_CLASSResponseNames.h"

void
OPENDAP_CLASSModule::initialize( const string &modname )
{
    BESDEBUG( modname, "Initializing OPENDAP_CLASS Module " << modname << endl )

    BESDEBUG( modname, "    adding " << modname << " request handler" << endl )
    BESRequestHandlerList::TheList()->add_handler( modname, new OPENDAP_CLASSRequestHandler( modname ) ) ;

    // If new commands are needed, then let's declare this once here. If
    // not, then you can remove this line.
    string cmd_name ;

    // INIT_END
    BESDEBUG( modname, "    adding OPENDAP_CLASS debug context" << endl )
    BESDebug::Register( modname ) ;

    BESDEBUG( modname, "Done Initializing OPENDAP_CLASS Module " << modname << endl )
}

void
OPENDAP_CLASSModule::terminate( const string &modname )
{
    BESDEBUG( modname, "Cleaning OPENDAP_CLASS module " << modname << endl )

    BESDEBUG( modname, "    removing " << modname << " request handler" << endl )
    BESRequestHandler *rh = BESRequestHandlerList::TheList()->remove_handler( modname ) ;
    if( rh ) delete rh ;

    // If new commands are needed, then let's declare this once here. If
    // not, then you can remove this line.
    string cmd_name ;

    // TERM_END
    BESDEBUG( modname, "Done Cleaning OPENDAP_CLASS module " << modname << endl )
}

extern "C"
{
    BESAbstractModule *maker()
    {
	return new OPENDAP_CLASSModule ;
    }
}

void
OPENDAP_CLASSModule::dump( ostream &strm ) const
{
    strm << BESIndent::LMarg << "OPENDAP_CLASSModule::dump - ("
			     << (void *)this << ")" << endl ;
}

