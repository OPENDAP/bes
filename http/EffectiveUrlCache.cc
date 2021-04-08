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
#include <cstdlib>
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

EffectiveUrlCache *EffectiveUrlCache::d_instance = nullptr;
static std::once_flag d_euc_init_once;

/** @brief Get the singleton EffectiveUrlCache instance.
 *
 * This static method returns the instance of this singleton class.
 * The implementation will only build one instance of EffectiveUrlCache and
 * thereafter simple return that pointer.
 *
 * @return A pointer to the EffectiveUrlCache singleton
 */
EffectiveUrlCache *
EffectiveUrlCache::TheCache()
{
    std::call_once(d_euc_init_once,EffectiveUrlCache::initialize_instance);

    return d_instance;
}

/**
 * private static that only get's called once by using std::call_once() and
 * std::mutex.
 */
void EffectiveUrlCache::initialize_instance()
{

    d_instance = new EffectiveUrlCache;
#ifdef HAVE_ATEXIT
    atexit(delete_instance);
#endif

}

/**
 * Private static function can only be called by friends.
 */
void EffectiveUrlCache::delete_instance()
{
    delete d_instance;
    d_instance = 0;
}


/** @brief list destructor deletes all cached http::urls
 *
 * @see BESCatalog
 */
EffectiveUrlCache::~EffectiveUrlCache()
{
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
shared_ptr<http::EffectiveUrl> EffectiveUrlCache::get_cached_eurl(string const &url_key){
    shared_ptr<http::EffectiveUrl> effective_url(nullptr);
    auto it = d_effective_urls.find(url_key);
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
shared_ptr<http::EffectiveUrl> EffectiveUrlCache::get_effective_url(shared_ptr<http::url> source_url) {

    // This lock is a RAII implementation. It will block until the mutex is
    // available and the lock will be released when the instance is destroyed.
    std::lock_guard<std::mutex> lock_me(d_cache_lock_mutex);

    BESDEBUG(MODULE, prolog << "BEGIN url: " << source_url->str() << endl);
    BESDEBUG(MODULE_DUMPER, prolog << "dump: " << endl << dump() << endl);

    if (!is_enabled()) {
        BESDEBUG(MODULE, prolog << "CACHE IS DISABLED." << endl);
        return shared_ptr<http::EffectiveUrl>(new http::EffectiveUrl(source_url));

    }

    // if it's not an HTTP url there is nothing to cache.
    if (source_url->str().find(HTTP_PROTOCOL) != 0 && source_url->str().find(HTTPS_PROTOCOL) != 0) {
        BESDEBUG(MODULE, prolog << "END Not an HTTP request, SKIPPING." << endl);
        return shared_ptr<http::EffectiveUrl>(new http::EffectiveUrl(source_url));
    }

    BESRegex *skip_regex = get_skip_regex();
    if( skip_regex ) {
        size_t match_length = 0;
        match_length = skip_regex->match(source_url->str().c_str(), source_url->str().length());
        if (match_length == source_url->str().length()) {
            BESDEBUG(MODULE, prolog << "END Candidate url matches the "
                                       "no_redirects_regex_pattern [" << skip_regex->pattern() <<
                                    "][match_length=" << match_length << "] SKIPPING." << endl);
            return shared_ptr<http::EffectiveUrl>(new http::EffectiveUrl(source_url));
        }
        BESDEBUG(MODULE, prolog << "Candidate url: '" << source_url->str() << "' does NOT match the "
                                                                              "skip_regex pattern [" << skip_regex->pattern() << "]" << endl);
    }
    else {
        BESDEBUG(MODULE, prolog << "The cache_effective_urls_skip_regex() was NOT SET "<< endl);
    }

    shared_ptr<http::EffectiveUrl> effective_url = get_cached_eurl(source_url->str());

    // If the source_url does not have an associated EffectiveUrl instance in the cache
    // the we know we have to get one.
    bool retrieve_and_cache = !effective_url;

    // But, if there is a value in the cache, we must check to see
    // if it is expired, in which case we will retrive and cache it.
    if(effective_url){
        // It was in the cache. w00t. But, is it expired?.
        BESDEBUG(MODULE, prolog << "Cache hit for: " << source_url->str() << endl);
        retrieve_and_cache = effective_url->is_expired();
        BESDEBUG(MODULE, prolog << "Cached target URL is " << (retrieve_and_cache?"":"not ") << "expired." << endl);
    }

    // It not found or expired, reload.
    if(retrieve_and_cache){
        BESDEBUG(MODULE, prolog << "Acquiring effective URL for  " << source_url->str() << endl);
        {
            BESStopWatch sw;
            if(BESDebug::IsSet(MODULE) || BESDebug::IsSet(TIMING_LOG_KEY))
                sw.start(prolog + "Retrieve and cache effective url for source url: " + source_url->str());
            effective_url = curl::retrieve_effective_url(source_url);
        }
        BESDEBUG(MODULE, prolog << "   source_url: " << source_url->str() << " (" << (source_url->is_trusted()?"":"NOT ") << "trusted)" << endl);
        BESDEBUG(MODULE, prolog << "effective_url: " << effective_url->dump() << " (" << (source_url->is_trusted()?"":"NOT ") << "trusted)" << endl);

        d_effective_urls[source_url->str()] = effective_url;

        BESDEBUG(MODULE, prolog << "Updated record for "<< source_url->str() << " cache size: " << d_effective_urls.size() << endl);

        // Since we don't want there to be a concurrency issue when we release the lock, we don't
        // return the instance of shared_ptr<EffectiveUrl> that we placed in the cache. Rather
        // we make a clone and return that. It will have it's own lifecycle independent of
        // the instance we placed in the cache - it can be modified and the one in the cache
        // is unchanged. Trusted state was established from source_url when effective_url was
        // created in curl::retrieve_effective_url()
        effective_url = shared_ptr<EffectiveUrl>(new EffectiveUrl(effective_url));
    }
    else {
        // Here we have a !expired instance of a shared_ptr<EffectiveUrl> retrieved from the cache.
        // Now we need to make a copy to return, inheriting trust from the
        // requesting URL.
        effective_url = shared_ptr<EffectiveUrl>(new EffectiveUrl(effective_url,source_url->is_trusted()));
    }

    BESDEBUG(MODULE_DUMPER, prolog << "dump: " << endl << dump() << endl);

    BESDEBUG(MODULE, prolog << "END" << endl);

    return effective_url;
}// The lock is released when the point of execution reaches this brace and lock_me goes out of scope.


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