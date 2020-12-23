
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

#include "RemoteHttpResourceCache.h"
#include "HttpdCatalogNames.h"

#ifdef HAVE_ATEXIT
#define AT_EXIT(x) atexit((x))
#else
#define AT_EXIT(x)
#endif

using std::endl;
using std::string;
namespace httpd_catalog {

RemoteHttpResourceCache *RemoteHttpResourceCache::d_instance = 0;
bool RemoteHttpResourceCache::d_enabled = true;

const string RemoteHttpResourceCache::DIR_KEY = "HttpResourceCache.dir";
const string RemoteHttpResourceCache::PREFIX_KEY = "HttpResourceCache.prefix";
const string RemoteHttpResourceCache::SIZE_KEY = "HttpResourceCache.size";

unsigned long RemoteHttpResourceCache::getCacheSizeFromConfig()
{
    bool found;
    string size;
    unsigned long size_in_megabytes = 0;
    TheBESKeys::TheKeys()->get_value(SIZE_KEY, size, found);

    if (found) {
        std::istringstream iss(size);
        iss >> size_in_megabytes;
    }
    else {
        string msg = "HttpdCatalogCache - The BES Key " + SIZE_KEY + " is not set.";
        BESDEBUG(MODULE, msg << endl);
        throw BESInternalError(msg, __FILE__, __LINE__);
    }

    return size_in_megabytes;
}

string RemoteHttpResourceCache::getCacheDirFromConfig()
{
    bool found;
    string subdir = "";
    TheBESKeys::TheKeys()->get_value(DIR_KEY, subdir, found);

    if (!found) {
        string msg = "HttpdCatalogCache - The BES Key " + DIR_KEY + " is not set.";
        BESDEBUG(MODULE, msg << endl);
        throw BESInternalError(msg, __FILE__, __LINE__);
    }

    return subdir;
}

string RemoteHttpResourceCache::getCachePrefixFromConfig()
{
    bool found;
    string prefix = "";
    TheBESKeys::TheKeys()->get_value(PREFIX_KEY, prefix, found);

    if (found) {
        prefix = BESUtil::lowercase(prefix);
    }
    else {
        string msg = "HttpdCatalogCache - The BES Key " + PREFIX_KEY + " is not set.";
        BESDEBUG(MODULE, msg << endl);
        throw BESInternalError(msg, __FILE__, __LINE__);
    }

    return prefix;
}

RemoteHttpResourceCache::RemoteHttpResourceCache()
{
    BESDEBUG(MODULE, "HttpdCatalogCache::HttpdCatalogCache() -  BEGIN" << endl);

    string cacheDir = getCacheDirFromConfig();
    string cachePrefix = getCachePrefixFromConfig();
    unsigned long cacheSizeMbytes = getCacheSizeFromConfig();

    BESDEBUG(MODULE, "HttpdCatalogCache() - Cache configuration params: " << cacheDir << ", " << cachePrefix << ", " << cacheSizeMbytes << endl);

    initialize(cacheDir, cachePrefix, cacheSizeMbytes);

    BESDEBUG(MODULE, "HttpdCatalogCache::HttpdCatalogCache() -  END" << endl);
}

RemoteHttpResourceCache::RemoteHttpResourceCache(const string &cache_dir, const string &prefix, unsigned long long size)
{
    BESDEBUG(MODULE, "HttpdCatalogCache::HttpdCatalogCache() -  BEGIN" << endl);

    initialize(cache_dir, prefix, size);

    BESDEBUG(MODULE, "HttpdCatalogCache::HttpdCatalogCache() -  END" << endl);
}

RemoteHttpResourceCache *
RemoteHttpResourceCache::get_instance(const string &cache_dir, const string &cache_file_prefix, unsigned long long max_cache_size)
{
    if (d_enabled && d_instance == 0) {
        if (dir_exists(cache_dir)) {
            d_instance = new RemoteHttpResourceCache(cache_dir, cache_file_prefix, max_cache_size);
            d_enabled = d_instance->cache_enabled();
            if (!d_enabled) {
                delete d_instance;
                d_instance = 0;
                BESDEBUG(MODULE, "HttpdCatalogCache::"<<__func__ << "() - " << "Cache is DISABLED"<< endl);
            }
            else {
                AT_EXIT(delete_instance);

                BESDEBUG(MODULE, "HttpdCatalogCache::"<<__func__ << "() - " << "Cache is ENABLED"<< endl);
            }
        }
    }

    return d_instance;
}

/** Get the default instance of the HttpdCatalogCache object. This will read "TheBESKeys" looking for the values
 * of SUBDIR_KEY, PREFIX_KEY, an SIZE_KEY to initialize the cache.
 */
RemoteHttpResourceCache *
RemoteHttpResourceCache::get_instance()
{
    if (d_enabled && d_instance == 0) {
        try {
            d_instance = new RemoteHttpResourceCache();
            d_enabled = d_instance->cache_enabled();
            if (!d_enabled) {
                delete d_instance;
                d_instance = 0;
                BESDEBUG(MODULE, "HttpdCatalogCache::"<<__func__ << "() - " << "Cache is DISABLED"<< endl);
            }
            else {
                AT_EXIT(delete_instance);

                BESDEBUG(MODULE, "HttpdCatalogCache::" << __func__ << "() - " << "Cache is ENABLED"<< endl);
            }
        }
        catch (BESInternalError &bie) {
            BESDEBUG(MODULE, "[ERROR] HttpdCatalogCache::get_instance(): Failed to obtain cache! msg: " << bie.get_message() << endl);
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
inline string RemoteHttpResourceCache::get_hash(const string &name)
{
    if (name.empty()) throw BESInternalError("Empty name passed to the Metadata Store.", __FILE__, __LINE__);
    return picosha2::hash256_hex_string(name[0] == '/' ? name : "/" + name);
}

string RemoteHttpResourceCache::get_cache_file_name(const string &src, bool /*mangle*/)
{
    return BESUtil::assemblePath(this->get_cache_directory(), get_cache_file_prefix() + get_hash(src));
}


} // namespqce httpd_catalog
