// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of the BES

// Copyright (c) 2020 OPeNDAP, Inc.
// Author: Nathan Potter<ndp@opendap.org>
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
// Created by ndp on 12/11/19.
//

#ifndef HYRAX_CREDENTIALSMANAGER_H
#define HYRAX_CREDENTIALSMANAGER_H

#include <memory>
#include <string>
#include <vector>
#include <mutex>
#include "url_impl.h"
#include "AccessCredentials.h"


// These are the names of the bes keys used to configure the handler.
#define CATALOG_MANAGER_CREDENTIALS "CredentialsManager.config"

class CredentialsManager {
public:
    static const char* ENV_ID_KEY;
    static const char* ENV_ACCESS_KEY;
    static const char* ENV_REGION_KEY;
    static const char* ENV_BUCKET_KEY;
    static const char* ENV_URL_KEY;
    static const char* USE_ENV_CREDS_KEY_VALUE;

private:
    std::recursive_mutex d_lock_mutex{};
    // std::string d_netrc_filename;
    bool ngaps3CredentialsLoaded;

    std::map<std::string, AccessCredentials* > creds;

    CredentialsManager();

    static void initialize_instance();
    static void delete_instance();
    AccessCredentials *load_credentials_from_env( );
    void load_ngap_s3_credentials( );


public:
    static CredentialsManager *theMngr;

    ~CredentialsManager();
    static CredentialsManager *theCM();

    void add(const std::string &url, AccessCredentials *ac);
    void load_credentials();

    void clear(){
        creds.clear();
        ngaps3CredentialsLoaded = false;
    }

    AccessCredentials *get(std::shared_ptr<http::url> &url);

    unsigned int size(){
        return creds.size();
    }

    bool hasNgapS3Credentials(){
        return ngaps3CredentialsLoaded;
    }
};





#endif //HYRAX_CREDENTIALSMANAGER_H
