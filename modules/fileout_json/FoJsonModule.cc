
// -*- mode: c++; c-basic-offset:4 -*-

// FoJsonModule.cc

// This file is part of BES JSON File Out Module

// Copyright (c) 2014 OPeNDAP, Inc.
// Author: Nathan Potter <ndp@opendap.org>
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
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.
// (c) COPYRIGHT URI/MIT 1995-1999
// Please read the full copyright statement in the file COPYRIGHT_URI.
//

#include "config.h"

#include <iostream>

using std::endl;
using std::ostream;
using std::string;

#include "FoJsonModule.h"
#include "FoDapJsonTransmitter.h"
#include "FoInstanceJsonTransmitter.h"
#include "FoJsonRequestHandler.h"
#include "BESRequestHandlerList.h"

#include <BESReturnManager.h>

#include <BESServiceRegistry.h>
#include <BESDapNames.h>

#include <TheBESKeys.h>
#include <BESDebug.h>

#define RETURNAS_JSON "json"
#define RETURNAS_IJSON "ijson"



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
void FoJsonModule::initialize(const string &modname)
{
    BESDEBUG( "fojson", "Initializing module " << modname << endl );

    BESRequestHandler *handler = new FoJsonRequestHandler(modname);
    BESRequestHandlerList::TheList()->add_handler(modname, handler);


    BESDEBUG( "fojson", "    adding " << RETURNAS_JSON << " transmitter" << endl );
    BESReturnManager::TheManager()->add_transmitter(RETURNAS_JSON, new FoDapJsonTransmitter());


    BESDEBUG( "fojson", "    adding " << RETURNAS_IJSON << " transmitter" << endl );
    BESReturnManager::TheManager()->add_transmitter(RETURNAS_IJSON, new FoInstanceJsonTransmitter());


    BESDebug::Register("fojson");
    BESDEBUG( "fojson", "Done Initializing module " << modname << endl );
}

/** @brief removes any registered callbacks or objects from the
 * framework
 *
 * Any registered callbacks or objects, registered during
 * initialization, are removed from the framework.
 *
 * @param modname The name of the module being removed
 */
void FoJsonModule::terminate(const string &modname)
{
    BESDEBUG( "fojson", "Cleaning module " << modname << endl );
    BESDEBUG( "fojson", "    removing " << RETURNAS_JSON << " transmitter" << endl );

    BESReturnManager::TheManager()->del_transmitter(RETURNAS_JSON);

    BESDEBUG( "fojson", "    removing " << RETURNAS_IJSON << " transmitter" << endl );

    BESReturnManager::TheManager()->del_transmitter(RETURNAS_IJSON);

    BESDEBUG( "fojson", "    removing " << modname << " request handler " << endl );

    BESRequestHandler *rh = BESRequestHandlerList::TheList()->remove_handler(modname);
    if (rh)
        delete rh;

    BESDEBUG( "fojson", "Done Cleaning module " << modname << endl );
}

/** @brief dumps information about this object for debugging purposes
 *
 * Displays the pointer value of this instance plus any instance data
 *
 * @param strm C++ i/o stream to dump the information to
 */
void FoJsonModule::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << "FoJsonModule::dump - (" << (void *) this << ")" << endl;
}

/** @brief A c function that adds this module to the list of modules to
 * be dynamically loaded.
 */
extern "C"
BESAbstractModule *maker()
{
    return new FoJsonModule;
}

