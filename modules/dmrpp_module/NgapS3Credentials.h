// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of the Hyrax data server.

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

#ifndef HYRAX_GIT_S3CREDENTIALS_H
#define HYRAX_GIT_S3CREDENTIALS_H

#include "AccessCredentials.h"
#include <string>

class NgapS3Credentials: public AccessCredentials {
public:
    // These are the string keys used to express the normative key names
    // for the credentials components.
    static const std::string AWS_SESSION_TOKEN;
    static const std::string AWS_TOKEN_EXPIRATION;
    static const std::string BES_CONF_S3_ENDPOINT_KEY;
    static const std::string BES_CONF_REFRESH_KEY;
    static const std::string BES_CONF_URL_BASE;

private:
    time_t d_expiration_time;
    long refresh_margin;
    std::string distribution_api_endpoint;

public:
    NgapS3Credentials():
        d_expiration_time(0), refresh_margin(600), distribution_api_endpoint("") {}

    NgapS3Credentials(const std::string &credentials_endpoint, long refresh_margin):
        d_expiration_time(0), refresh_margin(refresh_margin), distribution_api_endpoint(credentials_endpoint) {}

    void get_temporary_credentials();

    time_t expires(){
        return d_expiration_time;
    }
    bool needsRefresh(){
        return (d_expiration_time - time(0)) < refresh_margin;
    }

    bool isS3Cred();

    std::string get(const std::string &key);

};


#endif //HYRAX_GIT_S3CREDENTIALS_H
