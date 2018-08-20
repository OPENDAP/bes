// GatewayUtils.h

// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of gateway_module, A C++ module that can be loaded in to
// the OPeNDAP Back-End Server (BES) and is able to handle remote requests.

// Copyright (c) 2002,2003 OPeNDAP, Inc.
// Author: Patrick West <pwest@ucar.edu>
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
//      pcw       Patrick West <pwest@ucar.edu>

#ifndef I_CmrUtils_H
#define I_CmrUtils_H 1

#include <string>
#include <map>
#include <vector>

#include "Granule.h"

namespace cmr {

/** @brief utility class for the gateway remote request mechanism
 *
 */
class CmrUtils {
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

    static Granule *getTemporalFacetGranule(const std::string granule_path);
};

} // namespace cmr

#endif // I_CmrUtils_H

