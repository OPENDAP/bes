// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of S3_module, A C++ module that can be loaded in to
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

#include "S3Module.h"
#include "S3RequestHandler.h"
#include "S3ContainerStorage.h"

using namespace std;
using namespace s3;

void S3Module::initialize(const string &modname)
{
    BESDEBUG(modname, "Initializing s3 Module " << modname << endl);

    BESRequestHandlerList::TheList()->add_handler(modname, new S3RequestHandler(modname));

    BESContainerStorageList::TheList()->add_persistence(new S3ContainerStorage(modname));

    BESDebug::Register(modname);

    BESDEBUG(modname, "Done Initializing s3 Module " << modname << endl);
}

void S3Module::terminate(const string &modname)
{
    BESDEBUG(modname, "Cleaning s3 module " << modname << endl);

    BESDEBUG(modname, "Done Cleaning s3 module " << modname << endl);
}

void S3Module::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << "S3Module::dump - (" << (void *) this << ")" << endl;
}

extern "C"
BESAbstractModule *maker()
{
    return new S3Module;
}

