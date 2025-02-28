

// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of the Hyrax data server.

// This code is derived from https://github.com/bradclawsie/awsv4-cpp
// Copyright (c) 2013, brad clawsie
// All rights reserved.
// see the file AWSV4_LICENSE

// Copyright (c) 2019 OPeNDAP, Inc.
// Modifications Author: James Gallagher <jgallagher@opendap.org>
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

#include "config.h"

#include "awsv4.h"

#include <cstring>

#include <stdexcept>
#include <algorithm>
#include <map>
#include <ctime>
#include <iostream>
#include <sstream>

#include <openssl/sha.h>
#include <openssl/hmac.h>

#include "url_impl.h"
#include "BESInternalError.h"
#include "BESDebug.h"
#include "HttpNames.h"

#define prolog std::string("AWSV4::").append(__func__).append("() - ")

namespace AWSV4 {

// used in sha256_base16() and hmac_to_string(). jhrg 1/5/20
const int SHA256_DIGEST_STRING_LENGTH = (SHA256_DIGEST_LENGTH << 1);

/**
 * @brief join strings using a delimiter
 * @param ss Strings to join
 * @param delim Dilimiter to separate the strings
 * @return The string result
 */
std::string join(const std::vector<std::string> &ss, const std::string &delim) {
    if (ss.empty())
        return "";

    std::stringstream sstream;
    const size_t l = ss.size() - 1;
    for (size_t i = 0; i < l; i++) {
        sstream << ss[i] << delim;
    }
    sstream << ss.back();
    return sstream.str();
}

std::string sha256_base16(const std::string &str) {

    unsigned char hashOut[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, (const unsigned char *) str.c_str(), str.size());
    SHA256_Final(hashOut, &sha256);

    char outputBuffer[SHA256_DIGEST_STRING_LENGTH + 1];
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        snprintf(outputBuffer + (i * 2), 3, "%02x", hashOut[i]);
    }
    outputBuffer[SHA256_DIGEST_STRING_LENGTH] = 0;
    return std::string{outputBuffer};
}

// From https://stackoverflow.com/questions/1798112/removing-leading-and-trailing-spaces-from-a-string
static std::string trim(const std::string &str, const std::string &whitespace = " \t") {
    const auto strBegin = str.find_first_not_of(whitespace);
    if (strBegin == std::string::npos)
        return ""; // no content

    const auto strEnd = str.find_last_not_of(whitespace);
    const auto strRange = strEnd - strBegin + 1;

    return str.substr(strBegin, strRange);
}

// -----------------------------------------------------------------------------------
// TASK 1 - create a canonical request
// http://docs.aws.amazon.com/general/latest/gr/sigv4-create-canonical-request.html

// create a map of the "canonicalized" headers
// will return empty map on malformed input.
//
// headers A vector where each element is a header name and value, separated by a colon. No spaces.
std::map<std::string, std::string> canonicalize_headers(const std::vector<std::string> &headers) {
    std::map<std::string, std::string> header_key2val;
    for (const auto &header: headers) {
        auto i = header.find(':');
        if (i == std::string::npos) {
            header_key2val.clear();
            return header_key2val;
        }

        std::string key = trim(header.substr(0, i));
        const std::string val = trim(header.substr(i + 1));
        if (key.empty() || val.empty()) {
            header_key2val.clear();
            return header_key2val;
        }

        std::transform(key.begin(), key.end(), key.begin(), ::tolower);
        header_key2val[key] = val;
    }
    return header_key2val;
}

// get a string representation of header:value lines
std::string map_headers_string(const std::map<std::string, std::string> &header_key2val) {
    const std::string pair_delim{":"};
    std::string h;
    for (const auto &kv: header_key2val) {
        h.append(kv.first).append(pair_delim).append(kv.second).append(ENDL);
    }
    return h;
}

// get a string representation of the header names
std::string map_signed_headers(const std::map<std::string, std::string> &header_key2val) {
    const std::string signed_headers_delim{";"};
    std::vector<std::string> ks;
    for ( const auto &kv: header_key2val) {
        ks.push_back(kv.first);
    }
    return join(ks, signed_headers_delim);
}

std::string canonicalize_request(const std::string &http_request_method,
                                       const std::string &canonical_uri,
                                       const std::string &canonical_query_string,
                                       const std::string &canonical_headers,
                                       const std::string &signed_headers,
                                       const std::string &shar256_of_payload) {
    return std::string(http_request_method).append(ENDL).append(canonical_uri).append(ENDL)
            .append(canonical_query_string).append(ENDL).append(canonical_headers).append(ENDL)
            .append(signed_headers).append(ENDL).append(shar256_of_payload);
}

// -----------------------------------------------------------------------------------
// TASK 2 - create a string-to-sign
// http://docs.aws.amazon.com/general/latest/gr/sigv4-create-string-to-sign.html

