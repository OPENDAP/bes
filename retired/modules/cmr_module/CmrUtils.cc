// CmrUtils.cc

// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of gateway_module, A C++ module that can be loaded in to
// the OPeNDAP Back-End Server (BES) and is able to handle remote requests.

// Copyright (c) 2002,2003 OPeNDAP, Inc.
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

#include "CmrNames.h"
#include "CmrUtils.h"
#include "CmrApi.h"

<<<<<<< HEAD
#include <GNURegex.h>
#include <libdap/util.h>
=======
#include "BESRegex.h"
#include <util.h>
>>>>>>> master

using namespace libdap;
using namespace cmr;
using std::vector;

std::map<string, string> CmrUtils::MimeList;
string CmrUtils::ProxyProtocol;
string CmrUtils::ProxyHost;
string CmrUtils::ProxyUser;
string CmrUtils::ProxyPassword;
string CmrUtils::ProxyUserPW;
int CmrUtils::ProxyPort = 0;
int CmrUtils::ProxyAuthType = 0;
bool CmrUtils::useInternalCache = false;

string CmrUtils::NoProxyRegex;

#define prolog std::string("CmrUtils::").append(__func__).append("() - ")


// Initialization routine for the gateway module for certain parameters
// and keys, like the white list, the MimeTypes translation.
void CmrUtils::Initialize()
{
    // MimeTypes - translate from a mime type to a module name
    bool found = false;
    std::string key = CMR_MIMELIST;
    std::vector<string> vals;
    TheBESKeys::TheKeys()->get_values(key, vals, found);
    if (found && vals.size()) {
        std::vector<string>::iterator i = vals.begin();
        std::vector<string>::iterator e = vals.end();
        for (; i != e; i++) {
            size_t colon = (*i).find(":");
            if (colon == string::npos) {
                string err = (string) "Malformed " + CMR_MIMELIST + " " + (*i)
                    + " specified in the gateway configuration";
                throw BESSyntaxUserError(err, __FILE__, __LINE__);
            }
            string mod = (*i).substr(0, colon);
            string mime = (*i).substr(colon + 1);
            MimeList[mod] = mime;
        }
    }

    found = false;
    key = CMR_PROXYHOST;
    TheBESKeys::TheKeys()->get_value(key, CmrUtils::ProxyHost, found);
    if (found && !CmrUtils::ProxyHost.empty()) {
        // if the proxy host is set, then check to see if the port is
        // set. Does not need to be.
        found = false;
        key = CMR_PROXYPORT;
        string port;
        TheBESKeys::TheKeys()->get_value(key, port, found);
        if (found && !port.empty()) {
            CmrUtils::ProxyPort = atoi(port.c_str());
            if (!CmrUtils::ProxyPort) {
                string err = (string) "CMR proxy host is specified, but specified port is absent";
                throw BESSyntaxUserError(err, __FILE__, __LINE__);
            }
        }

        // @TODO Either use this or remove it - right now this variable is never used downstream
        // find the protocol to use for the proxy server. If none set,
        // default to http
        found = false;
        key = CMR_PROXYPROTOCOL;
        TheBESKeys::TheKeys()->get_value(key, CmrUtils::ProxyProtocol, found);
        if (!found || CmrUtils::ProxyProtocol.empty()) {
            CmrUtils::ProxyProtocol = "http";
        }

        // find the user to use for authenticating with the proxy server. If none set,
        // default to ""
        found = false;
        key = CMR_PROXYUSER;
        TheBESKeys::TheKeys()->get_value(key, CmrUtils::ProxyUser, found);
        if (!found) {
            CmrUtils::ProxyUser = "";
        }

        // find the password to use for authenticating with the proxy server. If none set,
        // default to ""
        found = false;
        key = CMR_PROXYPASSWORD;
        TheBESKeys::TheKeys()->get_value(key, CmrUtils::ProxyPassword, found);
        if (!found) {
            CmrUtils::ProxyPassword = "";
        }

        // find the user:password string to use for authenticating with the proxy server. If none set,
        // default to ""
        found = false;
        key = CMR_PROXYUSERPW;
        TheBESKeys::TheKeys()->get_value(key, CmrUtils::ProxyUserPW, found);
        if (!found) {
            CmrUtils::ProxyUserPW = "";
        }

        // find the authentication mechanism to use with the proxy server. If none set,
        // default to BASIC authentication.
        found = false;
        key = CMR_PROXYAUTHTYPE;
        string authType;
        TheBESKeys::TheKeys()->get_value(key, authType, found);
        if (found) {
            authType = BESUtil::lowercase(authType);
            if (authType == "basic") {
                CmrUtils::ProxyAuthType = CURLAUTH_BASIC;
                BESDEBUG(MODULE, prolog << "ProxyAuthType BASIC set." << endl);
            }
            else if (authType == "digest") {
                CmrUtils::ProxyAuthType = CURLAUTH_DIGEST;
                BESDEBUG(MODULE, prolog << "ProxyAuthType DIGEST set." << endl);
            }

            else if (authType == "ntlm") {
                CmrUtils::ProxyAuthType = CURLAUTH_NTLM;
                BESDEBUG(MODULE, prolog << "ProxyAuthType NTLM set." << endl);
            }
            else {
                CmrUtils::ProxyAuthType = CURLAUTH_BASIC;
                BESDEBUG(MODULE,
                        prolog << "User supplied an invalid value '"<< authType << "'  for Gateway.ProxyAuthType. Falling back to BASIC authentication scheme." << endl);
            }
        }
        else {
            CmrUtils::ProxyAuthType = CURLAUTH_BASIC;
        }
    }

    found = false;
    key = CMR_USE_INTERNAL_CACHE;
    string use_cache;
    TheBESKeys::TheKeys()->get_value(key, use_cache, found);
    if (found) {
        if (use_cache == "true" || use_cache == "TRUE" || use_cache == "True" || use_cache == "yes"
            || use_cache == "YES" || use_cache == "Yes")
            CmrUtils::useInternalCache = true;
        else
            CmrUtils::useInternalCache = false;
    }
    else {
        // If not set, default to false. Assume squid or ...
        CmrUtils::useInternalCache = false;
    }
    // Grab the value for the NoProxy regex; empty if there is none.
    found = false; // Not used
    TheBESKeys::TheKeys()->get_value("Gateway.NoProxy", CmrUtils::NoProxyRegex, found);
}



