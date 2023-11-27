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

///@name Get data from a URL
///@{
void http_get_and_write_resource(const std::shared_ptr<http::url>& target_url, int fd,
                                 std::vector<std::string> *http_response_headers);

void http_get(const std::string &target_url, std::vector<char> &buf);
void http_get(const std::string &target_url, std::string &buf);

void super_easy_perform(CURL *ceh);
///@}

std::string get_redirect_url( const std::shared_ptr<http::url> &url);

std::shared_ptr<http::EffectiveUrl> retrieve_effective_url(const std::shared_ptr<http::url> &starting_point_url);
std::string filter_aws_url(const std::string &eff_url);

CURL *init(const std::string &target_url, const struct curl_slist *http_request_headers,
           std::vector<std::string> *resp_hdrs);

void set_error_buffer(CURL *ceh, char *error_buffer);

std::string get_netrc_filename();

std::string get_cookie_filename();

bool is_retryable(const std::string &url);

unsigned long max_redirects();

std::string hyrax_user_agent();

void eval_curl_easy_setopt_result(CURLcode result,  const std::string &msg_base, const std::string &opt_name,
                                  const char *ebuf, const std::string &file, unsigned int line);

std::string get_range_arg_string(const unsigned long long &offset, const unsigned long long &size);

std::string error_message(CURLcode response_code, const char *error_buf);

void read_data(CURL *c_handle);

curl_slist *append_http_header(curl_slist *slist, const std::string &header_name, const std::string &value);

curl_slist *add_edl_auth_headers(curl_slist *request_headers);

curl_slist *sign_s3_url(const std::shared_ptr<http::url> &target_url, http::AccessCredentials *ac,
                         curl_slist *req_headers);
} // namespace curl

#endif /*  _bes_http_CURL_UTILS_H_ */