std::string string_to_sign(const std::string &algorithm,
                                 const std::time_t &request_date,
                                 const std::string &credential_scope,
                                 const std::string &hashed_canonical_request) {
    return algorithm + ENDL +
           ISO8601_date(request_date) + ENDL +
           credential_scope + ENDL +
           hashed_canonical_request;
}

std::string credential_scope(const std::time_t &request_date,
                                   const std::string &region,
                                   const std::string &service) {
    const std::string s{"/"};
    return utc_yyyymmdd(request_date).append(s).append(region).append(s).append(service).append(s).append(AWS4_REQUEST);
}

// time_t -> 20131222T043039Z
std::string ISO8601_date(const std::time_t &t) {
    char buf[sizeof "20111008T070709Z"];
    struct tm tm_buf{};
    std::strftime(buf, sizeof buf, "%Y%m%dT%H%M%SZ", gmtime_r(&t, &tm_buf));
    return buf;
}

// time_t -> 20131222
std::string utc_yyyymmdd(const std::time_t &t) {
    char buf[sizeof "20111008"];
    struct tm tm_buf{};
    std::strftime(buf, sizeof buf, "%Y%m%d", gmtime_r(&t, &tm_buf));
    return buf;
}

// HMAC --> string. jhrg 11/25/19
std::string hmac_to_string(const unsigned char *hmac) {
    // Added to print the kSigning value to check against AWS example. jhrg 11/24/19
    char buf[SHA256_DIGEST_STRING_LENGTH + 1];
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        // size is 3 for each call (2 chars plus null). jhrg 1/3/20
        snprintf(buf + (i * 2), 3, "%02x", hmac[i]);
    }
    buf[SHA256_DIGEST_STRING_LENGTH] = 0;
    return buf;
}

// -----------------------------------------------------------------------------------
// TASK 3
// http://docs.aws.amazon.com/general/latest/gr/sigv4-calculate-signature.html

/*
 * unsigned char *HMAC(const EVP_MD *evp_md,
 *          const void *key, int key_len,
 *          const unsigned char *d, int n,
 *          unsigned char *md, unsigned int *md_len);
 *          where md must be EVP_MAX_MD_SIZE in size
 */

std::string calculate_signature(const std::time_t &request_date,
                                      const std::string &secret,
                                      const std::string &region,
                                      const std::string &service,
                                      const std::string &string_to_sign) {

    // These are used/re-used for the various signatures. jhrg 1/3/20
    unsigned char md[EVP_MAX_MD_SIZE + 1];
    unsigned int md_len;

    const std::string k1 = AWS4 + secret;
    const std::string yyyymmdd = utc_yyyymmdd(request_date);
    // NB: The third argument for HMAC is an unsigned int. jhrg 10/31/22
    const unsigned char *kDate = HMAC(EVP_sha256(), (const void *)k1.c_str(), (unsigned int)k1.size(),
                                (const unsigned char *) yyyymmdd.c_str(), yyyymmdd.size(), md, &md_len);
    if (!kDate)
        throw BESInternalError("Could not compute AWS V4 request signature.", __FILE__, __LINE__);

    md[md_len] = '\0';
    BESDEBUG(HTTP_MODULE,
             prolog << "kDate: " << hmac_to_string(kDate) << " md_len: " << md_len << " md: " << hmac_to_string(md)
                    << std::endl);

    const unsigned char *kRegion = HMAC(EVP_sha256(), md, md_len,
                                  (const unsigned char *) region.c_str(), region.size(), md, &md_len);
    if (!kRegion)
        throw BESInternalError("Could not compute AWS V4 request signature.", __FILE__, __LINE__);

    md[md_len] = '\0';
    BESDEBUG(HTTP_MODULE,
             prolog << "kRegion: " << hmac_to_string(kRegion) << " md_len: " << md_len << " md: " << hmac_to_string(md)
                    << std::endl);

    const unsigned char *kService = HMAC(EVP_sha256(), md, md_len,
                                   (const unsigned char *) service.c_str(), service.size(), md, &md_len);
    if (!kService)
        throw BESInternalError("Could not compute AWS V4 request signature.", __FILE__, __LINE__);

    md[md_len] = '\0';
    BESDEBUG(HTTP_MODULE, prolog << "kService: " << hmac_to_string(kService) << " md_len: " << md_len << " md: "
                           << hmac_to_string(md) << std::endl);

    const unsigned char *kSigning = HMAC(EVP_sha256(), md, md_len,
                                   (const unsigned char *) AWS4_REQUEST.c_str(), AWS4_REQUEST.size(), md, &md_len);
    if (!kSigning)
        throw BESInternalError("Could not compute AWS V4 request signature.", __FILE__, __LINE__);

    md[md_len] = '\0';
    BESDEBUG(HTTP_MODULE,
             prolog << "kSigning: " << hmac_to_string(kRegion) << " md_len: " << md_len << " md: " << hmac_to_string(md)
                    << std::endl);

    const unsigned char *kSig = HMAC(EVP_sha256(), md, md_len,
                               (const unsigned char *) string_to_sign.c_str(), string_to_sign.size(), md, &md_len);
    if (!kSig)
        throw BESInternalError("Could not compute AWS V4 request signature.", __FILE__, __LINE__);

    md[md_len] = '\0';
    auto sig = hmac_to_string(md);
    BESDEBUG(HTTP_MODULE, prolog << "kSig: " << sig << " md_len: " << md_len << " md: " << hmac_to_string(md) << std::endl);
    return sig;
}

