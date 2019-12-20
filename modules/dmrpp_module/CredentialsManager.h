// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of the BES

// Copyright (c) 2019 OPeNDAP, Inc.
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

#include <string>
#include <vector>

// These are the names of the bes keys used to configure the handler.
#define CATALOG_MANAGER_CREDENTIALS "CM.credentials"

class AccessCredentials {
public:
    static const std::string ID;
    static const std::string KEY;
    static const std::string REGION;
    static const std::string BUCKET;
    static const std::string URL;
private:
    std::map<std::string, std::string> kvp;
    bool s3_tested, is_s3;
    std::string d_config_name;
public:
    AccessCredentials()= default;
    AccessCredentials(std::string config_name){ d_config_name = config_name;}
    AccessCredentials(const AccessCredentials &ac) = default;

    std::string get(const std::string &key);
    void add(const std::string &key, const std::string &value);
    bool isS3Cred();
    std::string to_json();
};



class CredentialsManager {
private:
    std::map<std::string, AccessCredentials* > creds;
    CredentialsManager();
    static void initialize_instance();
    static void delete_instance();

    static AccessCredentials *load_credentials_from_env( );

public:
    static CredentialsManager *theMngr;

    ~CredentialsManager();

    static CredentialsManager *theCM(){
        if (theMngr == 0) initialize_instance();
        return theMngr;
    }

    void add(const std::string &url, AccessCredentials *ac);

    AccessCredentials *get(const std::string &url);

    static void load_credentials();

    unsigned int size(){
        return creds.size();
    }
};





#endif //HYRAX_CREDENTIALSMANAGER_H
