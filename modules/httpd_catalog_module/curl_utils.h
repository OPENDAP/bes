// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of BES httpd_catalog_module, A C++ module that can be loaded in to
// the OPeNDAP Back-End Server (BES) and is able to handle remote requests.

// Copyright (c) 2018 OPeNDAP, Inc.
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

// Please read the full copyright statement in the file COPYRIGHT_URI.
//
// Authors:
//      ndp       Nathan Potter <ndp@opendap.org>

#ifndef I_HTTPD_CURL_UTILS_H_
#define I_HTTPD_CURL_UTILS_H_

#include <string>
#include <vector>

#include <curl/curl.h>
#include <curl/easy.h>

namespace httpd_catalog {

CURL *init(char *error_buffer);

bool configureProxy(CURL *curl, const std::string &url);

long read_url(CURL *curl, const std::string &url, int fd, std::vector<std::string> *resp_hdrs,
    const std::vector<std::string> *headers, char error_buffer[]);

std::string http_status_to_string(int status);

} // namespace httpd_catalog

#endif /* I_HTTPD_CURL_UTILS_H_ */
