// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of the Hyrax data server.

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

#include "config.h"

#include <string>
#include <sstream>

#include "AccessCredentials.h"

using std::string;
using std::map;
using std::pair;
using std::stringstream;
using std::endl;

namespace http {

const char *AccessCredentials::ID_KEY = "id";
const char *AccessCredentials::KEY_KEY = "key";
const char *AccessCredentials::REGION_KEY = "region";
const char *AccessCredentials::URL_KEY = "url";

/**
 * Retrieves the value of key
 * @param key The key value to retrieve
 * @return The value of the key, empty string if the key does not exist.
 */
string
AccessCredentials::get(const string &key) {
    string value;
    auto const it = d_kvp.find(key);
    if (it != d_kvp.end())
        value = it->second;
    return value;
}

/**
 * @brief Add the key and value pair
 * @param key
 * @param value
 */
void
AccessCredentials::add(const string &key, const string &value) {
    d_kvp.insert(pair<string, string>(key, value));
}

/**
 * @brief Do the URL, ID, Key amd Region items make up an S3 Credential?
 * @return True
 */
bool AccessCredentials::is_s3_cred() {
    if (!d_s3_tested) {
        d_is_s3 = !(get(URL_KEY).empty() || get(ID_KEY).empty() || get(KEY_KEY).empty() || get(REGION_KEY).empty());
        d_s3_tested = true;
    }
    return d_is_s3;
}

string AccessCredentials::to_json() const {
    stringstream ss;
    ss << "{" << endl << R"(  "AccessCredentials": { )" << endl;
    ss << R"(    "name": ")" << d_config_name << R"(",)" << endl;
    bool first = true;
    for (const auto &item: d_kvp) {
        string key = item.first;
        string value = item.second;

        if (first) {
            first = false;
            ss << ", " << endl;
        }

        ss << R"(    ")" << item.first << R"(": ")" << item.second << R"(")";
    }
    ss << endl << "  }" << endl << "}" << endl;
    return ss.str();
}

} // namespace http