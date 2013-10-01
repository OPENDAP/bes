// HDF5Module.cc

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
/// \file HDF5Module.cc
/// \brief The starting and ending fuctions for the HDF5 OPeNDAP handler via BES
///
/// \author  Patrick West <pwest@ucar.edu>


#include <iostream>

using std::endl;

#include "HDF5Module.h"
#include <BESRequestHandlerList.h>
#include "HDF5RequestHandler.h"
#include <BESDapService.h>
#include <BESContainerStorageList.h>
#include <BESContainerStorageCatalog.h>
#include <BESCatalogDirectory.h>
#include <BESCatalogList.h>
#include <BESDebug.h>

#define HDF5_CATALOG "catalog"

void
 HDF5Module::initialize(const string & modname)
{
    BESDEBUG("h5", "Initializing HDF5 module " << modname << endl) ;

        BESDEBUG("h5",
                 "    adding " << modname << " request handler" << endl) ;
    BESRequestHandler *handler = new HDF5RequestHandler(modname);
    BESRequestHandlerList::TheList()->add_handler(modname, handler);

    BESDEBUG( "h5", modname << " handles dap services" << endl ) ;
    BESDapService::handle_dap_service( modname ) ;

    BESDEBUG("h5", "    adding " << HDF5_CATALOG << " catalog" << endl) ;
    if( !BESCatalogList::TheCatalogList()->ref_catalog( HDF5_CATALOG ) )
    {
	BESCatalogList::TheCatalogList()->
	    add_catalog(new BESCatalogDirectory(HDF5_CATALOG));
    }
    else
    {
	BESDEBUG( "h5", "    catalog already exists, skipping" << endl ) ;
    }

    BESDEBUG("h5", "    adding catalog container storage " << HDF5_CATALOG
		   << endl) ;
    if( !BESContainerStorageList::TheList()->ref_persistence( HDF5_CATALOG ) )
    {
	BESContainerStorageCatalog *csc =
	    new BESContainerStorageCatalog(HDF5_CATALOG);
	BESContainerStorageList::TheList()->add_persistence(csc);
    }
    else
    {
	BESDEBUG( "h5", "    storage already exists, skipping" << endl ) ;
    }

    BESDEBUG("h5", "    adding h5 debug context" << endl) ;
        BESDebug::Register("h5");

    BESDEBUG("h5", "Done Initializing HDF5 " << modname << endl) ;
}

void HDF5Module::terminate(const string & modname)
{
    BESDEBUG("h5", "Cleaning HDF5 module " << modname << endl) ;

        BESDEBUG("h5",
                 "    removing " << modname << " request handler" << endl) ;
    BESRequestHandler *rh =
        BESRequestHandlerList::TheList()->remove_handler(modname);
    if (rh)
        delete rh;

    BESDEBUG("h5",
             "    removing catalog container storage " << HDF5_CATALOG <<
             endl) ;
    BESContainerStorageList::TheList()->deref_persistence(HDF5_CATALOG);

    BESDEBUG("h5", "    removing " << HDF5_CATALOG << " catalog" << endl) ;
    BESCatalogList::TheCatalogList()->deref_catalog(HDF5_CATALOG);

    BESDEBUG("h5", "Done Cleaning HDF5 module " << modname << endl) ;
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance
 *
 * @param strm C++ i/o stream to dump the information to
 */
void HDF5Module::dump(ostream & strm) const
{
    strm << BESIndent::LMarg << "HDF5Module::dump - ("
        << (void *) this << ")" << endl;
}

extern "C" {
    BESAbstractModule *maker() {
        return new HDF5Module;
}}
