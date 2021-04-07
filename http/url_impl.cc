
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
#include <time.h>

#include "BESDebug.h"
#include "BESUtil.h"
#include "BESCatalogList.h"
#include "HttpNames.h"

#include "url_impl.h"

using namespace std;
using std::chrono::system_clock;

#define MODULE HTTP_MODULE
#define prolog string("url::").append(__func__).append("() - ")

#define PROTOCOL_KEY "http_url_protocol"
#define HOST_KEY  "http_url_host"
#define PATH_KEY  "http_url_path"
#define QUERY_KEY "http_url_query"
#define SOURCE_URL_KEY  "http_url_target_url"
#define INGEST_TIME_KEY  "http_url_ingest_time"


namespace http {

#if 0
/**
 *
 * @param kvp
 */
url::url(const map<string,string> &kvp)
{
    map<string,string> kvp_copy = kvp;
    map<string,string>::const_iterator it;
    map<string,string>::const_iterator itc;

    it = kvp.find(PROTOCOL_KEY);
    itc = kvp_copy.find(PROTOCOL_KEY);
    if(it != kvp.end() && itc != kvp_copy.end()){
        d_protocol = it->second;
        kvp_copy.erase(it->first);
        BESDEBUG(MODULE, prolog << "Located PROTOCOL_KEY(" << PROTOCOL_KEY << ") value: " << d_protocol << endl);
    }
    it = kvp.find(HOST_KEY);
    itc = kvp_copy.find(HOST_KEY);
    if(it != kvp.end() && itc != kvp_copy.end()){
        d_host = it->second;
        kvp_copy.erase(it->first);
        BESDEBUG(MODULE, prolog << "Located HOST_KEY(" << HOST_KEY << ") value: " << d_host << endl);
    }
    it = kvp.find(PATH_KEY);
    itc = kvp_copy.find(PATH_KEY);
    if(it != kvp.end() && itc != kvp_copy.end()){
        d_path = it->second;
        kvp_copy.erase(it->first);
        BESDEBUG(MODULE, prolog << "Located PATH_KEY(" << PATH_KEY << ") value: " << d_path << endl);
    }
    it = kvp.find(QUERY_KEY);
    itc = kvp_copy.find(QUERY_KEY);
    if(it != kvp.end() && itc != kvp_copy.end()){
        d_query = it->second;
        kvp_copy.erase(it->first);
        BESDEBUG(MODULE, prolog << "Located QUERY_KEY(" << QUERY_KEY << ") value: " << d_query << endl);
    }
    it = kvp.find(SOURCE_URL_KEY);
    itc = kvp_copy.find(SOURCE_URL_KEY);
    if(it != kvp.end() && itc != kvp_copy.end()){
        d_source_url_str = it->second;
        kvp_copy.erase(it->first);
        BESDEBUG(MODULE, prolog << "Located SOURCE_URL_KEY(" << SOURCE_URL_KEY << ") value: " << d_source_url_str << endl);
    }

    for(itc = kvp_copy.begin(); itc != kvp_copy.end(); itc++){
        string key =  itc->first;
        string value = itc->second;
        map<string, vector<string>* >::const_iterator record_it;
        record_it = d_query_kvp.find(key);
        if(record_it != d_query_kvp.end()){
            vector<string> *values = record_it->second;
            values->push_back(value);
        }
        else {
            vector<string> *values = new vector<string>();
            values->push_back(value);
            d_query_kvp.insert(pair<string, vector<string>*>(key, values));
        }
    }

}
#endif

/**
 *
 */
url::~url()
{
    if(!d_query_kvp.empty()){
        map<string, vector<string>* >::const_iterator it;
        for(it = d_query_kvp.begin() ; it != d_query_kvp.end(); it++){
            delete it->second;
        }
    }
}


/**
 * @brief Parses the URL into it's components and makes some BES file system magic.
 *
 *
 * Tip of the hat to: https://stackoverflow.com/questions/2616011/easy-way-to-parse-a-url-in-c-cross-platform
 * @param source_url
 */
void url::parse() {
    const string protcol_end("://");

    // If the supplied string does not start with a protocol, we assume it must be a
    // path relative the BES.Catalog.catalog.RootDirectory because that's the only
    // thing we are going to allow, even when it starts with slash '/'. Basically
    // we force it to be in the BES.Catalog.catalog.RootDirectory tree.
    if(d_source_url_str.find(protcol_end) == string::npos){
        // Since we want a valid path in the file system tree for data we make it so by adding the
        // the file path starts with the catalog root dir.
        BESCatalogList *bcl = BESCatalogList::TheCatalogList();
        string default_catalog_name = bcl->default_catalog_name();
        BESDEBUG(MODULE, prolog << "Searching for  catalog: " << default_catalog_name << endl);
        BESCatalog *bcat = bcl->find_catalog(default_catalog_name);
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

    string::const_iterator prot_i = search(parse_url_target.begin(), parse_url_target.end(),
                                           protcol_end.begin(), protcol_end.end());

    if (prot_i != parse_url_target.end())
        advance(prot_i, protcol_end.length());

    d_protocol.reserve(distance(parse_url_target.begin(), prot_i));
    transform(parse_url_target.begin(), prot_i,
              back_inserter(d_protocol),
              ptr_fun<int, int>(tolower)); // protocol is icase
    if (prot_i == parse_url_target.end())
        return;

    if (d_protocol == FILE_PROTOCOL) {
        d_path = parse_url_target.substr(parse_url_target.find(protcol_end) + protcol_end.length());
        BESDEBUG(MODULE, prolog << "FILE_PROTOCOL d_path: " << d_path << endl);
    }
    else if( d_protocol == HTTP_PROTOCOL || d_protocol == HTTPS_PROTOCOL){
        string::const_iterator path_i = find(prot_i, parse_url_target.end(), '/');
        d_host.reserve(distance(prot_i, path_i));
        transform(prot_i, path_i,
                  back_inserter(d_host),
                  ptr_fun<int, int>(tolower)); // host is icase
        string::const_iterator query_i = find(path_i, parse_url_target.end(), '?');
        d_path.assign(path_i, query_i);
        if (query_i != parse_url_target.end())
            ++query_i;
        d_query.assign(query_i, parse_url_target.end());

        if (!d_query.empty()) {
            vector<string> records;
            string delimiters = "&";
            BESUtil::tokenize(d_query, records, delimiters);
            vector<string>::iterator i = records.begin();
            for (; i != records.end(); i++) {
                size_t index = i->find('=');
                if (index != string::npos) {
                    string key = i->substr(0, index);
                    string value = i->substr(index + 1);
                    BESDEBUG(MODULE, prolog << "key: " << key << " value: " << value << endl);
                    map<string, vector<string> *>::const_iterator record_it;
                    record_it = d_query_kvp.find(key);
                    if (record_it != d_query_kvp.end()) {
                        vector<string> *values = record_it->second;
                        values->push_back(value);
                    } else {
                        vector<string> *values = new vector<string>();
                        values->push_back(value);
                        d_query_kvp.insert(pair<string, vector<string> *>(key, values));
                    }
                }
            }
        }
    }
    else {
        stringstream msg;
        msg << prolog << "Unsupported URL protocol " << d_protocol << " found in URL: " << d_source_url_str;
        BESDEBUG(MODULE, msg.str() << endl);
        throw BESInternalError(msg.str(), __FILE__, __LINE__);
    }
}


/**
 *
 * @param key
 * @return
 */
string url::query_parameter_value(const string &key) const
{
    string value;
    map<string, vector<string>* >::const_iterator it;
    it = d_query_kvp.find(key);
    if(it != d_query_kvp.end()){
        vector<string> *values = it->second;
        if(!values->empty()){
            value = (*values)[0];
        }
    }
    return value;
}

/**
 *
 * @param key
 * @param values
 */
void url::query_parameter_values(const string &key, vector<string> &values) const
{
    map<string, vector<string>* >::const_iterator it;
    it = d_query_kvp.find(key);
    if(it != d_query_kvp.end()){
        values = *it->second;
    }
}

#if 0

/**
 *
 * @param kvp
 */
void url::kvp(map<string,string>  &kvp){
    stringstream ss;

    // Do the basic stuff
    kvp.insert(pair<string,string>(PROTOCOL_KEY, d_protocol));
    kvp.insert(pair<string,string>(HOST_KEY, d_host));
    kvp.insert(pair<string,string>(PATH_KEY, d_path));
    kvp.insert(pair<string,string>(QUERY_KEY, d_query));
    kvp.insert(pair<string,string>(SOURCE_URL_KEY, d_source_url_str));
    ss << d_ingest_time;
    kvp.insert(pair<string,string>(INGEST_TIME_KEY,ss.str()));

    // Now grab the query string. Only the first value of multi valued keys is used.
    map<string, vector<string>* >::const_iterator it;
    for(it=d_query_kvp.begin(); it != d_query_kvp.end(); it++){
        kvp.insert(pair<string,string>(it->first,(*it->second)[0]));
    }
}
#endif

/**
 *
 * @return True if the URL appears within the REFRESH_THRESHOLD of the
 * expires time read from one of CLOUDFRONT_EXPIRES_HEADER_KEY, AMS_EXPIRES_HEADER_KEY;
 *
 */
bool url::is_expired()
{

    bool stale;
    std::time_t now = system_clock::to_time_t(system_clock::now());

    BESDEBUG(MODULE, prolog << "now: " << now << endl);
    // We set the expiration time to the default, in case other avenues don't work out so well.
    std::time_t  expires_time = ingest_time() + HTTP_EFFECTIVE_URL_DEFAULT_EXPIRES_INTERVAL;

    string cf_expires = query_parameter_value(CLOUDFRONT_EXPIRES_HEADER_KEY);
    string aws_expires_str = query_parameter_value(AMS_EXPIRES_HEADER_KEY);

    if(!cf_expires.empty()){ // CloudFront expires header?
        std::istringstream(cf_expires) >> expires_time;
        BESDEBUG(MODULE, prolog << "Using "<< CLOUDFRONT_EXPIRES_HEADER_KEY << ": " << expires_time << endl);
    }
    else if(!aws_expires_str.empty()){

        long long aws_expires;
        std::istringstream(aws_expires_str) >> aws_expires;
        // AWS Expires header?
        //
        // By default we'll use the time we made the URL object, ingest_time
        std::time_t aws_start_time = ingest_time();

        // But if there's an AWS Date we'll parse that and compute the time
        // @TODO move to NgapApi::decompose_url() and add the result to the map
        string aws_date = query_parameter_value(AWS_DATE_HEADER_KEY);

        if(!aws_date.empty()){

            string date = aws_date; // 20200624T175046Z
            string year = date.substr(0,4);
            string month = date.substr(4,2);
            string day = date.substr(6,2);
            string hour = date.substr(9,2);
            string minute = date.substr(11,2);
            string second = date.substr(13,2);

            BESDEBUG(MODULE, prolog << "date: "<< date <<
                                    " year: " << year << " month: " << month << " day: " << day <<
                                    " hour: " << hour << " minute: " << minute  << " second: " << second << endl);

            std::time_t old_now;
            time(&old_now);  /* get current time; same as: timer = time(NULL)  */
            BESDEBUG(MODULE, prolog << "old_now: " << old_now << endl);
            struct tm *ti = gmtime(&old_now);
            ti->tm_year = stoll(year) - 1900;
            ti->tm_mon = stoll(month) - 1;
            ti->tm_mday = stoll(day);
            ti->tm_hour = stoll(hour);
            ti->tm_min = stoll(minute);
            ti->tm_sec = stoll(second);

            BESDEBUG(MODULE, prolog << "ti->tm_year: "<< ti->tm_year <<
                                    " ti->tm_mon: " << ti->tm_mon <<
                                    " ti->tm_mday: " << ti->tm_mday <<
                                    " ti->tm_hour: " << ti->tm_hour <<
                                    " ti->tm_min: " << ti->tm_min <<
                                    " ti->tm_sec: " << ti->tm_sec << endl);


            aws_start_time = mktime(ti);
            BESDEBUG(MODULE, prolog << "AWS start_time (computed): " << aws_start_time << endl);
        }

        expires_time = aws_start_time + aws_expires;
        BESDEBUG(MODULE, prolog << "Using "<< AMS_EXPIRES_HEADER_KEY << ": " << aws_expires <<
                                " (expires_time: " << expires_time << ")" << endl);
    }
    std::time_t remaining = expires_time - now;
    BESDEBUG(MODULE, prolog << "expires_time: " << expires_time <<
                            "  remaining: " << remaining <<
                            " threshold: " << HTTP_URL_REFRESH_THRESHOLD << endl);

    stale = remaining < HTTP_URL_REFRESH_THRESHOLD;
    BESDEBUG(MODULE, prolog << "stale: " << (stale?"true":"false") << endl);

    return stale;
}

/**
 * Returns a string representation of the URL and its bits.
 * @return the representation mentioned above.
 */
string url::dump(){
    stringstream ss;
    string indent_inc = "  ";
    string indent = indent_inc;

    ss << "http::url [" << this << "] " << endl;
    ss << indent << "d_source_url_str: " << d_source_url_str << endl;
    ss << indent << "d_protocol:   " << d_protocol << endl;
    ss << indent << "d_host:       " << d_host << endl;
    ss << indent << "d_path:       " << d_path << endl;
    ss << indent << "d_query:      " << d_query << endl;

    std::map<std::string, std::vector<std::string>* >::iterator it;

    string idt = indent+indent_inc;
    for(it=d_query_kvp.begin(); it !=d_query_kvp.end(); it++){
        ss << indent << "d_query_kvp["<<it->first<<"]: " << endl;
        std::vector<std::string> *values = it->second;
        for(size_t i=0; i<values->size(); i++){
            ss << idt << "value[" << i << "]: " << (*values)[i] << endl;
        }
    }
    ss << indent << "d_ingest_time:      " << d_ingest_time.time_since_epoch().count() << endl;
    return ss.str();
}



} // namespace http