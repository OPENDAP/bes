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

#ifndef _bes_http_SignedUrlCache_h_
#define _bes_http_SignedUrlCache_h_ 1

#include <memory>
#include <map>
#include <unordered_map>
#include <string>
#include <mutex>

#include "BESObj.h"
#include "BESRegex.h"   // for std::unique_ptr<BESRegex>

namespace http {

class EffectiveUrl;
class url;

/**
 * This is a singleton class. It is used to associate a URL with its "pre-signed" AWS s3 URL. This means that
 * a URL is signed locally rather than sent through a potentially large number of external redirect actions, as 
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

    std::map<std::string, std::shared_ptr<http::EffectiveUrl>> d_signed_urls;

    std::map<std::string, std::string> d_href_to_s3credentials_cache;
    std::map<std::string, std::string> d_href_to_s3_cache;

    std::shared_ptr<S3AccessKeyTuple> get_s3credentials_from_endpoint(std::string const &s3credentials_url);
    static std::shared_ptr<S3AccessKeyTuple> extract_s3_credentials_from_response_json(std::string const &s3credentials_json_string);

    std::map<std::string, std::shared_ptr<S3AccessKeyTuple>> d_s3credentials_cache;
    std::shared_ptr<S3AccessKeyTuple> retrieve_cached_s3credentials(std::string const &url_key);
    static bool is_timestamp_after_now(std::string const &timestamp);

    // URLs that match are not cached.
    std::unique_ptr<BESRegex> d_skip_regex = nullptr;

    int d_enabled = -1;

    std::shared_ptr<EffectiveUrl> sign_url(std::string const &s3_url,
                                           std::shared_ptr<S3AccessKeyTuple> const s3_access_key_tuple);
    std::shared_ptr<EffectiveUrl> get_cached_signed_url(std::string const &url_key);

    void set_skip_regex();

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
    static SignedUrlCache *TheCache() {
        // Create a local static object the first time the function is called
        static SignedUrlCache instance;
        return &instance;
    }

    SignedUrlCache(const SignedUrlCache &src) = delete;
    SignedUrlCache &operator=(const SignedUrlCache &rhs) = delete;

    ~SignedUrlCache() override = default;

    void cache_signed_url_components(const std::string &key_href_url, const std::string &s3_url, const std::string &s3credentials_url);
    std::pair<std::string, std::string> retrieve_cached_signed_url_components(const std::string &key_href_url) const;
    std::shared_ptr<EffectiveUrl> get_signed_url(std::shared_ptr<url> source_url);

    void dump(std::ostream &strm) const override;

    std::string dump() const {
        std::stringstream sstrm;
        dump(sstrm);
        return sstrm.str();
    }
};

} // namespace http

#endif // _bes_http_SignedUrlCache_h_

