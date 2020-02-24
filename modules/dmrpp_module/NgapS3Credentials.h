//
// Created by ndp on 2/12/20.
//

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

    bool isS3Cred() override {return true;}

    std::string get(const std::string &key) override {
        if(needsRefresh()){
            this->get_temporary_credentials();
        }
        return AccessCredentials::get(key);
    }


};


#endif //HYRAX_GIT_S3CREDENTIALS_H
