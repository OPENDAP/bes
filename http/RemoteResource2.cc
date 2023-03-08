// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of the BES http package, part of the Hyrax data server.

// Copyright (c) 2020,2023 OPeNDAP, Inc.
// Authors: Nathan Potter <ndp@opendap.org>,
// James Gallagher <jgallagher@opendap.org>
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

#include "config.h"

#include <sys/stat.h>
#include <unistd.h>

#include <sstream>
#include <fstream>
#include <string>
#include <utility>
#include <memory>

#include "rapidjson/document.h"

#include "BESInternalError.h"
#include "BESNotFoundError.h"

#include "BESDebug.h"
#include "BESUtil.h"

#include "HttpCache.h"
#include "HttpUtils.h"
#include "CurlUtils.h"
#include "HttpNames.h"
#include "RemoteResource.h"
#include "TheBESKeys.h"
#include "BESStopWatch.h"
#include "BESLog.h"

using namespace std;

#define BES_CATALOG_ROOT_KEY "BES.Catalog.catalog.RootDirectory"

#define prolog string("RemoteResource::").append(__func__).append("() - ")
#define MODULE HTTP_MODULE

namespace http {

RemoteResource::RemoteResource(shared_ptr<http::url> target_url, string uid, unsigned long expiredInterval)
                               : d_remoteResourceUrl(std::move(target_url)), d_uid(std::move(uid)),
                               d_expires_interval(expiredInterval) {

    d_resourceCacheFileName.clear();

    if (d_remoteResourceUrl->protocol() == FILE_PROTOCOL) {
        BESDEBUG(MODULE, prolog << "Found FILE protocol." << endl);
        d_resourceCacheFileName = d_remoteResourceUrl->path();
        while (BESUtil::endsWith(d_resourceCacheFileName, "/")) {
            // Strip trailing slashes, because this about files, not directories
            d_resourceCacheFileName = d_resourceCacheFileName.substr(0, d_resourceCacheFileName.size() - 1);
        }
        // Now we check that the data is in the BES_CATALOG_ROOT
        string catalog_root;
        bool found;
        TheBESKeys::TheKeys()->get_value(BES_CATALOG_ROOT_KEY, catalog_root, found);
        if (!found) {
            throw BESInternalError(prolog + "ERROR - " + BES_CATALOG_ROOT_KEY + "is not set", __FILE__, __LINE__);
        }
        if (d_resourceCacheFileName.find(catalog_root) != 0) {
            d_resourceCacheFileName = BESUtil::pathConcat(catalog_root, d_resourceCacheFileName);
        }
        BESDEBUG(MODULE, "d_resourceCacheFileName: " << d_resourceCacheFileName << endl);
        d_initialized = true;
    }
    else if (d_remoteResourceUrl->protocol() == HTTPS_PROTOCOL || d_remoteResourceUrl->protocol() == HTTP_PROTOCOL) {
        BESDEBUG(MODULE, prolog << "URL: " << d_remoteResourceUrl->str() << endl);
    }
    else {
        string err = prolog + "Unsupported protocol: " + d_remoteResourceUrl->protocol();
        throw BESInternalError(err, __FILE__, __LINE__);
    }
}

/**
 * This method will check the cache for the resource. If it's not there then it will lock the cache and retrieve
 * the remote resource content using HTTP GET.
 *
 * When this method returns the RemoteResource object is fully initialized and the cache file name for the resource
 * is available along with an open file descriptor for the (now read-locked) cache file.
 */
void RemoteResource::retrieveResource() {
    map<string, string> content_filters;
    retrieveResource(content_filters);
}

/**
 * This method will check the cache for the resource. If it's not there then it will lock the cache and retrieve
 * the remote resource content using HTTP GET.
 *
 * When this method returns the RemoteResource object is fully initialized and the cache file name for the resource
 * is available along with an open file descriptor for the (now read-locked) cache file.
 *
 * @param content_filters A C++ map<String, string> that is used to substitute specific values
 * for templates found in the remote resource. These values are substituted only when the remote
 * resource is accessed and cached (i.e., the information in the cache has the substituted values).
 *
 * @note Calling this method once a remote resource has been retrieved (and cached) does nothing,
 * this method returns immediately in that case. Use getCacheFileName() to get the name of the file
 * with the cached information, use methods like get_response_as_string() to access copies of the
 * data in the cached remote resource.
 */
void RemoteResource::retrieveResource(const map<string, string> &content_filters) {
    BESDEBUG(MODULE, prolog << "BEGIN   resourceURL: " << d_remoteResourceUrl->str() << endl);

    if (d_initialized) {
        BESDEBUG(MODULE, prolog << "END  Already initialized." << endl);
        return;
    }
    // Get a pointer to the singleton cache instance for this process.
    HttpCache *cache = HttpCache::get_instance();
    if (!cache) {
        ostringstream oss;
        oss << prolog << "FAILED to get local cache. ";
        oss << "Unable to proceed with request for " << this->d_remoteResourceUrl->str();
        oss << " The server MUST have a valid HTTP cache configuration to operate." << endl;
        BESDEBUG(MODULE, oss.str());
        throw BESInternalError(oss.str(), __FILE__, __LINE__);
    }

    // Get the name of the file in the cache (either the code finds this file or it makes it).
    bool mangle = true;
    d_resourceCacheFileName = cache->get_cache_file_name(d_uid, d_remoteResourceUrl->str(), mangle);
    BESDEBUG(MODULE, prolog << "d_resourceCacheFileName: " << d_resourceCacheFileName << endl);

    http::get_type_from_url(d_remoteResourceUrl->str(), d_type);
    if (d_type.empty()) {
        string err = prolog + "Unable to determine the type of data returned from '" + d_remoteResourceUrl->str()
                + ",' Setting type to 'unknown'";
        BESDEBUG(MODULE, err << endl);
        // INFO_LOG(err << endl);
        d_type = "unknown";
    }

    BESDEBUG(MODULE, prolog << "d_type: " << d_type << endl);

    try {
#if 0
        // NB: I am leaving this in place because we may want to referr to its logic. jhrg 11/6/22
        if (cache->get_exclusive_lock(d_resourceCacheFileName, d_fd)) {
            if (cached_resource_is_expired()) {
                 update_file_and_headers(content_filters);
                cache->exclusive_to_shared_lock(d_fd);
            }
            else {
                 cache->exclusive_to_shared_lock(d_fd);
                load_hdrs_from_file();
            }
            d_initialized = true;
         }
#endif
        // Now we actually need to reach out across the interwebs and retrieve the remote resource and put its
        // content into a local cache file, given that it's not in the cache.
        // First make an empty file and get an exclusive lock on it.
        if (cache->create_and_lock(d_resourceCacheFileName, d_fd)) {
            BESDEBUG(MODULE, prolog << "DOESN'T EXIST - CREATING " << endl);
            writeResourceToFile(d_fd);
            filter_retrieved_resource(content_filters);
        }
        else {
            BESDEBUG(MODULE, prolog << " EXISTS - CHECKING EXPIRY " << endl);
            cache->get_read_lock(d_resourceCacheFileName, d_fd);
        }
        d_initialized = true;
    }
    catch (const BESError &besError) {
        BESDEBUG(MODULE, prolog << "Caught BESError. type: " << besError.get_bes_error_type() <<
                                " message: '" << besError.get_message() <<
                                "' file: " << besError.get_file() << " line: " << besError.get_line() <<
                                " Will unlock cache and re-throw." << endl);
        cache->unlock_cache();
        throw;
    }
    catch (...) {
        BESDEBUG(MODULE, prolog << "Caught unknown exception. Will unlock cache and re-throw." << endl);
        cache->unlock_cache();
        throw;
    }

} //end RemoteResource::retrieveResource()


/**
 *
 * Retrieves the remote resource and write it the the open file associated with the open file
 * descriptor parameter 'fd'. In the process of caching the file a FILE * is fdopen'd from 'fd' and that is used buy
 * curl to write the content. At the end the stream is rewound and the FILE * pointer is returned.
 *
 * @param fd An open file descriptor the is associated with the target file.
 */
void RemoteResource::writeResourceToFile(int fd) {

    BESDEBUG(MODULE, prolog << "BEGIN" << endl);

    BESStopWatch besTimer;
    if (BESDebug::IsSet("rr") || BESDebug::IsSet(MODULE) || BESDebug::IsSet(TIMING_LOG_KEY) ||
        BESLog::TheLog()->is_verbose()) {
        besTimer.start(prolog + "source url: " + d_remoteResourceUrl->str());
    }

    int status = lseek(fd, 0, SEEK_SET);
    if (-1 == status)
        throw BESNotFoundError("Could not seek within the response file.", __FILE__, __LINE__);
    BESDEBUG(MODULE, prolog << "Reset file descriptor to start of file." << endl);

    status = ftruncate(fd, 0);
    if (-1 == status)
        throw BESInternalError("Could not truncate the file prior to updating from remote. ", __FILE__, __LINE__);
    BESDEBUG(MODULE, prolog << "Truncated file, length is zero." << endl);

    BESDEBUG(MODULE, prolog << "Saving resource " << d_remoteResourceUrl << " to cache file "
                            << d_resourceCacheFileName << endl);
    // Throws BESInternalError if there is a curl error.
    curl::http_get_and_write_resource(d_remoteResourceUrl, fd, &d_response_headers);
    BESDEBUG(MODULE, prolog << "Resource " << d_remoteResourceUrl->str() << " saved to cache file "
                            << d_resourceCacheFileName << endl);

    // rewind the file
    status = lseek(fd, 0, SEEK_SET);
    if (-1 == status)
        throw BESNotFoundError("Could not seek within the response file.", __FILE__, __LINE__);
    BESDEBUG(MODULE, prolog << "Reset file descriptor to start of file." << endl);

    BESDEBUG(MODULE, prolog << "END" << endl);
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
void RemoteResource::filter_retrieved_resource(const map<string, string> &content_filters) const {

    // No filters?
    if (content_filters.empty()) {
        // No problem...
        return;
    }
    string resource_content;
    {
        stringstream buffer;
        //  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
        // Read the cached file into a string object
        ifstream cr_istrm(d_resourceCacheFileName);
        if (!cr_istrm.is_open()) {
            string msg = "Could not open '" + d_resourceCacheFileName + "' to read cached response.";
            BESDEBUG(MODULE, prolog << msg << endl);
            throw BESInternalError(msg, __FILE__, __LINE__);
        }
        buffer << cr_istrm.rdbuf();

        resource_content = buffer.str();
    } // cr_istrm is closed here.

    for (const auto &apair: content_filters) {
        unsigned int replace_count = BESUtil::replace_all(resource_content, apair.first, apair.second);
        BESDEBUG(MODULE, prolog << "Replaced " << replace_count << " instance(s) of template(" <<
                                apair.first << ") with " << apair.second << " in cached RemoteResource" << endl);
    }


    //  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
    // Replace the contents of the cached file with the modified string.
    ofstream cr_ostrm(d_resourceCacheFileName);
    if (!cr_ostrm.is_open()) {
        string msg = "Could not open '" + d_resourceCacheFileName + "' to write modified cached response.";
        BESDEBUG(MODULE, prolog << msg << endl);
        throw BESInternalError(msg, __FILE__, __LINE__);
    }
    cr_ostrm << resource_content;

}

/**
 * Returns cache file content in a string..
 */
string RemoteResource::get_response_as_string() const {

    if (!d_initialized) {
        stringstream msg;
        msg << "ERROR. Internal state error. " << __PRETTY_FUNCTION__ << " was called prior to retrieving resource.";
        BESDEBUG(MODULE, prolog << msg.str() << endl);
        throw BESInternalError(msg.str(), __FILE__, __LINE__);
    }
    string cache_file = getCacheFileName();
    //  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
    // Set up cache file input stream.
    ifstream file_istream(cache_file, ofstream::in);

    // If the cache filename is not valid, the stream will not open. Empty is not valid.
    if (file_istream.is_open()) {
        // If it's open we've got a valid input stream.
        BESDEBUG(MODULE, prolog << "Using cached file: " << cache_file << endl);
        stringstream buffer;
        buffer << file_istream.rdbuf();
        return buffer.str();
    }
    else {
        stringstream msg;
        msg << "ERROR. Failed to open cache file " << cache_file << " for reading.";
        BESDEBUG(MODULE, prolog << msg.str() << endl);
        throw BESInternalError(msg.str(), __FILE__, __LINE__);
    }
}

} //  namespace http

