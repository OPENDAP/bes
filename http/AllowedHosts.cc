// RemoteAccess.cc

// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of the OPeNDAP Back-End Server (BES)
// and creates an allowed hosts list of which systems that may be
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

#include <sstream>

#include "BESUtil.h"
#include "BESCatalog.h"
#include "BESCatalogList.h"
#include "BESCatalogUtils.h"
#include "BESRegex.h"
#include "TheBESKeys.h"
#include "BESInternalError.h"
#include "BESDebug.h"
#include "BESNotFoundError.h"
#include "BESForbiddenError.h"
#include "BESLog.h"

#include "HttpNames.h"
#include "url_impl.h"

#include "AllowedHosts.h"

using namespace std;

#define MODULE "ah"
#define prolog string("AllowedHosts::").append(__func__).append("() - ")

namespace http {

AllowedHosts *AllowedHosts::d_instance = nullptr;

/**
 * Run once_flag for initializing the singleton instance.
 */
static std::once_flag d_ah_init_once;

/**
 * @brief Static accessor for the singleton
 *
 * @return A pointer to the singleton instance
 */
AllowedHosts *
AllowedHosts::theHosts() {
    std::call_once(d_ah_init_once, AllowedHosts::initialize_instance);
    return d_instance;
}

AllowedHosts::AllowedHosts() {
    bool found = false;
    string key = ALLOWED_HOSTS_BES_KEY;
    TheBESKeys::TheKeys()->get_values(ALLOWED_HOSTS_BES_KEY, d_allowed_hosts, found);
    if (!found) {
        throw BESInternalError(string("The allowed hosts key, '") + ALLOWED_HOSTS_BES_KEY
                               + "' has not been configured.", __FILE__, __LINE__);
    }
}

/**
* @brief This private static function initializes the singleton instance.
*/
void AllowedHosts::initialize_instance() {
    d_instance = new AllowedHosts();
#ifdef HAVE_ATEXIT
    atexit(delete_instance);
#endif
}

/**
 * @brief This private static function can only be called once since it destroys the singleton instance for the duration of the process.
 */
void AllowedHosts::delete_instance() {
    delete d_instance;
    d_instance = 0;
}

/**
 * This method provides an access condition assessment for URLs and files
 * to be accessed by the BES. The http and https URLs are verified against a
 * allowed hosts list assembled from configuration. All file URLs are checked to be
 * sure that they reference a resource within the BES default catalog.
 *
 * @param candidate_url The URL to test
 * @return True if the URL may be dereferenced, given the BES's configuration,
 * false otherwise.
 */
bool AllowedHosts::is_allowed(shared_ptr<http::url> candidate_url) {
    string error_msg;
    return is_allowed(candidate_url, error_msg);
}

// TODO change this so that it does not throw an exception for the last case. OR, it always throws.
//  jhrg 11/22/24
bool AllowedHosts::is_allowed(shared_ptr<http::url> candidate_url, std::string &why_not) {
    BESDEBUG(MODULE, prolog << "BEGIN candidate_url: " << candidate_url->str() << endl);
    bool isAllowed = false;

    // Special case: This allows any file: URL to pass if the URL starts with the default
    // catalog's path.
    if (candidate_url->protocol() == FILE_PROTOCOL) {

        // Ensure that the file path starts with the catalog root dir.
        // We know that when a file URL is parsed by http::url it stores everything in after the "file://" mark in
        // the path, as there is no hostname.
        string file_path = candidate_url->path();
        BESDEBUG(MODULE, prolog << "   file_path: '" << file_path <<
                                "' (length: " << file_path.size() << " size: " << file_path.size() << ")" <<endl);
        // Get the BES Catalog
        BESCatalogList *bcl = BESCatalogList::TheCatalogList();
        string default_catalog_name = bcl->default_catalog_name();
        BESDEBUG(MODULE, prolog << "Searching for catalog named: '" << default_catalog_name << "'" << endl);
        BESCatalog *bcat = bcl->find_catalog(default_catalog_name);
        if (bcat) {
            BESDEBUG(MODULE, prolog << "Found catalog named: '" << bcat->get_catalog_name() << "'" << endl);
        } else {
            string error_msg = "INTERNAL_ERROR: Unable to locate default catalog. Check BES configuration.";
            BESDEBUG(MODULE, prolog << error_msg << endl);
            throw BESInternalError(error_msg, __FILE__, __LINE__);
        }

        string catalog_root = bcat->get_root();
        BESDEBUG(MODULE, prolog << "catalog_root: '" << catalog_root <<
            "' (length: " << catalog_root.size() << " size: " << catalog_root.size() << ")" << endl);

        string relative_path;
        if (file_path[0] == '/') {
            if (file_path.size() < catalog_root.size()) {
                // Upward traversal is not allowed (specified resource path is shorter than data root path)
                why_not = "Path is out of scope from configuration.";
                isAllowed = false;
            } else {
                BESDEBUG(MODULE, prolog << "file_path: " << file_path << endl);
                BESDEBUG(MODULE, prolog << "catalog_root: " << catalog_root << endl);
                size_t ret = file_path.find(catalog_root);
                BESDEBUG(MODULE, prolog << "file_path.find(catalog_root): " << ret << endl);
                isAllowed = (ret == 0);
                relative_path = file_path.substr(catalog_root.size());
                BESDEBUG(MODULE, prolog << "relative_path: " << relative_path << endl);
                BESDEBUG(MODULE, prolog << "isAllowed: " << (isAllowed?"true":"false") << endl);

            }
        } else {
            BESDEBUG(MODULE, prolog << "Relative path detected");
            relative_path = file_path;
            isAllowed = true;
        }

        // string::find() returns 0 if the submitted path begins with the catalog root.
        // And since we are just looking at the catalog.root as a prefix of the resource
        // name we only allow access to the resource for an exact match.
        if (isAllowed) {
            // If we stop adding a '/' to file_path values that don't begin with one
            // then we need to detect the use of the relative path here
            bool follow_sym_links = bcat->get_catalog_utils()->follow_sym_links();
            try {
                BESUtil::check_path(relative_path, catalog_root, follow_sym_links);
            }
            catch (BESNotFoundError &e) {
                why_not = e.get_message();
                isAllowed = false;
            }
            catch (BESForbiddenError &e) {
                why_not = e.get_message();
                isAllowed = false;
            }
        }
        BESDEBUG(MODULE, prolog << "File Access Allowed: " << (isAllowed ? "true " : "false ") << endl);
    } else if(candidate_url->protocol() == HTTPS_PROTOCOL || candidate_url->protocol() == HTTP_PROTOCOL ){

        isAllowed = candidate_url->is_trusted() || check(candidate_url->str());

        BESDEBUG(MODULE, prolog << "HTTP Access Allowed: " << (isAllowed ? "true " : "false ") << endl);
    }
    else {
        stringstream ss;
        ss << "The candidate_url utilizes an unsupported protocol '" << candidate_url->protocol() << "'" ;
        BESDEBUG(MODULE, prolog << ss.str() << endl);
        throw BESInternalError(ss.str(),__FILE__,__LINE__);
    }
    BESDEBUG(MODULE, prolog << "END Access Allowed: " << (isAllowed ? "true " : "false ") << endl);
    return isAllowed;
}

bool AllowedHosts::check(const std::string &url){
    bool isAllowed=false;
    auto it = d_allowed_hosts.begin();
    auto end_it = d_allowed_hosts.end();
    for (; it != end_it && !isAllowed; it++) {
        string a_regex_pattern = *it;
        BESRegex reg_expr(a_regex_pattern.c_str());
        int match_result = reg_expr.match(url.c_str(), url.size());
        if (match_result >= 0) {
            auto match_length = (unsigned int) match_result;
            if (match_length == url.size()) {
                BESDEBUG(MODULE,
                         prolog << "FULL MATCH. pattern: " << a_regex_pattern << " url: " << url << endl);
                isAllowed = true;;
            } else {
                BESDEBUG(MODULE,
                         prolog << "No Match. pattern: " << a_regex_pattern << " url: " << url << endl);
            }
        }
    }
    return isAllowed;
}

} // namespace http
