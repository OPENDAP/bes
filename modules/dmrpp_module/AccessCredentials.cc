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
    map<string, string>::iterator it;
    string value={""};
    it = kvp.find(key);
    if (it != kvp.end())
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
    kvp.insert(pair<string, string>(key, value));
}

/**
 * @brief Do the URL, ID, Key amd Region items make up an S3 Credential?
 * @return True
 */
bool AccessCredentials::is_s3_cred() {
    if (!d_s3_tested) {
        d_is_s3 = get(URL_KEY).size() > 0 &&
                get(ID_KEY).size() > 0 &&
                get(KEY_KEY).size() > 0 &&
                get(REGION_KEY).size() > 0; //&&
        //get(BUCKET_KEY).size()>0;
        d_s3_tested = true;
    }
    return d_is_s3;
}

string AccessCredentials::to_json() {
    stringstream ss;
    ss << "{" << endl << "  \"AccessCredentials\": { " << endl;
    ss << "    \"name\": \"" << d_config_name << "\"," << endl;
    for (map<string, string>::iterator it = kvp.begin(); it != kvp.end(); ++it) {
        string key = it->first;
        string value = it->second;

        if (it != kvp.begin())
            ss << ", " << endl;

        ss << "    \"" << it->first << "\": \"" << it->second << "\"";
    }
    ss << endl << "  }" << endl << "}" << endl;
    return ss.str();
}
