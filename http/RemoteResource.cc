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

// FIXME I don't know how, but this key should not be hard coded. jhrg 2/8/23
#define BES_CATALOG_ROOT_KEY "BES.Catalog.catalog.RootDirectory"

#define prolog string("RemoteResource::").append(__func__).append("() - ")
#define MODULE HTTP_MODULE

namespace http {

const std::string RemoteResource::d_temp_file_dir = "/tmp/bes_rr_cache";

RemoteResource::RemoteResource(shared_ptr<http::url> target_url, string uid)
    : d_url(std::move(target_url)), d_uid(std::move(uid)) {

    if (d_url->protocol() == FILE_PROTOCOL) {
        BESDEBUG(MODULE, prolog << "Found FILE protocol." << endl);
        d_filename = d_url->path();
        while (BESUtil::endsWith(d_filename, "/")) {
            // Strip trailing slashes, because this about files, not directories
            d_filename = d_filename.substr(0, d_filename.size() - 1);
        }
        // Now we check that the data is in the BES_CATALOG_ROOT
        string catalog_root;
        bool found;
        TheBESKeys::TheKeys()->get_value(BES_CATALOG_ROOT_KEY, catalog_root, found);
        if (!found) {
            throw BESInternalError(prolog + "ERROR - " + BES_CATALOG_ROOT_KEY + "is not set", __FILE__, __LINE__);
        }
        if (d_filename.find(catalog_root) != 0) {
            d_filename = BESUtil::pathConcat(catalog_root, d_filename);
        }
        BESDEBUG(MODULE, "d_resourceCacheFileName: " << d_filename << endl);
        d_initialized = true;
    }
    else if (d_url->protocol() == HTTPS_PROTOCOL || d_url->protocol() == HTTP_PROTOCOL) {
        BESDEBUG(MODULE, prolog << "URL: " << d_url->str() << endl);
        d_delete_file = true;
    }
    else {
        string err = prolog + "Unsupported protocol: " + d_url->protocol();
        throw BESInternalError(err, __FILE__, __LINE__);
    }
}

/**
 * Releases any memory resources and also any existing cache file locks for the cached resource.
 * ( Closes the file descriptor opened when retrieve_resource() was called.)
 */
RemoteResource::~RemoteResource() {
    if (!d_filename.empty() && d_delete_file)
        unlink(d_filename.c_str());
}

/**
 * This method will check the cache for the resource. If it's not there then it will lock the cache and retrieve
 * the remote resource content using HTTP GET.
 *
 * When this method returns the RemoteResource object is fully initialized and the cache file name for the resource
 * is available along with an open file descriptor for the (now read-locked) cache file.
 */
void RemoteResource::retrieve_resource() {
    map<string, string> content_filters;
    retrieveResource(content_filters);
}

/**
 * @brief Make and open a temporary file.
 * The file is opened such that we know it is unique and not in use by another process.
 * The name and file descriptor are set in the RemoteResource object.
 * @param temp_file_dir The directory to hold the temporary file.
 */
void RemoteResource::make_temp_file(const string &temp_file_dir) {
    string temp_file = "/dodsXXXXXX"; // mkstemp uses six characters.

    // mkstemp uses the storage passed to it; must be writable and local.
    vector<char> templat(temp_file_dir.size() + temp_file.size() + 1);
    copy(temp_file_dir.begin(), temp_file_dir.end(), templat.begin());
    copy(temp_file.begin(), temp_file.end(), templat.begin() + temp_file_dir.size());
    templat[temp_file_dir.size() + temp_file.size()] = '\0';

    // Open truncated for update. NB: mkstemp() returns a file descriptor.
    // man mkstemp says "... The file is opened with the O_EXCL flag,
    // guaranteeing that when mkstemp returns successfully we are the only
    // user." 09/19/02 jhrg
    d_fd = mkstemp(templat.data()); // fd mode is 666 or 600 (Unix)
    if (d_fd < 0) {
        throw BESInternalError("mkstemp() failed.", __FILE__, __LINE__);
    }

    d_filename = templat.data();
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
    BESDEBUG(MODULE, prolog << "BEGIN   resourceURL: " << d_url->str() << endl);

    if (d_initialized) {
        BESDEBUG(MODULE, prolog << "END  Already initialized." << endl);
        return;
    }

    http::get_type_from_url(d_url->str(), d_type);
    if (d_type.empty()) {
        BESDEBUG(MODULE, prolog + "Unable to determine the type of data returned from '" + d_url->str()
                         + ",' Setting type to 'unknown'" << endl);
        d_type = "unknown";
    }

    BESDEBUG(MODULE, prolog << "d_type: " << d_type << endl);

    // Make a temporary file, get an open descriptor for it, and read the remote resource into it.
    make_temp_file(d_temp_file_dir);

    get_url(d_fd);
    filter_url(content_filters);

    d_initialized = true;
}

/**
 *
 * Retrieve the remote resource and write it the the file associated with the open file
 * descriptor 'fd'.
 *
 * @note The file descriptor 'fd' is passed in so this is easy to test. Nominally, the file
 * descriptor is the one held by the RemoteResource object.
 *
 * @param fd An open file descriptor the is associated with the target file.
 */
void RemoteResource::get_url(int fd) {

    BESDEBUG(MODULE, prolog << "BEGIN" << endl);

#ifndef NDEBUG
    BESStopWatch besTimer;
    if (BESDebug::IsSet("rr") || BESDebug::IsSet(MODULE) || BESDebug::IsSet(TIMING_LOG_KEY) ||
        BESLog::TheLog()->is_verbose()) {
        besTimer.start(prolog + "source url: " + d_url->str());
    }
#endif

    // Throws BESInternalError if there is a curl error.
    curl::http_get_and_write_resource(d_url, fd, &d_response_headers);
    BESDEBUG(MODULE, prolog << "Resource " << d_url->str() << " saved to cache file " << d_filename << endl);

    // rewind the file
    int status = lseek(fd, 0, SEEK_SET);
    if (-1 == status)
        throw BESInternalError("Could not seek within the response file.", __FILE__, __LINE__);
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
void RemoteResource::filter_url(const map<string, string> &content_filters) const {

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
        ifstream cr_istrm(d_filename);
        if (!cr_istrm.is_open()) {
            string msg = "Could not open '" + d_filename + "' to read cached response.";
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
    ofstream cr_ostrm(d_filename);
    if (!cr_ostrm.is_open()) {
        string msg = "Could not open '" + d_filename + "' to write modified cached response.";
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

    string cache_file = d_filename;
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

/**
 * @brief get_as_json() This function returns the cached resource parsed into a JSON document.
 *
 * @param target_url The URL to dereference.
 *
 * @return JSON document parsed from the response document returned by target_url
 */
rapidjson::Document RemoteResource::get_as_json() const {
    string response = get_response_as_string();
    rapidjson::Document d;
    d.Parse(response.c_str());
    return d;
}

} //  namespace http

