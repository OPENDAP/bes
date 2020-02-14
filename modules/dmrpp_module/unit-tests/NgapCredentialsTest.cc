// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of libdap, A C++ implementation of the OPeNDAP Data
// Access Protocol.

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

#include <memory>

#include <stdlib.h>
#include <unistd.h>
#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>

#include <GetOpt.h>
#include <util.h>
#include <debug.h>

#include <curl/curl.h>
#include "xml2json/include/xml2json.hpp"

#include "xml2json/include/rapidjson/document.h"
#include "xml2json/include/rapidjson/writer.h"

#if HAVE_CURL_MULTI_H
#include <curl/multi.h>
#endif

#include "BESContextManager.h"
#include "BESError.h"
#include "BESInternalError.h"
#include "BESDebug.h"
#include "TheBESKeys.h"

#include "test_config.h"

using namespace libdap;

static bool debug = false;
static bool bes_debug = false;

#undef DBG
#define DBG(x) do { if (debug) x; } while(false)

namespace dmrpp {


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
    size_t ngap_write_data(void *buffer, size_t size, size_t nmemb, void *data) {
        size_t nbytes = size * nmemb;
        //cerr << "ngap_write_data() bytes: " << nbytes << "  size: " << size << "  nmemb: " << nmemb << " buffer: " << buffer << "  data: " << data << endl;
        memcpy(data,buffer,nbytes);
        return nbytes;
    }
    string uid;
    string pw;

    class NgapCredentialsTest : public CppUnit::TestFixture {
    private:
        string weak_config;
        char d_errbuf[CURL_ERROR_SIZE]; ///< raw error message info from libcurl


        string curl_error_msg(CURLcode res, char *errbuf) {
            ostringstream oss;
            size_t len = strlen(errbuf);
            if (len) {
                oss << errbuf;
                oss << " (code: " << (int) res << ")";
            } else {
                oss << curl_easy_strerror(res) << "(result: " << res << ")";
            }
            return oss.str();
        }