/**
 * @brief Return the AWS V4 signature for a given GET request.
 *
 * This version takes strings for the path, query string and host.
 *
 * @param canonical_uri The path part of the URI
 * @param canonical_query The URI query string
 * @param host The host part of the URI
 * @param request_date The current date & time
 * @param secret_key The Secret key for this resource (the thing referenced by the URI).
 * @param region The AWS region where the request is being made (us-west-2 by default)
 * @param service The AWS service that is the target of the request (S3 by default)
 * @return The AWS V4 Signature string.
 */
std::string compute_awsv4_signature(const std::string &canonical_uri, const std::string &canonical_query,
                                    const std::string &host, const std::time_t &request_date,
                                    const std::string &public_key, const std::string &secret_key,
                                    const std::string &region, const std::string &service) {
    // We can eliminate one call to sha256 if the payload is null, which
    // is the case for a GET request. jhrg 11/25/19
    const std::string sha256_empty_payload = {"e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"};
    // All AWS V4 signature require x-amz-content-sha256. jhrg 11/24/19

    // We used to do it like the code in the other half of this #if
    // But it seems we don't need the x-amz-content-sha256 header for empty payload
    // so here it is without.
    //
    // NOTE: Changing this will break the awsv4_test using tests. jhrg 1/3/20
    std::vector<std::string> headers{"host:", "x-amz-date:"};
    headers[0].append(host);
    headers[1].append(ISO8601_date(request_date));

    const auto canonical_headers_map = canonicalize_headers(headers);
    if (canonical_headers_map.empty()) {
        throw std::runtime_error("Empty header list while building AWS V4 request signature");
    }
    const auto headers_string = map_headers_string(canonical_headers_map);
    const auto signed_headers = map_signed_headers(canonical_headers_map);
    const auto canonical_request = canonicalize_request(AWSV4::GET,
                                                        canonical_uri,
                                                        canonical_query,
                                                        headers_string,
                                                        signed_headers,
                                                        sha256_empty_payload);

    BESDEBUG(HTTP_MODULE, prolog << "Canonical Request: " << canonical_request << std::endl);

    auto hashed_canonical_request = sha256_base16(canonical_request);
    auto credential_scope = AWSV4::credential_scope(request_date, region, service);
    auto string_to_sign = AWSV4::string_to_sign(STRING_TO_SIGN_ALGO,
                                                request_date,
                                                credential_scope,
                                                hashed_canonical_request);

    BESDEBUG(HTTP_MODULE, prolog << "String to Sign: " << string_to_sign << std::endl);

    auto signature = calculate_signature(request_date,
                                         secret_key,
                                         region,
                                         service,
                                         string_to_sign);

    BESDEBUG(HTTP_MODULE, prolog << "signature: " << signature << std::endl);

    std::string authorization_header = STRING_TO_SIGN_ALGO + " Credential=" + public_key + "/"
                                       + credential_scope + ", SignedHeaders=" + signed_headers + ", Signature=" +
                                       signature;

    BESDEBUG(HTTP_MODULE, prolog << "authorization_header: " << authorization_header << std::endl);

    return authorization_header;
}

/**
 * @brief Return the AWS V4 signature for a given GET request
 *
 * This version takes an http::url object for the path, query string and host.
 *
 * @param uri The URI to fetch
 * @param request_date The current date & time
 * @param secret_key The Secret key for this resource (the thing referenced by the URI).
 * @param public_key The Public key associated with the secret key.
 * @param region The AWS region where the request is being made (us-west-2 by default)
 * @param service The AWS service that is the target of the request (S3 by default)
 * @return The AWS V4 Signature string.
 */
std::string compute_awsv4_signature(
        const std::shared_ptr<http::url> &uri,
        const std::time_t &request_date,
        const std::string &public_key,
        const std::string &secret_key,
        const std::string &region,
        const std::string &service) {
    return compute_awsv4_signature(uri->path(), uri->query(), uri->host(), request_date, public_key, secret_key,
                                   region, service);
}

}
