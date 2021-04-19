// -*- mode: c++; c-basic-offset:4 -*-
// This file is part of the BES http package, part of the Hyrax data server.
//
// Copyright (c) 2020 OPeNDAP, Inc.
// Author: Nathan Potter <ndp@opendap.org>
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
// Authors:
//      ndp       Nathan Potter <ndp@opendap.org>
#include "config.h"

#include <curl/curl.h>
#include <cstdio>
#include <sstream>
#include <map>
#include <vector>
#include <unistd.h>
#include <algorithm>    // std::for_each
#include <time.h>
#include <BESContextManager.h>

#include "rapidjson/document.h"


#include "BESSyntaxUserError.h"
#include "BESForbiddenError.h"
#include "BESNotFoundError.h"
#include "BESTimeoutError.h"
#include "BESInternalError.h"
#include "BESDebug.h"
#include "BESRegex.h"
#include "TheBESKeys.h"
#include "BESUtil.h"
#include "BESLog.h"
#include "BESStopWatch.h"

#include "BESSyntaxUserError.h"
#include "HttpNames.h"
#include "HttpUtils.h"
#include "ProxyConfig.h"
#include "AllowedHosts.h"
#include "CurlUtils.h"
#include "EffectiveUrlCache.h"

#include "url_impl.h"

#define MODULE "curl"

using std::endl;
using std::string;
using std::map;
using std::vector;
using std::stringstream;
using std::ostringstream;
using namespace http;

#define prolog std::string("CurlUtils::").append(__func__).append("() - ")

namespace curl {

static const unsigned int retry_limit = 10; // Amazon's suggestion
static const useconds_t uone_second = 1000 * 1000; // one second in micro seconds (which is 1000

// Forward declaration
curl_slist *add_edl_auth_headers(struct curl_slist *request_headers);

// Set this to 1 to turn on libcurl's verbose mode (for debugging).
int curl_trace = 0;

#define CLIENT_ERR_MIN 400
#define CLIENT_ERR_MAX 417
const char *http_client_errors[CLIENT_ERR_MAX - CLIENT_ERR_MIN + 1] = {
        "Bad Request:",
        "Unauthorized: Contact the server administrator.",
        "Payment Required.",
        "Forbidden: Contact the server administrator.",
        "Not Found: The underlying data source or server could not be found.",
        "Method Not Allowed.",
        "Not Acceptable.",
        "Proxy Authentication Required.",
        "Request Time-out.",
        "Conflict.",
        "Gone.",
        "Length Required.",
        "Precondition Failed.",
        "Request Entity Too Large.",
        "Request URI Too Large.",
        "Unsupported Media Type.",
        "Requested Range Not Satisfiable.",
        "Expectation Failed."
};

#define SERVER_ERR_MIN 500
#define SERVER_ERR_MAX 505
const char *http_server_errors[SERVER_ERR_MAX - SERVER_ERR_MIN + 1] =
        {
                "Internal Server Error.",
                "Not Implemented.",
                "Bad Gateway.",
                "Service Unavailable.",
                "Gateway Time-out.",
                "HTTP Version Not Supported."
        };

/**
 * @brief Translates an HTTP status code into an error message.
 *
 * It works for those code greater than or equal to 400.
 *
 * @param status The HTTP status to associate with an error message
 * @return The error message assciated with status.
 */
string http_status_to_string(int status) {
    if (status >= CLIENT_ERR_MIN && status <= CLIENT_ERR_MAX)
        return string(http_client_errors[status - CLIENT_ERR_MIN]);
    else if (status >= SERVER_ERR_MIN && status <= SERVER_ERR_MAX)
        return string(http_server_errors[status - SERVER_ERR_MIN]);
    else {
        stringstream msg;
        msg << "Unknown HTTP Error: " << status;
        return msg.str();
    }
}

/**
 * @brief Translate a cURL authentication type value (int) into a human readable string.
 * @param auth_type The cURL authentication type value to convert
 * @return The human readable string associated with auth_type.
 */
static string getCurlAuthTypeName(const int auth_type) {

    string authTypeString;
    int match;

    match = auth_type & CURLAUTH_BASIC;
    if (match) {
        authTypeString += "CURLAUTH_BASIC";
    }

    match = auth_type & CURLAUTH_DIGEST;
    if (match) {
        if (!authTypeString.empty())
            authTypeString += " ";
        authTypeString += "CURLAUTH_DIGEST";
    }

    match = auth_type & CURLAUTH_DIGEST_IE;
    if (match) {
        if (!authTypeString.empty())
            authTypeString += " ";
        authTypeString += "CURLAUTH_DIGEST_IE";
    }

    match = auth_type & CURLAUTH_GSSNEGOTIATE;
    if (match) {
        if (!authTypeString.empty())
            authTypeString += " ";
        authTypeString += "CURLAUTH_GSSNEGOTIATE";
    }

    match = auth_type & CURLAUTH_NTLM;
    if (match) {
        if (!authTypeString.empty())
            authTypeString += " ";
        authTypeString += "CURLAUTH_NTLM";
    }

#if 0
    match = auth_type & CURLAUTH_ANY;
    if(match){
        if(!authTypeString.empty())
            authTypeString += " ";
        authTypeString += "CURLAUTH_ANY";
    }


    match = auth_type & CURLAUTH_ANY;
    if(match){
        if(!authTypeString.empty())
            authTypeString += " ";
        authTypeString += "CURLAUTH_ANYSAFE";
    }


    match = auth_type & CURLAUTH_ANY;
    if(match){
        if(!authTypeString.empty())
            authTypeString += " ";
        authTypeString += "CURLAUTH_ONLY";
    }
#endif

    return authTypeString;
}

/**
 * @brief A libcurl callback function that ignores the data entirely. nothing is written. Ever.
 */
static size_t writeNothing(char */* data */, size_t /* size */, size_t nmemb, void * /* userdata */) {
    return nmemb;
}

/**
 * libcurl call back function that is used to write data to a passed open file descriptor (that would
 * be instead of the default open FILE *)
 */
    static size_t writeToOpenFileDescriptor(char *data, size_t /* size */, size_t nmemb, void *userdata) {

        int *fd = (int *) userdata;

        BESDEBUG(MODULE, prolog << "Bytes received " << nmemb << endl);
        int wrote = write(*fd, data, nmemb);
        BESDEBUG(MODULE, prolog << "Bytes written " << wrote << endl);

        return wrote;
    }


/**
 * @brief A libcurl callback function used to read response headers.
 *
 * Read headers, line by line, from ptr.
 * The fourth param is really supposed to be a FILE,
 * but libcurl just holds the pointer and passes it to this function
 * without using it itself.
 * What actually happens? This code was moved and I am not sure how this works
 * (which it does) now. I use that to pass in a pointer to the HTTPConnect that
 * initiated the HTTP request so that there's some place to dump the headers.
 *
 * Note that this function just saves the headers in a
 * vector of strings. Later on the code (see fetch_url()) parses the headers
 * special to the DAP.
 *
 * @param ptr A pointer to one line of character data; one header.
 * @param size Size of each character (nominally one byte).
 * @param nmemb Number of bytes.
 * @param resp_hdrs A pointer to a vector<string>. Set in read_url.
 * @return The number of bytes processed. Must be equal to size * nmemb or
 * libcurl will report an error.
 */
static size_t save_http_response_headers(void *ptr, size_t size, size_t nmemb, void *resp_hdrs) {
    BESDEBUG(MODULE, prolog << "Inside the header parser." << endl);
    vector<string> *hdrs = static_cast<vector<string> * >(resp_hdrs);

    // Grab the header, minus the trailing newline. Or \r\n pair.
    string complete_line;
    if (nmemb > 1 && *(static_cast<char *>(ptr) + size * (nmemb - 2)) == '\r')
        complete_line.assign(static_cast<char *>(ptr), size * (nmemb - 2));
    else
        complete_line.assign(static_cast<char *>(ptr), size * (nmemb - 1));

    // Store all non-empty headers that are not HTTP status codes
    if (complete_line != "" && complete_line.find("HTTP") == string::npos) {
        BESDEBUG(MODULE, prolog << "Header line: " << complete_line << endl);
        hdrs->push_back(complete_line);
    }

    return size * nmemb;
}


/**
 * @brief A libcurl callback for debugging protocol issues.
 * @param info
 * @param msg
 * @param size
 * @return
 */
static int curl_debug(CURL *, curl_infotype info, char *msg, size_t size, void *) {
    string message(msg, size);

    switch (info) {
        case CURLINFO_TEXT:
            BESDEBUG(MODULE, prolog << "Text: " << message << endl);
            break;
        case CURLINFO_HEADER_IN:
            BESDEBUG(MODULE, prolog << "Header in: " << message << endl);
            break;
        case CURLINFO_HEADER_OUT:
            BESDEBUG(MODULE, prolog << "Header out: " << endl << message << endl);
            break;
        case CURLINFO_DATA_IN:
            BESDEBUG(MODULE, prolog << "Data in: " << message << endl);
            break;
        case CURLINFO_DATA_OUT:
            BESDEBUG(MODULE, prolog << "Data out: " << message << endl);
            break;
        case CURLINFO_END:
            BESDEBUG(MODULE, prolog << "End: " << message << endl);
            break;
#ifdef CURLINFO_SSL_DATA_IN
            case CURLINFO_SSL_DATA_IN:
        BESDEBUG(MODULE,  prolog << "SSL Data in: " << message << endl ); break;
#endif
#ifdef CURLINFO_SSL_DATA_OUT
            case CURLINFO_SSL_DATA_OUT:
        BESDEBUG(MODULE,  prolog << "SSL Data out: " << message << endl ); break;
#endif
        default:
            BESDEBUG(MODULE, prolog << "Curl info: " << message << endl);
            break;
    }
    return 0;
}


/**
 * @brief Functor to add a single string to a curl_slist.
 *
 * This is used to transfer a list of headers from a vector<string> object to a curl_slist.
 */
class BuildHeaders : public std::unary_function<const string &, void> {
    struct curl_slist *d_cl;

public:
    BuildHeaders() : d_cl(0) {}