        std::map<CURLINFO, std::vector<std::string>> curl_info={
            {CURLINFO_EFFECTIVE_URL, {"CURLINFO_EFFECTIVE_URL","Last used URL. See CURLINFO_EFFECTIVE_URL"}},
            {CURLINFO_RESPONSE_CODE, {"CURLINFO_RESPONSE_CODE","Last received response code. See CURLINFO_RESPONSE_CODE"}},
            {CURLINFO_HTTP_CONNECTCODE, {"CURLINFO_HTTP_CONNECTCODE","Last proxy CONNECT response code. See CURLINFO_HTTP_CONNECTCODE"}},
            {CURLINFO_HTTP_VERSION, {"CURLINFO_HTTP_VERSION","The http version used in the connection. See CURLINFO_HTTP_VERSION"}},
            {CURLINFO_FILETIME, {"CURLINFO_FILETIME","Remote time of the retrieved document. See CURLINFO_FILETIME"}},
            //{CURLINFO_FILETIME_T, {"CURLINFO_FILETIME_T","Remote time of the retrieved document. See CURLINFO_FILETIME_T"}},
            {CURLINFO_TOTAL_TIME, {"CURLINFO_TOTAL_TIME","Total time of previous transfer. See CURLINFO_TOTAL_TIME"}},
            //{CURLINFO_TOTAL_TIME_T, {"CURLINFO_TOTAL_TIME_T","Total time of previous transfer. See CURLINFO_TOTAL_TIME_T"}},
            {CURLINFO_NAMELOOKUP_TIME, {"CURLINFO_NAMELOOKUP_TIME","Time from start until name resolving completed. See CURLINFO_NAMELOOKUP_TIME"}},
            //{CURLINFO_NAMELOOKUP_TIME_T, {"CURLINFO_NAMELOOKUP_TIME_T","Time from start until name resolving completed. See CURLINFO_NAMELOOKUP_TIME_T"}},
            {CURLINFO_CONNECT_TIME, {"CURLINFO_CONNECT_TIME","Time from start until remote host or proxy completed. See CURLINFO_CONNECT_TIME"}},
            //{CURLINFO_CONNECT_TIME_T, {"CURLINFO_CONNECT_TIME_T","Time from start until remote host or proxy completed. See CURLINFO_CONNECT_TIME_T"}},
            {CURLINFO_APPCONNECT_TIME, {"CURLINFO_APPCONNECT_TIME","Time from start until SSL/SSH handshake completed. See CURLINFO_APPCONNECT_TIME"}},
            //{CURLINFO_APPCONNECT_TIME_T, {"CURLINFO_APPCONNECT_TIME_T","Time from start until SSL/SSH handshake completed. See CURLINFO_APPCONNECT_TIME_T"}},
            {CURLINFO_PRETRANSFER_TIME, {"CURLINFO_PRETRANSFER_TIME","Time from start until just before the transfer begins. See CURLINFO_PRETRANSFER_TIME"}},
            //{CURLINFO_PRETRANSFER_TIME_T, {"CURLINFO_PRETRANSFER_TIME_T","Time from start until just before the transfer begins. See CURLINFO_PRETRANSFER_TIME_T"}},
            {CURLINFO_STARTTRANSFER_TIME, {"CURLINFO_STARTTRANSFER_TIME","Time from start until just when the first byte is received. See CURLINFO_STARTTRANSFER_TIME"}},
            //{CURLINFO_STARTTRANSFER_TIME_T, {"CURLINFO_STARTTRANSFER_TIME_T","Time from start until just when the first byte is received. See CURLINFO_STARTTRANSFER_TIME_T"}},
            {CURLINFO_REDIRECT_TIME, {"CURLINFO_REDIRECT_TIME","Time taken for all redirect steps before the final transfer. See CURLINFO_REDIRECT_TIME"}},
            //{CURLINFO_REDIRECT_TIME_T, {"CURLINFO_REDIRECT_TIME_T","Time taken for all redirect steps before the final transfer. See CURLINFO_REDIRECT_TIME_T"}},
            {CURLINFO_REDIRECT_COUNT, {"CURLINFO_REDIRECT_COUNT","Total number of redirects that were followed. See CURLINFO_REDIRECT_COUNT"}},
            {CURLINFO_REDIRECT_URL, {"CURLINFO_REDIRECT_URL","URL a redirect would take you to, had you enabled redirects. See CURLINFO_REDIRECT_URL"}},
            {CURLINFO_SIZE_UPLOAD, {"CURLINFO_SIZE_UPLOAD","(Deprecated) Number of bytes uploaded. See CURLINFO_SIZE_UPLOAD"}},
            //{CURLINFO_SIZE_UPLOAD_T, {"CURLINFO_SIZE_UPLOAD_T","Number of bytes uploaded. See CURLINFO_SIZE_UPLOAD_T"}},
            {CURLINFO_SIZE_DOWNLOAD, {"CURLINFO_SIZE_DOWNLOAD","(Deprecated) Number of bytes downloaded. See CURLINFO_SIZE_DOWNLOAD"}},
            //{CURLINFO_SIZE_DOWNLOAD_T, {"CURLINFO_SIZE_DOWNLOAD_T","Number of bytes downloaded. See CURLINFO_SIZE_DOWNLOAD_T"}},
            {CURLINFO_SPEED_DOWNLOAD, {"CURLINFO_SPEED_DOWNLOAD","(Deprecated) Average download speed. See CURLINFO_SPEED_DOWNLOAD"}},
            //{CURLINFO_SPEED_DOWNLOAD_T, {"CURLINFO_SPEED_DOWNLOAD_T","Average download speed. See CURLINFO_SPEED_DOWNLOAD_T"}},
            {CURLINFO_SPEED_UPLOAD, {"CURLINFO_SPEED_UPLOAD","(Deprecated) Average upload speed. See CURLINFO_SPEED_UPLOAD"}},
            //{CURLINFO_SPEED_UPLOAD_T, {"CURLINFO_SPEED_UPLOAD_T","Average upload speed. See CURLINFO_SPEED_UPLOAD_T"}},
            {CURLINFO_HEADER_SIZE, {"CURLINFO_HEADER_SIZE","Number of bytes of all headers received. See CURLINFO_HEADER_SIZE"}},
            {CURLINFO_REQUEST_SIZE, {"CURLINFO_REQUEST_SIZE","Number of bytes sent in the issued HTTP requests. See CURLINFO_REQUEST_SIZE"}},
            {CURLINFO_SSL_VERIFYRESULT, {"CURLINFO_SSL_VERIFYRESULT","Certificate verification result. See CURLINFO_SSL_VERIFYRESULT"}},
            {CURLINFO_PROXY_SSL_VERIFYRESULT, {"CURLINFO_PROXY_SSL_VERIFYRESULT","Proxy certificate verification result. See CURLINFO_PROXY_SSL_VERIFYRESULT"}},
            {CURLINFO_SSL_ENGINES, {"CURLINFO_SSL_ENGINES","A list of OpenSSL crypto engines. See CURLINFO_SSL_ENGINES"}},
            {CURLINFO_CONTENT_LENGTH_DOWNLOAD, {"CURLINFO_CONTENT_LENGTH_DOWNLOAD","(Deprecated) Content length from the Content-Length header. See CURLINFO_CONTENT_LENGTH_DOWNLOAD"}},
            //{CURLINFO_CONTENT_LENGTH_DOWNLOAD_T, {"CURLINFO_CONTENT_LENGTH_DOWNLOAD_T","Content length from the Content-Length header. See CURLINFO_CONTENT_LENGTH_DOWNLOAD_T"}},
            {CURLINFO_CONTENT_LENGTH_UPLOAD, {"CURLINFO_CONTENT_LENGTH_UPLOAD","(Deprecated) Upload size. See CURLINFO_CONTENT_LENGTH_UPLOAD"}},
            //{CURLINFO_CONTENT_LENGTH_UPLOAD_T, {"CURLINFO_CONTENT_LENGTH_UPLOAD_T","Upload size. See CURLINFO_CONTENT_LENGTH_UPLOAD_T"}},
            {CURLINFO_CONTENT_TYPE, {"CURLINFO_CONTENT_TYPE","Content type from the Content-Type header. See CURLINFO_CONTENT_TYPE"}},
            //{CURLINFO_RETRY_AFTER, {"CURLINFO_RETRY_AFTER","The value from the from the Retry-After header. See CURLINFO_RETRY_AFTER"}},
            {CURLINFO_PRIVATE, {"CURLINFO_PRIVATE","User's private data pointer. See CURLINFO_PRIVATE"}},
            {CURLINFO_HTTPAUTH_AVAIL, {"CURLINFO_HTTPAUTH_AVAIL","Available HTTP authentication methods. See CURLINFO_HTTPAUTH_AVAIL"}},
            {CURLINFO_PROXYAUTH_AVAIL, {"CURLINFO_PROXYAUTH_AVAIL","Available HTTP proxy authentication methods. See CURLINFO_PROXYAUTH_AVAIL"}},
            {CURLINFO_OS_ERRNO, {"CURLINFO_OS_ERRNO","The errno from the last failure to connect. See CURLINFO_OS_ERRNO"}},
            {CURLINFO_NUM_CONNECTS, {"CURLINFO_NUM_CONNECTS","Number of new successful connections used for previous transfer. See CURLINFO_NUM_CONNECTS"}},
            {CURLINFO_PRIMARY_IP, {"CURLINFO_PRIMARY_IP","IP address of the last connection. See CURLINFO_PRIMARY_IP"}},
            {CURLINFO_PRIMARY_PORT, {"CURLINFO_PRIMARY_PORT","Port of the last connection. See CURLINFO_PRIMARY_PORT"}},
            {CURLINFO_LOCAL_IP, {"CURLINFO_LOCAL_IP","Local-end IP address of last connection. See CURLINFO_LOCAL_IP"}},
            {CURLINFO_LOCAL_PORT, {"CURLINFO_LOCAL_PORT","Local-end port of last connection. See CURLINFO_LOCAL_PORT"}},
            {CURLINFO_COOKIELIST, {"CURLINFO_COOKIELIST","List of all known cookies. See CURLINFO_COOKIELIST"}},
            {CURLINFO_LASTSOCKET, {"CURLINFO_LASTSOCKET","Last socket used. See CURLINFO_LASTSOCKET"}},
            {CURLINFO_ACTIVESOCKET, {"CURLINFO_ACTIVESOCKET","The session's active socket. See CURLINFO_ACTIVESOCKET"}},
            {CURLINFO_FTP_ENTRY_PATH, {"CURLINFO_FTP_ENTRY_PATH","The entry path after logging in to an FTP server. See CURLINFO_FTP_ENTRY_PATH"}},
            {CURLINFO_CERTINFO, {"CURLINFO_CERTINFO","Certificate chain. See CURLINFO_CERTINFO"}},
            {CURLINFO_TLS_SSL_PTR, {"CURLINFO_TLS_SSL_PTR","TLS session info that can be used for further processing. See CURLINFO_TLS_SSL_PTR"}},
            {CURLINFO_TLS_SESSION, {"CURLINFO_TLS_SESSION","TLS session info that can be used for further processing. See CURLINFO_TLS_SESSION. Deprecated option, use CURLINFO_TLS_SSL_PTR instead!"}},
            {CURLINFO_CONDITION_UNMET, {"CURLINFO_CONDITION_UNMET","Whether or not a time conditional was met. See CURLINFO_CONDITION_UNMET"}},
            {CURLINFO_RTSP_SESSION_ID, {"CURLINFO_RTSP_SESSION_ID","RTSP session ID. See CURLINFO_RTSP_SESSION_ID"}},
            {CURLINFO_RTSP_CLIENT_CSEQ, {"CURLINFO_RTSP_CLIENT_CSEQ","RTSP CSeq that will next be used. See CURLINFO_RTSP_CLIENT_CSEQ"}},
            {CURLINFO_RTSP_SERVER_CSEQ, {"CURLINFO_RTSP_SERVER_CSEQ","RTSP CSeq that will next be expected. See CURLINFO_RTSP_SERVER_CSEQ"}},
            {CURLINFO_RTSP_CSEQ_RECV, {"CURLINFO_RTSP_CSEQ_RECV","RTSP CSeq last received. See CURLINFO_RTSP_CSEQ_RECV"}},
            {CURLINFO_PROTOCOL, {"CURLINFO_PROTOCOL","The protocol used for the connection. (Added in 7.52.0) See CURLINFO_PROTOCOL"}},
            {CURLINFO_SCHEME, {"CURLINFO_SCHEME","The scheme used for the connection. (Added in 7.52.0) See CURLINFO_SCHEME"}},
        };

