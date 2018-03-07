// GatewayUtils.cc

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

#include "config.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <cstdlib>
#include <cstring>
#include <curl/curl.h>

#include "GatewayUtils.h"
#include "GatewayResponseNames.h"

#include <BESUtil.h>
#include <BESCatalogUtils.h>
#include <BESRegex.h>
#include <TheBESKeys.h>
#include <BESInternalError.h>
#include <BESDapError.h>
#include <BESSyntaxUserError.h>
#include <BESDebug.h>

#include <GNURegex.h>
#include <util.h>

using namespace libdap;
using namespace gateway;

vector<string> GatewayUtils::WhiteList;
map<string, string> GatewayUtils::MimeList;
string GatewayUtils::ProxyProtocol;
string GatewayUtils::ProxyHost;
string GatewayUtils::ProxyUser;
string GatewayUtils::ProxyPassword;
string GatewayUtils::ProxyUserPW;
int GatewayUtils::ProxyPort = 0;
int GatewayUtils::ProxyAuthType = 0;
bool GatewayUtils::useInternalCache = false;

string GatewayUtils::NoProxyRegex;

// Initialization routine for the gateway module for certain parameters
// and keys, like the white list, the MimeTypes translation.
void GatewayUtils::Initialize()
{
    // Whitelist - list of domain that the gateway is allowed to
    // communicate with.
    bool found = false;
    string key = Gateway_WHITELIST;
    TheBESKeys::TheKeys()->get_values(key, WhiteList, found);
    if (!found || WhiteList.size() == 0) {
        string err = (string) "The parameter " + Gateway_WHITELIST + " is not set or has no values in the gateway"
            + " configuration file";
        throw BESSyntaxUserError(err, __FILE__, __LINE__);

    }

    // MimeTypes - translate from a mime type to a module name
    found = false;
    key = Gateway_MIMELIST;
    vector<string> vals;
    TheBESKeys::TheKeys()->get_values(key, vals, found);
    if (found && vals.size()) {
        vector<string>::iterator i = vals.begin();
        vector<string>::iterator e = vals.end();
        for (; i != e; i++) {
            size_t colon = (*i).find(":");
            if (colon == string::npos) {
                string err = (string) "Malformed " + Gateway_MIMELIST + " " + (*i)
                    + " specified in the gateway configuration";
                throw BESSyntaxUserError(err, __FILE__, __LINE__);
            }
            string mod = (*i).substr(0, colon);
            string mime = (*i).substr(colon + 1);
            MimeList[mod] = mime;
        }
    }

    found = false;
    key = Gateway_PROXYHOST;
    TheBESKeys::TheKeys()->get_value(key, GatewayUtils::ProxyHost, found);
    if (found && !GatewayUtils::ProxyHost.empty()) {
        // if the proxy host is set, then check to see if the port is
        // set. Does not need to be.
        found = false;
        key = Gateway_PROXYPORT;
        string port;
        TheBESKeys::TheKeys()->get_value(key, port, found);
        if (found && !port.empty()) {
            GatewayUtils::ProxyPort = atoi(port.c_str());
            if (!GatewayUtils::ProxyPort) {
                string err = (string) "gateway proxy host specified," + " but proxy port specified is invalid";
                throw BESSyntaxUserError(err, __FILE__, __LINE__);
            }
        }

        // @TODO Either use this or remove it - right now this variable is never used downstream
        // find the protocol to use for the proxy server. If none set,
        // default to http
        found = false;
        key = Gateway_PROXYPROTOCOL;
        TheBESKeys::TheKeys()->get_value(key, GatewayUtils::ProxyProtocol, found);
        if (!found || GatewayUtils::ProxyProtocol.empty()) {
            GatewayUtils::ProxyProtocol = "http";
        }

        // find the user to use for authenticating with the proxy server. If none set,
        // default to ""
        found = false;
        key = Gateway_PROXYUSER;
        TheBESKeys::TheKeys()->get_value(key, GatewayUtils::ProxyUser, found);
        if (!found) {
            GatewayUtils::ProxyUser = "";
        }

        // find the password to use for authenticating with the proxy server. If none set,
        // default to ""
        found = false;
        key = Gateway_PROXYPASSWORD;
        TheBESKeys::TheKeys()->get_value(key, GatewayUtils::ProxyPassword, found);
        if (!found) {
            GatewayUtils::ProxyPassword = "";
        }

        // find the user:password string to use for authenticating with the proxy server. If none set,
        // default to ""
        found = false;
        key = Gateway_PROXYUSERPW;
        TheBESKeys::TheKeys()->get_value(key, GatewayUtils::ProxyUserPW, found);
        if (!found) {
            GatewayUtils::ProxyUserPW = "";
        }

        // find the authentication mechanism to use with the proxy server. If none set,
        // default to BASIC authentication.
        found = false;
        key = Gateway_PROXYAUTHTYPE;
        string authType;
        TheBESKeys::TheKeys()->get_value(key, authType, found);
        if (found) {
            authType = BESUtil::lowercase(authType);
            if (authType == "basic") {
                GatewayUtils::ProxyAuthType = CURLAUTH_BASIC;
                BESDEBUG("gateway", "GatewayUtils::Initialize() - ProxyAuthType BASIC set." << endl);
            }
            else if (authType == "digest") {
                GatewayUtils::ProxyAuthType = CURLAUTH_DIGEST;
                BESDEBUG("gateway", "GatewayUtils::Initialize() - ProxyAuthType DIGEST set." << endl);
            }

            else if (authType == "ntlm") {
                GatewayUtils::ProxyAuthType = CURLAUTH_NTLM;
                BESDEBUG("gateway", "GatewayUtils::Initialize() - ProxyAuthType NTLM set." << endl);
            }
            else {
                GatewayUtils::ProxyAuthType = CURLAUTH_BASIC;
                BESDEBUG("gateway",
                    "GatewayUtils::Initialize() - User supplied an invalid value '"<< authType << "'  for Gateway.ProxyAuthType. Falling back to BASIC authentication scheme." << endl);
            }

        }
        else {
            GatewayUtils::ProxyAuthType = CURLAUTH_BASIC;
        }

    }

    found = false;
    key = Gateway_USE_INTERNAL_CACHE;
    string use_cache;
    TheBESKeys::TheKeys()->get_value(key, use_cache, found);
    if (found) {
        if (use_cache == "true" || use_cache == "TRUE" || use_cache == "True" || use_cache == "yes"
            || use_cache == "YES" || use_cache == "Yes")
            GatewayUtils::useInternalCache = true;
        else
            GatewayUtils::useInternalCache = false;
    }
    else {
        // If not set, default to false. Assume squid or ...
        GatewayUtils::useInternalCache = false;
    }

    // Grab the value for the NoProxy regex; empty if there is none.
    found = false; // Not used
    TheBESKeys::TheKeys()->get_value("Gateway.NoProxy", GatewayUtils::NoProxyRegex, found);
}

