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

#include "NgapRequestHandler.h"
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
 * @todo move to the header jhrg 9/20/23
 * @param sym_name symbolic name representing this remote container
 * @param real_name The NGAP restified path.
 * @throws BESSyntaxUserError if the url does not validate
 * @see NgapUtils
 */
NgapContainer::NgapContainer(const string &sym_name,
                             const string &real_name,
                             const string &) :
        BESContainer(sym_name, real_name, "ngap"), d_ngap_path(real_name) {
#if 0
    initialize();
#endif
}

void NgapContainer::_duplicate(NgapContainer &copy_to) {
    if (copy_to.d_dmrpp_rresource) {
        throw BESInternalError("The Container has already been accessed, cannot duplicate.", __FILE__, __LINE__);
    }
    copy_to.d_dmrpp_rresource = d_dmrpp_rresource;
    copy_to.d_ngap_path = d_ngap_path;
    BESContainer::_duplicate(copy_to);
}

BESContainer *
NgapContainer::ptr_duplicate() {
    auto container = new NgapContainer;
    _duplicate(*container);
    BESDEBUG(MODULE, prolog << "object address: "<< (void *) this << " to: " << (void *)container << endl);
    return container;
}

// TODO Move this to the header and set it as default;
NgapContainer::~NgapContainer() {
    BESDEBUG(MODULE, prolog << "BEGIN  object address: "<< (void *) this <<  endl);
#if 0
    if (d_dmrpp_rresource) {
        release();
    }
#endif
    BESDEBUG(MODULE, prolog << "END  object address: "<< (void *) this <<  endl);
}

/**
 * @brief Set the real name of the container using the CMR or cache.
 *
 * This uses CMR to translate a 'restified' path to a true NGAP URL for the granule.
 * Once this is done, the result is cached in an unordered_map keyed using the the
 * restified path and the UID. The restified path is initially the value of the
 * real_name property of the Container, but the action of performing the translation
 * changes the name of the real_name property to the true NGAP URL. The restified
 * path is stored in the d_ngap_path property of the NgapContainer instance.
 *
 * @note: The cache is global to the NgapRequestHandler class. This is a per-process
 * cache and it is not thread-safe.
 */
void NgapContainer::set_real_name_using_cmr_or_cache()
{
    BESDEBUG(MODULE, prolog << "BEGIN (obj_addr: "<< (void *) this << ")" << endl);
    BESDEBUG(MODULE, prolog << "sym_name: "<< get_symbolic_name() << endl);
    BESDEBUG(MODULE, prolog << "real_name: "<< get_real_name() << endl);
    BESDEBUG(MODULE, prolog << "type: "<< get_container_type() << endl);

#ifndef NDEBUG
    BESStopWatch besTimer;
    if (BESISDEBUG(MODULE) || BESISDEBUG(TIMING_LOG_KEY) || BESLog::TheLog()->is_verbose()) {
        besTimer.start(prolog + get_real_name());
    }
#endif

    bool found;
    string uid = BESContextManager::TheManager()->get_context(EDL_UID_KEY, found);
    BESDEBUG(MODULE, prolog << "EDL_UID_KEY(" << EDL_UID_KEY << "): " << uid << endl);

    string url_key = d_ngap_path + '.' + uid;
    if (NgapRequestHandler::d_use_cmr_cache
        && NgapRequestHandler::d_cmr_cache.find(url_key) != NgapRequestHandler::d_cmr_cache.end()) {
        set_real_name(NgapRequestHandler::d_cmr_cache[url_key]);
        set_relative_name(get_real_name());
        BESDEBUG(NGAP_CACHE, prolog << "Cache hit, translated URL: " << get_real_name() << endl);
        BESDEBUG(MODULE, prolog << "END (obj_addr: "<< (void *) this << ")" << endl);
        return;
    }

    NgapApi ngap_api;
    string data_access_url = ngap_api.convert_ngap_resty_path_to_data_access_url(get_real_name(), uid);
    set_real_name(data_access_url);

    // Because we know the name is really a URL, then we know the "relative_name" is meaningless
    // So we set it to be the same as "name"
    set_relative_name(get_real_name());

    if (NgapRequestHandler::d_use_cmr_cache) {
        NgapRequestHandler::d_cmr_cache[url_key] = get_real_name();
        BESDEBUG(NGAP_CACHE, prolog << "Cache miss, cached translated URL: " << get_real_name() << endl);
    }

    BESDEBUG(MODULE, prolog << "END (obj_addr: "<< (void *) this << ")" << endl);
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
void NgapContainer::filter_response(const map<string, string, std::less<string>> &content_filters) const {

    string resource_content = BESUtil::file_to_string(d_dmrpp_rresource->get_filename());

    for (const auto &apair: content_filters) {
        unsigned int replace_count = BESUtil::replace_all(resource_content, apair.first, apair.second);
        BESDEBUG(MODULE, prolog << "Replaced " << replace_count << " instance(s) of template(" <<
                                apair.first << ") with " << apair.second << " in cached RemoteResource" << endl);
    }

    // This call will invalidate the file descriptor of the RemoteResource. jhrg 3/9/23
    BESUtil::string_to_file(d_dmrpp_rresource->get_filename(), resource_content);
}

/**
 * Build the content filters if needed
 * @param content_filters Value-result parameter
 * @return True if the filters were built, false otherwise
 */
bool
NgapContainer::get_content_filters(map<string,string, std::less<string>> &content_filters) const
{
    if (inject_data_url()) {
        const string data_access_url_str = get_real_name();
        const string missing_data_url_str = data_access_url_str + ".missing";
        const string href=R"(href=")";
        const string trusted_url_hack=R"(" dmrpp:trust="true")";
        const string data_access_url_key = href + DATA_ACCESS_URL_KEY + "\"";
        const string data_access_url_with_trusted_attr_str = href + data_access_url_str + trusted_url_hack;
        const string missing_data_access_url_key = href + MISSING_DATA_ACCESS_URL_KEY + "\"";
        const string missing_data_url_with_trusted_attr_str = href + missing_data_url_str + trusted_url_hack;

        content_filters.insert(pair<string,string>(data_access_url_key, data_access_url_with_trusted_attr_str));
        content_filters.insert(pair<string,string>(missing_data_access_url_key, missing_data_url_with_trusted_attr_str));
        return true;
    }

    return false;
}

