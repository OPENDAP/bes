// DapModule.cc

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

#include <iostream>

using std::endl;
using std::ostream;

#include "DapModule.h"
#include "DapRequestHandler.h"

#include <BESRequestHandlerList.h>
#include <BESDebug.h>

#include <BESDapService.h>
#include <BESResponseNames.h>
#include <BESContainerStorageList.h>
#include <BESFileContainerStorage.h>
#include <BESCatalogDirectory.h>
#include <BESCatalogList.h>

#include "BESInternalError.h"

void DapModule::initialize(const string &modname)
{
    BESDEBUG(modname, "Initializing Dap Reader Module " << modname << endl);

    BESRequestHandlerList::TheList()->add_handler(modname, new DapRequestHandler(modname));

    BESDapService::handle_dap_service(modname);

    string default_catalog_name = BESCatalogList::TheCatalogList()->default_catalog_name();
    if (!BESCatalogList::TheCatalogList()->ref_catalog(default_catalog_name)) {
        throw BESInternalError("Should never have to add the default catalog.", __FILE__, __LINE__);
    }

    // TODO this is probably bogus too. jhrg 7/23/18
    if (!BESContainerStorageList::TheList()->ref_persistence(default_catalog_name)) {
        BESFileContainerStorage *csc = new BESFileContainerStorage(default_catalog_name);
        BESContainerStorageList::TheList()->add_persistence(csc);
    }

    BESDebug::Register(modname);

    BESDEBUG(modname, "Done Initializing Dap Reader Module " << modname << endl);
}

void DapModule::terminate(const string &modname)
{
    BESDEBUG(modname, "Cleaning Dap Reader Module " << modname << endl);

    delete BESRequestHandlerList::TheList()->remove_handler(modname);

    string default_catalog_name = BESCatalogList::TheCatalogList()->default_catalog_name();
    BESContainerStorageList::TheList()->deref_persistence(default_catalog_name);

    BESCatalogList::TheCatalogList()->deref_catalog(default_catalog_name);

    BESDEBUG(modname, "Done Cleaning Dap Reader Module " << modname << endl);
}

extern "C" {
BESAbstractModule *maker()
{
    return new DapModule;
}
}

void DapModule::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << "DapModule::dump - (" << (void *) this << ")" << endl;
}