// Not used. There's a better version of this that returns a string in libdap.
// jhrg 3/24/11

#if 0
// Look around for a reasonable place to put a temporary file. Check first
// the value of the TMPDIR env var. If that does not yield a path that's
// writable (as defined by access(..., W_OK|R_OK)) then look at P_tmpdir (as
// defined in stdio.h. If both come up empty, then use `./'.
//
// This function allocates storage using new. The caller must delete the char
// array.

// Change this to a version that either returns a string or an open file
// descriptor. Use information from https://buildsecurityin.us-cert.gov/
// (see open()) to make it more secure. Ideal solution: get deserialize()
// methods to read from a stream returned by libcurl, not from a temporary
// file. 9/21/07 jhrg
char *
GatewayUtils::Get_tempfile_template( char *file_template )
{
#ifdef WIN32
    // white list for a WIN32 directory
    Regex directory("[-a-zA-Z0-9_\\]*");

    string c = getenv("TEMP") ? getenv("TEMP") : "";
    if (!c.empty() && directory.match(c.c_str(), c.length()) && (access(c.c_str(), 6) == 0))
    goto valid_temp_directory;

    c = getenv("TMP") ? getenv("TMP") : "";
    if (!c.empty() && directory.match(c.c_str(), c.length()) && (access(c.c_str(), 6) == 0))
    goto valid_temp_directory;
#else
    // white list for a directory
    Regex directory("[-a-zA-Z0-9_/]*");

    string c = getenv("TMPDIR") ? getenv("TMPDIR") : "";
    if (!c.empty() && directory.match(c.c_str(), c.length())
        && (access(c.c_str(), W_OK | R_OK) == 0))
    goto valid_temp_directory;

#ifdef P_tmpdir
    if (access(P_tmpdir, W_OK | R_OK) == 0) {
        c = P_tmpdir;
        goto valid_temp_directory;
    }
#endif

#endif  // WIN32
    c = ".";

    valid_temp_directory:

#ifdef WIN32
    c.append("\\");
#else
    c.append("/");
#endif
    c.append(file_template);

    char *temp = new char[c.length() + 1];
    strncpy(temp, c.c_str(), c.length());
    temp[c.length()] = '\0';

    return temp;
}
#endif

