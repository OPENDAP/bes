//////////////////////////////////////////////////////////////////////////////
// This file is part of the "NcML Module" project, a BES module designed
// to allow NcML files to be used to be used as a wrapper to add
// AIS to existing datasets of any format.
//
// Copyright (c) 2009 OPeNDAP, Inc.
// Author: Michael Johnson  <m.johnson@opendap.org>
//
// For more information, please also see the main website: http://opendap.org/
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
// Please see the files COPYING and COPYRIGHT for more information on the GLPL.
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.
/////////////////////////////////////////////////////////////////////////////

#include "config.h"

#include <sstream>
#include <algorithm>

#include <libdap/DataDDS.h>

#include <BESConstraintFuncs.h>
#include <BESContainerStorage.h>
#include <BESContainerStorageList.h>
#include <BESDapNames.h>
#include <BESDapResponse.h>
#include <BESDataDDSResponse.h>
#include <BESDataHandlerInterface.h>
#include <BESDDSResponse.h>
#include <BESStopWatch.h>
#include <BESInternalError.h>
#include <BESResponseHandler.h>
#include <BESResponseNames.h>
#include <BESRequestHandlerList.h>
#include <BESServiceRegistry.h>
#include <BESTextInfo.h>
#include <BESUtil.h>
#include <BESVersionInfo.h>

#include <BESDebug.h>
#include <BESLog.h>

#include "DDSLoader.h"
#include "NCMLDebug.h"
#include "NCMLUtil.h"

using namespace std;
using namespace agg_util;
using namespace libdap;

#define prolog std::string("DDSLoader::").append(__func__).append("() - ")

// Rep Init

/* static */
long DDSLoader::_gensymID = 0L;

// Impl

DDSLoader::DDSLoader(BESDataHandlerInterface& dhi) :
    _dhi(dhi), /*d_saved_dhi(0),*/_hijacked(false), _filename(""), _store(0), _containerSymbol(""), _origAction(""), _origActionName(
        ""), _origContainer(0), _origResponse(0)
{
}

// WE ONLY COPY THE DHI!  I got forced to impl this.
DDSLoader::DDSLoader(const DDSLoader& proto) :
    _dhi(proto._dhi), /*d_saved_dhi(0),*/_hijacked(false), _filename(""), _store(0), _containerSymbol(""), _origAction(
        ""), _origActionName(""), _origContainer(0), _origResponse(0)
{
}

DDSLoader&
DDSLoader::operator=(const DDSLoader& rhs)
{
    BESDEBUG("ncml", "DDSLoader::operator=: " << endl);

    if (&rhs == this) {
        return *this;
    }

    // First cleanup any state

    // Old comment, written in the midst of fixing bug #2176...
    // I removed this call because ensureClean() will call restoreDHI()
    // and then we will call the BESDataHandlerInterface::clone() method
    // which will take the DHI from the DDSLoader passed in and clone it.
    // ...no sense setting it twice.
    //
    // After the fix...
    // Added it back in because I think it might be used after all. In many
    // (all?) cases, the rhs._dhi is the same object as this->_dhi, so the
    // clone() method will never get called. However, the 'saved state' of
    // the DHI instance might be needed. It's not needed for the current
    // operations (no tests fail when it is removed), but future versions
    // might make use of the saved state.
    // jhrg 4/18/14
    ensureClean();

    // Now copy the dhi only, since
    // we assume we'll be using this fresh.
    // Normally we don't want to copy these
    // but I got forced into it.
    //
    // Update. Fix for bug #2176. With clang-503.0.40 calling
    // BESDataHandlerInterface::make_copy() was inexplicably nulling the 'data'
    // map member of the DHI. This was happening because the two maps were the same
    // because the two DHI instances were the same - that is the 'rhs._dhi' field
    // and this' _dhi field were/are one and the same. I'm going to leave this
    // test here even though the code in BESDataHandlerInterface has been fixed to
    // test for this case - and new copy ctor and operator=() methods added.
    // jhrg 4/18/14
    if (&_dhi != &rhs._dhi) _dhi.make_copy(rhs._dhi);

    return *this;
}