    void operator()(const string &header) {
        BESDEBUG(MODULE, prolog << "Adding '" << header.c_str() << "' to the header list." << endl);
        d_cl = curl_slist_append(d_cl, header.c_str());
    }

    struct curl_slist *get_headers() {
        return d_cl;
    }
};

    /**
     * @brief Configure the proxy options for the passed curl object.
     * The passed URL is the target URL. If the target URL matches the
     * HttpUtils::NoProxyRegex in the config file, then no proxying is done.
     *
     * The proxy configuration is stored in the http configuration file, http.conf.
     * The configuration utilizes the following keys. The:
     * Http.ProxyHost=<hostname or ip address>
     * Http.ProxyPort=<port number>
     * Http.ProxyAuthType=<basic | digest | ntlm>
     * Http.ProxyUser=<username>
     * Http.ProxyPassword=<password>
     * Http.ProxyUserPW=<username:password>
     * Http.ProxyProtocol=< https | http >
     * Http.NoProxy=<regex_to_match_no_proxy_urls>
     *
     * @param curl The cURL easy handle to configure.
     * @param target_url The url used to configure the proxy
     * @return
     */
    bool configure_curl_handle_for_proxy(CURL *ceh, const string &target_url) {
        BESDEBUG(MODULE, prolog << "BEGIN." << endl);

        bool using_proxy = http::ProxyConfig::theOne()->is_configured();
        if (using_proxy) {

            BESDEBUG(MODULE, prolog << "Proxy has been configured..." << endl);

            http::ProxyConfig *proxy = http::ProxyConfig::theOne();

            // TODO remove these local variables (if possible) and pass the values into curl_easy_setopt() directly from HttpUtils
            string proxyHost = proxy->host();
            int proxyPort = proxy->port();
            string proxyPassword = proxy->proxy_password();
            string proxyUser = proxy->user();
            string proxyUserPW = proxy->password();
            int proxyAuthType = proxy->auth_type();
            string no_proxy_regex = proxy->no_proxy_regex();


            // Don't set up the proxy server for URLs that match the 'NoProxy'
            // regex set in the gateway.conf file.

            // Don't create the regex if the string is empty
            if (!no_proxy_regex.empty()) {
                BESDEBUG(MODULE, prolog << "Found NoProxyRegex." << endl);
                BESRegex r(no_proxy_regex.c_str());
                if (r.match(target_url.c_str(), target_url.length()) != -1) {
                    BESDEBUG(MODULE,
                             prolog << "Found NoProxy match. Regex: " << no_proxy_regex << "; Url: " << target_url
                                    << endl);
                    using_proxy = false;
                }
            }

            if (using_proxy) {
                CURLcode res;
                char error_buffer[CURL_ERROR_SIZE];

                BESDEBUG(MODULE, prolog << "Setting up a proxy server." << endl);
                BESDEBUG(MODULE, prolog << "Proxy host: " << proxyHost << endl);
                BESDEBUG(MODULE, prolog << "Proxy port: " << proxyPort << endl);

                set_error_buffer(ceh, error_buffer);

                res = curl_easy_setopt(ceh, CURLOPT_PROXY, proxyHost.data());
                eval_curl_easy_setopt_result(res, prolog, "CURLOPT_PROXY", error_buffer, __FILE__, __LINE__);

                res = curl_easy_setopt(ceh, CURLOPT_PROXYPORT, proxyPort);
                eval_curl_easy_setopt_result(res, prolog, "CURLOPT_PROXYPORT", error_buffer, __FILE__, __LINE__);

                // oddly "#ifdef CURLOPT_PROXYAUTH" doesn't work - even though CURLOPT_PROXYAUTH is defined and valued at 111 it
                // fails the test. Eclipse hover over the CURLOPT_PROXYAUTH symbol shows: "CINIT(PROXYAUTH, LONG, 111)",
                // for what that's worth

                // According to http://curl.haxx.se/libcurl/c/curl_easy_setopt.html#CURLOPTPROXYAUTH
                // As of 4/21/08 only NTLM, Digest and Basic work.

                res = curl_easy_setopt(ceh, CURLOPT_PROXYAUTH, proxyAuthType);
                eval_curl_easy_setopt_result(res, prolog, "CURLOPT_PROXYAUTH", error_buffer, __FILE__, __LINE__);
                BESDEBUG(MODULE, prolog << "Using CURLOPT_PROXYAUTH = " << getCurlAuthTypeName(proxyAuthType) << endl);

                if (!proxyUser.empty()) {
                    res = curl_easy_setopt(ceh, CURLOPT_PROXYUSERNAME, proxyUser.data());
                    eval_curl_easy_setopt_result(res, prolog, "CURLOPT_PROXYUSERNAME", error_buffer, __FILE__,
                                                 __LINE__);
                    BESDEBUG(MODULE, prolog << "CURLOPT_PROXYUSERNAME : " << proxyUser << endl);

                    if (!proxyPassword.empty()) {
                        res = curl_easy_setopt(ceh, CURLOPT_PROXYPASSWORD, proxyPassword.data());
                        eval_curl_easy_setopt_result(res, prolog, "CURLOPT_PROXYPASSWORD", error_buffer, __FILE__,
                                                     __LINE__);
                        BESDEBUG(MODULE, prolog << "CURLOPT_PROXYPASSWORD: " << proxyPassword << endl);
                    }
                } else if (!proxyUserPW.empty()) {
                    res = curl_easy_setopt(ceh, CURLOPT_PROXYUSERPWD, proxyUserPW.data());
                    eval_curl_easy_setopt_result(res, prolog, "CURLOPT_PROXYUSERPWD", error_buffer, __FILE__, __LINE__);
                    BESDEBUG(MODULE, prolog << "CURLOPT_PROXYUSERPWD : " << proxyUserPW << endl);
                }
                unset_error_buffer(ceh);
            }
        }
        BESDEBUG(MODULE, prolog << "END. using_proxy: " << (using_proxy ? "true" : "false") << endl);
        return using_proxy;
    }

#if 0
        bool configure_curl_handle_for_proxy(CURL *ceh, const string &target_url) {
        BESDEBUG(MODULE, prolog << "BEGIN." << endl);

        bool using_proxy = false;

        http::ProxyConfig *proxy = http::ProxyConfig::TheConfig();

        // TODO remove these local variables (if possible) and pass the values into curl_easy_setopt() directly from HttpUtils
        string proxyHost = proxy->host();
        int proxyPort = proxy->port();
        string proxyPassword = proxy->proxy_password();
        string proxyUser = proxy->user();
        string proxyUserPW = proxy->password();
        int proxyAuthType = proxy->auth_type();
        string no_proxy_regex = proxy->no_proxy_regex();

        if (!proxyHost.empty()) {
            using_proxy = true;
            if (proxyPort == 0)
                proxyPort = 8080;

            // Apparently we don't need this...
            //if(proxyProtocol.empty())
            // proxyProtocol = "http";

        }
        if (using_proxy) {
            BESDEBUG(MODULE, prolog << "Found proxy configuration." << endl);

            // Don't set up the proxy server for URLs that match the 'NoProxy'
            // regex set in the gateway.conf file.

            // Don't create the regex if the string is empty
            if (!no_proxy_regex.empty()) {
                BESDEBUG(MODULE, prolog << "Found NoProxyRegex." << endl);
                BESRegex r(no_proxy_regex.c_str());
                if (r.match(target_url.c_str(), target_url.length()) != -1) {
                    BESDEBUG(MODULE,
                             prolog << "Found NoProxy match. Regex: " << no_proxy_regex << "; Url: " << target_url
                                    << endl);
                    using_proxy = false;
                }
            }

            if (using_proxy) {
                CURLcode res;
                char error_buffer[CURL_ERROR_SIZE];

                BESDEBUG(MODULE, prolog << "Setting up a proxy server." << endl);
                BESDEBUG(MODULE, prolog << "Proxy host: " << proxyHost << endl);
                BESDEBUG(MODULE, prolog << "Proxy port: " << proxyPort << endl);

                set_error_buffer(ceh, error_buffer);

                res = curl_easy_setopt(ceh, CURLOPT_PROXY, proxyHost.data());
                eval_curl_easy_setopt_result(res, prolog, "CURLOPT_PROXY", error_buffer, __FILE__, __LINE__);

                res = curl_easy_setopt(ceh, CURLOPT_PROXYPORT, proxyPort);
                eval_curl_easy_setopt_result(res, prolog, "CURLOPT_PROXYPORT", error_buffer, __FILE__, __LINE__);

                // oddly "#ifdef CURLOPT_PROXYAUTH" doesn't work - even though CURLOPT_PROXYAUTH is defined and valued at 111 it
                // fails the test. Eclipse hover over the CURLOPT_PROXYAUTH symbol shows: "CINIT(PROXYAUTH, LONG, 111)",
                // for what that's worth

                // According to http://curl.haxx.se/libcurl/c/curl_easy_setopt.html#CURLOPTPROXYAUTH
                // As of 4/21/08 only NTLM, Digest and Basic work.

                res = curl_easy_setopt(ceh, CURLOPT_PROXYAUTH, proxyAuthType);
                eval_curl_easy_setopt_result(res, prolog, "CURLOPT_PROXYAUTH", error_buffer, __FILE__, __LINE__);
                BESDEBUG(MODULE, prolog << "Using CURLOPT_PROXYAUTH = " << getCurlAuthTypeName(proxyAuthType) << endl);

                if (!proxyUser.empty()) {
                    res = curl_easy_setopt(ceh, CURLOPT_PROXYUSERNAME, proxyUser.data());
                    eval_curl_easy_setopt_result(res, prolog, "CURLOPT_PROXYUSERNAME", error_buffer, __FILE__, __LINE__);
                    BESDEBUG(MODULE, prolog << "CURLOPT_PROXYUSERNAME : " << proxyUser << endl);

                    if (!proxyPassword.empty()) {
                        res = curl_easy_setopt(ceh, CURLOPT_PROXYPASSWORD, proxyPassword.data());
                        eval_curl_easy_setopt_result(res, prolog, "CURLOPT_PROXYPASSWORD", error_buffer, __FILE__,
                                                     __LINE__);
                        BESDEBUG(MODULE, prolog << "CURLOPT_PROXYPASSWORD: " << proxyPassword << endl);
                    }
                }
                else if (!proxyUserPW.empty()) {
                    res = curl_easy_setopt(ceh, CURLOPT_PROXYUSERPWD, proxyUserPW.data());
                    eval_curl_easy_setopt_result(res, prolog, "CURLOPT_PROXYUSERPWD", error_buffer, __FILE__, __LINE__);
                    BESDEBUG(MODULE, prolog << "CURLOPT_PROXYUSERPWD : " << proxyUserPW << endl);
                }
                unset_error_buffer(ceh);
            }
        }
        BESDEBUG(MODULE, prolog << "END." << endl);

        return using_proxy;
    }
#endif



CURL *init(const string &target_url,
           const struct curl_slist *http_request_headers,
           vector<string> *http_response_hdrs) {
    CURL *swanky_new_curl_easy_handle = curl_easy_init();
    return init(swanky_new_curl_easy_handle, target_url, http_request_headers, http_response_hdrs);
}

/**
 * Get's a new instance of a cURL easy handle (CURL*) and performs
 * a basic configuration of that instance.
 *  - Accept compressed responses
 *  - Any authentication type
 *  - Follow redirects
 *  - User agent set to value of hyrax_user_agent().
 *
 *
 * @param target_url
 * @param http_request_headers
 * @param http_response_hdrs
 * @return
 */
CURL *init(CURL *ceh,
           const string &target_url,
           const struct curl_slist *http_request_headers,
           vector<string> *http_response_hdrs
) {
    char error_buffer[CURL_ERROR_SIZE];
    error_buffer[0] = 0; // Null terminate this string for safety.
    CURLcode res;

    if (!ceh)
        throw BESInternalError("Could not initialize cURL easy handle.", __FILE__, __LINE__);

    // SET Error Buffer (for use during this setup) ----------------------------------------------------------------
    set_error_buffer(ceh, error_buffer);

    // Target URL --------------------------------------------------------------------------------------------------
    res = curl_easy_setopt(ceh, CURLOPT_URL, target_url.c_str());
    eval_curl_easy_setopt_result(res, prolog, "CURLOPT_URL", error_buffer, __FILE__, __LINE__);

    // Load in the default headers to send with a request. The empty Pragma
    // headers overrides libcurl's default Pragma: no-cache header (which
    // will disable caching by Squid, etc.).
    // the empty Pragma never appears in the outgoing headers when this isn't present
    // d_request_headers->push_back(string("Pragma: no-cache"));
    // d_request_headers->push_back(string("Cache-Control: no-cache"));

    //TODO Do we need this test? what if the pointer is null? Probably it's fine...
    if (http_request_headers) {
        // Add the http_request_headers to the cURL handle.
        res = curl_easy_setopt(ceh, CURLOPT_HTTPHEADER, http_request_headers);
        eval_curl_easy_setopt_result(res, prolog, "CURLOPT_HTTPHEADER", error_buffer, __FILE__, __LINE__);
    }


    if (http_response_hdrs) {
        res = curl_easy_setopt(ceh, CURLOPT_HEADERFUNCTION, save_http_response_headers);
        eval_curl_easy_setopt_result(res, prolog, "CURLOPT_HEADERFUNCTION", error_buffer, __FILE__, __LINE__);

        // Pass save_http_response_headers() a pointer to the vector<string> where the
        // response headers may be stored. Callers can use the resp_hdrs
        // value/result parameter to get the raw response header information .
        res = curl_easy_setopt(ceh, CURLOPT_WRITEHEADER, http_response_hdrs);
        eval_curl_easy_setopt_result(res, prolog, "CURLOPT_WRITEHEADER", error_buffer, __FILE__, __LINE__);
    }

    // Allow compressed responses. Sending an empty string enables all supported compression types.
#ifndef CURLOPT_ACCEPT_ENCODING
    res = curl_easy_setopt(ceh, CURLOPT_ENCODING, "");
    eval_curl_easy_setopt_result(res, prolog, "CURLOPT_ENCODING", error_buffer, __FILE__, __LINE__);
#else
    res = curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "");
    check_setopt_result(res, prolog, "CURLOPT_ACCEPT_ENCODING", error_buffer, __FILE__,__LINE__);
#endif
    // Disable Progress Meter
    res = curl_easy_setopt(ceh, CURLOPT_NOPROGRESS, 1L);
    eval_curl_easy_setopt_result(res, prolog, "CURLOPT_NOPROGRESS", error_buffer, __FILE__, __LINE__);

