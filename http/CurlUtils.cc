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

#include <unistd.h>
#include <fcntl.h>
#include <ctime>
#include <cstring>

#include <curl/curl.h>

#include <sstream>
#include <vector>
#include <algorithm>    // std::for_each

#include "BESContextManager.h"
#include "BESSyntaxUserError.h"
#include "BESInternalError.h"
#include "HttpError.h"
#include "BESDebug.h"
#include "BESRegex.h"
#include "TheBESKeys.h"
#include "BESLog.h"
#include "BESStopWatch.h"

#include "HttpNames.h"

#include "HttpUtils.h"
#include "ProxyConfig.h"
#include "AllowedHosts.h"
#include "CurlUtils.h"
#include "CredentialsManager.h"
#include "AccessCredentials.h"
#include "RequestServiceTimer.h"

#include "awsv4.h"
#include "url_impl.h"

#define MODULE "curl"
#define CURL_TIMING "curl:timing"

using namespace AWSV4;
using namespace http;
using namespace std;

#define prolog std::string("CurlUtils::").append(__func__).append("() - ")

namespace curl {

static void super_easy_perform(CURL *c_handle, int fd);

const unsigned int retry_limit = 3; // 10; // Amazon's suggestion
const useconds_t url_retry_time = 250'000; // 1/4 second in micro seconds

// Set this to 1 to turn on libcurl's verbose mode (for debugging).
const int curl_trace = 0;

const int CLIENT_ERR_MIN = 400;
const int CLIENT_ERR_MAX = 417;
const vector <string> http_client_errors = {
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

const int SERVER_ERR_MIN = 500;
const int SERVER_ERR_MAX = 505;
const vector <string> http_server_errors = {
        "Internal Server Error.",
        "Not Implemented.",
        "Bad Gateway.",
        "Service Unavailable.",
        "Gateway Time-out.",
        "HTTP Version Not Supported."
};

/**
 * @brief Translates an HTTP code into an error message.
 *
 * It works for those code greater than or equal to 400.
 *
 * @param code The HTTP code to associate with an error message
 * @return The error message associated with code.
 */
static string http_code_to_string(long code) {
    if (code >= CLIENT_ERR_MIN && code <= CLIENT_ERR_MAX)
        return {http_client_errors[code - CLIENT_ERR_MIN]};
    else if (code >= SERVER_ERR_MIN && code <= SERVER_ERR_MAX)
        return {http_server_errors[code - SERVER_ERR_MIN]};
    else {
        return {"Unknown HTTP Error: " + to_string(code)};
    }
}

/**
 * @brief Translate a cURL authentication type value (int) into a human readable string.
 * @param auth_type The cURL authentication type value to convert
 * @return The human readable string associated with auth_type.
 */
static string getCurlAuthTypeName(unsigned long auth_type) {

    string authTypeString;
    unsigned long match;

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

    return authTypeString;
}

/**
 * libcurl call back function that is used to write data to a passed open file descriptor (that would
 * be instead of the default open FILE *)
 */
#define CURL_WRITE_TO_FILE_TIMEOUT_MSG  "The function curl::writeToOpenFileDescriptor() was unable to complete the download process because it ran out of time."

static size_t writeToOpenFileDescriptor(const char *data, size_t /* size */, size_t nmemb, const void *userdata) {

    const auto fd = static_cast<const int *>(userdata);

    BESDEBUG(MODULE, prolog << "Bytes received: " << nmemb << endl);
    size_t bytes_written = write(*fd, data, nmemb);
    BESDEBUG(MODULE, prolog << " Bytes written: " << bytes_written << endl);

    // Verify the request hasn't exceeded bes_timeout, and throw if it has...
    RequestServiceTimer::TheTimer()->throw_if_timeout_expired(CURL_WRITE_TO_FILE_TIMEOUT_MSG, __FILE__, __LINE__);

    return bytes_written;
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
    auto hdrs = static_cast<vector<string> * >(resp_hdrs);

    // Grab the header, minus the trailing newline. Or \r\n pair.
    string complete_line;
    if (nmemb > 1 && *(static_cast<char *>(ptr) + size * (nmemb - 2)) == '\r')
        complete_line.assign(static_cast<char *>(ptr), size * (nmemb - 2));
    else
        complete_line.assign(static_cast<char *>(ptr), size * (nmemb - 1));

    // Store all non-empty headers that are not HTTP codes
    if (!complete_line.empty() && complete_line.find("HTTP") == string::npos) {
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
static int curl_debug(const CURL *, curl_infotype info, const char *msg, size_t size, const void *) {
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
 * Based on this thread: https://curl.haxx.se/mail/lib-2011-10/0078.html
 * We "unset" the error buffer using a null pointer.
 * @param ceh
 */
static void unset_error_buffer(CURL *ceh) {
    set_error_buffer(ceh, nullptr);
}

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
 * @note used only by init() below. jhrg 3/7/23
 *
 * @param ceh The cURL easy handle to configure.
 * @param target_url The url used to configure the proxy
 * @return
 */
static bool configure_curl_handle_for_proxy(CURL *ceh, const string &target_url) {
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
            if (r.match(target_url.c_str(), static_cast<int>(target_url.size())) != -1) {
                BESDEBUG(MODULE,
                         prolog << "Found NoProxy match. BESRegex: " << no_proxy_regex << "; Url: " << target_url
                                << endl);
                using_proxy = false;
            }
        }

        if (using_proxy) {
            CURLcode res;
            vector<char> error_buffer(CURL_ERROR_SIZE, (char) 0);

            BESDEBUG(MODULE, prolog << "Setting up a proxy server." << endl);
            BESDEBUG(MODULE, prolog << "Proxy host: " << proxyHost << endl);
            BESDEBUG(MODULE, prolog << "Proxy port: " << proxyPort << endl);

            set_error_buffer(ceh, error_buffer.data());

            res = curl_easy_setopt(ceh, CURLOPT_PROXY, proxyHost.data());
            eval_curl_easy_setopt_result(res, prolog, "CURLOPT_PROXY", error_buffer.data(), __FILE__, __LINE__);

            res = curl_easy_setopt(ceh, CURLOPT_PROXYPORT, proxyPort);
            eval_curl_easy_setopt_result(res, prolog, "CURLOPT_PROXYPORT", error_buffer.data(), __FILE__, __LINE__);

            // oddly "#ifdef CURLOPT_PROXYAUTH" doesn't work - even though CURLOPT_PROXYAUTH is defined and valued at 111 it
            // fails the test. Eclipse hover over the CURLOPT_PROXYAUTH symbol shows: "CINIT(PROXYAUTH, LONG, 111)",
            // for what that's worth

            // According to http://curl.haxx.se/libcurl/c/curl_easy_setopt.html#CURLOPTPROXYAUTH
            // As of 4/21/08 only NTLM, Digest and Basic work.

            res = curl_easy_setopt(ceh, CURLOPT_PROXYAUTH, proxyAuthType);
            eval_curl_easy_setopt_result(res, prolog, "CURLOPT_PROXYAUTH", error_buffer.data(), __FILE__, __LINE__);
            BESDEBUG(MODULE, prolog << "Using CURLOPT_PROXYAUTH = " << getCurlAuthTypeName(proxyAuthType) << endl);

            if (!proxyUser.empty()) {
                res = curl_easy_setopt(ceh, CURLOPT_PROXYUSERNAME, proxyUser.data());
                eval_curl_easy_setopt_result(res, prolog, "CURLOPT_PROXYUSERNAME", error_buffer.data(), __FILE__,
                                             __LINE__);
                BESDEBUG(MODULE, prolog << "CURLOPT_PROXYUSERNAME : " << proxyUser << endl);

                if (!proxyPassword.empty()) {
                    res = curl_easy_setopt(ceh, CURLOPT_PROXYPASSWORD, proxyPassword.data());
                    eval_curl_easy_setopt_result(res, prolog, "CURLOPT_PROXYPASSWORD", error_buffer.data(), __FILE__,
                                                 __LINE__);
                    BESDEBUG(MODULE, prolog << "CURLOPT_PROXYPASSWORD: " << proxyPassword << endl);
                }
            } else if (!proxyUserPW.empty()) {
                res = curl_easy_setopt(ceh, CURLOPT_PROXYUSERPWD, proxyUserPW.data());
                eval_curl_easy_setopt_result(res, prolog, "CURLOPT_PROXYUSERPWD", error_buffer.data(), __FILE__,
                                             __LINE__);
                BESDEBUG(MODULE, prolog << "CURLOPT_PROXYUSERPWD : " << proxyUserPW << endl);
            }
            unset_error_buffer(ceh);
        }
    }
    BESDEBUG(MODULE, prolog << "END. using_proxy: " << (using_proxy ? "true" : "false") << endl);
    return using_proxy;
}

// This is used in only one place.
static CURL *init(CURL *ceh, const string &target_url, const curl_slist *http_request_headers,
                  vector <string> *http_response_hdrs) {
    vector<char> error_buffer(CURL_ERROR_SIZE, (char) 0);
    CURLcode res;

    if (!ceh)
        throw BESInternalError("Could not initialize cURL easy handle.", __FILE__, __LINE__);

    // SET Error Buffer (for use during this setup) ----------------------------------------------------------------
    set_error_buffer(ceh, error_buffer.data());

    // Target URL --------------------------------------------------------------------------------------------------
    res = curl_easy_setopt(ceh, CURLOPT_URL, target_url.c_str());
    eval_curl_easy_setopt_result(res, prolog, "CURLOPT_URL", error_buffer.data(), __FILE__, __LINE__);

    // Load in the default headers to send with a request. The empty Pragma
    // headers overrides libcurl's default Pragma: no-cache header (which
    // will disable caching by Squid, etc.).
    // the empty Pragma never appears in the outgoing headers when this isn't present
    // d_request_headers->push_back(string("Pragma: no-cache"));
    // d_request_headers->push_back(string("Cache-Control: no-cache"));

    if (http_request_headers) {
        // Add the http_request_headers to the cURL handle.
        res = curl_easy_setopt(ceh, CURLOPT_HTTPHEADER, http_request_headers);
        eval_curl_easy_setopt_result(res, prolog, "CURLOPT_HTTPHEADER", error_buffer.data(), __FILE__, __LINE__);
    }


    if (http_response_hdrs) {
        res = curl_easy_setopt(ceh, CURLOPT_HEADERFUNCTION, save_http_response_headers);
        eval_curl_easy_setopt_result(res, prolog, "CURLOPT_HEADERFUNCTION", error_buffer.data(), __FILE__, __LINE__);

        // Pass save_http_response_headers() a pointer to the vector<string> where the
        // response headers may be stored. Callers can use the resp_hdrs
        // value/result parameter to get the raw response header information .
        res = curl_easy_setopt(ceh, CURLOPT_WRITEHEADER, http_response_hdrs);
        eval_curl_easy_setopt_result(res, prolog, "CURLOPT_WRITEHEADER", error_buffer.data(), __FILE__, __LINE__);
    }

    // Allow compressed responses. Sending an empty string enables all supported compression types.
#ifndef CURLOPT_ACCEPT_ENCODING
    res = curl_easy_setopt(ceh, CURLOPT_ENCODING, "");
    eval_curl_easy_setopt_result(res, prolog, "CURLOPT_ENCODING", error_buffer.data(), __FILE__, __LINE__);
#else
    res = curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "");
    check_setopt_result(res, prolog, "CURLOPT_ACCEPT_ENCODING", error_buffer, __FILE__,__LINE__);
#endif
    // Disable Progress Meter
    res = curl_easy_setopt(ceh, CURLOPT_NOPROGRESS, 1L);
    eval_curl_easy_setopt_result(res, prolog, "CURLOPT_NOPROGRESS", error_buffer.data(), __FILE__, __LINE__);

    // Disable cURL signal handling
    res = curl_easy_setopt(ceh, CURLOPT_NOSIGNAL, 1L);
    eval_curl_easy_setopt_result(res, prolog, "CURLOPT_NOSIGNAL", error_buffer.data(), __FILE__, __LINE__);


    // -  -  -  - -  -  -  - -  -  -  - -  -  -  - -  -  -  - -  -  -  - -  -  -  -
    // Authentication config.
    //

    // We have to set FailOnError to false for any of the non-Basic
    // authentication schemes to work. 07/28/03 jhrg
    res = curl_easy_setopt(ceh, CURLOPT_FAILONERROR, 0L);
    eval_curl_easy_setopt_result(res, prolog, "CURLOPT_FAILONERROR", error_buffer.data(), __FILE__, __LINE__);


    // CURLAUTH_ANY means libcurl will use Basic, Digest, GSS Negotiate, or NTLM,
    // choosing the 'safest' one supported by the server.
    // This requires curl 7.10.6 which is still in pre-release. 07/25/03 jhrg
    res = curl_easy_setopt(ceh, CURLOPT_HTTPAUTH, (long) CURLAUTH_ANY);
    eval_curl_easy_setopt_result(res, prolog, "CURLOPT_HTTPAUTH", error_buffer.data(), __FILE__, __LINE__);


    // CURLOPT_NETRC means to use the netrc file for credentials.
    // CURL_NETRC_OPTIONAL Means that if the supplied URL contains a username
    // and password to prefer that to using the content of the netrc file.
    res = curl_easy_setopt(ceh, CURLOPT_NETRC, CURL_NETRC_OPTIONAL);
    eval_curl_easy_setopt_result(res, prolog, "CURLOPT_NETRC", error_buffer.data(), __FILE__, __LINE__);

    // If the configuration specifies a particular .netrc credentials file, use it.
    string netrc_file = get_netrc_filename();
    if (!netrc_file.empty()) {
        res = curl_easy_setopt(ceh, CURLOPT_NETRC_FILE, netrc_file.c_str());
        eval_curl_easy_setopt_result(res, prolog, "CURLOPT_NETRC_FILE", error_buffer.data(), __FILE__, __LINE__);

    }
    VERBOSE(prolog + " is using the netrc file '"
                   + (!netrc_file.empty() ? netrc_file : "~/.netrc") + "'");


    // -  -  -  - -  -  -  - -  -  -  - -  -  -  - -  -  -  - -  -  -  - -  -  -  -
    // Cookies
    //
    res = curl_easy_setopt(ceh, CURLOPT_COOKIEFILE, curl::get_cookie_filename().c_str());
    eval_curl_easy_setopt_result(res, prolog, "CURLOPT_COOKIEFILE", error_buffer.data(), __FILE__, __LINE__);

    res = curl_easy_setopt(ceh, CURLOPT_COOKIEJAR, curl::get_cookie_filename().c_str());
    eval_curl_easy_setopt_result(res, prolog, "CURLOPT_COOKIEJAR", error_buffer.data(), __FILE__, __LINE__);

    // save_http_response_headers

    // Follow 302 (redirect) responses
    res = curl_easy_setopt(ceh, CURLOPT_FOLLOWLOCATION, 1L);
    eval_curl_easy_setopt_result(res, prolog, "CURLOPT_FOLLOWLOCATION", error_buffer.data(), __FILE__, __LINE__);

    res = curl_easy_setopt(ceh, CURLOPT_MAXREDIRS, max_redirects());
    eval_curl_easy_setopt_result(res, prolog, "CURLOPT_MAXREDIRS", error_buffer.data(), __FILE__, __LINE__);

    // Set the user agent to Hyrax's user agent value
    res = curl_easy_setopt(ceh, CURLOPT_USERAGENT, hyrax_user_agent().c_str());
    eval_curl_easy_setopt_result(res, prolog, "CURLOPT_USERAGENT", error_buffer.data(), __FILE__, __LINE__);

    if (curl_trace) {
        BESDEBUG(MODULE, prolog << "Curl version: " << curl_version() << endl);
        res = curl_easy_setopt(ceh, CURLOPT_VERBOSE, 1L);
        eval_curl_easy_setopt_result(res, prolog, "CURLOPT_VERBOSE", error_buffer.data(), __FILE__, __LINE__);
        BESDEBUG(MODULE, prolog << "Curl in verbose mode." << endl);

        res = curl_easy_setopt(ceh, CURLOPT_DEBUGFUNCTION, curl_debug);
        eval_curl_easy_setopt_result(res, prolog, "CURLOPT_DEBUGFUNCTION", error_buffer.data(), __FILE__, __LINE__);
        BESDEBUG(MODULE, prolog << "Curl debugging function installed." << endl);
    }

    // We unset the error buffer here because we know that curl::configure_curl_handle_for_proxy() will use it's own.
    unset_error_buffer(ceh);
    // Configure the proxy for this url (if appropriate).
    curl::configure_curl_handle_for_proxy(ceh, target_url);

    BESDEBUG(MODULE, prolog << "curl: " << (void *) ceh << endl);
    return ceh;
}

/**
 * Gets a new instance of a cURL easy handle (CURL*) and performs a basic configuration of that instance.
 *  - Accept compressed responses
 *  - Any authentication type
 *  - Follow redirects
 *  - User agent set to value of hyrax_user_agent().
 *
 * @param target_url
 * @param http_request_headers
 * @param http_response_hdrs
 * @return
 */
CURL *init(const string &target_url,
           const curl_slist *http_request_headers,
           vector <string> *http_response_hdrs) {
    CURL *swanky_new_curl_easy_handle = curl_easy_init();
    return init(swanky_new_curl_easy_handle, target_url, http_request_headers, http_response_hdrs);
}


string get_range_arg_string(const unsigned long long &offset, const unsigned long long &size) {
    ostringstream range;   // range-get needs a string arg for the range
    range << offset << "-" << offset + size - 1;
    BESDEBUG(MODULE, prolog << " range: " << range.str() << endl);
    return range.str();
}

/**
 * @brief Sign the URL if it matches S3 credentials held by the CredentialsManager
 *
 * @param url The URL as a string.
 * @param request_headers An existing list of curl request headers. If this is empty,
 * the append operation used by this function will result in a blank entry in the
 * first node
 * @return The modified list of request headers, if the URL was signed, or the original
 * list of headers if it was not.
 */
static curl_slist *
sign_url_for_s3_if_possible(const string &url, curl_slist *request_headers) {
    // If this is a URL that references an S3 bucket, and there are credentials for the URL,
    // sign the URL.
    if (CredentialsManager::theCM()->size() > 0) {
        auto ac = CredentialsManager::theCM()->get(url);
        if (ac && ac->is_s3_cred()) {
            BESDEBUG(MODULE, prolog << "Located S3 credentials for url: " << url
                    << " Using request headers to hold AWS signature\n");
            request_headers = sign_s3_url(url, ac, request_headers);
        }
        else {
            if(ac){
                BESDEBUG(MODULE, prolog << "Located credentials for url: " << url  << "They are "
                << (ac->is_s3_cred()?"":"NOT ") << "S3 credentials.\n");
            }
            else {
                BESDEBUG(MODULE, prolog << "Unable to locate credentials for url: " << url << "\n");
            }
        }
    }

    return request_headers;
}

/**
 * @brief Sign the URL if it matches S3 credentials held by the CredentialsManager
 *
 * @param url An instance of http::url
 * @param request_headers An existing list of curl request headers. If this is empty,
 * the append operation used by this function will result in a blank entry in the
 * first node
 * @return The modified list of request headers, if the URL was signed, or the original
 * list of headers if it was not.
 */
static curl_slist *
sign_url_for_s3_if_possible(const shared_ptr <url> &url, curl_slist *request_headers) {
    return sign_url_for_s3_if_possible(url->str(), request_headers);
}

/**
 * @brief Queries the passed cURL easy handle, ceh, for the value of CURLINFO_EFFECTIVE_URL and returns said value.
 *
 * @note Not to be confused with EffectiveUrlCache::get_effective_url().
 *
 * @param ceh The cURL easy handle to query
 * @param requested_url The original URL that was set in the cURL handle prior to a call to curl_easy_perform.
 * @return  The value of CURLINFO_EFFECTIVE_URL from the cURL handle ceh.
 */
static string get_effective_url(CURL *ceh, const string &requested_url) {
    char *effective_url = nullptr;
    CURLcode curl_code = curl_easy_getinfo(ceh, CURLINFO_EFFECTIVE_URL, &effective_url);
    if (curl_code != CURLE_OK) {
        stringstream msg;
        msg << prolog << "Unable to determine CURLINFO_EFFECTIVE_URL! Requested URL: " << requested_url;
        BESDEBUG(MODULE, msg.str() << endl);
        throw BESInternalError(msg.str(), __FILE__, __LINE__);
    }
    return effective_url;
}


/**
 * @brief A helper function for filter_aws_url()
 * @param kvp_string The string to be evaluated and possibly added to the vector
 * @param kvp A vector of strings to hold the kvp strings that are not obviously AWS things.
 */
void add_if_not_aws(const string &kvp_string, std::vector<std::string> &kvp) {
    if (kvp_string.find("X-Amz-") == string::npos) {
        kvp.push_back(kvp_string);
    }
}


// https://<<host>>>/<<path>>?A-userid=jhrg&amp;X-Amz-Algorithm=AWS4-HMAC-SHA256&amp;X-Amz-Credential=...;
// X-Amz-Date=20230417T193403Z&amp;X-Amz-Expires=3467&amp;X-Amz-Security-Token=...
/**
 * @brief Remove AWS tokens from the URL
 * This function will look for the first ampersand and remove everything after it. Then
 * it will look anything with an X-Amz- prefix if that is found it will remove the whole
 * query string. This code should only be called if an error is being reported, so high
 * performance is not required.
 * @note Public only to enable testing.
 * @param eff_url
 * @return The URL with the tokens removed
 */
string filter_aws_url(const string &eff_url) {

    std::vector<std::string> kvp;
    string filtered_url = eff_url;
    size_t start = 0;
    auto position = eff_url.find('?');

    if (position != string::npos) {
        constexpr char delimiter = '&';
        // We found a '?' which indicates that there may be a query string.
        start = position + 1;

        // Stash the URl's domain name and path for reconstruction...
        filtered_url = eff_url.substr(0, position);

        // Find the first delimiter in the query_string
        position  = eff_url.find(delimiter, start);

        while (position != std::string::npos) {

            // Find the current kvp record and add it (or not if it's an AWS thing) to the kvp vector
            add_if_not_aws(eff_url.substr(start, position - start), kvp);

            // Start at the beginning of the next field (if there is one)
            start = position + 1;
            position = eff_url.find(delimiter, start);
        }
        // Handle any remaining chars in the query, add them (or not if they're an AWS thing) to the kvp vector
        add_if_not_aws(eff_url.substr(start, position - start), kvp);

        // Now rebuild the URL, but without the AWS stuff.
        bool first = true;
        for (const auto &kvp_str:kvp) {
            if (!first) {
                filtered_url += delimiter;
            }
            else {
                filtered_url += '?';
            }
            filtered_url += kvp_str;
            first = false;
        }
    }
    return filtered_url;
}

/**
 * Checks to see if the entire url matches any of the "no retry" regular expressions held in the TheBESKeys
 * under the HTTP_NO_RETRY_URL_REGEX_KEY which atm, is set to "Http.No.Retry.BESRegex"
 *
 * @note Used only locally by eval_get_http_response() and tests. jhrg 3/8/23
 *
 * @param target_url The URL to be examined
 * @return True if the target_url does not match a no retry regex, false if the entire target_url matches
 * a "no retry" regex.
 */

// TODO If these regexes are complex, they will take a significant amount of time to compile. Fix.
//  A better solution is to compile the regex once and store the compiled regex for future use. It's
//  the compilation that takes a long time. jhrg 11/3/22
bool is_retryable(const string &target_url) {
    BESDEBUG(MODULE, prolog << "BEGIN" << endl);
    bool retryable = true;

    vector<string> nr_regexs;
    bool found;
    TheBESKeys::TheKeys()->get_values(HTTP_NO_RETRY_URL_REGEX_KEY, nr_regexs, found);
    if (found) {
        for (const auto &nr_regex: nr_regexs) {
            BESDEBUG(MODULE, prolog << "nr_regex: '" << nr_regex << "'" << endl);
            BESRegex no_retry_regex(nr_regex.c_str(), (int) nr_regex.size());
            size_t match_length = no_retry_regex.match(target_url.c_str(), (int) target_url.size(), 0);
            if (match_length == target_url.size()) {
                BESDEBUG(MODULE, prolog << "The url: '" << target_url << "' fully matched the "
                                        << HTTP_NO_RETRY_URL_REGEX_KEY << ": '" << nr_regex << "'" << endl);
                retryable = false;
                break;
            }

        }
    }

    BESDEBUG(MODULE, prolog << "END retryable: " << (retryable ? "true" : "false") << endl);
    return retryable;
}

/**
 * This function evaluates the CURLcode value returned by curl_easy_perform.
 *
 * This function assumes that three types of cURL error may be retried:
 *  - CURLE_SSL_CONNECT_ERROR
 *  - CURLE_SSL_CACERT_BADFILE
 *  - CURLE_GOT_NOTHING
 *  For these values of curl_code the function returns false.
 *
 *  The function returns success iff curl_code == CURLE_OK.
 *
 *  If the curl_code is another value, a BESInternalError is thrown.
 *
 * @param eff_req_url The requested URL - This should be the 'effective URL'.
 * @param curl_code The CURLcode value to evaluate.
 * @param error_buffer The CURLOPT_ERRORBUFFER used in the request.
 * @param attempt The number of attempts on the url that this request represents.
 *
 * @return True if the curl_easy_perform was successful, false if not, but the request may be retried.
 * @throws BESInternalError When the curl_code is an error that should not be retried.
 */
static bool eval_curl_easy_perform_code(
        const string &eff_req_url,
        CURLcode curl_code,
        const char *error_buffer,
        const unsigned int attempt
) {
    if (curl_code == CURLE_SSL_CONNECT_ERROR) {
        stringstream msg;
        msg << prolog << "ERROR - cURL experienced a CURLE_SSL_CONNECT_ERROR error. Message: ";
        msg << curl::error_message(curl_code, error_buffer) << ". ";
        msg << "A retry may be possible for: " << filter_aws_url(eff_req_url) << " (attempt: " << attempt << ")."
            << endl;
        BESDEBUG(MODULE, msg.str());
        ERROR_LOG(msg.str());
        return false;
    } else if (curl_code == CURLE_SSL_CACERT_BADFILE) {
        stringstream msg;
        msg << prolog << "ERROR - cURL experienced a CURLE_SSL_CACERT_BADFILE error. Message: ";
        msg << curl::error_message(curl_code, error_buffer) << ". ";
        msg << "A retry may be possible for: " << filter_aws_url(eff_req_url) << " (attempt: " << attempt << ")."
            << endl;
        BESDEBUG(MODULE, msg.str());
        ERROR_LOG(msg.str());
        return false;
    } else if (curl_code == CURLE_GOT_NOTHING) {
        // First we check to see if the response was empty. This is a cURL error, not an HTTP error
        // so we have to handle it like this. And we do that because this is one of the failure modes
        // we see in the AWS cloud and by trapping this and returning false we are able to be resilient and retry.
        stringstream msg;
        msg << prolog << "ERROR - cURL returned CURLE_GOT_NOTHING. Message: ";
        msg << error_message(curl_code, error_buffer) << ". ";
        msg << "A retry may be possible for: " << filter_aws_url(eff_req_url) << " (attempt: " << attempt << ")."
            << endl;
        BESDEBUG(MODULE, msg.str());
        ERROR_LOG(msg.str());
        return false;
    } else if (curl_code != CURLE_OK) {
        stringstream msg;
        msg << "ERROR - Problem with data transfer. Message: " << curl::error_message(curl_code, error_buffer);
        msg << " CURLINFO_EFFECTIVE_URL: " << filter_aws_url(eff_req_url);
        BESDEBUG(MODULE, prolog << msg.str() << endl);
        ERROR_LOG(msg.str());
        return false;
    }

    return true;
}

/**
 * Helper for the eval_http_get_response() function that evaluates the HTTP code. Only call this
 * with HTTP codes that are >= 400. This returns if the request can be retried. If it cannot
 * be retried then a BESInternalError is thrown (and this never returns).
 *
 * @note: Only call this when there was a problem with the HTTP request.
 *
 * @param http_code
 * @param requested_url
 * @param last_accessed_url
 */
static void
process_http_code_helper(const long http_code, const string &requested_url, const string &last_accessed_url) {
    stringstream msg;
    if (http_code >= 400) {
        msg << "ERROR - The HTTP GET request for the source URL: " << requested_url << " FAILED. ";
        msg << "CURLINFO_EFFECTIVE_URL: " << filter_aws_url(last_accessed_url) << " ";
        BESDEBUG(MODULE, prolog << msg.str() << endl);
    }

    msg << "The response from " << last_accessed_url << " (Originally: " << requested_url << ") ";
    msg << "returned an HTTP code of " << http_code;
    msg << " which means " << http_code_to_string(http_code) << " ";

    switch (http_code) {
        case 400: // Bad Request
        case 401: // Unauthorized
        case 402: // Payment Required
        case 403: // Forbidden
        case 404: // Not Found
        case 408: // Request Timeout
        {
            // These issues are not considered retryable problems, so we throw immediately.
            // Remove this redundant call to ERROR_LOG since the thrown exception is
            //  logged as an error. jhrg 1/24/25
            // ERROR_LOG(msg.str());
            throw http::HttpError(msg.str(),
                                  CURLE_OK,
                                  http_code,
                                  requested_url,
                                  last_accessed_url,
                                  __FILE__, __LINE__);

        }
        case 422: // Unprocessable Entity
        case 500: // Internal server error
        case 502: // Bad Gateway
        case 503: // Service Unavailable
        case 504: // Gateway Timeout
        {
            // These problems might actually be retryable, so we check and then act accordingly.
            if (!is_retryable(last_accessed_url)) {
                msg << " The HTTP response code of this last accessed URL indicate that it should not be retried.";
                ERROR_LOG(msg.str());
                throw http::HttpError(msg.str(),
                                      CURLE_OK,
                                      http_code,
                                      requested_url,
                                      last_accessed_url,
                                      __FILE__, __LINE__);
            } else {
                msg << " The HTTP response code of this last accessed URL indicate that it should be retried.";
                BESDEBUG(MODULE, prolog << msg.str() << endl);
            }
        }
            break;

        default:
            // ERROR_LOG(msg.str());
            throw BESInternalError(msg.str(), __FILE__, __LINE__);
    }
}


/**
 *
 *
 * @param ceh
 * @return
 */
long get_http_code(CURL *ceh) {
    long http_code = 0;
    CURLcode curl_code = curl_easy_getinfo(ceh, CURLINFO_RESPONSE_CODE, &http_code);
    if (curl_code != CURLE_OK) {
        throw BESInternalError(prolog + "Error acquiring HTTP response code.", __FILE__, __LINE__);
    }
    BESDEBUG(MODULE, prolog << "http_code: " << http_code << "\n");
    return http_code;
}


/**
 * @brief Evaluates the HTTP semantics of a the result of issuing a cURL GET request.
 *
 * @note used only locally by super_easy_perform. jhrg 3/8/23
 * @note Do not call this for file:// URLs. jhrg 4/20/23
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
 *   case 422:  Unprocessable Entity
 *   case 500:  Internal server error
 *   case 502:  Bad Gateway
 *   case 503:  Service Unavailable
 *   case 504:  Gateway Timeout
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
static bool eval_http_get_response(CURL *ceh, const string &requested_url, long &http_code) {
    BESDEBUG(MODULE, prolog << "Requested URL: " << requested_url << endl);

    http_code = get_http_code(ceh);

    // Special case for file:// URLs. An HTTP Code is zero means success in that case. jhrg 4/20/23
    if (requested_url.find(FILE_PROTOCOL) == 0 && http_code == 0)
        return true;

#ifndef NDEBUG
    if (BESISDEBUG(MODULE)) {   // BESISDEBUG is a macro that expands to false when NDEBUG is defined. jhrg 4/19/23
        CURLcode curl_code;
        long redirects;
        curl_code = curl_easy_getinfo(ceh, CURLINFO_REDIRECT_COUNT, &redirects);
        if (curl_code != CURLE_OK)
            throw BESInternalError("Error acquiring CURLINFO_REDIRECT_COUNT.", __FILE__, __LINE__);
        BESDEBUG(MODULE, prolog << "CURLINFO_REDIRECT_COUNT: " << redirects << endl);

        char *redirect_url = nullptr;
        curl_code = curl_easy_getinfo(ceh, CURLINFO_REDIRECT_URL, &redirect_url);
        if (curl_code != CURLE_OK)
            throw BESInternalError("Error acquiring CURLINFO_REDIRECT_URL.", __FILE__, __LINE__);

        if (redirect_url)
            BESDEBUG(MODULE, prolog << "CURLINFO_REDIRECT_URL: " << redirect_url << endl);
    }
#endif

    // Newer Apache servers return 206 for range requests. jhrg 8/8/18
    switch (http_code) {
        case 0:
        case 200: // OK
        case 206: // Partial content - this is to be expected since we use range gets
            // cases 201-205 are things we should probably reject, unless we add more
            // comprehensive HTTP/S processing here. jhrg 8/8/18
            return true;

        default:
            string last_accessed_url = get_effective_url(ceh, requested_url);
            BESDEBUG(MODULE, prolog << "Last Accessed URL(CURLINFO_EFFECTIVE_URL): "
                                    << filter_aws_url(last_accessed_url) << endl);

            // process_http_code_helper() _only_ returns if the request can be retried, otherwise
            // it throws an exception. Pass the unfiltered last_accessed_url because the
            // query string params might be needed to determine if the URL should be retried.
            // jhrg 4/20/23
            process_http_code_helper(http_code, requested_url, last_accessed_url);
            return false;   // if we get here, retry the request
    }
}

// Truncate the file that holds information read off the wire when the
// library has to retry a request made to S3. The file will contain error
// text from the failed attempt and that needs to be cleaned out before
// the next attempt. jhrg 5/9/23
static void truncate_file(int fd) {
    auto status = ftruncate(fd, 0);
    if (status == -1)
        throw BESInternalError(string("Could not truncate the file before retrying request (") + strerror(errno) + ").",
                               __FILE__, __LINE__);

    // Removing this call to lseek will cause tests for the retry code to fail, which demonstrates that
    // this fixes the issue with retires without this call having corrupted data. jhrg 5/9/23
    status = lseek(fd, 0, SEEK_SET);
    if (-1 == status)
        throw BESInternalError(string("Could not seek within the response file (") + strerror(errno) + ").",
                               __FILE__, __LINE__);
}

// used only in one place here. jhrg 3/8/23
/**
 * @brief Performs a curl_easy_perform(), retrying if certain types of errors are encountered.
 *
 * @note Used in dmrpp_module and by two functions here (http_get, http_get_and_write_resource).
 * jhrg 3/8/23
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
    int fd = -1;
    super_easy_perform(c_handle, fd);
}

static void super_easy_perform(CURL *c_handle, int fd) {
    BESDEBUG(MODULE, prolog << "BEGIN\n");

    useconds_t retry_time = url_retry_time; // 0.25 seconds
    bool curl_success{false};
    bool http_success{false};
    long http_code{0};
    unsigned int attempts{0};

    vector<char> error_buffer(CURL_ERROR_SIZE, (char) 0);
    set_error_buffer(c_handle, error_buffer.data());

    string target_url = get_effective_url(c_handle, ""); // This is a trick to get the URL from the cURL handle.
    // We check the value of target_url to see if the URL was correctly set in the cURL handle.
    if (target_url.empty())
        throw BESInternalError("URL acquisition failed.", __FILE__, __LINE__);

    // This either works or throws an exception after retry_limit attempts
    while (!curl_success || !http_success) {
        ++attempts;
        BESDEBUG(MODULE,
                 prolog << "Requesting URL: " << filter_aws_url(target_url) << " attempt: " << attempts << endl);

        CURLcode curl_code = curl_easy_perform(c_handle);
        curl_success = eval_curl_easy_perform_code(target_url, curl_code, error_buffer.data(), attempts);
        BESDEBUG(MODULE, prolog << "curl_success: " << (curl_success ? "true" : "false") << endl);
        if (curl_success) {
            // Nothing obvious went wrong with the curl_easy_perform() so now we check the HTTP stuff
            http_success = eval_http_get_response(c_handle, target_url, http_code);
            BESDEBUG(MODULE, prolog << "http_success: " << (http_success ? "true" : "false") << endl);
        }
        // If the curl_easy_perform failed, or if the http request failed, then
        // we keep trying until we have exceeded the retry_limit at which point we throw
        // an exception.
        if (!curl_success || !http_success) {
            string effective_url;
            try {
                effective_url = filter_aws_url(get_effective_url(c_handle, target_url));
            }
            catch (BESInternalError &bie) {
                effective_url = "Unable_To_Determine_CURLINFO_EFFECTIVE_URL: " + bie.get_message();
            }
            if (attempts == retry_limit) {
                stringstream msg;
                msg << prolog << "ERROR - Made " << retry_limit << " failed attempts to retrieve the URL ";
                msg << filter_aws_url(target_url) << " The retry limit has been exceeded. Giving up! ";
                msg << "CURLINFO_EFFECTIVE_URL: " << effective_url << " ";
                msg << "Returned HTTP_STATUS: " << http_code;
                throw HttpError(msg.str(),
                                curl_code,
                                http_code,
                                target_url,
                                effective_url,
                                __FILE__, __LINE__);
            } else {
                INFO_LOG(prolog + "Problem with data transfer. Will retry (url: "
                                 + filter_aws_url(target_url) + " attempt: " + std::to_string(attempts) + "). "
                                 + "CURLINFO_EFFECTIVE_URL: " + effective_url + " "
                                 + "Returned HTTP_STATUS: " + std::to_string(http_code));
                usleep(retry_time);
                retry_time *= 2;

                if (fd >= 0)
                    truncate_file(fd);
            }
        }
    }

    // Unset the buffer before it goes out of scope
    unset_error_buffer(c_handle);

    BESDEBUG(MODULE, prolog << "cURL operations completed. fd: " << fd << "\n");

    // rewind the file, if the descriptor is valid
    if (fd >= 0) {
        BESDEBUG(MODULE, prolog << "Rewinding fd(" << fd << ")\n");
        auto status = lseek(fd, 0, SEEK_SET);
        if (-1 == status)
            throw BESInternalError("Could not seek within the response file.", __FILE__, __LINE__);
    }
    BESDEBUG(MODULE, prolog << "END\n");
}

/**
 *
 * Use libcurl to dereference a URL. Read the information referenced by
 * url into the file pointed to by the open file descriptor fd.
 *
 * @todo Continue the shared_ptr<http::url> removal refactor with this method and its callers. jhrg 2/20/25
 *
 * @param target_url The URL to dereference.
 * @param fd  An open file descriptor (as in 'open' as opposed to 'fopen') which
 * will be the destination for the data; the caller can assume that when this
 * method returns that the body of the response can be retrieved by reading
 * from this file descriptor.
 * @param http_response_headers Value/result parameter for the HTTP Response Headers.
 * @param http_request_headers A pointer to a vector of HTTP request headers. Default is
 * null. These headers will be appended to the list of default headers.
 * @exception Error Thrown if libcurl encounters a problem; the libcurl
 * error message is stuffed into the Error object.
 */
void http_get_and_write_resource(const std::shared_ptr<http::url> &target_url, int fd,
                                 vector <string> *http_response_headers) {

    vector<char> error_buffer(CURL_ERROR_SIZE, (char) 0);
    CURLcode res;
    CURL *ceh = nullptr;
    curl_slist *req_headers = nullptr;

    BESDEBUG(MODULE, prolog << "BEGIN" << endl);
    // Before we do anything, make sure that the URL is OK to pursue.
    if (!http::AllowedHosts::theHosts()->is_allowed(target_url)) {
        string err = (string) "The specified URL " + target_url->str()
                     + " does not match any of the accessible services in"
                     + " the allowed hosts list.";
        BESDEBUG(MODULE, prolog << err << endl);
        throw BESSyntaxUserError(err, __FILE__, __LINE__);
    }

    try {
        // Add the EDL authorization headers if the Information is in the BES Context Manager
        req_headers = add_edl_auth_headers(req_headers);
        // Add AWS credentials if they're available.
        req_headers = sign_url_for_s3_if_possible(target_url->str(), req_headers);

        // OK! Make the cURL handle
        ceh = init(target_url->str(), req_headers, http_response_headers);

        set_error_buffer(ceh, error_buffer.data());

        res = curl_easy_setopt(ceh, CURLOPT_WRITEFUNCTION, writeToOpenFileDescriptor);
        eval_curl_easy_setopt_result(res, prolog, "CURLOPT_WRITEFUNCTION", error_buffer.data(), __FILE__, __LINE__);

        // since curl 7.9.7 CURLOPT_FILE is the same as CURLOPT_WRITEDATA.
        res = curl_easy_setopt(ceh, CURLOPT_FILE, &fd);
        eval_curl_easy_setopt_result(res, prolog, "CURLOPT_FILE", error_buffer.data(), __FILE__, __LINE__);

        // We do this because we know super_easy_perform() is going to set it.
        unset_error_buffer(ceh);

        super_easy_perform(ceh, fd);

        // Free the header list
        BESDEBUG(MODULE, prolog << "Cleanup request headers. Calling curl_slist_free_all()." << endl);
        curl_slist_free_all(req_headers);

        if (ceh) {
            curl_easy_cleanup(ceh);
            BESDEBUG(MODULE, prolog << "Called curl_easy_cleanup()." << endl);
        }

    }
    catch (...) {
        curl_slist_free_all(req_headers);
        if (ceh) {
            curl_easy_cleanup(ceh);
        }
        throw;
    }

    BESDEBUG(MODULE, prolog << "END" << endl);
}

/**
 * Returns a cURL error message string based on the contents of the error_buf or, if the error_buf is empty, the
 * CURLcode code.
 * @note Used here and in dmrpp_module in one place. jhrg 3/8/23
 * @param response_code
 * @param error_buf
 * @return A formated error message in a C++ string.
 */
string error_message(const CURLcode response_code, const char *error_buffer) {
    string msg;
    if (error_buffer) {
        msg = string("cURL_error_buffer: ") + error_buffer + ", ";
    }
    msg += string("cURL_message: ") + curl_easy_strerror(response_code) + " (code: "
            + to_string(response_code) + ")\n";
    return msg;
}


static size_t string_write_data(void *buffer, size_t size, size_t nmemb, void *data) {
    auto str = reinterpret_cast<string *>(data);
    size_t nbytes = size * nmemb;
    size_t current_size = str->size();
    str->resize(current_size + nbytes);
    memcpy((void *) (str->data() + current_size), buffer, nbytes);
    return nbytes;
}

/**
 * Dereference the target URL and put the response in buf.
 *
 * @note The intent here is to read data and store it directly into the string.
 * @see This version has not been tested to show that the new data will be
 * appended if the string is not empty.
 *
 * @param target_url The URL to dereference.
 * @param buf The string into which to put the response. New data will be
 * appended to this string.
 * @exception Throws when libcurl encounters a problem.
 */
void http_get(const string &target_url, string &buf) {
    BESDEBUG(MODULE, prolog << "BEGIN\n");
    BES_PROFILE_TIMING(string("Make HTTP GET request - ") + target_url);

    vector<char> error_buffer(CURL_ERROR_SIZE, (char) 0);
    CURL *ceh = nullptr;     ///< The libcurl handle object.
    CURLcode res;
    curl_slist *request_headers = nullptr;

    try {
        // Add the authorization headers
        request_headers = add_edl_auth_headers(request_headers);

        request_headers = sign_url_for_s3_if_possible(target_url, request_headers);

#ifdef DEVELOPER
        AccessCredentials *credentials = CredentialsManager::theCM()->get(target_url);
        if (credentials) {
            INFO_LOG(prolog + "Looking for EDL Token for URL: " + target_url );
            string edl_token = credentials->get("edl_token");
            if (!edl_token.empty()) {
                INFO_LOG(prolog + "Using EDL Token for URL: " + target_url + '\n');
                request_headers = curl::append_http_header(request_headers, "Authorization", edl_token);
            }
        }
#endif

        ceh = curl::init(target_url, request_headers, nullptr);
        if (!ceh)
            throw BESInternalError(string("ERROR! Failed to acquire cURL Easy Handle! "), __FILE__, __LINE__);

        // Error Buffer (for use during this setup) ----------------------------------------------------------------
        set_error_buffer(ceh, error_buffer.data());

        // Pass all data to the 'write_data' function --------------------------------------------------------------
        res = curl_easy_setopt(ceh, CURLOPT_WRITEFUNCTION, string_write_data);
        eval_curl_easy_setopt_result(res, prolog, "CURLOPT_WRITEFUNCTION", error_buffer.data(), __FILE__, __LINE__);

        // Pass this to write_data as the fourth argument ----------------------------------------------------------
        res = curl_easy_setopt(ceh, CURLOPT_WRITEDATA, reinterpret_cast<void *>(&buf));
        eval_curl_easy_setopt_result(res, prolog, "CURLOPT_WRITEDATA", error_buffer.data(), __FILE__, __LINE__);

        // We do this because we know super_easy_perform() is going to set it.
        unset_error_buffer(ceh);

        super_easy_perform(ceh);

        // Free the header list
        BESDEBUG(MODULE, prolog << "Cleanup request headers. Calling curl_slist_free_all()." << endl);
        curl_slist_free_all(request_headers);

        if (ceh) {
            curl_easy_cleanup(ceh);
            BESDEBUG(MODULE, prolog << "Called curl_easy_cleanup()." << endl);
        }
    }
    catch (...) {
        curl_slist_free_all(request_headers);
        if (ceh) {
            curl_easy_cleanup(ceh);
        }
        throw;
    }
    BESDEBUG(MODULE, prolog << "END\n");
}

// used only in one place here. jhrg 3/8/23
static string get_cookie_file_base() {
    return TheBESKeys::read_string_key(HTTP_COOKIES_FILE_KEY, HTTP_DEFAULT_COOKIES_FILE);
}

// used here in init() and clear_cookies (which itself is never used) and in dmrpp_module
// jhrg 3/8/23
string get_cookie_filename() {
    string cookie_file_base = get_cookie_file_base();
    stringstream cf_with_pid;
    cf_with_pid << cookie_file_base << "-" << getpid();
    return cf_with_pid.str();
}

/**
 * @brief Return the location of the netrc file for Hyrax to utilize when making requests for remote resources.
 *
 * If the HTTP_NETRC_FILE_KEY ("Http.netrc.file") is not set, an empty string is returned.
 *
 * @note only used here and in dmrpp_module. jhrg 3/8/23
 *
 * @return The name of the netrc file specified in the configuration, possibly the empty
 * string of none was specified.
 */
string get_netrc_filename() {
    return TheBESKeys::read_string_key(HTTP_NETRC_FILE_KEY, "");
}

/**
 * Set the error buffer for the cURL easy handle ceh to error_buffer
 *
 * @note used here and in dmrpp_module. jhrg 3/8/23
 *
 * @param ceh
 * @param error_buffer
 */
void set_error_buffer(CURL *ceh, char *error_buffer) {
    CURLcode res;
    res = curl_easy_setopt(ceh, CURLOPT_ERRORBUFFER, error_buffer);
    eval_curl_easy_setopt_result(res, prolog, "CURLOPT_ERRORBUFFER", error_buffer, __FILE__, __LINE__);
}

/**
 * A single source of truth for the User-Agent string used by Hyrax.
 *
 * @note used here and in dmrpp_module. jhrg 3/8/23
 *
 * @return The Hyrax User-Agent string.
 */
string hyrax_user_agent() {
    string user_agent;
    bool found;
    TheBESKeys::TheKeys()->get_value(HTTP_USER_AGENT_KEY, user_agent, found);
    if (!found || user_agent.empty()) {
        user_agent = HTTP_DEFAULT_USER_AGENT;
    }
    BESDEBUG(MODULE, prolog << "User-Agent: " << user_agent << endl);
    return user_agent;
}

/**
 * @brief Evaluates the CURLcode returned by curl_easy_setopt()
 *
 * Throws a BESInternalError if curl_code != CURLE_OK
 *
 * A SSOT for this activity, which gets done a bunch.
 *
 * @note used here and in dmrpp_module. jhrg 3/8/23
 *
 * @param result The CURLcode value returned by the call to curl_easy_setopt()
 * @param msg_base The prefix for any error message that gets generated.
 * @param opt_name The string name of the option that was set.
 * @param ebuf The cURL error buffer associated with the cURL easy handle
 * at the time of the setopt call
 * @param file The value of __FILE__ of the calling function.
 * @param line The value of __LINE__ of the calling function.
 */
void eval_curl_easy_setopt_result(CURLcode curl_code, const string &msg_base, const string &opt_name,
                                  const char *ebuf, const string &file, unsigned int line) {
    if (curl_code != CURLE_OK) {
        stringstream msg;
        msg << msg_base << "ERROR - cURL failed to set " << opt_name << " Message: "
            << curl::error_message(curl_code, ebuf);
        throw BESInternalError(msg.str(), file, line);
    }
}

// Used here and in dmrpp_module. jhrg 3/8/23
unsigned long max_redirects() {
    return http::load_max_redirects_from_keys();
}

/**
 * @brief Add the given header & value to the curl slist.
 *
 * The caller must free the slist after the curl_easy_perform() is called, not after
 * the headers are added to the curl handle.
 *
 * @note used here and in dmrpp_module. jhrg 3/8/23
 *
 * @param slist The list; initially pass nullptr to create a new list
 * @param header The header
 * @param value The value
 * @return The modified slist pointer or nullptr if an error occurred.
 */
curl_slist *append_http_header(curl_slist *slist, const string &header_name, const string &value) {

    string full_header = header_name;
    full_header.append(": ").append(value);

    BESDEBUG(MODULE, prolog << full_header << endl);

    auto temp = curl_slist_append(slist, full_header.c_str());
    if (!temp) {
        stringstream msg;
        msg << prolog << "Encountered cURL Error setting the " << header_name << " header. full_header: "
            << full_header;
        throw BESInternalError(msg.str(), __FILE__, __LINE__);
    }
    return temp;
}

/**
 * @brief Adds the user id and/or the associated EDL auth token
 * to request_headers.
 *
 * @note used here and in the tests. jhrg 3/8/23
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
        request_headers = append_http_header(request_headers, "User-Id", s);
    }

    s = BESContextManager::TheManager()->get_context(EDL_AUTH_TOKEN_KEY, found);
    if (found && !s.empty()) {
        request_headers = append_http_header(request_headers, "Authorization", s);
    }

    s = BESContextManager::TheManager()->get_context(EDL_ECHO_TOKEN_KEY, found);
    if (found && !s.empty()) {
        request_headers = append_http_header(request_headers, "Echo-Token", s);
    }

    return request_headers;
}

/**
 * @brief Sign a URL for S3
 *
 * @note used here and in the tests. jhrg 3/8/23
 *
 * Given a URL that references an object in an S3 bucket for which the server
 * has credentials, sign that URL using the AWS V4 signing algorithm and put
 * the resulting headers in the list of HTTP/S request headers.
 *
 * This method modifies the third argument. It _appends_ three new headers,
 * so if the list of request headers was empty, the list will start with an
 * element where the 'data' component is NULL.
 *
 * @param target_url The URL that should be signed
 * @param ac AccessCredentials instance that will not be modified (not const
 * because some of the methods used modify internal state).
 * @param req_headers The header list that will hold the Authorization, etc.,
 * headers.
 * @param The modified list of request headers
 */
curl_slist *
sign_s3_url(const string &target_url, AccessCredentials *ac, curl_slist *req_headers) {
    BES_PROFILE_TIMING(string("Sign url - ") + target_url);
    const time_t request_time = time(nullptr);
    const auto url_obj = http::url(target_url); // parse the URL using the http::url object. jhrg 2/20/25
    const string auth_header = compute_awsv4_signature(url_obj.path(), url_obj.query(), url_obj.host(),
                                                       request_time, ac->get(AccessCredentials::ID_KEY),
                                                       ac->get(AccessCredentials::KEY_KEY),
                                                       ac->get(AccessCredentials::REGION_KEY), "s3");

    BESDEBUG(MODULE, prolog << "Authorization: " << auth_header << "\n");
    req_headers = append_http_header(req_headers, "Authorization", auth_header);
    req_headers = append_http_header(req_headers, "x-amz-content-sha256",
                                     "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
    req_headers = append_http_header(req_headers, "x-amz-date", AWSV4::ISO8601_date(request_time));
    INFO_LOG(prolog + "Signed S3 request for " + target_url);

    return req_headers;
}

/**
 * @brief Support the shared_ptr<http::url> interface of this function.
 * @todo Remove when no longer needed. jhrg 2/20/25
 * @param target_url
 * @param ac
 * @param req_headers
 * @return
 */
curl_slist *
sign_s3_url(const shared_ptr <url> &target_url, AccessCredentials *ac, curl_slist *req_headers) {
    return sign_s3_url(target_url->str(), ac, req_headers);
}

/**
 * @brief Checks if a URL has been signed for S3.
 *
 * This function looks for the presence of specific query parameters
 * that are typically used in S3 signed URLs.
 *
 * @param url The URL to check.
 * @return True if the URL is signed for S3, false otherwise.
 */
bool is_url_signed_for_s3(const std::string &url) {
    return url.find("X-Amz-Algorithm=") != string::npos &&
           url.find("X-Amz-Credential=") != string::npos &&
           url.find("X-Amz-Signature=") != string::npos;
}

/**
 * @brief Checks if a URL has been signed for S3.
 *
 * This function looks for the presence of specific query parameters
 * that are typically used in S3 signed URLs.
 *
 * @param target_url The URL to check.
 * @return True if the URL is signed for S3, false otherwise.
 */
bool is_url_signed_for_s3(const std::shared_ptr<http::url> &target_url) {
    return is_url_signed_for_s3(target_url->str());
}

/**
 * @brief Returns an cURL easy handle for recovering the location
 * from a redirects response
 *
 * The returned cURL easy handle will write the response to a string
 * and it will NOT follow redirects.
 *
 * @param target_url The URL to target
 * @param req_headers A curl_slist containing any necessary request headers
 * to be transmitted with the HTTP request.
 * @param resp_hdrs A vector into which any response headers associated
 * the servers response will be placed.
 * @param response_body The returned response body.
 * @return A cURL easy handle configured as described above,
 */
static CURL *init_no_follow_redirects_handle(const string &target_url, const curl_slist *req_headers,
                                             vector <string> &resp_hdrs, string &response_body) {

    vector<char> error_buffer(CURL_ERROR_SIZE, (char) 0);
    CURL *ceh = curl::init(target_url, req_headers, &resp_hdrs);

    set_error_buffer(ceh, error_buffer.data());

    // Pass all data to the 'write_data' function --------------------------------------------------------------
    CURLcode res = curl_easy_setopt(ceh, CURLOPT_WRITEFUNCTION, string_write_data);
    eval_curl_easy_setopt_result(res, prolog, "CURLOPT_WRITEFUNCTION", error_buffer.data(), __FILE__, __LINE__);

    // Pass this to write_data as the fourth argument ----------------------------------------------------------
    res = curl_easy_setopt(ceh, CURLOPT_WRITEDATA, reinterpret_cast<void *>(&response_body));
    eval_curl_easy_setopt_result(res, prolog, "CURLOPT_WRITEDATA", error_buffer.data(), __FILE__, __LINE__);

    // Pass save_raw_http_headers() a pointer to the vector<string> where the
    // response headers may be stored. Callers can use the resp_hdrs
    // value/result parameter to get the raw response header information .
    res = curl_easy_setopt(ceh, CURLOPT_WRITEHEADER, &resp_hdrs);
    eval_curl_easy_setopt_result(res, prolog, "CURLOPT_WRITEHEADER", error_buffer.data(), __FILE__, __LINE__);

    // DO NOT Follow 302/306 (redirect) responses
    res = curl_easy_setopt(ceh, CURLOPT_FOLLOWLOCATION, 0L);
    eval_curl_easy_setopt_result(res, prolog, "CURLOPT_FOLLOWLOCATION", error_buffer.data(), __FILE__, __LINE__);

    unset_error_buffer(ceh);
    return ceh;
}


/**
 *
 * @param http_code
 * @param response_headers
 * @param response_body
 * @param msg
 */
void write_response_details(const long http_code,
                            const vector <string> &response_headers,
                            const string &response_body,
                            stringstream &msg) {
    msg << "# -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --\n";
    msg << "HTTP Response Details\n";
    msg << "The remote service returned an HTTP code of: " << http_code << "\n";
    msg << "Response Headers -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --\n";
    for (const auto &hdr: response_headers) {
        msg << "  " << hdr << "\n";
    }
    msg << "# BEGIN Response Body -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --\n";
    msg << response_body << "\n";
    msg << "# END Response Body   -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --\n";
}

/**
 * #brief Error handling for the gru_mk_attempt()
 * @param http_code The http code of the response
 * @param response_headers <--
 * @param response_body <--
 * @param redirect_url_str The redirect url string, might be empty if there was a non 3xx response.
 * @param origin_url_str The origin url
 * @param attempt The attempt number of the current attempt
 * @param max_attempts The maximum number af attempts allowed
 * @return
 */
bool process_get_redirect_http_code(const long http_code,
                                    const vector <string> &response_headers,
                                    const string &response_body,
                                    const string &redirect_url_str,
                                    const string &origin_url_str,
                                    const unsigned int attempt,
                                    const unsigned int max_attempts) {
    bool success = false;
    switch (http_code) {
        case 301: // Moved Permanently
        case 302: // Found (fka Move Temporarily)
        case 303: // See Other
        case 307: // Temporary Redirect
        case 308: // Permanent Redirect
        {
            // Check for EDL redirect
            http::url rdu(redirect_url_str);
            if (rdu.host().find("urs.earthdata.nasa.gov") != string::npos) {
                if (attempt >= max_attempts) {
                    stringstream msg;
                    msg << prolog << "ERROR - I tried " << attempt << " times to access the url:\n";
                    msg << "    " << origin_url_str << "\n";
                    msg << "It seems that the provided access credentials are either missing, invalid, or expired.\n";
                    msg << "Here are the details from the most recent attempt:\n\n";
                    write_response_details(http_code, response_headers, response_body, msg);
                    throw BESSyntaxUserError(msg.str(), __FILE__, __LINE__);
                }
                //  EDL is not the redirect we were looking for...
                success = false;
            } else {
                success = true;
            }
            break;
        }

        default: {
            if (attempt >= max_attempts) {
                // Everything else is bad.
                stringstream msg;
                msg << prolog << "ERROR -  I tried " << attempt << " times to access:\n";
                msg << "    " << origin_url_str << "\n";
                msg << "I was expecting to receive an HTTP redirect code and location header in the response. \n";
                msg << "Unfortunately this did not happen.\n";
                msg << "Here are the details of the most recent transaction:\n\n";
                write_response_details(http_code, response_headers, response_body, msg);
                throw HttpError(msg.str(),
                                CURLE_OK,
                                http_code,
                                origin_url_str,
                                redirect_url_str,
                                response_headers,
                                response_body,
                                __FILE__, __LINE__);
            }
            success = false;
            break;
        }
    }
    return success;
}

/**
 * @brief  Make a single attempt to acquire a redirect response from origin_url
 * @param origin_url The URL to access
 * @param attempt The attempt number of this effort.
 * @param max_attempts The maximum number of attempts allowed.
 * @param redirect_url A returned value parameter to receive the redirect url, if located.
 * @return true if the redirect url was found, false otherwise.
 */
static bool gru_mk_attempt(const shared_ptr <url> &origin_url,
                    const unsigned int attempt,
                    const unsigned int max_attempts,
                    shared_ptr <EffectiveUrl> &redirect_url) {

    BESDEBUG(MODULE, prolog << " BEGIN This is attempt #" << attempt << " for " << origin_url->str() << "\n");
    bool http_success = false;
    bool curl_success = false;
    CURL *ceh = nullptr;
    vector<char> error_buffer(CURL_ERROR_SIZE, (char) 0);
    curl_slist *req_headers = nullptr;

    vector<string> response_headers;
    string response_body;
    CURLcode curl_code;
    long http_code;
    string redirect_url_str;

    // Add the EDL authorization headers if the Information is in the BES Context Manager
    req_headers = add_edl_auth_headers(req_headers);
    req_headers = sign_url_for_s3_if_possible(origin_url, req_headers);

    // FIXME Hackery for DMR++ Ownership POC code - see dmrpp_module CurlHandlePool.cc
    //  for more info. jhrg 5/24/24
    AccessCredentials *credentials = CredentialsManager::theCM()->get(origin_url);
    if (credentials) {
        INFO_LOG(prolog + "Looking for EDL Token for URL: " + origin_url->str() + '\n');
        string edl_token = credentials->get("edl_token");
        if (!edl_token.empty()) {
            INFO_LOG(prolog + "Using EDL Token for URL: " + origin_url->str() + '\n');
            req_headers = curl::append_http_header(req_headers, "Authorization", edl_token);
        }
    }

    try {

        // OK! Make the cURL handle
        ceh = init_no_follow_redirects_handle(
                origin_url->str(),
                req_headers,
                response_headers,
                response_body);

#ifndef NDEBUG
        {
            BES_STOPWATCH_START(MODULE,prolog + "Retrieved HTTP response from origin_url: " + origin_url->str());
#endif

            curl_code = curl_easy_perform(ceh);
#ifndef NDEBUG
        }
#endif
        curl_success = eval_curl_easy_perform_code(
                origin_url->str(), // In this situation we use the origin url because we did NOT follow a redirect
                curl_code,
                error_buffer.data(),
                attempt);

        if (curl_success) {
            http_code = get_http_code(ceh);
            char *url = nullptr;
            curl_easy_getinfo(ceh, CURLINFO_REDIRECT_URL, &url);
            if (url) {
                redirect_url_str = url;
            }
            BESDEBUG(MODULE, prolog << "redirect_url_str: " << redirect_url_str << "\n");
            http_success = process_get_redirect_http_code(http_code,
                                                          response_headers,
                                                          response_body,
                                                          redirect_url_str,
                                                          origin_url->str(),
                                                          attempt,
                                                          max_attempts);
            if (http_success) {
                redirect_url = make_shared<http::EffectiveUrl>(redirect_url_str,
                                                               response_headers,
                                                               origin_url->is_trusted());
            }
        } else if (attempt >= max_attempts) {
            // Everything is bad now.
            stringstream msg;
            msg << prolog << "ERROR -  I tried " << attempt << " times to access:\n";
            msg << "    " << origin_url << "\n";
            msg << "I was expecting to receive an HTTP redirect code and location header in the response. \n";
            msg << "Unfortunately this did not happen.\n";
            msg << "This failure appears to be a problem with cURL.\n";
            msg << "The cURL message associated with the most recent failure is:\n";
            msg << "    " << error_message(curl_code, error_buffer.data()) << "\n";
            throw BESInternalError(msg.str(), __FILE__, __LINE__);
        }

        // Free the header list
        curl_slist_free_all(req_headers);
        // clean up cURL handle
        if (ceh) {
            curl_easy_cleanup(ceh);
            BESDEBUG(MODULE, prolog << "Called curl_easy_cleanup()." << "\n");
        }

    }
    catch (...) {
        // Free the header list
        curl_slist_free_all(req_headers);

        // clean up cURL handle
        if (ceh) {
            curl_easy_cleanup(ceh);
            BESDEBUG(MODULE, prolog << "Called curl_easy_cleanup()." << "\n");
        }
        throw;
    }
    BESDEBUG(MODULE, prolog << "curl_success: " << (curl_success ? "true" : "false") << "\n");
    BESDEBUG(MODULE, prolog << "http_success: " << (http_success ? "true" : "false") << "\n");
    BESDEBUG(MODULE, prolog << " END success: " << ((curl_success && http_success) ? "true" : "false") <<
                            " on attempt #" << attempt << " for " << origin_url->str() << "\n");

    return curl_success && http_success;
}

/**
 * @brief Gets the redirect url returned by the origin_url
 * The assumption is that the origin_url will always return a redirect, thus
 * an http code of 2xx, 4xx, or 5xx is considered an error.
 *
 * @param origin_url The origin url for the request
 * @param redirect_url Returned value parameter for the redirect url.
 * @return The redirect URL string.
 */
std::shared_ptr<http::EffectiveUrl> get_redirect_url(const std::shared_ptr<http::url> &origin_url) {

    BESDEBUG(MODULE, prolog << "BEGIN" << endl);
    // Before we do anything, make sure that the URL is OK to pursue.
    if (!http::AllowedHosts::theHosts()->is_allowed(origin_url)) {
        string err = (string) "The specified URL " + origin_url->str()
                     + " does not match any of the accessible services in"
                     + " the allowed hosts list.";
        BESDEBUG(MODULE, prolog << err << endl);
        throw BESSyntaxUserError(err, __FILE__, __LINE__);
    }

    std::shared_ptr<http::EffectiveUrl> redirect_url;

    unsigned int attempt = 0;
    bool success = false;

    while (!success && (attempt < retry_limit)) {
        attempt++;
        success = gru_mk_attempt(origin_url, attempt, retry_limit, redirect_url);
    }
    // This is a failsafe test - the gru_mk_attempt)_ should detect the errors and throw an exception
    // if the attempt count exceeds the retry_limit, but if for some reason there's flaw in that
    // logic I add this check as well... ndp-12/01/23
    if (attempt >= retry_limit) {
        stringstream msg;
        msg << prolog << "ERROR: I tried " << attempt << " times to determine the redirect URL for the origin_url:\n";
        msg << "    " << origin_url->str() << "\n";
        msg << "Oddly, I was unable to detect an error, but nonetheless I have made the maximum ";
        msg << "number of attempts and I must now give up...\n";
        throw BESInternalError(msg.str(), __FILE__, __LINE__);
    }

    BESDEBUG(MODULE, prolog << "END redirect_url: " << redirect_url->str() << "\n");
    return redirect_url;
}


} /* namespace curl */
