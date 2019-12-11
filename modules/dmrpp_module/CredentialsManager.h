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

//class access_credentials;

class access_credentials {
public:
    static const std::string ID;
    static const std::string KEY;
    static const std::string REGION;
    static const std::string BUCKET;
private:
    std::map<std::string, std::string> kvp;
    bool s3_tested, is_s3;
public:
    access_credentials()= default;
    access_credentials(const access_credentials &ac) = default;
    access_credentials(const std::string &id, const std::string &key);
    access_credentials(
        const std::string &id,
        const std::string &key,
        const std::string &region,
        const std::string &bucket);

    std::string get(const std::string &key);
    void add(const std::string &key, const std::string &value);
    bool isS3Cred();
};



class CredentialsManager {
private:
    CredentialsManager();
    std::map<std::string, access_credentials* > creds;

public:
    static CredentialsManager *theMngr;

    ~CredentialsManager();

    static CredentialsManager *theCM(){
        if(!theMngr){
            theMngr= new CredentialsManager();
        }
        return theMngr;
    }

    void add(const std::string &url, access_credentials *ac);

    access_credentials *get(const std::string &url);

    void load_credentials();

};





#endif //HYRAX_CREDENTIALSMANAGER_H
