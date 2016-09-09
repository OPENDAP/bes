// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of libdap, A C++ implementation of the OPeNDAP Data
// Access Protocol.

// Copyright (c) 2013 OPeNDAP, Inc.
// Author: James Gallagher <jgallagher@opendap.org>
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

#ifndef _bes_dap_response_cache_h
#define _bes_dap_response_cache_h

#include <string>

#include "BESFileLockingCache.h"

class BESDapResponseBuilder;

namespace libdap {
class DDS;
class ConstraintEvaluator;
class BaseTypeFactory;
}

/**
 * This class is used to cache DAP2 response objects.
 * @author jhrg 5/3/13
 */
class BESDapResponseCache: public BESFileLockingCache {
private:
    static BESDapResponseCache *d_instance;

    /**
     * Called by atexit().
     */
    static void delete_instance() {
        delete d_instance;
        d_instance = 0;
    }

    BESDapResponseCache();

    BESDapResponseCache(const BESDapResponseCache &src);

    bool is_valid(const std::string &cache_file_name, const std::string &dataset);
    void read_data_from_cache(const string &cache_file_name, libdap::DDS *fdds);
    libdap::DDS *get_cached_data_ddx(const std::string &cache_file_name, libdap::BaseTypeFactory *factory,
        const std::string &dataset_name);

    friend class ResponseCacheTest;
    friend class StoredResultTest;

protected:

    /** @brief Protected constructor that takes as arguments keys to the cache directory,
     * file prefix, and size of the cache to be looked up a configuration file
     *
     * The keys specified are looked up in the specified keys object. If not
     * found or not set correctly then an exception is thrown. I.E., if the
     * cache directory is empty, the size is zero, or the prefix is empty.
     *
     * @param cache_dir_key key to look up in the keys file to find cache dir
     * @param prefix_key key to look up in the keys file to find the cache prefix
     * @param size_key key to look up in the keys file to find the cache size (in MBytes)
     * @throws BESSyntaxUserError if keys not set, cache dir or prefix empty,
     * size is 0, or if cache dir does not exist.
     */
    BESDapResponseCache(const string &cache_dir, const string &prefix, unsigned long long size) :
        BESFileLockingCache(cache_dir, prefix, size)
    {
    }

public:
    static const string PATH_KEY;
    static const string PREFIX_KEY;
    static const string SIZE_KEY;

    static BESDapResponseCache *get_instance(const string &cache_dir, const string &prefix, unsigned long long size);
    static BESDapResponseCache *get_instance();

    virtual ~BESDapResponseCache()
    {
    }

    // If the DDS is in the cache and valid, return it otherwise, build the dds, cache it and return it.
    virtual libdap::DDS *cache_dataset(libdap::DDS &dds, const std::string &constraint, BESDapResponseBuilder *rb,
        libdap::ConstraintEvaluator *eval, std::string &cache_token);

    static string getCacheDirFromConfig();
    static string getCachePrefixFromConfig();
    static unsigned long getCacheSizeFromConfig();
};

#endif // _bes_dap_response_cache_h
