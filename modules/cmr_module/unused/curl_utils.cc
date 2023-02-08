// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of cmr_module, A C++ module that can be loaded in to
// the OPeNDAP Back-End Server (BES) and is able to handle remote requests.

// Copyright (c) 2013 OPeNDAP, Inc.
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

#include <unistd.h>
#include <sstream>
#include <algorithm>    // std::for_each

#include "BESRegex.h"

#include <libdap/util.h>
#include "BESDebug.h"
#include "BESLog.h"
#include "BESSyntaxUserError.h"
#include "TheBESKeys.h"
#include "BESInternalError.h"
#include "CmrUtils.h"
#include "AllowedHosts.h"

#include "curl_utils.h"

#define MODULE "cmr"

using namespace std;

namespace cmr {

// Set this to 1 to turn on libcurl's verbose mode (for debugging).
int curl_trace = 0;

#define CLIENT_ERR_MIN 400
#define CLIENT_ERR_MAX 417
const char *http_client_errors[CLIENT_ERR_MAX - CLIENT_ERR_MIN +1] =
    {
        "Bad Request:",
        "Unauthorized: Contact the server administrator.",
        "Payment Required.",
        "Forbidden: Contact the server administrator.",
        "Not Found: The data source or server could not be found. "
            "Often this means that the OPeNDAP server is missing or needs attention. "
            "Please contact the server administrator.",
        "Method Not Allowed.",
        "Not Acceptable.",
        "Proxy Authentication Required.",
        "Request Time-out.",
        "Conflict.",
        "Gone:.",
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


/** This function translates an HTTP status code into an error messages. It
    works for those code greater than or equal to 400. */
string http_status_to_string(int status)
{
    if (status >= CLIENT_ERR_MIN && status <= CLIENT_ERR_MAX)
        return string(http_client_errors[status - CLIENT_ERR_MIN]);
    else if (status >= SERVER_ERR_MIN && status <= SERVER_ERR_MAX)
        return string(http_server_errors[status - SERVER_ERR_MIN]);
    else
        return string("Unknown Error: This indicates a problem with libdap++.\nPlease report this to support@opendap.org.");
}

static string getCurlAuthTypeName(const int authType){

    string authTypeString;
    int match;

    match = authType & CURLAUTH_BASIC;
    if(match){
        authTypeString += "CURLAUTH_BASIC";
    }

    match = authType & CURLAUTH_DIGEST;
    if(match){
        if(!authTypeString.empty())
            authTypeString += " ";
        authTypeString += "CURLAUTH_DIGEST";
    }

    match = authType & CURLAUTH_DIGEST_IE;
    if(match){
        if(!authTypeString.empty())
            authTypeString += " ";
        authTypeString += "CURLAUTH_DIGEST_IE";
    }

    match = authType & CURLAUTH_GSSNEGOTIATE;
    if(match){
        if(!authTypeString.empty())
            authTypeString += " ";
        authTypeString += "CURLAUTH_GSSNEGOTIATE";
    }

    match = authType & CURLAUTH_NTLM;
    if(match){
        if(!authTypeString.empty())
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
 * libcurl call back function that is used to write data to a passed open file descriptor (that would
 * be instead of the default open FILE *)
 */
static size_t writeToOpenfileDescriptor( char *data, size_t /* size */, size_t nmemb, void *userdata){

    int *fd = (int *) userdata;

    BESDEBUG(MODULE, "curl_utils::writeToOpenfileDescriptor() - Bytes received " << libdap::long_to_string(nmemb) << endl);
    int wrote = write(*fd, data, nmemb);
    BESDEBUG(MODULE, "curl_utils::writeToOpenfileDescriptor() - Bytes written " << libdap::long_to_string(wrote) << endl);

    return wrote;
}


/** A libcurl callback function used to read response headers. Read headers,
    line by line, from ptr. The fourth param is really supposed to be a FILE
    *, but libcurl just holds the pointer and passes it to this function
    without using it itself.

    What actually happens? This code was moved and I am not sure how this works (which it does) now.

    I use that to pass in a pointer to the
    HTTPConnect that initiated the HTTP request so that there's some place to
    dump the headers.


    Note that this function just saves the headers in a
    vector of strings. Later on the code (see fetch_url()) parses the headers
    special to the DAP.

    @param ptr A pointer to one line of character data; one header.
    @param size Size of each character (nominally one byte).
    @param nmemb Number of bytes.
    @param resp_hdrs A pointer to a vector<string>. Set in read_url.
    @return The number of bytes processed. Must be equal to size * nmemb or
    libcurl will report an error. */

static size_t save_raw_http_headers(void *ptr, size_t size, size_t nmemb, void *resp_hdrs)
{
    BESDEBUG(MODULE,  "curl_utils::save_raw_http_headers() - Inside the header parser." << endl);
    vector<string> *hdrs = static_cast<vector<string> * >(resp_hdrs);

    // Grab the header, minus the trailing newline. Or \r\n pair.
    string complete_line;
    if (nmemb > 1 && *(static_cast<char*>(ptr) + size * (nmemb - 2)) == '\r')
        complete_line.assign(static_cast<char *>(ptr), size * (nmemb - 2));
    else
        complete_line.assign(static_cast<char *>(ptr), size * (nmemb - 1));

    // Store all non-empty headers that are not HTTP status codes
    if (complete_line != "" && complete_line.find("HTTP") == string::npos) {
        BESDEBUG(MODULE,  "curl_utils::save_raw_http_headers() - Header line: " << complete_line << endl);
        hdrs->push_back(complete_line);
    }

    return size * nmemb;
}





/** A libcurl callback for debugging protocol issues. */
static int curl_debug(CURL *, curl_infotype info, char *msg, size_t size, void  *)
{
    string message(msg, size);

    switch (info) {
    case CURLINFO_TEXT:
        BESDEBUG(MODULE, "curl_utils::curl_debug() - Text: " << message << endl ); break;
    case CURLINFO_HEADER_IN:
        BESDEBUG(MODULE, "curl_utils::curl_debug() - Header in: " << message << endl ); break;
    case CURLINFO_HEADER_OUT:
        BESDEBUG(MODULE, "curl_utils::curl_debug() - Header out: " << endl << message << endl ); break;
    case CURLINFO_DATA_IN:
        BESDEBUG(MODULE, "curl_utils::curl_debug() - Data in: " << message << endl ); break;
    case CURLINFO_DATA_OUT:
        BESDEBUG(MODULE, "curl_utils::curl_debug() - Data out: " << message << endl ); break;
    case CURLINFO_END:
        BESDEBUG(MODULE, "curl_utils::curl_debug() - End: " << message << endl ); break;
#ifdef CURLINFO_SSL_DATA_IN
    case CURLINFO_SSL_DATA_IN:
        BESDEBUG(MODULE, "curl_utils::curl_debug() - SSL Data in: " << message << endl ); break;
#endif
#ifdef CURLINFO_SSL_DATA_OUT
    case CURLINFO_SSL_DATA_OUT:
        BESDEBUG(MODULE, "curl_utils::curl_debug() - SSL Data out: " << message << endl ); break;
#endif
    default:
        BESDEBUG(MODULE, "curl_utils::curl_debug() - Curl info: " << message << endl ); break;
    }
    return 0;
}







/** Functor to add a single string to a curl_slist. This is used to transfer
    a list of headers from a vector<string> object to a curl_slist. */
class BuildHeaders : public std::unary_function<const string &, void>
{
    struct curl_slist *d_cl;

public:
    BuildHeaders() : d_cl(0)
    {}

    void operator()(const string &header)
    {
        BESDEBUG(MODULE, "BuildHeaders::operator() - Adding '" << header.c_str() << "' to the header list." << endl);
        d_cl = curl_slist_append(d_cl, header.c_str());
    }

    struct curl_slist *get_headers()
    {
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
    BESDEBUG( MODULE, "curl_utils::configureProxy() - BEGIN." << endl);

    bool using_proxy = false;

    // I pulled this because I could never find where it was applied
    // to the curl state in HTTPConnect
    //string proxyProtocol = GatewayUtils::ProxyProtocol;

    string proxyHost     = cmr::CmrUtils::ProxyHost;
    int proxyPort        = cmr::CmrUtils::ProxyPort;
    string proxyPassword = cmr::CmrUtils::ProxyPassword;
    string proxyUser     = cmr::CmrUtils::ProxyUser;
    string proxyUserPW   = cmr::CmrUtils::ProxyUserPW;
    int proxyAuthType    = cmr::CmrUtils::ProxyAuthType;

    if (!proxyHost.empty()) {
        using_proxy = true;
        if(proxyPort==0)
            proxyPort = 8080;

        // Apparently we don't need this...
        //if(proxyProtocol.empty())
           // proxyProtocol = "http";

    }
    if (using_proxy) {
        BESDEBUG( MODULE, "curl_utils::configureProxy() - Found proxy configuration." << endl);

        // Don't set up the proxy server for URLs that match the 'NoProxy'
        // regex set in the gateway.conf file.

        // Don't create the regex if the string is empty
        if (!CmrUtils::NoProxyRegex.empty()) {
            BESDEBUG( MODULE, "curl_utils::configureProxy() - Found NoProxyRegex." << endl);
            libdap::BESRegex r(CmrUtils::NoProxyRegex.c_str());
            if (r.match(url.c_str(), url.size()) != -1) {
                BESDEBUG( MODULE, "curl_utils::configureProxy() - Found NoProxy match. BESRegex: " << CmrUtils::NoProxyRegex << "; Url: " << url << endl);
                using_proxy = false;
            }
        }

        if (using_proxy) {

            BESDEBUG(MODULE, "curl_utils::configureProxy() - Setting up a proxy server." << endl);
            BESDEBUG(MODULE, "curl_utils::configureProxy() - Proxy host: " << proxyHost << endl);
            BESDEBUG(MODULE, "curl_utils::configureProxy() - Proxy port: " << proxyPort << endl);

            curl_easy_setopt(curl, CURLOPT_PROXY, proxyHost.data());
            curl_easy_setopt(curl, CURLOPT_PROXYPORT, proxyPort);

// #ifdef CURLOPT_PROXYAUTH

            // oddly "#ifdef CURLOPT_PROXYAUTH" doesn't work - even though CURLOPT_PROXYAUTH is defined and valued at 111 it
            // fails the test. Eclipse hover over the CURLOPT_PROXYAUTH symbol shows: "CINIT(PROXYAUTH, LONG, 111)",
            // for what that's worth

            // According to http://curl.haxx.se/libcurl/c/curl_easy_setopt.html#CURLOPTPROXYAUTH As of 4/21/08 only NTLM, Digest and Basic work.

#if 0
            BESDEBUG(MODULE, "curl_utils::configureProxy() - CURLOPT_PROXYAUTH       = " << CURLOPT_PROXYAUTH << endl);
            BESDEBUG(MODULE, "curl_utils::configureProxy() - CURLAUTH_BASIC          = " << CURLAUTH_BASIC << endl);
            BESDEBUG(MODULE, "curl_utils::configureProxy() - CURLAUTH_DIGEST         = " << CURLAUTH_DIGEST << endl);
            BESDEBUG(MODULE, "curl_utils::configureProxy() - CURLAUTH_DIGEST_IE      = " << CURLAUTH_DIGEST_IE << endl);
            BESDEBUG(MODULE, "curl_utils::configureProxy() - CURLAUTH_GSSNEGOTIATE   = " << CURLAUTH_GSSNEGOTIATE << endl);
            BESDEBUG(MODULE, "curl_utils::configureProxy() - CURLAUTH_NTLM           = " << CURLAUTH_NTLM << endl);
            BESDEBUG(MODULE, "curl_utils::configureProxy() - CURLAUTH_ANY            = " << CURLAUTH_ANY << endl);
            BESDEBUG(MODULE, "curl_utils::configureProxy() - CURLAUTH_ANYSAFE        = " << CURLAUTH_ANYSAFE << endl);
            BESDEBUG(MODULE, "curl_utils::configureProxy() - CURLAUTH_ONLY           = " << CURLAUTH_ONLY << endl);
            BESDEBUG(MODULE, "curl_utils::configureProxy() - Using CURLOPT_PROXYAUTH = " << proxyAuthType << endl);
#endif

            BESDEBUG(MODULE, "curl_utils::configureProxy() - Using CURLOPT_PROXYAUTH = " << getCurlAuthTypeName(proxyAuthType) << endl);
            curl_easy_setopt(curl, CURLOPT_PROXYAUTH, proxyAuthType);
// #endif



            if (!proxyUser.empty()){
                curl_easy_setopt(curl, CURLOPT_PROXYUSERNAME, proxyUser.data());
                BESDEBUG(MODULE, "curl_utils::configureProxy() - CURLOPT_PROXYUSER : " << proxyUser << endl);

                if (!proxyPassword.empty()){
                    curl_easy_setopt(curl, CURLOPT_PROXYPASSWORD, proxyPassword.data());
                    BESDEBUG(MODULE, "curl_utils::configureProxy() - CURLOPT_PROXYPASSWORD: " << proxyPassword << endl);
                }
            }
            else if (!proxyUserPW.empty()){
                BESDEBUG(MODULE,
                        "curl_utils::configureProxy() - CURLOPT_PROXYUSERPWD : " << proxyUserPW << endl);
                curl_easy_setopt(curl, CURLOPT_PROXYUSERPWD, proxyUserPW.data());
            }

        }
    }
    BESDEBUG( MODULE, "curl_utils::configureProxy() - END." << endl);

    return using_proxy;
}

/**
 * @brief Checks the BES config (aka TheBESKeys) for the NETRC_FILE_KEY to see if a netrc file was
 * injected.
 * @return
 */
const string NETRC_FILE_KEY="BES.netrc.file";
string get_netrc_filename(){
    bool found;
    string name;
    TheBESKeys::TheKeys()->get_value(NETRC_FILE_KEY,name,found);
    return name;
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
CURL *init(char *error_buffer)
{

    CURL *curl = curl_easy_init();
    if (!curl)
        throw libdap::InternalErr(__FILE__, __LINE__, "Could not initialize libcurl.");

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
    curl_easy_setopt(curl, CURLOPT_HTTPAUTH, (long)CURLAUTH_ANY);

    // I added these next three to support Hyrax accessing data held behind URS auth. ndp - 8/20/18
    curl_easy_setopt(curl, CURLOPT_NETRC, 1);

    // #TODO #FIXME Make these file names configuration based.
    curl_easy_setopt(curl, CURLOPT_COOKIEFILE, "/tmp/.hyrax_cookies");
    curl_easy_setopt(curl, CURLOPT_COOKIEJAR, "/tmp/.hyrax_cookies");

    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, save_raw_http_headers);
    // In read_url a call to CURLOPT_WRITEHEADER is used to set the fourth
    // param of save_raw_http_headers to a vector<string> object.

    // Follow 302 (redirect) responses
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 20);

    // If the configuration specifies a particular .netrc credentials file, use it.
    string netrc_file = get_netrc_filename();
    if(!netrc_file.empty()){
        curl_easy_setopt(curl, CURLOPT_NETRC_FILE, netrc_file.c_str());
    }
    VERBOSE(__FILE__ << "::init() is using the netrc file '" << ((!netrc_file.empty())?netrc_file:"~/.netrc")<< "'" << endl );

        // Set the user agent to curls version response because, well, that's what command line curl does :)
    curl_easy_setopt(curl, CURLOPT_USERAGENT, curl_version() );


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
        BESDEBUG(cerr << "Setting the cookie jar to: " << d_cookie_jar << endl);
        curl_easy_setopt(curl, CURLOPT_COOKIEJAR, d_cookie_jar.c_str());
        curl_easy_setopt(curl, CURLOPT_COOKIESESSION, 1);
    }
#endif


    if (curl_trace) {
        BESDEBUG(MODULE, "curl_utils::www_lib_init() - Curl version: " << curl_version() << endl);
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
        BESDEBUG(MODULE, "curl_utils::www_lib_init() - Curl in verbose mode."<< endl);
        curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, curl_debug);
        BESDEBUG(MODULE, "curl_utils::www_lib_init() - Curl debugging function installed."<< endl);
    }


    BESDEBUG(MODULE, "curl_utils::www_lib_init() - curl: " << curl << endl);

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
long read_url(CURL *curl,
              const string &url,
              int fd,
              vector<string> *resp_hdrs,
              const vector<string> *request_headers,
              char error_buffer[])
{
    string prolog = string("curl_utils.cc ") + __func__ + "() - ";

    BESDEBUG(MODULE, prolog << "BEGIN" << endl);

    // Before we do anything, make sure that the URL is OK to pursue.
    if (!bes::AllowedHosts::get_white_list()->is_white_listed(url)) {
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

    // This call is the one that makes curl go get the thing.
    CURLcode res = curl_easy_perform(curl);

    // Free the header list and null the value in d_curl.
    curl_slist_free_all(req_hdrs.get_headers());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, 0);

    // Did cURL have a problem?
    if (res != CURLE_OK){
        stringstream msg;
        msg << "The cURL library encountered an error when asked to retrieve the URL: " << url <<
            " cURL message: " << error_buffer;
        BESDEBUG(MODULE, prolog << "OUCH! CURL returned an error! curl msg:  " << curl_easy_strerror(res) << endl);
        BESDEBUG(MODULE, prolog << msg.str() << endl);
        throw BESInternalError(msg.str(),__FILE__,__LINE__);
    }

    long status;
    res = curl_easy_getinfo(curl, CURLINFO_HTTP_CODE, &status);
    BESDEBUG(MODULE, prolog << "HTTP Status " << status << endl);
    if (res != CURLE_OK){
        string msg = "The cURL library encountered an error when asked for the HTTP "
                     "status associated with the response from : " + url;
        throw BESInternalError(msg,__FILE__,__LINE__);
    }
    BESDEBUG(MODULE, prolog << "END" << endl);

    return status;
}



} /* namespace cmr */
