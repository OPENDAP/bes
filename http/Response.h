// Response.h

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

#ifndef BES_RESPONSE_H
#define BES_RESPONSE_H

#include <string>
#include <vector>

#include <curl/curl.h>

namespace http {

class Response {
    CURLcode d_curl_code = CURLE_OK;
    unsigned int d_http_status = 0;
    std::string d_origin_url;
    std::string d_redirect_url;
    std::vector<std::string> d_headers;
    std::string d_body;
    int d_fd = -1;

public:

    Response() = default;
    ~Response() = default;
/*
    Response(CURLcode curl_code,
             unsigned int http_status,
             std::string origin_url,
             std::string redirect_url,
             std::vector<std::string> response_headers,
             std::string response_body,
             int fd
             ) :
             d_curl_code(curl_code),
             d_http_status(http_status),
             d_origin_url(std::move(origin_url)),
             d_redirect_url(std::move(redirect_url)),
             d_headers(std::move(response_headers)),
             d_body(std::move(response_body)),
             d_fd(fd)
             {}
*/
    Response(const Response &r) = default;
    Response(Response &&r) = default;

    Response &operator=(const Response &r) = default;
    Response &operator=(Response &&r) = default;

    void curl_code(CURLcode code)  {  d_curl_code = code; }
    CURLcode curl_code() const { return d_curl_code; }

    void status(unsigned int status)  { d_http_status = status; }
    unsigned int status() const { return d_http_status; }

    void origin_url(std::string url) { d_origin_url = std::move(url); }
    std::string origin_url() const { return d_origin_url; }

    void redirect_url(std::string url)  { d_redirect_url = std::move(url); }
    std::string redirect_url() const { return d_redirect_url; }

    // void headers(std::vector<std::string> hdrs)  {  d_headers = std::move(hdrs); }
    std::vector<std::string> &headers() { return d_headers; }

    void body(std::string &response_body)  {  d_body = response_body; }
    std::string &body() { return d_body; }

    void fd(int fd)  {  d_fd = fd; }
    int fd() const { return d_fd; }

    void write_response_details(std:: stringstream &msg) const;

    std::string dump() const;
};

} // http

#endif //BES_RESPONSE_H