        void asString(stringstream &ss, CURL *c_handle, CURLINFO kurl){
            char *strValue = NULL;
            curl_easy_getinfo(c_handle, kurl, &strValue);
            if(strValue)
                ss << curl_info.find(kurl)->second[0] << ": " << strValue << "  (" << curl_info.find(kurl)->second[1] << ")" << endl;
            else
                ss << curl_info.find(kurl)->second[0] << ": **MISSING** (" << curl_info.find(kurl)->second[1] << ")" << endl;
        }

        void asLong(stringstream &ss, CURL *c_handle, CURLINFO kurl){
            long lintValue;
            curl_easy_getinfo(c_handle, kurl, &lintValue);
            ss << curl_info.find(kurl)->second[0] << ": "<< lintValue << "  (" << curl_info.find(kurl)->second[1] << ")" << endl;
        }

        void asDouble(stringstream &ss, CURL *c_handle, CURLINFO kurl){
            double dValue;
            curl_easy_getinfo(c_handle, kurl, &dValue);
            ss << curl_info.find(kurl)->second[0] << ": "<< dValue << "  (" << curl_info.find(kurl)->second[1] << ")" << endl;
        }

        void asCurlOffT(stringstream &ss, CURL *c_handle, CURLINFO kurl){
            curl_off_t coft_value;
            curl_easy_getinfo(c_handle, kurl, &coft_value);
            ss << curl_info.find(kurl)->second[0] << ": "<< coft_value << "  (" << curl_info.find(kurl)->second[1] << ")" << endl;
        }

