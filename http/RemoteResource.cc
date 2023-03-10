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

#include <cstdio>
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

// FIXME Make this configurable. jhrg 3/8/23
std::string RemoteResource::d_temp_file_dir = "/tmp/bes_rr_cache";

RemoteResource::RemoteResource(shared_ptr<http::url> target_url, string uid)
    : d_url(target_url), d_uid(std::move(uid)) {

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

    // Now set d_basename using the URL path (this elides any query string).
    // d_basename may stay empty (its init value) if the URL path is empty.
    vector<string> path_elements;
    BESUtil::tokenize(d_url->path(), path_elements);
    if (!path_elements.empty()) {
        d_basename = path_elements.back();
    }
}

/**
 * Releases any memory resources and also any existing cache file locks for the cached resource.
 * ( Closes the file descriptor opened when retrieve_resource() was called.)
 */
RemoteResource::~RemoteResource() {
    if (!d_filename.empty() && d_delete_file)
        unlink(d_filename.c_str());
    if (d_fd != -1)
        close(d_fd);
}

/**
 * This method will check the cache for the resource. If it's not there then it will lock the cache and retrieve
 * the remote resource content using HTTP GET.
 *
 * When this method returns the RemoteResource object is fully initialized and the cache file name for the resource
 * is available along with an open file descriptor for the (now read-locked) cache file.
 */
void RemoteResource::retrieve_resource() {
    if (d_initialized) {
        return;
    }

    http::get_type_from_url(d_url->str(), d_type);
    if (d_type.empty()) {
        d_type = "unknown";
    }

    // Make a temporary file, get an open descriptor for it, and read the remote resource into it.
    d_fd = BESUtil::make_temp_file(d_temp_file_dir, d_filename);

    // Get the contents of the URL and put them in the temp file
    get_url(d_fd);

    string new_name = d_filename + d_uid + "#" + d_basename;
    if (rename(d_filename.c_str(), new_name.c_str()) != 0) {
        throw BESInternalError("Could not rename " + d_filename + " to " + new_name, __FILE__, __LINE__);
    }

    d_filename = new_name;

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

} //  namespace http

