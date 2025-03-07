

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

#ifndef AWSV4_HPP
#define AWSV4_HPP

#include <cstdio>
#include <memory>
#include <map>
#include <vector>
#include <ctime>
#include <iostream>

#include <openssl/sha.h>

#include "url_impl.h"

namespace AWSV4 {

const std::string ENDL{"\n"};
const std::string POST{"POST"};
const std::string GET{"GET"};
const std::string STRING_TO_SIGN_ALGO{"AWS4-HMAC-SHA256"};
const std::string AWS4{"AWS4"};
const std::string AWS4_REQUEST{"aws4_request"};

std::string join(const std::vector<std::string> &ss, const std::string &delim);

std::string sha256_base16(const std::string &str);

std::map<std::string, std::string> canonicalize_headers(const std::vector<std::string> &headers);

std::string map_headers_string(const std::map<std::string, std::string> &header_key2val);

std::string map_signed_headers(const std::map<std::string, std::string> &header_key2val);

std::string canonicalize_request(const std::string &http_request_method,
                                       const std::string &canonical_uri,
                                       const std::string &canonical_query_string,
                                       const std::string &canonical_headers,
                                       const std::string &signed_headers,
                                       const std::string &payload);

std::string string_to_sign(const std::string &algorithm,
                                 const std::time_t &request_date,
                                 const std::string &credential_scope,
                                 const std::string &hashed_canonical_request);

std::string ISO8601_date(const std::time_t &t);

std::string utc_yyyymmdd(const std::time_t &t);

std::string credential_scope(const std::time_t &t,
                                   const std::string &region,
                                   const std::string &service);

std::string calculate_signature(const std::time_t &request_date,
                                      const std::string &secret,
                                      const std::string &region,
                                      const std::string &service,
                                      const std::string &string_to_sign);

// The whole enchilada. Added jhrg 11/25/19
std::string compute_awsv4_signature(const std::shared_ptr<http::url> &uri_str, const std::time_t &request_date,
                                    const std::string &public_key, const std::string &secret_key,
                                    const std::string &region, const std::string &service = "s3");

// Added these two as part of removing the use of std::shared_ptr<http::url>. jhrg 2/20/25
std::string compute_awsv4_signature(const http::url &uri, const std::time_t &request_date,
                                    const std::string &public_key, const std::string &secret_key,
                                    const std::string &region, const std::string &service = "s3");

std::string compute_awsv4_signature(const std::string &canonical_uri, const std::string &canonical_query,
                                    const std::string &host, const std::time_t &request_date,
                                    const std::string &public_key, const std::string &secret_key,
                                    const std::string &region, const std::string &service);
}

#endif
