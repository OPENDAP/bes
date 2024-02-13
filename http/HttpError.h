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

#include "BESError.h"

namespace http {

class HttpError : public BESError {
    unsigned int d_http_status;
    std::vector<std::string> d_response_headers;
    std::string d_response_body;

public:
    HttpError(std::string msg, std::string file, unsigned int line) :
            BESError(std::move(msg), BES_HTTP_ERROR, std::move(file), line), d_http_status(0) { }

    HttpError(std::string msg,
              unsigned int http_status,
              std::vector<std::string> response_headers,
              std::string response_body,
              std::string file,
              unsigned int line) :
            BESError(std::move(msg), BES_HTTP_ERROR, std::move(file), line),
            d_http_status(http_status),
            d_response_headers(std::move(response_headers)),
            d_response_body(std::move(response_body)){ }

    ~HttpError() override = default;

    void set_response_body(std::string body){
        d_response_body = std::move(body);
    }
    std::string get_response_body() const{
        return d_response_body;
    }

    void set_response_headers(std::vector<std::string> hdrs){
        d_response_headers = std::move(hdrs);
    }
    std::vector<std::string> get_response_headers() const {
        return d_response_headers;
    }

    void set_http_status(unsigned int status){
        d_http_status = status;
    }

    unsigned int get_http_status() const {
        return d_http_status;
    }

};

} // http

#endif //BES_HTTPERROR_H