void CmrUtils::Get_type_from_disposition(const string &disp, string &type)
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

#if 0
            const BESCatalogUtils *utils = BESCatalogUtils::Utils("catalog");
            BESCatalogUtils::match_citer i = utils->match_list_begin();
            BESCatalogUtils::match_citer ie = utils->match_list_end();
            bool done = false;
            for (; i != ie && !done; i++) {
                BESCatalogUtils::handler_regex match = (*i);
                try {
                    BESDEBUG(MODULE,
                            prolog << "Comparing disp filename " << filename << " against expr " << match.regex << endl);
                    BESRegex reg_expr(match.regex.c_str());
                    if (reg_expr.match(filename.c_str(), filename.size()) == static_cast<int>(filename.size())) {
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
#endif
         }
    }
}

void CmrUtils::Get_type_from_content_type(const string &ctype, string &type)
{
    BESDEBUG(MODULE, prolog << "BEGIN" << endl);
    std::map<string, string>::iterator i = MimeList.begin();
    std::map<string, string>::iterator e = MimeList.end();
    bool done = false;
    for (; i != e && !done; i++) {
        BESDEBUG(MODULE,
                prolog << "Comparing content type '" << ctype << "' against mime list element '" << (*i).second << "'"<< endl);
        BESDEBUG(MODULE,
                prolog << "first: " << (*i).first << "  second: " << (*i).second << endl);

        if ((*i).second == ctype) {

            BESDEBUG(MODULE, prolog << "MATCH" << endl);

            type = (*i).first;
            done = true;
        }
    }
    BESDEBUG(MODULE, "GatewayUtils::Get_type_from_content_type() - END" << endl);
}

void CmrUtils::Get_type_from_url(const string &url, string &type)
{
    // Just run the url through the type match from the configuration

    const BESCatalogUtils *utils = BESCatalogList::TheCatalogList()->find_catalog(CMR_CATALOG_NAME)->get_catalog_utils();

    type = utils->get_handler_name(url);

#if 0

    BESCatalogUtils::match_citer i = utils->match_list_begin();
    BESCatalogUtils::match_citer ie = utils->match_list_end();
    bool done = false;
    for (; i != ie && !done; i++) {
        BESCatalogUtils::handler_regex match = (*i);
        try {
            BESDEBUG(MODULE,
                    prolog << "Comparing url " << url << " against type match expr " << match.regex << endl);
            BESRegex reg_expr(match.regex.c_str());
            if (reg_expr.match(url.c_str(), url.size()) == static_cast<int>(url.size())) {
                type = match.handler;
                done = true;
                BESDEBUG(MODULE, prolog << "MATCH   type: " << type << endl);
            }
        }
        catch (Error &e) {
            string serr = (string) "Unable to match data type! Malformed Catalog TypeMatch parameter "
                + "in bes configuration file around " + match.regex + ": " + e.get_error_message();
            throw BESInternalError(serr, __FILE__, __LINE__);
        }
    }
#endif

}

#if 0
bool GatewayUtils::Is_Whitelisted(const std::string &url){
    bool whitelisted = false;
    std::vector<std::string>::const_iterator i = WhiteList.begin();
    std::vector<std::string>::const_iterator e = WhiteList.end();
    for (; i != e && !whitelisted; i++) {
        if ((*i).size() <= url.size()) {
            if (url.substr(0, (*i).size()) == (*i)) {
                whitelisted = true;
            }
        }
    }
    return whitelisted;
}

#endif

Granule *
CmrUtils::getTemporalFacetGranule(const std::string granule_path)
{

    BESDEBUG(MODULE, prolog << "BEGIN  (granule_path: '" << granule_path  << ")" << endl);

    string collection;
    string facet = "temporal";
    string year = "-";
    string month = "-";
    string day = "-";
    string granule_id = "-";

    string path = BESUtil::normalize_path(granule_path,false, false);
    vector<string> path_elements = BESUtil::split(path);
    BESDEBUG(MODULE, prolog << "path: '" << path << "'   path_elements.size(): " << path_elements.size() << endl);

    switch(path_elements.size()){
    case 6:
    {
        collection = path_elements[0];
        BESDEBUG(MODULE, prolog << "collection: '" << collection << endl);
        facet = path_elements[1];
        BESDEBUG(MODULE, prolog << "facet: '" << facet << endl);
        year = path_elements[2];
        BESDEBUG(MODULE, prolog << "year: '" << year << endl);
        month = path_elements[3];
        BESDEBUG(MODULE, prolog << "month: '" << month << endl);
        day = path_elements[4];
        BESDEBUG(MODULE, prolog << "day: '" << day << endl);
        granule_id = path_elements[5];
        BESDEBUG(MODULE, prolog << "granule_id: '" << granule_id << endl);
}
    break;
    default:
    {
        throw BESNotFoundError("Can't find it man...",__FILE__,__LINE__);
    }
    break;
    }
    CmrApi cmrApi;

    return cmrApi.get_granule( collection, year, month, day, granule_id);
}



