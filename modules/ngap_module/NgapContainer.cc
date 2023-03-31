// NgapContainer.cc

// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of ngap_module, A C++ module that can be loaded in to
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
// Authors:
//      ndp       Nathan Potter <ndp@opendap.org>

#include "config.h"

#include <map>
#include <sstream>
#include <string>

#include "BESStopWatch.h"
#include "BESLog.h"
#include "BESSyntaxUserError.h"
#include "BESInternalError.h"
#include "BESDebug.h"
#include "TheBESKeys.h"
#include "BESUtil.h"
#include "BESContextManager.h"
#include "CurlUtils.h"
#include "RemoteResource.h"

#include "NgapContainer.h"
#include "NgapApi.h"
#include "NgapNames.h"

#define prolog std::string("NgapContainer::").append(__func__).append("() - ")

using namespace std;
using namespace bes;

namespace ngap {

/**
 * @brief Creates an instances of NgapContainer with symbolic name and real
 * name, which is the remote request.
 *
 * The real_name is the remote request URL.
 *
 * @param sym_name symbolic name representing this remote container
 * @param real_name The NGAP restified path.
 * @throws BESSyntaxUserError if the url does not validate
 * @see NgapUtils
 */
NgapContainer::NgapContainer(const string &sym_name,
                             const string &real_name,
                             const string &type) :
        BESContainer(sym_name, real_name, type) {
    initialize();
}

void NgapContainer::initialize()
{
    BESDEBUG(MODULE, prolog << "BEGIN (obj_addr: "<< (void *) this << ")" << endl);
    BESDEBUG(MODULE, prolog << "sym_name: "<< get_symbolic_name() << endl);
    BESDEBUG(MODULE, prolog << "real_name: "<< get_real_name() << endl);
    BESDEBUG(MODULE, prolog << "type: "<< get_container_type() << endl);

    if (get_container_type().empty())
        set_container_type("ngap");

    bool found;
    string uid = BESContextManager::TheManager()->get_context(EDL_UID_KEY, found);
    BESDEBUG(MODULE, prolog << "EDL_UID_KEY(" << EDL_UID_KEY << "): " << uid << endl);

    NgapApi ngap_api;
    string data_access_url = ngap_api.convert_ngap_resty_path_to_data_access_url(get_real_name(), uid);

    set_real_name(data_access_url);

    // Because we know the name is really a URL, then we know the "relative_name" is meaningless
    // So we set it to be the same as "name"
    set_relative_name(data_access_url);

    BESDEBUG(MODULE, prolog << "END (obj_addr: "<< (void *) this << ")" << endl);
}

void NgapContainer::_duplicate(NgapContainer &copy_to) {
    if (copy_to.d_dmrpp_rresource) {
        throw BESInternalError("The Container has already been accessed, cannot duplicate.", __FILE__, __LINE__);
    }
    copy_to.d_dmrpp_rresource = d_dmrpp_rresource;
    BESContainer::_duplicate(copy_to);
}

BESContainer *
NgapContainer::ptr_duplicate() {
    auto container = new NgapContainer;
    _duplicate(*container);
    BESDEBUG(MODULE, prolog << "object address: "<< (void *) this << " to: " << (void *)container << endl);
    return container;
}

NgapContainer::~NgapContainer() {
    BESDEBUG(MODULE, prolog << "BEGIN  object address: "<< (void *) this <<  endl);
    if (d_dmrpp_rresource) {
        release();
    }
    BESDEBUG(MODULE, prolog << "END  object address: "<< (void *) this <<  endl);
}

/**
 * @brief Filter the cached resource. Each key in content_filters is replaced with its associated map value.
 *
 * WARNING: Does not lock cache. This method assumes that the process has already
 * acquired an exclusive lock on the cache file.
 *
 * WARNING: This method will overwrite the cached data with the filtered result.
 *
 * @param content_filters A map of key value pairs which define the filter operation. Each key found in the
 * resource will be replaced with its associated value.
 */
void NgapContainer::filter_response(const map<string, string> &content_filters) const {

    string resource_content = BESUtil::file_to_string(d_dmrpp_rresource->get_filename());

    for (const auto &apair: content_filters) {
        unsigned int replace_count = BESUtil::replace_all(resource_content, apair.first, apair.second);
        BESDEBUG(MODULE, prolog << "Replaced " << replace_count << " instance(s) of template(" <<
                                apair.first << ") with " << apair.second << " in cached RemoteResource" << endl);
    }

    BESUtil::string_to_file(d_dmrpp_rresource->get_filename(), resource_content);
}

/**
 * @brief access the remote target response by making the remote request
 *
 * @return full path to the remote request response data file
 * @throws BESError if there is a problem making the remote request
 */
string NgapContainer::access() {
    BESDEBUG(MODULE, prolog << "BEGIN  (obj_addr: "<< (void *) this << ")" << endl);

    // Since this the ngap we know that the real_name is a URL.
    string data_access_url_str = get_real_name();

    // And we know that the dmr++ file should "right next to it" (side-car)
    string dmrpp_url_str = data_access_url_str + ".dmrpp";

    // And if there's a missing data file (side-car) it should be "right there" too.
    string missing_data_url_str = data_access_url_str + ".missing";

    BESDEBUG(MODULE, prolog << " data_access_url: " << data_access_url_str << endl);
    BESDEBUG(MODULE, prolog << "       dmrpp_url: " << dmrpp_url_str << endl);
    BESDEBUG(MODULE, prolog << "missing_data_url: " << missing_data_url_str << endl);

    string href=R"(href=")";
    string trusted_url_hack=R"( dmrpp:trust="true")";

