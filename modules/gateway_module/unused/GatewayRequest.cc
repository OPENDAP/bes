// GatewayRequest.cc

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

<<<<<<< HEAD
#include <libdap/HTTPConnect.h>
#include <libdap/RCReader.h>
#include <libdap/Error.h>
#include <GNURegex.h>
=======
#include <HTTPConnect.h>
#include <RCReader.h>
#include <Error.h>
>>>>>>> master

using namespace libdap;

#include <BESInternalError.h>
#include <BESDapError.h>
#include <BESSyntaxUserError.h>
#include <BESDebug.h>
#include <BESCatalogUtils.h>
#include <BESRegex.h>

#include "GatewayRequest.h"
#include "GatewayError.h"
#include "GatewayUtils.h"
#include "config.h"

/** @brief make the remote request against the given information
 *
 * function that makes a remote request using HTTPConnect from libdap given the
 * remote request url. An HTTPResponse object is returned from the HTTPConnect
 * fetch_url method. This response contains the FILE pointer as well as the
 * name of the file to be used by the corresponding data handler
 *
 * @param url remote request url
 * @return HTTPResponse pointer for the remote request response
 * @throws BESInternalError if there is a problem making the remote request or
 * the request fails
 */
HTTPResponse *
GatewayRequest::make_request(const string &url, string &type)
{
    if (url.empty()) {
        string err = "Remote Request URL is empty";
        throw BESInternalError(err, __FILE__, __LINE__);
    }

    BESDEBUG("gateway", "GatewayRequest::make_request" << endl);
    BESDEBUG("gateway", "  request = " << url << endl);

    RCReader *rcr = RCReader::instance();

    // Don't set up the proxy server for URLs that match the 'NoProxy'
    // regex set in the gateway.conf file.
    bool configure_proxy = true;
    // Don't create the regex if the string is empty
    if (!GatewayUtils::NoProxyRegex.empty()) {
        BESRegex r(GatewayUtils::NoProxyRegex.c_str());
        if (r.match(url.c_str(), url.size()) != -1) {
            BESDEBUG("gateway",
                "Gateway found NoProxy match. BESRegex: " << GatewayUtils::NoProxyRegex << "; Url: " << url << endl);
            configure_proxy = false;
        }
    }

    if (configure_proxy) {
        rcr->set_proxy_server_protocol(GatewayUtils::ProxyProtocol);
        rcr->set_proxy_server_host(GatewayUtils::ProxyHost);
        rcr->set_proxy_server_port(GatewayUtils::ProxyPort);
    }

    // GatewayUtils::useInternalCache defaults to false; use squid...
    rcr->set_use_cache(GatewayUtils::useInternalCache);
    HTTPConnect connect(rcr);
    connect.set_cache_enabled(GatewayUtils::useInternalCache);

    HTTPResponse *response = 0;
    try {
        response = connect.fetch_url(url);
    }
    catch (Error &e) {
        throw BESDapError("Failed to connect to remote resource. Msg: " + e.get_error_message(), false,
            e.get_error_code(), __FILE__, __LINE__);
    }
    catch (BESError &e) {
        throw;
    }
    catch (...) {
        string msg = (string) "Unknown exception fetching remote request " + url;
        BESInternalError err(msg, __FILE__, __LINE__);
        throw err;
    }

    if (!response) {
        string msg = (string) "Response empty fetching remote request " + url;
        BESInternalError err(msg, __FILE__, __LINE__);
        throw err;
    }

    // A remote request is successful if we get data or if there is a
    // failed response in the http header.
    if (response->get_status() != 200 && response->get_status() != 304) {
        BESDEBUG("gateway", " request FAILED with status " << response->get_status() << endl);

        // get the error information from the temporary file
        string err;
        try {
            BESDEBUG("gateway", " reading text error from response file " << response->get_file() << endl);
            GatewayError::read_error(response->get_file(), err, url);
        }
        catch (...) {
            err += "Unable to load the error text";
        }

        // toss the response
        delete response;
        response = 0;

        throw BESInternalError(err, __FILE__, __LINE__);
    }

    if (type.empty() || type == "gateway") {
        // make sure that the type is empty, and not gateway
        type = "";

        // Try and figure out the file type first from the
        // Content-Disposition in the http header response.
        string disp;
        string ctype;
        vector<string> *hdrs = response->get_headers();
        if (hdrs) {
            vector<string>::const_iterator i = hdrs->begin();
            vector<string>::const_iterator e = hdrs->end();
            for (; i != e; i++) {
                string hdr_line = (*i);
                hdr_line = BESUtil::lowercase(hdr_line);
                if (hdr_line.find("content-disposition") != string::npos) {
                    // Content disposition exists
                    disp = hdr_line;
                }
                if (hdr_line.find("content-type") != string::npos) {
                    ctype = hdr_line;
                }
            }
        }

        if (!disp.empty()) {
            // Content disposition exists, grab the filename
            // attribute
            GatewayUtils::Get_type_from_disposition(disp, type);
            BESDEBUG( "gateway", "Looked at disposition " << disp
                << " for type, returned \"" << type
                << "\"" << endl );
            }

            // still haven't figured out the type. Check the content-type
            // next, translate to the BES module name. It's also possible
            // that even though Content-disposition was available, we could
            // not determine the type of the file.
        if (type.empty() && !ctype.empty()) {
            GatewayUtils::Get_type_from_content_type(ctype, type);
            BESDEBUG( "gateway", "Looked at content type " << ctype
                << " for type, returned \"" << type
                << "\"" << endl );
            }

            // still haven't figured out the type. Now check the actual URL
            // and see if we can't match the URL to a module name
        if (type.empty()) {
            GatewayUtils::Get_type_from_url(url, type);
            BESDEBUG( "gateway", "Looked at url " << url
                << " for type, returned \"" << type
                << "\"" << endl );
            }

            // still couldn't figure it out ... throw an exception
        if (type.empty()) {
            string err = (string) "Unable to determine the type of data" + " returned from " + url;
            throw BESSyntaxUserError(err, __FILE__, __LINE__);
        }
    }

    BESDEBUG("gateway", "GatewayRequest::make_request - done" << endl);

    return response;
}

