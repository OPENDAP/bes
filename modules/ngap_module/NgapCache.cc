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
 * NgapCache.cc
 *
 *  Created on: July, 10 2018
 *      Author: ndp
 */

#include "config.h"

#include <sys/stat.h>

#include <string>
#include <fstream>
#include <sstream>

#include <cstdlib>
#include <BESInternalFatalError.h>

#include "PicoSHA2/picosha2.h"

#include "BESInternalError.h"
#include "BESDebug.h"
#include "BESUtil.h"
#include "TheBESKeys.h"

#include "NgapNames.h"
#include "NgapCache.h"

using std::endl;
using std::string;
using std::stringstream;

#ifdef HAVE_ATEXIT
#define AT_EXIT(x) atexit((x))
#else
#define AT_EXIT(x)
#endif

#define prolog string("NgapCache::").append(__func__).append("() - ")


namespace ngap {


NgapCache *NgapCache::d_instance = 0;
bool NgapCache::d_enabled = true;

const string NgapCache::DIR_KEY = "NGAP.Cache.dir";
const string NgapCache::PREFIX_KEY = "NGAP.Cache.prefix";
const string NgapCache::SIZE_KEY = "NGAP.Cache.size";

unsigned long NgapCache::getCacheSizeFromConfig()
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
        stringstream msg;
        msg <<  prolog <<  "The BES Key " << SIZE_KEY << " is not set.";
        BESDEBUG(MODULE,  msg.str() << endl);
        throw BESInternalError(msg.str(), __FILE__, __LINE__);
    }
    return size_in_megabytes;
}

string NgapCache::getCacheDirFromConfig()
{
    bool found;
    string subdir = "";
    TheBESKeys::TheKeys()->get_value(DIR_KEY, subdir, found);

    if (!found) {
        stringstream msg;
        msg <<  prolog <<  "The BES Key " + DIR_KEY + " is not set.";
        BESDEBUG(MODULE,msg.str() << endl);
        throw BESInternalError(msg.str(), __FILE__, __LINE__);
    }

    return subdir;
}

string NgapCache::getCachePrefixFromConfig()
{
    bool found;
    string prefix = "";
    TheBESKeys::TheKeys()->get_value(PREFIX_KEY, prefix, found);
    if (found) {
        prefix = BESUtil::lowercase(prefix);
    }
    else {
        stringstream msg;
        msg <<  prolog <<  "The BES Key " + PREFIX_KEY + " is not set.";
        BESDEBUG(MODULE,  msg.str() << endl);
        throw BESInternalError(msg.str(), __FILE__, __LINE__);
    }

    return prefix;
}

NgapCache::NgapCache()
{
    BESDEBUG(MODULE, "NgapCache::NgapCache() -  BEGIN" << endl);

    string cacheDir = getCacheDirFromConfig();
    string cachePrefix = getCachePrefixFromConfig();
    unsigned long cacheSizeMbytes = getCacheSizeFromConfig();

    BESDEBUG(MODULE,prolog << "NgapCache() - Cache configuration params: " << cacheDir <<
    ", " << cachePrefix << ", " << cacheSizeMbytes << endl);

    initialize(cacheDir, cachePrefix, cacheSizeMbytes);

    BESDEBUG(MODULE, prolog << "NgapCache::NgapCache() -  END" << endl);
}

NgapCache::NgapCache(const string &cache_dir, const string &prefix, unsigned long long size)
{
    BESDEBUG(MODULE, prolog << "BEGIN" << endl);

    initialize(cache_dir, prefix, size);

    BESDEBUG(MODULE, prolog << "END" << endl);
}

#if 0
NgapCache *
NgapCache::get_instance(const string &cache_dir, const string &cache_file_prefix, unsigned long long max_cache_size)
{
    if (d_enabled && d_instance == 0) {
        if (dir_exists(cache_dir)) {
            d_instance = new NgapCache(cache_dir, cache_file_prefix, max_cache_size);
            d_enabled = d_instance->cache_enabled();
            if (!d_enabled) {
                delete d_instance;
                d_instance = 0;
                BESDEBUG(MODULE, "NgapCache::"<<__func__ << "() - " << "Cache is DISABLED"<< endl);
            }
            else {
                AT_EXIT(delete_instance);

                BESDEBUG(MODULE, "NgapCache::"<<__func__ << "() - " << "Cache is ENABLED"<< endl);
            }
        }
    }

    return d_instance;
}
#endif

/** Get the default instance of the NgapCache object. This will read "TheBESKeys" looking for the values
 * of SUBDIR_KEY, PREFIX_KEY, an SIZE_KEY to initialize the cache.
 */
NgapCache *
NgapCache::get_instance()
{
    if (d_enabled && d_instance == 0) {
        try {
            d_instance = new NgapCache();
            d_enabled = d_instance->cache_enabled();
            if (!d_enabled) {
                delete d_instance;
                d_instance = 0;
                BESDEBUG(MODULE, prolog << "Cache is DISABLED"<< endl);
            }
            else {
                AT_EXIT(delete_instance);

                BESDEBUG(MODULE, prolog << "Cache is ENABLED"<< endl);
            }
        }
        catch (BESInternalError &bie) {
            BESDEBUG(MODULE,prolog << "[ERROR] NgapCache::get_instance(): "
                                      "Failed to obtain cache! msg: " << bie.get_message() << endl);
        }
    }

    return d_instance;
}


/**
 * Compute the SHA256 hash for the string s
 *
 * @param s The string to hash
 * @return The SHA256 hash of the string s.
 */
inline string
NgapCache::get_hash(const string &s)
{
    if (s.empty()){
        string msg = ".";
        BESDEBUG(MODULE, prolog << msg<< endl);
        throw BESInternalError(msg, __FILE__, __LINE__);
    }
    return picosha2::hash256_hex_string(s[0] == '/' ? s : "/" + s);
}

    string NgapCache::get_cache_file_name(const string &uid, const string &src,  bool /*mangle*/){

        string uid_part;
        if(!uid.empty())
            uid_part = uid + "_";

        return  BESUtil::assemblePath(this->get_cache_directory(),
                                      get_cache_file_prefix() + uid_part + get_hash(src));
    }


    string NgapCache::get_cache_file_name( const string &,  bool /*mangle*/){

        string msg = prolog+ "ERROR! THIS METHOD IS NOT TO BE USED IN NGAP.";
        BESDEBUG(MODULE,  msg<< endl);
        throw BESInternalFatalError(msg, __FILE__, __LINE__);

        // return  get_cache_file_name("",src);
    }

} // namespace ngap

