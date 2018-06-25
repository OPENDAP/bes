// RempteAccess.cc

// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of the OPeNDAP Back-End Server (BES)
// and embodies a whitelist of remote system that may be
// accessed by the server as part of it's routine operation.

// Copyright (c) 2018 OPeNDAP, Inc.
// Author:Nathan D. Potter <ndp@opendap.org>
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

#include "config.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <cstdlib>
#include <cstring>
#include <curl/curl.h>

#include "RemoteAccess.h"

#include <BESUtil.h>
#include <BESCatalogUtils.h>
#include <BESRegex.h>
#include <TheBESKeys.h>
#include <BESInternalError.h>
#include <BESSyntaxUserError.h>
#include <BESDebug.h>

#include <util.h>

using namespace bes;


std::vector<string> RemoteAccess::WhiteList;

// Initialization routine for the gateway module for certain parameters
// and keys, like the white list, the MimeTypes translation.
void RemoteAccess::Initialize()
{
    // Whitelist - list of domain that the gateway is allowed to
    // communicate with.
    bool found = false;
    string key = REMOTE_ACCESS_WHITELIST;
    TheBESKeys::TheKeys()->get_values(key, WhiteList, found);

    /*
    if (!found || WhiteList.size() == 0) {
        string err = (string) "The parameter " + REMOTE_ACCESS_WHITELIST + " is not set or has no values in the gateway"
            + " configuration file";
        throw BESSyntaxUserError(err, __FILE__, __LINE__);
    }
    */
}


bool RemoteAccess::Is_Whitelisted(const std::string &url){
    bool whitelisted = false;
    std::vector<std::string>::const_iterator i = WhiteList.begin();
    std::vector<std::string>::const_iterator e = WhiteList.end();
    for (; i != e && !whitelisted; i++) {
        if ((*i).length() <= url.length()) {
            if (url.substr(0, (*i).length()) == (*i)) {
                whitelisted = true;
            }
        }
    }
    return whitelisted;
}





