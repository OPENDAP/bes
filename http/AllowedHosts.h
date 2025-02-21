// AllowedHosts.h

// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of the OPeNDAP Back-End Server (BES)
// and creates a set of allowed hosts that may be
// accessed by the server as part of it's routine operation.

// Copyright (c) 2018 OPeNDAP, Inc.
// Author: Nathan D. Potter <ndp@opendap.org>
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
//      ndp       Nathan D. Potter <ndp@opendap.org>

#ifndef I_AllowedHosts_H
#define I_AllowedHosts_H 1

#include <memory>
#include <string>
#include <vector>

#include "url_impl.h"

#define ALLOWED_HOSTS_BES_KEY "AllowedHosts"

namespace http {

/**
 * @brief Can a given URL be dereferenced given the BES's configuration?
 *
 * Embodies a configuration based remote access allowed list
 * and provides a simple API, is_allowed() for determining which
 * resources may be accessed. This enables a system administrator to control
 * the remote systems a particular BES daemon can access.
 *
 * @note This class is a singleton
 */
class AllowedHosts {
private:
    std::vector<std::string> d_allowed_hosts;

#if 0

    static void initialize_instance();
    static void delete_instance();

#endif

    bool check(const std::string &url);

    // Private constructor to prevent direct instantiation
    AllowedHosts();
public:
    AllowedHosts(const AllowedHosts &) = delete;
    AllowedHosts &operator=(const AllowedHosts &) = delete;

    virtual ~AllowedHosts() = default;

    // Static member function that returns the pointer to the singleton instance
    static AllowedHosts *theHosts();

    bool is_allowed(const http::url &candidate_url);
    bool is_allowed(const http::url &candidate_url, std::string &whynot);

    bool is_allowed(const std::shared_ptr<http::url> &candidate_url);
    bool is_allowed(const std::shared_ptr<http::url> &candidate_url, std::string &whynot);
};

} // namespace bes

#endif // I_AllowedHosts_H

