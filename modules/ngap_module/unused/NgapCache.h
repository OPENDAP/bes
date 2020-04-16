// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of ngap_module, A C++ module that can be loaded in to
// the OPeNDAP Back-End Server (BES) and is able to handle remote requests.

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

/*
 * NgapCache.h
 *
 *  Created on: July, 10 2018
 *      Author: ndp
 */

#ifndef MODULES_CMR_MODULE_NGAPCACHE_H_
#define MODULES_CMR_MODULE_NGAPCACHE_H_

#include "BESFileLockingCache.h"

namespace ngap {

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
class NgapCache: public BESFileLockingCache
{
private:
    static bool d_enabled;
    static NgapCache * d_instance;
    static void delete_instance() { delete d_instance; d_instance = 0; }

    NgapCache();
    NgapCache(const NgapCache &src);

    static std::string getCacheDirFromConfig();
    static std::string getCachePrefixFromConfig();
    static unsigned long getCacheSizeFromConfig();

protected:
    NgapCache(const std::string &cache_dir, const std::string &prefix, unsigned long long size);

public:
	static const std::string DIR_KEY;
	static const std::string PREFIX_KEY;
	static const std::string SIZE_KEY;

    // static NgapCache *get_instance(const std::string &cache_dir, const std::string &prefix, unsigned long long size);
    static NgapCache *get_instance();

    virtual std::string get_cache_file_name(const std::string &uid, const std::string &src, bool mangle=true);
    virtual std::string get_cache_file_name(const std::string &src, bool mangle=true);
    inline  std::string get_hash(const std::string &s);

	virtual ~NgapCache() { }
};

} /* namespace ngap */

#endif /* MODULES_CMR_MODULE_NGAPCACHE_H_ */
