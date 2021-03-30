
// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of the BES http package, part of the Hyrax data server.

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

// Authors:
//      ndp       Nathan Potter <ndp@opendap.org>

#include <config.h>

#include <sys/stat.h>

#include <string>
#include <fstream>
#include <sstream>
#include <vector>

#include <cstdlib>

#include "PicoSHA2/picosha2.h"

#include <BESInternalError.h>
#include <BESDebug.h>
#include <BESUtil.h>
#include <TheBESKeys.h>

#include "HttpCache.h"
#include "HttpUtils.h"
#include "HttpNames.h"
#include "url_impl.h"

#ifdef HAVE_ATEXIT
#define AT_EXIT(x) atexit((x))
#else
#define AT_EXIT(x)
#endif
#define prolog string("HttpCache::").append(__func__).append("() - ")


using std::endl;
using std::string;
using std::vector;
using std::stringstream;

namespace http {

    HttpCache *HttpCache::d_instance = 0;
    bool HttpCache::d_enabled = true;

    unsigned long HttpCache::getCacheSizeFromConfig() {
        bool found = false;
        string size;
        unsigned long size_in_megabytes = 0;
        TheBESKeys::TheKeys()->get_value(HTTP_CACHE_SIZE_KEY, size, found);

        if (found) {
            std::istringstream iss(size);
            iss >> size_in_megabytes;
        } else {
            stringstream msg;
            msg << prolog << "The BES Key " << HTTP_CACHE_SIZE_KEY << " is not set.";
            BESDEBUG(HTTP_MODULE, msg.str() << endl);
            throw BESInternalError(msg.str(), __FILE__, __LINE__);
        }

        return size_in_megabytes;
    }

    unsigned long HttpCache::getCacheExpiresTime() {
        bool found = false;
        string time;
        unsigned long time_in_seconds = 0;
        TheBESKeys::TheKeys()->get_value(HTTP_CACHE_EXPIRES_TIME_KEY, time, found);

        if (found) {
            std::istringstream iss(time);
            iss >> time_in_seconds;
        } else {
            time_in_seconds = REMOTE_RESOURCE_DEFAULT_EXPIRED_INTERVAL;
        }

        return time_in_seconds;
    }

    string HttpCache::getCacheDirFromConfig() {
        bool found;
        string subdir = "";
        TheBESKeys::TheKeys()->get_value(HTTP_CACHE_DIR_KEY, subdir, found);

        if (!found) {
            stringstream msg;
            msg << prolog << "The BES Key " << HTTP_CACHE_DIR_KEY << " is not set.";
            BESDEBUG(HTTP_MODULE, msg.str() << endl);
            throw BESInternalError(msg.str(), __FILE__, __LINE__);
        }

        return subdir;
    }

    string HttpCache::getCachePrefixFromConfig() {
        bool found;
        string prefix = "";
        TheBESKeys::TheKeys()->get_value(HTTP_CACHE_PREFIX_KEY, prefix, found);

        if (found) {
            prefix = BESUtil::lowercase(prefix);
        } else {
            stringstream msg;
            msg << prolog << "The BES Key " << HTTP_CACHE_PREFIX_KEY << " is not set.";
            BESDEBUG(HTTP_MODULE, msg.str() << endl);
            throw BESInternalError(msg.str(), __FILE__, __LINE__);
        }

        return prefix;
    }