DDSLoader::~DDSLoader()
{
    ensureClean();
}

#if 0
// Never used. 10/16/15 jhrg
unique_ptr<BESDapResponse> DDSLoader::load(const string& location, ResponseType type)
{
    // We need to make the proper response object as well, since in this call the dhi is coming in with the
    // response object for the original ncml request.
    std::unique_ptr<BESDapResponse> response = makeResponseForType(type);
    loadInto(location, type, response.get());
    return response; // relinquish
}
#endif

void DDSLoader::loadInto(const std::string& location, ResponseType type, BESDapResponse* pResponse)
{
    VALID_PTR(pResponse);
    VALID_PTR(_dhi.response_handler);

    // Just be sure we're cleaned up before doing anything, in case the caller calls load again after exception
    // and before dtor.
    ensureClean();

    _filename = location;

    // Remember current state of dhi before we touch it -- _hijacked is now true!!
    snapshotDHI();

#if 0
    BESContainer *container = nullptr;

    try {
        // Add a new symbol to the storage list and return container for it.
        // We will remove this new container on the way out.
        //
        // HK-474: If there is no handler configured to read the dataset (based on the values
        // of the various TypeMatch parameters), code nested inside this call with throw a
        // BESInternalError and the 'unwinding' of the containers will fail with some odd
        // ramifications, including segfaults and no message to the user/client. jhrg 11/13/19
        container = addNewContainerToStorage();
    }
    catch (BESError &e) {
        // Get rid of the container we added.
        removeContainerFromStorage();

        // Put back the dhi state we hijacked
        restoreDHI();

        throw e;
    }
#endif

    try {
        // Add a new symbol to the storage list and return container for it.
        // We will remove this new container on the way out.
        //
        // HK-474: If there is no handler configured to read the dataset (based on the values
        // of the various TypeMatch parameters), code nested inside this call with throw a
        // BESInternalError and the 'unwinding' of the containers will fail with some odd
        // ramifications, including segfaults and no message to the user/client. jhrg 11/13/19
        BESContainer *container = addNewContainerToStorage();

        // Take over the dhi
        // Container is allocated using ptr_duplicate. Must free existing container. See RestoreDHI. jhrg 6/19/19
        _dhi.container = container;
        _dhi.response_handler->set_response_object(pResponse);

        // Choose the proper request type...
        _dhi.action = getActionForType(type);
        _dhi.action_name = getActionNameForType(type);

        // Figure out which underlying type of response it is to get the DDS (or DataDDS via DDS super).
        DDS* pDDS = ncml_module::NCMLUtil::getDDSFromEitherResponse(pResponse);
        if (!pDDS) {
            THROW_NCML_INTERNAL_ERROR("DDSLoader::load expected BESDDSResponse or BESDataDDSResponse but got neither!");
        }
        pDDS->set_request_xml_base(pResponse->get_request_xml_base());

        // DO IT!

        BESDEBUG("ncml", "Before BESRequestHandlerList::TheList()->execute_current" << endl);
        BESDEBUG("ncml", "Handler name: " << BESRequestHandlerList::TheList()->get_handler_names() << endl);

        BESRequestHandlerList::TheList()->execute_current(_dhi);

        // Some NcML operations like rename/add attributes need to have attributes in the data access.
        // So we need to check if the attributes are added by the underneath handlers.
        // If not, it will be added here. KY 10/30/19
        if(type == eRT_RequestDataDDS) {

            BESResponseObject *response = _dhi.response_handler->get_response_object();
            BESDataDDSResponse *bdds = dynamic_cast<BESDataDDSResponse *> (response);
            if (!bdds)
                throw BESInternalError("cast error", __FILE__, __LINE__);

            if(bdds->get_ia_flag() == false) {
                BESDEBUG("ncml", "Underneath handler "<< _dhi.container->get_container_type() << " call add_attributes() " << endl);
                BESRequestHandler *besRH = BESRequestHandlerList::TheList()->find_handler(_dhi.container->get_container_type());
                besRH->add_attributes(_dhi);
            }
        }

        BESDEBUG("ncml", "After BESRequestHandlerList::TheList()->execute_current.\n");

        _filename = "";

        ensureClean();
    }
    catch (BESError &e) {
        // We should be clean here too.
        ensureClean();

        throw e;
    }
}

