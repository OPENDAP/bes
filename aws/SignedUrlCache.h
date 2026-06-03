// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of the BES aws package, part of the Hyrax data server.

// Copyright (c) 2026 OPeNDAP, Inc.
// Authors: Hannah Robertson <hrobertson@opendap.org>
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
//      Hannah Robertson <hrobertson@opendap.org>

#ifndef _bes_aws_SignedUrlCache_h_
#define _bes_aws_SignedUrlCache_h_ 1

#include <memory>
#include <map>
#include <unordered_map>
#include <string>
#include <mutex>

#include "BESObj.h"

namespace http {
    class EffectiveUrl;
    class url;
}

namespace bes {

/**
 * This is a singleton class. It is used to associate a URL with its presigned AWS s3 URL, where
 * the URL is signed locally rather than sent through a potentially large number of external redirect actions as
 * in EffectiveUrlCache.h. This url location plus the requisite AWS signature headers, from which the requested bytes
 * are transmitted, is termed the "effective url" and is stored in an in memory cache (std::map) so that later
 * requests may skip the external signing service and just get required bytes from the actual source.
 */
class SignedUrlCache : public BESObj {
public:
    typedef std::tuple<std::string, std::string, std::string, std::string> S3AccessKeyTuple;

private:
    SignedUrlCache() = default;

    std::mutex d_cache_lock_mutex;

    std::map<std::string, std::shared_ptr<http::EffectiveUrl>> d_presigned_s3_urls_cache;

    std::map<std::string, std::string> d_href_to_tea_endpoint_cache;
    std::map<std::string, std::string> d_href_to_s3_uri_cache;


    std::map<std::string, std::shared_ptr<S3AccessKeyTuple>> d_tea_endpoint_sts_credentials_cache;
    static std::string append_edl_username_to_key(std::string const &key);

    static std::shared_ptr<S3AccessKeyTuple> extract_sts_credentials_from_json_response(std::string const &s3credentials_json_string);
    std::shared_ptr<S3AccessKeyTuple> cache_sts_credentials_from_tea_endpoint(std::string const &tea_endpoint_url);
    std::shared_ptr<S3AccessKeyTuple> retrieve_cached_sts_credentials(std::string const &tea_endpoint_url_key);

    static bool is_timestamp_after_now(std::string const &timestamp);
    const bool is_cache_supported_within_current_aws_region();

    int d_enabled = -1;

    // The SignedUrlCache is used to generate urls that enable direct copy, and is therefore
    // only enabled within a supported aws region (i.e., the region that supports such
    // direct copy).
    std::string d_aws_region_in_which_direct_copy_is_supported = "us-west-2";

    static std::pair<std::string, std::string> split_s3_uri(std::string const &s3_uri);
    static uint64_t num_seconds_until_expiration(
        const std::string &credentials_expiration_datetime,
        const std::chrono::system_clock::time_point current_time = std::chrono::system_clock::now());
    std::shared_ptr<http::EffectiveUrl> sign_s3_uri_with_sts_credentials(std::string const &s3_uri,
                                                                         std::shared_ptr<S3AccessKeyTuple> const s3_access_key_tuple);
    std::shared_ptr<http::EffectiveUrl> get_cached_presigned_s3_url(std::string const &url_key);

    bool is_enabled();

    friend class SignedUrlCacheTest;

public:
    /** @brief Get the singleton SignedUrlCache instance.
     *
     * This static method returns the instance of this singleton class.
     * The implementation will only build one instance of SignedUrlCache and
     * thereafter return a pointer to that instance.
     *
     * Thread safe with C++-11 and greater.
     *
     * @return A pointer to the SignedUrlCache singleton
     */
    static SignedUrlCache *TheCache();

    SignedUrlCache(const SignedUrlCache &src) = delete;
    SignedUrlCache &operator=(const SignedUrlCache &rhs) = delete;

    ~SignedUrlCache() override = default;

    void cache_prerequisites_for_url_signing(const std::string &key_href_url, const std::string &s3_uri,
                                             const std::string &tea_endpoint_url);
    std::pair<std::string, std::string> retrieve_cached_prerequisites_for_url_signing(const std::string &key_href_url) const;

    std::shared_ptr<http::EffectiveUrl> get_presigned_s3_url(std::shared_ptr<http::url> source_url);

    void dump(std::ostream &strm) const override;
    std::string dump() const;
};

} // namespace bes

#endif // _bes_aws_SignedUrlCache_h_