    // Disable cURL signal handling
    res = curl_easy_setopt(ceh, CURLOPT_NOSIGNAL, 1L);
    eval_curl_easy_setopt_result(res, prolog, "CURLOPT_NOSIGNAL", error_buffer, __FILE__, __LINE__);


    // -  -  -  - -  -  -  - -  -  -  - -  -  -  - -  -  -  - -  -  -  - -  -  -  -
    // Authentication config.
    //

    // We have to set FailOnError to false for any of the non-Basic
    // authentication schemes to work. 07/28/03 jhrg
    res = curl_easy_setopt(ceh, CURLOPT_FAILONERROR, 0L);
    eval_curl_easy_setopt_result(res, prolog, "CURLOPT_FAILONERROR", error_buffer, __FILE__, __LINE__);


    // CURLAUTH_ANY means libcurl will use Basic, Digest, GSS Negotiate, or NTLM,
    // choosing the the 'safest' one supported by the server.
    // This requires curl 7.10.6 which is still in pre-release. 07/25/03 jhrg
    res = curl_easy_setopt(ceh, CURLOPT_HTTPAUTH, (long) CURLAUTH_ANY);
    eval_curl_easy_setopt_result(res, prolog, "CURLOPT_HTTPAUTH", error_buffer, __FILE__, __LINE__);


    // CURLOPT_NETRC means to use the netrc file for credentials.
    // CURL_NETRC_OPTIONAL Means that if the supplied URL contains a username
    // and password to prefer that to using the content of the netrc file.
    res = curl_easy_setopt(ceh, CURLOPT_NETRC, CURL_NETRC_OPTIONAL);
    eval_curl_easy_setopt_result(res, prolog, "CURLOPT_NETRC", error_buffer, __FILE__, __LINE__);

    // If the configuration specifies a particular .netrc credentials file, use it.
    string netrc_file = get_netrc_filename();
    if (!netrc_file.empty()) {
        res = curl_easy_setopt(ceh, CURLOPT_NETRC_FILE, netrc_file.c_str());
        eval_curl_easy_setopt_result(res, prolog, "CURLOPT_NETRC_FILE", error_buffer, __FILE__, __LINE__);

    }
    VERBOSE(prolog << " is using the netrc file '"
                   << ((!netrc_file.empty()) ? netrc_file : "~/.netrc") << "'" << endl);


    // -  -  -  - -  -  -  - -  -  -  - -  -  -  - -  -  -  - -  -  -  - -  -  -  -
    // Cookies
    //
    res = curl_easy_setopt(ceh, CURLOPT_COOKIEFILE, curl::get_cookie_filename().c_str());
    eval_curl_easy_setopt_result(res, prolog, "CURLOPT_COOKIEFILE", error_buffer, __FILE__, __LINE__);

    res = curl_easy_setopt(ceh, CURLOPT_COOKIEJAR, curl::get_cookie_filename().c_str());
    eval_curl_easy_setopt_result(res, prolog, "CURLOPT_COOKIEJAR", error_buffer, __FILE__, __LINE__);

    // save_http_response_headers

    // Follow 302 (redirect) responses
    res = curl_easy_setopt(ceh, CURLOPT_FOLLOWLOCATION, 1L);
    eval_curl_easy_setopt_result(res, prolog, "CURLOPT_FOLLOWLOCATION", error_buffer, __FILE__, __LINE__);

    res = curl_easy_setopt(ceh, CURLOPT_MAXREDIRS, max_redirects());
    eval_curl_easy_setopt_result(res, prolog, "CURLOPT_MAXREDIRS", error_buffer, __FILE__, __LINE__);