// Write a simple file --> string and string --> file set of functions.

void
NgapContainer::cache_dmrpp_contents(shared_ptr<http::RemoteResource> &d_dmrpp_rresource) {
    string resource_content = BESUtil::file_to_string(d_dmrpp_rresource->get_filename());
    NgapRequestHandler::d_dmrpp_cache[get_real_name()] = resource_content;
    set_attributes("cached");    // This means access() returns cache content and not a filename. hack. jhrg 9/22/23
    // TODO Added cache purge code here. jhrg 9/22/23
}

bool
NgapContainer::get_cached_dmrpp_string(string &dmrpp_string) const {
    if (NgapRequestHandler::d_dmrpp_cache.find(get_real_name()) != NgapRequestHandler::d_dmrpp_cache.end()) {
        dmrpp_string = NgapRequestHandler::d_dmrpp_cache[get_real_name()];
        return true;
    }
    return false;
}

bool
NgapContainer::is_dmrpp_cached() const {
    return NgapRequestHandler::d_dmrpp_cache.find(get_real_name()) != NgapRequestHandler::d_dmrpp_cache.end();
}

/**
 * @brief access the remote target response by making the remote request
 *
 * @note The Container::access() methods are called by the framework when it
 * runs execute_commands() and then, often, a second time in the RequestHandler
 * code when it is looking for data.
 *
 * @return full path to the remote request response data file
 * @throws BESError if there is a problem making the remote request
 */
string NgapContainer::access() {
    BESDEBUG(MODULE, prolog << "BEGIN  (obj_addr: "<< (void *) this << ")" << endl);

    set_real_name_using_cmr_or_cache();

#ifndef NDEBUG
    BESStopWatch besTimer;
    if (BESISDEBUG(MODULE) || BESDebug::IsSet(TIMING_LOG_KEY) || BESLog::TheLog()->is_verbose()) {
        besTimer.start("NGAP Container access: " + get_real_name());
    }
#endif

    if (is_dmrpp_cached()) {
        // set_container_type() because access() is called from within the framework and the DMR++ handler
        BESDEBUG(NGAP_CACHE, prolog << "Cache hit, translated URL: " << get_real_name() << endl);
        set_container_type("dmrpp");
        set_attributes("cached");
        string dmrpp_string;
        get_cached_dmrpp_string(dmrpp_string);
        return dmrpp_string;
    }

    BESDEBUG(NGAP_CACHE, prolog << "Cache miss, translated URL: " << get_real_name() << endl);
    if (!d_dmrpp_rresource) {
        // Assume the DMR++ is a sidecar file to the granule. jhrg 9/20/23
        string dmrpp_url_str = get_real_name() + ".dmrpp";
        auto dmrpp_url = make_shared<http::url>(dmrpp_url_str, true);
        {
            d_dmrpp_rresource = make_shared<http::RemoteResource>(dmrpp_url);
#ifndef NDEBUG
            BESStopWatch besTimer;
            if (BESISDEBUG(MODULE) || BESDebug::IsSet(TIMING_LOG_KEY) || BESLog::TheLog()->is_verbose()) {
                besTimer.start("DMR++ retrieval: "+ dmrpp_url->str());
            }
#endif
            d_dmrpp_rresource->retrieve_resource();
            // Substitute the data_access_url and missing_data_access_url in the dmr++ file.
            map<string,string, std::less<string>> content_filters;
            if (get_content_filters(content_filters))
                filter_response(content_filters);

            cache_dmrpp_contents(d_dmrpp_rresource);
        }
        BESDEBUG(MODULE, prolog << "Retrieved remote resource: " << dmrpp_url->str() << endl);
    }

    string dmrpp_file_name = d_dmrpp_rresource->get_filename();
    BESDEBUG(MODULE, prolog << "Using local temporary file: " << dmrpp_file_name << endl);

    set_container_type(d_dmrpp_rresource->get_type());

    BESDEBUG(MODULE, prolog << "Type: " << get_container_type() << endl);
    BESDEBUG(MODULE, prolog << "END  (obj_addr: "<< (void *) this << ")" << endl);

    return dmrpp_file_name;    // this should return the dmr++ file name from the NgapCache
}


/** @brief release the resources
 *
 * Release the resource
 *
 * @return true if the resource is released successfully and false otherwise
 */
bool NgapContainer::release() {
#if 0
    if (d_dmrpp_rresource) {
        BESDEBUG(MODULE, prolog << "Releasing RemoteResource" << endl);
        delete d_dmrpp_rresource;
        d_dmrpp_rresource = nullptr;
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