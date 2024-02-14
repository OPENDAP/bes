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

#include "BESError.h"

namespace http {

class HttpError : public BESError {
    unsigned int d_http_status = 0;
    std::vector<std::string> d_response_headers;
    std::string d_response_body;

public:
    HttpError() = default;

    HttpError(std::string msg, std::string file, unsigned int line) :
            BESError(std::move(msg), BES_HTTP_ERROR, std::move(file), line) { }

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


    HttpError(const HttpError &src)  noexcept = default;

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

    /** @brief dumps information about this object
 *
 * Displays the pointer value of this instance along with the exception
 * message, the file from which the exception was generated, and the line
 * number in that file.
 *
 * @param strm C++ i/o stream to dump the information to
 */
    std::string dump( ) const
    {
        std::stringstream msg;

        msg << BESIndent::LMarg << "HTTPError::dump - ("
             << (void *)this << ")\n";
        BESError::dump(msg);
        BESIndent::Indent() ;
        msg << BESIndent::LMarg << "http_status: " << d_http_status << "\n" ;
        for(const auto &hdr: d_response_headers) {
            msg << BESIndent::LMarg << "response_header: " << hdr << "\n";
        }
        msg << BESIndent::LMarg << "response_body: " << d_response_body << "\n" ;
        BESIndent::UnIndent() ;
    }

    void dump(std::ostream &strm){
        strm << dump();
    }

};

} // http

#endif //BES_HTTPERROR_H