void GatewayUtils::Get_type_from_disposition(const string &disp, string &type)
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

            // now see if it's wrapped in quotes
            if (filename[0] == '"') {
                filename = filename.substr(1);
            }
            if (filename[filename.length() - 1] == '"') {
                filename = filename.substr(0, filename.length() - 1);
            }

            // we have the filename now, run it through
            // the type match to get the file type
            const BESCatalogUtils *utils = BESCatalogUtils::Utils("catalog");
            BESCatalogUtils::match_citer i = utils->match_list_begin();
            BESCatalogUtils::match_citer ie = utils->match_list_end();
            bool done = false;
            for (; i != ie && !done; i++) {
                BESCatalogUtils::handler_regex match = (*i);
                try {
                    BESDEBUG("gateway",
                        "  Comparing disp filename " << filename << " against expr " << match.regex << endl);
                    BESRegex reg_expr(match.regex.c_str());
                    if (reg_expr.match(filename.c_str(), filename.length()) == static_cast<int>(filename.length())) {
                        type = match.handler;
                        done = true;
                    }
                }
                catch (Error &e) {
                    string serr = (string) "Unable to match data type, " + "malformed Catalog TypeMatch parameter "
                        + "in bes configuration file around " + match.regex + ": " + e.get_error_message();
                    throw BESDapError(serr, false, e.get_error_code(), __FILE__, __LINE__);
                }
            }
        }
    }
}

void GatewayUtils::Get_type_from_content_type(const string &ctype, string &type)
{
    BESDEBUG("gateway", "GatewayUtils::Get_type_from_content_type() - BEGIN" << endl);
    map<string, string>::iterator i = MimeList.begin();
    map<string, string>::iterator e = MimeList.end();
    bool done = false;
    for (; i != e && !done; i++) {
        BESDEBUG("gateway",
            "GatewayUtils::Get_type_from_content_type() - Comparing content type '" << ctype << "' against mime list element '" << (*i).second << "'"<< endl);
        BESDEBUG("gateway",
            "GatewayUtils::Get_type_from_content_type() - first: " << (*i).first << "  second: " << (*i).second << endl);

        if ((*i).second == ctype) {

            BESDEBUG("gateway", "GatewayUtils::Get_type_from_content_type() - MATCH" << endl);

            type = (*i).first;
            done = true;
        }
    }
    BESDEBUG("gateway", "GatewayUtils::Get_type_from_content_type() - END" << endl);
}

void GatewayUtils::Get_type_from_url(const string &url, string &type)
{
    // just run the url through the type match from the configuration
    const BESCatalogUtils *utils = BESCatalogUtils::Utils("catalog");
    BESCatalogUtils::match_citer i = utils->match_list_begin();
    BESCatalogUtils::match_citer ie = utils->match_list_end();
    bool done = false;
    for (; i != ie && !done; i++) {
        BESCatalogUtils::handler_regex match = (*i);
        try {
            BESDEBUG("gateway",
                "GatewayUtils::Get_type_from_url() - Comparing url " << url << " against type match expr " << match.regex << endl);
            BESRegex reg_expr(match.regex.c_str());
            if (reg_expr.match(url.c_str(), url.length()) == static_cast<int>(url.length())) {
                type = match.handler;
                done = true;
                BESDEBUG("gateway", "GatewayUtils::Get_type_from_url() - MATCH   type: " << type << endl);
            }
        }
        catch (Error &e) {
            string serr = (string) "Unable to match data type, " + "malformed Catalog TypeMatch parameter "
                + "in bes configuration file around " + match.regex + ": " + e.get_error_message();
            throw BESInternalError(serr, __FILE__, __LINE__);
        }
    }
}
