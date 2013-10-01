// HDF4Module.cc

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2004,2005 University Corporation for Atmospheric Research
// Author: Patrick West <pwest@ucar.org>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
// 
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//
// You can contact University Corporation for Atmospheric Research at
// 3080 Center Green Drive, Boulder, CO 80301

// (c) COPYRIGHT University Corporation for Atmospheric Research 2004-2005
// Please read the full copyright statement in the file COPYRIGHT_UCAR.
//
// Authors:
//      pwest       Patrick West <pwest@ucar.edu>

#include <iostream>

using std::endl;

#include "HDF4Module.h"
#include <BESRequestHandlerList.h>
#include "HDF4RequestHandler.h"
#include <BESDapService.h>
#include <BESContainerStorageList.h>
#include <BESContainerStorageCatalog.h>
#include <BESCatalogDirectory.h>
#include <BESCatalogList.h>
#include <BESDebug.h>

#define HDF4_CATALOG "catalog"

void
 HDF4Module::initialize(const string & modname)
{
    BESDEBUG("h4", "Initializing HDF4 module " << modname << endl) ;

        BESDEBUG("h4",
                 "    adding " << modname << " request handler" << endl) ;
    BESRequestHandler *handler = new HDF4RequestHandler(modname);
    BESRequestHandlerList::TheList()->add_handler(modname, handler);

    BESDEBUG( "h4", modname << " handles dap services" << endl ) ;
    BESDapService::handle_dap_service( modname ) ;

    BESDEBUG("h4", "    adding " << HDF4_CATALOG << " catalog" << endl) ;
    if( !BESCatalogList::TheCatalogList()->ref_catalog( HDF4_CATALOG ) )
    {
	BESCatalogList::TheCatalogList()->
	    add_catalog(new BESCatalogDirectory(HDF4_CATALOG));
    }
    else
    {
	BESDEBUG( "h4", "    catalog already exists, skipping" << endl ) ;
    }

    BESDEBUG("h4",
             "    adding catalog container storage" << HDF4_CATALOG <<
             endl) ;
    if( !BESContainerStorageList::TheList()->ref_persistence( HDF4_CATALOG ) )
    {
	BESContainerStorageCatalog *csc =
	    new BESContainerStorageCatalog(HDF4_CATALOG);
	BESContainerStorageList::TheList()->add_persistence(csc);
    }
    else
    {
	BESDEBUG( "h4", "    storage already exists, skipping" << endl ) ;
    }

    BESDEBUG("h4", "    adding h4 debug context" << endl) ;
        BESDebug::Register("h4");

    BESDEBUG("h4", "Done Initializing HDF4 module " << modname << endl) ;
}

void HDF4Module::terminate(const string & modname)
{
    BESDEBUG("h4", "Cleaning HDF4 module " << modname << endl) ;

    BESDEBUG("h4", "    removing HDF4 Handler" << modname << endl) ;
    BESRequestHandler *rh =
        BESRequestHandlerList::TheList()->remove_handler(modname);
    if (rh)
        delete rh;

    BESDEBUG("h4", "    removing catalog container storage" << HDF4_CATALOG
                   << endl) ;
    BESContainerStorageList::TheList()->deref_persistence(HDF4_CATALOG);

    BESDEBUG("h4", "    removing " << HDF4_CATALOG << " catalog" << endl) ;
        BESCatalogList::TheCatalogList()->deref_catalog(HDF4_CATALOG);

    BESDEBUG("h4", "Done Cleaning HDF4 module " << modname << endl) ;
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance
 *
 * @param strm C++ i/o stream to dump the information to
 */
void HDF4Module::dump(ostream & strm) const
{
    strm << BESIndent::LMarg << "HDF4Module::dump - ("
        << (void *) this << ")" << endl;
}

extern "C" {
    BESAbstractModule *maker() {
        return new HDF4Module;
}}
