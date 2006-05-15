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
#include "DODSRequestHandlerList.h"
#include "HDF4RequestHandler.h"
#include "ContainerStorageList.h"
#include "ContainerStorageCatalog.h"
#include "DirectoryCatalog.h"
#include "CatalogList.h"
#include "DODSLog.h"

#define HDF4_NAME "hdf"
#define HDF4_CATALOG "catalog"

void
HDF4Module::initialize()
{
    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "Initializing HDF4:" << endl ;

    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "    adding " << HDF4_NAME << " request handler" 
		      << endl ;
    DODSRequestHandlerList::TheList()->add_handler( HDF4_NAME, new HDF4RequestHandler( HDF4_NAME ) ) ;

    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "    adding " << HDF4_NAME << " catalog" 
		      << endl ;
    CatalogList::TheCatalogList()->add_catalog( new DirectoryCatalog( HDF4_CATALOG ) ) ;

    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "Adding Catalog Container Storage" << endl;
    ContainerStorageCatalog *csc = new ContainerStorageCatalog( HDF4_CATALOG ) ;
    ContainerStorageList::TheList()->add_persistence( csc ) ;
}

void
HDF4Module::terminate()
{
    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "Removing HDF4 Handlers" << endl;
    DODSRequestHandler *rh = DODSRequestHandlerList::TheList()->remove_handler( HDF4_NAME ) ;
    if( rh ) delete rh ;

    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "Removing catalog Container Storage" << endl;
    ContainerStorageList::TheList()->del_persistence( HDF4_CATALOG ) ;
}

extern "C"
{
    OPeNDAPAbstractModule *maker()
    {
	return new HDF4Module ;
    }
}

