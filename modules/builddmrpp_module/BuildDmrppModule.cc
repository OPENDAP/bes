// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of builddmrpp_module, A C++ module that can be loaded in to
// the OPeNDAP Back-End Server (BES) and is able to handle remote requests.

// Copyright (c) 2023 OPeNDAP, Inc.
// Author: Daniel Holloway <dholloway@opendap.org>
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
// Authors:
//      dan       Daniel Holloway <dholloway@opendap.org>
//      ndp       Nathan Potter <ndp@opendap.org>

#include "config.h"

#include <iostream>
#include <vector>
#include <string>

#include <BESRequestHandlerList.h>
#include <BESDebug.h>
#include <BESContainerStorageList.h>

#include "BuildDmrppModule.h"
#include "BuildDmrppRequestHandler.h"
#include "NgapBuildDmrppContainerStorage.h"

using namespace std;
using namespace builddmrpp;

void BuildDmrppModule::initialize(const string &modname)
{
    BESDEBUG(modname, "Initializing BuildDmrpp Module " << modname << endl);

    BESRequestHandlerList::TheList()->add_handler(modname, new BuildDmrppRequestHandler(modname));

    BESContainerStorageList::TheList()->add_persistence(new NgapBuildDmrppContainerStorage(modname));

    BESDebug::Register(modname);

    BESDEBUG(modname, "Done Initializing BuildDmrpp Module " << modname << endl);
}

void BuildDmrppModule::terminate(const string &modname)
{
    BESDEBUG(modname, "Cleaning BuildDmrpp module " << modname << endl);

    BESRequestHandler *rh = BESRequestHandlerList::TheList()->remove_handler(modname);
    delete rh;

    BESContainerStorageList::TheList()->deref_persistence(modname);

    BESDEBUG(modname, "Done Cleaning BuildDmrpp module " << modname << endl);
}

void BuildDmrppModule::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << "BuildDmrppModule::dump - (" << (void *) this << ")" << endl;
}

extern "C"
BESAbstractModule *maker()
{
    return new BuildDmrppModule;
}

