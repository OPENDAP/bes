// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of the BES http package, part of the Hyrax data server.

// Copyright (c) 2025 OPeNDAP, Inc.
// Authors: Nathan Potter <ndp@opendap.org>, Hannah Robertson <hrobertson@opendap.org>
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
//      Hannah Robertson <hrobertson@opendap.org>

#include "config.h"

#include <mutex>
#include <regex>
#include <sstream>
#include <string>

#include "AWS_SDK.h"
#include "TheBESKeys.h"
#include "BESDebug.h"
#include "BESStopWatch.h"
#include "BESUtil.h"
#include "CurlUtils.h"
#include "HttpError.h"
#include "HttpNames.h"
#include "EffectiveUrl.h"
#include "SignedUrlCache.h"

#include <aws/core/Aws.h>
#include <aws/core/auth/AWSCredentialsProvider.h>
#include <aws/s3/S3Client.h>

#include "rapidjson/document.h"

using namespace std;

constexpr auto MODULE = "euc";
constexpr auto MODULE_TIMER = "euc:timer";
constexpr auto MODULE_DUMPER = "euc:dump";

#define prolog std::string("SignedUrlCache::").append(__func__).append("() - ")

namespace bes {

/**
 * @brief Get the cached presigned URL.
 * @param url_key Key to a cached presigned URL.
 * @note This method is not, itself, thread safe.
 */
shared_ptr <http::EffectiveUrl> SignedUrlCache::get_cached_presigned_s3_url(string const &url_key) {
    shared_ptr<http::EffectiveUrl> signed_url(nullptr);
    auto it = d_presigned_s3_urls_cache.find(url_key);
    if (it != d_presigned_s3_urls_cache.end()) {
        signed_url = (*it).second;
    }
    return signed_url;
}

/**
 * @brief Return true if the input occurred before the current time, false otherwise
 * @param timestamp_str Timestamp must be formatted as returned by AWS endpoint, YYYY-MM-DD HH:mm:dd+timezone, where timezone is formatted `HH:MM`, e.g. `1980-07-16 18:40:58+00:00`
 * @note Return false if `timestamp_str` is not a valid timestamp string.
 */
bool SignedUrlCache::is_timestamp_after_now(std::string const &timestamp_str) {
    auto now = std::chrono::system_clock::now();
    auto now_secs = std::chrono::time_point_cast<std::chrono::seconds>(now);

    auto effective_timestamp_str = timestamp_str;
    if (timestamp_str.size() == 25) {
        // Hack to handle fact that s3credentials from aws include an
        // extra colon in their timezone field
        // This changes "1980-07-16 18:40:58+00:00" to "1980-07-16 18:40:58+0000"
        effective_timestamp_str = string(timestamp_str).erase(22, 1);
    }

    std::tm timestamp_time = {};
    auto time_parse_result = strptime(effective_timestamp_str.c_str(), "%F %T%z", &timestamp_time);
    if (time_parse_result == nullptr) {
        INFO_LOG(prolog + string("SERVICE CHAIN WARNING - s3 credentials timestamp was not parseable - " + timestamp_str));
        return false;
    }
    auto timestamp_time_point = std::chrono::system_clock::from_time_t(std::mktime(&timestamp_time));
    auto timestamp_secs = std::chrono::time_point_cast<std::chrono::seconds>(timestamp_time_point);
    return timestamp_secs > now_secs;
}

/**
 * @brief Get the cached STS credentials for a given TEA endpoint URL.
 * @param tea_endpoint_url_key URL to a TEA endpoint.
 * @note This method is not, itself, thread safe.
 */
shared_ptr<SignedUrlCache::S3AccessKeyTuple> SignedUrlCache::retrieve_cached_sts_credentials(string const &tea_endpoint_url_key) {
    shared_ptr<S3AccessKeyTuple> s3_access_key_tuple(nullptr);
    auto it = d_tea_endpoint_sts_credentials_cache.find(tea_endpoint_url_key);
    if (it != d_tea_endpoint_sts_credentials_cache.end()) {
        // Is it expired? If so, erase it!
        auto timestamp_str = get<3>(*(it->second));
        if (!is_timestamp_after_now(timestamp_str)) {
            // Expired!
            d_tea_endpoint_sts_credentials_cache.erase(it);
        } else {
            s3_access_key_tuple = it->second;
        }
    }
    return s3_access_key_tuple;
}

/**
 * Generate a the terminal (effective) presigned url for the source_url. If the source_url matches the
 * skip_regex then it will not be cached.
 *
 * Unlike EffectiveUrlCache, return nullptr instead of making a new EffectiveUrl(source_url)
 * when unable to construct a signed url for any reason.
 *
 * @param source_url
 * @returns The presigned effective URL, nullptr if none able to be created
*/
shared_ptr <http::EffectiveUrl> SignedUrlCache::get_presigned_s3_url(shared_ptr <http::url> source_url) {

    BESDEBUG(MODULE, prolog << "BEGIN url: " << source_url->str() << endl);
    BESDEBUG(MODULE_DUMPER, prolog << "dump: " << endl << dump() << endl);

    // Lock access to the cache, the d_presigned_s3_urls_cache map. Released when the lock goes out of scope.
    std::lock_guard<std::mutex> lock_me(d_cache_lock_mutex);

    if (!is_enabled()) {
        BESDEBUG(MODULE, prolog << "CACHE IS DISABLED." << endl);
        return nullptr;
    }

    // if it's not an HTTP url there is nothing to sign or cache.
    if (source_url->str().find(HTTP_PROTOCOL) != 0 && source_url->str().find(HTTPS_PROTOCOL) != 0) {
        BESDEBUG(MODULE, prolog << "END Not an HTTP request, SKIPPING." << endl);
        return nullptr;
    }

    if (!d_skip_regex) {
        set_skip_regex();
    }

    if (d_skip_regex) {
        size_t match_length = 0;
        match_length = d_skip_regex->match(source_url->str().c_str(), (int) source_url->str().size());
        if (match_length == source_url->str().size()) {
            BESDEBUG(MODULE, prolog << "END Candidate url matches the "
                                       "no_redirects_regex_pattern [" << d_skip_regex->pattern() <<
                                    "][match_length=" << match_length << "] SKIPPING." << endl);
            INFO_LOG(prolog + "SERVICE CHAIN WARNING - Failed to generate presigned url because url matches the no_redirects_regex_pattern -  " + source_url->get_url_no_query());
            return nullptr;
        }
        BESDEBUG(MODULE, prolog << "Candidate url: '" << source_url->str()
                                << "' does NOT match the skip_regex pattern [" << d_skip_regex->pattern() << "]"
                                << endl);
    } else {
        BESDEBUG(MODULE, prolog << "The cache_effective_urls_skip_regex() was NOT SET " << endl);
    }

    shared_ptr<http::EffectiveUrl> signed_url = get_cached_presigned_s3_url(source_url->str());
    bool retrieve_and_cache = !signed_url || signed_url->is_expired();

    // It not found or expired, (re)load.
    if (retrieve_and_cache) {
        BESDEBUG(MODULE, prolog << "Acquiring signed URL for  " << source_url->str() << endl);
        {
            BES_STOPWATCH_START(MODULE_TIMER, prolog + "Retrieve and cache signed url for source url: " + source_url->str());

            // 1. Get requisite s3 data urls...
            string s3_uri;
            string tea_endpoint_url;
            tie(s3_uri, tea_endpoint_url) = retrieve_cached_prerequisites_for_url_signing(source_url->str());
            if (s3_uri.empty() || tea_endpoint_url.empty()) {
                INFO_LOG(prolog + "SERVICE CHAIN WARNING - Cannot generate signed url due to lack of valid s3credentials endpoint or s3_uri -  " + source_url->get_url_no_query());
                return nullptr;
            }

            // 2. ...get unexpired access credentials from cache or s3credentials endpoint...
            auto s3_access_key_tuple = retrieve_cached_sts_credentials(tea_endpoint_url);
            if (!s3_access_key_tuple) {
                s3_access_key_tuple = cache_sts_credentials_from_tea_endpoint(tea_endpoint_url);
            }
            if (!s3_access_key_tuple) {
                INFO_LOG(prolog + "SERVICE CHAIN WARNING - Cannot generate signed url due to error when acquiring s3credentials from endpoint -  " + source_url->get_url_no_query());
                return nullptr;
            }

            // 3: ...and use them to create a signed url!
            signed_url = sign_s3_uri_with_sts_credentials(s3_uri, s3_access_key_tuple);
            if (!signed_url) {
                // Not logging a warning here, as a detailed warning will have already been
                // logged during failed signing.
                return nullptr;
            }
            d_presigned_s3_urls_cache[source_url->str()] = signed_url;
        }
        BESDEBUG(MODULE, prolog << "   source_url: " << source_url->str() << " ("
                                << (source_url->is_trusted() ? "" : "NOT ") << "trusted)" << endl);
        BESDEBUG(MODULE, prolog << "signed_url: " << signed_url->dump() << " ("
                                << (source_url->is_trusted() ? "" : "NOT ") << "trusted)" << endl);


        BESDEBUG(MODULE, prolog << "Updated record for " << source_url->str() << " cache size: "
                                << d_presigned_s3_urls_cache.size() << endl);

        // Since we don't want there to be a concurrency issue when we release the lock, we don't
        // return the instance of shared_ptr<EffectiveUrl> that we placed in the cache. Rather
        // we make a clone and return that. It will have its own lifecycle independent of
        // the instance we placed in the cache - it can be modified and the one in the cache
        // is unchanged. Trusted state was established from source_url when signed_url was
        // created
        signed_url = make_shared<http::EffectiveUrl>(signed_url, true);
    } else {
        // Here we have a !expired instance of a shared_ptr<EffectiveUrl> retrieved from the cache.
        // Now we need to make a copy to return, inheriting trust from the requesting URL.
        signed_url = make_shared<http::EffectiveUrl>(signed_url, source_url->is_trusted());
    }

    BESDEBUG(MODULE_DUMPER, prolog << "dump: " << endl << dump() << endl);
    BESDEBUG(MODULE, prolog << "END" << endl);

    return signed_url;
}

/**
 * @brief Store each `s3_uri` and `tea_endpoint_url` for key `key_href_url`
 * @note Does not cache anything if any of the three inputs are empty
 */
void SignedUrlCache::cache_prerequisites_for_url_signing(const std::string &key_href_url, const std::string &s3_uri, const std::string &tea_endpoint_url) {
    if (key_href_url.empty() || s3_uri.empty() || tea_endpoint_url.empty() ) {
        // Don't cache if any field is empty.
        return;
    }
    d_href_to_s3_uri_cache[key_href_url] = s3_uri;
    d_href_to_tea_endpoint_cache[key_href_url] = tea_endpoint_url;
}

/**
 * @brief Return pair of (s3_uri, tea_endpoint_url) cached for key_href_url
 * @note If key_href_url not in cache, returns pair of empty strings
 */
std::pair<std::string, std::string> SignedUrlCache::retrieve_cached_prerequisites_for_url_signing(const std::string &key_href_url) const {
    auto it_s3_uri = d_href_to_s3_uri_cache.find(key_href_url);
    auto it_s3credentials_url = d_href_to_tea_endpoint_cache.find(key_href_url);
    if (it_s3_uri == d_href_to_s3_uri_cache.end() || it_s3credentials_url == d_href_to_tea_endpoint_cache.end() ) {
        INFO_LOG(prolog + "SERVICE CHAIN WARNING - No url available for TEA s3credentials endpoint.");
        return std::pair<std::string, std::string>("", "");
    }
    return std::pair<std::string, std::string>(it_s3_uri->second, it_s3credentials_url->second);
}

/**
 * @brief Return credentials tuple for given endpoint url `tea_endpoint_url`, stores credentials for endpoint in `d_tea_endpoint_sts_credentials_cache`
 * @note If credential retrieval fails at any point, returns nullptr and does not cache results
 */
shared_ptr<SignedUrlCache::S3AccessKeyTuple> SignedUrlCache::cache_sts_credentials_from_tea_endpoint(std::string const &tea_endpoint_url) {
    // 1. Get the credentials from TEA
    std::string s3credentials_json_string;
    try {
        BES_PROFILE_TIMING(string("Request s3 credentials from TEA - ") + tea_endpoint_url);

        // Note: this http_get call internally adds edl auth headers, if available
        curl::http_get(tea_endpoint_url, s3credentials_json_string);
    }
    catch (http::HttpError &http_error) {
        string err_msg = prolog + "Encountered an error while "
                         "attempting to retrieve s3 credentials from TEA. " + http_error.get_message();
        INFO_LOG(err_msg);
        return nullptr;
    }
    if (s3credentials_json_string.empty()) {
        string err_msg = prolog + "Unable to retrieve s3 credentials from TEA endpoint " + tea_endpoint_url;
        INFO_LOG(err_msg);
        return nullptr;
    }

    // 2. Parse the response to pull out the credentials
    auto credentials = extract_sts_credentials_from_json_response(s3credentials_json_string);
    if (credentials) {
        // Store credentials if any were retrieved
        d_tea_endpoint_sts_credentials_cache[tea_endpoint_url] = credentials;
    }
    return credentials;
}


/**
 * @brief Extract credentials tuple from json response returned from an s3credentials endpoint
 * @note Returns nullptr if input is not valid json or does not contain one of the four requisite
 *  strings: `accessKeyId`, `secretAccessKey`, `sessionToken`, or `expiration`
 * @note Lightly adapted from get_urls_from_granules_umm_json_v1_4
 */
std::shared_ptr<SignedUrlCache::S3AccessKeyTuple> SignedUrlCache::extract_sts_credentials_from_json_response(std::string const &s3credentials_json_string) {
    rapidjson::Document s3credentials_response;
    s3credentials_response.Parse(s3credentials_json_string.c_str());

    if (s3credentials_response.HasParseError()) {
        return nullptr;
    }

    string access_key_id;
    string secret_access_key;
    string session_token;
    string expiration;

    auto itr = s3credentials_response.FindMember("accessKeyId");
    if (itr != s3credentials_response.MemberEnd() && itr->value.IsString()) {
        access_key_id = itr->value.GetString();
    }

    itr = s3credentials_response.FindMember("secretAccessKey");
    if (itr != s3credentials_response.MemberEnd() && itr->value.IsString()) {
        secret_access_key = itr->value.GetString();
    }

    itr = s3credentials_response.FindMember("sessionToken");
    if (itr != s3credentials_response.MemberEnd() && itr->value.IsString()) {
        session_token = itr->value.GetString();
    }

    itr = s3credentials_response.FindMember("expiration");
    if (itr != s3credentials_response.MemberEnd() && itr->value.IsString()) {
        expiration = itr->value.GetString();
    }

    if (access_key_id.empty() || secret_access_key.empty() || session_token.empty() || expiration.empty()) {
        return nullptr;
    }
    return make_shared<SignedUrlCache::S3AccessKeyTuple>(access_key_id,
                                                         secret_access_key,
                                                         session_token,
                                                         expiration);
}

SignedUrlCache *SignedUrlCache::TheCache() {
    // Create a local static object the first time the function is called
    static SignedUrlCache instance;

    // Initialize the aws library (as per SDK, must only occur once in application!)
    bes::AWS_SDK::aws_library_initialize();
    return &instance;
}


/**
 * @brief Split `s3_uri` into bucket, object strings
 */
std::pair<std::string, std::string> SignedUrlCache::split_s3_uri(std::string const &s3_uri) {

    // Safety first (even though if we were missing the s3:// prefix, s3_uri wouldn't have been extracted from the cmr result in the first place....)
    if (s3_uri.size() < 6 || s3_uri.find("s3://") != 0) {
        return std::pair<std::string, std::string>("", "");
    }

    // Get the bucket name by removing prefix "s3://" (which must exist or the path
    // wouldn't have been extracted from cmr) and including everything up to the first slash
    std::string bucket = s3_uri.substr(5, s3_uri.substr(5).find("/"));

    // The object name is everything after the bucket name, not including the first "/"
    std::string object = s3_uri.substr(s3_uri.find(bucket) + 1 + bucket.size());

    return std::pair<std::string, std::string>(bucket, object);
}


/**
 * @brief Return difference between expiration time and now, if expiration is in future; 0 otherwise
 * @param credentials_expiration_datetime Timestamp must be formatted as returned by AWS endpoint, YYYY-MM-DD HH:mm:dd+timezone, where timezone is formatted `HH:MM`, e.g. `1980-07-16 18:40:58+00:00`
 * @note Return 0 if `timestamp_str` is not a valid timestamp string.
 * @note Shares code with `is_timestamp_after_now`, could maybe converge
 * @param current_time Exposed as parameter to aid in testing; defaults to now()
 */
uint64_t SignedUrlCache::num_seconds_until_expiration(const string &credentials_expiration_datetime, const chrono::system_clock::time_point current_time) {
    auto now_secs = std::chrono::time_point_cast<std::chrono::seconds>(current_time);

    auto effective_timestamp_str = credentials_expiration_datetime;
    if (credentials_expiration_datetime.size() == 25) {
        // Hack to handle fact that s3credentials from aws include an
        // extra colon in their timezone field
        // This changes "1980-07-16 18:40:58+00:00" to "1980-07-16 18:40:58+0000"
        effective_timestamp_str = string(credentials_expiration_datetime).erase(22, 1);
    }

    std::tm timestamp_time = {};
    auto time_parse_result = strptime(effective_timestamp_str.c_str(), "%F %T%z", &timestamp_time);
    if (time_parse_result == nullptr) {
        INFO_LOG(prolog + string("SERVICE CHAIN WARNING - s3 credentials timestamp was not parseable - " + credentials_expiration_datetime));
        return 0;
    }
    auto timestamp_time_point = std::chrono::system_clock::from_time_t(std::mktime(&timestamp_time));
    auto timestamp_secs = std::chrono::time_point_cast<std::chrono::seconds>(timestamp_time_point);
    return timestamp_secs > now_secs ? (timestamp_secs - now_secs).count() : 0;
}

/**
 * @brief Sign `s3_uri` with aws credentials in `s3_access_key_tuple`, or nullptr if any part of signing process fails
 */
std::shared_ptr<http::EffectiveUrl> SignedUrlCache::sign_s3_uri_with_sts_credentials(std::string const &s3_uri,
                                                                                     std::shared_ptr<S3AccessKeyTuple> const s3_access_key_tuple) {

    if (s3_access_key_tuple == nullptr) {
        INFO_LOG(prolog + "SERVICE CHAIN WARNING - Failed to generate signed url due to missing s3credentials -  " + s3_uri);
        return nullptr;
    }
                                                                       
    bes::AWS_SDK aws_sdk;
    string id = get<0>(*s3_access_key_tuple);
    string secret = get<1>(*s3_access_key_tuple);
    string token = get<2>(*s3_access_key_tuple);
    string expiration = get<3>(*s3_access_key_tuple);
    auto expiration_seconds = num_seconds_until_expiration(get<3>(*s3_access_key_tuple));
    if (expiration_seconds == 0) {
        // NB: We should never hit this error, as we intentionally check expiration upstream
        // But in case we DO end up here, we want to know about it
        INFO_LOG(prolog + "SERVICE CHAIN WARNING - Failed to generate signed url due to expired s3credentials -  " + s3_uri);
        return nullptr;
    }

    string bucket;
    string object;
    tie(bucket, object) = split_s3_uri(s3_uri);
    if (bucket.empty() || object.empty()) {
        INFO_LOG(prolog + "SERVICE CHAIN WARNING - Failed to generate signed url due to either an empty bucket or object string -  " + s3_uri);
        return nullptr;
    }

    // Use temporary credentials returned from given TEA s3credentials endpoint,
    // which are AWS Security Token Service (STS) credentials
    Aws::Auth::AWSCredentials credentials(id, secret, token);
    Aws::Client::ClientConfiguration config;
    config.region = d_aws_region_in_which_direct_copy_is_supported;
    Aws::S3::S3Client s3_client(credentials, nullptr, config);

    // Use that info to generate our signed url!
    // Internal signing function won't throw unless uninitialized, which happens in SignedUrlCache constructor
    Aws::String signed_url = s3_client.GeneratePresignedUrl(
        bucket,
        object,
        Aws::Http::HttpMethod::HTTP_GET,
        expiration_seconds
    );

    if (signed_url.empty()) {
        // NB: It would be very surprising to end up here, but if we do, we want to know about it
        INFO_LOG(prolog + "SERVICE CHAIN WARNING - Failed to generate signed url - " + s3_uri);
    }
    return make_shared<http::EffectiveUrl>(signed_url);
}

/**
 * @brief Return if the current aws region of the running application matches the pre-defined supported region
 */
const bool SignedUrlCache::is_cache_supported_within_current_aws_region() {
    bes::AWS_SDK aws_sdk;
    auto region = aws_sdk.get_aws_region_of_running_application();
    return region == d_aws_region_in_which_direct_copy_is_supported;
}

/**
 * @brief Return if the cache is enabled, which is set in the bes.conf file
 * @note Follows the same settings (and relies on the same bes.conf key) as the EffectiveUrlsCache
 * @note Will always be disabled when run outside a region supported by ngap direct object access, as such copy is always disallowed and therefore presigned urls will always result in 400 errors when used to copy objects.
 */
bool SignedUrlCache::is_enabled() {
    // The first time here, the value of d_enabled is -1.
    // Once we confirm a supported aws region and then check for its enablement in TheBESKeys
    // The value will be 0 (false) or 1 (true) and TheBESKeys will not be checked again.
    if (d_enabled < 0) {
        if (is_cache_supported_within_current_aws_region()) {
            string value = TheBESKeys::TheKeys()->read_string_key(HTTP_CACHE_EFFECTIVE_URLS_KEY, "false");
            d_enabled = BESUtil::lowercase(value) == "true";
        } else {
            d_enabled = false;
            INFO_LOG(prolog + "SERVICE CHAIN WARNING - Direct s3 access via presigned urls is not supported for services running in current aws region. Supported region:" + d_aws_region_in_which_direct_copy_is_supported);
        }
    }
    BESDEBUG(MODULE, prolog << "d_enabled: " << (d_enabled ? "true" : "false") << endl);
    return d_enabled;
}

/**
 * @return Set the regex used to skip cache keys, which is set in the bes.conf file
 * @note Follows the same settings (and relies on the same bes.conf key) as the EffectiveUrlsCache
 */
void SignedUrlCache::set_skip_regex() {
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
 * @brief dumps information about this object into string
 */
std::string SignedUrlCache::dump() const {
    std::stringstream sstrm;
    dump(sstrm);
    return sstrm.str();
}

/**
 * @brief dumps information about this object
 * @param strm C++ i/o stream to dump the information to
 */
void SignedUrlCache::dump(ostream &strm) const {
    strm << BESIndent::LMarg << prolog << "(this: " << (void *) this << ")" << endl;
    BESIndent::Indent();
    strm << BESIndent::LMarg << "d_skip_regex: " << (d_skip_regex ? d_skip_regex->pattern() : "WAS NOT SET") << endl;
    if (!d_presigned_s3_urls_cache.empty()) {
        strm << BESIndent::LMarg << "presigned url list:" << endl;
        BESIndent::Indent();
        for (auto const &i: d_presigned_s3_urls_cache) {
            strm << BESIndent::LMarg << i.first << " --> " << i.second->str() << "\n";
        }
        BESIndent::UnIndent();
    } else {
        strm << BESIndent::LMarg << "presigned url list: EMPTY" << endl;
    }

    if (!d_href_to_tea_endpoint_cache.empty()) {
        strm << BESIndent::LMarg << "href-to-s3credentials list:" << endl;
        BESIndent::Indent();
        for (auto const &i: d_href_to_tea_endpoint_cache) {
            strm << BESIndent::LMarg << i.first << " --> " << i.second << "\n";
        }
        BESIndent::UnIndent();
    } else {
        strm << BESIndent::LMarg << "href-to-s3credentials list: EMPTY" << endl;
    }

    if (!d_href_to_s3_uri_cache.empty()) {
        strm << BESIndent::LMarg << "href-to-s3url list:" << endl;
        BESIndent::Indent();
        for (auto const &i: d_href_to_s3_uri_cache) {
            strm << BESIndent::LMarg << i.first << " --> " << i.second << "\n";
        }
        BESIndent::UnIndent();
    } else {
        strm << BESIndent::LMarg << "href-to-s3url list: EMPTY" << endl;
    }

    if (!d_tea_endpoint_sts_credentials_cache.empty()) {
        strm << BESIndent::LMarg << "s3 credentials list:" << endl;
        BESIndent::Indent();
        for (auto const &i: d_tea_endpoint_sts_credentials_cache) {
            strm << BESIndent::LMarg << i.first << " --> Expires: " << get<3>(*(i.second)) << "\n";
        }
        BESIndent::UnIndent();
    } else {
        strm << BESIndent::LMarg << "s3 credentials list: EMPTY" << endl;
    }

    BESIndent::UnIndent();
}

} // namespace bes
