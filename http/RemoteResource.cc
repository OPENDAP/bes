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
#include <unistd.h>

#include <sstream>
#include <string>
#include <utility>
#include <memory>
#include <thread>

#include "BESInternalError.h"

#include "BESDebug.h"
#include "BESUtil.h"

#include "HttpUtils.h"
#include "CurlUtils.h"
#include "HttpError.h"
#include "HttpNames.h"
#include "RemoteResource.h"
#include "TheBESKeys.h"
#include "BESStopWatch.h"
#include "BESLog.h"

#define BES_CATALOG_ROOT_KEY "BES.Catalog.catalog.RootDirectory"
// See HttpNames.h for the key definitions.

#define prolog string("RemoteResource::").append(__func__).append("() - ")
#define MODULE HTTP_MODULE

using namespace std;

namespace http {

string RemoteResource::d_temp_file_dir;
std::mutex RemoteResource::d_temp_file_dir_mutex;
std::mutex RemoteResource::d_mkstemp_mutex;

/**
 * @brief Construct a new RemoteResource object
 *
 * If the target_url is a file:// url then this will check make sure
 * that file is within the BES 'catalog' root directory. If it is a
 * remote resource then this will make sure the directory set by the
 * Http.RemoteResource.TmpDir key exists and is writable.
 *
 * To get a remote resource, use the retrieve_resource() method.
 *
 * @param target_url An instance of http::url that points to the resource to be retrieved.
 * @param uid The user ID to use when retrieving the resource.
 */
RemoteResource::RemoteResource(shared_ptr<http::url> target_url, string uid)
    : d_url(std::move(target_url)), d_uid(std::move(uid)) {

    if (d_url->protocol() == FILE_PROTOCOL) {
        set_filename_for_file_url();
        // d_delete_file is true by default; don't delete things referenced by file:// URLs
        d_delete_file = false;
        d_initialized = true;
    }
    else if (d_url->protocol() == HTTPS_PROTOCOL || d_url->protocol() == HTTP_PROTOCOL) {
        BESDEBUG(MODULE, prolog << "URL: " << d_url->str() << endl);

        // d_initialized is false until the resource is retrieved (for http/s URLs)
        set_delete_temp_file();
        set_temp_file_dir();
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

    http::get_type_from_url(d_url->str(), d_type);
    if (d_type.empty()) {
        d_type = "unknown";
    }
}

/**
 * Unlink the temporary file and close its open file descriptor. This
 * will remove the temporary file from the file system.
 */
RemoteResource::~RemoteResource() {
    if (!d_filename.empty() && d_delete_file)
        unlink(d_filename.c_str());
    if (d_fd != -1)
        close(d_fd);
}

/// @name Private methods used by the constructor
/// @{

/**
 * @brief Set the directory where the temporary files are created.
 *
 * @note Private
 *
 * This method is called by the constructor and sets the directory where the temporary files are created.
 * The directory is set using the REMOTE_RESOURCE_TMP_DIR_KEY key. If the key is not set then the
 * directory is set to /tmp/bes_rr_tmp.
 */
void RemoteResource::set_temp_file_dir()
{
    lock_guard<mutex> lock(d_temp_file_dir_mutex);

    // d_temp_file_dir is static, so we only need to set it once.
    if (!d_temp_file_dir.empty())
        return;

    d_temp_file_dir = TheBESKeys::TheKeys()->read_string_key(REMOTE_RESOURCE_TMP_DIR_KEY, "/tmp/bes_rr_tmp");

    if (BESUtil::mkdir_p(d_temp_file_dir, 0775) != 0) {
        throw BESInternalError("Temporary file directory '" + d_temp_file_dir + "' error: " + strerror(errno),
                               __FILE__, __LINE__);
    }
}

/**
 * @brief Set the delete_file flag based on the REMOTE_RESOURCE_DELETE_TMP_FILE key.
 *
 * @note URLs that start with 'file://' are not deleted. By default, http/s URLs are deleted.
 * However, the behaviour for http/s URLs can be changed by setting the REMOTE_RESOURCE_DELETE_TMP_FILE
 * key to false.
 */
void RemoteResource::set_delete_temp_file() {
    d_delete_file = TheBESKeys::TheKeys()->read_bool_key(REMOTE_RESOURCE_DELETE_TMP_FILE, true);
}

/**
 * @brief Set the filename field for a file URL
 *
 * @note Private
 *
 * This method makes sure that the file is within the BES 'catalog' root directory.
 */
void RemoteResource::set_filename_for_file_url() {
    BESDEBUG(MODULE, prolog << "Found FILE protocol." << endl);
    d_filename = d_url->path();
    while (BESUtil::endsWith(d_filename, "/")) {
        // Strip trailing slashes, because this about files, not directories
        d_filename = d_filename.substr(0, d_filename.size() - 1);
    }

    // Now we check that the data is in the BES_CATALOG_ROOT
    string catalog_root = TheBESKeys::TheKeys()->read_string_key(BES_CATALOG_ROOT_KEY, "");
    if (catalog_root.empty()) {
        throw BESInternalError(prolog + "ERROR - " + BES_CATALOG_ROOT_KEY + "is not set", __FILE__, __LINE__);
    }

    if (d_filename.find(catalog_root) != 0) {
        d_filename = BESUtil::pathConcat(catalog_root, d_filename);
    }
    BESDEBUG(MODULE, "d_filename: " << d_filename << endl);
}

/// @}

/**
 * This method will retrieve the remote resource content using HTTP GET.
 *
 * When this method returns the RemoteResource object is fully initialized
 * URL contents are available in the temporary file. For file:// URLs this
 * method is a no-op.
 */
void RemoteResource::retrieve_resource() {
    if (d_initialized) {
        return;
    }

    {
        lock_guard<mutex> lock(d_mkstemp_mutex);
        // Make a temporary file, get an open descriptor for it. The make_temp_file() function
        // throws BESInternalError if it can't make the file.
        d_fd = BESUtil::make_temp_file(d_temp_file_dir, d_filename);
    }

    // Get the contents of the URL and put them in the temp file
    get_url(d_fd);

    string new_name = d_filename + "_" + d_uid + "#" + d_basename;
    if (rename(d_filename.c_str(), new_name.c_str()) != 0) {
        throw BESInternalError("Could not rename " + d_filename + " to " + new_name + " ("
                                + ::strerror(errno) + ")", __FILE__, __LINE__);
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
 * @note Private
 *
 * @param fd An open file descriptor the is associated with the target file.
 */
void RemoteResource::get_url(int fd) {

    BESDEBUG(MODULE, prolog << "BEGIN" << endl);

    BESStopWatch besTimer;
    if (BESDebug::IsSet("rr") || BESDebug::IsSet(MODULE) || BESDebug::IsSet(TIMING_LOG_KEY) ||
        BESLog::TheLog()->is_verbose()) {
        besTimer.start(prolog + "source url: " + d_url->str());
    }
    try {
        // Throws an HttpError if there is a curl error.
        curl::http_get_and_write_resource(d_url, fd, &d_response_headers);
        BESDEBUG(MODULE, prolog << "Resource " << d_url->str() << " saved to temporary file " << d_filename << endl);
    }
    catch(http::HttpError &http_error){
        string err_msg = "Hyrax encountered a Service Chaining Error while "
                         "attempting to retrieve a RemoteResource.\n" + http_error.get_message();;
        http_error.set_message(err_msg);
        throw;
    }


    // Moved into curl::super_easy_perform(CURL*, int fd)
#if 0
    auto status = lseek(fd, 0, SEEK_SET);
    if (-1 == status)
        throw BESInternalError("Could not seek within the response file.", __FILE__, __LINE__);
    BESDEBUG(MODULE, prolog << "Reset file descriptor to start of file." << endl);
#endif

    BESDEBUG(MODULE, prolog << "END" << endl);
}

} //  namespace http

