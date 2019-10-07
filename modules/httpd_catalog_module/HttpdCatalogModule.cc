
// -*- mode: c++; c-basic-offset:4 -*-
//
// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.
//
// Copyright (c) 2018 OPeNDAP, Inc.
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

#include <iostream>
#include <string>

#include <BESRequestHandlerList.h>
#include <BESResponseHandlerList.h>
#include <BESResponseNames.h>

#include <BESContainerStorageList.h>
#include <BESFileContainerStorage.h>
#include <BESCatalogDirectory.h>
#include <BESCatalogList.h>
#include <BESDapService.h>

#include <BESLog.h>
#include <BESDebug.h>

#include "HttpdCatalogNames.h"
#include "HttpdCatalogModule.h"
#include "HttpdCatalogContainerStorage.h"
#include "HttpdCatalog.h"

using namespace std;

#define prolog string("HttpdCatalogModule::").append(__func__).append("() - ")

namespace httpd_catalog {

void HttpdCatalogModule::initialize(const string &modname)
{
    BESDebug::Register(MODULE);

    BESDEBUG(MODULE, prolog << "Initializing Module: " << modname << endl);

    if (!BESCatalogList::TheCatalogList()->ref_catalog(HTTPD_CATALOG_NAME)) {
        BESCatalogList::TheCatalogList()->add_catalog(new HttpdCatalog(HTTPD_CATALOG_NAME));
    }

    if (!BESContainerStorageList::TheList()->ref_persistence(HTTPD_CATALOG_NAME)) {
        BESContainerStorageList::TheList()->add_persistence(new HttpdCatalogContainerStorage(HTTPD_CATALOG_NAME));
    }

    BESDEBUG(MODULE, "Done Initializing Handler: " << modname << endl);
}

void HttpdCatalogModule::terminate(const string &modname)
{
    BESDEBUG(MODULE, prolog << "Cleaning Module: " << modname << endl);

    BESContainerStorageList::TheList()->deref_persistence(HTTPD_CATALOG_NAME);

    BESCatalogList::TheCatalogList()->deref_catalog(HTTPD_CATALOG_NAME);

    BESDEBUG(MODULE, prolog << "Done Cleaning Module: " << modname << endl);
}

extern "C" {
BESAbstractModule *maker()
{
    return new HttpdCatalogModule;
}
}

void HttpdCatalogModule::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << prolog<< "(" << (void *) this << ")" << endl;
}

} // namespace httpd_catalog
