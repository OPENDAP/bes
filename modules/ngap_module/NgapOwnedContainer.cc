// NgapOwnedContainer.cc

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
#include "BESDebug.h"
#include "TheBESKeys.h"
#include "BESUtil.h"
#include "BESContextManager.h"
#include "CurlUtils.h"
#include "HttpError.h"

#include "NgapRequestHandler.h"
#include "NgapOwnedContainer.h"
#include "NgapApi.h"
#include "NgapNames.h"

auto OPENDAP_BUCKET_NAME = "dmrpp-sit-poc"; // FIXME: This is a placeholder. jhrg 4/29/24

#define prolog std::string("NgapOwnedContainer::").append(__func__).append("() - ")
// CACHE_LOG is defined separately from INFO_LOG so that we can turn it off easily. jhrg 11/19/23
#define CACHE_LOG(x) MR_LOG("info", x)

#ifndef NDEBUG
#define BES_STOPWATCH_START(x) \
do { \
BESStopWatch besTimer; \
if (BESISDEBUG(MODULE) || BESISDEBUG(TIMING_LOG_KEY) || BESLog::TheLog()->is_verbose()) \
    besTimer.start((x)); \
} while(false)
#else
#define BES_STOPWATCH_START(x)
#endif

using namespace std;
using namespace bes;

