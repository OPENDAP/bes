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
#include <map>
#include <vector>
#include <sstream>
#include <time.h>

#include <curl/curl.h>

#include <GNURegex.h>

#include <BESUtil.h>
#include <BESCatalogUtils.h>
#include <BESCatalogList.h>
#include <BESCatalog.h>
#include <BESRegex.h>
#include <TheBESKeys.h>
#include <BESInternalError.h>
#include <BESNotFoundError.h>
#include <BESSyntaxUserError.h>
#include <BESDebug.h>

#include "HttpNames.h"
#include "HttpUtils.h"

#define MODULE "http"

using namespace std;
using namespace http;

// These are static class members
map<string, string> HttpUtils::MimeList;
string HttpUtils::ProxyProtocol;
string HttpUtils::ProxyHost;
string HttpUtils::ProxyUser;
string HttpUtils::ProxyPassword;
string HttpUtils::ProxyUserPW;

int HttpUtils::ProxyPort = 0;
int HttpUtils::ProxyAuthType = 0;
bool HttpUtils::useInternalCache = false;

string HttpUtils::NoProxyRegex;

#define prolog string("HttpUtils::").append(__func__).append("() - ")

// Initialization routine for the httpd_catalog_HTTPD_CATALOG for certain parameters
// and keys, like the white list, the MimeTypes translation.
void HttpUtils::Initialize()
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
            MimeList[mod] = mime;
        }
    }

    found = false;
    TheBESKeys::TheKeys()->get_value(HTTP_PROXYHOST_KEY, HttpUtils::ProxyHost, found);
    if (found && !HttpUtils::ProxyHost.empty()) {
        // if the proxy host is set, then check to see if the port is
        // set. Does not need to be.
        found = false;
        string port;
        TheBESKeys::TheKeys()->get_value(HTTP_PROXYPORT_KEY, port, found);
        if (found && !port.empty()) {
            HttpUtils::ProxyPort = atoi(port.c_str());
            if (!HttpUtils::ProxyPort) {
                string err = (string) "httpd catalog proxy host is specified, but specified port is absent";
                throw BESSyntaxUserError(err, __FILE__, __LINE__);
            }
        }

        // @TODO Either use this or remove it - right now this variable is never used downstream
        // find the protocol to use for the proxy server. If none set, default to http
        found = false;
        TheBESKeys::TheKeys()->get_value(HTTP_PROXYPROTOCOL_KEY, HttpUtils::ProxyProtocol, found);
        if (!found || HttpUtils::ProxyProtocol.empty()) {
            HttpUtils::ProxyProtocol = "http";
        }

        // find the user to use for authenticating with the proxy server. If none set,
        // default to ""
        found = false;
        TheBESKeys::TheKeys()->get_value(HTTP_PROXYUSER_KEY, HttpUtils::ProxyUser, found);
        if (!found) {
            HttpUtils::ProxyUser = "";
        }

        // find the password to use for authenticating with the proxy server. If none set,
        // default to ""
        found = false;
        TheBESKeys::TheKeys()->get_value(HTTP_PROXYPASSWORD_KEY, HttpUtils::ProxyPassword, found);
        if (!found) {
            HttpUtils::ProxyPassword = "";
        }

        // find the user:password string to use for authenticating with the proxy server. If none set,
        // default to ""
        found = false;
        TheBESKeys::TheKeys()->get_value(HTTP_PROXYUSERPW_KEY, HttpUtils::ProxyUserPW, found);
        if (!found) {
            HttpUtils::ProxyUserPW = "";
        }

        // find the authentication mechanism to use with the proxy server. If none set,
        // default to BASIC authentication.
        found = false;
        string authType;
        TheBESKeys::TheKeys()->get_value(HTTP_PROXYAUTHTYPE_KEY, authType, found);
        if (found) {
            authType = BESUtil::lowercase(authType);
            if (authType == "basic") {
                HttpUtils::ProxyAuthType = CURLAUTH_BASIC;
                BESDEBUG(MODULE, prolog << "ProxyAuthType BASIC set." << endl);
            } else if (authType == "digest") {
                HttpUtils::ProxyAuthType = CURLAUTH_DIGEST;
                BESDEBUG(MODULE, prolog << "ProxyAuthType DIGEST set." << endl);
            } else if (authType == "ntlm") {
                HttpUtils::ProxyAuthType = CURLAUTH_NTLM;
                BESDEBUG(MODULE, prolog << "ProxyAuthType NTLM set." << endl);
            } else {
                HttpUtils::ProxyAuthType = CURLAUTH_BASIC;
                BESDEBUG(MODULE,
                         prolog << "User supplied an invalid value '" << authType
                                << "'  for Gateway.ProxyAuthType. Falling back to BASIC authentication scheme."
                                << endl);
            }
        } else {
            HttpUtils::ProxyAuthType = CURLAUTH_BASIC;
        }
    }

    found = false;
    string use_cache;
    TheBESKeys::TheKeys()->get_value(HTTP_USE_INTERNAL_CACHE_KEY, use_cache, found);
    if (found) {
        if (use_cache == "true" || use_cache == "TRUE" || use_cache == "True" || use_cache == "yes" ||
            use_cache == "YES" || use_cache == "Yes")
            HttpUtils::useInternalCache = true;
        else
            HttpUtils::useInternalCache = false;
    } else {
        // If not set, default to false. Assume squid or ...
        HttpUtils::useInternalCache = false;
    }
    // Grab the value for the NoProxy regex; empty if there is none.
    found = false; // Not used
    TheBESKeys::TheKeys()->get_value("Http.NoProxy", HttpUtils::NoProxyRegex, found);
}

// Not used. There's a better version of this that returns a string in libdap.
// jhrg 3/24/11

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
void HttpUtils::Get_type_from_disposition(const string &disp, string &type)
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

            // now see if it's wrapped in quotes
            if (filename[0] == '"') {
                filename = filename.substr(1);
            }
            if (filename[filename.length() - 1] == '"') {
                filename = filename.substr(0, filename.length() - 1);
            }

            // we have the filename now, run it through
            // the type match to get the file type.

            const BESCatalogUtils *utils = BESCatalogList::TheCatalogList()->default_catalog()->get_catalog_utils();
            type = utils->get_handler_name(filename);
        }
    }
}

void HttpUtils::Get_type_from_content_type(const string &ctype, string &type)
{
    BESDEBUG(MODULE, prolog << "BEGIN content-type: " << ctype << endl);
    map<string, string>::iterator i = MimeList.begin();
    map<string, string>::iterator e = MimeList.end();
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

void HttpUtils::Get_type_from_url(const string &url, string &type) {
    const BESCatalogUtils *utils = BESCatalogList::TheCatalogList()->find_catalog("catalog")->get_catalog_utils();

    type = utils->get_handler_name(url);
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



