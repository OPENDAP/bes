
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

#include "config.h"

#include <string>
#include <fstream>
#include <sstream>
#include <sys/stat.h>

#include "BESInternalError.h"
#include "BESUtil.h"
#include "BESDebug.h"
#include "TheBESKeys.h"
#include "BESUncompressCache.h"

using std::endl;
using std::string;

BESUncompressCache *BESUncompressCache::d_instance = 0;
bool BESUncompressCache::d_enabled = true;

const string BESUncompressCache::DIR_KEY = "BES.UncompressCache.dir";
const string BESUncompressCache::PREFIX_KEY = "BES.UncompressCache.prefix";
const string BESUncompressCache::SIZE_KEY = "BES.UncompressCache.size";

#define MODULE "cache"
#define prolog std::string("BESUncompressCache::").append(__func__).append("() - ")

unsigned long BESUncompressCache::getCacheSizeFromConfig()
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
        string msg = prolog+ "The BES Key " + SIZE_KEY
            + " is not set! It MUST be set to utilize the decompression cache. ";
        BESDEBUG( MODULE, msg << endl);
        throw BESInternalError(msg, __FILE__, __LINE__);
    }
    return size_in_megabytes;
}

string BESUncompressCache::getCacheDirFromConfig()
{
    bool found;
    string subdir = "";
    TheBESKeys::TheKeys()->get_value(DIR_KEY, subdir, found);

    if (!found) {
        string msg = prolog + "The BES Key " + DIR_KEY
            + " is not set! It MUST be set to utilize the decompression cache. ";
        BESDEBUG( MODULE, msg << endl);
        throw BESInternalError(msg, __FILE__, __LINE__);
    }

    return subdir;
}

string BESUncompressCache::getCachePrefixFromConfig()
{
    bool found;
    string prefix = "";
    TheBESKeys::TheKeys()->get_value(PREFIX_KEY, prefix, found);
    if (found) {
        prefix = BESUtil::lowercase(prefix);
    }
    else {
        string msg = prolog + "The BES Key " + PREFIX_KEY
            + " is not set! It MUST be set to utilize the decompression cache. ";
        BESDEBUG( MODULE, msg << endl);
        throw BESInternalError(msg, __FILE__, __LINE__);
    }

    return prefix;
}

/**
 * @brief Build the name of file that will holds the uncompressed data from
 * 'src' in the cache.
 *
 * Overrides the generic file name generator in BESFileLocking cache.
 * Because this is the uncompress cache, we know that our job is
 * to simply decompress the file and hand it off to the appropriate
 * response handler for the associated file type. Since the state of
 * file's "compressedness" is determined in ALL cases by suffix on the
 * file name (or resource ID if you wish) we know that in addition to
 * building the generic name we want to remove the compression suffix
 * so that the resulting name/id (unmangled if previously mangled)
 * will correctly match the BES TypeMatch regex system.
 *
 *
 * @note How names are mangled: 'src' is the full name of the file to be
 * cached.The file name passed has an extension on the end that will be
 * stripped once the file is cached. For example, if the full path to the
 * file name is /usr/lib/data/fnoc1.nc.gz then the resulting file name
 * will be \#&lt;prefix&gt;\#usr\#lib\#data\#fnoc1.nc.
 *
 * @param src The source name to cache
 * @param mangle if True, assume the name is a file pathname and mangle it.
 * If false, do not mangle the name (assume the caller has sent a suitable
 * string) but do turn the string into a pathname located in the cache directory
 * with the cache prefix. the 'mangle' param is true by default.
 */
string BESUncompressCache::get_cache_file_name(const string &src, bool mangle)
{
    string cache_file_name = src;

    if (mangle) {
        string::size_type last_dot = cache_file_name.rfind('.');
        if (last_dot != string::npos) {
            cache_file_name = cache_file_name.substr(0, last_dot);
        }
    }
    cache_file_name = BESFileLockingCache::get_cache_file_name(cache_file_name);

    BESDEBUG( MODULE, prolog << "cache_file_name:      '" << cache_file_name << "'" << endl);

    return cache_file_name;
}

BESUncompressCache::BESUncompressCache()
{
    BESDEBUG( MODULE, prolog << "BEGIN" << endl);

    d_enabled = true;
    d_dimCacheDir = getCacheDirFromConfig();
    d_dimCacheFilePrefix = getCachePrefixFromConfig();
    d_maxCacheSize = getCacheSizeFromConfig();

    BESDEBUG( MODULE, prolog << "Cache configuration params: " << d_dimCacheDir << ", " << d_dimCacheFilePrefix << ", " << d_maxCacheSize << endl);

    initialize(d_dimCacheDir, d_dimCacheFilePrefix, d_maxCacheSize);

    BESDEBUG( MODULE, prolog << "END" << endl);

}

/** Get the default instance of the GatewayCache object. This will read "TheBESKeys" looking for the values
 * of SUBDIR_KEY, PREFIX_KEY, an SIZE_KEY to initialize the cache.
 * @note if fct is called without cache dir set code with throw internal error
 */
BESUncompressCache *
BESUncompressCache::get_instance()
{
    static BESUncompressCache cache;
    if (cache.cache_enabled()){
        return &cache;
    }
    else{
        return nullptr;
    }
#if 0
    if (d_enabled && d_instance == 0) {
        static BESUncompressCache cache;
        d_enabled = d_instance->cache_enabled();
        if(!d_enabled){
            delete d_instance;
            d_instance = NULL;
            BESDEBUG( MODULE, prolog << "Cache is DISABLED"<< endl);
        }
        else {
            BESDEBUG( MODULE, prolog << "Cache is ENABLED"<< endl);
        }
    }

    return d_instance;
#endif
}

BESUncompressCache::~BESUncompressCache() {}

/**
 * Is the item named by cache_entry_name valid? This code tests that the
 * cache entry is non-zero in size (returns false if that is the case, although
 * that might not be correct) and that the dataset associated with this
 * ResponseBulder instance is at least as old as the cached entry.
 *
 * @note This method id used only by the compressT unit test. jhrg 2/17/18
 *
 * @param cache_file_name File name of the cached entry
 * @param local_id The id, relative to the BES Catalog/Data root of the source dataset.
 * @return True if the thing is valid, false otherwise.
 */
bool BESUncompressCache::is_valid(const string &cache_file_name, const string &local_id)
{
    // If the cached response is zero bytes in size, it's not valid.
    // (hmmm...)
    string datasetFileName = BESUtil::assemblePath(d_dataRootDir, local_id, true);

    off_t entry_size = 0;
    time_t entry_time = 0;
    struct stat buf;
    if (stat(cache_file_name.c_str(), &buf) == 0) {
        entry_size = buf.st_size;
        entry_time = buf.st_mtime;
    }
    else {
        return false;
    }

    if (entry_size == 0) return false;

    time_t dataset_time = entry_time;
    if (stat(datasetFileName.c_str(), &buf) == 0) {
        dataset_time = buf.st_mtime;
    }

    // Trick: if the d_dataset is not a file, stat() returns error and
    // the times stay equal and the code uses the cache entry.

    // TODO Fix this so that the code can get a LMT from the correct handler.
    // TODO Consider adding a getLastModified() method to the libdap::DDS object to support this
    // TODO The DDS may be expensive to instantiate - I think the handler may be a better location
    // for an LMT method, if we can access the handler when/where needed.
    if (dataset_time > entry_time) return false;

    return true;
}

