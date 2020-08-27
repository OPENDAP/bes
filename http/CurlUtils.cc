// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of the BES http package, part of the Hyrax data server.

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

#include <curl/curl.h>
#include <cstdio>
#include <sstream>
#include <map>
#include <vector>
#include <unistd.h>
#include <algorithm>    // std::for_each
#include <time.h>

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

#include "BESDebug.h"
#include "BESSyntaxUserError.h"
#include "HttpNames.h"
#include "HttpUtils.h"
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
static const useconds_t uone_second = 1000*1000; // one second in micro seconds (which is 1000


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


/** This function translates an HTTP status code into an error message. It
    works for those code greater than or equal to 400. */
    string http_status_to_string(int status) {
        if (status >= CLIENT_ERR_MIN && status <= CLIENT_ERR_MAX)
            return string(http_client_errors[status - CLIENT_ERR_MIN]);
        else if (status >= SERVER_ERR_MIN && status <= SERVER_ERR_MAX)
            return string(http_server_errors[status - SERVER_ERR_MIN]);
        else
            return string(
                    "Unknown Error: This indicates a problem with libdap++.\nPlease report this to support@opendap.org.");
    }

    static string getCurlAuthTypeName(const int authType) {

        string authTypeString;
        int match;

        match = authType & CURLAUTH_BASIC;
        if (match) {
            authTypeString += "CURLAUTH_BASIC";
        }

        match = authType & CURLAUTH_DIGEST;
        if (match) {
            if (!authTypeString.empty())
                authTypeString += " ";
            authTypeString += "CURLAUTH_DIGEST";
        }

        match = authType & CURLAUTH_DIGEST_IE;
        if (match) {
            if (!authTypeString.empty())
                authTypeString += " ";
            authTypeString += "CURLAUTH_DIGEST_IE";
        }

        match = authType & CURLAUTH_GSSNEGOTIATE;
        if (match) {
            if (!authTypeString.empty())
                authTypeString += " ";
            authTypeString += "CURLAUTH_GSSNEGOTIATE";
        }

        match = authType & CURLAUTH_NTLM;
        if (match) {
            if (!authTypeString.empty())
                authTypeString += " ";
            authTypeString += "CURLAUTH_NTLM";
        }

#if 0
        match = authType & CURLAUTH_ANY;
    if(match){
        if(!authTypeString.empty())
            authTypeString += " ";
        authTypeString += "CURLAUTH_ANY";
    }


    match = authType & CURLAUTH_ANY;
    if(match){
        if(!authTypeString.empty())
            authTypeString += " ";
        authTypeString += "CURLAUTH_ANYSAFE";
    }


    match = authType & CURLAUTH_ANY;
    if(match){
        if(!authTypeString.empty())
            authTypeString += " ";
        authTypeString += "CURLAUTH_ONLY";
    }
#endif

        return authTypeString;
    }

    /**
     * libcurl call back function that ignores the data entirely. nothing is written. Ever.
     */
    static size_t writeNothing(char */* data */, size_t /* size */, size_t nmemb, void * /* userdata */) {
        return nmemb;
    }

    /**
     * libcurl call back function that is used to write data to a passed open file descriptor (that would
     * be instead of the default open FILE *)
     */
    static size_t writeToOpenfileDescriptor(char *data, size_t /* size */, size_t nmemb, void *userdata) {

        int *fd = (int *) userdata;

        BESDEBUG(MODULE, prolog << "Bytes received " << nmemb << endl);
        int wrote = write(*fd, data, nmemb);
        BESDEBUG(MODULE, prolog << "Bytes written " << wrote << endl);

        return wrote;
    }


    /**
     * A libcurl callback function used to read response headers. Read headers,
     * line by line, from ptr. The fourth param is really supposed to be a FILE,
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
    static size_t save_raw_http_headers(void *ptr, size_t size, size_t nmemb, void *resp_hdrs) {
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


/** A libcurl callback for debugging protocol issues. */
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


