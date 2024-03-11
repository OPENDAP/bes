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

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <cstdlib>
#include <cstring>
#include <string>
#include <map>
#include <vector>
#include <sstream>
#include <time.h>

#include <curl/curl.h>

#include "BESUtil.h"
#include "BESCatalogUtils.h"
#include "BESCatalogList.h"
#include "BESCatalog.h"
#include "BESRegex.h"
#include "TheBESKeys.h"
#include "BESInternalError.h"
#include "BESNotFoundError.h"
#include "BESSyntaxUserError.h"
#include "BESDebug.h"
#include "BESUtil.h"

#include "HttpNames.h"
#include "HttpUtils.h"
#include "ProxyConfig.h"

#define MODULE "http"

using namespace std;
using namespace http;

// These are static class members

#define prolog string("HttpUtils::").append(__func__).append("() - ")

namespace http {
/**
 * Loads the passed
 * @param mime_list
 */
void load_mime_list_from_keys(map<string, string> &mime_list)
{
    // MimeTypes - translate from a mime type to a module name
    bool found = false;
    vector<string> vals;
    TheBESKeys::TheKeys()->get_values(HTTP_MIMELIST_KEY, vals, found);
    if (found && vals.size()) {
        vector<string>::iterator i = vals.begin();
        vector<string>::iterator e = vals.end();
        for (; i != e; i++) {
            size_t colon = (*i).find(":");
            if (colon == string::npos) {
                string err = (string) "Malformed " + HTTP_MIMELIST_KEY + " " + (*i) +
                             " specified in the gateway configuration";
                throw BESSyntaxUserError(err, __FILE__, __LINE__);
            }
            string mod = (*i).substr(0, colon);
            string mime = (*i).substr(colon + 1);
            mime_list[mod] = mime;
        }
    }
}



/**
 * Look for the type of handler that can read the filename found in the \arg disp.
 * The string \arg disp (probably from a HTTP Content-Dispoition header) has the
 * format:
 *
 * ~~~xml
 * filename[#|=]<value>[ <attribute name>[#|=]<value>]
 * ~~~
 *
 * @param disp The disposition string
 * @param type The type of the handler that can read this file or the empty
 * string if the BES Catalog Utils cannot find a handler to read it.
 */
void get_type_from_disposition(const string &disp, string &type)
{
    // If this function extracts a filename from disp and it matches a handler's
    // regex using the Catalog Utils, this will be set to a non-empty value.
    type = "";

    size_t fnpos = disp.find("filename");
    if (fnpos != string::npos) {
        // Got the filename attribute, now get the
        // filename, which is after the pound sign (#)
        size_t pos = disp.find("#", fnpos);
        if (pos == string::npos) pos = disp.find("=", fnpos);
        if (pos != string::npos) {
            // Got the filename to the end of the
            // string, now get it to either the end of
            // the string or the start of the next
            // attribute
            string filename;
            size_t sp = disp.find(" ", pos);
            if (pos != string::npos) {
                // space before the next attribute
                filename = disp.substr(pos + 1, sp - pos - 1);
            } else {
                // to the end of the string
                filename = disp.substr(pos + 1);
            }

            BESUtil::trim_if_surrounding_quotes(filename);

            // we have the filename now, run it through
            // the type match to get the file type.

            const BESCatalogUtils *utils = BESCatalogList::TheCatalogList()->default_catalog()->get_catalog_utils();
            type = utils->get_handler_name(filename);
        }
    }
}

void get_type_from_content_type(const string &ctype, string &type)
{
    BESDEBUG(MODULE, prolog << "BEGIN content-type: " << ctype << endl);
    map<string,string> mime_list;
    load_mime_list_from_keys(mime_list);
    map<string, string>::iterator i = mime_list.begin();
    map<string, string>::iterator e = mime_list.end();
    bool done = false;
    for (; i != e && !done; i++) {
        BESDEBUG(MODULE, prolog << "Comparing content type '" << ctype << "' against mime list element '" << (*i).second << "'" << endl);
        BESDEBUG(MODULE, prolog << "first: " << (*i).first << "  second: " << (*i).second << endl);
        if ((*i).second == ctype) {
            BESDEBUG(MODULE, prolog << "MATCH" << endl);
            type = (*i).first;
            done = true;
        }
    }
    BESDEBUG(MODULE, prolog << "END" << endl);
}

void get_type_from_url(const string &url, string &type) {
    const BESCatalogUtils *utils = BESCatalogList::TheCatalogList()->find_catalog("catalog")->get_catalog_utils();

    type = utils->get_handler_name(url);
}

/**
 * Loads the value of Http.MaxRedirects from TheBESKeys.
 * If the value is not found, then it is set to the default, HTTP_MAX_REDIRECTS_DEFAULT
 */
size_t load_max_redirects_from_keys(){
    size_t max_redirects=0;
    bool found = false;
    string value;
    TheBESKeys::TheKeys()->get_value(HTTP_MAX_REDIRECTS_KEY, value, found);
    if (found && !value.empty()) {
        std::istringstream(value) >> max_redirects; // Returns 0 if the parse fails.
    }
    if(!max_redirects){
        max_redirects = HTTP_MAX_REDIRECTS_DEFAULT;
    }
    return max_redirects;
}

// http://help.adobe.com/en_US/FlashPlatform/reference/actionscript/3/package.html#encodeURIComponent()



/**
 * Thanks to https://gist.github.com/litefeel for this implementation
 * @param c
 * @param hex1
 * @param hex2
 */
void hexchar(const unsigned char &c, unsigned char &hex1, unsigned char &hex2)
{
    hex1 = c / 16;
    hex2 = c % 16;
    hex1 += hex1 <= 9 ? '0' : 'a' - 10;
    hex2 += hex2 <= 9 ? '0' : 'a' - 10;
}

/**
 * Thanks to https://gist.github.com/litefeel for this implementation
 * @param s
 * @return s, but url encoded
 */
string url_encode(const string &s)
{
    const char *str = s.c_str();
    vector<char> v(s.size());
    v.clear();
    for (size_t i = 0, l = s.size(); i < l; i++)
    {
        char c = str[i];
        if ((c >= '0' && c <= '9') ||
            (c >= 'a' && c <= 'z') ||
            (c >= 'A' && c <= 'Z') ||
            c == '-' || c == '_' || c == '.' || c == '!' || c == '~' ||
            c == '*' || c == '\'' || c == '(' || c == ')')
        {
            v.push_back(c);
        }
        else if (c == ' ')
        {
            v.push_back('+');
        }
        else
        {
            v.push_back('%');
            unsigned char d1, d2;
            hexchar(c, d1, d2);
            v.push_back(d1);
            v.push_back(d2);
        }
    }

    return {v.cbegin(), v.cend()};
}

#if 0
    /**
     * [UTC Sun Jun 21 16:17:47 2020 id: 14314][dmrpp:curl] CurlHandlePool::evaluate_curl_response() - Last Accessed URL(CURLINFO_EFFECTIVE_URL):
     *     https://ghrcw-protected.s3.us-west-2.amazonaws.com/rss_demo/rssmif16d__7/f16_ssmis_20200512v7.nc?
     *         A-userid=hyrax&
     *         X-Amz-Algorithm=AWS4-HMAC-SHA256&
     *         X-Amz-Credential=SomeBigMessyAwfulEncodedEscapeBunchOfCryptoPhaffing&
     *         X-Amz-Date=20200621T161746Z&
     *         X-Amz-Expires=86400&
     *         X-Amz-Security-Token=SomeBigMessyAwfulEncodedEscapeBunchOfCryptoPhaffingSomeBigMessyAwfulEncodedEscapeBunchOfCryptoPhaffing&
     *         X-Amz-SignedHeaders=host&
     *         X-Amz-Signature=SomeBigMessyAwfulEncodedEscapeBunchOfCryptoPhaffing
     * @param source_url
     * @param data_access_url_info
     */


