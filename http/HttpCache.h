
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

#ifndef  _bes_http_HTTP_CACHE_H_
#define  _bes_http_HTTP_CACHE_H_ 1

#include "BESFileLockingCache.h"

namespace http {

/**
 * @brief A cache for content accessed via HTTP.
 *
 * This cache is a simple cache for data files implemented using
 * advisory file locking on a POSIX file system (it is a specialization
 * of BESFileLockingCache).
 *
 * This cache uses the following keys in the bes.conf file:
 * - _HttpResourceCache.dir_: The directory where retrieved data files should be stored
 * - _HttpResourceCache.prefix_: The item-name prefix for this cache
 * - _HttpResourceCache.size_: The size of the cache. Limit the total amount of
 *   data cached to this many MB. If zero, the cache size is unlimited.
 *
 * All of the keys must be defined for this cache (the BES uses several caches
 * and some of them are optional - this cache is not optional).
 *
 */
class HttpCache : public BESFileLockingCache {
private:
    static std::unique_ptr<HttpCache> d_instance;

public:
    HttpCache() = default;
    HttpCache(const HttpCache &src) = delete;
    HttpCache &operator=(const HttpCache &rhs) = delete;

    ~HttpCache() override = default;

    static HttpCache *get_instance();

    std::string get_cache_file_name(const std::string &uid, const std::string &src, bool mangle = true);

    std::string get_cache_file_name(const std::string &src, bool mangle = true) override;
};

std::string get_http_cache_dir_from_config();
std::string get_http_cache_prefix_from_config();
unsigned long get_http_cache_size_from_config();
unsigned long get_http_cache_exp_time_from_config();

} /* namespace http */

#endif /*  _bes_http_HTTP_CACHE_H_ */