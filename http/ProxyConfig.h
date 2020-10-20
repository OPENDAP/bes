//
// Created by ndp on 10/19/20.
//

#ifndef HYRAX_GIT_PROXYCONFIG_H
#define HYRAX_GIT_PROXYCONFIG_H

#include <string>
#include <map>
#include <vector>

namespace http {

    class ProxyConfig {
    private:
        static ProxyConfig *d_instance;

        std::string d_protocol;
        std::string d_host;
        std::string d_user_password;
        std::string d_user_id;
        std::string d_proxy_password;
        int d_port;
        int d_auth_type;
        std::string d_no_proxy_regex;
        bool d_configured;

        ProxyConfig() :
                d_protocol(""),
                d_host(""),
                d_user_password(""),
                d_user_id(""),
                d_proxy_password(""),
                d_port(-1),
                d_auth_type(-1),
                d_no_proxy_regex(""),
                d_configured(false) {
            load_proxy_from_keys();
        }

        void load_proxy_from_keys();

    public:
        static ProxyConfig *theOne();

        std::string protocol() { return d_protocol; }

        std::string host() { return d_host; }

        std::string password() { return d_user_password; }

        std::string user() { return d_user_id; }

        std::string proxy_password() { return d_proxy_password; }

        int port() { return d_port; }

        int auth_type() { return d_auth_type; }

        std::string no_proxy_regex() { return d_no_proxy_regex; }

        bool is_configured() { return d_configured; }

    };


} // namespace http
#endif //HYRAX_GIT_PROXYCONFIG_H
