// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of the BES http package, part of the Hyrax data server.

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

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <sstream>
#include <fstream>
#include <string>
#include <iostream>
#include <utility>

#include "rapidjson/document.h"

#include "BESInternalError.h"
#include "BESForbiddenError.h"
#include "BESSyntaxUserError.h"
#include "BESNotFoundError.h"
#include "BESTimeoutError.h"

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

#define prolog std::string("RemoteResource::").append(__func__).append("() - ")
#define MODULE HTTP_MODULE

namespace http {

RemoteResource::RemoteResource(
        std::shared_ptr<http::url> target_url,
        const std::string &uid,
        long long expiredInterval)
        : d_remoteResourceUrl(std::move(target_url)) {

    d_fd = 0;
    d_initialized = false;

    d_uid = uid;

    d_resourceCacheFileName.clear();
    d_response_headers = new vector<string>();
    d_http_response_headers = new map<string, string>();

    d_expires_interval = expiredInterval;


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

    // BESDEBUG(MODULE, prolog << "d_curl: " << d_curl << endl);

}

/**
 * Releases any memory resources and also any existing cache file locks for the cached resource.
 * ( Closes the file descriptor opened when retrieveResource() was called.)
 */
RemoteResource::~RemoteResource() {
    BESDEBUG(MODULE, prolog << "BEGIN resourceURL: " << d_remoteResourceUrl->str() << endl);

    delete d_response_headers;
    d_response_headers = 0;
    BESDEBUG(MODULE, prolog << "Deleted d_response_headers." << endl);

    if (!d_resourceCacheFileName.empty()) {
        HttpCache *cache = HttpCache::get_instance();
        if (cache) {
            cache->unlock_and_close(d_resourceCacheFileName);
            BESDEBUG(MODULE, prolog << "Closed and unlocked " << d_resourceCacheFileName << endl);
            d_resourceCacheFileName.clear();
        }
    }
    BESDEBUG(MODULE, prolog << "END" << endl);
}

/**
 * Returns the (read-locked) cache file name on the local system in which the content of the remote
 * resource is stored. Deleting of the instance of this class will release the read-lock.
 */
std::string RemoteResource::getCacheFileName() {
    if (!d_initialized) {
        throw BESInternalError(prolog + "STATE ERROR: Remote Resource " + d_remoteResourceUrl->str() +
                               " has Not Been Retrieved.", __FILE__, __LINE__);
    }
    return d_resourceCacheFileName;
}

/**
 * This method will check the cache for the resource. If it's not there then it will lock the cache and retrieve
 * the remote resource content using HTTP GET.
 *
 * When this method returns the RemoteResource object is fully initialized and the cache file name for the resource
 * is available along with an open file descriptor for the (now read-locked) cache file.
 */
void RemoteResource::retrieveResource() {
    std::map<std::string, std::string> content_filters;
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
void RemoteResource::retrieveResource(const std::map<std::string, std::string> &content_filters) {
    BESDEBUG(MODULE, prolog << "BEGIN   resourceURL: " << d_remoteResourceUrl->str() << endl);
    bool mangle = true;

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
    d_resourceCacheFileName = cache->get_cache_file_name(d_uid, d_remoteResourceUrl->str(), mangle);
    BESDEBUG(MODULE, prolog << "d_resourceCacheFileName: " << d_resourceCacheFileName << endl);

    // @TODO MAKE THIS RETRIEVE THE CACHED DATA TYPE IF THE CACHED RESPONSE IF FOUND
    //   We need to know the type of the resource. HTTP headers are the preferred  way to determine the type.
    //   Unfortunately, the current code losses both the HTTP headers sent from the request and the derived type
    //   to subsequent accesses of the cached object. Since we have to have a type, for now we just set the type
    //   from the url. If down below we DO an HTTP GET then the headers will be evaluated and the type set by setType()
    //   But really - we gotta fix this.
    http::get_type_from_url(d_remoteResourceUrl->str(), d_type);
    if (d_type.empty()) {
        string err = prolog + "Unable to determine the type of data returned from '" + d_remoteResourceUrl->str()
                + ",' Setting type to 'unknown'";
        BESDEBUG(MODULE, err << endl);
        INFO_LOG(err);
        d_type = "unknown";
    }

    BESDEBUG(MODULE, prolog << "d_type: " << d_type << endl);

    try {
#if 0
        // NB: I am leaving this in place because want to referr to its logic. jhrg 11/6/22
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
    catch (BESError &besError) {
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
 * Checks if a cache resource is older than an hour
 *
 * @param filename - name of the resource to be checked
 * @param uid
 * @return true if the resource is over an hour old
 */
bool RemoteResource::cached_resource_is_expired() const {
    BESDEBUG(MODULE, prolog << "BEGIN" << endl);

    struct stat statbuf;
    if (stat(d_resourceCacheFileName.c_str(), &statbuf) == -1) {
        throw BESNotFoundError(strerror(errno), __FILE__, __LINE__);
    }//end if
    BESDEBUG(MODULE, prolog << "File exists" << endl);

    time_t cacheTime = statbuf.st_ctime;
    BESDEBUG(MODULE, prolog << "Cache file creation time: " << cacheTime << endl);
    time_t nowTime = time(0);
    BESDEBUG(MODULE, prolog << "Time now: " << nowTime << endl);
    double diffSeconds = difftime(nowTime, cacheTime);
    BESDEBUG(MODULE, prolog << "Time difference between cacheTime and nowTime: " << diffSeconds << endl);

    if (diffSeconds > d_expires_interval) {
        BESDEBUG(MODULE, prolog << " refresh = TRUE " << endl);
        return true;
    }
    else {
        BESDEBUG(MODULE, prolog << " refresh = FALSE " << endl);
        return false;
    }
} //end RemoteResource::is_cache_resource_expired()

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
    try {

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

        BESDEBUG(MODULE,
                 prolog << "Saving resource " << d_remoteResourceUrl << " to cache file " << d_resourceCacheFileName
                        << endl);
        curl::http_get_and_write_resource(d_remoteResourceUrl, fd,
                                          d_response_headers); // Throws BESInternalError if there is a curl error.

        BESDEBUG(MODULE, prolog << "Resource " << d_remoteResourceUrl->str() << " saved to cache file "
                                << d_resourceCacheFileName << endl);

        // rewind the file
        status = lseek(fd, 0, SEEK_SET);
        if (-1 == status)
            throw BESNotFoundError("Could not seek within the response file.", __FILE__, __LINE__);
        BESDEBUG(MODULE, prolog << "Reset file descriptor to start of file." << endl);
    }
    catch (BESError &e) {
        throw;
    }
    BESDEBUG(MODULE, prolog << "END" << endl);
}

#if 0

/**
 *
 */
void RemoteResource::derive_type_from_url() {
    BESDEBUG(MODULE, prolog << "BEGIN" << endl);
    string type;

    // still haven't figured out the type. Now check the actual URL
    // and see if we can't match the URL to a MODULE name
    BESDEBUG(MODULE, prolog << "Checking URL path for type information." << endl);
    if (type.empty()) {
        http::get_type_from_url(d_remoteResourceUrl->str(), type);
        BESDEBUG(MODULE,
                 prolog << "Evaluated url '" << d_remoteResourceUrl->str() << "' matched type: \"" << type << "\""
                        << endl);
    }
    // still couldn't figure it out, punt
    if (type.empty()) {
        string err = prolog + "Unable to determine the type of data"
                     + " returned from '" + d_remoteResourceUrl->str() + "'  Setting type to 'unknown'";
        BESDEBUG(MODULE, err << endl);
        type = "unknown";
        //throw BESSyntaxUserError( err, __FILE__, __LINE__ ) ;
    }
    d_type = type;
    BESDEBUG(MODULE, prolog << "END (dataset type: " << d_type << ")" << endl);
}

#endif

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
void RemoteResource::filter_retrieved_resource(const std::map<std::string, std::string> &content_filters) {

    // No filters?
    if (content_filters.empty()) {
        // No problem...
        return;
    }
    string resource_content;
    {
        std::stringstream buffer;
        //  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
        // Read the cached file into a string object
        std::ifstream cr_istrm(d_resourceCacheFileName);
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
    std::ofstream cr_ostrm(d_resourceCacheFileName);
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
std::string RemoteResource::get_response_as_string() {

    if (!d_initialized) {
        stringstream msg;
        msg << "ERROR. Internal state error. " << __PRETTY_FUNCTION__ << " was called prior to retrieving resource.";
        BESDEBUG(MODULE, prolog << msg.str() << endl);
        throw BESInternalError(msg.str(), __FILE__, __LINE__);
    }
    string cache_file = getCacheFileName();
    //  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
    // Set up cache file input stream.
    std::ifstream file_istream(cache_file, std::ofstream::in);

    // If the cache filename is not valid, the stream will not open. Empty is not valid.
    if (file_istream.is_open()) {
        // If it's open we've got a valid input stream.
        BESDEBUG(MODULE, prolog << "Using cached file: " << cache_file << endl);
        std::stringstream buffer;
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

/**
 * @brief get_as_json() This function returns the cached resource parsed into a JSON document.
 *
 * @param target_url The URL to dereference.
 * @TODO Move this to ../curl_utils.cc (Requires moving the rapidjson lib too)
 * @return JSON document parsed from the response document returned by target_url
 */
rapidjson::Document RemoteResource::get_as_json() {
    string response = get_response_as_string();
    rapidjson::Document d;
    d.Parse(response.c_str());
    return d;
}

} //  namespace http

