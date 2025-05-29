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
#include <BESFileContainerStorage.h>
#include <BESCatalogDirectory.h>
#include <BESCatalogList.h>

#include <BESReturnManager.h>
#include <BESServiceRegistry.h>
#include <BESDapNames.h>

#include "DmrppModule.h"
#include "DmrppRequestHandler.h"

#include "ngap_container/NgapOwnedContainerStorage.h"
#include "dmrpp_transmitter/ReturnAsDmrppTransmitter.h"

#define RETURNAS_DMRPP "dmrpp"

using namespace std;

#define DAP_CATALOG "catalog"

#define prolog string("DmrppModule::").append(__func__).append("() - ")

namespace dmrpp {

void DmrppModule::initialize(const string &modname)
{
    BESDebug::Register(modname);

    BESDEBUG(modname, prolog << "Initializing DMR++ Reader Module " << modname << endl);

    BESRequestHandlerList::TheList()->add_handler(modname, new DmrppRequestHandler(modname));

    BESDapService::handle_dap_service(modname);

    if (!BESCatalogList::TheCatalogList()->ref_catalog(DAP_CATALOG)) {
        BESCatalogList::TheCatalogList()->add_catalog(new BESCatalogDirectory(DAP_CATALOG));
    }

    if (!BESContainerStorageList::TheList()->ref_persistence(DAP_CATALOG)) {
        BESFileContainerStorage *csc = new BESFileContainerStorage(DAP_CATALOG);
        BESContainerStorageList::TheList()->add_persistence(csc);
    }

    /*BESDEBUG(modname, "    adding " << modname << " dmrpp-ngap container storage" << endl);
    BESContainerStorageList::TheList()->add_persistence(new NgapBuildDmrppContainerStorage(modname));*/

    BESContainerStorageList::TheList()->add_persistence(new ngap::NgapOwnedContainerStorage(modname));

    // This part of the handler sets up transmitters that return DMRPP responses
    BESReturnManager::TheManager()->add_transmitter(RETURNAS_DMRPP, new ReturnAsDmrppTransmitter());
    BESServiceRegistry::TheRegistry()->add_format(OPENDAP_SERVICE, DAP4DATA_SERVICE, RETURNAS_DMRPP);

    BESDEBUG(modname, prolog << "Done Initializing DMR++ Reader Module " << modname << endl);
}

void DmrppModule::terminate(const string &modname)
{
    BESDEBUG(modname, prolog << "Cleaning DMR++ Reader Module " << modname << endl);

    BESRequestHandler *rh = BESRequestHandlerList::TheList()->remove_handler(modname);
    delete rh;

    BESContainerStorageList::TheList()->deref_persistence(modname);

    BESContainerStorageList::TheList()->deref_persistence(DAP_CATALOG);

    BESCatalogList::TheCatalogList()->deref_catalog(DAP_CATALOG);

    BESDEBUG(modname, prolog << "Done Cleaning DMR++ Reader Module " << modname << endl);
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

} // namespace dmrpp

