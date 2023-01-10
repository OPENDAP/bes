
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

#include <config.h>

#include <string>
#include <sstream>
#include <vector>

#include "PicoSHA2/picosha2.h"

#include <BESInternalError.h>
#include <BESDebug.h>
#include <BESUtil.h>
#include <TheBESKeys.h>

#include "HttpCache.h"
#include "HttpNames.h"
#include "url_impl.h"

#define prolog string("HttpCache::").append(__func__).append("() - ")

using std::endl;
using std::string;
using std::vector;
using std::stringstream;

namespace http {

std::unique_ptr<HttpCache> HttpCache::d_instance = nullptr;

/**
 * @brief locally-scoped function for common exception code
 * @param key The key that was not defined
 * @param line The line number in the code where the exception is thrown
 */
static void throw_if_key_not_found(const string &key, int line) {
    string msg = prolog + "The BES Key " + key + " is not set.";
    BESDEBUG(HTTP_MODULE, msg << endl);
    throw BESInternalError(msg, __FILE__, line);
}

unsigned long get_http_cache_exp_time_from_config() {
    unsigned long time_in_seconds = TheBESKeys::TheKeys()->read_ulong_key(HTTP_CACHE_EXPIRES_TIME_KEY,
                                                                          REMOTE_RESOURCE_DEFAULT_EXPIRED_INTERVAL);
    return time_in_seconds;
}

// FIXME Resolve this question: If these are errors and they are in the configuration file,
//  should they be fatal errors and should they be detected when the daemon starts? jhrg 1/9/23
unsigned long get_http_cache_size_from_config() {
    unsigned long time_in_seconds = TheBESKeys::TheKeys()->read_ulong_key(HTTP_CACHE_SIZE_KEY, 0);
    if (time_in_seconds == 0) {
        throw_if_key_not_found(HTTP_CACHE_SIZE_KEY, __LINE__);
    }
    return time_in_seconds;
}

string get_http_cache_dir_from_config() {
    string dir = TheBESKeys::TheKeys()->read_string_key(HTTP_CACHE_DIR_KEY, "");
    if (dir.empty()) {
        throw_if_key_not_found(HTTP_CACHE_DIR_KEY, __LINE__);
    }
    return dir;
}

string get_http_cache_prefix_from_config() {
    string prefix = TheBESKeys::TheKeys()->read_string_key(HTTP_CACHE_PREFIX_KEY, "");
    if (prefix.empty()) {
        throw_if_key_not_found(HTTP_CACHE_PREFIX_KEY, __LINE__);
    }
    return prefix;
}

/**
 * @brief Get the singleton instance of the HttpCache.
 * This will read "TheBESKeys" looking for the values of SUBDIR_KEY, PREFIX_KEY, and
 * SIZE_KEY to initialize the cache.
 * @exception Throws BESInternalError if the keys are not found.
 * @return The singleton instance of the HttpCache.
 */
HttpCache *
HttpCache::get_instance() {
    if (d_instance == nullptr) {
        static std::once_flag d_euc_init_once;
        std::call_once(d_euc_init_once, []() {
            d_instance.reset(new HttpCache());
        });

        string cacheDir = get_http_cache_dir_from_config();
        string cachePrefix = get_http_cache_prefix_from_config();
        unsigned long cacheSizeMbytes = get_http_cache_size_from_config();

        BESDEBUG(HTTP_MODULE, prolog << "Cache configuration params: " << cacheDir << ", " << cachePrefix << ", "
                                     << cacheSizeMbytes << endl);
        d_instance->initialize(cacheDir, cachePrefix, cacheSizeMbytes);
    }

    return d_instance.get();
}

string get_hash(const string &s) {
    if (s.empty()) {
        string msg = "You cannot hash the empty string.";
        BESDEBUG(HTTP_MODULE, prolog << msg << endl);
        throw BESInternalError(msg, __FILE__, __LINE__);
    }
    return picosha2::hash256_hex_string(s[0] == '/' ? s : "/" + s);
}

bool is_url(const string &candidate) {
    size_t index = candidate.find(HTTP_PROTOCOL);
    if (index) {
        index = candidate.find(HTTPS_PROTOCOL);
        if (index) {
            return false;
        }
    }
    return true;
}

/**
 * This helper function looks at the passed identifier and tries to formulate
 * a human readable summary string for use in dataset naming etc.
 *
 * @param identifier A string holding the identifier to summarize.
 * @return A human readable summary string for use in dataset naming etc.
 */
string get_real_name_extension(const string &identifier) {
    string path_part;

    if (is_url(identifier)) {
        // Since it's a URL it might have a massive query string attached, and since we
        // have no idea what the query parameters mean, we'll just punt and look at the
        // path part of the URL. We make an instance of http::url which will carve up
        // the URL for us.
        http::url target_url(identifier);
        path_part = target_url.path();
    }
    else {
        path_part = identifier;
    }

    string real_name_extension;
    vector<string> path_elements;
    // Now that we have a "path" (none of that query string mess) we can tokenize it.
    BESUtil::tokenize(path_part, path_elements);
    if (!path_elements.empty()) {
        string last = path_elements.back();
        if (last != path_part)
            real_name_extension = "#" + last; // This utilizes a hack in libdap
    }
    return real_name_extension;
}

/**
 * Builds a cache file name that contains a hashed version of the src_id, the user id (uid) if non-empty, and a
 * human readable name that may be utilized when naming datasets whose data are held in the cache file.
 * @param uid The user id of the requesting user.
 * @param src_id The source identifier of the resource to cache.
 * @param mangle If true, the cache file names will be hashed (more or less).
 * @return The name of the cache file based on the inputs.
 */
string HttpCache::get_cache_file_name(const string &uid, const string &src_id, bool mangle) {
    string uid_part = uid.empty() ? "" : uid + "_";
    string src_id_part = mangle ? get_hash(src_id) : src_id;

    string cache_filename = get_cache_file_prefix() + uid_part + src_id_part + get_real_name_extension(src_id);

    return BESUtil::assemblePath(get_cache_directory(), cache_filename);
}


string HttpCache::get_cache_file_name(const string &src, bool mangle) {
    string uid;
    return get_cache_file_name(uid, src, mangle);
}

} // namespace http