    // Set the user agent to Hyrax's user agent value
    res = curl_easy_setopt(ceh, CURLOPT_USERAGENT, hyrax_user_agent().c_str());
    eval_curl_easy_setopt_result(res, prolog, "CURLOPT_USERAGENT", error_buffer, __FILE__, __LINE__);

#if 0
    // If the user turns off SSL validation...
if (!d_rcr->get_validate_ssl() == 0) {
    res = curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
    check_setopt_result(res, prolog, "CURLOPT_SSL_VERIFYPEER", error_buffer, __FILE__, __LINE__);
    res = curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);
    check_setopt_result(res, prolog, "CURLOPT_SSL_VERIFYHOST", error_buffer, __FILE__, __LINE__);
}
#endif

    if (curl_trace) {
        BESDEBUG(MODULE, prolog << "Curl version: " << curl_version() << endl);
        res = curl_easy_setopt(ceh, CURLOPT_VERBOSE, 1L);
        eval_curl_easy_setopt_result(res, prolog, "CURLOPT_VERBOSE", error_buffer, __FILE__, __LINE__);
        BESDEBUG(MODULE, prolog << "Curl in verbose mode." << endl);

        res = curl_easy_setopt(ceh, CURLOPT_DEBUGFUNCTION, curl_debug);
        eval_curl_easy_setopt_result(res, prolog, "CURLOPT_DEBUGFUNCTION", error_buffer, __FILE__, __LINE__);
        BESDEBUG(MODULE, prolog << "Curl debugging function installed." << endl);
    }

    // We unset the error buffer here because we know that curl::configure_curl_handle_for_proxy() will use it's own.
    unset_error_buffer(ceh);
    // Configure the a proxy for this url (if appropriate).
    curl::configure_curl_handle_for_proxy(ceh, target_url);

    BESDEBUG(MODULE, prolog << "curl: " << (void *) ceh << endl);
    return ceh;
}

string get_range_arg_string(const unsigned long long &offset, const unsigned long long &size) {
    ostringstream range;   // range-get needs a string arg for the range
    range << offset << "-" << offset + size - 1;
    BESDEBUG(MODULE, prolog << " range: " << range.str() << endl);
    return range.str();
}

/**
 * @brief Returns an cURL easy handle for tracing redirects.
 *
 * The returned cURL easy handle is configured to make a 4 byte
 * range get from the url. When theis cURL handle is "exercised"
 * at the end the cURL handles CURLINFO_EFFECTIVE_URL value will
 * be the place from which the 4 bytes were retrieved, the
 * terminus if the redirect sequence.
 * @param target_url The URL to target
 * @param req_headers A curl_slist containing any necessary request headers
 * to be transmitted with the HTTP request.
 * @param resp_hdrs A vector into which any response headeres associated
 * the servers response will be placed.
 * @return A cURL easy handle configured as described above,
 */
CURL *init_effective_url_retriever_handle(const string &target_url, struct curl_slist *req_headers,
                                          vector<string> &resp_hdrs) {
    char error_buffer[CURL_ERROR_SIZE];
    CURLcode res;
    CURL *ceh = 0;

    error_buffer[0] = 0; // null terminate empty string

    ceh = curl::init(target_url, req_headers, &resp_hdrs);

    set_error_buffer(ceh, error_buffer);

    // get the offset to offset + size bytes
    res = curl_easy_setopt(ceh, CURLOPT_RANGE, get_range_arg_string(0, 4).c_str());
    eval_curl_easy_setopt_result(res, prolog, "CURLOPT_RANGE", error_buffer, __FILE__, __LINE__);

    res = curl_easy_setopt(ceh, CURLOPT_WRITEFUNCTION, writeNothing);
    eval_curl_easy_setopt_result(res, prolog, "CURLOPT_WRITEFUNCTION", error_buffer, __FILE__, __LINE__);

    // Pass save_raw_http_headers() a pointer to the vector<string> where the
    // response headers may be stored. Callers can use the resp_hdrs
    // value/result parameter to get the raw response header information .
    res = curl_easy_setopt(ceh, CURLOPT_WRITEHEADER, &resp_hdrs);
    eval_curl_easy_setopt_result(res, prolog, "CURLOPT_WRITEHEADER", error_buffer, __FILE__, __LINE__);

    unset_error_buffer(ceh);

    return ceh;
}

/**
 *
 * Use libcurl to dereference a URL. Read the information referenced by
 * url into the file pointed to by the open file descriptor fd.
 *
 * @param target_url The URL to dereference.
 * @param fd  An open file descriptor (as in 'open' as opposed to 'fopen') which
 * will be the destination for the data; the caller can assume that when this
 * method returns that the body of the response can be retrieved by reading
 * from this file descriptor.
 * @param http_response_headers Value/result parameter for the HTTP Response Headers.
 * @param http_request_headers A pointer to a vector of HTTP request headers. Default is
 * null. These headers will be appended to the list of default headers.
 * @return The HTTP status code.
 * @exception Error Thrown if libcurl encounters a problem; the libcurl
 * error message is stuffed into the Error object.
 */
void http_get_and_write_resource(const std::shared_ptr<http::url>& target_url,
                                 const int fd,
                                 vector<string> *http_response_headers) {

    char error_buffer[CURL_ERROR_SIZE];
    CURLcode res;
    CURL *ceh = nullptr;
    curl_slist *req_headers = nullptr;
    BuildHeaders header_builder;

    BESDEBUG(MODULE, prolog << "BEGIN" << endl);
    // Before we do anything, make sure that the URL is OK to pursue.
    if (!http::AllowedHosts::theHosts()->is_allowed(target_url)) {
        string err = (string) "The specified URL " + target_url->str()
                     + " does not match any of the accessible services in"
                     + " the allowed hosts list.";
        BESDEBUG(MODULE, prolog << err << endl);
        throw BESSyntaxUserError(err, __FILE__, __LINE__);
    }

    // Add the authorization headers
    req_headers = add_edl_auth_headers(req_headers);

    try {
        // OK! Make the cURL handle
        ceh = init(target_url->str(), req_headers, http_response_headers);

        set_error_buffer(ceh, error_buffer);

        res = curl_easy_setopt(ceh, CURLOPT_WRITEFUNCTION, writeToOpenFileDescriptor);
        eval_curl_easy_setopt_result(res, prolog, "CURLOPT_WRITEFUNCTION", error_buffer, __FILE__, __LINE__);

#ifdef CURLOPT_WRITEDATA
        res = curl_easy_setopt(ceh, CURLOPT_WRITEDATA, &fd);
        eval_curl_easy_setopt_result(res, prolog, "CURLOPT_WRITEDATA", error_buffer, __FILE__, __LINE__);
#else
        res = curl_easy_setopt(ceh, CURLOPT_FILE, &fd);
        eval_curl_easy_setopt_result(res, prolog, "CURLOPT_FILE", error_buffer, __FILE__, __LINE__);
#endif
        unset_error_buffer(ceh);

        super_easy_perform(ceh);

        // Free the header list
        if (req_headers)
            curl_slist_free_all(req_headers);
        if (ceh)
            curl_easy_cleanup(ceh);
        BESDEBUG(MODULE, prolog << "Called curl_easy_cleanup()." << endl);
    }
    catch (...) {
        if (req_headers)
            curl_slist_free_all(req_headers);
        if (ceh)
            curl_easy_cleanup(ceh);
        throw;
    }
    BESDEBUG(MODULE, prolog << "END" << endl);
}

/**
 * Returns a cURL error message string based on the conents of the error_buf or, if the error_buf is empty, the
 * CURLcode code.
 * @param response_code
 * @param error_buf
 * @return
 */
string error_message(const CURLcode response_code, char *error_buffer) {
    std::ostringstream oss;
    size_t len = strlen(error_buffer);
    if (len) {
        oss << "cURL_error_buffer: '" << error_buffer;
    }
    oss << "' cURL_message: '" << curl_easy_strerror(response_code);
    oss << "' (code: " << (int) response_code << ")";
    return oss.str();
}

/*
* @brief Callback passed to libcurl to handle reading a single byte.
*
* This callback assumes that the size of the data is small enough
* that all of the bytes will be either read at once or that a local
        * temporary buffer can be used to build up the values.
*
* @param buffer Data from libcurl
* @param size Number of bytes
* @param nmemb Total size of data in this call is 'size * nmemb'
* @param data Pointer to this
* @return The number of bytes read
*/
size_t c_write_data(void *buffer, size_t size, size_t nmemb, void *data) {
    size_t nbytes = size * nmemb;
    //cerr << "ngap_write_data() bytes: " << nbytes << "  size: " << size << "  nmemb: " << nmemb << " buffer: " << buffer << "  data: " << data << endl;
    memcpy(data, buffer, nbytes);
    return nbytes;
}

/**
 * @brief http_get_as_string() This function de-references the target_url and returns the response document as a std:string.
 *
 * @param target_url The URL to dereference.
 * @return JSON document parsed from the response document returned by target_url
 */
std::string http_get_as_string(const std::string &target_url) {

    // @TODO @FIXME Make the size of this buffer one of:
    //              a) A configuration setting.
    //              b) A new parameter to the function. (unsigned long)
    //              c) Do a HEAD on the URL, check for the Content-Length header and plan accordingly.
    //
    char response_buf[1024 * 1024];

    http_get(target_url, response_buf);
    string response(response_buf);
    return response;
}

/**
 * @brief http_get_as_json() This function de-references the target_url and parses the response into a JSON document.
 * No attempt to cache is performed, the HTTP request is made for each invocation of this method.
 *
 * @param target_url The URL to dereference.
 * @return JSON document parsed from the response document returned by target_url
 */
rapidjson::Document http_get_as_json(const std::string &target_url) {

    // @TODO @FIXME Make the size of this buffer one of:
    //              a) A configuration setting.
    //              b) A new parameter to the function. (unsigned long)
    //              c) Do a HEAD on the URL, check for the Content-Length header and plan accordingly.
    //

    char response_buf[1024 * 1024];

    curl::http_get(target_url, response_buf);
    rapidjson::Document d;
    d.Parse(response_buf);
    return d;
}

/**
 * Derefernce the target URL and put the response in response_buf
 * @param target_url The URL to dereference.
 * @param response_buf The buffer into which to put the response.
 */
