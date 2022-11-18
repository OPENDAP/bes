// -*- mode: c++; c-basic-offset:4 -*-
//
// This file is part of cmr_module, A C++ module that can be loaded in to
// the OPeNDAP Back-End Server (BES) and is able to handle remote requests.
//
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
//
#include <sstream>
#include <fstream>

#include "nlohmann/json.hpp"

#include <BESError.h>
#include <BESDebug.h>
#include <BESUtil.h>
#include "RemoteResource.h"

#include "CmrNames.h"

#include "rjson_utils.h"

using namespace std;
using json = nlohmann::json;

#define prolog std::string("json_utils::").append(__func__).append("() - ")

namespace cmr {
/**
 * Utilizes the RemoteHttpResource machinery to retrieve the document
 * referenced by the parameter 'url'. Once retrieved the document is fed to the RapidJSON
 * parser to populate the parameter 'd'
 *
 * @param url The URL of the JSON document to parse.
 * @return The json document parsed from the source URL response..
 *
 */
json rjson_utils::get_as_json(const string &url)
{
    BESDEBUG(MODULE,prolog << "Trying url: " << url << endl);
    shared_ptr<http::url> target_url(new http::url(url));
    http::RemoteResource remoteResource(target_url);
    remoteResource.retrieveResource();
    std::ifstream f(remoteResource.getCacheFileName());
    json data = json::parse(f);
    return data;
}

std::string rjson_utils::typeName(unsigned int t)
{
    const char* tnames[] =
            { "Null", "False", "True", "Object", "Array", "String", "Number" };
    return string(tnames[t]);
}


}  // namespace cmr
