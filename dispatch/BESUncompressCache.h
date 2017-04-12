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

#include "BESFileLockingCache.h"

class BESUncompressCache: public BESFileLockingCache {
    friend class uncompressT;
private:
    static bool d_enabled;
    static BESUncompressCache * d_instance;
    static void delete_instance()
    {
        delete d_instance;
        d_instance = 0;
    }

    string d_dimCacheDir;
    string d_dataRootDir;
    string d_dimCacheFilePrefix;
    unsigned long d_maxCacheSize;

    BESUncompressCache();
    BESUncompressCache(const BESUncompressCache &src);

    bool is_valid(const std::string &cache_file_name, const std::string &dataset_file_name);

    static string getCacheDirFromConfig();
    static string getCachePrefixFromConfig();
    static unsigned long getCacheSizeFromConfig();

protected:

    BESUncompressCache(const string &data_root_dir, const string &cache_dir, const string &prefix,
        unsigned long long size);

public:
    static const string DIR_KEY;
    static const string PREFIX_KEY;
    static const string SIZE_KEY;

    static BESUncompressCache *get_instance(const string &bes_catalog_root_dir, const string &cache_dir,
        const string &prefix, unsigned long long size);
    static BESUncompressCache *get_instance();

    virtual string get_cache_file_name(const string &src, bool mangle = true);

    virtual ~BESUncompressCache();
};

#endif /* DISPATCH_BESUNCOMPRESSCACHE_H_ */