    void HttpUtils::decompose_url(const string target_url, map<string,string> &url_info)
    {
        string url_base;
        string query_string;

        size_t query_index = target_url.find_first_of("?");
        BESDEBUG(MODULE, prolog << "query_index: " << query_index << endl);
        if(query_index != string::npos){
            query_string = target_url.substr(query_index+1);
            url_base = target_url.substr(0,query_index);
        }
        else {
            url_base = target_url;
        }
        url_info.insert( std::pair<string,string>(HTTP_TARGET_URL_KEY,target_url));
        BESDEBUG(MODULE, prolog << HTTP_TARGET_URL_KEY << ": " << target_url << endl);
        url_info.insert( std::pair<string,string>(HTTP_URL_BASE_KEY,url_base));
        BESDEBUG(MODULE, prolog << HTTP_URL_BASE_KEY <<": " << url_base << endl);
        url_info.insert( std::pair<string,string>(HTTP_QUERY_STRING_KEY,query_string));
        BESDEBUG(MODULE, prolog << HTTP_QUERY_STRING_KEY << ": " << query_string << endl);
        if(!query_string.empty()){
            vector<string> records;
            string delimiters = "&";
            BESUtil::tokenize(query_string,records, delimiters);
            vector<string>::iterator i = records.begin();
            for(; i!=records.end(); i++){
                size_t index = i->find('=');
                if(index != string::npos) {
                    string key = i->substr(0, index);
                    string value = i->substr(index+1);
                    BESDEBUG(MODULE, prolog << "key: " << key << " value: " << value << endl);
                    url_info.insert( std::pair<string,string>(key,value));
                }
            }
        }
        time_t now;
        time(&now);  /* get current time; same as: timer = time(NULL)  */
        stringstream unix_time;
        unix_time << now;
        url_info.insert( std::pair<string,string>(HTTP_INGEST_TIME_KEY,unix_time.str()));
    }

#endif

}

