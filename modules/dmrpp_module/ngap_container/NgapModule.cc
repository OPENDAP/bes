// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of ngap_module, A C++ module that can be loaded in to
// the OPeNDAP Back-End Server (BES) and is able to handle remote requests.

// Copyright (c) 2020 OPeNDAP, Inc.
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


#include "config.h"

#include <iostream>
#include <vector>
#include <string>

#include <BESRequestHandlerList.h>
#include <BESDebug.h>
#include <BESContainerStorageList.h>

#include "NgapModule.h"
#include "NgapRequestHandler.h"
#include "NgapOwnedContainerStorage.h"

using namespace std;
using namespace ngap;

void NgapModule::initialize(const string &modname)
{
    BESDEBUG(modname, "Initializing NGAP Module " << modname << endl);

    BESDEBUG(modname, "    adding " << modname << " request handler" << endl);
    BESRequestHandlerList::TheList()->add_handler(modname, new NgapRequestHandler(modname));

    BESDEBUG(modname, "    adding OWNED " << modname << " container storage" << endl);
    // FIXME Do NOT merge this code as is since it breaks the handler needed for NGAP.
    //  This is a POC modification to test access to data using DMR++ documents that
    //  OPeNDAP 'owns.' jhrg 5/17/24
    BESContainerStorageList::TheList()->add_persistence(new NgapOwnedContainerStorage(modname));

    BESDEBUG(modname, "    adding NGAP debug context" << endl);
    BESDebug::Register(modname);

    BESDEBUG(modname, "Done Initializing NGAP Module " << modname << endl);
}

void NgapModule::terminate(const string &modname)
{
    BESDEBUG(modname, "Cleaning NGAP module " << modname << endl);

    BESDEBUG(modname, "    removing " << modname << " request handler" << endl);
    BESRequestHandler *rh = BESRequestHandlerList::TheList()->remove_handler(modname);
    delete rh;

    BESContainerStorageList::TheList()->deref_persistence(modname);

    BESDEBUG(modname, "Done Cleaning NGAP module " << modname << endl);
}

void NgapModule::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << "NgapModule::dump - (" << (void *) this << ")" << endl;
}

extern "C"
BESAbstractModule *maker()
{
    return new NgapModule;
}

