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
#include <mutex>

// These are the names of the bes keys used to configure the handler.
#define CATALOG_MANAGER_CREDENTIALS "CredentialsManager.config"

namespace http {

class AccessCredentials;
class url;

class CredentialsManager {
public:
    static const char *ENV_ID_KEY;
    static const char *ENV_ACCESS_KEY;
    static const char *ENV_REGION_KEY;
    static const char *ENV_URL_KEY;
    static const char *USE_ENV_CREDS_KEY_VALUE;

private:
    std::recursive_mutex d_lock_mutex{};

    bool ngaps3CredentialsLoaded = false;
    std::map<std::string, AccessCredentials *, std::less<>> creds{};

    static std::unique_ptr<CredentialsManager> d_instance;
    static std::once_flag d_init_once;

    CredentialsManager() = default;   // only called here to build the singleton

    void load_credentials();
    static AccessCredentials *load_credentials_from_env();
    void load_ngap_s3_credentials() const;

    friend class CredentialsManagerTest;
    friend class CurlUtilsTest;     // so that a test can call load_credentials() jhrg 5/16/23

public:
    ~CredentialsManager();

    static CredentialsManager *theCM();

    void add(const std::string &url, AccessCredentials *ac);
    AccessCredentials *get(const std::shared_ptr<http::url> &url);

    void clear() {
        creds.clear();
        ngaps3CredentialsLoaded = false;
    }

    size_t size() const {
        return creds.size();
    }

    bool hasNgapS3Credentials() const {
        return ngaps3CredentialsLoaded;
    }
};

}   // namespace http

#endif // HYRAX_CREDENTIALSMANAGER_H
