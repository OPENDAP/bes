
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

#ifndef _bes_store_result_cache_h
#define _bes_store_result_cache_h

#include <string>
#include "BESFileLockingCache.h"

class BESDapResponseBuilder;

class libdap::DDS;
class libdap::ConstraintEvaluator;
class libdap::BaseTypeFactory;

/**
 * This class is used to cache DAP2 response objects.
 * @author jhrg 5/3/13
 */

class BESStoredDapResultCache: public BESFileLockingCache
{
private:

    static BESStoredDapResultCache * d_instance;

    /** Initialize the cache using the default values for the cache. */
    BESStoredDapResultCache();

    BESStoredDapResultCache(const BESStoredDapResultCache &src);

    bool is_valid(const std::string &cache_file_name, const std::string &dataset);
    void read_dap2_data_from_cache(const string &cache_file_name/*FILE *data*/, libdap::DDS *fdds);
    libdap::DDS *get_cached_data_ddx(const std::string &cache_file_name, libdap::BaseTypeFactory *factory, const std::string &dataset);

    friend class StoredResultTest;

    static void delete_instance();

    string build_stored_result_file_name(const string &dataset, const string &ce);

protected:

    BESStoredDapResultCache(const string &cache_dir, const string &prefix, unsigned long long size);


public:
	static const string SUBDIR_KEY;
	static const string PREFIX_KEY;
	static const string SIZE_KEY;

    static BESStoredDapResultCache *get_instance(const string &cache_dir, const string &prefix, unsigned long long size);
    static BESStoredDapResultCache *get_instance();

    virtual ~BESStoredDapResultCache() {}

    // If the DDS is in the cache and valid, return it otherwise, build the dds, cache it and return it.
    virtual libdap::DDS *cache_dap2_dataset(libdap::DDS &dds, const std::string &constraint, BESDapResponseBuilder *rb,
    		libdap::ConstraintEvaluator *eval, std::string &cache_token);

    // virtual void unlock_and_close(const std::string &cache_token);

    static string getStoredResultsDirFromConfig();
    static string getSubDirFromConfig();
    static string getResultPrefixFromConfig();
    static unsigned long getCacheSizeFromConfig();

    // Overrides parent
    virtual string get_cache_file_name(const string &src, bool mangle = false);

};

#endif // _bes_store_result_cache_h
