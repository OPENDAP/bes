// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2015 OPeNDAP, Inc
// Author: Nathan Potter <npotter@opendap.org>
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

#ifndef DISPATCH_BESUNCOMPRESSCACHE_H_
#define DISPATCH_BESUNCOMPRESSCACHE_H_

#include <string>
#include <mutex>

#include "BESFileLockingCache.h"

class BESUncompressCache: public BESFileLockingCache {
    friend class uncompressT;
private:
    static bool d_enabled;
    static std::once_flag d_initialize;
#if 0
    static BESUncompressCache * d_instance;
    static void delete_instance()
    {
        delete d_instance;
        d_instance = 0;
    }
#endif

    std::string d_dimCacheDir;
    std::string d_dataRootDir;
    std::string d_dimCacheFilePrefix;
    unsigned long d_maxCacheSize;

    BESUncompressCache() = default;

    bool is_valid(const std::string &cache_file_name, const std::string &dataset_file_name);

    static std::string getCacheDirFromConfig();
    static std::string getCachePrefixFromConfig();
    static unsigned long getCacheSizeFromConfig();

public:
    static const std::string DIR_KEY;
    static const std::string PREFIX_KEY;
    static const std::string SIZE_KEY;

    BESUncompressCache(const BESUncompressCache&) = delete;
    BESUncompressCache& operator=(const BESUncompressCache&) = delete;

    static BESUncompressCache *get_instance();

    std::string get_cache_file_name(const std::string &src, bool mangle = true) override;

    virtual ~BESUncompressCache();
};

#endif /* DISPATCH_BESUNCOMPRESSCACHE_H_ */
