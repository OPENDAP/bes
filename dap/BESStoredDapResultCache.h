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

#include <DapXmlNamespaces.h>   // needed for libdap::DAPVersion
//#include <DMR.h>

#include "BESFileLockingCache.h"

#undef DAP2_STORED_RESULTS

namespace libdap {
class DDS;
class ConstraintEvaluator;
class BaseTypeFactory;

class DMR;
}

class BESDapResponseBuilder;

/**
 * This class is used to cache DAP2 response objects.
 * @author jhrg 5/3/13
 */

class BESStoredDapResultCache: public BESFileLockingCache {
private:
    static bool d_enabled;
    static BESStoredDapResultCache *d_instance;
    static void delete_instance()
    {
        delete d_instance;
        d_instance = 0;
    }

    string d_storedResultsSubdir;
    string d_dataRootDir;
    string d_resultFilePrefix;
    unsigned long d_maxCacheSize;

    /** Initialize the cache using the default values for the cache. */
    BESStoredDapResultCache();

    BESStoredDapResultCache(const BESStoredDapResultCache &src);

    bool is_valid(const std::string &cache_file_name, const std::string &dataset);
#ifdef DAP2_STORED_RESULTS
    bool read_dap2_data_from_cache(const string &cache_file_name, libdap::DDS *fdds);
#endif
    bool read_dap4_data_from_cache(const string &cache_file_name, libdap::DMR *dmr);

    friend class StoredDap2ResultTest;
    friend class StoredDap4ResultTest;
    friend class ResponseBuilderTest;

    string get_stored_result_local_id(const string &dataset, const string &ce, libdap::DAPVersion version);

    string getBesDataRootDirFromConfig();
    string getSubDirFromConfig();
    string getResultPrefixFromConfig();
    unsigned long getCacheSizeFromConfig();

protected:

    BESStoredDapResultCache(const string &data_root_dir, const string &stored_results_subdir, const string &prefix,
        unsigned long long size);

public:
    static const string SUBDIR_KEY;
    static const string PREFIX_KEY;
    static const string SIZE_KEY;

    virtual ~BESStoredDapResultCache() { }

    static BESStoredDapResultCache *get_instance(const string &bes_catalog_root_dir,
        const string &stored_results_subdir, const string &prefix, unsigned long long size);
    static BESStoredDapResultCache *get_instance();

#ifdef DAP2_STORED_RESULTS
    libdap::DDS *get_cached_dap2_data_ddx(const std::string &cache_file_name, libdap::BaseTypeFactory *factory, const std::string &dataset);
    // Store the passed DDS to disk as a serialized DAP2 object.
    virtual string store_dap2_result(libdap::DDS &dds, const std::string &constraint, BESDapResponseBuilder *rb,
        libdap::ConstraintEvaluator *eval);
#endif

    libdap::DMR *get_cached_dap4_data(const string &cache_file_name, libdap::D4BaseTypeFactory *factory,
        const string &filename);

    // Store the passed DMR to disk as a serialized DAP4 object.
    virtual string store_dap4_result(libdap::DMR &dmr, const string &constraint, BESDapResponseBuilder *rb);
};

#endif // _bes_store_result_cache_h