/** Functor to add a single string to a curl_slist. This is used to transfer
    a list of headers from a vector<string> object to a curl_slist. */
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
 * Configure the proxy options for the passed curl object. The passed URL is the target URL. If the target URL
 * matches the Gateway.NoProxyRegex in the config file, then no proxying is done.
 *
 * The proxy configuration is stored in the gateway_modules configuration file, gateway.conf. The allowed values are:
 * Gateway.ProxyHost=warsaw.wonderproxy.com
 * Gateway.ProxyPort=8080
 * Gateway.ProxyUser=username
 * Gateway.ProxyPassword=password
 * Gateway.ProxyUserPW=username:password
 * Gateway.ProxyAuthType=basic | digest | ntlm
 *
 */
    bool configureProxy(CURL *curl, const string &url) {
        BESDEBUG(MODULE,  prolog << "BEGIN." << endl);

        bool using_proxy = false;

        // I pulled this because I could never find where it was applied
        // to the curl state in HTTPConnect
        //string proxyProtocol = GatewayUtils::ProxyProtocol;

        string proxyHost = HttpUtils::ProxyHost;
        int proxyPort = HttpUtils::ProxyPort;
        string proxyPassword = HttpUtils::ProxyPassword;
        string proxyUser = HttpUtils::ProxyUser;
        string proxyUserPW = HttpUtils::ProxyUserPW;
        int proxyAuthType = HttpUtils::ProxyAuthType;

        if (!proxyHost.empty()) {
            using_proxy = true;
            if (proxyPort == 0)
                proxyPort = 8080;

            // Apparently we don't need this...
            //if(proxyProtocol.empty())
            // proxyProtocol = "http";

        }
        if (using_proxy) {
            BESDEBUG(MODULE,  prolog << "Found proxy configuration." << endl);

            // Don't set up the proxy server for URLs that match the 'NoProxy'
            // regex set in the gateway.conf file.

            // Don't create the regex if the string is empty
            if (!HttpUtils::NoProxyRegex.empty()) {
                BESDEBUG(MODULE,  prolog << "Found NoProxyRegex." << endl);
                BESRegex r(HttpUtils::NoProxyRegex.c_str());
                if (r.match(url.c_str(), url.length()) != -1) {
                    BESDEBUG(MODULE, prolog << "Found NoProxy match. Regex: " << HttpUtils::NoProxyRegex << "; Url: " << url << endl);
                    using_proxy = false;
                }
            }

            if (using_proxy) {

                BESDEBUG(MODULE, prolog << "Setting up a proxy server." << endl);
                BESDEBUG(MODULE, prolog << "Proxy host: " << proxyHost << endl);
                BESDEBUG(MODULE, prolog << "Proxy port: " << proxyPort << endl);

                curl_easy_setopt(curl, CURLOPT_PROXY, proxyHost.data());
                curl_easy_setopt(curl, CURLOPT_PROXYPORT, proxyPort);

// #ifdef CURLOPT_PROXYAUTH

                // oddly "#ifdef CURLOPT_PROXYAUTH" doesn't work - even though CURLOPT_PROXYAUTH is defined and valued at 111 it
                // fails the test. Eclipse hover over the CURLOPT_PROXYAUTH symbol shows: "CINIT(PROXYAUTH, LONG, 111)",
                // for what that's worth

                // According to http://curl.haxx.se/libcurl/c/curl_easy_setopt.html#CURLOPTPROXYAUTH As of 4/21/08 only NTLM, Digest and Basic work.

#if 0
            BESDEBUG(MODULE,  prolog << "CURLOPT_PROXYAUTH       = " << CURLOPT_PROXYAUTH << endl);
            BESDEBUG(MODULE,  prolog << "CURLAUTH_BASIC          = " << CURLAUTH_BASIC << endl);
            BESDEBUG(MODULE,  prolog << "CURLAUTH_DIGEST         = " << CURLAUTH_DIGEST << endl);
            BESDEBUG(MODULE,  prolog << "CURLAUTH_DIGEST_IE      = " << CURLAUTH_DIGEST_IE << endl);
            BESDEBUG(MODULE,  prolog << "CURLAUTH_GSSNEGOTIATE   = " << CURLAUTH_GSSNEGOTIATE << endl);
            BESDEBUG(MODULE,  prolog << "CURLAUTH_NTLM           = " << CURLAUTH_NTLM << endl);
            BESDEBUG(MODULE,  prolog << "CURLAUTH_ANY            = " << CURLAUTH_ANY << endl);
            BESDEBUG(MODULE,  prolog << "CURLAUTH_ANYSAFE        = " << CURLAUTH_ANYSAFE << endl);
            BESDEBUG(MODULE,  prolog << "CURLAUTH_ONLY           = " << CURLAUTH_ONLY << endl);
            BESDEBUG(MODULE,  prolog << "Using CURLOPT_PROXYAUTH = " << proxyAuthType << endl);
#endif

                BESDEBUG(MODULE, prolog << "Using CURLOPT_PROXYAUTH = " << getCurlAuthTypeName(proxyAuthType) << endl);
                curl_easy_setopt(curl, CURLOPT_PROXYAUTH, proxyAuthType);
// #endif



                if (!proxyUser.empty()) {
                    curl_easy_setopt(curl, CURLOPT_PROXYUSERNAME, proxyUser.data());
                    BESDEBUG(MODULE,  prolog << "CURLOPT_PROXYUSER : " << proxyUser << endl);

                    if (!proxyPassword.empty()) {
                        curl_easy_setopt(curl, CURLOPT_PROXYPASSWORD, proxyPassword.data());
                        BESDEBUG(MODULE,
                                  prolog << "CURLOPT_PROXYPASSWORD: " << proxyPassword << endl);
                    }
                } else if (!proxyUserPW.empty()) {
                    BESDEBUG(MODULE,
                              prolog << "CURLOPT_PROXYUSERPWD : " << proxyUserPW << endl);
                    curl_easy_setopt(curl, CURLOPT_PROXYUSERPWD, proxyUserPW.data());
                }

            }
        }
        BESDEBUG(MODULE,  prolog << "END." << endl);

        return using_proxy;
    }


