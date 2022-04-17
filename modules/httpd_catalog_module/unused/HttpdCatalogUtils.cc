// HttpCatalogUtils.cc
// -*- mode: c++; c-basic-offset:4 -*-
//
// This file is part of httpd_catalog_module, A C++ module that can be loaded in to
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

#include "config.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <cstdlib>
#include <cstring>
#include <map>
#include <vector>

#include <curl/curl.h>

<<<<<<< HEAD
#include <GNURegex.h>
#include <libdap/util.h>
=======
#include "BESRegex.h"
#include <util.h>
>>>>>>> master

#include <BESUtil.h>
#include <BESCatalogUtils.h>
#include <BESCatalogList.h>
#include <BESCatalog.h>
#include <BESRegex.h>
#include <TheBESKeys.h>
#include <BESInternalError.h>
#include <BESDapError.h>
#include <BESNotFoundError.h>
#include <BESSyntaxUserError.h>
#include <BESDebug.h>

#include "HttpdCatalogNames.h"
#include "HttpdCatalogUtils.h"

using namespace libdap;
using namespace std;

namespace httpd_catalog {

// These are static class members
map<string, string> HttpdCatalogUtils::MimeList;
string HttpdCatalogUtils::ProxyProtocol;
string HttpdCatalogUtils::ProxyHost;
string HttpdCatalogUtils::ProxyUser;
string HttpdCatalogUtils::ProxyPassword;
string HttpdCatalogUtils::ProxyUserPW;

int HttpdCatalogUtils::ProxyPort = 0;
int HttpdCatalogUtils::ProxyAuthType = 0;
bool HttpdCatalogUtils::useInternalCache = false;

string HttpdCatalogUtils::NoProxyRegex;

#define prolog string("HttpCatalogUtils::").append(__func__).append("() - ")

// Initialization routine for the httpd_catalog_module for certain parameters
// and keys, like the white list, the MimeTypes translation.
void HttpdCatalogUtils::initialize()
{
    // MimeTypes - translate from a mime type to a module name
    bool found = false;
    string key = HTTPD_CATALOG_MIMELIST;
    vector<string> vals;
    TheBESKeys::TheKeys()->get_values(key, vals, found);
    if (found && vals.size()) {
        vector<string>::iterator i = vals.begin();
        vector<string>::iterator e = vals.end();
        for (; i != e; i++) {
            size_t colon = (*i).find(":");
            if (colon == string::npos) {
                string err = (string) "Malformed " + HTTPD_CATALOG_MIMELIST + " " + (*i) + " specified in the gateway configuration";
                throw BESSyntaxUserError(err, __FILE__, __LINE__);
            }
            string mod = (*i).substr(0, colon);
            string mime = (*i).substr(colon + 1);
            MimeList[mod] = mime;
        }
    }

    found = false;
    key = HTTPD_CATALOG_PROXYHOST;
    TheBESKeys::TheKeys()->get_value(key, HttpdCatalogUtils::ProxyHost, found);
    if (found && !HttpdCatalogUtils::ProxyHost.empty()) {
        // if the proxy host is set, then check to see if the port is
        // set. Does not need to be.
        found = false;
        key = HTTPD_CATALOG_PROXYPORT;
        string port;
        TheBESKeys::TheKeys()->get_value(key, port, found);
        if (found && !port.empty()) {
            HttpdCatalogUtils::ProxyPort = atoi(port.c_str());
            if (!HttpdCatalogUtils::ProxyPort) {
                string err = (string) "httpd catalog proxy host is specified, but specified port is absent";
                throw BESSyntaxUserError(err, __FILE__, __LINE__);
            }
        }

        // @TODO Either use this or remove it - right now this variable is never used downstream
        // find the protocol to use for the proxy server. If none set, default to http
        found = false;
        key = HTTPD_CATALOG_PROXYPROTOCOL;
        TheBESKeys::TheKeys()->get_value(key, HttpdCatalogUtils::ProxyProtocol, found);
        if (!found || HttpdCatalogUtils::ProxyProtocol.empty()) {
            HttpdCatalogUtils::ProxyProtocol = "http";
        }

        // find the user to use for authenticating with the proxy server. If none set,
        // default to ""
        found = false;
        key = HTTPD_CATALOG_PROXYUSER;
        TheBESKeys::TheKeys()->get_value(key, HttpdCatalogUtils::ProxyUser, found);
        if (!found) {
            HttpdCatalogUtils::ProxyUser = "";
        }

        // find the password to use for authenticating with the proxy server. If none set,
        // default to ""
        found = false;
        key = HTTPD_CATALOG_PROXYPASSWORD;
        TheBESKeys::TheKeys()->get_value(key, HttpdCatalogUtils::ProxyPassword, found);
        if (!found) {
            HttpdCatalogUtils::ProxyPassword = "";
        }

        // find the user:password string to use for authenticating with the proxy server. If none set,
        // default to ""
        found = false;
        key = HTTPD_CATALOG_PROXYUSERPW;
        TheBESKeys::TheKeys()->get_value(key, HttpdCatalogUtils::ProxyUserPW, found);
        if (!found) {
            HttpdCatalogUtils::ProxyUserPW = "";
        }

        // find the authentication mechanism to use with the proxy server. If none set,
        // default to BASIC authentication.
        found = false;
        key = HTTPD_CATALOG_PROXYAUTHTYPE;
        string authType;
        TheBESKeys::TheKeys()->get_value(key, authType, found);
        if (found) {
            authType = BESUtil::lowercase(authType);
            if (authType == "basic") {
                HttpdCatalogUtils::ProxyAuthType = CURLAUTH_BASIC;
                BESDEBUG(MODULE, prolog << "ProxyAuthType BASIC set." << endl);
            }
            else if (authType == "digest") {
                HttpdCatalogUtils::ProxyAuthType = CURLAUTH_DIGEST;
                BESDEBUG(MODULE, prolog << "ProxyAuthType DIGEST set." << endl);
            }

            else if (authType == "ntlm") {
                HttpdCatalogUtils::ProxyAuthType = CURLAUTH_NTLM;
                BESDEBUG(MODULE, prolog << "ProxyAuthType NTLM set." << endl);
            }
            else {
                HttpdCatalogUtils::ProxyAuthType = CURLAUTH_BASIC;
                BESDEBUG(MODULE,
                    prolog << "User supplied an invalid value '"<< authType << "'  for Gateway.ProxyAuthType. Falling back to BASIC authentication scheme." << endl);
            }
        }
        else {
            HttpdCatalogUtils::ProxyAuthType = CURLAUTH_BASIC;
        }
    }

    found = false;
    key = HTTPD_CATALOG_USE_INTERNAL_CACHE;
    string use_cache;
    TheBESKeys::TheKeys()->get_value(key, use_cache, found);
    if (found) {
        if (use_cache == "true" || use_cache == "TRUE" || use_cache == "True" || use_cache == "yes" || use_cache == "YES" || use_cache == "Yes")
            HttpdCatalogUtils::useInternalCache = true;
        else
            HttpdCatalogUtils::useInternalCache = false;
    }
    else {
        // If not set, default to false. Assume squid or ...
        HttpdCatalogUtils::useInternalCache = false;
    }
    // Grab the value for the NoProxy regex; empty if there is none.
    found = false; // Not used
    TheBESKeys::TheKeys()->get_value("Gateway.NoProxy", HttpdCatalogUtils::NoProxyRegex, found);
}

void HttpdCatalogUtils::get_type_from_disposition(const string &disp, string &type)
{
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
            }
            else {
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

void HttpdCatalogUtils::get_type_from_content_type(const string &ctype, string &type)
{
    BESDEBUG(MODULE, prolog << "BEGIN" << endl);

    map<string, string>::iterator i = MimeList.begin();
    map<string, string>::iterator e = MimeList.end();
    bool done = false;
    for (; i != e && !done; i++) {
        BESDEBUG(MODULE, prolog << "Comparing content type '" << ctype << "' against mime list element '" << (*i).second << "'"<< endl);
        BESDEBUG(MODULE, prolog << "first: " << (*i).first << "  second: " << (*i).second << endl);

        if ((*i).second == ctype) {

            BESDEBUG(MODULE, prolog << "MATCH" << endl);

            type = (*i).first;
            done = true;
        }
    }

    BESDEBUG(MODULE, prolog << "END" << endl);
}

void HttpdCatalogUtils::get_type_from_url(const string &url, string &type)
{
    const BESCatalogUtils *utils = BESCatalogList::TheCatalogList()->find_catalog("catalog")->get_catalog_utils();

    type = utils->get_handler_name(url);
}

} // namespace httpd_catalog

