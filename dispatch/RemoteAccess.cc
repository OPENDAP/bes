// RemoteAccess.cc

// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of the OPeNDAP Back-End Server (BES)
// and embodies a whitelist of remote system that may be
// accessed by the server as part of it's routine operation.

// Copyright (c) 2018 OPeNDAP, Inc.
// Author: Nathan D. Potter <ndp@opendap.org>
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

#include "RemoteAccess.h"

using namespace std;
using namespace bes;

std::vector<string> RemoteAccess::WhiteList;
bool RemoteAccess::is_init = false;

// Initialization routine for the RemoteAccess white list
void RemoteAccess::init()
{
    if (is_init) return;

    // Whitelist - gather list of domains that
    // hyrax is allowed to communicate with.
    bool found = false;
    string key = REMOTE_ACCESS_WHITELIST;
    TheBESKeys::TheKeys()->get_values(REMOTE_ACCESS_WHITELIST, WhiteList, found);

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
 * to be accessed by the BES. The http and https URLs are verified against a
 * whitelist assembled from configuration. All file URLs are checked to be
 * sure that they reference a resource within the BES default catalog.
 *
 * @note RemoteAccess is a singleton. This method will instantiate the class
 * if that has not already been done. This method should only be called from
 * the main thread of a multi-threaded application.
 *
 * @param url The URL to test
 * @return True if the URL may be dereferenced, given the BES's configuration,
 * false otherwise.
 */
bool RemoteAccess::Is_Whitelisted(const std::string &url)
{
    if (!is_init) init();

    bool whitelisted = false;
    const string file_url("file://");
    const string http_url("http://");
    const string https_url("https://");

    // Special case: This allows any file: URL to pass if the URL starts with the default
    // catalog's path.
    if (url.compare(0, file_url.size(), file_url) == 0 /*equals a file url*/) {

        // Ensure that the file path starts with the catalog root dir.
        string file_path = url.substr(file_url.size());
        BESDEBUG("bes", "RemoteAccess::Is_Whitelisted() - file_path: "<< file_path << endl);

        BESCatalog *bcat = BESCatalogList::TheCatalogList()->find_catalog(BES_DEFAULT_CATALOG);
        if (bcat) {
            BESDEBUG("bes", "RemoteAccess::Is_Whitelisted() - Found catalog: "<< bcat->get_catalog_name() << endl);
        }
        else {
            string msg = "OUCH! Unable to locate default catalog!";
            BESDEBUG("bes", "RemoteAccess::Is_Whitelisted() - " << msg << endl);
            throw BESInternalError(msg, __FILE__, __LINE__);
        }

        string catalog_root = bcat->get_root();
        BESDEBUG("bes", "RemoteAccess::Is_Whitelisted() - Catalog root: "<< catalog_root << endl);

        whitelisted = file_path.compare(0, catalog_root.size(), catalog_root) == 0;
        BESDEBUG("bes", "RemoteAccess::Is_Whitelisted() - Is_Whitelisted: "<< (whitelisted?"true":"false") << endl);
    }
    else {
        // This checks HTTP and HTTPS URLs against the whitelist patterns.
        if (url.compare(0, http_url.size(), http_url) == 0 /*equals http url */
            || url.compare(0, https_url.size(), https_url) == 0 /*equals https url */) {

            vector<string>::const_iterator i = WhiteList.begin();
            vector<string>::const_iterator e = WhiteList.end();
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
            msg = "ERROR! Unknown URL protocol! Only " + http_url + ", " + https_url + ", and " + file_url + " are supported.";
            BESDEBUG("bes", msg << endl);
            throw BESForbiddenError(msg, __FILE__, __LINE__);
        }
    }

    return whitelisted;
}

