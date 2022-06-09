// GDALModule.cc

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// This file is part of the GDAL OPeNDAP Adapter

// Copyright (c) 2004 OPeNDAP, Igdal.
// Author: Frank Warmerdam <warmerdam@pobox.com>
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
// Foundation, Igdal., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//
// You can contact OPeNDAP, Igdal. at PO Box 112, Saunderstown, RI. 02874-0112.

#include "config.h"

#include <iostream>

#include <dispatch/BESRequestHandlerList.h>
#include <dispatch/BESContainerStorageList.h>
#include <dispatch/BESFileContainerStorage.h>
#include <dispatch/BESCatalogDirectory.h>
#include <dispatch/BESCatalogList.h>
#include <dispatch/BESReturnManager.h>
#include <dispatch/BESServiceRegistry.h>
#include <dispatch/BESDebug.h>

#include <dap/BESDapNames.h>
#include <dap/BESDapService.h>

#include "GDALModule.h"
#include "GDALRequestHandler.h"
#include "writer/GeoTiffTransmitter.h"
#include "writer/JPEG2000Transmitter.h"

#define GDAL_CATALOG "catalog"
#define RETURNAS_GEOTIFF "geotiff"
#define RETURNAS_JPEG2000 "jpeg2000"

#define JP2 1

using namespace std;

void GDALModule::initialize(const string & modname)
{
    BESDEBUG("gdal", "Initializing GDAL module " << modname << endl);

    BESRequestHandler *handler = new GDALRequestHandler(modname);
    BESRequestHandlerList::TheList()->add_handler(modname, handler);

    // This part if the module responds to requests for DMR, DAP, etc., responses.

    BESDapService::handle_dap_service(modname);

    if (!BESCatalogList::TheCatalogList()->ref_catalog(GDAL_CATALOG)) {
        BESCatalogList::TheCatalogList()-> add_catalog(new BESCatalogDirectory(GDAL_CATALOG));
    }

    if (!BESContainerStorageList::TheList()->ref_persistence(GDAL_CATALOG)) {
        BESFileContainerStorage *csc = new BESFileContainerStorage(GDAL_CATALOG);
        BESContainerStorageList::TheList()->add_persistence(csc);
    }

    // This part of the handler sets up transmitters that return geotiff and jp2000 responses
    BESReturnManager::TheManager()->add_transmitter(RETURNAS_GEOTIFF, new GeoTiffTransmitter());
    BESServiceRegistry::TheRegistry()->add_format(OPENDAP_SERVICE, DATA_SERVICE, RETURNAS_GEOTIFF);

#if JP2
    BESReturnManager::TheManager()->add_transmitter(RETURNAS_JPEG2000, new JPEG2000Transmitter());
    BESServiceRegistry::TheRegistry()->add_format(OPENDAP_SERVICE, DATA_SERVICE, RETURNAS_JPEG2000);
#endif

    BESDebug::Register("gdal");

    BESDEBUG("gdal", "Done Initializing GDAL module " << modname << endl);
}

void GDALModule::terminate(const string & modname)
{
    BESDEBUG("gdal", "Cleaning GDAL module " << modname << endl);

    BESContainerStorageList::TheList()->deref_persistence(GDAL_CATALOG);

    BESCatalogList::TheCatalogList()->deref_catalog(GDAL_CATALOG);

    BESReturnManager::TheManager()->del_transmitter(RETURNAS_GEOTIFF);
#if JP2
    BESReturnManager::TheManager()->del_transmitter(RETURNAS_JPEG2000);
#endif

    delete BESRequestHandlerList::TheList()->remove_handler(modname);

    BESDEBUG("gdal", "Done Cleaning GDAL module " << modname << endl);
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance
 *
 * @param strm C++ i/o stream to dump the information to
 */
void GDALModule::dump(ostream & strm) const
{
    strm << BESIndent::LMarg << "GDALModule::dump - (" << (void *) this << ")" << endl;
}

extern "C" BESAbstractModule * maker()
{
    return new GDALModule;
}

