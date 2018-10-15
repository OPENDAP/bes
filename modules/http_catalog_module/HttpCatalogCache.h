// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of cmr_module, A C++ module that can be loaded in to
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

#ifndef http_catalog_cache_H_
#define http_catalog_cache_H_

#include "BESFileLockingCache.h"

namespace http_catalog {

/**
 * @brief A cache for content accessed via the CMR.
 *
 * This cache is a simple cache for data files implemented using
 * advisory file locking on a POSIX file system (it is a specialization
 * of BESFileLockingCache).
 *
 * This cache uses the following keys in the bes.conf file:
 * - _CMR.Cache.dir_: The directory where retrieved data files should be stored
 * - _CMR.Cache.prefix_: The item-name prefix for this cache
 * - _CMR.Cache.size_: The size of the cache. Limit the total amount of
 *   data cached to this many MB. If zero, the cache size is unlimited.
 *
 * All of the keys must be defined for this cache (the BES uses several caches
 * and some of them are optional - this cache is not optional).
 *
 */
class HttpCatalogCache: public BESFileLockingCache
{
private:
    static bool d_enabled;
    static HttpCatalogCache * d_instance;
    static void delete_instance() { delete d_instance; d_instance = 0; }

    HttpCatalogCache();
    HttpCatalogCache(const HttpCatalogCache &src);

    static string getCacheDirFromConfig();
    static string getCachePrefixFromConfig();
    static unsigned long getCacheSizeFromConfig();

protected:
    HttpCatalogCache(const string &cache_dir, const string &prefix, unsigned long long size);

public:
	static const string DIR_KEY;
	static const string PREFIX_KEY;
	static const string SIZE_KEY;

    static HttpCatalogCache *get_instance(const string &cache_dir, const string &prefix, unsigned long long size);
    static HttpCatalogCache *get_instance();

    virtual string get_cache_file_name(const string &src, bool mangle=true);
    inline  string get_hash(const string &name);

	virtual ~HttpCatalogCache() { }
};

} /* namespace http_catalog */

#endif /* http_catalog_cache_H_ */
