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

#ifndef NGAP_CURL_UTILS_H_
#define NGAP_CURL_UTILS_H_

#include <string>
#include <vector>

#include <curl/curl.h>
#include <curl/easy.h>

#include "rapidjson/document.h"

namespace BESCurlUtils {

CURL *init(char *error_buffer);

bool configureProxy(CURL *curl, const std::string &url);

long read_url(CURL *curl, const std::string &url, int fd, std::vector<std::string> *resp_hdrs,
              const std::vector<std::string> *headers, char error_buffer[]);

std::string http_status_to_string(int status);

std::string http_get_as_string(const std::string &target_url);

rapidjson::Document http_get_as_json(const std::string &target_url);

} // namespace bes_curl

#endif /* NGAP_CURL_UTILS_H_ */
