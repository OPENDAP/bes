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

#include "BESContainerStorageList.h"
#include "BESFileContainerStorage.h"
#include "BESCatalogDirectory.h"
#include "BESCatalogList.h"

#include "BESInternalError.h"
#include "BESDebug.h"

#include "TestModule.h"

using std::endl;

const string &catalog_name = "second";

/**
 * @brief Add a second, non-default, catalog.
 *
 * All this module needs is to load a catalog, it does not do anything else.
 *
 * @param modname The name of the module, set by the value used in the
 * bes.conf file.
 */
void TestModule::initialize(const string &modname)
{
    BESDEBUG(modname, "Initializing Non-default Catalog Test Module " << modname << endl);

    if (!BESCatalogList::TheCatalogList()->ref_catalog(catalog_name)) {
        BESCatalogList::TheCatalogList()->add_catalog(new BESCatalogDirectory(catalog_name));
    }

    if (!BESContainerStorageList::TheList()->ref_persistence(catalog_name)) {
        BESFileContainerStorage *csc = new BESFileContainerStorage(catalog_name);
        BESContainerStorageList::TheList()->add_persistence(csc);
    }

    BESDebug::Register(modname);

    BESDEBUG(modname, "Done Initializing Test Module " << modname << endl);
}

/**
 * @brief Remove/delete the catalog and its persistance.
 * @param modname
 */
void TestModule::terminate(const string &modname)
{
    BESDEBUG(modname, "Cleaning Test Module " << modname << endl);

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

