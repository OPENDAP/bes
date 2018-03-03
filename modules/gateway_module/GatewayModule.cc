// GatewayModule.cc

// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of gateway_module, A C++ module that can be loaded in to
// the OPeNDAP Back-End Server (BES) and is able to handle remote requests.

// Copyright (c) 2002,2003 OPeNDAP, Inc.
// Author: Patrick West <pwest@ucar.edu>
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

#include "config.h"

#include <iostream>
#include <vector>
#include <string>

#include <BESRequestHandlerList.h>
#include <BESDebug.h>
#include <BESResponseHandlerList.h>
#include <BESResponseNames.h>
#include <BESContainerStorageList.h>
#include <TheBESKeys.h>
#include <BESSyntaxUserError.h>

#include "GatewayModule.h"
#include "GatewayRequestHandler.h"
#include "GatewayResponseNames.h"
#include "GatewayContainerStorage.h"
#include "GatewayUtils.h"

using namespace std;
using namespace gateway;

void GatewayModule::initialize(const string &modname)
{
    BESDEBUG(modname, "Initializing Gateway Module " << modname << endl);

    BESDEBUG(modname, "    adding " << modname << " request handler" << endl);
    BESRequestHandlerList::TheList()->add_handler(modname, new GatewayRequestHandler(modname));

    BESDEBUG(modname, "    adding " << modname << " container storage" << endl);
    BESContainerStorageList::TheList()->add_persistence(new GatewayContainerStorage(modname));

    BESDEBUG(modname, "    initialize the gateway utilities and params" << endl);
    GatewayUtils::Initialize();

    BESDEBUG(modname, "    adding Gateway debug context" << endl);
    BESDebug::Register(modname);

    BESDEBUG(modname, "Done Initializing Gateway Module " << modname << endl);
}

void GatewayModule::terminate(const string &modname)
{
    BESDEBUG(modname, "Cleaning Gateway module " << modname << endl);

    BESDEBUG(modname, "    removing " << modname << " request handler" << endl);
    BESRequestHandler *rh = BESRequestHandlerList::TheList()->remove_handler(modname);
    if (rh)
        delete rh;

    BESContainerStorageList::TheList()->deref_persistence(modname);

    // TERM_END
    BESDEBUG(modname, "Done Cleaning Gateway module " << modname << endl);
}

void GatewayModule::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << "GatewayModule::dump - (" << (void *) this << ")" << endl;
}

extern "C"
BESAbstractModule *maker()
{
    return new GatewayModule;
}

