//
// Created by ndp on 10/19/20.
//

#ifndef HYRAX_GIT_HTTPPROXY_H
#define HYRAX_GIT_HTTPPROXY_H

#include <string>
#include <map>
#include <vector>

class HttpProxy {
private:
    static HttpProxy *d_instance;

    std::string d_protocol;
    std::string d_host;
    std::string d_user_password;
    std::string d_user_id;
    std::string d_proxy_password;
    int d_port;
    int d_auth_type;
    std::string d_no_proxy_regex;
    bool d_configured;

    HttpProxy() :
        d_protocol(""),
        d_host(""),
        d_user_password(""),
        d_user_id(""),
        d_proxy_password(""),
        d_port(-1),
        d_auth_type(-1),
        d_no_proxy_regex(""),
        d_configured(false)
        {
            load_proxy_from_keys();
        }

    void load_proxy_from_keys();

public:
    static HttpProxy *TheProxy();
    std::string protocol(){return d_protocol;}
    std::string host() {return d_host;}
    std::string password(){return d_user_password;}
    std::string user(){return d_user_id;}
    std::string proxy_password(){return d_proxy_password;}
    int port(){return d_port;}
    int auth_type(){return d_auth_type;}
    std::string no_proxy_regex(){return d_no_proxy_regex;}

};



#endif //HYRAX_GIT_HTTPPROXY_H
