// DapModule.cc

#include <iostream>

using std::endl ;

#include "DapModule.h"
#include "DapRequestHandler.h"
#include "DapResponseNames.h"

#include <BESRequestHandlerList.h>
#include <BESDebug.h>
#include <BESResponseHandlerList.h>
#include <BESResponseNames.h>
#include <BESContainerStorageList.h>
#include <BESContainerStorageCatalog.h>
#include <BESCatalogDirectory.h>
#include <BESCatalogList.h>

#define DAP_CATALOG "catalog"

void
DapModule::initialize( const string &modname )
{
    BESDEBUG( modname, "Initializing Dap Module "
		       << modname << endl ) ;

    BESDEBUG( modname, "    adding dap request handler" << endl ) ;
    BESRequestHandlerList::TheList()->add_handler( modname, new DapRequestHandler( modname ) ) ;

    BESDEBUG( "nc", "    adding " << DAP_CATALOG << " catalog" << endl ) ;
    if( !BESCatalogList::TheCatalogList()->ref_catalog( DAP_CATALOG ) )
    {
	BESCatalogList::TheCatalogList()->
	    add_catalog( new BESCatalogDirectory( DAP_CATALOG ) ) ;
    }
    else
    {
	BESDEBUG( "nc", "    catalog already exists, skipping" << endl ) ;
    }

    BESDEBUG( "nc", "    adding catalog container storage " << DAP_CATALOG
		    << endl ) ;
    if( !BESContainerStorageList::TheList()->ref_persistence( DAP_CATALOG ) )
    {
	BESContainerStorageCatalog *csc =
	    new BESContainerStorageCatalog( DAP_CATALOG ) ;
	BESContainerStorageList::TheList()->add_persistence( csc ) ;
    }
    else
    {
	BESDEBUG( "nc", "    storage already exists, skipping" << endl ) ;
    }

    BESDEBUG( modname, "    adding Dap debug context" << endl ) ;
    BESDebug::Register( modname ) ;

    BESDEBUG( modname, "Done Initializing Dap Module "
		       << modname << endl ) ;
}

void
DapModule::terminate( const string &modname )
{
    BESDEBUG( modname, "Cleaning Dap module " << modname << endl ) ;

    BESRequestHandler *rh = 0 ;

    BESDEBUG( modname, "    removing dap request handler" << endl ) ;
    rh = BESRequestHandlerList::TheList()->remove_handler( modname ) ;
    if( rh ) delete rh ; rh = 0 ;

    BESDEBUG( "nc", "    removing catalog container storage"
                    << DAP_CATALOG << endl ) ;
    BESContainerStorageList::TheList()->deref_persistence( DAP_CATALOG ) ;

    BESDEBUG( "nc", "    removing " << DAP_CATALOG << " catalog" << endl ) ;
    BESCatalogList::TheCatalogList()->deref_catalog( DAP_CATALOG ) ;

    BESDEBUG( modname, "Done Cleaning Dap module "
		       << modname << endl ) ;
}

extern "C"
{
    BESAbstractModule *maker()
    {
	return new DapModule ;
    }
}

void
DapModule::dump( ostream &strm ) const
{
    strm << BESIndent::LMarg << "DapModule::dump - ("
			     << (void *)this << ")" << endl ;
}