/**
 * Get's a new instance of CURL* and performs basic configuration of that instance.
 *  - Accept compressed responses
 *  - Any authentication type
 *  - Follow redirects
 *  - User agent set to curl versio.
 *
 *  @param url The url used to configure the proy.
 */
    CURL *init(char *error_buffer) {
        error_buffer[0]=0; // Null terminate this string for safety.
        CURL *curl = curl_easy_init();
        if (!curl)
            throw BESInternalError("Could not initialize libcurl.",__FILE__, __LINE__);

            // Load in the default headers to send with a request. The empty Pragma
            // headers overrides libcurl's default Pragma: no-cache header (which
            // will disable caching by Squid, etc.).

            // the empty Pragma never appears in the outgoing headers when this isn't present
            // d_request_headers->push_back(string("Pragma: no-cache"));

            // d_request_headers->push_back(string("Cache-Control: no-cache"));

            // Allow compressed responses. Sending an empty string enables all supported compression types.
#ifndef CURLOPT_ACCEPT_ENCODING
        curl_easy_setopt(curl, CURLOPT_ENCODING, "");
#else
        curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "");
#endif
        curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, error_buffer);
        // We have to set FailOnError to false for any of the non-Basic
        // authentication schemes to work. 07/28/03 jhrg
        curl_easy_setopt(curl, CURLOPT_FAILONERROR, 0);

        // This means libcurl will use Basic, Digest, GSS Negotiate, or NTLM,
        // choosing the the 'safest' one supported by the server.
        // This requires curl 7.10.6 which is still in pre-release. 07/25/03 jhrg
        curl_easy_setopt(curl, CURLOPT_HTTPAUTH, (long) CURLAUTH_ANY);

        // I added these next three to support Hyrax accessing data held behind URS auth. ndp - 8/20/18
        curl_easy_setopt(curl, CURLOPT_NETRC, 1);

        // #TODO #FIXME Make these file names configuration based.
        curl_easy_setopt(curl, CURLOPT_COOKIEFILE, curl::get_cookie_filename().c_str());
        curl_easy_setopt(curl, CURLOPT_COOKIEJAR, curl::get_cookie_filename().c_str());

        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1);
        curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
        curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, save_raw_http_headers);
        // In read_url a call to CURLOPT_WRITEHEADER is used to set the fourth
        // param of save_raw_http_headers to a vector<string> object.

        // Follow 302 (redirect) responses
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
        curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 5);

        // Set the user agent to curls version response because, well, that's what command line curl does :)
        curl_easy_setopt(curl, CURLOPT_USERAGENT, curl_version());

#if 0
        // If the user turns off SSL validation...
    if (!d_rcr->get_validate_ssl() == 0) {
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);
    }

    // Look to see if cookies are turned on in the .dodsrc file. If so,
    // activate here. We honor 'session cookies' (cookies without an
    // expiration date) here so that session-base SSO systems will work as
    // expected.
    if (!d_cookie_jar.empty()) {
        BESDEBUG(cerr <<  prolog << "Setting the cookie jar to: " << d_cookie_jar << endl);
        curl_easy_setopt(curl, CURLOPT_COOKIEJAR, d_cookie_jar.c_str());
        curl_easy_setopt(curl, CURLOPT_COOKIESESSION, 1);
    }
#endif

        if (curl_trace) {
            BESDEBUG(MODULE,  prolog << "Curl version: " << curl_version() << endl);
            curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
            BESDEBUG(MODULE,  prolog << "Curl in verbose mode." << endl);
            curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, curl_debug);
            BESDEBUG(MODULE,  prolog << "Curl debugging function installed." << endl);
        }
        BESDEBUG(MODULE,  prolog << "curl: " << curl << endl);
        return curl;
    }

    string get_range_arg_string(const unsigned long long &offset, const unsigned long long &size)
    {
        ostringstream range;   // range-get needs a string arg for the range
        range << offset << "-" << offset + size - 1;
        BESDEBUG(MODULE,  prolog << " range: " << range.str() << endl);
        return range.str();
    }


    CURL *init_effective_url_retriever_handle(const string url, vector<string> &resp_hdrs, char *error_buffer){
        CURL *curl = 0;
        curl = init(error_buffer);
        string err_msg_base = "cURL Error setting ";
        // set target URL.
        CURLcode res = curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        if (res != CURLE_OK)
            throw BESInternalError((err_msg_base + "CURLOPT_URL: ").append(error_message(res, error_buffer)), __FILE__, __LINE__);

        // get the offset to offset + size bytes
        res = curl_easy_setopt(curl, CURLOPT_RANGE, get_range_arg_string(0,4).c_str());
        if (res != CURLE_OK)
            throw BESInternalError((err_msg_base+"CURLOPT_RANGE: ").append(error_message(res, error_buffer)), __FILE__,
                                   __LINE__);

        res = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeNothing);
        if (res != CURLE_OK)
            throw BESInternalError((err_msg_base+"CURLOPT_WRITEFUNCTION: ").append(
                    error_message(res, error_buffer)), __FILE__, __LINE__);

        // Pass save_raw_http_headers() a pointer to the vector<string> where the
        // response headers may be stored. Callers can use the resp_hdrs
        // value/result parameter to get the raw response header information .
        res = curl_easy_setopt(curl, CURLOPT_WRITEHEADER, &resp_hdrs);
        if (res != CURLE_OK)
            throw BESInternalError((err_msg_base+"CURLOPT_WRITEHEADER: ").append(
                    error_message(res, error_buffer)), __FILE__, __LINE__);

        return curl;
    }


    /** Use libcurl to dereference a URL. Read the information referenced by \c
    url into the file pointed to by the open file descriptor \c fd.

    @param url The URL to dereference.
    @param fd  An open file descriptor (as in 'open' as opposed to 'fopen') which
    will be the destination for the data; the caller can assume that when this
    method returns that the body of the response can be retrieved by reading
    from this file descriptor.
    @param resp_hdrs Value/result parameter for the HTTP Response Headers.
    @param request_headers A pointer to a vector of HTTP request headers. Default is
    null. These headers will be appended to the list of default headers.
    @return The HTTP status code.
    @exception Error Thrown if libcurl encounters a problem; the libcurl
    error message is stuffed into the Error object.
    */
    void read_url(CURL *curl,
                  const string &url,
                  int fd,
                  vector<string> *resp_hdrs,
                  const vector<string> *request_headers) {

        BESDEBUG(MODULE, prolog << "BEGIN" << endl);

        // Before we do anything, make sure that the URL is OK to pursue.
        if (!bes::AllowedHosts::theHosts()->is_allowed(url)) {
            string err = (string) "The specified URL " + url
                         + " does not match any of the accessible services in"
                         + " the white list.";
            BESDEBUG(MODULE, prolog << err << endl);
            throw BESSyntaxUserError(err, __FILE__, __LINE__);
        }

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeToOpenfileDescriptor);

