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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// You can contact University Corporation for Atmospheric Research at
// 3080 Center Green Drive, Boulder, CO 80301
 
// (c) COPYRIGHT University Corporation for Atmostpheric Research 2004-2005
// Please read the full copyright statement in the file COPYRIGHT_UCAR.
//
// Authors:
//      pwest       Patrick West <pwest@ucar.edu>

#include <iostream>

using std::endl ;

#include "HDF4Module.h"
#include "BESRequestHandlerList.h"
#include "HDF4RequestHandler.h"
#include "BESContainerStorageList.h"
#include "BESContainerStorageCatalog.h"
#include "BESCatalogDirectory.h"
#include "BESCatalogList.h"
#include "BESLog.h"

#define HDF4_CATALOG "catalog"

void
HDF4Module::initialize( const string &modname )
{
    if( BESLog::TheLog()->is_verbose() )
	(*BESLog::TheLog()) << "Initializing HDF4:" << endl ;

    if( BESLog::TheLog()->is_verbose() )
	(*BESLog::TheLog()) << "    adding " << modname << " request handler" 
		      << endl ;
    BESRequestHandlerList::TheList()->add_handler( modname, new HDF4RequestHandler( modname ) ) ;

    if( BESLog::TheLog()->is_verbose() )
	(*BESLog::TheLog()) << "    adding " << HDF4_CATALOG << " catalog" 
		      << endl ;
    BESCatalogList::TheCatalogList()->add_catalog( new BESCatalogDirectory( HDF4_CATALOG ) ) ;

    if( BESLog::TheLog()->is_verbose() )
	(*BESLog::TheLog()) << "Adding Catalog Container Storage" << endl;
    BESContainerStorageCatalog *csc = new BESContainerStorageCatalog( HDF4_CATALOG ) ;
    BESContainerStorageList::TheList()->add_persistence( csc ) ;
}

void
HDF4Module::terminate( const string &modname )
{
    if( BESLog::TheLog()->is_verbose() )
	(*BESLog::TheLog()) << "Removing HDF4 Handlers" << endl;
    BESRequestHandler *rh = BESRequestHandlerList::TheList()->remove_handler( modname ) ;
    if( rh ) delete rh ;

    if( BESLog::TheLog()->is_verbose() )
	(*BESLog::TheLog()) << "Removing catalog Container Storage" << endl;
    BESContainerStorageList::TheList()->del_persistence( HDF4_CATALOG ) ;
}

extern "C"
{
    BESAbstractModule *maker()
    {
	return new HDF4Module ;
    }
}

