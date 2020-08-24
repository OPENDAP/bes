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
#ifndef _REMOTE_UTILS_H_
#define _REMOTE_UTILS_H_ 1

#include <string>
#include <map>
#include <vector>

namespace http {

/** @brief utility class for the gateway remote request mechanism
 *
 */
    class HttpUtils {
    public:
        static std::map<std::string, std::string> MimeList;
        static std::string ProxyProtocol;
        static std::string ProxyHost;
        static std::string ProxyUserPW;
        static std::string ProxyUser;
        static std::string ProxyPassword;
        static int ProxyPort;
        static int ProxyAuthType;
        static bool useInternalCache;

        static std::string NoProxyRegex;

        static void Initialize();

        static void Get_type_from_disposition(const std::string &disp, std::string &type);

        static void Get_type_from_content_type(const std::string &ctype, std::string &type);

        static void Get_type_from_url(const std::string &url, std::string &type);

        // static void decompose_url(const std::string target_url, std::map<std::string,std::string> &url_info);
    };

} // namespace http

#endif // _REMOTE_UTILS_H_

