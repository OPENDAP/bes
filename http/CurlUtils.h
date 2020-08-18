// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of cmr_module, A C++ module that can be loaded in to
// the OPeNDAP Back-End Server (BES) and is able to handle remote requests.

// Copyright (c) 2013 OPeNDAP, Inc.
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

#ifndef _CURL_UTILS_H_
#define _CURL_UTILS_H_ 1

#include <string>
#include <vector>

#include <curl/curl.h>
#include <curl/easy.h>

#include "rapidjson/document.h"
#include "BESRegex.h"

namespace curl {

    CURL *init(char *error_buffer);

    bool configureProxy(CURL *curl, const std::string &url);

    long read_url(CURL *curl, const std::string &url, int fd, std::vector<std::string> *resp_hdrs,
                  const std::vector<std::string> *headers, char error_buffer[]);

    void http_get(const std::string &url, char *response_buf);
    std::string http_get_as_string(const std::string &url);
    rapidjson::Document http_get_as_json(const std::string &target_url);
    std::string http_status_to_string(int status);
    std::string error_message(const CURLcode response_code, char *error_buf);
    size_t c_write_data(void *buffer, size_t size, size_t nmemb, void *data);
    CURL *set_up_easy_handle(const std::string &target_url, const std::string &cookies_file, char *response_buff);
    bool eval_get_response(CURL *eh, const std::string &requested_url);
    void read_data(CURL *c_handle);
    std::string get_cookie_filename();
    void find_last_redirect(const std::string &url, std::string &last_accessed_url);
    std::string get_range_arg_string(const unsigned long long &offset, const unsigned long long &size);
    void cache_effective_url(const std::string &data_access_url_str);
    bool cache_effective_urls();
    void cache_effective_url(const std::string &data_access_url_str, BESRegex *cache_effective_urls_skip_regex);
    BESRegex *get_cache_effective_urls_skip_regex();
    bool is_retryable(std::string url);


} // namespace curl

#endif /* _CURL_UTILS_H_ */
