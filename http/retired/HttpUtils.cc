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

#if 0
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
        size_t pos = disp.find('#', fnpos);
        if (pos == string::npos) pos = disp.find('=', fnpos);
        if (pos != string::npos) {
            // Got the filename to the end of the
            // string, now get it to either the end of
            // the string or the start of the next
            // attribute
            string filename;
            size_t sp = disp.find(' ', pos);
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

/**
 * @brief Look for the type of handler that can read the content type found in the \arg ctype.
 * @param ctype
 * @param type
 */
void get_type_from_content_type(const string &ctype, string &type)
{
    BESDEBUG(MODULE, prolog << "BEGIN content-type: " << ctype << endl);
    map<string,string> mime_list;
    load_mime_list_from_keys(mime_list);
    for (auto pair: mime_list) {
        BESDEBUG(MODULE, prolog << "Mime list entry: " << pair.first << " -> " << pair.second << endl);
        if (pair.second == ctype) {
            BESDEBUG(MODULE, prolog << "MATCH" << endl);
            type = pair.first;
            break;
        }

    }
#if 0
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
#endif
    BESDEBUG(MODULE, prolog << "END" << endl);
}

/**
 * Look for the type of handler that can read the url found in the \arg url.
 * This function uses the BES Catalog Utils to find the handler that can read
 * the data referenced by the URL. Essentially, it uses the filename extension
 * to make the determination.
 *
 * @note This function is only called from RemoteResource::retrieveResource()
 *
 * @param url The URL to the data
 * @param type The type of the handler that can read this file or the empty string.
 */
void get_type_from_url(const string &url, string &type) {
    const BESCatalogUtils *utils = BESCatalogList::TheCatalogList()->find_catalog("catalog")->get_catalog_utils();

    type = utils->get_handler_name(url);
}

/**
 * Loads the value of Http.MaxRedirects from TheBESKeys.
 * If the value is not found, then it is set to the default, HTTP_MAX_REDIRECTS_DEFAULT
 */
size_t load_max_redirects_from_keys() {
#if 0
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
#endif
    return TheBESKeys::TheKeys()->read_ulong_key(HTTP_MAX_REDIRECTS_KEY, HTTP_MAX_REDIRECTS_DEFAULT);
}

#endif

// http://help.adobe.com/en_US/FlashPlatform/reference/actionscript/3/package.html#encodeURIComponent()

/**
 * Thanks to https://gist.github.com/litefeel for this implementation
 * @param c
 * @param hex1
 * @param hex2
 */
static void hexchar(const unsigned char &c, unsigned char &hex1, unsigned char &hex2)
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

}