void DDSLoader::cleanup()
{
    ensureClean();
}

bool is_url(const std::string &location) {
    std::string http("http://");
    std::string https("https://");

    // case insensitive check
    std::string tip = location.substr(0,http.size());
    std::transform(tip.begin(), tip.end(), tip.begin(), ::tolower);
    bool result = http == tip; // http.compare(tip)==0;

    // case insensitive check
    tip = location.substr(0,https.size());
    std::transform(tip.begin(), tip.end(), tip.begin(), ::tolower);

    result = result || http == tip; //http.compare(tip)==0;

    return result;
}

BESContainer*
DDSLoader::addNewContainerToStorage()
{
    // Make sure we can find the storage
    BESContainerStorageList *store_list = BESContainerStorageList::TheList();
    VALID_PTR(store_list);

    // Check for URL in the _filename, 'cause that means gateway!
    BESContainerStorage* store;
    if(is_url(_filename)){
        BESDEBUG("ncml", __func__ << "() - GATEWAY CONTAINER!" << endl);
        store = store_list->find_persistence("gateway");
    }
    else {
        store = store_list->find_persistence("catalog");
    }
    if (!store) {
        throw BESInternalError("couldn't find the catalog storage", __FILE__, __LINE__);
    }

    // Make a new symbol from the ncml filename
    string newSymbol = getNextContainerName() + "__" + _filename;

    // this will throw an exception if the location isn't found in the
    // catalog. Might want to catch this. Wish the add returned the
    // container object created. Might want to change it.
    //
    // HK-474 This is the crux of the problem.
    store->add_container(newSymbol, _filename, "");

    // If we were successful, note the store location and symbol we added  for removal later.
    _store = store;
    _containerSymbol = newSymbol;

    // Now look up the symbol we added
    BESContainer *container = store->look_for(_containerSymbol);
    if (!container) {
        throw BESInternalError("couldn't find the container we just added:" + newSymbol, __FILE__, __LINE__);
    }

    return container;
}

void DDSLoader::removeContainerFromStorage()
{
    // If we have non-null _store, we added the container symbol,
    // so get rid of it
    if (_store) {
        try {
            // This should go through, but if there's an exception, we could unwind through the dtor,
            // so make sure we don't.
            _store->del_container(_containerSymbol);
        }
        catch (BESError& besErr) {
            ERROR_LOG("WARNING: tried to remove symbol " + _containerSymbol
                + " from singleton but unexpectedly it was not there.\n");
        }
        _containerSymbol = "";
        _store = 0;
    }
}

void DDSLoader::snapshotDHI()
{
    VALID_PTR(_dhi.response_handler);

    BESDEBUG( "ncml", "DDSLoader::snapshotDHI() - Taking snapshot of DataHAndlerInterface for (action: " << _dhi.action << " action_name: " << _dhi.action_name << ")" << endl );
    BESDEBUG( "ncml_verbose", "original dhi = " << _dhi << endl );

        // Store off the container for the original ncml file call and replace with the new one
    _origContainer = _dhi.container;
    _dhi.container = 0; // Mark the container so that we do not call release() more than once. jhrg 11/14/19
    _origAction = _dhi.action;
    _origActionName = _dhi.action_name;

    _origResponse = _dhi.response_handler->get_response_object();

    BESDEBUG( "ncml", "DDSLoader::snapshotDHI() - Replaced with DataHAndlerInterface for (action: " << _dhi.action << " action_name: " << _dhi.action_name << ")" << endl );

    _hijacked = true;
}