        void asCurlSList(stringstream &ss, CURL *c_handle, CURLINFO kurl){
            struct curl_slist *engine_list;
            curl_easy_getinfo(c_handle, kurl, &engine_list);
            ss << curl_info.find(kurl)->second[0] << ": "<< engine_list << "  (" << curl_info.find(kurl)->second[1] << ")" << endl;

            curl_slist_free_all(engine_list);
        }

        void asCurlSocket(stringstream &ss, CURL *c_handle, CURLINFO kurl){
            curl_socket_t *c_sock;
            curl_easy_getinfo(c_handle, kurl, &c_sock);
            ss << curl_info.find(kurl)->second[0] << ": "<< c_sock << "  (" << curl_info.find(kurl)->second[1] << ")" << endl;
        }

        void asCurlCertInfo(stringstream &ss, CURL *c_handle, CURLINFO kurl){
            struct curl_certinfo *chainp;
            curl_easy_getinfo(c_handle, kurl, &chainp);
            ss << curl_info.find(kurl)->second[0] << ": "<< chainp << "  (" << curl_info.find(kurl)->second[1] << ")" << endl;

        }

        void asCurlTlsSessionInfo(stringstream &ss, CURL *c_handle, CURLINFO kurl){
            struct curl_tlssessioninfo *session;
            curl_easy_getinfo(c_handle, kurl, &session);
            ss << curl_info.find(kurl)->second[0] << ": "<< session << "  (" << curl_info.find(kurl)->second[1] << ")" << endl;

        }

