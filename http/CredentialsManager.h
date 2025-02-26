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

namespace http {

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
    std::map<std::string, AccessCredentials *> creds;

    CredentialsManager() = default;   // only called here to build the singleton

    // These are static because they must use C-linkage.
    static void initialize_instance();
    static void delete_instance();

    void load_credentials();
    AccessCredentials *load_credentials_from_env();

    friend class CredentialsManagerTest;
    friend class CurlUtilsTest;

public:
    static CredentialsManager *theMngr;

    ~CredentialsManager();

    static CredentialsManager *theCM();

    void add(const std::string &url, AccessCredentials *ac);

    void clear() {
        creds.clear();
        ngaps3CredentialsLoaded = false;
    }

    AccessCredentials *get(const std::shared_ptr<http::url> &url);
    AccessCredentials *get(const std::string &url);

    size_t size() const {
        return creds.size();
    }

    bool hasNgapS3Credentials() const {
        return ngaps3CredentialsLoaded;
    }
};

}   // namespace http

#endif //HYRAX_CREDENTIALSMANAGER_H
