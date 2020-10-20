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
/**
 * @brief utility class for the HTTP catalog module
 *
 * This class provides utilities that extract information from a URL
 * or the returned headers of an HTTP response. It also provides
 * storage for a number of values read from the httpd_catalog.conf
 * configuration file.
 *
 * @note This class holds only static methods and fields. It has no
 * constructor or destructor. Use the initialize() method to configure
 * the various static fields based on the values of the BES configuration
 * file(s).
 */
#ifndef  _bes_http_HTTP_UTILS_H_
#define  _bes_http_HTTP_UTILS_H_ 1

#include <string>
#include <map>
#include <vector>

namespace http {

    void load_mime_list_from_keys(std::map<std::string, std::string> &mime_list);
    void load_proxy_from_keys();
    size_t load_max_redirects_from_keys();

    void get_type_from_disposition(const std::string &disp, std::string &type);
    void get_type_from_content_type(const std::string &ctype, std::string &type);
    void get_type_from_url(const std::string &url, std::string &type);


} // namespace http

#endif //  _bes_http_HTTP_UTILS_H_