        string probe_curl_handle(CURL *c_handle){
            stringstream ss;

            asString(ss, c_handle,CURLINFO_EFFECTIVE_URL);
            asLong(ss, c_handle,CURLINFO_RESPONSE_CODE);

            asLong(ss, c_handle,CURLINFO_HTTP_CONNECTCODE);
            asLong(ss, c_handle,CURLINFO_HTTP_VERSION);
            asLong(ss, c_handle,CURLINFO_FILETIME);
            //asCurlOffT(ss, c_handle,CURLINFO_FILETIME_T);
            asLong(ss, c_handle,CURLINFO_TOTAL_TIME);
            //asCurlOffT(ss, c_handle,CURLINFO_TOTAL_TIME_T);
            asDouble(ss, c_handle,CURLINFO_NAMELOOKUP_TIME);
            //asCurlOffT(ss, c_handle,CURLINFO_NAMELOOKUP_TIME_T);
            asDouble(ss, c_handle,CURLINFO_CONNECT_TIME);
            //asCurlOffT(ss, c_handle,CURLINFO_CONNECT_TIME_T);
            asDouble(ss, c_handle,CURLINFO_APPCONNECT_TIME);
            //asCurlOffT(ss, c_handle,CURLINFO_APPCONNECT_TIME_T);
            asDouble(ss, c_handle,CURLINFO_PRETRANSFER_TIME);
            //asCurlOffT(ss, c_handle,CURLINFO_PRETRANSFER_TIME_T);
            asDouble(ss, c_handle,CURLINFO_STARTTRANSFER_TIME);
            //asCurlOffT(ss, c_handle,CURLINFO_STARTTRANSFER_TIME_T);
            asDouble(ss, c_handle,CURLINFO_REDIRECT_TIME);
            //asCurlOffT(ss, c_handle,CURLINFO_REDIRECT_TIME_T);
            asLong(ss, c_handle,CURLINFO_REDIRECT_COUNT);
            asString(ss, c_handle,CURLINFO_REDIRECT_URL);
            asDouble(ss, c_handle,CURLINFO_SIZE_UPLOAD);
            //asCurlOffT(ss, c_handle,CURLINFO_SIZE_UPLOAD_T);
            asDouble(ss, c_handle,CURLINFO_SIZE_DOWNLOAD);
            // asCurlOffT(ss, c_handle,CURLINFO_SIZE_DOWNLOAD_T);
            asDouble(ss, c_handle,CURLINFO_SPEED_DOWNLOAD);
            // asCurlOffT(ss, c_handle,CURLINFO_SPEED_DOWNLOAD_T);
            asDouble(ss, c_handle,CURLINFO_SPEED_UPLOAD);
            // asCurlOffT(ss, c_handle,CURLINFO_SPEED_UPLOAD_T);
            asLong(ss, c_handle,CURLINFO_HEADER_SIZE);
            asLong(ss, c_handle,CURLINFO_REQUEST_SIZE);
            asLong(ss, c_handle,CURLINFO_SSL_VERIFYRESULT);
            asLong(ss, c_handle,CURLINFO_PROXY_SSL_VERIFYRESULT);
            asCurlSList(ss, c_handle,CURLINFO_SSL_ENGINES);
            asDouble(ss, c_handle,CURLINFO_CONTENT_LENGTH_DOWNLOAD);
            // asCurlOffT(ss, c_handle,CURLINFO_CONTENT_LENGTH_DOWNLOAD_T);
            asDouble(ss, c_handle,CURLINFO_CONTENT_LENGTH_UPLOAD);
            // asCurlOffT(ss, c_handle,CURLINFO_CONTENT_LENGTH_UPLOAD_T);
            asString(ss, c_handle,CURLINFO_CONTENT_TYPE);
            // asCurlOffT(ss, c_handle,CURLINFO_RETRY_AFTER);
            asString(ss, c_handle,CURLINFO_PRIVATE);
            asLong(ss, c_handle,CURLINFO_HTTPAUTH_AVAIL);
            asLong(ss, c_handle,CURLINFO_PROXYAUTH_AVAIL);
            asLong(ss, c_handle,CURLINFO_OS_ERRNO);
            asLong(ss, c_handle,CURLINFO_NUM_CONNECTS);
            asString(ss, c_handle,CURLINFO_PRIMARY_IP);
            asLong(ss, c_handle,CURLINFO_PRIMARY_PORT);
            asString(ss, c_handle,CURLINFO_LOCAL_IP);
            asLong(ss, c_handle,CURLINFO_LOCAL_PORT);
            asCurlSList(ss, c_handle,CURLINFO_COOKIELIST);
            asLong(ss, c_handle,CURLINFO_LASTSOCKET);
            asCurlSocket(ss, c_handle,CURLINFO_ACTIVESOCKET);
            asString(ss, c_handle,CURLINFO_FTP_ENTRY_PATH);
            asCurlCertInfo(ss,c_handle,CURLINFO_CERTINFO);
            asCurlTlsSessionInfo(ss,c_handle,CURLINFO_TLS_SSL_PTR);
            asCurlTlsSessionInfo(ss,c_handle,CURLINFO_TLS_SESSION);
            asLong(ss, c_handle,CURLINFO_CONDITION_UNMET);
            asString(ss, c_handle,CURLINFO_RTSP_SESSION_ID);
            asLong(ss, c_handle,CURLINFO_RTSP_CLIENT_CSEQ);
            asLong(ss, c_handle,CURLINFO_RTSP_SERVER_CSEQ);
            asLong(ss, c_handle,CURLINFO_RTSP_CSEQ_RECV);
            asLong(ss, c_handle,CURLINFO_PROTOCOL);
            asString(ss, c_handle,CURLINFO_SCHEME);
            return ss.str();
        }