#ifdef CURLOPT_WRITEDATA
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &fd);
#else
        curl_easy_setopt(curl, CURLOPT_FILE, &fd);
#endif
        //DBG(copy(d_request_headers.begin(), d_request_headers.end(), ostream_iterator<string>(cerr, "\n")));
        BuildHeaders req_hdrs;
        //req_hdrs = for_each(d_request_headers.begin(), d_request_headers.end(), req_hdrs);
        if (request_headers)
            req_hdrs = for_each(request_headers->begin(), request_headers->end(), req_hdrs);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, req_hdrs.get_headers());

        // Pass save_raw_http_headers() a pointer to the vector<string> where the
        // response headers may be stored. Callers can use the resp_hdrs
        // value/result parameter to get the raw response header information .
        curl_easy_setopt(curl, CURLOPT_WRITEHEADER, resp_hdrs);

#if 0
        // This call is the one that makes curl go get the thing.
        CURLcode res = curl_easy_perform(curl);
#endif

        read_data(curl);

        // Free the header list and null the value in d_curl.
        curl_slist_free_all(req_hdrs.get_headers());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, 0);

#if 0
        if (res != 0) {
            BESDEBUG(MODULE, prolog << "OUCH! CURL returned an error! curl msg:  " << curl_easy_strerror(res) << endl);
            BESDEBUG(MODULE, prolog << "OUCH! CURL returned an error! error_buffer:  " << error_buffer << endl);
            throw libdap::Error(error_buffer);
        }
        long status;
        res = curl_easy_getinfo(curl, CURLINFO_HTTP_CODE, &status);
        BESDEBUG(MODULE, prolog << "HTTP Status " << status << endl);
        if (res != CURLE_OK)
            throw libdap::Error(error_buffer);
