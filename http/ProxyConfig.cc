//
// Created by ndp on 10/19/20.
//
#include "config.h"

#include <string>
#include <sstream>
#include <vector>

#include <curl/curl.h>

#include "BESSyntaxUserError.h"
#include "TheBESKeys.h"
#include "BESUtil.h"
#include "BESDebug.h"
#include "HttpNames.h"
#include "ProxyConfig.h"

using std::string;
using std::vector;
using std::stringstream;
using std::endl;

#define prolog string("ProxyConfig::").append(__func__).append("() - ")
namespace http {

    ProxyConfig *ProxyConfig::d_instance = 0;

    ProxyConfig *ProxyConfig::theOne() {
        if (d_instance)
            return d_instance;

        d_instance = new ProxyConfig();
        return d_instance;
    }


/**
 * Read the proxy configuration from the keys if possible...
 */
    void ProxyConfig::load_proxy_from_keys() {
        bool found = false;
        vector<string> vals;
        string key;

        found = false;
        key = HTTP_PROXYHOST_KEY;
        TheBESKeys::TheKeys()->get_value(key, d_host, found);
        if (found && !d_host.empty()) {
            // if the proxy host is set, then check to see if the port is
            // set. Does not need to be.
            found = false;
            string port;
            key = HTTP_PROXYPORT_KEY;
            TheBESKeys::TheKeys()->get_value(key, port, found);
            if (found && !port.empty()) {
                d_port = atoi(port.c_str());
                if (!d_port) {
                    stringstream err_msg;
                    err_msg << prolog << "The Httpd catalog proxy host is specified, but a specified port is absent";
                    throw BESSyntaxUserError(err_msg.str(), __FILE__, __LINE__);
                }
            }
            // Everything else is optional, the minimum for a proxy is host and port, so:
            d_configured = true;

            // @TODO Either use this or remove it - right now this variable is never used downstream
            // find the protocol to use for the proxy server. If none set, default to http
            found = false;
            TheBESKeys::TheKeys()->get_value(HTTP_PROXYPROTOCOL_KEY, d_protocol, found);
            if (!found || d_protocol.empty()) {
                d_protocol = "http";
            }

            // find the user to use for authenticating with the proxy server. If none set,
            // default to ""
            found = false;
            key = HTTP_PROXYUSER_KEY;
            TheBESKeys::TheKeys()->get_value(key, d_user_id, found);
            if (!found) {
                d_user_id = "";
            }

            // find the password to use for authenticating with the proxy server. If none set,
            // default to ""
            found = false;
            key = HTTP_PROXYPASSWORD_KEY;
            TheBESKeys::TheKeys()->get_value(key, d_proxy_password, found);
            if (!found) {
                d_proxy_password = "";
            }

            // find the user:password string to use for authenticating with the proxy server. If none set,
            // default to ""
            found = false;
            key = HTTP_PROXYUSERPW_KEY;
            TheBESKeys::TheKeys()->get_value(key, d_user_password, found);
            if (!found) {
                d_user_password = "";
            }

            // find the authentication mechanism to use with the proxy server. If none set,
            // default to BASIC authentication.
            found = false;
            string authType;
            key = HTTP_PROXYAUTHTYPE_KEY;
            TheBESKeys::TheKeys()->get_value(key, authType, found);
            if (found) {
                authType = BESUtil::lowercase(authType);
                if (authType == "basic") {
                    d_auth_type = CURLAUTH_BASIC;
                    BESDEBUG(HTTP_MODULE, prolog << "ProxyAuthType BASIC set." << endl);
                } else if (authType == "digest") {
                    d_auth_type = CURLAUTH_DIGEST;
                    BESDEBUG(HTTP_MODULE, prolog << "ProxyAuthType DIGEST set." << endl);
                } else if (authType == "ntlm") {
                    d_auth_type = CURLAUTH_NTLM;
                    BESDEBUG(HTTP_MODULE, prolog << "ProxyAuthType NTLM set." << endl);
                } else {
                    d_auth_type = CURLAUTH_BASIC;
                    BESDEBUG(HTTP_MODULE,
                             prolog << "User supplied an invalid value '" << authType
                                    << "'  for Gateway.ProxyAuthType. Falling back to BASIC authentication scheme."
                                    << endl);
                }
            } else {
                d_auth_type = CURLAUTH_BASIC;
            }
        }

        // Grab the value for the NoProxy regex; empty if there is none.
        found = false; // Not used
        key = HTTP_NO_PROXY_REGEX_KEY;
        TheBESKeys::TheKeys()->get_value(key, d_no_proxy_regex, found);
        if (!found) {
            d_no_proxy_regex = "";
        }

    }

} // namespace http