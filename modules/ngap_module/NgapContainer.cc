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
#include "HttpUtils.h"

#include "NgapRequestHandler.h"
#include "NgapContainer.h"
#include "NgapApi.h"
#include "NgapNames.h"

#define prolog std::string("NgapContainer::").append(__func__).append("() - ")

using namespace std;
using namespace bes;

namespace ngap {

void NgapContainer::_duplicate(NgapContainer &copy_to) {
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
#ifndef NDEBUG
    BESStopWatch besTimer;
    if (BESISDEBUG(MODULE) || BESISDEBUG(TIMING_LOG_KEY) || BESLog::TheLog()->is_verbose()) {
        besTimer.start(prolog + get_real_name());
    }
#endif

    bool found;
    string uid = BESContextManager::TheManager()->get_context(EDL_UID_KEY, found);
    BESDEBUG(MODULE, prolog << "EDL_UID_KEY(" << EDL_UID_KEY << "): " << uid << endl);

    // If using the cache, look there. Note that the UID is part of the key to the cached data.
    string url_key = d_ngap_path + ':' + uid;
    string real_name;
    if (NgapRequestHandler::d_use_cmr_cache) {
        if (NgapRequestHandler::d_cmr_mem_cache.get(url_key, real_name)) {
            set_real_name(real_name);
            set_relative_name(real_name);
            BESDEBUG(NGAP_CACHE, prolog << "CMR Cache hit, translated URL: " << get_real_name() << endl);
            BESDEBUG(MODULE, prolog << "END (obj_addr: " << (void *) this << ")" << endl);
            return;
        }
        else {
            BESDEBUG(NGAP_CACHE, prolog << "CMR Cache miss, URL: " << get_real_name() << endl);
        }
    }

    real_name = NgapApi::convert_ngap_resty_path_to_data_access_url(get_real_name());
    set_real_name(real_name);

    // Because we know the name is really a URL, then we know the "relative_name" is meaningless
    // So we set it to be the same as "name"
    set_relative_name(real_name);

    // If using the CMR cache, cache the response.
    if (NgapRequestHandler::d_use_cmr_cache) {
        NgapRequestHandler::d_cmr_mem_cache.put(url_key, real_name);
        BESDEBUG(NGAP_CACHE, prolog << "CMR Cache, cached translated URL: " << get_real_name() << endl);
    }
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
 * @param content A reference to the C++ string to filter
 */
void NgapContainer::filter_response(const map<string, string, std::less<>> &content_filters, string &content) const {
    for (const auto &apair: content_filters) {
        unsigned int replace_count = BESUtil::replace_all(content, apair.first, apair.second);
        BESDEBUG(MODULE, prolog << "Replaced " << replace_count << " instance(s) of template(" <<
                                apair.first << ") with " << apair.second << " in cached RemoteResource" << endl);
    }
}

/**
 * Build the content filters if needed
 * @note If the filters are built, clear the content_filters value/result parameter first.
 * @param content_filters Value-result parameter
 * @return True if the filters were built, false otherwise
 */
bool
NgapContainer::get_content_filters(map<string, string, std::less<>> &content_filters) const
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

        content_filters.clear();
        content_filters.insert(pair<string,string>(data_access_url_key, data_access_url_with_trusted_attr_str));
        content_filters.insert(pair<string,string>(missing_data_access_url_key, missing_data_url_with_trusted_attr_str));
        return true;
    }

    return false;
}

static bool file_to_string(int fd, string &content) {
    vector<char> buffer(4096);
    ssize_t bytes_read;
    while ((bytes_read = read(fd, buffer.data(), buffer.size())) > 0) {
        content.append(buffer.data(), bytes_read);
    }
    return bytes_read == 0;
}

/**
 * @brief Should the server inject the data URL into DMR++ documents?
 *
 * @return True if the NGAP_INJECT_DATA_URL_KEY key indicates that the
 * code should inject the data URL, false otherwise.
 */
bool NgapContainer::inject_data_url() {
    bool result = TheBESKeys::TheKeys()->read_bool_key(NGAP_INJECT_DATA_URL_KEY, false);
    BESDEBUG(MODULE, prolog << "NGAP_INJECT_DATA_URL_KEY(" << NGAP_INJECT_DATA_URL_KEY << "): " << result << endl);
    return result;
}

/**
 * @brief Get the DMR++ from a remote source or a cache
 * @param dmrpp_string Value-result parameter that will contain the DMR++ as a string
 * @return True if the DMR++ was found and no caching issues were encountered, false if there
 * was a failure of the caching system. Error messages are logged if there is a caching issue.
 * @exception BESError if there is a problem making the remote request if one is needed.
 */
