// DmrppModule.cc

// Copyright (c) 2016 OPeNDAP, Inc. Author: Nathan Potter <npotter@opendap.org>
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

#include <iostream>
#include <string>

#include <BESRequestHandlerList.h>
#include <BESDebug.h>

#include <BESDapService.h>
#include <BESResponseNames.h>
#include <BESContainerStorageList.h>
#include <BESContainerStorageCatalog.h>
#include <BESCatalogDirectory.h>
#include <BESCatalogList.h>

#include "DmrppModule.h"
#include "DmrppRequestHandler.h"

using namespace std;

#define DAP_CATALOG "catalog"

void DmrppModule::initialize(const string &modname)
{
    BESDebug::Register(modname);

    BESDEBUG(modname, "Initializing DMR++ Reader Module " << modname << endl);

    BESRequestHandlerList::TheList()->add_handler(modname, new DmrppRequestHandler(modname));

    BESDapService::handle_dap_service(modname);

    if (!BESCatalogList::TheCatalogList()->ref_catalog(DAP_CATALOG)) {
        BESCatalogList::TheCatalogList()->add_catalog(new BESCatalogDirectory(DAP_CATALOG));
    }

    if (!BESContainerStorageList::TheList()->ref_persistence(DAP_CATALOG)) {
        BESContainerStorageCatalog *csc = new BESContainerStorageCatalog(DAP_CATALOG);
        BESContainerStorageList::TheList()->add_persistence(csc);
    }

    BESDEBUG(modname, "Done Initializing DMR++ Reader Module " << modname << endl);
}

void DmrppModule::terminate(const string &modname)
{
    BESDEBUG(modname, "Cleaning DMR++ Reader Module " << modname << endl);

    BESRequestHandler *rh = BESRequestHandlerList::TheList()->remove_handler(modname);
    delete rh;

    BESContainerStorageList::TheList()->deref_persistence(DAP_CATALOG);

    BESCatalogList::TheCatalogList()->deref_catalog(DAP_CATALOG);

    BESDEBUG(modname, "Done Cleaning DMR++ Reader Module " << modname << endl);
}

extern "C" {
BESAbstractModule *maker()
{
    return new DmrppModule;
}
}

void DmrppModule::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << "DmrppModule::dump - (" << (void *) this << ")" << endl;
}