namespace ngap {

bool NgapOwnedContainer::file_to_string(int fd, string &content) {
    vector<char> buffer(4096);
    ssize_t bytes_read;
    while ((bytes_read = read(fd, buffer.data(), buffer.size())) > 0) {
        content.append(buffer.data(), bytes_read);
    }
    return bytes_read == 0;
}

string NgapOwnedContainer::build_dmrpp_url_to_owned_bucket(const string &rest_path) {
    // The PATH part of a URL to the NGAP/DMR++ is an 'NGAP REST path' that has the form:
    // /collections/<ccid>/granules/<granule_id>. In our 'owned' S3 bucket, we use object
    // names of the form: /<ccid>/<granule_id>.dmrpp.

    auto parts =  BESUtil::split(rest_path, '/');
    if (parts.size() != 4 || parts[0] != "collections" || parts[2] != "granules") {
        throw BESSyntaxUserError("Invalid NGAP path: " + rest_path, __FILE__, __LINE__);
    }

    string s3_object = parts[1] + '/' + parts[3] + ".dmrpp";

    // http://<bucket_name>.s3.amazonaws.com/<object_key>
    string dmrpp_url_str = "https://" + string(OPENDAP_BUCKET_NAME) + ".s3.amazonaws.com/" + s3_object;

    return dmrpp_url_str;
}

bool NgapOwnedContainer::get_item_from_cache(string &dmrpp_string) const {

    // Read the cache entry if it exists. jhrg 4/29/24
    if (NgapRequestHandler::d_dmrpp_mem_cache.get(get_real_name(), dmrpp_string)) {
        CACHE_LOG(prolog + "Memory Cache hit, DMR++: " + get_real_name() + '\n');
        return true;
    }
    else {
        CACHE_LOG(prolog + "Memory Cache miss, DMR++: " + get_real_name() + '\n');
    }

    // Before going over the network to get the DMR++, look in the FileCache.
    // If found, put it in the memory cache and return it as a string.

    FileCache::Item item;
    if (NgapRequestHandler::d_dmrpp_file_cache.get(FileCache::hash_key(get_real_name()), item)) { // got it
        // read data from the file into the string.
        CACHE_LOG(prolog + "File Cache hit, DMR++: " + get_real_name() + '\n');
        if (file_to_string(item.get_fd(), dmrpp_string)) {
            // put it in the memory cache
            NgapRequestHandler::d_dmrpp_mem_cache.put(get_real_name(), dmrpp_string);
            CACHE_LOG(prolog + "Memory Cache put, DMR++: " + get_real_name() + '\n');
            return true;
        }
        else {
            ERROR_LOG("NgapOwnedContainer::access() - failed to read DMR++ from file cache\n");
            return false;
        }
    }
    else {
        CACHE_LOG(prolog + "File Cache miss, DMR++: " + get_real_name() + '\n');
    }

    return false;
}

bool NgapOwnedContainer::put_item_in_cache(const std::string &dmrpp_string) const {

    FileCache::PutItem item(NgapRequestHandler::d_dmrpp_file_cache);
    if (NgapRequestHandler::d_dmrpp_file_cache.put(FileCache::hash_key(get_real_name()), item)) {
        // Do this in a child thread someday. jhrg 11/14/23
        if (write(item.get_fd(), dmrpp_string.data(), dmrpp_string.size()) != dmrpp_string.size()) {
            ERROR_LOG("NgapOwnedContainer::access() - failed to write DMR++ to file cache\n");
            return false;
        }
        CACHE_LOG(prolog + "File Cache put, DMR++: " + get_real_name() + '\n');
    }
    else {
        ERROR_LOG("NgapOwnedContainer::access() - failed to put DMR++ in file cache\n");
        return false;
    }

    if (!NgapRequestHandler::d_dmrpp_file_cache.purge()) {
        ERROR_LOG("NgapOwnedContainer::access() - call to FileCache::purge() failed\n");
    }

    NgapRequestHandler::d_dmrpp_mem_cache.put(get_real_name(), dmrpp_string);
    CACHE_LOG(prolog + "Memory Cache put, DMR++: " + get_real_name() + '\n');

    return true;
}

/**
 * @brief Get the DMR++ from a remote source or a cache
 * @param dmrpp_string Value-result parameter that will contain the DMR++ as a string
 * @return True if the DMR++ was found and no caching issues were encountered, false if there
 * was a failure of the caching system. Error messages are logged if there is a caching issue.
 * @exception BESError if there is a problem making the remote request if one is needed.
 */
bool NgapOwnedContainer::get_dmrpp_from_cache_or_remote_source(string &dmrpp_string) const {
    BES_STOPWATCH_START(prolog + get_real_name());

    // If the DMR++ is cached, return it.
    if (NgapRequestHandler::d_use_dmrpp_cache && get_item_from_cache(dmrpp_string)) {
        return true;
    }

    // Else, the DMR++ is neither in the memory cache nor the file cache.
    // Read it from S3, etc., and filter it. Put it in the memory cache.

    string dmrpp_url_str = build_dmrpp_url_to_owned_bucket(d_ngap_path);

    BES_STOPWATCH_START("DMR++ retrieval: " + dmrpp_url_str);

    try {
        // This code throws an exception if there is a problem. jhrg 11/16/23
        curl::http_get(dmrpp_url_str, dmrpp_string);
    }
    catch (http::HttpError &http_error) {
        string err_msg = "Hyrax encountered a Service Chaining Error while attempting to retrieve a dmr++ file.\n"
                         "This could be a problem with TEA (the AWS URL signing authority),\n"
                         "or with accessing the dmr++ file at its resident location (typically S3).\n"
                         + http_error.get_message();
        http_error.set_message(err_msg);
        throw;
    }

    // if we get here, the DMR++ has been pulled over the network. Put it in both caches.
    // The memory cache is for use by this process, the file cache for other processes/VMs
    if (NgapRequestHandler::d_use_dmrpp_cache) {
        if (!put_item_in_cache(dmrpp_string))
            return false;
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
string NgapOwnedContainer::access() {

    string dmrpp_string;

    // Get the DMR++ from the S3 bucket or the cache.
    // get_dmrpp...() returns false for various caching errors, but throws if it cannot
    // get the remote DMR++. jhrg 4/29/24
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
void NgapOwnedContainer::dump(ostream &strm) const {
    strm << BESIndent::LMarg << "NgapOwnedContainer::dump - (" << (void *) this << ")\n";
    BESIndent::Indent();
    BESContainer::dump(strm);
    BESIndent::UnIndent();
}

} // namespace ngap