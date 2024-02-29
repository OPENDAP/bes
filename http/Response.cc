// Response.cc

// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of the OPeNDAP Back-End Server (BES)
// and creates a set of allowed hosts that may be
// accessed by the server as part of it's routine operation.

// Copyright (c) 2024 OPeNDAP, Inc.
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

// Authors:
//      ndp       Nathan D. Potter <ndp@opendap.org>

#include "config.h"

#include <string>
#include <vector>
#include <sstream>


#include "Response.h"

#include "BESError.h"

namespace http {

/**
 *
 * @param http_status
 * @param response_headers
 * @param response_body
 * @param msg
 */
void Response::write_response_details(std:: stringstream &msg) const
{
    msg << "# -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --\n";
    msg << "# http::Response Details\n";
    msg << "# origin_url: " << d_origin_url << "\n";
    msg << "# curl_code: " << d_curl_code << "\n";
    msg << "# http_status: " << d_http_status << "\n";
    msg << "# last accessed url: " << d_redirect_url << "\n";
    msg << "# Response Headers -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --\n";
    for (const auto &hdr: d_headers) {
        msg << "  "<<  hdr << "\n";
    }
    msg << "# BEGIN Response Body -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --\n";
    msg << d_body << "\n";
    msg << "# END Response Body   -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --\n";
}



std::string Response::dump( ) const
{
    std::stringstream msg;

    msg << BESIndent::LMarg << "http::Response::dump - (" << (void *)this << ")\n";
    BESIndent::Indent() ;
    msg << BESIndent::LMarg << "        curl_code: " << d_curl_code << "\n" ;
    msg << BESIndent::LMarg << "      http_status: " << d_http_status << "\n" ;
    msg << BESIndent::LMarg << "       origin_url: " << d_origin_url << "\n" ;
    msg << BESIndent::LMarg << "last_accessed_url: " << d_redirect_url << "\n" ;

    for(const auto &hdr: d_headers) {
        msg << BESIndent::LMarg << "  response_header: " << hdr << "\n";
    }

    msg << BESIndent::LMarg << "    response_body: " << d_body << "\n" ;
    BESIndent::UnIndent() ;

    return msg.str();
}






} // http