#endif

        BESDEBUG(MODULE, prolog << "END" << endl);
    }

    /**
     * Returns a cURL error message string based on the conents of the error_buf or, if the error_buf is empty, the
     * CURLcode code.
     * @param response_code
     * @param error_buf
     * @return
     */
    string error_message(const CURLcode response_code, char *error_buf) {
        std::ostringstream oss;
        size_t len = strlen(error_buf);
        if (len) {
            oss << error_buf;
            oss << " (code: " << (int) response_code << ")";
        } else {
            oss << curl_easy_strerror(response_code) << "(result: " << response_code << ")";
        }
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
    std::string http_get_as_string(const std::string &target_url){

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
    rapidjson::Document http_get_as_json(const std::string &target_url){

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
        // char name[] = "/tmp/ngap_cookiesXXXXXX";
        string cf_name = get_cookie_filename();
        if (cf_name.empty())
            throw BESInternalError(string(prolog + "Failed to make temporary file for HTTP cookies in module 'ngap' (").append(strerror(errno)).append(")"), __FILE__, __LINE__);

        try {
            CURL *c_handle = curl::set_up_easy_handle(target_url, cf_name, response_buf);
            read_data(c_handle);
            curl_easy_cleanup(c_handle);
            unlink(cf_name.c_str());
        }
        catch(...) {
            unlink(cf_name.c_str());
            throw;
        }
    }

    /**
     *
     * @param target_url
     * @param cookies_file
     * @param response_buff
     * @return
     */
    CURL *set_up_easy_handle(const string &target_url, const string &cookies_file, char *response_buff) {
        char d_errbuf[CURL_ERROR_SIZE]; ///< raw error message info from libcurl
        CURL *d_handle;     ///< The libcurl handle object.
        d_handle = curl_easy_init();
        if (!d_handle)
            throw BESInternalError(string("ERROR! Failed to acquire cURL Easy Handle! "), __FILE__, __LINE__);

        CURLcode res;
        // Target URL --------------------------------------------------------------------------------------------------
        if (CURLE_OK != (res = curl_easy_setopt(d_handle, CURLOPT_URL, target_url.c_str())))
            throw BESInternalError(string("HTTP Error setting URL: ").append(error_message(res, d_errbuf)),
                                   __FILE__, __LINE__);

        // Pass all data to the 'write_data' function ------------------------------------------------------------------
        if (CURLE_OK != (res = curl_easy_setopt(d_handle, CURLOPT_WRITEFUNCTION, c_write_data)))
            throw BESInternalError(string("CURL Error: ").append(error_message(res, d_errbuf)),
                                   __FILE__, __LINE__);

        // Pass this to write_data as the fourth argument --------------------------------------------------------------
        if (CURLE_OK != (res = curl_easy_setopt(d_handle, CURLOPT_WRITEDATA, reinterpret_cast<void *>(response_buff))))
            throw BESInternalError(
                    string("CURL Error setting chunk as data buffer: ").append(error_message(res, d_errbuf)),
                    __FILE__, __LINE__);

        // Follow redirects --------------------------------------------------------------------------------------------
        if (CURLE_OK != (res = curl_easy_setopt(d_handle, CURLOPT_FOLLOWLOCATION, 1L)))
            throw BESInternalError(string("Error setting CURLOPT_FOLLOWLOCATION: ").append(
                    error_message(res, d_errbuf)),
                                   __FILE__, __LINE__);


        // Use cookies -------------------------------------------------------------------------------------------------
        if (CURLE_OK != (res = curl_easy_setopt(d_handle, CURLOPT_COOKIEFILE, cookies_file.c_str())))
            throw BESInternalError(
                    string("Error setting CURLOPT_COOKIEFILE to '").append(cookies_file).append("' msg: ").append(
                            error_message(res, d_errbuf)),
                    __FILE__, __LINE__);

        if (CURLE_OK != (res = curl_easy_setopt(d_handle, CURLOPT_COOKIEJAR, cookies_file.c_str())))
            throw BESInternalError(string("Error setting CURLOPT_COOKIEJAR: ").append(
                    error_message(res, d_errbuf)),
                                   __FILE__, __LINE__);


        // Authenticate using best available ---------------------------------------------------------------------------
        if (CURLE_OK != (res = curl_easy_setopt(d_handle, CURLOPT_HTTPAUTH, (long) CURLAUTH_ANY)))
            throw BESInternalError(string("Error setting CURLOPT_HTTPAUTH to CURLAUTH_ANY msg: ").append(
                    error_message(res, d_errbuf)),
                                   __FILE__, __LINE__);

#if 0
        if(debug) cout << "uid: " << uid << endl;
            if (CURLE_OK != (res = curl_easy_setopt(d_handle, CURLOPT_USERNAME, uid.c_str())))
                throw BESInternalError(string("Error setting CURLOPT_USERNAME: ").append(error_message(res, d_errbuf)),
                                       __FILE__, __LINE__);

            if(debug) cout << "pw: " << pw << endl;
            if (CURLE_OK != (res = curl_easy_setopt(d_handle, CURLOPT_PASSWORD, pw.c_str())))
                throw BESInternalError(string("Error setting CURLOPT_PASSWORD: ").append(error_message(res, d_errbuf)),
                                       __FILE__, __LINE__);
#else
        // Use .netrc for credentials ----------------------------------------------------------------------------------
        if (CURLE_OK != (res = curl_easy_setopt(d_handle, CURLOPT_NETRC, CURL_NETRC_OPTIONAL)))
            throw BESInternalError(string("Error setting CURLOPT_NETRC to CURL_NETRC_OPTIONAL: ").append(
                    error_message(res, d_errbuf)),
                                   __FILE__, __LINE__);
#endif

        // Error Buffer ------------------------------------------------------------------------------------------------
        if (CURLE_OK != (res = curl_easy_setopt(d_handle, CURLOPT_ERRORBUFFER, d_errbuf)))
            throw BESInternalError(string("CURL Error: ").append(curl_easy_strerror(res)), __FILE__, __LINE__);


        return d_handle;
    }

    /**
    * Execute the HTTP VERB from the passed cURL handle "c_handle" and retrieve the response.
    * @param c_handle The cURL easy handle to execute and read.
    */
    void read_data(CURL *c_handle) {

        unsigned int tries = 0;
        useconds_t retry_time = uone_second / 4;
        bool success;
        CURLcode curl_code;
        char curlErrorBuf[CURL_ERROR_SIZE]; ///< raw error message info from libcurl
        char *urlp = NULL;

        curl_easy_getinfo(c_handle, CURLINFO_EFFECTIVE_URL, &urlp);
        // Checking the curl_easy_getinfo return value in this case is pointless. If it's CURL_OK then we
        // still have to check the value of urlp to see if the URL was correctly set in the
        // cURL handle. If it fails then it fails, and urlp is not set. If we just focus on the value of urlp then
        // we can just check the one thing.
        if (!urlp)
            throw BESInternalError("URL acquisition failed.", __FILE__, __LINE__);

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
    }

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
        cf_with_pid <<  cookie_file_base << "-" << getpid();
        return cf_with_pid.str();
    }

    void clear_cookies(){
        string cf = get_cookie_filename();
        int ret = unlink(cf.c_str());
        if(ret){
            string msg = prolog + "() - Failed to unlink the cookie file: " + cf;
            LOG(msg << endl);
            BESDEBUG(MODULE, prolog << msg << endl);
        }
    }


    /**
     * Checks to see if the entire url matches any of the  "no retry" regular expressions held in the TheBESKeys
     * under the HTTP_NO_RETRY_URL_REGEX_KEY which atm, is set to "Http.No.Retry.Regex"
     * @param url The URL to be examined
     * @return True if the the url does not match a no retry regex, false if the entire url matches
     * a "no retry" regex.
     */
    bool is_retryable(std::string url)
    {
        BESDEBUG(MODULE, prolog << "BEGIN" << endl);
        bool retryable = true;

        vector<string> nr_regexs;
        bool found;
        TheBESKeys::TheKeys()->get_values(HTTP_NO_RETRY_URL_REGEX_KEY,nr_regexs, found);
        if(found){
            vector<string>::iterator it;
            for(it=nr_regexs.begin(); it != nr_regexs.end() && retryable ; it++){
                BESRegex no_retry_regex((*it).c_str(), (*it).size());
                int match_length;
                match_length = no_retry_regex.match(url.c_str(), url.size(), 0);
                if(match_length == url.size()){
                    BESDEBUG(MODULE, prolog << "The url: '"<< url << "' fully matched the "
                    << HTTP_NO_RETRY_URL_REGEX_KEY << ": '" <<  *it << "'" <<  endl);
                    retryable = false;
                }
            }
        }
        BESDEBUG(MODULE, prolog << "END retryable: "<< (retryable?"true":"false") << endl);
        return retryable;
    }

    /**
     * Check the response for errors and such.
     * @param eh The cURL easy_handle to evaluate.
     * @return true if at all worked out, false if it didn't and a retry is reasonable.
     * @throws BESInternalError When something really bad happens.
    */
    bool eval_get_response(CURL *eh, const string &requested_url) {
        BESDEBUG(MODULE, prolog << "Requested URL: " << requested_url << endl);

        char *last_accessed_url = 0;
        CURLcode res =curl_easy_getinfo(eh, CURLINFO_EFFECTIVE_URL, &last_accessed_url);
        if(res != CURLE_OK) {
            stringstream msg;
            msg << prolog << "Unable to determine CURLINFO_EFFECTIVE_URL! Requested URL: " << requested_url;
            BESDEBUG(MODULE, msg.str() << endl);
            throw BESInternalError(msg.str(), __FILE__, __LINE__);
        }
        BESDEBUG(MODULE, prolog << "Last Accessed URL(CURLINFO_EFFECTIVE_URL): " << last_accessed_url << endl);

        long http_code = 0;
        res = curl_easy_getinfo(eh, CURLINFO_RESPONSE_CODE, &http_code);
        if (res == CURLE_GOT_NOTHING) {
            // First we check to see if the response was empty. This is a cURL error, not an HTTP error
            // so we have to handle it like this. And we do that because this is one of the failure modes
            // we see in the AWS cloud and by trapping this and returning false we are able to be resilient and retry.
            // We maye eventually need to check other CURLCode errors
            stringstream msg;
            msg << prolog << "Ouch. cURL returned CURLE_GOT_NOTHING, returning false.  CURLINFO_EFFECTIVE_URL: " << last_accessed_url << endl;
            BESDEBUG(MODULE, msg.str());
            LOG(msg.str());
            return false;
        }
        else if(res != CURLE_OK) {
            // Not an error we are trapping so it's fail time.
            throw BESInternalError(
                    string("Error acquiring HTTP response code: ").append(curl::error_message(res, (char *) "")),
                    __FILE__, __LINE__);
        }

        if (BESDebug::IsSet(MODULE)) {
            long redirects;
            curl_easy_getinfo(eh, CURLINFO_REDIRECT_COUNT, &redirects);
            BESDEBUG(MODULE, prolog << "CURLINFO_REDIRECT_COUNT: " << redirects << endl);

            char *redirect_url = 0;
            curl_easy_getinfo(eh, CURLINFO_REDIRECT_URL, &redirect_url);
            if (redirect_url)
                BESDEBUG(MODULE, prolog << "CURLINFO_REDIRECT_URL: " << redirect_url << endl);
        }

        stringstream msg;
        if(http_code >= 400){
            msg << "ERROR - The HTTP GET request for the source URL: " << requested_url << " FAILED."
                << " The last accessed URL (CURLINFO_EFFECTIVE_URL) was: " << last_accessed_url
                << " The response had an HTTP status of " << http_code
                << " which means '" << http_status_to_string(http_code) << "'" << endl;
            BESDEBUG(MODULE, prolog << msg.str());
            LOG(msg.str());
        }

        // Newer Apache servers return 206 for range requests. jhrg 8/8/18
        switch (http_code) {
            case 200: // OK
            case 206: // Partial content - this is to be expected since we use range gets
                // cases 201-205 are things we should probably reject, unless we add more
                // comprehensive HTTP/S processing here. jhrg 8/8/18
                return true;

            case 400: // Bad Request
                throw BESSyntaxUserError(msg.str(), __FILE__, __LINE__);

            case 401: // Unauthorized
            case 402: // Payment Required
            case 403: // Forbidden
                throw BESForbiddenError(msg.str(), __FILE__, __LINE__);

            case 404: // Not Found
                throw BESNotFoundError(msg.str(), __FILE__, __LINE__);

            case 408: // Request Timeout
                throw BESTimeoutError(msg.str(), __FILE__, __LINE__);

            case 422: // Unprocessable Entity
            case 500: // Internal server error
            case 502: // Bad Gateway
            case 503: // Service Unavailable
            case 504: // Gateway Timeout
            {
                if(!is_retryable(last_accessed_url)){
                    msg << "The semantics of this particular last accessed URL indicate that it should not be retried.";
                    LOG(msg.str());
                    throw BESInternalError(msg.str(), __FILE__, __LINE__);
                }
                return false;
            }

            default: {
                BESDEBUG(MODULE, msg.str() << endl);
                throw BESInternalError(msg.str(), __FILE__, __LINE__);
            }
        }
    }


#if 0
    bool do_the_curl_perform_boogie(CURL *eh){
        CURLcode curl_code = curl_easy_perform(eh);
        if( curl_code == CURLE_SSL_CONNECT_ERROR ){
            stringstream msg;
            msg << prolog << "cURL experienced a CURLE_SSL_CONNECT_ERROR error. Will retry (url: "<< url << " attempt: " << tries << ")." << endl;
            BESDEBUG(MODULE,msg.str());
            LOG(msg.str());
            return false;
        }
        else if( curl_code == CURLE_SSL_CACERT_BADFILE ){
            stringstream msg;
            msg << prolog << "cURL experienced a CURLE_SSL_CACERT_BADFILE error. Will retry (url: " << url << " attempt: " << tries << ")." << endl;
            BESDEBUG(MODULE,msg.str());
            LOG(msg.str());
            return false;
        }
        else if (CURLE_OK != curl_code) {
            stringstream msg;
            msg << "Data transfer error: " << error_message(curl_code, error_buffer);
            char *effective_url = 0;
            curl_easy_getinfo(curl, CURLINFO_EFFECTIVE_URL, &effective_url);
            msg << " last_url: " << effective_url;
            BESDEBUG(MODULE, prolog << msg.str() << endl);
            throw BESInternalError(msg.str(), __FILE__, __LINE__);
        }
        long http_code = 0;
        CURLcode res = curl_easy_getinfo(eh, CURLINFO_RESPONSE_CODE, &http_code);
        if (res == CURLE_GOT_NOTHING) {
            // First we check to see if the response was empty. This is a cURL error, not an HTTP error
            // so we have to handle it like this. And we do that because this is one of the failure modes
            // we see in the AWS cloud and by trapping this and returning false we are able to be resilient and retry.
            // We maye eventually need to check other CURLCode errors
            char *effective_url = 0;
            curl_easy_getinfo(eh, CURLINFO_EFFECTIVE_URL, &effective_url);
            stringstream msg;
            msg << prolog << "Ouch. cURL returned CURLE_GOT_NOTHING, returning false.  CURLINFO_EFFECTIVE_URL: " << effective_url << endl;
            BESDEBUG(MODULE, msg.str());
            LOG(msg.str());
            return false;
        }
        else if(res != CURLE_OK) {
            // Not an error we are trapping so it's fail time.
            throw BESInternalError(
                    string("Error getting HTTP response code: ").append(curl::error_message(res, (char *) "")),
                    __FILE__, __LINE__);
        }
        if (BESDebug::IsSet(MODULE)) {
            char *last_url = 0;
            curl_easy_getinfo(eh, CURLINFO_EFFECTIVE_URL, &last_url);
            BESDEBUG(MODULE, prolog << "Last Accessed URL(CURLINFO_EFFECTIVE_URL): " << last_url << endl);
            long redirects;
            curl_easy_getinfo(eh, CURLINFO_REDIRECT_COUNT, &redirects);
            BESDEBUG(MODULE, prolog << "CURLINFO_REDIRECT_COUNT: " << redirects << endl);
            char *redirect_url = 0;
            curl_easy_getinfo(eh, CURLINFO_REDIRECT_URL, &redirect_url);
            if (redirect_url)
                BESDEBUG(MODULE, prolog << "CURLINFO_REDIRECT_URL: " << redirect_url << endl);
        }
        //  FIXME Expand the list of handled status to at least include the 4** stuff for authentication
        //  FIXME so that something sensible can be done.
        // Newer Apache servers return 206 for range requests. jhrg 8/8/18
        switch (http_code) {
            case 200: // OK
            case 206: // Partial content - this is to be expected since we use range gets
                // cases 201-205 are things we should probably reject, unless we add more
                // comprehensive HTTP/S processing here. jhrg 8/8/18
                return true;
            case 500: // Internal server error
            case 502: // Bad Gateway
            case 503: // Service Unavailable
            case 504: // Gateway Timeout
            {
                char *effective_url = 0;
                curl_easy_getinfo(eh, CURLINFO_EFFECTIVE_URL, &effective_url);
                stringstream msg;
                msg << prolog << "HTTP transfer " << http_code << " error, returning false.  CURLINFO_EFFECTIVE_URL: " << effective_url << endl;
                BESDEBUG(MODULE, msg.str());
                LOG(msg.str());
                return false;
            }
            default: {
                stringstream msg;
                char *effective_url = 0;
                curl_easy_getinfo(eh, CURLINFO_EFFECTIVE_URL, &effective_url);
                msg << prolog << "HTTP status error: Expected an OK status, but got: " << http_code  << " from (CURLINFO_EFFECTIVE_URL): " << effective_url;
                BESDEBUG(MODULE, msg.str() << endl);
                throw BESInternalError(msg.str(), __FILE__, __LINE__);
            }
        }
    }
#endif
    /**
     * @brief Performs a small (4 byte) range get on the target URL. If successfull the value of  last_accessed_url will
     * be set to the value of the last accessed URL (CURLINFO_EFFECTIVE_URL), including the query string.
     * are
     * @param url The URL to follow
     * @param last_accessed_url The last accessed URL (CURLINFO_EFFECTIVE_URL), including the query string
     */
    void retrieve_effective_url(const string &url, string &last_accessed_url) {

        unsigned int tries = 0;
        bool success = true;
        useconds_t retry_time = uone_second / 4;

        char error_buffer[CURL_ERROR_SIZE];
        vector<string> resp_hdrs;
        CURL *curl = 0;

        try {
            curl = init_effective_url_retriever_handle(url, resp_hdrs, error_buffer);
            do {
                bool do_retry = false;
                ++tries;
                error_buffer[0]=0; // Initialize to empty string

                BESDEBUG(MODULE, prolog << "Requesting URL: " << url << " attempt: " << tries <<  endl);
                CURLcode curl_code = curl_easy_perform(curl);
                if( curl_code == CURLE_SSL_CONNECT_ERROR ){
                    stringstream msg;
                    msg << prolog << "cURL experienced a CURLE_SSL_CONNECT_ERROR error. Will retry (url: "<< url << " attempt: " << tries << ")." << endl;
                    BESDEBUG(MODULE,msg.str());
                    LOG(msg.str());
                    do_retry = true;
                }
                else if( curl_code == CURLE_SSL_CACERT_BADFILE ){
                    stringstream msg;
                    msg << prolog << "cURL experienced a CURLE_SSL_CACERT_BADFILE error. Will retry (url: " << url << " attempt: " << tries << ")." << endl;
                    BESDEBUG(MODULE,msg.str());
                    LOG(msg.str());
                    do_retry = true;
                }
                else if (CURLE_OK != curl_code) {
                    stringstream msg;
                    msg << "Data transfer error: " << error_message(curl_code, error_buffer);
                    char *effective_url = 0;
                    curl_easy_getinfo(curl, CURLINFO_EFFECTIVE_URL, &effective_url);
                    msg << " last_url: " << effective_url;
                    BESDEBUG(MODULE, prolog << msg.str() << endl);
                    throw BESInternalError(msg.str(), __FILE__, __LINE__);
                }
                else {
                    success = eval_get_response(curl, url);
                    if (!success) {
                        if (tries == retry_limit) {
                            string msg = prolog + "Data transfer error: Number of re-tries exceeded: "+ error_message(curl_code, error_buffer);
                            LOG(msg << endl);
                            throw BESInternalError(msg, __FILE__, __LINE__);
                        }
                        else {
                            LOG(prolog << "HTTP Range-GET failed. Will retry (url: " << url <<
                                                                           " attempt: " << tries << ")." << endl);
                            do_retry = true;
                        }
                    }
                }

                if(do_retry){
                    usleep(retry_time);
                    retry_time *= 2;
                }

            } while (!success);

            char *effective_url = 0;
            curl_easy_getinfo(curl, CURLINFO_EFFECTIVE_URL, &effective_url);
            BESDEBUG(MODULE, prolog << " CURLINFO_EFFECTIVE_URL: " << effective_url << endl);
            last_accessed_url = effective_url;

            LOG(prolog << "Source URL: '" << url << "' Last Accessed URL: '" << last_accessed_url << "'" << endl);

            if(curl){
                curl_easy_cleanup(curl);
                curl = 0;
            }
        }
        catch(...){
            if(curl){
                curl_easy_cleanup(curl);
                curl = 0;
            }
            throw;
        }
    }




} /* namespace curl */
