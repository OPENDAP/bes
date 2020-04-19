
// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of cmr_module, A C++ MODULE that can be loaded in to
// the OPeNDAP Back-End Server (BES) and is able to handle remote requests.

// Copyright (c) 2018 OPeNDAP, Inc.
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

#include <config.h>

#include <sys/stat.h>

#include <string>
#include <fstream>
#include <sstream>

#include <cstdlib>

#include "PicoSHA2/picosha2.h"

#include <BESInternalError.h>
#include <BESDebug.h>
#include <BESUtil.h>
#include <TheBESKeys.h>

#include "BESRemoteCache.h"
#include "BESProxyNames.h"

#ifdef HAVE_ATEXIT
#define AT_EXIT(x) atexit((x))
#else
#define AT_EXIT(x)
#endif

#define MODULE "http"

using std::endl;
using std::string;

namespace remote_cache {

    BESRemoteCache *BESRemoteCache::d_instance = 0;
    bool BESRemoteCache::d_enabled = true;

    unsigned long BESRemoteCache::getCacheSizeFromConfig() {
        bool found = false;
        string size;
        unsigned long size_in_megabytes = 0;
        // TODO: Use BESProxyNames.h
        TheBESKeys::TheKeys()->get_value("Http.Cache.size", size, found);

        if (found) {
            std::istringstream iss(size);
            iss >> size_in_megabytes;
        } else {
            string msg = "BESRemoteCache - The BES Key Http.Cache.size is not set.";
            BESDEBUG(MODULE, msg << endl);
            throw BESInternalError(msg, __FILE__, __LINE__);
        }

        return size_in_megabytes;
    }

    string BESRemoteCache::getCacheDirFromConfig() {
        bool found;
        string subdir = "";
        TheBESKeys::TheKeys()->get_value("Http.Cache.dir", subdir, found);

        if (!found) {
            string msg = "BESRemoteCache - The BES Key Http.Cache.dir is not set.";
            BESDEBUG(MODULE, msg << endl);
            throw BESInternalError(msg, __FILE__, __LINE__);
        }

        return subdir;
    }

    string BESRemoteCache::getCachePrefixFromConfig() {
        bool found;
        string prefix = "";
        TheBESKeys::TheKeys()->get_value("Http.Cache.prefix", prefix, found);

        if (found) {
            prefix = BESUtil::lowercase(prefix);
        } else {
            string msg = "BESRemoteCache - The BES Key Http.Cache.prefix is not set.";
            BESDEBUG(MODULE, msg << endl);
            throw BESInternalError(msg, __FILE__, __LINE__);
        }

        return prefix;
    }

    BESRemoteCache::BESRemoteCache() {
        BESDEBUG(MODULE, "BESRemoteCache::BESRemoteCache() -  BEGIN" << endl);

        string cacheDir = getCacheDirFromConfig();
        string cachePrefix = getCachePrefixFromConfig();
        unsigned long cacheSizeMbytes = getCacheSizeFromConfig();

        BESDEBUG(MODULE, "BESRemoteCache() - Cache configuration params: " << cacheDir << ", " << cachePrefix << ", "
                                                                           << cacheSizeMbytes << endl);

        initialize(cacheDir, cachePrefix, cacheSizeMbytes);

        BESDEBUG(MODULE, "BESRemoteCache::BESRemoteCache() -  END" << endl);
    }

    BESRemoteCache::BESRemoteCache(const string &cache_dir, const string &prefix, unsigned long long size) {
        BESDEBUG(MODULE, "BESRemoteCache::BESRemoteCache() -  BEGIN" << endl);

        initialize(cache_dir, prefix, size);

        BESDEBUG(MODULE, "BESRemoteCache::BESRemoteCache() -  END" << endl);
    }

    BESRemoteCache *
    BESRemoteCache::get_instance(const string &cache_dir, const string &cache_file_prefix,
                                 unsigned long long max_cache_size) {
        if (d_enabled && d_instance == 0) {
            if (dir_exists(cache_dir)) {
                d_instance = new BESRemoteCache(cache_dir, cache_file_prefix, max_cache_size);
                d_enabled = d_instance->cache_enabled();
                if (!d_enabled) {
                    delete d_instance;
                    d_instance = 0;
                    BESDEBUG(MODULE, "BESRemoteCache::" << __func__ << "() - " << "Cache is DISABLED" << endl);
                } else {
                    AT_EXIT(delete_instance);

                    BESDEBUG(MODULE, "BESRemoteCache::" << __func__ << "() - " << "Cache is ENABLED" << endl);
                }
            }
        }

        return d_instance;
    }

/** Get the default instance of the HttpdCatalogCache object. This will read "TheBESKeys" looking for the values
 * of SUBDIR_KEY, PREFIX_KEY, an SIZE_KEY to initialize the cache.
 */
    BESRemoteCache *
    BESRemoteCache::get_instance() {
        if (d_enabled && d_instance == 0) {
            try {
                d_instance = new BESRemoteCache();
                d_enabled = d_instance->cache_enabled();
                if (!d_enabled) {
                    delete d_instance;
                    d_instance = 0;
                    BESDEBUG(MODULE, "BESRemoteCache::" << __func__ << "() - " << "Cache is DISABLED" << endl);
                } else {
                    AT_EXIT(delete_instance);

                    BESDEBUG(MODULE, "BESRemoteCache::" << __func__ << "() - " << "Cache is ENABLED" << endl);
                }
            }
            catch (BESInternalError &bie) {
                BESDEBUG(MODULE,
                         "[ERROR] BESRemoteCache::get_instance(): Failed to obtain cache! msg: " << bie.get_message()
                                                                                                 << endl);
            }
        }

        return d_instance;
    }

/**
 * Compute the SHA256 hash for the item name
 *
 * @param name The name to hash
 * @return The SHA256 hash of the name.
 */
    inline string BESRemoteCache::get_hash(const string &name) {
        if (name.empty()) throw BESInternalError("Empty name passed to the Metadata Store.", __FILE__, __LINE__);
        return picosha2::hash256_hex_string(name[0] == '/' ? name : "/" + name);
    }

    string BESRemoteCache::get_cache_file_name(const string &src, bool /*mangle*/) {
        return BESUtil::assemblePath(this->get_cache_directory(), get_cache_file_prefix() + get_hash(src));
    }

    string BESRemoteCache::get_cache_file_name(const string &uid, const string &src, bool /*mangle*/) {

        string uid_part;
        if (!uid.empty())
            uid_part = uid + "_";

        return BESUtil::assemblePath(this->get_cache_directory(),
                                     get_cache_file_prefix() + uid_part + get_hash(src));
    }


} // namespace remote_cache