void http_get(const std::string &target_url, char *response_buf) {

    char errbuf[CURL_ERROR_SIZE]; ///< raw error message info from libcurl
    CURL *ceh = nullptr;     ///< The libcurl handle object.
    CURLcode res;

    curl_slist *request_headers = nullptr;
    // Add the authorization headers
    request_headers = add_edl_auth_headers(request_headers);

    try {

        ceh = curl::init(target_url, request_headers, nullptr);
        if (!ceh)
            throw BESInternalError(string("ERROR! Failed to acquire cURL Easy Handle! "), __FILE__, __LINE__);

        // Error Buffer (for use during this setup) ----------------------------------------------------------------
        set_error_buffer(ceh, errbuf);

        // Pass all data to the 'write_data' function --------------------------------------------------------------
        res = curl_easy_setopt(ceh, CURLOPT_WRITEFUNCTION, c_write_data);
        eval_curl_easy_setopt_result(res, prolog, "CURLOPT_WRITEFUNCTION", errbuf, __FILE__, __LINE__);

        // Pass this to write_data as the fourth argument ----------------------------------------------------------
        res = curl_easy_setopt(ceh, CURLOPT_WRITEDATA, reinterpret_cast<void *>(response_buf));
        eval_curl_easy_setopt_result(res, prolog, "CURLOPT_WRITEDATA", errbuf, __FILE__, __LINE__);

        unset_error_buffer(ceh);

        super_easy_perform(ceh);

        if (request_headers)
            curl_slist_free_all(request_headers);
        if (ceh)
            curl_easy_cleanup(ceh);
    }
    catch (...) {
        if (request_headers)
            curl_slist_free_all(request_headers);
        if (ceh)
            curl_easy_cleanup(ceh);
    }
}

#if 0
/**
 *
 * @param target_url
 * @param cookies_file
 * @param response_buff
 * @return
 */
CURL *set_up_easy_handle(const string &target_url, struct curl_slist *request_headers, char *response_buff) {
    char errbuf[CURL_ERROR_SIZE]; ///< raw error message info from libcurl
    CURL *d_handle;     ///< The libcurl handle object.
    CURLcode res;

    d_handle = curl::init(target_url,request_headers,NULL);
    if (!d_handle)
        throw BESInternalError(string("ERROR! Failed to acquire cURL Easy Handle! "), __FILE__, __LINE__);

    // Error Buffer (for use during this setup) --------------------------------------------------------------------
    set_error_buffer(d_handle,errbuf);

    // Pass all data to the 'write_data' function ------------------------------------------------------------------
    res = curl_easy_setopt(d_handle, CURLOPT_WRITEFUNCTION, c_write_data);
    check_setopt_result(res, prolog, "CURLOPT_WRITEFUNCTION", errbuf, __FILE__, __LINE__);

    // Pass this to write_data as the fourth argument --------------------------------------------------------------
    res = curl_easy_setopt(d_handle, CURLOPT_WRITEDATA, reinterpret_cast<void *>(response_buff));
    check_setopt_result(res, prolog, "CURLOPT_WRITEDATA", errbuf, __FILE__, __LINE__);

#if 0
    // handled by curl::init() - SBL 9.10.20
    // Follow redirects --------------------------------------------------------------------------------------------
    res = curl_easy_setopt(d_handle, CURLOPT_FOLLOWLOCATION, 1L);
    check_setopt_result(res, prolog, "CURLOPT_FOLLOWLOCATION", errbuf, __FILE__, __LINE__);

    // Use cookies -------------------------------------------------------------------------------------------------
    res = curl_easy_setopt(d_handle, CURLOPT_COOKIEFILE, cookies_file.c_str());
    check_setopt_result(res, prolog, "CURLOPT_COOKIEFILE", errbuf, __FILE__, __LINE__);

    res = curl_easy_setopt(d_handle, CURLOPT_COOKIEJAR, cookies_file.c_str());
    check_setopt_result(res, prolog, "CURLOPT_COOKIEJAR", errbuf, __FILE__, __LINE__);

    // Authenticate using best available ---------------------------------------------------------------------------
    res = curl_easy_setopt(d_handle, CURLOPT_HTTPAUTH, (long) CURLAUTH_ANY);
    check_setopt_result(res, prolog, "CURLOPT_HTTPAUTH", errbuf, __FILE__, __LINE__);

    // Use .netrc for credentials ----------------------------------------------------------------------------------
    res = curl_easy_setopt(d_handle, CURLOPT_NETRC, CURL_NETRC_OPTIONAL);
    check_setopt_result(res, prolog, "CURLOPT_NETRC", errbuf, __FILE__, __LINE__);

    // If the configuration specifies a particular .netrc credentials file, use it. --------------------------------
    string netrc_file = get_netrc_filename();
    if (!netrc_file.empty()) {
        res = curl_easy_setopt(d_handle, CURLOPT_NETRC_FILE, netrc_file.c_str());
        check_setopt_result(res, prolog, "CURLOPT_NETRC_FILE", errbuf, __FILE__, __LINE__);
    }

    VERBOSE(__FILE__ << "::get_easy_handle() is using the netrc file '"
                     << ((!netrc_file.empty()) ? netrc_file : "~/.netrc") << "'" << endl);
#endif

    unset_error_buffer(d_handle);

    return d_handle;
}
#endif

/**
 * @brief Performs a curl_easy_perform(), retrying if certain types of errors are encountered.
 *
 * This function contains the operational frame work and state checking for performing retries of
 * failed requests as necessary.
 *
 * The code that contains the state assessment is held in the functions
 * - curl::eval_curl_easy_perform_code()
 * - curl::eval_http_get_response()
 *
 * These functions have a tri-state behavior:
 *   - If the assessed operation was a success they return true.
 *   - If the assessed operation had what is considered a retryable failure, they return false.
 *   - If the assessed operation had any other failure, a BESInternalError is thrown.
 * These functions are used in the retry logic of this curl::super_easy_perform() to
 * determine when there was success, when to keep trying, and if to give up.
 *
 * @param c_handle The CURL easy handle on which to operate
 */
void super_easy_perform(CURL *c_handle) {
    unsigned int attempts = 0;
    useconds_t retry_time = uone_second / 4;
    bool success;
    CURLcode curl_code;
    char curlErrorBuf[CURL_ERROR_SIZE]; ///< raw error message info from libcurl
    string target_url;

    string empty_str;
    target_url = get_effective_url(c_handle, empty_str);
    // We check the value of target_url to see if the URL was correctly set in the cURL handle.
    if (target_url.empty())
        throw BESInternalError("URL acquisition failed.", __FILE__, __LINE__);

    // SET Error Buffer --------------------------------------------------------------------------------------------
    set_error_buffer(c_handle, curlErrorBuf);
    do {
        curlErrorBuf[0] = 0; // Initialize to empty string
        ++attempts;
        BESDEBUG(MODULE, prolog << "Requesting URL: " << target_url << " attempt: " << attempts << endl);

        curl_code = curl_easy_perform(c_handle);
        success = eval_curl_easy_perform_code(c_handle, target_url, curl_code, curlErrorBuf, attempts);
        if (success) {
            // Nothing obvious went wrong with the curl_easy_perform() so now we check the HTTP stuff
            success = eval_http_get_response(c_handle, curlErrorBuf, target_url);
        }
        // If the curl_easy_perform failed, or if the http request failed then
        // we keep trying until we have exceeded the retry_limit.
        if (!success) {
            if (attempts == retry_limit) {
                string msg = prolog + "ERROR - Problem with data transfer. Number of re-tries exceeded. Giving up.";
                ERROR_LOG(msg << endl);
                throw BESInternalError(msg, __FILE__, __LINE__);
            }
            else {
                ERROR_LOG(prolog << "ERROR - Problem with data transfer. Will retry (url: " << target_url <<
                           " attempt: " << attempts << ")." << endl);
                usleep(retry_time);
                retry_time *= 2;
            }
        }
    } while (!success);
    // Unset the buffer as it goes out of scope
    unset_error_buffer(c_handle);
}

#if 0

