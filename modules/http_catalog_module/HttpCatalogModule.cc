// CSVModule.cc

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

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

#include <BESRequestHandlerList.h>
#include <BESResponseHandlerList.h>
#include <BESResponseNames.h>

#include <BESContainerStorageList.h>
#include <BESContainerStorageCatalog.h>
#include <BESCatalogDirectory.h>
#include <BESCatalogList.h>
#include <BESDapService.h>

#include <BESLog.h>
#include <BESDebug.h>

#if 0
#include "HttpCatalog.h"
#include "HttpCatalogContainerStorage.h"
#endif

#include "HttpCatalogNames.h"
#include "HttpCatalogModule.h"

using namespace std;
#if 0
using namespace http_catalog;
#endif


#define prolog std::string("CmrModule::").append(__func__).append("() - ")

void CmrModule::initialize(const string &modname)
{
    BESDebug::Register(MODULE);

    BESDEBUG(MODULE, prolog << "Initializing Module: " << modname << endl);

#if 0
    if (!BESCatalogList::TheCatalogList()->ref_catalog(HTTP_CATALOG_NAME)) {
        BESCatalogList::TheCatalogList()->add_catalog(new HttpCatalog(HTTP_CATALOG_NAME));
    }

    if (!BESContainerStorageList::TheList()->ref_persistence(HTTP_CATALOG_NAME)) {
        BESContainerStorageList::TheList()->add_persistence(newHttpCatalogContainerStorage(HTTP_CATALOG_NAME));
    }
#endif

    BESDEBUG(MODULE, "Done Initializing Handler: " << modname << endl);
}

void CmrModule::terminate(const string &modname)
{
    BESDEBUG(MODULE, prolog << "Cleaning Module: " << modname << endl);

    BESContainerStorageList::TheList()->deref_persistence(HTTP_CATALOG_NAME);

    BESCatalogList::TheCatalogList()->deref_catalog(HTTP_CATALOG_NAME);

    BESDEBUG(MODULE, prolog << "Done Cleaning Module: " << modname << endl);
}

extern "C" {
BESAbstractModule *maker()
{
    return new CmrModule;
}
}

void CmrModule::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << prolog<< "(" << (void *) this << ")" << endl;
}

