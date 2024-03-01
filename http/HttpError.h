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

#include "BESError.h"
#include "unused/Response.h"

namespace http {

class HttpError : public BESError {
    Response d_http_response;

public:
    HttpError() = default;

    HttpError(std::string msg, Response resp, std::string file, unsigned int line):
            BESError(std::move(msg), BES_HTTP_ERROR, std::move(file), line),
            d_http_response{std::move(resp)} {}

    HttpError(const HttpError &src)  noexcept = default;

    ~HttpError() override = default;

    std::string body() {
        return d_http_response.body();
    }

    std::vector<std::string> headers()  {
        return d_http_response.headers();
    }

    unsigned int http_status() const {
        return d_http_response.status();
    }

    /** @brief dumps information about this object
 *
 * Displays the pointer value of this instance along with the exception
 * message, the file from which the exception was generated, and the line
 * number in that file.
 *
 * @param strm C++ i/o stream to dump the information to
 */
    std::string dump() const
    {
        std::stringstream msg;

        msg << BESIndent::LMarg << "HTTPError::dump - ("
             << (void *)this << ")\n";
        BESError::dump(msg);
        BESIndent::Indent() ;
        msg << d_http_response.dump();
        BESIndent::UnIndent() ;
        return msg.str();
    }

    void dump(std::ostream &strm) const override
    {
        strm << dump();
    }

};

} // http

#endif //BES_HTTPERROR_H
