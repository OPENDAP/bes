// FONgModule.cc

// This file is part of BES GDAL File Out Module

// Copyright (c) 2012 OPeNDAP, Inc.
// Author: James Gallagher <jgallagher@opendap.org>
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

#include "config.h"

#include <iostream>

using std::endl;
using std::ostream;
using std::string;

#include "FONgModule.h"
#include "GeoTiffTransmitter.h"
#include "JPEG2000Transmitter.h"
#include "FONgRequestHandler.h"
#include "BESRequestHandlerList.h"

#include <BESReturnManager.h>

#include <BESServiceRegistry.h>
#include <BESDapNames.h>

#include <TheBESKeys.h>
#include <BESDebug.h>

#define RETURNAS_GEOTIFF "geotiff"
#define RETURNAS_JPEG2000 "jpeg2000"

#define JP2 1


/** @brief initialize the module by adding call backs and registering
 * objects with the framework
 *
 * Registers the request handler to add to a version or help request,
 * and adds the File Out transmitter for a "returnAs geotiff" request.
 * Also adds geotiff as a return for the dap service dods request and
 * registers the debug context.
 *
 * @param modname The name of the module being loaded
 */
void FONgModule::initialize(const string &modname)
{
    BESDEBUG( "fong", "Initializing module " << modname << endl );

    BESRequestHandler *handler = new FONgRequestHandler(modname);
    BESRequestHandlerList::TheList()->add_handler(modname, handler);

    BESDEBUG( "fong", "    adding " << RETURNAS_GEOTIFF << " transmitter" << endl );
    BESReturnManager::TheManager()->add_transmitter(RETURNAS_GEOTIFF, new GeoTiffTransmitter());

#if JP2
    BESDEBUG( "fong", "    adding " << RETURNAS_JPEG2000 << " transmitter" << endl );
    BESReturnManager::TheManager()->add_transmitter(RETURNAS_JPEG2000, new JPEG2000Transmitter());
#endif

    BESDEBUG( "fong", "    adding geotiff service to dap" << endl );
    BESServiceRegistry::TheRegistry()->add_format(OPENDAP_SERVICE, DATA_SERVICE, RETURNAS_GEOTIFF);

#if JP2
    BESDEBUG( "fong", "    adding jpeg2000 service to dap" << endl );
    BESServiceRegistry::TheRegistry()->add_format(OPENDAP_SERVICE, DATA_SERVICE, RETURNAS_JPEG2000);
#endif

    BESDebug::Register("fong");
    BESDEBUG( "fong", "Done Initializing module " << modname << endl );
}

/** @brief removes any registered callbacks or objects from the
 * framework
 *
 * Any registered callbacks or objects, registered during
 * initialization, are removed from the framework.
 *
 * @param modname The name of the module being removed
 */
void FONgModule::terminate(const string &modname)
{
    BESDEBUG( "fong", "Cleaning module " << modname << endl );
    BESDEBUG( "fong", "    removing " << RETURNAS_GEOTIFF << " transmitter" << endl );

    BESReturnManager::TheManager()->del_transmitter(RETURNAS_GEOTIFF);

#if JP2
    BESDEBUG( "fong", "    removing " << RETURNAS_JPEG2000 << " transmitter" << endl );
    BESReturnManager::TheManager()->del_transmitter(RETURNAS_JPEG2000);
#endif

    BESDEBUG( "fong", "    removing " << modname << " request handler " << endl );

    BESRequestHandler *rh = BESRequestHandlerList::TheList()->remove_handler(modname);
    if (rh)
        delete rh;

    BESDEBUG( "fong", "Done Cleaning module " << modname << endl );
}

/** @brief dumps information about this object for debugging purposes
 *
 * Displays the pointer value of this instance plus any instance data
 *
 * @param strm C++ i/o stream to dump the information to
 */
void FONgModule::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << "FONgModule::dump - (" << (void *) this << ")" << endl;
}

/** @brief A c function that adds this module to the list of modules to
 * be dynamically loaded.
 */
extern "C"
BESAbstractModule *maker()
{
    return new FONgModule;
}