    HttpCache::HttpCache() {
        BESDEBUG(HTTP_MODULE, prolog << "BEGIN" << endl);

        string cacheDir = getCacheDirFromConfig();
        string cachePrefix = getCachePrefixFromConfig();
        unsigned long cacheSizeMbytes = getCacheSizeFromConfig();

        BESDEBUG(HTTP_MODULE, prolog << "Cache configuration params: " << cacheDir << ", " << cachePrefix << ", "
                                                                           << cacheSizeMbytes << endl);
        initialize(cacheDir, cachePrefix, cacheSizeMbytes);

        BESDEBUG(HTTP_MODULE, prolog << "END" << endl);
    }

#if 1
    HttpCache::HttpCache(const string &cache_dir, const string &prefix, unsigned long long size) {

        BESDEBUG(HTTP_MODULE, prolog << "BEGIN" << endl);

        initialize(cache_dir, prefix, size);

        BESDEBUG(HTTP_MODULE, prolog << "END" << endl);
    }
#endif
#if 0
    HttpCache *
    HttpCache::get_instance(const string &cache_dir, const string &cache_file_prefix,
                                 unsigned long long max_cache_size) {
        if (d_enabled && d_instance == 0) {
            if (dir_exists(cache_dir)) {
                d_instance = new HttpCache(cache_dir, cache_file_prefix, max_cache_size);
                d_enabled = d_instance->cache_enabled();
                if (!d_enabled) {
                    delete d_instance;
                    d_instance = 0;
                    BESDEBUG(HTTP_MODULE, "HttpCache::" << __func__ << "() - " << "Cache is DISABLED" << endl);
                } else {
                    AT_EXIT(delete_instance);

                    BESDEBUG(HTTP_MODULE, "HttpCache::" << __func__ << "() - " << "Cache is ENABLED" << endl);
                }
            }
        }

        return d_instance;
    }
#endif

/** Get the default instance of the HttpdCatalogCache object. This will read "TheBESKeys" looking for the values
 * of SUBDIR_KEY, PREFIX_KEY, an SIZE_KEY to initialize the cache.
 */
    HttpCache *
    HttpCache::get_instance() {
        if (d_enabled && d_instance == 0) {
            try {
                d_instance = new HttpCache();
                d_enabled = d_instance->cache_enabled();
                if (!d_enabled) {
                    delete d_instance;
                    d_instance = 0;
                    BESDEBUG(HTTP_MODULE, prolog << "Cache is DISABLED" << endl);
                } else {
                    AT_EXIT(delete_instance);

                    BESDEBUG(HTTP_MODULE, prolog << "Cache is ENABLED" << endl);
                }
            }
            catch (BESInternalError &bie) {
                BESDEBUG(HTTP_MODULE,
                         "[ERROR] HttpCache::get_instance(): Failed to obtain cache! msg: " << bie.get_message()
                                                                                                 << endl);
            }
        }

        return d_instance;
    }

#if HASH_CACHE_FILENAME

    string
    HttpCache::get_hash(const string &s)
    {
        if (s.empty()){
            string msg = "You cannot hash the empty string.";
            BESDEBUG(HTTP_MODULE, prolog << msg << endl);
            throw BESInternalError(msg, __FILE__, __LINE__);
        }
        return picosha2::hash256_hex_string(s[0] == '/' ? s : "/" + s);
    }


    bool is_url(const string &candidate){
        size_t index = candidate.find(HTTP_PROTOCOL);
        if(index){
            index = candidate.find(HTTPS_PROTOCOL);
            if(index){
                return false;
            }
        }
        return true;
    }


    /**
     * This helper function looks at the passed identifier and tries to formulate
     * a human readable summary string for use in dataset naming etc.
     *
     * @param identifier A string holding the identifier to summarize.
     * @return A human readable summary string for use in dataset naming etc.
     */
    string get_real_name_extension(const string &identifier){
        string real_name_extension;

        string path_part;

        if(is_url(identifier)) {
            // Since it's a URL it might have a massive query string attached, and since wee
            // have no idea what the query parameters mean, we'll just punt and look at the path part of the URL.
            // We make an instance of http::url which will carve up the URL for us.
            http::url target_url(identifier);
            path_part = target_url.path();
        }
        else {
            path_part = identifier;
        }

        vector<string> path_elements;
        // Now that we a "path" (none of that query string mess) we can tokenize it.
        BESUtil::tokenize(path_part,path_elements);
        if(!path_elements.empty()){
            string last = path_elements.back();
            if(last != path_part)
                real_name_extension = "#" + last; // This utilizes a hack in libdap
        }
        return real_name_extension;
    }


    /**
     * Builds a cache file name that contains a hashed version of the src_id, the user id (uid) if non-empty, and a
     * human readable name that may be utilized when naming datasets whose data are held in the cache file.
     * @param uid The user id of the requesting user.
     * @param src_id The source identifier of the resource to cache.
     * @param mangle If true, the cache file names will be hashed (more or less).
     * @return The name of the cache file based on the inputs.
     */
    string HttpCache::get_cache_file_name(const string &uid, const string &src_id,  bool mangle){
        stringstream cache_filename;
        string hashed_part;
        string real_name_extension;
        string uid_part;

        if(!uid.empty())
            uid_part = uid + "_";

        if(mangle){
            hashed_part = get_hash(src_id);
        }
        else {
            hashed_part = src_id;
        }
        real_name_extension = get_real_name_extension(src_id);

        cache_filename << get_cache_file_prefix() << uid_part << hashed_part << real_name_extension;

        string cf_name =  BESUtil::assemblePath(this->get_cache_directory(), cache_filename.str() );

        return cf_name;
    }


    string HttpCache::get_cache_file_name( const string &src,  bool mangle){
        string uid;
        return  get_cache_file_name(uid,src, mangle);
    }


#endif

} // namespace http