/**
* Execute the HTTP VERB from the passed cURL handle "c_handle" and retrieve the response.
* @param c_handle The cURL easy handle to execute and read.
*/
void read_data(CURL *c_handle) {

    unsigned int attempts = 0;
    useconds_t retry_time = uone_second / 4;
    bool success;
    CURLcode curl_code;
    char curlErrorBuf[CURL_ERROR_SIZE]; ///< raw error message info from libcurl
    char *urlp = NULL;

    curl_easy_getinfo(c_handle, CURLINFO_EFFECTIVE_URL, &urlp);
    // Checking the curl_easy_getinfo return value in this case is pointless. If it's CURLE_OK then we
    // still have to check the value of urlp to see if the URL was correctly set in the
    // cURL handle. If it fails then it fails, and urlp is not set. If we just focus on the value of urlp then
    // we can just check the one thing.
    if (!urlp)
        throw BESInternalError("URL acquisition failed.", __FILE__, __LINE__);

    // SET Error Buffer --------------------------------------------------------------------------------------------
    set_error_buffer(c_handle, curlErrorBuf);
    do {
        // bool do_retry;
        curlErrorBuf[0]=0; // Initialize to empty string
        ++attempts;
        BESDEBUG(MODULE, prolog << "Requesting URL: " << urlp << " attempt: " << attempts <<  endl);

        curl_code = curl_easy_perform(c_handle);
        success = eval_curl_easy_perform_code(c_handle, urlp, curl_code, curlErrorBuf, attempts);
        if(success){
            // Nothing obvious went wrong with the curl_easy_perfom() so now we check the HTTP stuff
            success = eval_http_get_response(c_handle, urlp);
        }
        // If the curl_easy_perform failed, or if the http request failed then
        // we keep trying until we have exceeded the retry_limit.
        if (!success) {
            if (attempts == retry_limit) {
                string msg = prolog + "ERROR - Problem with data transfer. Number of re-tries exceeded. Giving up.";
                LOG(msg << endl);
                throw BESInternalError(msg, __FILE__, __LINE__);
            }
            else {
                LOG(prolog << "ERROR - Problem with data transfer. Will retry (url: " << urlp <<
                           " attempt: " << attempts << ")." << endl);
                usleep(retry_time);
                retry_time *= 2;
            }
        }
    } while (!success);

#if 0
    // Try until retry_limit or success...
    do {
        curlErrorBuf[0] = 0; // clear the error buffer with a null termination at index 0.
        curl_code = curl_easy_perform(c_handle); // Do the thing...
        ++tries;

        if (CURLE_OK != curl_code) { // Failure here is not an HTTP error, but a cURL error.
            throw BESInternalError(
                    string("read_data() - ERROR! Message: ").append(error_message(curl_code, curlErrorBuf)),
                    __FILE__, __LINE__);
        }

        success = eval_get_response(c_handle, urlp);
        // if(debug) cout << ngap_curl::probe_easy_handle(c_handle) << endl;
        if (!success) {
            if (tries == retry_limit) {
                string msg = prolog + "Data transfer error: Number of re-tries exceeded: "+ error_message(curl_code, curlErrorBuf);
                LOG(msg << endl);
                throw BESInternalError(msg, __FILE__, __LINE__);
            }
            else {
                if (BESDebug::IsSet(MODULE)) {
                    stringstream ss;
                    ss << "HTTP transfer 500 error, will retry (trial " << tries << " for: " << urlp << ").";
                    BESDEBUG(MODULE, ss.str());
                }
                usleep(retry_time);
                retry_time *= 2;
            }
        }

    } while (!success);
#endif
    unset_error_buffer(c_handle);
}
#endif

string get_cookie_file_base() {
    bool found = false;
    string cookie_filename;
    TheBESKeys::TheKeys()->get_value(HTTP_COOKIES_FILE_KEY, cookie_filename, found);
    if (!found) {
        cookie_filename = HTTP_DEFAULT_COOKIES_FILE;
    }
    return cookie_filename;
}

string get_cookie_filename() {
    string cookie_file_base = get_cookie_file_base();
    stringstream cf_with_pid;
    cf_with_pid << cookie_file_base << "-" << getpid();
    return cf_with_pid.str();
}

void clear_cookies() {
    string cf = get_cookie_filename();
    int ret = unlink(cf.c_str());
    if (ret) {
        string msg = prolog + "Failed to unlink the cookie file: " + cf;
        ERROR_LOG(msg << endl);
        BESDEBUG(MODULE, prolog << msg << endl);
    }
}


/**
 * Checks to see if the entire url matches any of the  "no retry" regular expressions held in the TheBESKeys
 * under the HTTP_NO_RETRY_URL_REGEX_KEY which atm, is set to "Http.No.Retry.Regex"
 * @param target_url The URL to be examined
 * @return True if the target_url does not match a no retry regex, false if the entire target_url matches
 * a "no retry" regex.
 */
bool is_retryable(std::string target_url) {
    BESDEBUG(MODULE, prolog << "BEGIN" << endl);
    bool retryable = true;

    vector<string> nr_regexs;
    bool found;
    TheBESKeys::TheKeys()->get_values(HTTP_NO_RETRY_URL_REGEX_KEY, nr_regexs, found);
    if (found) {
        vector<string>::iterator it;
        for (it = nr_regexs.begin(); it != nr_regexs.end() && retryable; it++) {
            BESRegex no_retry_regex((*it).c_str(), (*it).size());
            size_t match_length;
            match_length = no_retry_regex.match(target_url.c_str(), target_url.size(), 0);
            if (match_length == target_url.size()) {
                BESDEBUG(MODULE, prolog << "The url: '" << target_url << "' fully matched the "
                                        << HTTP_NO_RETRY_URL_REGEX_KEY << ": '" << *it << "'" << endl);
                retryable = false;
            }
        }
    }
    BESDEBUG(MODULE, prolog << "END retryable: " << (retryable ? "true" : "false") << endl);
    return retryable;
}

/**
 * @brief Evaluates the HTTP semantics of a the result of issuing a cURL GET request.
 *
 * This code requires that:
 * - curl_easy_perform() has been called on the handle ceh.
 * - The returned CURLcode value was CURLE_OK.
 * This code operates under the assumption that the world is not perfect and that things
 * fail, sometimes for no good reason, and trying the thing a again may be worth while.
 *
 * With that in mind, if:
 *
 *    CURLINFO_RESPONSE_CODE returns CURLE_GOT_NOTHING
 *
 * or if the HTTP response code was one of:
 *
 *   case 422: // Unprocessable Entity
 *   case 500: // Internal server error
 *   case 502: // Bad Gateway
 *   case 503: // Service Unavailable
 *   case 504: // Gateway Timeout
 *
 * This function will return false, indicating that there was a problem, but a retry
 * might be reasonable.
 *
 * If another cURL error or different HTTP response error code is encountered a
 * BESInternalError is thrown.
 *
 * This function returns true if the CURLINFO_RESPONSE_CODE response code is 200 (OK) or
 * 206 (Partial Content)
 *
 * @param ceh The cURL easy_handle to evaluate.
 * @return true if at all worked out, false if it didn't and a retry is reasonable.
 * @throws BESInternalError When something really bad happens.
*/
bool eval_http_get_response(CURL *ceh, char *error_buffer, const string &requested_url) {
    BESDEBUG(MODULE, prolog << "Requested URL: " << requested_url << endl);
    CURLcode curl_code;
    string last_accessed_url = get_effective_url(ceh, requested_url);
    BESDEBUG(MODULE, prolog << "Last Accessed URL(CURLINFO_EFFECTIVE_URL): " << last_accessed_url << endl);

    long http_code = 0;

    curl_code = curl_easy_getinfo(ceh, CURLINFO_RESPONSE_CODE, &http_code);
    if (curl_code == CURLE_GOT_NOTHING) {
        // First we check to see if the response was empty. This is a cURL error, not an HTTP error
        // so we have to handle it like this. And we do that because this is one of the failure modes
        // we see in the AWS cloud and by trapping this and returning false we are able to be resilient and retry.
        stringstream msg;
        msg << prolog << "ERROR - cURL returned CURLE_GOT_NOTHING. Message: '";
        msg << error_message(curl_code, error_buffer) << "' ";
        msg << "CURLINFO_EFFECTIVE_URL: " << last_accessed_url << " ";
        msg << "A retry may be possible for: " << requested_url << ")." << endl;
        BESDEBUG(MODULE, msg.str());
        ERROR_LOG(msg.str());
        return false;
    }
    else if (curl_code != CURLE_OK) {
        // Not an error we are trapping so it's fail time.
        throw BESInternalError(
                string("Error acquiring HTTP response code: ").append(curl::error_message(curl_code, error_buffer)),
                __FILE__, __LINE__);
    }

    if (BESDebug::IsSet(MODULE)) {
        long redirects;
        curl_easy_getinfo(ceh, CURLINFO_REDIRECT_COUNT, &redirects);
        BESDEBUG(MODULE, prolog << "CURLINFO_REDIRECT_COUNT: " << redirects << endl);

        char *redirect_url = NULL;
        curl_easy_getinfo(ceh, CURLINFO_REDIRECT_URL, &redirect_url);
        if (redirect_url)
            BESDEBUG(MODULE, prolog << "CURLINFO_REDIRECT_URL: " << redirect_url << endl);
    }

    stringstream msg;
    if (http_code >= 400) {
        msg << "ERROR - The HTTP GET request for the source URL: " << requested_url << " FAILED. ";
        msg << "CURLINFO_EFFECTIVE_URL: " << last_accessed_url << " ";
        BESDEBUG(MODULE, prolog << msg.str() << endl);
    }
    msg << "The response had an HTTP status of " << http_code;
    msg << " which means '" << http_status_to_string(http_code) << "'";

    // Newer Apache servers return 206 for range requests. jhrg 8/8/18
    switch (http_code) {
        case 0:
        {
            if(requested_url.find(FILE_PROTOCOL)!=0){
                ERROR_LOG(msg.str() << endl);
                throw BESInternalError(msg.str(), __FILE__, __LINE__);
            }
            return true;
        }
        case 200: // OK
        case 206: // Partial content - this is to be expected since we use range gets
            // cases 201-205 are things we should probably reject, unless we add more
            // comprehensive HTTP/S processing here. jhrg 8/8/18
            return true;

        //case 301: // Moved Permanently - but that's ok for now?
        //    return true;

        case 400: // Bad Request
            ERROR_LOG(msg.str() << endl);
            throw BESSyntaxUserError(msg.str(), __FILE__, __LINE__);

        case 401: // Unauthorized
        case 402: // Payment Required
        case 403: // Forbidden
            ERROR_LOG(msg.str() << endl);
            throw BESForbiddenError(msg.str(), __FILE__, __LINE__);

        case 404: // Not Found
            ERROR_LOG(msg.str() << endl);
            throw BESNotFoundError(msg.str(), __FILE__, __LINE__);

        case 408: // Request Timeout
            ERROR_LOG(msg.str() << endl);
            throw BESTimeoutError(msg.str(), __FILE__, __LINE__);

        case 422: // Unprocessable Entity
        case 500: // Internal server error
        case 502: // Bad Gateway
        case 503: // Service Unavailable
        case 504: // Gateway Timeout
        {
            if (!is_retryable(last_accessed_url)) {
                msg << " The semantics of this particular last accessed URL indicate that it should not be retried.";
                ERROR_LOG(msg.str() << endl);
                throw BESInternalError(msg.str(), __FILE__, __LINE__);
            }
            return false;
        }

        default: {
            ERROR_LOG(msg.str() << endl);
            throw BESInternalError(msg.str(), __FILE__, __LINE__);
        }
    }
}


