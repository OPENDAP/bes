

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

#include "awsv4.h"

#include <cstring>

#include <stdexcept>
#include <algorithm>
#include <map>
#include <ctime>
#include <iostream>
#include <sstream>

#if 0
#include <regex>
#endif

#include <openssl/sha.h>
#include <openssl/hmac.h>

#include "url_parser.h"

namespace AWSV4 {

    std::string join(const std::vector<std::string>& ss,const std::string delim) {
        std::stringstream sstream;
        const auto l = ss.size() - 1;
        std::vector<int>::size_type i;
        for (i = 0; i < l; i++) {
            sstream << ss.at(i) << delim;
        }
        sstream << ss.back();
        return sstream.str();
    }

    // http://stackoverflow.com/questions/2262386/generate-sha256-with-openssl-and-c
    void sha256(const std::string str, unsigned char outputBuffer[SHA256_DIGEST_LENGTH]) {
        char *c_string = new char [str.length()+1];
        std::strcpy(c_string, str.c_str());
        unsigned char hash[SHA256_DIGEST_LENGTH];
        SHA256_CTX sha256;
        SHA256_Init(&sha256);
        SHA256_Update(&sha256, c_string, strlen(c_string));
        SHA256_Final(hash, &sha256);
        for (int i=0;i<SHA256_DIGEST_LENGTH;i++) {
            outputBuffer[i] = hash[i];
        }
    }

