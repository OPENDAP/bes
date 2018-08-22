// RemoteAccess.h

// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of the OPeNDAP Back-End Server (BES)
// and embodies a whitelist of remote system that may be
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

#ifndef I_RemoteAccess_H
#define I_RemoteAccess_H 1

#include <string>
#include <vector>

#define REMOTE_ACCESS_WHITELIST "Gateway.Whitelist"

namespace bes {

/**
 * @brief Can a given URL be dereferenced given the BES's configuration?
 *
 * Embodies a configuration based remote access white list
 * and provides a simple API (Is_Whitelisted()) for determining which
 * resources may be accessed. This enables a system administrator to control
 * the remote systems a particular BES daemon can access.
 *
 * @note This class is a singleton
 */
class WhiteList {
private:
	static WhiteList *d_instance;

    std::vector<std::string> d_white_list;

protected:
    WhiteList();

public:
    virtual ~WhiteList() {}

    static WhiteList *get_white_list();

    bool is_white_listed(const std::string &url);
};

} // namespace bes

#endif // I_RemoteAccess_H

