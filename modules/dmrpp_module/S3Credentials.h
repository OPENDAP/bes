//
// Created by ndp on 2/12/20.
//

#ifndef HYRAX_GIT_S3CREDENTIALS_H
#define HYRAX_GIT_S3CREDENTIALS_H


#include "AccessCredentials.h"
#include <string>

class S3Credentials: public AccessCredentials{

public:

    std::string get_aws_access_key_id(){
        return get(AccessCredentials::ID_KEY);
    }
    std::string get_aws_secret_access_key(){
        return get(AccessCredentials::KEY_KEY);
    }
};


#endif //HYRAX_GIT_S3CREDENTIALS_H