/**
 * This function evaluates the CURLcode value returned by curl_easy_perform.
 *
 * This function assumes that two types of cURL error may be retried:
 *  - CURLE_SSL_CONNECT_ERROR
 *  - CURLE_SSL_CACERT_BADFILE
 *  And for these values of curl_code the fundtion returns false.
 *
 *  The function returns success iff curl_code == CURLE_OK.
 *
 *  If the curl_code is another value a BESInternalError is thrown.
 *
 * @param curl The cURL easy handle used in the request.
 * @param requested_url The requested URL.
 * @param curl_code The CURLcode value to evaluate.
 * @param error_buffer The CURLOPT_ERRORBUFFER used in the request.
 * @param attempt The number of attempts on the url that this request represents.
 * @return True if the curl_easy_perform was successful, false if not but the request may be retried.
 * @throws BESInternalError When the curl_code is an error that should not be retried.
 */
bool eval_curl_easy_perform_code(
        CURL *ceh,
        const string requested_url,
        CURLcode curl_code,
        char *error_buffer,
        const unsigned int attempt
) {
    bool success = true;
    string last_accessed_url = get_effective_url(ceh, requested_url);
    if (curl_code == CURLE_SSL_CONNECT_ERROR) {
        stringstream msg;
        msg << prolog << "ERROR - cURL experienced a CURLE_SSL_CONNECT_ERROR error. Message: '";
        msg << error_message(curl_code, error_buffer) << "' ";
        msg << "CURLINFO_EFFECTIVE_URL: " << last_accessed_url << " ";
        msg << "A retry may be possible for: " << requested_url << " (attempt: " << attempt << ")." << endl;
        BESDEBUG(MODULE, msg.str());
        ERROR_LOG(msg.str());
        success = false;
    }
    else if (curl_code == CURLE_SSL_CACERT_BADFILE) {
        stringstream msg;
        msg << prolog << "ERROR - cURL experienced a CURLE_SSL_CACERT_BADFILE error. Message: '";
        msg << error_message(curl_code, error_buffer) << "' ";
        msg << "CURLINFO_EFFECTIVE_URL: " << last_accessed_url << " ";
        msg << "A retry may be possible for: " << requested_url << " (attempt: " << attempt << ")." << endl;
        BESDEBUG(MODULE, msg.str());
        ERROR_LOG(msg.str());
        success = false;
    }
    else if (curl_code == CURLE_GOT_NOTHING) {
        // First we check to see if the response was empty. This is a cURL error, not an HTTP error
        // so we have to handle it like this. And we do that because this is one of the failure modes
        // we see in the AWS cloud and by trapping this and returning false we are able to be resilient and retry.
        stringstream msg;
        msg << prolog << "ERROR - cURL returned CURLE_GOT_NOTHING. Message: ";
        msg << error_message(curl_code, error_buffer) << "' ";
        msg << "CURLINFO_EFFECTIVE_URL: " << last_accessed_url << " ";
        msg << "A retry may be possible for: " << requested_url << " (attempt: " << attempt << ")." << endl;
        BESDEBUG(MODULE, msg.str());
        ERROR_LOG(msg.str());
        return false;
    }
    else if (CURLE_OK != curl_code) {
        stringstream msg;
        msg << "ERROR - Problem with data transfer. Message: " << error_message(curl_code, error_buffer);
        string effective_url = get_effective_url(ceh, requested_url);
        msg << " CURLINFO_EFFECTIVE_URL: " << effective_url;
        BESDEBUG(MODULE, prolog << msg.str() << endl);
        ERROR_LOG(msg.str() << endl);
        throw BESInternalError(msg.str(), __FILE__, __LINE__);
    }
    return success;
}

#if 0
/**
 * @brief Performs a small (4 byte) range get on the target URL. If successfull the value of  last_accessed_url will
 * be set to the value of the last accessed URL (CURLINFO_EFFECTIVE_URL), including the query string.
 *
 * @param target_url The URL to follow
 * @param last_accessed_url The last accessed URL (CURLINFO_EFFECTIVE_URL), including the query string
 */
    void retrieve_effective_url(const string &target_url, string &last_accessed_url) {
        vector<string> resp_hdrs;
        CURL *ceh = NULL;
        // CURLcode curl_code;
        curl_slist *request_headers = NULL;

        BESDEBUG(MODULE, prolog << "BEGIN" << endl);

        // Add the authorization headers
        request_headers = add_auth_headers(request_headers);

        try {
            BESDEBUG(MODULE,
                     prolog << "BESDebug::IsSet(" << MODULE << "): " << (BESDebug::IsSet(MODULE) ? "true" : "false")
                            << endl);
            BESDEBUG(MODULE, prolog << "BESDebug::IsSet(" << TIMING_LOG_KEY << "): "
                                    << (BESDebug::IsSet(TIMING_LOG_KEY) ? "true" : "false") << endl);
            BESDEBUG(MODULE,
                     prolog << "BESLog::TheLog()->is_verbose(): " << (BESLog::TheLog()->is_verbose() ? "true" : "false")
                            << endl);

            ceh = init_effective_url_retriever_handle(target_url, request_headers, resp_hdrs);

            {
                BESStopWatch sw;
                if (BESDebug::IsSet("euc") || BESDebug::IsSet(MODULE) || BESDebug::IsSet(TIMING_LOG_KEY) ||
                    BESLog::TheLog()->is_verbose()) {
                    sw.start(prolog + " Following Redirects Starting With: " + target_url);
                }
                super_easy_perform(ceh);
            }

            // After doing the thing with super_easy_perform() we retrieve the effective URL form the cURL handle.
            last_accessed_url = get_effective_url(ceh, target_url);
            BESDEBUG(MODULE, prolog << "Last Accessed URL(CURLINFO_EFFECTIVE_URL): " << last_accessed_url << endl);
            INFO_LOG(
                    prolog << "Source URL: '" << target_url << "' CURLINFO_EFFECTIVE_URL: '" << last_accessed_url << "'"
                           << endl);

            if (request_headers)
                curl_slist_free_all(request_headers);
            if (ceh)
                curl_easy_cleanup(ceh);
        }
        catch (...) {
            if (request_headers)
                curl_slist_free_all(request_headers);
            if (ceh)
                curl_easy_cleanup(ceh);
            throw;
        }
    }
#endif
    /**
     * @brief Performs a small (4 byte) range get on the target URL. If successful the value of  returende EffectiveUrl
     * will be set to the value of the last accessed URL (CURLINFO_EFFECTIVE_URL), including the query string and the
     * accumulated response headers from the journey, in the order recieved.
     *
     * @param starting_point_url The URL to follow
     * @return A 'new' EffectiveUrl wrapped in a shared_ptr
     */
    std::shared_ptr<http::EffectiveUrl> retrieve_effective_url(const std::shared_ptr<http::url> &starting_point_url) {

        vector<string> resp_hdrs;
        CURL *ceh = nullptr;
        // CURLcode curl_code;
        curl_slist *request_headers = nullptr;

        BESDEBUG(MODULE, prolog << "BEGIN" << endl);

        // Add the authorization headers
        request_headers = add_edl_auth_headers(request_headers);

        try {
            BESDEBUG(MODULE,
                     prolog << "BESDebug::IsSet(" << MODULE << "): " << (BESDebug::IsSet(MODULE) ? "true" : "false")
                            << endl);
            BESDEBUG(MODULE, prolog << "BESDebug::IsSet(" << TIMING_LOG_KEY << "): "
                                    << (BESDebug::IsSet(TIMING_LOG_KEY) ? "true" : "false") << endl);
            BESDEBUG(MODULE, prolog << "BESLog::TheLog()->is_verbose(): "
                                    << (BESLog::TheLog()->is_verbose() ? "true" : "false") << endl);

            ceh = init_effective_url_retriever_handle(starting_point_url->str(), request_headers, resp_hdrs);

            {
                BESStopWatch sw;
                if (BESDebug::IsSet("euc") || BESDebug::IsSet(MODULE) || BESDebug::IsSet(TIMING_LOG_KEY) ||
                    BESLog::TheLog()->is_verbose()) {
                    sw.start(prolog + " Following Redirects Starting With: " + starting_point_url->str());
                }
                super_easy_perform(ceh);
            }

            // After doing the thing with super_easy_perform() we retrieve the effective URL form the cURL handle.
            string e_url_str = get_effective_url(ceh, starting_point_url->str());
            std::shared_ptr<http::EffectiveUrl> eurl(new EffectiveUrl(e_url_str, resp_hdrs, starting_point_url->is_trusted()));

            BESDEBUG(MODULE, prolog << "Last Accessed URL(CURLINFO_EFFECTIVE_URL): " << eurl->str() <<
                "(" << (eurl->is_trusted()?"":"NOT ") << "trusted)" << endl);

            INFO_LOG(prolog << "Source URL: '" << starting_point_url->str() << "(" << (starting_point_url->is_trusted() ? "" : "NOT ") << "trusted)" <<
                            "' CURLINFO_EFFECTIVE_URL: '" << eurl->str() << "'" << "(" << (eurl->is_trusted()?"":"NOT ") << "trusted)" << endl);


            if (request_headers)
                curl_slist_free_all(request_headers);
            if (ceh)
                curl_easy_cleanup(ceh);

            return eurl;
        }
        catch (...) {
            if (request_headers)
                curl_slist_free_all(request_headers);
            if (ceh)
                curl_easy_cleanup(ceh);
            throw;
        }

#if 0
        {
            unsigned int attempts = 0;
            bool success = true;
            useconds_t retry_time = uone_second / 4;

            char error_buffer[CURL_ERROR_SIZE];
            vector<string> resp_hdrs;
            CURL *ceh = NULL;
            CURLcode curl_code;

            struct curl_slist *request_headers = NULL;
            // Add the authorization headers
            request_headers = get_auth_headers(request_headers);

            try {
                ceh = init_effective_url_retriever_handle(url, request_headers, resp_hdrs);
                set_error_buffer(ceh, error_buffer);
                do {
                    // bool do_retry;
                    error_buffer[0] = 0; // Initialize to empty string
                    ++attempts;
                    BESDEBUG(MODULE, prolog << "Requesting URL: " << starting_point_url << " attempt: " << attempts << endl);

                    curl_code = curl_easy_perform(ceh);
                    success = eval_curl_easy_perform_code(ceh, starting_point_url, curl_code, error_buffer, attempts);
                    if (success) {
                        // Nothing obvious went wrong with the curl_easy_perfom() so now we check the HTTP stuff
                        success = eval_http_get_response(ceh, starting_point_url);
                        if (!success) {
                            if (attempts == retry_limit) {
                                string msg = prolog +
                                             "ERROR - Problem with data transfer. Number of re-tries exceeded. Giving up.";
                                LOG(msg << endl);
                                throw BESInternalError(msg, __FILE__, __LINE__);
                            } else {
                                LOG(prolog << "ERROR - Problem with data transfer. Will retry (url: " << starting_point_url <<
                                           " attempt: " << attempts << ")." << endl);
                            }
                        }
                    }
                    // If it did not work we keep trying until we have exceeded the retry_limit.
                    if (!success) {
                        usleep(retry_time);
                        retry_time *= 2;
                    }
                } while (!success);

                char *effective_url = 0;
                curl_easy_getinfo(ceh, CURLINFO_EFFECTIVE_URL, &effective_url);
                BESDEBUG(MODULE, prolog << " CURLINFO_EFFECTIVE_URL: " << effective_url << endl);
                last_accessed_url = effective_url;

                LOG(prolog << "Source URL: '" << starting_point_url << "' Last Accessed URL: '" << last_accessed_url << "'" << endl);

                unset_error_buffer(ceh);

                if (ceh) {
                    curl_slist_free_all(request_headers);
                    curl_easy_cleanup(ceh);
                    ceh = 0;
                }
            }
            catch (...) {
                if (request_headers)
                    curl_slist_free_all(request_headers);
                if (ceh) {
                    curl_easy_cleanup(ceh);
                    ceh = 0;
                }
                throw;
            }
        }
#endif
    }

