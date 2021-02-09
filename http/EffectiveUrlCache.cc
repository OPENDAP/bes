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

#include "config.h"

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include <mutex>

#include <sstream>
#include <string>

#include "EffectiveUrlCache.h"

#include "BESSyntaxUserError.h"
#include "TheBESKeys.h"
#include "BESDebug.h"
#include "BESStopWatch.h"
#include "BESUtil.h"
#include "BESLog.h"
#include "CurlUtils.h"
#include "HttpNames.h"
#include "EffectiveUrl.h"

using namespace std;

#define MODULE "euc"
#define MODULE_DUMPER "euc:dump"
#define prolog std::string("EffectiveUrlCache::").append(__func__).append("() - ")

namespace http {

EffectiveUrlCache *EffectiveUrlCache::d_instance = 0;
static std::once_flag d_euc_init_once;

/** @brief Get the singleton BESCatalogList instance.
 *
 * This static method returns the instance of this singleton class. It
 * uses the protected constructor below to read the name of the default
 * catalog from the BES's configuration file, using the key "BES.Catalog.Default".
 * If the key is not found or the key lookup fails for any reason, it
 * uses the the value of BES_DEFAULT_CATALOG as defined in this class'
 * header file (currently the confusing name "catalog").
 *
 * The implementation will only build one instance of CatalogList and
 * thereafter simple return that pointer.
 *
 * For this code, the default catalog is implemented suing CatalogDirectory,
 * which exposes the BES's local POSIX file system, rooted at a place set in
 * the BES configuration file.
 *
 * @return A pointer to the CatalogList singleton
 */
EffectiveUrlCache *
EffectiveUrlCache::TheCache()
{
    std::call_once(d_euc_init_once,EffectiveUrlCache::initialize_instance);

    return d_instance;
}

/**
 * private static that only get's called once by using pthread_once and
 * pthread_once_t mutex.
 */
void EffectiveUrlCache::initialize_instance()
{

    d_instance = new EffectiveUrlCache;
#ifdef HAVE_ATEXIT
    atexit(delete_instance);
#endif

}

/**
 * Private static function can only be called by friends and pThreads code.
 */
void EffectiveUrlCache::delete_instance()
{
    delete d_instance;
    d_instance = 0;
}

/** @brief construct a catalog list
 *
 * @see BESCatalog
 */
EffectiveUrlCache::EffectiveUrlCache(): d_skip_regex(nullptr), d_enabled(-1)
{
    //if (pthread_mutex_init(&d_get_effective_url_cache_mutex, 0) != 0)
    //    throw BESInternalError("Could not initialize mutex in CurlHandlePool", __FILE__, __LINE__);

}

/** @brief list destructor deletes all cached http::urls
 *
 * @see BESCatalog
 */
EffectiveUrlCache::~EffectiveUrlCache()
{
    map<string , http::EffectiveUrl *>::iterator it;
    for(it = d_effective_urls.begin(); it!= d_effective_urls.end(); it++){
        delete it->second;
    }
    d_effective_urls.clear();

    if(d_skip_regex){
        delete d_skip_regex;
        d_skip_regex = 0;
    }
}


/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance along with the catalogs
 * registered in this list.
 *
 * @param strm C++ i/o stream to dump the information to
 */
void EffectiveUrlCache::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << prolog << "(this: " << (void *) this << ")" << endl;
    BESIndent::Indent();
    strm << BESIndent::LMarg << "d_skip_regex: " << (d_skip_regex?d_skip_regex->pattern():"WAS NOT SET") << endl;
    if (!d_effective_urls.empty()) {
        strm << BESIndent::LMarg << "effective url list:" << endl;
        BESIndent::Indent();
        auto it = d_effective_urls.begin();
        while( it!= d_effective_urls.end()){
            strm << BESIndent::LMarg << (*it).first << " --> " << (*it).second->str();
            it++;
        }
        BESIndent::UnIndent();
    }
    else {
        strm << BESIndent::LMarg << "effective url list: EMPTY" << endl;
    }
    BESIndent::UnIndent();
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance along with the catalogs
 * registered in this list.
 *
 * @param strm C++ i/o stream to dump the information to
 */
string EffectiveUrlCache::dump() const
{
    stringstream sstrm;
    dump(sstrm);
    return sstrm.str();
}


/**
 *
 * @param source_url
 */
http::EffectiveUrl *EffectiveUrlCache::get(const std::string  &source_url){
    http::EffectiveUrl *effective_url=NULL;
    auto it = d_effective_urls.find(source_url);
    if(it!=d_effective_urls.end()){
        effective_url = (*it).second;
    }
    return effective_url;
}


//########################################################################################
//########################################################################################
//########################################################################################


/**
 * Find the terminal (effective) url for the source_url. If the source_url matches the
 * skip_regex then it will not be cached.
 *
 * @param source_url
 * @returns The effective URL
*/
string EffectiveUrlCache::get_effective_url(const string &source_url)
{

    // This lock is a RAII implementation. It will block until the mutex is available
    // the lock will be released when the instance of std::lock_guard is destroyed.
    std::lock_guard<std::mutex> lock_me(d_euc_cache_lock_mutex);

    BESDEBUG(MODULE, prolog << "BEGIN url: " << source_url << endl);
    string effective_url_str = source_url;

    if(is_enabled()){

        BESDEBUG(MODULE_DUMPER, prolog << "dump: " << endl << dump() << endl);

        size_t match_length=0;

        // if it's not an HTTP url there is nothing to cache.
        if (source_url.find("http://") != 0 && source_url.find("https://") != 0) {
            BESDEBUG(MODULE, prolog << "END Not an HTTP request, SKIPPING." << endl);
            return effective_url_str;
        }

        BESRegex *skip_regex = get_skip_regex();
        if( skip_regex ) {
            match_length = skip_regex->match(source_url.c_str(), source_url.length());
            if (match_length == source_url.length()) {
                BESDEBUG(MODULE, prolog << "END Candidate url matches the "
                                           "no_redirects_regex_pattern [" << skip_regex->pattern() <<
                                        "][match_length=" << match_length << "] SKIPPING." << endl);
                return effective_url_str;
            }
            BESDEBUG(MODULE, prolog << "Candidate url: '" << source_url << "' does NOT match the "
                                                                           "skip_regex pattern [" << skip_regex->pattern() << "]" << endl);
        }
        else {
            BESDEBUG(MODULE, prolog << "The cache_effective_urls_skip_regex() was NOT SET "<< endl);
        }

        http::EffectiveUrl *effective_url = get(source_url);

        // See if the data_access_url has already been processed into a terminal URL
        bool retrieve_and_cache = !effective_url; // If there's no effective_url we gotta go get it.
        if(effective_url){
            BESDEBUG(MODULE, prolog << "Cache hit for: " << source_url << endl);
            retrieve_and_cache = effective_url->is_expired();
            BESDEBUG(MODULE, prolog << "Cached target URL is " << (retrieve_and_cache?"":"not ") << "expired." << endl);
        }
        // It not found or expired, reload.
        if(retrieve_and_cache){
            BESDEBUG(MODULE, prolog << "Acquiring effective URL for  " << source_url << endl);

            {
                BESStopWatch sw;
                if(BESDebug::IsSet(MODULE) || BESDebug::IsSet(TIMING_LOG_KEY)) sw.start(prolog + "Retrieve and cache effective url for source url: " + source_url);
                effective_url = curl::retrieve_effective_url(source_url);
            }
            BESDEBUG(MODULE, prolog << "   source_url: " << source_url << endl);
            BESDEBUG(MODULE, prolog << "effective_url: " << effective_url->dump() << endl);

            d_effective_urls[source_url] = effective_url;

            BESDEBUG(MODULE, prolog << "Updated record for "<< source_url << " cache size: " << d_effective_urls.size() << endl);
        }
        effective_url_str = effective_url->str();
        BESDEBUG(MODULE_DUMPER, prolog << "dump: " << endl << dump() << endl);
    } // EucLock dat_lock is released when the point of execution reaches this brace and dat_lock goes out of scope.
    else {
        BESDEBUG(MODULE, prolog << "CACHE IS DISABLED." << endl);
    }
    BESDEBUG(MODULE, prolog << "END" << endl);
    return effective_url_str;
}


/**
 *
 * @return
 */
bool EffectiveUrlCache::is_enabled()
{
    // The first time here, the value of d_enabled is -1. Once we check for it in TheBESKeys
    // The value will be 0 (false) or 1 (true) and TheBESKeys will not be checked again.
    if(d_enabled < 0){
        bool found;
        string value;
        TheBESKeys::TheKeys()->get_value(HTTP_CACHE_EFFECTIVE_URLS_KEY,value,found);
        BESDEBUG(MODULE, prolog << HTTP_CACHE_EFFECTIVE_URLS_KEY <<":  '" << value << "'" << endl);
        d_enabled = found && BESUtil::lowercase(value)=="true";
    }
    BESDEBUG(MODULE, prolog << "d_enabled: " << (d_enabled?"true":"false") << endl);
    return d_enabled;
}

/**
 *
 * @return
 */
BESRegex *EffectiveUrlCache::get_skip_regex()
{
    if(!d_skip_regex){
        bool found;
        string value;
        TheBESKeys::TheKeys()->get_value(HTTP_CACHE_EFFECTIVE_URLS_SKIP_REGEX_KEY, value, found);
        if(found && value.length()){
            BESDEBUG(MODULE, prolog << HTTP_CACHE_EFFECTIVE_URLS_SKIP_REGEX_KEY <<":  " << value << endl);
            d_skip_regex = new BESRegex(value.c_str());
        }
    }
    BESDEBUG(MODULE, prolog << "d_skip_regex:  " << (d_skip_regex?d_skip_regex->pattern():"Value has not been set.") << endl);
    return d_skip_regex;
}





} // namespace http