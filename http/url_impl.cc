
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

#include "config.h"

#include <string>
#include <sstream>
#include <map>
#include <vector>
#include <algorithm>
#include <cctype>
#include <functional>
#include <ctime>

#include "BESDebug.h"
#include "BESUtil.h"
#include "BESCatalogList.h"
#include "HttpNames.h"

#include "url_impl.h"

using namespace std;
using std::chrono::system_clock;

#define MODULE HTTP_MODULE
#define prolog string("url::").append(__func__).append("() - ")

namespace http {

/**
 * @brief Parses the URL into it's components and makes some BES file system magic.
 *
 * Tip of the hat to: https://stackoverflow.com/questions/2616011/easy-way-to-parse-a-url-in-c-cross-platform
 */
void url::parse() {
    const string protocol_end("://");
    BESDEBUG(MODULE, prolog << "BEGIN (parsing: '" << d_source_url_str << "')" << endl);

    // If the supplied string does not start with a protocol, we assume it must be a
    // path relative the BES.Catalog.catalog.RootDirectory because that's the only
    // thing we are going to allow, even when it starts with slash '/'. Basically
    // we force it to be in the BES.Catalog.catalog.RootDirectory tree.
    if(d_source_url_str.find(protocol_end) == string::npos){
        // Since we want a valid path in the file system tree for data, we make it so by adding
        // the file path that starts with the catalog root dir.
        const BESCatalogList *bcl = BESCatalogList::TheCatalogList();
        string default_catalog_name = bcl->default_catalog_name();
        BESDEBUG(MODULE, prolog << "Searching for  catalog: " << default_catalog_name << endl);
        const BESCatalog *bcat = bcl->find_catalog(default_catalog_name);
        if (bcat) {
            BESDEBUG(MODULE, prolog << "Found catalog: " << bcat->get_catalog_name() << endl);
        } else {
            string msg = "OUCH! Unable to locate default catalog!";
            BESDEBUG(MODULE, prolog << msg << endl);
            throw BESInternalError(msg, __FILE__, __LINE__);
        }
        string catalog_root = bcat->get_root();
        BESDEBUG(MODULE, prolog << "Catalog root: " << catalog_root << endl);

        string file_path = BESUtil::pathConcat(catalog_root,d_source_url_str);
        if(file_path[0] != '/')
            file_path = "/" + file_path;
        d_source_url_str = FILE_PROTOCOL + file_path;
    }

    const string parse_url_target(d_source_url_str);

    auto prot_i = search(parse_url_target.cbegin(), parse_url_target.cend(),
                         protocol_end.begin(), protocol_end.end());

    if (prot_i != parse_url_target.end())
        advance(prot_i, protocol_end.size());

    d_protocol.reserve(distance(parse_url_target.begin(), prot_i));
    transform(parse_url_target.begin(), prot_i,
              back_inserter(d_protocol),
              [](int c) { return tolower(c); }); // protocol is icase
    if (prot_i == parse_url_target.end())
        return;

    if (d_protocol == FILE_PROTOCOL) {
        d_path = parse_url_target.substr(d_protocol.size());
        BESDEBUG(MODULE, prolog << "FILE_PROTOCOL d_path: " << d_path << endl);
    }
    else if( d_protocol == HTTP_PROTOCOL || d_protocol == HTTPS_PROTOCOL){
        // parse the host
        const auto path_i = find(prot_i, parse_url_target.cend(), '/');
        d_host.reserve(distance(prot_i, path_i));
        transform(prot_i, path_i, back_inserter(d_host), [](int c) { return tolower(c); });
        // parse the path
        auto query_i = find(path_i, parse_url_target.cend(), '?');
        d_path.assign(path_i, query_i);
        // extract the query string
        if (query_i != parse_url_target.cend())
            ++query_i;
        d_query.assign(query_i, parse_url_target.cend());

        // parse the query string KVPs
        if (!d_query.empty()) {
            parse_query_string();
        }
    }
    else {
        stringstream msg;
        msg << prolog << "Unsupported URL protocol " << d_protocol << " found in URL: " << d_source_url_str;
        BESDEBUG(MODULE, msg.str() << endl);
        throw BESInternalError(msg.str(), __FILE__, __LINE__);
    }

    BESDEBUG(MODULE, prolog << "END (parsing: '" << d_source_url_str << "')" << endl);
}

/**
 * @brief Helper method to parse the query string KVP data
 */
void url::parse_query_string() {
    vector<string> records;
    string delimiters = "&";
    BESUtil::tokenize(d_query, records, delimiters);
    for (const auto &kvp: records) {
        size_t index = kvp.find('=');
        if (index != string::npos) {
            string key = kvp.substr(0, index);
            string value = kvp.substr(index + 1);
            BESDEBUG(MODULE, prolog << "key: " << key << " value: " << value << endl);

            const auto &record_it = d_query_kvp.find(key);
            if (record_it != d_query_kvp.end()) {
                record_it->second.push_back(value);
            } else {
                vector<string> values{value};
                d_query_kvp[key] = values;
            }
        }
    }
}

/**
 * @brief Get the value of a query string key.
 * @param key Key for the KVP query params
 * @return The associated value for the key or an empty string if the key is not found.
 */
string url::query_parameter_value(const string &key) const {
    const auto &it = d_query_kvp.find(key);
    if (it != d_query_kvp.end()) {
        vector<string> values = it->second;
        if (!it->second.empty()) {
            return  it->second[0];
        }
    }
    return "";
}

/**
 * @brief Return the number of query string values for a given key .
 * @param key Key for the KVP query params
 * @return Return the number of values or zero if the key is not found.
 */
size_t url::query_parameter_values_size(const std::string &key) const {
    const auto &it = d_query_kvp.find(key);
    if (it != d_query_kvp.end()) {
        return it->second.size();
    }
    return 0;
}

/**
 * @brief Get the vector of query string values for a given key.
 * @param key Key for the KVP query params
 * @return Return a const reference to a string of values. Throws BESInternalError
 * if the key is not found.
 * @see query_parameter_values_size() to test if the key is present.
 */
const vector<string> &url::query_parameter_values(const std::string &key) const {
    const auto &it = d_query_kvp.find(key);
    if (it != d_query_kvp.end()) {
        return it->second;
    }
    else {
        throw BESInternalError(string("Key '") + key + "' not found in url::query_parameter_values().", __FILE__, __LINE__);
    }
}

/**
 * @return True if the URL appears within the REFRESH_THRESHOLD of the expires time
 * read from one of CLOUDFRONT_EXPIRES_HEADER_KEY or AMS_EXPIRES_HEADER_KEY.
 */
bool url::is_expired()
{
    bool stale;
    std::time_t now = system_clock::to_time_t(system_clock::now());

    BESDEBUG(MODULE, prolog << "now: " << now << endl);
    // We set the expiration time to the default, in case other avenues don't work out so well.
    std::time_t expires_time = ingest_time() + HTTP_EFFECTIVE_URL_DEFAULT_EXPIRES_INTERVAL;

    string cf_expires = query_parameter_value(CLOUDFRONT_EXPIRES_HEADER_KEY);
    string aws_expires_str = query_parameter_value(AMS_EXPIRES_HEADER_KEY);

    if (!cf_expires.empty()) { // CloudFront expires header?
        std::istringstream(cf_expires) >> expires_time;
        BESDEBUG(MODULE, prolog << "Using " << CLOUDFRONT_EXPIRES_HEADER_KEY << ": " << expires_time << endl);
    }
    else if (!aws_expires_str.empty()) {
        long long aws_expires;
        std::istringstream(aws_expires_str) >> aws_expires;
        // AWS Expires header?
        //
        // By default, we'll use the time we made the URL object, ingest_time
        std::time_t aws_start_time = ingest_time();

        // But if there's an AWS Date we'll parse that and compute the time
        string aws_date = query_parameter_value(AWS_DATE_HEADER_KEY);

        if (!aws_date.empty()) {
            // aws_date looks like: 20200624T175046Z
            string year = aws_date.substr(0, 4);
            string month = aws_date.substr(4, 2);
            string day = aws_date.substr(6, 2);
            string hour = aws_date.substr(9, 2);
            string minute = aws_date.substr(11, 2);
            string second = aws_date.substr(13, 2);

            BESDEBUG(MODULE, prolog << "date: " << aws_date <<
                                    " year: " << year << " month: " << month << " day: " << day <<
                                    " hour: " << hour << " minute: " << minute << " second: " << second << endl);

            std::time_t old_now;
            time(&old_now);  /* get current time; same as: timer = time(NULL)  */
            BESDEBUG(MODULE, prolog << "old_now: " << old_now << endl);
            struct tm ti{};
            gmtime_r(&old_now, &ti);
            ti.tm_year = stoi(year) - 1900;
            ti.tm_mon = stoi(month) - 1;
            ti.tm_mday = stoi(day);
            ti.tm_hour = stoi(hour);
            ti.tm_min = stoi(minute);
            ti.tm_sec = stoi(second);

            BESDEBUG(MODULE, prolog << "ti.tm_year: " << ti.tm_year <<
                                    " ti.tm_mon: " << ti.tm_mon <<
                                    " ti.tm_mday: " << ti.tm_mday <<
                                    " ti.tm_hour: " << ti.tm_hour <<
                                    " ti.tm_min: " << ti.tm_min <<
                                    " ti.tm_sec: " << ti.tm_sec << endl);

            aws_start_time = mktime(&ti);
            BESDEBUG(MODULE, prolog << "AWS start_time (computed): " << aws_start_time << endl);
        }

        expires_time = aws_start_time + aws_expires;
        BESDEBUG(MODULE, prolog << "Using " << AMS_EXPIRES_HEADER_KEY << ": " << aws_expires <<
                                " (expires_time: " << expires_time << ")" << endl);
    }

    std::time_t remaining = expires_time - now;
    BESDEBUG(MODULE, prolog << "expires_time: " << expires_time <<
                            "  remaining: " << remaining <<
                            " threshold: " << HTTP_URL_REFRESH_THRESHOLD << endl);

    stale = remaining < HTTP_URL_REFRESH_THRESHOLD;
    BESDEBUG(MODULE, prolog << "stale: " << (stale ? "true" : "false") << endl);

    return stale;
}

/**
 * Returns a string representation of the URL and its bits.
 * @return the representation mentioned above.
 */
string url::dump(){
    stringstream ss;
    string indent = "  ";

    ss << "http::url [" << this << "] " << endl;
    ss << indent << "d_source_url_str: " << d_source_url_str << endl;
    ss << indent << "d_protocol:   " << d_protocol << endl;
    ss << indent << "d_host:       " << d_host << endl;
    ss << indent << "d_path:       " << d_path << endl;
    ss << indent << "d_query:      " << d_query << endl;

    string idt = indent+indent;
    for(const auto &it: d_query_kvp) {
        ss << indent << "d_query_kvp["<<it.first<<"]: " << endl;
        int i = 0;
        for(const auto &v: it.second) { // second is a vector<string>
            ss << idt << "value[" << i << "]: " << v << endl;
            i += 1;
        }
    }
    ss << indent << "d_ingest_time:      " << d_ingest_time.time_since_epoch().count() << endl;
    return ss.str();
}

} // namespace http