/**
 * @brief Return the location of the netrc file for Hyrax to utilize when
 * making requests for remote resources.
 *
 * If no file is specifed an empty string is returned.
 *
 * @return The name of the netrc file specified in the configuration, possibly the empty
 * string of none was specified.
 */
string get_netrc_filename() {
    string netrc_filename;
    bool found = false;
    TheBESKeys::TheKeys()->get_value(HTTP_NETRC_FILE_KEY, netrc_filename, found);
    if (found) {
        BESDEBUG(MODULE, prolog << "Using netrc file: " << netrc_filename << endl);
    }
    else {
        BESDEBUG(MODULE, prolog << "Using default netrc file. (~/.netrc)" << endl);
    }
    return netrc_filename;
}

/**
 * Set the error buffer for the cURL easy handle ceh to error_buffer
 * @param ceh
 * @param error_buffer
 */
void set_error_buffer(CURL *ceh, char *error_buffer) {
    CURLcode res;
    res = curl_easy_setopt(ceh, CURLOPT_ERRORBUFFER, error_buffer);
    curl::eval_curl_easy_setopt_result(res, prolog, "CURLOPT_ERRORBUFFER", error_buffer, __FILE__, __LINE__);
}

/**
 * Based on this thread: https://curl.haxx.se/mail/lib-2011-10/0078.html
 * We "unset" the error buffer using a null pointer.
 * @param ceh
 */
void unset_error_buffer(CURL *ceh) {
    set_error_buffer(ceh, NULL);
}


/**
 * A single source of truth for the User-Agent string used by Hyrax.
 * @return The Hyrax User-Agent string.
 */
string hyrax_user_agent() {
    string user_agent;
    bool found;
    TheBESKeys::TheKeys()->get_value(HTTP_USER_AGENT_KEY,user_agent, found);
    if(!found || user_agent.empty()){
        user_agent = HTTP_DEFAULT_USER_AGENT;
    }
    BESDEBUG(MODULE, prolog << "User-Agent: "<< user_agent << endl);
    return user_agent;
}

/**
 * @brief Evaluuates the CURLcode returned by curl_easy_setopt()
 *
 * Throws a BESInternalError if curl_code != CURLE_OK
 *
 * A SSOT for this activity, which get's done a bunch.
 *
 * @param result The CURLcode value returned by the call to curl_easy_setopt()
 * @param msg_base The prefix for any error message that gets generated.
 * @param opt_name The string name of the option that was set.
 * @param ebuf The cURL error buffer associated with the cURL easy handle
 * at the time of the setopt call
 * @param file The value of __FILE__ of the calling function.
 * @param line The value of __LINE__ of the calling function.
 */
void eval_curl_easy_setopt_result(
        CURLcode curl_code,
        string msg_base,
        string opt_name,
        char *ebuf,
        string file,
        unsigned int line) {
    if (curl_code != CURLE_OK) {
        stringstream msg;
        msg << msg_base << "ERROR - cURL failed to set " << opt_name << " Message: " << curl::error_message(curl_code, ebuf);
        throw BESInternalError(msg.str(), file, line);
    }
}

unsigned long max_redirects() {
    return http::load_max_redirects_from_keys();
}

/**
 * @brief Add the given header & value to the curl slist.
 *
 * The call must free the slist after the curl_easy_perform() is called, not after
 * the headers are added to the curl handle.
 *
 * @param slist The list; initially pass nullptr to create a new list
 * @param header The header
 * @param value The value
 * @return The modified slist pointer or nullptr if an error occurred.
 */
curl_slist *append_http_header(curl_slist *slist, const string &header_name, const string &value)
{

    string full_header = header_name;
    full_header.append(": ").append(value);

    BESDEBUG(MODULE, prolog << full_header << endl);
    // std::cerr << prolog << full_header << endl;

    struct curl_slist *temp = curl_slist_append(slist, full_header.c_str());
    if (!temp){
        stringstream msg;
        msg << prolog << "Encountered cURL Error setting the " << header_name << " header. full_header: " << full_header;
        throw BESInternalError(msg.str(), __FILE__, __LINE__);
    }
    return temp;
}


/**
 * @brief Adds the user id and/or the associated EDL auth token
 * to request_headers.
 *
 * Currently looks for 3 specific auth header values in the
 * BESContextManager set by the OLFS.
 *
 *  - uid: The user id of the logged in user.
 *
 * - edl_auth_token: The EDL authentication token recovered
 *  from EDL using the user's one time code. This may have
 *  come from the user logging in via the OLFS and EDL or it
 *  may have been transmitted from upstream as the header:
 *      Authorization: Bearer <edl_auth_token>
 *  from an upstream process.
 *
 * The Authorization header is made of the sting:
 *
 *    Authorization: Bearer edl_access_token
 *
 * - edl_echo_token: This soon to be legacy token is formed from
 *  the edl_auth_token and the server's EDL client_application_id.
 *     Echo-Token: edl_access_token:Client-Id
 *
 * If an aspirational auth header value is missing then that header
 * will not be added to the request_headers list.
 *
 * @param request_headers
 * @return
 */
curl_slist *add_edl_auth_headers(curl_slist *request_headers) {
    bool found;
    string s;

    s = BESContextManager::TheManager()->get_context(EDL_UID_KEY, found);
    if (found && !s.empty()) {
        request_headers = append_http_header(request_headers,"User-Id",s);
    }

    s = BESContextManager::TheManager()->get_context(EDL_AUTH_TOKEN_KEY, found);
    if (found && !s.empty()) {
        request_headers = append_http_header(request_headers,"Authorization",s);
    }

    s = BESContextManager::TheManager()->get_context(EDL_ECHO_TOKEN_KEY, found);
    if (found && !s.empty()) {
        request_headers = append_http_header(request_headers,"Echo-Token",s);
    }

    return request_headers;
}

/**
 * @brief Queries the passed cURL easy handle, ceh, for the value of CURLINFO_EFFECTIVE_URL and returns said value.
 *
 * @param ceh The cURL easy handle to query
 * @param requested_url The original URL that was set in the cURL handle prior to a call to curl_easy_perform.
 * @return  The value of CURLINFO_EFFECTIVE_URL from the cURL handle ceh.
 */
string get_effective_url(CURL *ceh, string requested_url) {
    char *effectve_url = nullptr;
    CURLcode curl_code = curl_easy_getinfo(ceh, CURLINFO_EFFECTIVE_URL, &effectve_url);
    if (curl_code != CURLE_OK) {
        stringstream msg;
        msg << prolog << "Unable to determine CURLINFO_EFFECTIVE_URL! Requested URL: " << requested_url;
        BESDEBUG(MODULE, msg.str() << endl);
        throw BESInternalError(msg.str(), __FILE__, __LINE__);
    }
    return effectve_url;
}


} /* namespace curl */
