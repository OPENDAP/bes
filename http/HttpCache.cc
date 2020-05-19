
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

#include "HttpCache.h"
#include "HttpNames.h"

#ifdef HAVE_ATEXIT
#define AT_EXIT(x) atexit((x))
#else
#define AT_EXIT(x)
#endif


using std::endl;
using std::string;
using std::stringstream;

namespace remote_cache {

    HttpCache *HttpCache::d_instance = 0;
    bool HttpCache::d_enabled = true;

    unsigned long HttpCache::getCacheSizeFromConfig() {
        bool found = false;
        string size;
        unsigned long size_in_megabytes = 0;
        TheBESKeys::TheKeys()->get_value(HTTP_SIZE_KEY, size, found);

        if (found) {
            std::istringstream iss(size);
            iss >> size_in_megabytes;
        } else {
            stringstream msg;
            msg << "HttpCache - The BES Key " << HTTP_SIZE_KEY << " is not set.";
            BESDEBUG(HTTP_MODULE, msg.str() << endl);
            throw BESInternalError(msg.str(), __FILE__, __LINE__);
        }

        return size_in_megabytes;
    }

    string HttpCache::getCacheDirFromConfig() {
        bool found;
        string subdir = "";
        TheBESKeys::TheKeys()->get_value(HTTP_DIR_KEY, subdir, found);

        if (!found) {
            stringstream msg;
            msg << "HttpCache - The BES Key " << HTTP_DIR_KEY << " is not set.";
            BESDEBUG(HTTP_MODULE, msg.str() << endl);
            throw BESInternalError(msg.str(), __FILE__, __LINE__);
        }

        return subdir;
    }

    string HttpCache::getCachePrefixFromConfig() {
        bool found;
        string prefix = "";
        TheBESKeys::TheKeys()->get_value(HTTP_PREFIX_KEY, prefix, found);

        if (found) {
            prefix = BESUtil::lowercase(prefix);
        } else {
            stringstream msg;
            msg << "HttpCache - The BES Key " << HTTP_PREFIX_KEY << " is not set.";
            BESDEBUG(HTTP_MODULE, msg.str() << endl);
            throw BESInternalError(msg.str(), __FILE__, __LINE__);
        }

        return prefix;
    }

    HttpCache::HttpCache() {
        BESDEBUG(HTTP_MODULE, "HttpCache::HttpCache() -  BEGIN" << endl);

        string cacheDir = getCacheDirFromConfig();
        string cachePrefix = getCachePrefixFromConfig();
        unsigned long cacheSizeMbytes = getCacheSizeFromConfig();

        BESDEBUG(HTTP_MODULE, "HttpCache() - Cache configuration params: " << cacheDir << ", " << cachePrefix << ", "
                                                                           << cacheSizeMbytes << endl);
        initialize(cacheDir, cachePrefix, cacheSizeMbytes);

        BESDEBUG(HTTP_MODULE, "HttpCache::HttpCache() -  END" << endl);
    }

#if 1
    HttpCache::HttpCache(const string &cache_dir, const string &prefix, unsigned long long size) {

        BESDEBUG(HTTP_MODULE, "HttpCache::HttpCache() -  BEGIN" << endl);

        initialize(cache_dir, prefix, size);

        BESDEBUG(HTTP_MODULE, "HttpCache::HttpCache() -  END" << endl);
    }
#endif
#if 0
    HttpCache *
    HttpCache::get_instance(const string &cache_dir, const string &cache_file_prefix,
                                 unsigned long long max_cache_size) {
        if (d_enabled && d_instance == 0) {
            if (dir_exists(cache_dir)) {
                d_instance = new HttpCache(cache_dir, cache_file_prefix, max_cache_size);
                d_enabled = d_instance->cache_enabled();
                if (!d_enabled) {
                    delete d_instance;
                    d_instance = 0;
                    BESDEBUG(HTTP_MODULE, "HttpCache::" << __func__ << "() - " << "Cache is DISABLED" << endl);
                } else {
                    AT_EXIT(delete_instance);

                    BESDEBUG(HTTP_MODULE, "HttpCache::" << __func__ << "() - " << "Cache is ENABLED" << endl);
                }
            }
        }

        return d_instance;
    }
#endif

/** Get the default instance of the HttpdCatalogCache object. This will read "TheBESKeys" looking for the values
 * of SUBDIR_KEY, PREFIX_KEY, an SIZE_KEY to initialize the cache.
 */
    HttpCache *
    HttpCache::get_instance() {
        if (d_enabled && d_instance == 0) {
            try {
                d_instance = new HttpCache();
                d_enabled = d_instance->cache_enabled();
                if (!d_enabled) {
                    delete d_instance;
                    d_instance = 0;
                    BESDEBUG(HTTP_MODULE, "HttpCache::" << __func__ << "() - " << "Cache is DISABLED" << endl);
                } else {
                    AT_EXIT(delete_instance);

                    BESDEBUG(HTTP_MODULE, "HttpCache::" << __func__ << "() - " << "Cache is ENABLED" << endl);
                }
            }
            catch (BESInternalError &bie) {
                BESDEBUG(HTTP_MODULE,
                         "[ERROR] HttpCache::get_instance(): Failed to obtain cache! msg: " << bie.get_message()
                                                                                                 << endl);
            }
        }

        return d_instance;
    }

} // namespace remote_cache
