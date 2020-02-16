//
// Created by ndp on 2/15/20.
//

#include "curl_utils.h"
#include <sstream>
#include <map>
#include <vector>

using std::endl, std::string, std::map, std::vector, std::stringstream, std::ostringstream;

namespace curl {


        string error_message(const CURLcode response_code, char *error_buf){
            ostringstream oss;
            size_t len = strlen(error_buf);
            if (len) {
                oss << error_buf;
                oss << " (code: " << (int) response_code << ")";
            } else {
                oss << curl_easy_strerror(response_code) << "(result: " << response_code << ")";
            }
            return oss.str();
        }

        map<CURLINFO, vector<string>> CURL_MESSAGE_INFO = {
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


        void curlValueAsString(stringstream &ss, CURL *c_handle, CURLINFO kurl){
            char *strValue = NULL;
            curl_easy_getinfo(c_handle, kurl, &strValue);
            if(strValue)
                ss << CURL_MESSAGE_INFO.find(kurl)->second[0] << ": " << strValue << "  (" << CURL_MESSAGE_INFO.find(kurl)->second[1] << ")" << endl;
            else
                ss << CURL_MESSAGE_INFO.find(kurl)->second[0] << ": **MISSING** (" << CURL_MESSAGE_INFO.find(kurl)->second[1] << ")" << endl;
        }

        void curlValueAsLong(stringstream &ss, CURL *c_handle, CURLINFO kurl){
            long lintValue;
            curl_easy_getinfo(c_handle, kurl, &lintValue);
            ss << CURL_MESSAGE_INFO.find(kurl)->second[0] << ": " << lintValue << "  (" << CURL_MESSAGE_INFO.find(kurl)->second[1] << ")" << endl;
        }

        void curlValueAsDouble(stringstream &ss, CURL *c_handle, CURLINFO kurl){
            double dValue;
            curl_easy_getinfo(c_handle, kurl, &dValue);
            ss << CURL_MESSAGE_INFO.find(kurl)->second[0] << ": " << dValue << "  (" << CURL_MESSAGE_INFO.find(kurl)->second[1] << ")" << endl;
        }

        void curlValueAsCurlOffT(stringstream &ss, CURL *c_handle, CURLINFO kurl){
            curl_off_t coft_value;
            curl_easy_getinfo(c_handle, kurl, &coft_value);
            ss << CURL_MESSAGE_INFO.find(kurl)->second[0] << ": " << coft_value << "  (" << CURL_MESSAGE_INFO.find(kurl)->second[1] << ")" << endl;
        }

        void curlValueAsCurlSList(stringstream &ss, CURL *c_handle, CURLINFO kurl){
            struct curl_slist *engine_list;
            curl_easy_getinfo(c_handle, kurl, &engine_list);
            ss << CURL_MESSAGE_INFO.find(kurl)->second[0] << ": " << engine_list << "  (" << CURL_MESSAGE_INFO.find(kurl)->second[1] << ")" << endl;

            curl_slist_free_all(engine_list);
        }

        void curlValueAsCurlSocket(stringstream &ss, CURL *c_handle, CURLINFO kurl){
            curl_socket_t *c_sock;
            curl_easy_getinfo(c_handle, kurl, &c_sock);
            ss << CURL_MESSAGE_INFO.find(kurl)->second[0] << ": " << c_sock << "  (" << CURL_MESSAGE_INFO.find(kurl)->second[1] << ")" << endl;
        }

        void curlValueAsCurlCertInfo(stringstream &ss, CURL *c_handle, CURLINFO kurl){
            struct curl_certinfo *chainp;
            curl_easy_getinfo(c_handle, kurl, &chainp);
            ss << CURL_MESSAGE_INFO.find(kurl)->second[0] << ": " << chainp << "  (" << CURL_MESSAGE_INFO.find(kurl)->second[1] << ")" << endl;

        }

        void curlValueAsCurlTlsSessionInfo(stringstream &ss, CURL *c_handle, CURLINFO kurl){
            struct curl_tlssessioninfo *session;
            curl_easy_getinfo(c_handle, kurl, &session);
            ss << CURL_MESSAGE_INFO.find(kurl)->second[0] << ": " << session << "  (" << CURL_MESSAGE_INFO.find(kurl)->second[1] << ")" << endl;

        }



    string probe_easy_handle(CURL *c_handle){
        stringstream ss;

        curlValueAsString(ss, c_handle, CURLINFO_EFFECTIVE_URL);
        curlValueAsLong(ss, c_handle, CURLINFO_RESPONSE_CODE);

        curlValueAsLong(ss, c_handle, CURLINFO_HTTP_CONNECTCODE);
        curlValueAsLong(ss, c_handle, CURLINFO_HTTP_VERSION);
        curlValueAsLong(ss, c_handle, CURLINFO_FILETIME);
        //curlValueAsCurlOffT(ss, c_handle,CURLINFO_FILETIME_T);
        curlValueAsLong(ss, c_handle, CURLINFO_TOTAL_TIME);
        //curlValueAsCurlOffT(ss, c_handle,CURLINFO_TOTAL_TIME_T);
        curlValueAsDouble(ss, c_handle, CURLINFO_NAMELOOKUP_TIME);
        //curlValueAsCurlOffT(ss, c_handle,CURLINFO_NAMELOOKUP_TIME_T);
        curlValueAsDouble(ss, c_handle, CURLINFO_CONNECT_TIME);
        //curlValueAsCurlOffT(ss, c_handle,CURLINFO_CONNECT_TIME_T);
        curlValueAsDouble(ss, c_handle, CURLINFO_APPCONNECT_TIME);
        //curlValueAsCurlOffT(ss, c_handle,CURLINFO_APPCONNECT_TIME_T);
        curlValueAsDouble(ss, c_handle, CURLINFO_PRETRANSFER_TIME);
        //curlValueAsCurlOffT(ss, c_handle,CURLINFO_PRETRANSFER_TIME_T);
        curlValueAsDouble(ss, c_handle, CURLINFO_STARTTRANSFER_TIME);
        //curlValueAsCurlOffT(ss, c_handle,CURLINFO_STARTTRANSFER_TIME_T);
        curlValueAsDouble(ss, c_handle, CURLINFO_REDIRECT_TIME);
        //curlValueAsCurlOffT(ss, c_handle,CURLINFO_REDIRECT_TIME_T);
        curlValueAsLong(ss, c_handle, CURLINFO_REDIRECT_COUNT);
        curlValueAsString(ss, c_handle, CURLINFO_REDIRECT_URL);
        curlValueAsDouble(ss, c_handle, CURLINFO_SIZE_UPLOAD);
        //curlValueAsCurlOffT(ss, c_handle,CURLINFO_SIZE_UPLOAD_T);
        curlValueAsDouble(ss, c_handle, CURLINFO_SIZE_DOWNLOAD);
        // curlValueAsCurlOffT(ss, c_handle,CURLINFO_SIZE_DOWNLOAD_T);
        curlValueAsDouble(ss, c_handle, CURLINFO_SPEED_DOWNLOAD);
        // curlValueAsCurlOffT(ss, c_handle,CURLINFO_SPEED_DOWNLOAD_T);
        curlValueAsDouble(ss, c_handle, CURLINFO_SPEED_UPLOAD);
        // curlValueAsCurlOffT(ss, c_handle,CURLINFO_SPEED_UPLOAD_T);
        curlValueAsLong(ss, c_handle, CURLINFO_HEADER_SIZE);
        curlValueAsLong(ss, c_handle, CURLINFO_REQUEST_SIZE);
        curlValueAsLong(ss, c_handle, CURLINFO_SSL_VERIFYRESULT);
        curlValueAsLong(ss, c_handle, CURLINFO_PROXY_SSL_VERIFYRESULT);
        curlValueAsCurlSList(ss, c_handle, CURLINFO_SSL_ENGINES);
        curlValueAsDouble(ss, c_handle, CURLINFO_CONTENT_LENGTH_DOWNLOAD);
        // curlValueAsCurlOffT(ss, c_handle,CURLINFO_CONTENT_LENGTH_DOWNLOAD_T);
        curlValueAsDouble(ss, c_handle, CURLINFO_CONTENT_LENGTH_UPLOAD);
        // curlValueAsCurlOffT(ss, c_handle,CURLINFO_CONTENT_LENGTH_UPLOAD_T);
        curlValueAsString(ss, c_handle, CURLINFO_CONTENT_TYPE);
        // curlValueAsCurlOffT(ss, c_handle,CURLINFO_RETRY_AFTER);
        curlValueAsString(ss, c_handle, CURLINFO_PRIVATE);
        curlValueAsLong(ss, c_handle, CURLINFO_HTTPAUTH_AVAIL);
        curlValueAsLong(ss, c_handle, CURLINFO_PROXYAUTH_AVAIL);
        curlValueAsLong(ss, c_handle, CURLINFO_OS_ERRNO);
        curlValueAsLong(ss, c_handle, CURLINFO_NUM_CONNECTS);
        curlValueAsString(ss, c_handle, CURLINFO_PRIMARY_IP);
        curlValueAsLong(ss, c_handle, CURLINFO_PRIMARY_PORT);
        curlValueAsString(ss, c_handle, CURLINFO_LOCAL_IP);
        curlValueAsLong(ss, c_handle, CURLINFO_LOCAL_PORT);
        curlValueAsCurlSList(ss, c_handle, CURLINFO_COOKIELIST);
        curlValueAsLong(ss, c_handle, CURLINFO_LASTSOCKET);
        curlValueAsCurlSocket(ss, c_handle, CURLINFO_ACTIVESOCKET);
        curlValueAsString(ss, c_handle, CURLINFO_FTP_ENTRY_PATH);
        curlValueAsCurlCertInfo(ss, c_handle, CURLINFO_CERTINFO);
        curlValueAsCurlTlsSessionInfo(ss, c_handle, CURLINFO_TLS_SSL_PTR);
        curlValueAsCurlTlsSessionInfo(ss, c_handle, CURLINFO_TLS_SESSION);
        curlValueAsLong(ss, c_handle, CURLINFO_CONDITION_UNMET);
        curlValueAsString(ss, c_handle, CURLINFO_RTSP_SESSION_ID);
        curlValueAsLong(ss, c_handle, CURLINFO_RTSP_CLIENT_CSEQ);
        curlValueAsLong(ss, c_handle, CURLINFO_RTSP_SERVER_CSEQ);
        curlValueAsLong(ss, c_handle, CURLINFO_RTSP_CSEQ_RECV);
        curlValueAsLong(ss, c_handle, CURLINFO_PROTOCOL);
        curlValueAsString(ss, c_handle, CURLINFO_SCHEME);
        return ss.str();
    }
















}