    public:
        string cm_config;

        // Called once before everything gets tested
        NgapCredentialsTest() {
        }

        // Called at the end of the test
        ~NgapCredentialsTest() {
        }

        // Called before each test
        void setUp() {
            if (debug) cout << endl;
            if (bes_debug) BESDebug::SetUp("cerr,dmrpp");

            TheBESKeys::ConfigFile = string(TEST_BUILD_DIR).append("/bes.conf");
            cm_config = string(TEST_BUILD_DIR).append("/credentials.conf");
            weak_config = string(TEST_SRC_DIR).append("/input-files/weak.conf");

        }

        // Called after each test
        void tearDown() {
        }


        /**
         *
         * @param target_url
         * @return
         */
        CURL *set_up_curl_handle(string target_url, char *response_buff) {
            CURL *d_handle;     ///< The libcurl handle object.
            d_handle = curl_easy_init();
            CPPUNIT_ASSERT(d_handle);

            CURLcode res;

            if (CURLE_OK != (res = curl_easy_setopt(d_handle, CURLOPT_ERRORBUFFER, d_errbuf)))
                throw BESInternalError(string("CURL Error: ").append(curl_easy_strerror(res)), __FILE__, __LINE__);

            // Pass all data to the 'write_data' function
            if (CURLE_OK != (res = curl_easy_setopt(d_handle, CURLOPT_WRITEFUNCTION, dmrpp::ngap_write_data)))
                throw BESInternalError(string("CURL Error: ").append(curl_error_msg(res, d_errbuf)), __FILE__,
                                       __LINE__);

            if (CURLE_OK != (res = curl_easy_setopt(d_handle, CURLOPT_URL, target_url.c_str())))
                throw BESInternalError(string("HTTP Error setting URL: ").append(curl_error_msg(res, d_errbuf)),
                                       __FILE__, __LINE__);

            if (CURLE_OK != (res = curl_easy_setopt(d_handle, CURLOPT_FOLLOWLOCATION, 1L)))
                throw BESInternalError(string("Error setting CURLOPT_FOLLOWLOCATION: ").append(curl_error_msg(res, d_errbuf)),
                                       __FILE__, __LINE__);

            if(debug) cout << "uid: " << uid << endl;
            if (CURLE_OK != (res = curl_easy_setopt(d_handle, CURLOPT_USERNAME, uid.c_str())))
                throw BESInternalError(string("Error setting CURLOPT_USERNAME: ").append(curl_error_msg(res, d_errbuf)),
                                       __FILE__, __LINE__);

            if(debug) cout << "pw: " << pw << endl;
            if (CURLE_OK != (res = curl_easy_setopt(d_handle, CURLOPT_PASSWORD, pw.c_str())))
                throw BESInternalError(string("Error setting CURLOPT_PASSWORD: ").append(curl_error_msg(res, d_errbuf)),
                                       __FILE__, __LINE__);

            if (CURLE_OK != (res = curl_easy_setopt(d_handle, CURLOPT_HTTPAUTH, CURLAUTH_ANY)))
                throw BESInternalError(string("Error setting CURLOPT_HTTPAUTH to CURLAUTH_ANY msg: ").append(curl_error_msg(res, d_errbuf)),
                                       __FILE__, __LINE__);


            // Pass this to write_data as the fourth argument
            if (CURLE_OK !=
                (res = curl_easy_setopt(d_handle, CURLOPT_WRITEDATA, reinterpret_cast<void *>(response_buff))))
                throw BESInternalError(
                        string("CURL Error setting chunk as data buffer: ").append(curl_error_msg(res, d_errbuf)),
                        __FILE__, __LINE__);

            return d_handle;
        }

