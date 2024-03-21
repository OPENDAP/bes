// -*- mode: c++; c-basic-offset:4 -*-
//
// This file is part of the BES http package, part of the Hyrax data server.
//
// Copyright (c) 2024 OPeNDAP, Inc.
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
//
// Authors:
//      ndp       Nathan Potter <ndp@opendap.org>

#ifndef BES_HTTPERROR_H
#define BES_HTTPERROR_H

#include <string>
#include <utility>
#include <vector>
#include <sstream>

#include <curl/curl.h>

#include "BESInfo.h"
#include "BESError.h"

namespace http {

class HttpError : public BESError {
    CURLcode d_curl_code = CURLE_OK;
    long d_http_status;
    std::string d_origin_url;
    std::string d_redirect_url;
    std::vector<std::string> d_response_headers;
    std::string d_response_body;

public:
    HttpError() = default;

    HttpError(const std::string &msg,
              const CURLcode code,
              const long http_status,
              const std::string &origin_url,
              const std::string &redirect_url,
              const std::vector<std::string> &response_headers,
              const std::string &response_body,
              const std::string &file,
              const int line) :
            BESError(msg, BES_HTTP_ERROR, file, line),
            d_curl_code(code),
            d_http_status(http_status),
            d_origin_url(origin_url),
            d_redirect_url(redirect_url),
            d_response_headers(response_headers),
            d_response_body(response_body) {};

    HttpError(const std::string &msg,
              const CURLcode code,
              const long http_status,
              const std::string &origin_url,
              const std::string &redirect_url,
              const std::string &file,
              const int line) :
            BESError(msg, BES_HTTP_ERROR, file, line),
            d_curl_code(code),
            d_http_status(http_status),
            d_origin_url(origin_url),
            d_redirect_url(redirect_url) {};


    HttpError(std::string msg, std::string file, unsigned int line) :
            BESError(std::move(msg), BES_HTTP_ERROR, std::move(file), line) {}

    HttpError(const HttpError &src) = default;

    ~HttpError() override = default;

    std::string origin_url() const { return d_origin_url; }

    std::string redirect_url() const { return d_redirect_url; }

    CURLcode curl_code() const { return d_curl_code; }

    long http_status() const { return d_http_status; }

    std::vector<std::string> response_headers() const { return d_response_headers; }

    std::string response_body() const { return d_response_body; }

    void add_my_error_details_to(BESInfo &info) const override;

    /**
     * @brief Returns a string describing this object and its state.
     * @return
     */
    std::string dump() const;

    /** @brief dumps information about this object
     *
     * Displays the pointer value of this instance along with the exception
     * message, the file from which the exception was generated, and the line
     * number in that file.
     *
     * @param strm C++ i/o stream to dump the information to
     */
    void dump(std::ostream &strm) const override {
        strm << dump();
    }

};

} // http

#endif //BES_HTTPERROR_H
