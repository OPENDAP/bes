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

#ifndef  _bes_http_CURL_UTILS_H_
#define  _bes_http_CURL_UTILS_H_ 1

#include <memory>
#include <string>
#include <vector>

#include <curl/curl.h>
#include <curl/easy.h>

#include "BESRegex.h"
#include "EffectiveUrl.h"

namespace http {
class AccessCredentials;

class EffectiveUrl;

class url;
}

namespace curl {

std::shared_ptr<http::EffectiveUrl> get_redirect_url(const std::shared_ptr<http::url> &origin_url);

std::string filter_aws_url(const std::string &eff_url);

CURL *init(const std::string &target_url, const curl_slist *http_request_headers,
           std::vector<std::string> *resp_hdrs);

void set_error_buffer(CURL *ceh, char *error_buffer);

std::string get_netrc_filename();

std::string get_cookie_filename();

bool is_retryable(const std::string &url);

unsigned long max_redirects();

std::string hyrax_user_agent();

void eval_curl_easy_setopt_result(CURLcode result, const std::string &msg_base, const std::string &opt_name,
                                  const char *ebuf, const std::string &file, unsigned int line);

std::string get_range_arg_string(const unsigned long long &offset, const unsigned long long &size);

std::string error_message(CURLcode response_code, const char *error_buf);

void read_data(CURL *c_handle);

curl_slist *append_http_header(curl_slist *slist, const std::string &header_name, const std::string &value);

curl_slist *add_edl_auth_headers(curl_slist *request_headers);

curl_slist *sign_s3_url(const std::string &target_url, http::AccessCredentials *ac, curl_slist *req_headers);

// TODO Remove when no longer needed. jhrg 2/20/25
curl_slist *sign_s3_url(const std::shared_ptr<http::url> &target_url, http::AccessCredentials *ac,
                        curl_slist *req_headers);

bool is_url_signed_for_s3(const std::string &url);
bool is_url_signed_for_s3(const std::shared_ptr<http::url> &target_url);

///@name Get data from a URL
///@{
#if 1
void http_get_and_write_resource(const std::shared_ptr<http::url> &target_url, int fd,
                                 std::vector<std::string> *http_response_headers);
#endif
#if 0
// No longer defined. jhrg 11/12/25
void http_get(const std::string &target_url, std::vector<char> &buf);
#endif

bool http_head(const std::string &target_url, int tries = 3, unsigned long wait_time_us = 1'000'000);

/// @brief General HTTP GET call using libcurl that accepts a list of request headers.
/// @note This is intended to be a 'private' helper function.
void http_get(const std::string &target_url, curl_slist *request_headers, std::string &buf);

/// @brief General HTTP GET call using libcurl
inline void http_get(const std::string &target_url, std::string &buf) {
    http_get(target_url, nullptr, buf);
}

#if 0
/// @brief HTTP GET tailored for NASA's Earthdata Cloud environment
inline void http_get_nasa_edc(const std::string &target_url, std::string &buf) {
    curl_slist *request_headers = nullptr;
    try {
        request_headers = add_edl_auth_headers(request_headers);
        bool found = false;
        std::string s = BESContextManager::TheManager()->get_context(EDL_UID_KEY, found);
        if (found && !s.empty()) {
            request_headers = append_http_header(request_headers, "Client-Id", s);
        }

        http_get(target_url, request_headers, buf);
    }
    catch (...) {
        curl_slist_free_all(request_headers);
        throw;
    }
}
#endif

void super_easy_perform(CURL *ceh);
///@}

} // namespace curl

#endif /*  _bes_http_CURL_UTILS_H_ */