void DDSLoader::restoreDHI()
{
    VALID_PTR(_dhi.response_handler);

    // Make sure we have state before we go mucking
    if (!_hijacked) {
        return;
    }

    // Before we overwrite the 'high jacked' DHI's container, call
    // release(). If this is a simple file, it's no big deal to
    // skip this (because the file is closed elsewhere). But, if the
    // DHI is working with a _compressed_ file, it is a big deal
    // because this is the call that closes the cached uncompressed
    // file and frees the lock. This was the bug associated with
    // ticket HR-64. jhrg 10/16/15
    // Only release the container if it is not null. This was happening
    // because DDSLoader:loadInto() was failing in mid-process and the
    // DHI had been hijacked but the new container was not set. See HK-474.
    // jhrg 11/14/19
    if (_dhi.container) _dhi.container->release();
    // Leak. Allocated locally by addNewContainerToStorage() in loadInto(). jhrg 6/19/19
    delete _dhi.container;

    // Restore saved state
    _dhi.container = _origContainer;
    _dhi.action = _origAction;
    _dhi.action_name = _origActionName;

    _dhi.response_handler->set_response_object(_origResponse);

    BESDEBUG( "ncml", "DDSLoader::restoreDHI() - Restored of DataHAndlerInterface for (action: " << _dhi.action << " action_name: " << _dhi.action_name << ")" << endl );

    BESDEBUG( "ncml_verbose", "restored dhi = " << _dhi << endl );

        // clear our copy of saved state
    _origAction = "";
    _origActionName = "";
    _origResponse = 0;
    _origContainer = 0;
    _filename = "";

    _hijacked = false;
}

/**
 * @brief Use this to restore the hijacked DHI object.
 */
void DDSLoader::ensureClean()
{
    // If we're still hijacked here, there was an exception in load, so clean
    // up if needed.
    if (_hijacked) {
        restoreDHI();
    }

    // Make sure we've removed the new symbol from the container list as well.
    removeContainerFromStorage();
}

/* static */
std::string DDSLoader::getNextContainerName()
{
    static const string _sPrefix = "__DDSLoader_Container_ID_";
    _gensymID++;
    std::ostringstream oss;
    oss << _sPrefix << (_gensymID);
    return oss.str();
}

unique_ptr<BESDapResponse> DDSLoader::makeResponseForType(ResponseType type)
{
    if (type == eRT_RequestDDX) {
        // The BaseTypeFactory is leaked. jhrg 6/19/19
        return unique_ptr<BESDapResponse>(new BESDDSResponse(new DDS(nullptr /*new BaseTypeFactory()*/, "virtual")));
    }
    else if (type == eRT_RequestDataDDS) {
        // Leak fix jhrg 6/19/19
        return unique_ptr<BESDapResponse>(new BESDataDDSResponse(new DDS(nullptr /*new BaseTypeFactory()*/, "virtual")));
    }
    else {
        THROW_NCML_INTERNAL_ERROR("DDSLoader::makeResponseForType() got unknown type!");
    }
}

std::string DDSLoader::getActionForType(ResponseType type)
{
    if (type == eRT_RequestDDX) {
        return DDS_RESPONSE;
    }
    else if (type == eRT_RequestDataDDS) {
        return DATA_RESPONSE;
    }

    THROW_NCML_INTERNAL_ERROR("DDSLoader::getActionForType(): unknown type!");
}

std::string DDSLoader::getActionNameForType(ResponseType type)
{
    if (type == eRT_RequestDDX) {
        return DDX_RESPONSE_STR;
    }
    else if (type == eRT_RequestDataDDS) {
        return DATA_RESPONSE_STR;
    }

    THROW_NCML_INTERNAL_ERROR("DDSLoader::getActionNameForType(): unknown type!");
}

bool DDSLoader::checkResponseIsValidType(ResponseType type, BESDapResponse* pResponse)
{
    if (type == eRT_RequestDDX) {
        return dynamic_cast<BESDDSResponse*>(pResponse);
    }
    else if (type == eRT_RequestDataDDS) {
        return dynamic_cast<BESDataDDSResponse*>(pResponse);
    }
    else {
        return false;
    }
}