    string data_access_url_key = href + DATA_ACCESS_URL_KEY + "\"";
    BESDEBUG(MODULE, prolog << "                   data_access_url_key: " << data_access_url_key << endl);

    string data_access_url_with_trusted_attr_str = href + data_access_url_str + trusted_url_hack;
    BESDEBUG(MODULE, prolog << " data_access_url_with_trusted_attr_str: " << data_access_url_with_trusted_attr_str << endl);

    string missing_data_access_url_key = href + MISSING_DATA_ACCESS_URL_KEY + "\"";
    BESDEBUG(MODULE, prolog << "           missing_data_access_url_key: " << missing_data_access_url_key << endl);

    string missing_data_url_with_trusted_attr_str = href + missing_data_url_str + trusted_url_hack;
    BESDEBUG(MODULE, prolog << "missing_data_url_with_trusted_attr_str: " << missing_data_url_with_trusted_attr_str << endl);

    string type = get_container_type();
    if (type == "ngap")
        type = "";

    if (!d_dmrpp_rresource) {
        BESDEBUG(MODULE, prolog << "Building new RemoteResource (dmr++)." << endl);
        map<string,string> content_filters;
        if (inject_data_url()) {
            content_filters.insert(pair<string,string>(data_access_url_key, data_access_url_with_trusted_attr_str));
            content_filters.insert(pair<string,string>(missing_data_access_url_key, missing_data_url_with_trusted_attr_str));
        }
        auto dmrpp_url = make_shared<http::url>(dmrpp_url_str, true);
        {
            // TODO unique_ptr. Needs work in the release() method, too. jhrg 3/9/23
            d_dmrpp_rresource = new http::RemoteResource(dmrpp_url);
#ifndef NDEBUG
            BESStopWatch besTimer;
            if (BESISDEBUG(MODULE) || BESDebug::IsSet(TIMING_LOG_KEY) || BESLog::TheLog()->is_verbose()){
                besTimer.start("DMR++ retrieval: "+ dmrpp_url->str());
            }
#endif
            d_dmrpp_rresource->retrieve_resource();
            // Substitute the data_access_url and missing_data_access_url in the dmr++ file.
            filter_response(content_filters);
        }
        BESDEBUG(MODULE, prolog << "Retrieved remote resource: " << dmrpp_url->str() << endl);
    }

    string cachedResource = d_dmrpp_rresource->get_filename();
    BESDEBUG(MODULE, prolog << "Using local cache file: " << cachedResource << endl);

    type = d_dmrpp_rresource->get_type();
    set_container_type(type);
    BESDEBUG(MODULE, prolog << "Type: " << type << endl);
    BESDEBUG(MODULE, prolog << "Done retrieving:  " << dmrpp_url_str << " returning cached file " << cachedResource << endl);
    BESDEBUG(MODULE, prolog << "END  (obj_addr: "<< (void *) this << ")" << endl);

    return cachedResource;    // this should return the dmr++ file name from the NgapCache
}


/** @brief release the resources
 *
 * Release the resource
 *
 * @return true if the resource is released successfully and false otherwise
 */
bool NgapContainer::release() {
    if (d_dmrpp_rresource) {
        BESDEBUG(MODULE, prolog << "Releasing RemoteResource" << endl);
        delete d_dmrpp_rresource;
        d_dmrpp_rresource = nullptr;
    }

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
void NgapContainer::dump(ostream &strm) const {
    strm << BESIndent::LMarg << "NgapContainer::dump - (" << (void *) this
         << ")" << endl;
    BESIndent::Indent();
    BESContainer::dump(strm);
    if (d_dmrpp_rresource) {
        strm << BESIndent::LMarg << "RemoteResource.get_filename(): " << d_dmrpp_rresource->get_filename()
             << endl;
    } else {
        strm << BESIndent::LMarg << "response not yet obtained" << endl;
    }
    BESIndent::UnIndent();
}

/**
 * @brief Should the server inject the data URL into DMR++ documents?
 *
 * @return True if the NGAP_INJECT_DATA_URL_KEY key indicates that the
 * code should inject the data URL, false otherwise.
 */
bool NgapContainer::inject_data_url() {
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

} // namespace ngap