bool NgapContainer::get_dmrpp_from_cache_or_remote_source(string &dmrpp_string) const {
#ifndef NDEBUG
    BESStopWatch besTimer;
    if (BESISDEBUG(MODULE) || BESISDEBUG(TIMING_LOG_KEY) || BESLog::TheLog()->is_verbose()) {
        besTimer.start("NGAP Container access: " + get_real_name());
    }
#endif

    if (NgapRequestHandler::d_use_dmrpp_cache) {
        if (NgapRequestHandler::d_dmrpp_mem_cache.get(get_real_name(), dmrpp_string)) {
            // set_container_type() because access() is called from both the framework and the DMR++ handler
            BESDEBUG(NGAP_CACHE, prolog << "Memory Cache hit, DMR++: " << get_real_name() << endl);
            return true;
        }
        else {
            BESDEBUG(NGAP_CACHE, prolog << "Memory Cache miss, DMR++: " << get_real_name() << endl);
        }
    }

    // Before going over the network to get the DMR++, look in the FileCache.
    // If found, put it in the memory cache and return it as a string.
    if (NgapRequestHandler::d_use_dmrpp_cache) {
        FileCache::Item item;
        if (NgapRequestHandler::d_dmrpp_file_cache.get(FileCache::hash_key(get_real_name()), item)) { // got it
            // read data from the file into the string.
            BESDEBUG(NGAP_CACHE, prolog << "File Cache hit, Memory Cache miss, DMR++: " << get_real_name() << endl);
            if (file_to_string(item.get_fd(), dmrpp_string)) {
                // put it in the memory cache
                NgapRequestHandler::d_dmrpp_mem_cache.put(get_real_name(), dmrpp_string);
                return true;
            }
            else {
                ERROR_LOG("NgapContainer::access() - failed to read DMR++ from file cache");
                return false;
            }
        }
        else {
            BESDEBUG(NGAP_CACHE, prolog << "File Cache miss, DMR++: " << get_real_name() << endl);
        }
    }   // end of FileCache::Item scope

    string dmrpp_url_str = get_real_name() + ".dmrpp";  // The URL of the DMR++ file

    // Else, the DMR++ is neither in the memory cache nor the file cache.
    // Read it from S3, etc., and filter it. Put it in the memory cache.
#ifndef NDEBUG
    BESStopWatch besTimer2;
    if (BESISDEBUG(MODULE) || BESISDEBUG(TIMING_LOG_KEY) || BESLog::TheLog()->is_verbose()) {
        besTimer2.start("DMR++ retrieval: " + dmrpp_url_str);
    }
#endif

    // This code throws an exception if there is a problem. jhrg 11/16/23
    curl::http_get(dmrpp_url_str, dmrpp_string);
#if 0
    vector<char> buffer;
    curl::http_get(dmrpp_url_str, buffer);
    copy(buffer.begin(), buffer.end(), back_inserter(dmrpp_string));
    buffer.clear(); // keep the original for as little time as possible.
#endif

    map<string,string, std::less<>> content_filters;
    if (get_content_filters(content_filters)) {
        filter_response(content_filters, dmrpp_string);
    }

    // if we get here, the DMR++ has been pulled over the network. Put it in both caches.
    // The memory cache is for use by this process, the file cache for other processes/VMs
    if (NgapRequestHandler::d_use_dmrpp_cache) {
        NgapRequestHandler::d_dmrpp_mem_cache.put(get_real_name(), dmrpp_string);
        BESDEBUG(NGAP_CACHE, prolog << "Memory Cache, cached DMR++: " << get_real_name() << endl);

        FileCache::PutItem item(NgapRequestHandler::d_dmrpp_file_cache);
        if (NgapRequestHandler::d_dmrpp_file_cache.put(FileCache::hash_key(get_real_name()), item)) {
            // TODO do this in a child thread someday. jhrg 11/14/23
            if (write(item.get_fd(), dmrpp_string.data(), dmrpp_string.size()) != dmrpp_string.size()) {
                ERROR_LOG("NgapContainer::access() - failed to write DMR++ to file cache");
                return false;
            }
            BESDEBUG(NGAP_CACHE, prolog << "File Cache, cached DMR++: " << get_real_name() << endl);
        }
        else {
            ERROR_LOG("NgapContainer::access() - failed to put DMR++ in file cache");
            return false;
        }
    }

    return true;
}

/**
 * @brief Get the DMR++ from a remote source or a local cache
 *
 * @note The Container::access() methods are called by the framework when it
 * runs execute_commands() and then, often, a second time in the RequestHandler
 * code when it is looking for data.
 *
 * @return The DMR++ as a string.
 * @throws BESError if there is a problem making the remote request
 */
string NgapContainer::access() {
    set_real_name_using_cmr_or_cache();

    string dmrpp_string;
    get_dmrpp_from_cache_or_remote_source(dmrpp_string);

    set_attributes("as-string");    // This means access() returns a string. jhrg 10/19/23
    // Originally, this was either hard-coded (as it is now) or was set using the 'extension'
    // on the URL. But it's always a DMR++. jhrg 11/16/23
    set_container_type("dmrpp");
    return dmrpp_string;
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
    BESIndent::UnIndent();
}

} // namespace ngap