    std::string sha256_base16(const std::string str) {
        unsigned char hashOut[SHA256_DIGEST_LENGTH];
        AWSV4::sha256(str,hashOut);
        char outputBuffer[65];
        for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
            sprintf(outputBuffer + (i * 2), "%02x", hashOut[i]);
        }
        outputBuffer[64] = 0;
        return std::string{outputBuffer};
    }

    // From https://stackoverflow.com/questions/1798112/removing-leading-and-trailing-spaces-from-a-string
    static std::string trim(const std::string& str, const std::string& whitespace = " \t")
    {
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
    std::map<std::string,std::string> canonicalize_headers(const std::vector<std::string>& headers) {
        std::map<std::string,std::string> header_key2val;
        for (const auto &h: headers) {
#if 0
            std::regex reg("\\:");
            std::sregex_token_iterator iter(h.begin(), h.end(), reg, -1);
            std::sregex_token_iterator end;
            std::vector<std::string> pair(iter, end);
            if (pair.size() != 2) {
                header_key2val.clear();
                return header_key2val;
            }

#endif
            // h is a header <key> : <val>

            auto i = h.find(':');
            if (i == std::string::npos) {
                header_key2val.clear();
                return header_key2val;
            }

            std::string key{trim(h.substr(0, i))};
            const std::string val{trim(h.substr(i+1))};
            if (key.empty() || val.empty()) {
                header_key2val.clear();
                return header_key2val;
            }

            std::transform(key.begin(), key.end(), key.begin(),::tolower);
            header_key2val[key] = val;
        }
        return header_key2val;
    }

    // get a string representation of header:value lines
    const std::string map_headers_string(const std::map<std::string,std::string>& header_key2val) {
        const std::string pair_delim{":"};
        std::string h;
        for (const auto& kv:header_key2val) {
            h.append(kv.first + pair_delim + kv.second + ENDL);
        }
        return h;
    }

    // get a string representation of the header names
    const std::string map_signed_headers(const std::map<std::string,std::string>& header_key2val) {
        const std::string signed_headers_delim{";"};
        std::vector<std::string> ks;
        for (const auto& kv:header_key2val) {
            ks.push_back(kv.first);
        }
        return join(ks,signed_headers_delim);
    }

    const std::string canonicalize_request(const std::string& http_request_method,
                                           const std::string& canonical_uri,
                                           const std::string& canonical_query_string,
                                           const std::string& canonical_headers,
                                           const std::string& signed_headers,
                                           const std::string& shar256_of_payload) {
        return http_request_method + ENDL +
            canonical_uri + ENDL +
            canonical_query_string + ENDL +
            canonical_headers + ENDL +
            signed_headers + ENDL +
                shar256_of_payload;
    }

    // -----------------------------------------------------------------------------------
    // TASK 2 - create a string-to-sign
    // http://docs.aws.amazon.com/general/latest/gr/sigv4-create-string-to-sign.html

    const std::string string_to_sign(const std::string& algorithm,
                                     const std::time_t& request_date,
                                     const std::string& credential_scope,
                                     const std::string& hashed_canonical_request) {
        return algorithm + ENDL +
            ISO8601_date(request_date) + ENDL +
            credential_scope + ENDL +
            hashed_canonical_request;
    }

    const std::string credential_scope(const std::time_t& request_date,
                                       const std::string region,
                                       const std::string service) {
        const std::string s{"/"};
        return utc_yyyymmdd(request_date) + s + region + s + service + s + AWS4_REQUEST;
    }

    // time_t -> 20131222T043039Z
    const std::string ISO8601_date(const std::time_t& t) {
        char buf[sizeof "20111008T070709Z"];
        std::strftime(buf, sizeof buf, "%Y%m%dT%H%M%SZ", std::gmtime(&t));
        return std::string{buf};
    }

    // time_t -> 20131222
    const std::string utc_yyyymmdd(const std::time_t& t) {
        char buf[sizeof "20111008"];
        std::strftime(buf, sizeof buf, "%Y%m%d", std::gmtime(&t));
        return std::string{buf};
    }

    // HMAC --> string. jhrg 11/25/19
    const std::string hmac_to_string(const unsigned char *hmac) {
        // Added to print the kSigning value to check against AWS example. jhrg 11/24/19
        char buf[65];
        for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
            sprintf(buf + (i * 2), "%02x", hmac[i]);
        }
        buf[64] = 0;
        return std::string{buf};
    }

    // -----------------------------------------------------------------------------------
    // TASK 3
    // http://docs.aws.amazon.com/general/latest/gr/sigv4-calculate-signature.html

    const std::string calculate_signature(const std::time_t& request_date,
                                          const std::string secret,
                                          const std::string region,
                                          const std::string service,
                                          const std::string string_to_sign,
                                          const bool verbose) {

        const std::string k1{AWS4 + secret};
        char *c_k1 = new char [k1.length()+1];
        std::strcpy(c_k1, k1.c_str());

        auto yyyymmdd = utc_yyyymmdd(request_date);
        char *c_yyyymmdd = new char [yyyymmdd.length()+1];
        std::strcpy(c_yyyymmdd, yyyymmdd.c_str());

        unsigned char* kDate;
        kDate = HMAC(EVP_sha256(), c_k1, strlen(c_k1),
                     (unsigned char*)c_yyyymmdd, strlen(c_yyyymmdd), NULL, NULL);
        if (verbose)
            std::cerr << "kDate: " << hmac_to_string(kDate) << std::endl;

        char *c_region = new char [region.length()+1];
        std::strcpy(c_region, region.c_str());
        unsigned char *kRegion;
        kRegion = HMAC(EVP_sha256(), kDate, strlen((char *)kDate),
                     (unsigned char*)c_region, strlen(c_region), NULL, NULL);

        if (verbose)
            std::cerr << "kRegion: " << hmac_to_string(kRegion) << std::endl;

        char *c_service = new char [service.length()+1];
        std::strcpy(c_service, service.c_str());
        unsigned char *kService;
        kService = HMAC(EVP_sha256(), kRegion, strlen((char *)kRegion),
                     (unsigned char*)c_service, strlen(c_service), NULL, NULL);

        if (verbose)
            std::cerr << "kService: " << hmac_to_string(kService) << std::endl;

        char *c_aws4_request = new char [AWS4_REQUEST.length()+1];
        std::strcpy(c_aws4_request, AWS4_REQUEST.c_str());
        unsigned char *kSigning;
        kSigning = HMAC(EVP_sha256(), kService, strlen((char *)kService),
                     (unsigned char*)c_aws4_request, strlen(c_aws4_request), NULL, NULL);

        if (verbose)
            std::cerr << "kSigning " << hmac_to_string(kSigning) << std::endl;

        char *c_string_to_sign = new char [string_to_sign.length()+1];
        std::strcpy(c_string_to_sign, string_to_sign.c_str());
        unsigned char *kSig;
        kSig = HMAC(EVP_sha256(), kSigning, strlen((char *)kSigning),
                     (unsigned char*)c_string_to_sign, strlen(c_string_to_sign), NULL, NULL);

        auto sig = hmac_to_string(kSig);
        return sig;
    }

    /**
    * @brief Return the AWS V4 signature for a given GET request
    *
    * @param uri_str The URI to fetch
    * @param request_date The current date & time
    * @param secret_key The Secret key for this resource (the thing referenced by the URI).
    * @param region The AWS region where the request is being made (us-west-2 by default)
    * @param service The AWS service that is the target of the request (S3 by default)
    * @pram verbose True, be chatty to stderr. False by default
    * @return The AWS V4 Signature string.
    */

    const std::string compute_awsv4_signature(const std::string uri_str, const std::time_t request_date,
                                              const std::string public_key, const std::string secret_key,
                                              const std::string region, const std::string service,
                                              const bool verbose) {

        url_parser uri(uri_str);

        // canonical_uri is the path component of the URL. Later we will need the host.
        const auto canonical_uri = uri.path(); // AWSV4::canonicalize_uri(uri);
        // The query string is null for our code.
        const auto canonical_query = uri.query(); // AWSV4::canonicalize_query(uri);

        // We can eliminate one call to sha256 if the payload is null, which
        // is the case for a GET request. jhrg 11/25/19
        const std::string sha256_empty_payload = {"e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"};
        // All AWS V4 signature require x-amz-content-sha256. jhrg 11/24/19
        std::vector<std::string> headers{"host: ", "x-amz-date: ", "x-amz-content-sha256: "};
        headers[0].append(uri.host()); // headers[0].append(uri.getHost());
        headers[1].append(ISO8601_date(request_date));
        headers[2].append(sha256_empty_payload);

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

        if (verbose)
            std::cerr << "-- Canonical Request\n" << canonical_request << "\n--\n" << std::endl;

        auto hashed_canonical_request = sha256_base16(canonical_request);
        auto credential_scope = AWSV4::credential_scope(request_date,region,service);
        auto string_to_sign = AWSV4::string_to_sign(STRING_TO_SIGN_ALGO,
                                                    request_date,
                                                    credential_scope,
                                                    hashed_canonical_request);

        if (verbose)
            std::cerr << "-- String to Sign\n" << string_to_sign << "\n----\n" << std::endl;

        auto signature = calculate_signature(request_date,
                                                    secret_key,
                                                    region,
                                                    service,
                                                    string_to_sign,
                                                    verbose);

        const std::string authorization_header = STRING_TO_SIGN_ALGO + " Credential=" + public_key + "/"
                + credential_scope + ", SignedHeaders=" + signed_headers + ", Signature=" + signature;

        return authorization_header;
    }
}
