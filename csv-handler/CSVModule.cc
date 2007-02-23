// CSVModule.cc

#include <iostream>

using std::endl ;

#include "CSVModule.h"
#include "BESRequestHandlerList.h"
#include "CSVRequestHandler.h"
#include "BESLog.h"
#include "BESResponseHandlerList.h"
#include "BESResponseNames.h"
#include "CSVResponseNames.h"

#include "BESContainerStorageList.h"
#include "BESContainerStorageCatalog.h"
#include "BESCatalogDirectory.h"
#include "BESCatalogList.h"

#include "BESDebug.h"

void
CSVModule::initialize( const string &modname )
{
    BESDEBUG( "Initializing CSV Handler:" << endl )

    BESDEBUG( "    adding " << modname << " request handler" << endl )
    BESRequestHandlerList::TheList()->
	add_handler( modname, new CSVRequestHandler( modname ) ) ;

    BESDEBUG( "    adding " << CSV_CATALOG << " catalog" << endl )
    BESCatalogList::TheCatalogList()->
        add_catalog(new BESCatalogDirectory( CSV_CATALOG ) ) ;

    BESDEBUG( "Adding Catalog Container Storage" << endl )
    BESContainerStorageList::TheList()->
	add_persistence( new BESContainerStorageCatalog( CSV_CATALOG ) ) ;

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

