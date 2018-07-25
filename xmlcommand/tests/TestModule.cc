// TestModule.cc

// Copyright (c) 2013 OPeNDAP, Inc. Author: James Gallagher
// <jgallagher@opendap.org>, Patrick West <pwest@opendap.org>
// Nathan Potter <npotter@opendap.org>
//                                                                            
// modify it under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation; either version 2.1 of
// the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// Lesser General Public License for more details.
//
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
// 02110-1301 U\ SA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI.
// 02874-0112.

#include "config.h"

#include <string>
#include <iostream>

#include "TestModule.h"
// #include "DapRequestHandler.h"

//#include <BESRequestHandlerList.h>
#include <BESDebug.h>

#if 0
#include <BESDapService.h>
#include <BESResponseNames.h>
#endif

#include <BESContainerStorageList.h>
#include <BESContainerStorageCatalog.h>
#include <BESCatalogDirectory.h>
#include <BESCatalogList.h>

#include "BESInternalError.h"

// #define DAP_CATALOG "catalog"

const string &catalog_name = "second";

void TestModule::initialize(const string &modname)
{
    BESDEBUG(modname, "Initializing Non-default Catalog Test Module " << modname << endl);

#if 0
    BESRequestHandlerList::TheList()->add_handler(modname, new DapRequestHandler(modname));

    BESDapService::handle_dap_service(modname);
#endif


    if (!BESCatalogList::TheCatalogList()->ref_catalog(catalog_name)) {
        BESCatalogList::TheCatalogList()->add_catalog(new BESCatalogDirectory(catalog_name));
    }

    if (!BESContainerStorageList::TheList()->ref_persistence(catalog_name)) {
        BESContainerStorageCatalog *csc = new BESContainerStorageCatalog(catalog_name);
        BESContainerStorageList::TheList()->add_persistence(csc);
    }

    BESDebug::Register(modname);

    BESDEBUG(modname, "Done Initializing Test Module " << modname << endl);
}

void TestModule::terminate(const string &modname)
{
    BESDEBUG(modname, "Cleaning Test Module " << modname << endl);

#if 0
    BESRequestHandler *rh = 0;

    rh = BESRequestHandlerList::TheList()->remove_handler(modname);
    if (rh) delete rh;
#endif

#if 0
    delete BESRequestHandlerList::TheList()->remove_handler(modname);
#endif

    BESContainerStorageList::TheList()->deref_persistence(catalog_name);

    BESCatalogList::TheCatalogList()->deref_catalog(catalog_name);

    BESDEBUG(modname, "Done Cleaning TEst Module " << modname << endl);
}

extern "C" {
BESAbstractModule *maker()
{
    return new TestModule;
}
}

void TestModule::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << "TestModule::dump - (" << (void *) this << ")" << endl;
}

