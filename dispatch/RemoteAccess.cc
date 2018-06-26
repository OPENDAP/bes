// RemoteAccess.cc

// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of the OPeNDAP Back-End Server (BES)
// and embodies a whitelist of remote system that may be
// accessed by the server as part of it's routine operation.

// Copyright (c) 2018 OPeNDAP, Inc.
// Author:Nathan D. Potter <ndp@opendap.org>
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

//#ifdef HAVE_UNISTD_H
//#include <unistd.h>
//#endif
//#include <cstdlib>
//#include <cstring>
//#include <curl/curl.h>

#include "RemoteAccess.h"

#include <BESUtil.h>
#include <BESCatalog.h>
#include <BESCatalogList.h>
#include <BESCatalogUtils.h>
#include <BESRegex.h>
#include <TheBESKeys.h>
#include <BESInternalError.h>
#include <BESSyntaxUserError.h>
#include <BESDebug.h>
#include <BESForbiddenError.h>

// #include <util.h>

using namespace bes;


std::vector<string> RemoteAccess::WhiteList;
bool RemoteAccess::is_init = false;

// Initialization routine for the RemoteAccess white list
void RemoteAccess::init()
{
    if(is_init)
        return;

    // Whitelist - gather list of domains that the
    // hyrax is allowed to communicate with.
    bool found = false;
    string key = REMOTE_ACCESS_WHITELIST;
    TheBESKeys::TheKeys()->get_values(key, WhiteList, found);

    is_init = true;

#if 0
    if (!found || WhiteList.size() == 0) {
        string err = (string) "The parameter " + REMOTE_ACCESS_WHITELIST + " is not set or has no values in the gateway"
            + " configuration file";
        throw BESSyntaxUserError(err, __FILE__, __LINE__);
    }
#endif


}


/**
 * This method provides an access condition assessment for URLs and files
 * to be accessed by the BES. The http and https urls are verified against a
 * whitelist assembled from configuration. All file urls are checked to be
 * sure that they reference a resource within the BES catalog.
 */
bool RemoteAccess::Is_Whitelisted(const std::string &url){

    if(!is_init)
        init();

    bool whitelisted = false;
    string file_url( "file://");
    string http_url( "http://");
    string https_url("https://");

    if (url.compare(0, file_url.size(), file_url) == 0 /*equals a file url*/) {

        // Ensure that the file path starts with the catalog root dir.
        string file_path = url.substr(file_url.size());

        BESCatalog *bcat = BESCatalogList::TheCatalogList()->find_catalog(BES_DEFAULT_CATALOG);
        string catalog_root = bcat->get_root();
        // BESDEBUG("bes","Catalog root: "<< root << endl);

        whitelisted = file_path.compare(0, catalog_root.size(), catalog_root) == 0;
        // BESDEBUG("bes","Is_Whitelisted: "<< (whitelisted?"true":"false") << endl);
    }
    else if (url.compare(0, http_url.size(),  http_url)  == 0  /*equals http url */   ||
             url.compare(0, https_url.size(), https_url) == 0  /*equals https url */ ) {

        std::vector<std::string>::const_iterator i = WhiteList.begin();
        std::vector<std::string>::const_iterator e = WhiteList.end();
        for (; i != e && !whitelisted; i++) {
            if ((*i).length() <= url.length()) {
                if (url.substr(0, (*i).length()) == (*i)) {
                    whitelisted = true;
                }
            }
        }
    }
    else {
        string msg;
        msg = "ERROR! Unknown URL protocol! Only "+http_url+", "+https_url+", and "+file_url+" are supported.";
        BESDEBUG("bes", msg << endl);
        throw BESForbiddenError(msg ,__FILE__,__LINE__);
    }
    return whitelisted;
}