        bool evaluate_curl_response(CURL *eh) {
            long http_code = 0;
            CURLcode res = curl_easy_getinfo(eh, CURLINFO_RESPONSE_CODE, &http_code);
            if (CURLE_OK != res) {
                throw BESInternalError(
                        string("Error getting HTTP response code: ").append(curl_error_msg(res, (char *) "")),
                        __FILE__, __LINE__);
            }

            // Newer Apache servers return 206 for range requests. jhrg 8/8/18
            switch (http_code) {
                case 200: // OK
                case 206: // Partial content - this is to be expected since we use range gets
                    // cases 201-205 are things we should probably reject, unless we add more
                    // comprehensive HTTP/S processing here. jhrg 8/8/18
                    return true;

                case 500: // Internal server error
                case 503: // Service Unavailable
                case 504: // Gateway Timeout
                    return false;

                default: {
                    ostringstream oss;
                    oss << "HTTP status error: Expected an OK status, but got: ";
                    oss << http_code;
                    throw BESInternalError(oss.str(), __FILE__, __LINE__);
                }
            }
        }

        void get_s3_creds() {
            if(debug) cout << endl;
            string distribution_api_endpoint = "https://d33imu0z1ajyhj.cloudfront.net/s3credentials";
            string fnoc1_dds = "http://test.opendap.org/opendap/data/nc/fnoc1.nc.dds";

            string target_url = distribution_api_endpoint;

            if(debug) cout << "Target URL: " << target_url<< endl;

            char response_buf[1024 * 1024];
            try {
                CURL *c_handle = set_up_curl_handle(target_url, response_buf);
                read_data(c_handle);
                string response(response_buf);
                cout << response << endl;
            }
            catch (BESError e) {
                cerr << "Caught BESError. Message: " << e.get_message() << "  ";
                cerr << "[" << e.get_file() << ":" << e.get_line() << "]" << endl;
                CPPUNIT_ASSERT(false);
            }


        }

