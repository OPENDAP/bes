// NgapBuildDmrppContainer.cc

// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of builddmrpp_module, A C++ module that can be loaded in to
// the OPeNDAP Back-End Server (BES) and is able to handle remote requests.

// Copyright (c) 2023 OPeNDAP, Inc.
// Authors: Daniel Holloway <dholloway@opendap.org>
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
// Authors:
//      dan       Daniel Holloway <dholloway@opendap.org>
//      ndp       Nathan Potter <ndp@opendap.org>

#include "config.h"

#include <map>
#include <memory>
#include <sstream>
#include <string>

#include "BESStopWatch.h"
#include "BESLog.h"
#include "BESSyntaxUserError.h"
#include "BESInternalError.h"
#include "BESDebug.h"
#include "TheBESKeys.h"
#include "BESContextManager.h"
#include "CurlUtils.h"
#include "RemoteResource.h"
#include "url_impl.h"

#include "NgapApi.h"
#include "NgapNames.h"
#include "NgapBuildDmrppContainer.h"

#define prolog std::string("NgapBuildDmrppContainer::").append(__func__).append("() - ")

using namespace std;
using namespace bes;
using namespace ngap;

namespace builddmrpp {

/** @brief Creates an instances of NgapBuildDmrppContainer with symbolic name and real
 * name, which is the remote request.
 *
 * The real_name is the remote request URL.
 *
 * @param sym_name symbolic name representing this remote container
 * @param real_name The NGAP restified path.
 * @throws BESSyntaxUserError if the url does not validate
 * @see NgapUtils
 */
NgapBuildDmrppContainer::NgapBuildDmrppContainer(const string &sym_name, const string &real_name,  const string &type) :
        BESContainer(sym_name, real_name, type) {
    initialize();
}


void NgapBuildDmrppContainer::initialize()
{
    BESDEBUG(MODULE, prolog << "BEGIN (obj_addr: "<< (void *) this << ")" << endl);
    BESDEBUG(MODULE, prolog << "sym_name: "<< get_symbolic_name() << endl);
    BESDEBUG(MODULE, prolog << "real_name: "<< get_real_name() << endl);
    BESDEBUG(MODULE, prolog << "type: "<< get_container_type() << endl);

    bool found;

    ngap::NgapApi ngap_api;

    string uid = BESContextManager::TheManager()->get_context(EDL_UID_KEY, found);
    BESDEBUG(MODULE, prolog << "EDL_UID_KEY(" << EDL_UID_KEY << "): " << uid << endl);

    string data_access_url = ngap_api.convert_ngap_resty_path_to_data_access_url(get_real_name(), uid);

    set_real_name(data_access_url);

    BESDEBUG(MODULE, prolog << "END (obj_addr: "<< (void *) this << ")" << endl);

}

/**
 * TODO: I think this implementation of the copy constructor is incomplete/inadequate. Review and fix as needed.
 */
NgapBuildDmrppContainer::NgapBuildDmrppContainer(const NgapBuildDmrppContainer &copy_from) :
        BESContainer(copy_from),
        d_data_rresource(copy_from.d_data_rresource) {
    BESDEBUG(MODULE, prolog << "BEGIN   object address: "<< (void *) this << " Copying from: " << (void *) &copy_from << endl);
    // we can not make a copy of this container once the request has
    // been made
    if (d_data_rresource) {
        string err = (string) "The Container has already been accessed, "
                     + "can not create a copy of this container.";
        throw BESInternalError(err, __FILE__, __LINE__);
    }
    BESDEBUG(MODULE, prolog << "object address: "<< (void *) this << endl);
}

/**
 * @brief Duplicate the contents of this instance into 'copy_to.'
 * @param copy_to
 */
void NgapBuildDmrppContainer::_duplicate(NgapBuildDmrppContainer &copy_to) {
    if (copy_to.d_data_rresource) {
        throw BESInternalError("The Container has already been accessed, cannot duplicate this resource.", __FILE__, __LINE__);
    }
    BESDEBUG(MODULE, prolog << "BEGIN   object address: "<< (void *) this << " Copying to: " << (void *) &copy_to << endl);

    copy_to.d_real_name = d_real_name;
    copy_to.d_data_rresource = d_data_rresource;

    BESContainer::_duplicate(copy_to);
}

BESContainer *
NgapBuildDmrppContainer::ptr_duplicate() {
    NgapBuildDmrppContainer *container = new NgapBuildDmrppContainer;
    _duplicate(*container);
    BESDEBUG(MODULE, prolog << "object address: "<< (void *) this << " to: " << (void *)container << endl);
    return container;
}

/**
 * TODO: I think this implementation of the destructor is incomplete/inadequate. Review and fix as needed.
 * TODO: The release() function is removed due to an inconsistency with the how this container is copied and used.
 */
NgapBuildDmrppContainer::~NgapBuildDmrppContainer() {
    BESDEBUG(MODULE, prolog << "BEGIN  object address: "<< (void *) this <<  endl);
#if 0
    if (d_data_rresource) {
        release();
    }
#endif
    BESDEBUG(MODULE, prolog << "END  object address: "<< (void *) this <<  endl);
}


/** @brief access the remote target response by making the remote request
 *
 * @return full path to the remote request response data file
 * @throws BESError if there is a problem making the remote request
 */
string NgapBuildDmrppContainer::access() {
    BESDEBUG(MODULE, prolog << "BEGIN  (obj_addr: "<< (void *) this << ")" << endl);

    // Since this the ngap we know that the real_name is a URL.
    string data_access_url_str = get_real_name();

    BESDEBUG(MODULE, prolog << " data_access_url: " << data_access_url_str << endl);

    string href="href=\"";
    string trusted_url_hack="\" dmrpp:trust=\"true\"";

    string data_access_url_key = href + DATA_ACCESS_URL_KEY + "\"";
    BESDEBUG(MODULE, prolog << "                   data_access_url_key: " << data_access_url_key << endl);

    string data_access_url_with_trusted_attr_str = href + data_access_url_str + trusted_url_hack;
    BESDEBUG(MODULE, prolog << " data_access_url_with_trusted_attr_str: " << data_access_url_with_trusted_attr_str << endl);

 /*   string missing_data_access_url_key = href + MISSING_DATA_ACCESS_URL_KEY + "\"";
    BESDEBUG(MODULE, prolog << "           missing_data_access_url_key: " << missing_data_access_url_key << endl);

    string missing_data_url_with_trusted_attr_str = href + missing_data_url_str + trusted_url_hack;
    BESDEBUG(MODULE, prolog << "missing_data_url_with_trusted_attr_str: " << missing_data_url_with_trusted_attr_str << endl);*/

    if (!d_data_rresource) {
        BESDEBUG(MODULE, prolog << "Building new RemoteResource (dmr++)." << endl);
        map<string, string> content_filters;
        /*if (inject_data_url()) {
            content_filters.insert(
                    pair<string, string>(data_access_url_key, data_access_url_with_trusted_attr_str));
            *//*content_filters.insert(
                    pair<string, string>(missing_data_access_url_key, missing_data_url_with_trusted_attr_str));*//*
        }*/
        auto data_url(std::make_shared<http::url>(data_access_url_str, true));
        {
            d_data_rresource = std::make_shared<http::RemoteResource>(data_url);
#ifndef NDEBUG
            BESStopWatch besTimer;
            if (BESISDEBUG(MODULE) || BESDebug::IsSet(TIMING_LOG_KEY) || BESLog::TheLog()->is_verbose()) {
                besTimer.start("DMR++ retrieval: " + data_url->str());
            }
#endif
            d_data_rresource->retrieve_resource();
        }
        BESDEBUG(MODULE, prolog << "Retrieved remote resource: " << data_url->str() << endl);
    }

    // TODO This file should be read locked before leaving this method.
    //  10/8/21 I think the RemoteResource should do that. jhrg
    string cachedResource = d_data_rresource->get_filename();
    BESDEBUG(MODULE, prolog << "Using local cache file: " << cachedResource << endl);
    BESDEBUG(MODULE, prolog << "Done retrieving:  " << data_access_url_str << " returning cached file " << cachedResource << endl);
    BESDEBUG(MODULE, prolog << "END  (obj_addr: "<< (void *) this << ")" << endl);

    return cachedResource;    // this should return the dmr++ file name from the NgapCache
}


/** @brief release the resources
 *
 * Release the resource
 *
 * @return true if the resource is released successfully and false otherwise
 */
bool NgapBuildDmrppContainer::release() {
    // TODO The cache file (that will be) read locked in the access() method must be unlocked here.
    //  If we make that part of the RemoteResource dtor, the unlock will happen here. jhrg
#if 0
    if (d_data_rresource) {
        BESDEBUG(MODULE, prolog << "Releasing RemoteResource" << endl);
        delete d_data_rresource;
        d_data_rresource = nullptr;
    }
#endif

    BESDEBUG(MODULE, prolog << "Done releasing Ngap response" << endl);
    return true;
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance along with information about
 * this container.
 *
 * @param strm C++ i/o stream to dump the information to
 */
void NgapBuildDmrppContainer::dump(ostream &strm) const {
    strm << BESIndent::LMarg << "NgapBuildDmrppContainer::dump - (" << (void *) this
         << ")" << endl;
    BESIndent::Indent();
    BESContainer::dump(strm);
    if (d_data_rresource) {
        //strm << BESIndent::LMarg << "RemoteResource.getCacheFileName(): " << d_data_rresource->getCacheFileName()
        strm << BESIndent::LMarg << "RemoteResource.getCacheFileName(): " << d_data_rresource->get_filename()
             << endl;
    } else {
        strm << BESIndent::LMarg << "response not yet obtained" << endl;
    }
    BESIndent::UnIndent();
}

bool NgapBuildDmrppContainer::inject_data_url(){
    bool result = false;
    bool found;
    string key_value;
    TheBESKeys::TheKeys()->get_value(NGAP_INJECT_DATA_URL_KEY, key_value, found);
    if (found && key_value == "true") {
        result = true;
    }
    BESDEBUG(MODULE, prolog << "NGAP_INJECT_DATA_URL_KEY(" << NGAP_INJECT_DATA_URL_KEY << "): " << result << endl);
    return result;
}

}   // namespace builddmrpp
