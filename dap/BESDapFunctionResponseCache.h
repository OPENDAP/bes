// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of Hyrax, A C++ implementation of the OPeNDAP Data
// Access Protocol.

// Copyright (c) 2016 OPeNDAP, Inc.
// Author: Nathan David Potter <ndp@opendap.org>
//         James Gallagher <jgallagher@opendap.org>
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

#ifndef _bes_dap_function_response_cache_h
#define _bes_dap_function_response_cache_h

#include <string>

#include "BESFileLockingCache.h"

class BESDapResponseBuilder;

namespace libdap {
class DDS;
class ConstraintEvaluator;
class BaseTypeFactory;
}

/**
 * @brief Cache the results from server functions.
 *
 * Serve-side functions build new datasets and can be quite large. This code
 * caches those results so that when clients ask for a suite of responses from
 * the function calls, the computations are run only once (in the best case)
 * and subsequent requests for data or metadata are satisfied using information
 * in this case.
 *
 * @note Cache entry collisions: This cache must hold objects that are identified
 * by the combination of a dataset and a constraint expression. The CE can be
 * quite large and contain a number of 'special' characters like '()' and so on.
 * Instead of building cache IDs using a simple concatenation of the dataset
 * and CE, we use the C++ std::hash class to generate a hash code. However, it's
 * possible that two different dataset/CE combinations will have the same hash
 * values. We use a simple collision resolution system where, when a collision
 * is detected, a suffix is appended to the hash value. After a number of
 * collision, we give up and simply do not cache the response (providing no worse
 * performance than if the cache did not exist).
 *
 * @note Cache entry format: The cache uses a specially formated 'response object'
 * that is more efficient to read and write than a typical DAP2 or DAP4 response
 * object. DAP2 serializes data using network byte order while the cache uses
 * native machine order. DAP4 computes checksums; the cache does not.
 *
 * @author jhrg
 */

class BESDapFunctionResponseCache: public BESFileLockingCache {
private:
    static BESDapFunctionResponseCache *d_instance;

    /**
     * Called by atexit()
     */
    static void delete_instance() {
        delete d_instance;
        d_instance = 0;
    }

    /** @name Suppressed constructors
     *  Do not use.
     */
    ///@{
    BESDapFunctionResponseCache();
    BESDapFunctionResponseCache(const BESDapFunctionResponseCache &src);
    ///@}

    bool is_valid(const std::string &cache_file_name, const std::string &dataset);

    std::string get_resource_id(libdap::DDS *dds, const std::string &constraint);

    libdap::DDS *read_cached_data(/*FILE **/ istream &cached_data);

    bool write_dataset_to_cache(libdap::DDS **dds, const string &resourceId, const string &constraint,
        libdap::ConstraintEvaluator *eval, const string &cache_file_name);
    libdap::DDS *load_from_cache(const string &resource_id, const string &cache_file_name);

    friend class FunctionResponseCacheTest;
    friend class StoredResultTest;

protected:
    /**
     * @brief Protected constructor that takes as arguments keys to the cache directory,
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
    BESDapFunctionResponseCache(const string &cache_dir, const string &prefix, unsigned long long size) :
        BESFileLockingCache(cache_dir, prefix, size)
    {
    }

public:
    static const string PATH_KEY;
    static const string PREFIX_KEY;
    static const string SIZE_KEY;

    static BESDapFunctionResponseCache *get_instance(const string &cache_dir, const string &prefix, unsigned long long size);
    static BESDapFunctionResponseCache *get_instance();

    virtual ~BESDapFunctionResponseCache()
    {
    }

    // If the DDS is in the cache and valid, return it otherwise, build the dds, cache it and return it.
    virtual std::string get_or_cache_dataset(libdap::DDS **dds, const std::string &constraint,
        libdap::ConstraintEvaluator *eval);

    virtual bool can_be_cached(libdap::DDS *dds, const std::string &constraint);

    static string getCacheDirFromConfig();
    static string getCachePrefixFromConfig();
    static unsigned long getCacheSizeFromConfig();
};

#endif // _bes_dap_response_cache_h