        void read_data(CURL *c_handle) {

            unsigned int tries = 0;
            unsigned int retry_limit = 3;
            useconds_t retry_time = 1000;
            bool success;
            CURLcode curl_code;

            string url = "URL assignment failed.";
            char *urlp = NULL;
            curl_easy_getinfo(c_handle, CURLINFO_EFFECTIVE_URL, &urlp);
            if(!urlp)
                throw BESInternalError(url,__FILE__,__LINE__);

            url = urlp;


            do {
                d_errbuf[0] = NULL;
                curl_code = curl_easy_perform(c_handle);
                ++tries;

                if (CURLE_OK != curl_code) {
                    throw BESInternalError(
                            string("read_data() - ERROR! Message: ").append(curl_error_msg(curl_code, d_errbuf)),
                            __FILE__, __LINE__);
                }

                success = evaluate_curl_response(c_handle);
                if(debug) cout << probe_curl_handle(c_handle) << endl;

                if (!success) {
                    if (tries == retry_limit) {
                        throw BESInternalError(
                                string("Data transfer error: Number of re-tries to S3 exceeded: ").append(
                                        curl_error_msg(curl_code, d_errbuf)), __FILE__, __LINE__);
                    } else {
                        BESDEBUG("dmrpp",
                                 "HTTP transfer 500 error, will retry (trial " << tries << " for: " << url << ").");
                        usleep(retry_time);
                        retry_time *= 2;
                    }
                }
#if 0
                curl_slist_free_all(d_headers);
                d_headers = 0;
#endif
            } while (!success);
        }


    CPPUNIT_TEST_SUITE(NgapCredentialsTest);

            CPPUNIT_TEST(get_s3_creds);

        CPPUNIT_TEST_SUITE_END();

    };

    CPPUNIT_TEST_SUITE_REGISTRATION(NgapCredentialsTest);

    static string cm_config;

} // namespace dmrpp

int main(int argc, char *argv[]) {
    CppUnit::TextTestRunner runner;
    runner.addTest(CppUnit::TestFactoryRegistry::getRegistry().makeTest());
    string cm_cnf = "";
    dmrpp::uid = "";
    dmrpp::pw = "";

    GetOpt getopt(argc, argv, "c:dbu:p:");
    int option_char;
    while ((option_char = getopt()) != -1)
        switch (option_char) {
            case 'c':
                cm_cnf = getopt.optarg;
                break;
            case 'u':
                dmrpp::uid = getopt.optarg;
                break;
            case 'p':
                dmrpp::pw = getopt.optarg;
                break;
            case 'd':
                debug = true;  // debug is a static global
                break;
            case 'b':
                debug = true;  // debug is a static global
                bes_debug = true;  // debug is a static global
                break;
            default:
                break;
        }

    bool wasSuccessful = true;
    int i = getopt.optind;




    if (i == argc) {
        // run them all
        wasSuccessful = runner.run("");
    } else {
        while (i < argc) {
            if (debug) cerr << "Running " << argv[i] << endl;
            string test = dmrpp::NgapCredentialsTest::suite()->getName().append("::").append(argv[i]);
            wasSuccessful = wasSuccessful && runner.run(test);
            ++i;
        }
    }

    return wasSuccessful ? 0 : 1;
}
