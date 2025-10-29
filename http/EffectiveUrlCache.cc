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

#include <mutex>

#include <sstream>
#include <string>

#include "TheBESKeys.h"
#include "BESDebug.h"
#include "BESStopWatch.h"
#include "BESUtil.h"
#include "CurlUtils.h"
#include "HttpError.h"
#include "HttpNames.h"
#include "EffectiveUrl.h"
#include "EffectiveUrlCache.h"

#include "rapidjson/document.h"

using namespace std;

constexpr auto MODULE = "euc";
constexpr auto MODULE_TIMER = "euc:timer";
constexpr auto MODULE_DUMPER = "euc:dump";

#define prolog std::string("EffectiveUrlCache::").append(__func__).append("() - ")

namespace http {

/**
 * @brief Get the cached effective URL.
 * @param url_key Key to a cached effective URL.
 * @note This method is not, itself, thread safe.
 */
shared_ptr <EffectiveUrl> EffectiveUrlCache::get_cached_eurl(string const &url_key) {
    shared_ptr<EffectiveUrl> effective_url(nullptr);
    auto it = d_effective_urls.find(url_key);
    if (it != d_effective_urls.end()) {
        effective_url = (*it).second;
    }
    return effective_url;
}

/**
 * Find the terminal (effective) url for the source_url. If the source_url matches the
 * skip_regex then it will not be cached.
 *
 * @param source_url
 * @returns The effective URL
*/
shared_ptr <EffectiveUrl> EffectiveUrlCache::get_effective_url(shared_ptr <url> source_url) {

    BESDEBUG(MODULE, prolog << "BEGIN url: " << source_url->str() << endl);
    BESDEBUG(MODULE_DUMPER, prolog << "dump: " << endl << dump() << endl);

    // Lock access to the cache, the d_effective_urls map. Released when the lock goes out of scope.
    std::lock_guard<std::mutex> lock_me(d_cache_lock_mutex);

    if (!is_enabled()) {
        BESDEBUG(MODULE, prolog << "CACHE IS DISABLED." << endl);
        return shared_ptr<EffectiveUrl>(new EffectiveUrl(source_url));
    }

    // if it's not an HTTP url there is nothing to cache.
    if (source_url->str().find(HTTP_PROTOCOL) != 0 && source_url->str().find(HTTPS_PROTOCOL) != 0) {
        BESDEBUG(MODULE, prolog << "END Not an HTTP request, SKIPPING." << endl);
        return shared_ptr<EffectiveUrl>(new EffectiveUrl(source_url));
    }

    if (!d_skip_regex)
        set_skip_regex();

    if (d_skip_regex) {
        size_t match_length = 0;
        match_length = d_skip_regex->match(source_url->str().c_str(), (int) source_url->str().size());
        if (match_length == source_url->str().size()) {
            BESDEBUG(MODULE, prolog << "END Candidate url matches the "
                                       "no_redirects_regex_pattern [" << d_skip_regex->pattern() <<
                                    "][match_length=" << match_length << "] SKIPPING." << endl);
            return shared_ptr<EffectiveUrl>(new EffectiveUrl(source_url));
        }
        BESDEBUG(MODULE, prolog << "Candidate url: '" << source_url->str()
                                << "' does NOT match the skip_regex pattern [" << d_skip_regex->pattern() << "]"
                                << endl);
    } else {
        BESDEBUG(MODULE, prolog << "The cache_effective_urls_skip_regex() was NOT SET " << endl);
    }

    shared_ptr<EffectiveUrl> effective_url = get_cached_eurl(source_url->str());
    bool retrieve_and_cache = !effective_url || effective_url->is_expired();

    // It not found or expired, (re)load.
    if (retrieve_and_cache) {
        BESDEBUG(MODULE, prolog << "Acquiring effective URL for  " << source_url->str() << endl);
        {
            BES_STOPWATCH_START(MODULE_TIMER, prolog + "Retrieve and cache effective url for source url: " + source_url->str());
            try {
                // This code throws an HttpError exception if there is a problem.
                effective_url = curl::get_redirect_url(source_url);
            }
            catch (http::HttpError &http_error) {
                string err_msg = prolog + "Hyrax encountered a Service Chaining Error while "
                                 "attempting to retrieve a redirect URL.\n"
                                 "This is most likely problem with TEA, the AWS URL "
                                 "signing service.\n" + http_error.get_message();
                http_error.set_message(err_msg);
                throw;
            }
        }
        BESDEBUG(MODULE, prolog << "   source_url: " << source_url->str() << " ("
                                << (source_url->is_trusted() ? "" : "NOT ") << "trusted)" << endl);
        BESDEBUG(MODULE, prolog << "effective_url: " << effective_url->dump() << " ("
                                << (source_url->is_trusted() ? "" : "NOT ") << "trusted)" << endl);

        d_effective_urls[source_url->str()] = effective_url;

        BESDEBUG(MODULE, prolog << "Updated record for " << source_url->str() << " cache size: "
                                << d_effective_urls.size() << endl);

        // Since we don't want there to be a concurrency issue when we release the lock, we don't
        // return the instance of shared_ptr<EffectiveUrl> that we placed in the cache. Rather
        // we make a clone and return that. It will have its own lifecycle independent of
        // the instance we placed in the cache - it can be modified and the one in the cache
        // is unchanged. Trusted state was established from source_url when effective_url was
        // created in curl::retrieve_effective_url()
        effective_url = make_shared<EffectiveUrl>(effective_url);
    } else {
        // Here we have a !expired instance of a shared_ptr<EffectiveUrl> retrieved from the cache.
        // Now we need to make a copy to return, inheriting trust from the requesting URL.
        effective_url = make_shared<EffectiveUrl>(effective_url, source_url->is_trusted());
    }

    BESDEBUG(MODULE_DUMPER, prolog << "dump: " << endl << dump() << endl);
    BESDEBUG(MODULE, prolog << "END" << endl);

    return effective_url;
}

// TODO: docstring
void EffectiveUrlCache::cache_signed_url_components(const std::string &key, const std::string &s3_url, const std::string &s3credentials_url) {
    // TODO: if urls are empty, do....something?? skip it or something
    INFO_LOG(prolog + "Caching signed url components for key: " + key);
    d_href_to_s3_cache[key] = s3_url;
    d_href_to_s3credentials_cache[key] = s3credentials_url;
}

shared_ptr<EffectiveUrlCache::S3AccessKeyTuple> EffectiveUrlCache::get_cache_s3credentials(std::string const &s3credentials_url) {
    //TODO-future: make cache for these credentials, check if they've already been retrieved
    
    // 1. Get the credentials from TEA
    std::string s3credentials_json_string;
    try {
        BES_PROFILE_TIMING(string("Request s3 credentials from TEA - ") + s3credentials_url);
        curl::http_get(s3credentials_url, s3credentials_json_string);
    }
    catch (http::HttpError &http_error) {
        string err_msg = prolog + "Encountered an error while "
                         "attempting to retrieve s3 credentials from TEA. " + http_error.get_message();
        INFO_LOG(err_msg);
        return nullptr;
    }
    if (s3credentials_json_string.empty()) {
        string err_msg = prolog + "Unable to retrieve s3 credentials from TEA endpoint " + s3credentials_url;
        INFO_LOG(err_msg);
        return nullptr;
    }

    // 2. Parse the response to pull out the credentials
    auto credentials = get_s3_credentials_from_tea_endpoint_json(s3credentials_json_string);

    //TODO-future: add these credentials to the cache

    return credentials;
}


// Lightly adapted from get_urls_from_granules_umm_json_v1_4
std::shared_ptr<EffectiveUrlCache::S3AccessKeyTuple> EffectiveUrlCache::get_s3_credentials_from_tea_endpoint_json(std::string const &s3credentials_json_string) {
    rapidjson::Document s3credentials_response;
    s3credentials_response.Parse(s3credentials_json_string.c_str());

    string access_key_id;
    string secret_access_key;
    string session_token;
    string expiration;

    auto itr = s3credentials_response.FindMember("accessKeyId");
    if (itr != s3credentials_response.MemberEnd()) {
        access_key_id = itr->value.GetString();
    }

    itr = s3credentials_response.FindMember("secretAccessKey");
    if (itr != s3credentials_response.MemberEnd()) {
        secret_access_key = itr->value.GetString();
    }

    itr = s3credentials_response.FindMember("sessionToken");
    if (itr != s3credentials_response.MemberEnd()) {
        session_token = itr->value.GetString();
    }

    itr = s3credentials_response.FindMember("expiration");
    if (itr != s3credentials_response.MemberEnd()) {
        expiration = itr->value.GetString();
    }

    if (access_key_id.empty() || secret_access_key.empty() || session_token.empty() || expiration.empty()) {
        return nullptr;
    }

    return make_shared<EffectiveUrlCache::S3AccessKeyTuple>(access_key_id, 
                                                            secret_access_key,
                                                            session_token,
                                                            expiration);
}



/**
 * TODO
 *
 * @param source_url
 * @returns The effective (signed) URL
*/
std::shared_ptr<EffectiveUrl> EffectiveUrlCache::get_signed_url(std::shared_ptr<url> source_url) {
    INFO_LOG(prolog + "GET SIGNED URL NOT YET IMPLEMENTED, falling back to get_effective_url; returns nullptr");

    //TODO-implement! Copy get_effective_url boilerplate, then...

    // 1. Get relevant urls
    auto s3_url = d_href_to_s3_cache[source_url->str()];
    auto s3credentials_url = d_href_to_s3credentials_cache[source_url->str()];
    if (s3_url.empty() || s3credentials_url.empty()) {
        INFO_LOG(prolog + "No url available for TEA s3credentials endpoint.");
        return get_effective_url(source_url);
    }

    // 2. Get values from s3credentials endpoint
    auto s3_access_key_tuple = get_cache_s3credentials(s3credentials_url);
    if (s3_access_key_tuple == nullptr) {
        return get_effective_url(source_url);
    }
    

    // TODO-2: use aws sdk to sign the url

    return nullptr;
}

/**
 * @return Is the cache enabled (set in the bes.conf file)?
 */
bool EffectiveUrlCache::is_enabled() {
    // The first time here, the value of d_enabled is -1. Once we check for it in TheBESKeys
    // The value will be 0 (false) or 1 (true) and TheBESKeys will not be checked again.
    if (d_enabled < 0) {
        string value = TheBESKeys::TheKeys()->read_string_key(HTTP_CACHE_EFFECTIVE_URLS_KEY, "false");
        d_enabled = BESUtil::lowercase(value) == "true";
    }
    BESDEBUG(MODULE, prolog << "d_enabled: " << (d_enabled ? "true" : "false") << endl);
    return d_enabled;
}

void EffectiveUrlCache::set_skip_regex() {
    if (!d_skip_regex) {
        string pattern = TheBESKeys::TheKeys()->read_string_key(HTTP_CACHE_EFFECTIVE_URLS_SKIP_REGEX_KEY, "");
        if (!pattern.empty()) {
            d_skip_regex.reset(new BESRegex(pattern.c_str()));
        }
        BESDEBUG(MODULE, prolog << "d_skip_regex:  "
                                << (d_skip_regex ? d_skip_regex->pattern() : "Value has not been set.") << endl);
    }
}

/**
 * @brief dumps information about this object
 * @param strm C++ i/o stream to dump the information to
 */
void EffectiveUrlCache::dump(ostream &strm) const {
    strm << BESIndent::LMarg << prolog << "(this: " << (void *) this << ")" << endl;
    BESIndent::Indent();
    strm << BESIndent::LMarg << "d_skip_regex: " << (d_skip_regex ? d_skip_regex->pattern() : "WAS NOT SET") << endl;
    if (!d_effective_urls.empty()) {
        strm << BESIndent::LMarg << "effective url list:" << endl;
        BESIndent::Indent();
        for (auto const &i: d_effective_urls) {
            strm << BESIndent::LMarg << i.first << " --> " << i.second->str();
        }
        BESIndent::UnIndent();
    } else {
        strm << BESIndent::LMarg << "effective url list: EMPTY" << endl;
    }
    BESIndent::UnIndent();
}

} // namespace http
