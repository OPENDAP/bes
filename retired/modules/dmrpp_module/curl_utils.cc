//
// Created by ndp on 2/15/20.
//

#include <curl/curl.h>
#include "curl_utils.h"
#include <sstream>
#include <map>
#include <vector>
#include <unistd.h>

#include "rapidjson/document.h"


#include <BESInternalError.h>
#include <BESDebug.h>

#define MODULE "dmrpp:curl"

using std::endl;
using std::string;
using std::map;
using std::vector;
using std::stringstream;
using std::ostringstream;

namespace curl {

    /**
     * Returns a cURL error message string based on the conents of the error_buf or, if the error_buf is empty, the
     * CURLcode code.
     * @param response_code
     * @param error_buf
     * @return
     */
    string error_message(const CURLcode response_code, char *error_buf) {
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


#if 0

    // TODO: Decide if probe_easy_handle(CURL *c_handle) and its many attendents beginning with
    //       the CURL_MESSAGE_INFO business below are one of the following:
    //         a) Worth implementing in CentOS-6. (Because this code does not work there.)
    //         b) Worth krufting until CentOS-7. (Works here, I think...)
    //         c) Time to pitch it.
    /**
     * This map connects CURLINFO types with the string names and descriptions.
     */
    map<CURLINFO, vector<string>> CURL_MESSAGE_INFO = {
            {CURLINFO_EFFECTIVE_URL,           {"CURLINFO_EFFECTIVE_URL",           "Last used URL. See CURLINFO_EFFECTIVE_URL"}},
            {CURLINFO_RESPONSE_CODE,           {"CURLINFO_RESPONSE_CODE",           "Last received response code. See CURLINFO_RESPONSE_CODE"}},
            {CURLINFO_HTTP_CONNECTCODE,        {"CURLINFO_HTTP_CONNECTCODE",        "Last proxy CONNECT response code. See CURLINFO_HTTP_CONNECTCODE"}},
            // {CURLINFO_HTTP_VERSION,            {"CURLINFO_HTTP_VERSION",            "The http version used in the connection. See CURLINFO_HTTP_VERSION"}}, // cURL 7.64
            {CURLINFO_FILETIME,                {"CURLINFO_FILETIME",                "Remote time of the retrieved document. See CURLINFO_FILETIME"}},
            //{CURLINFO_FILETIME_T, {"CURLINFO_FILETIME_T","Remote time of the retrieved document. See CURLINFO_FILETIME_T"}},
            {CURLINFO_TOTAL_TIME,              {"CURLINFO_TOTAL_TIME",              "Total time of previous transfer. See CURLINFO_TOTAL_TIME"}},
            //{CURLINFO_TOTAL_TIME_T, {"CURLINFO_TOTAL_TIME_T","Total time of previous transfer. See CURLINFO_TOTAL_TIME_T"}},
            {CURLINFO_NAMELOOKUP_TIME,         {"CURLINFO_NAMELOOKUP_TIME",         "Time from start until name resolving completed. See CURLINFO_NAMELOOKUP_TIME"}},
            //{CURLINFO_NAMELOOKUP_TIME_T, {"CURLINFO_NAMELOOKUP_TIME_T","Time from start until name resolving completed. See CURLINFO_NAMELOOKUP_TIME_T"}},
            {CURLINFO_CONNECT_TIME,            {"CURLINFO_CONNECT_TIME",            "Time from start until remote host or proxy completed. See CURLINFO_CONNECT_TIME"}},
            //{CURLINFO_CONNECT_TIME_T, {"CURLINFO_CONNECT_TIME_T","Time from start until remote host or proxy completed. See CURLINFO_CONNECT_TIME_T"}},
            {CURLINFO_APPCONNECT_TIME,         {"CURLINFO_APPCONNECT_TIME",         "Time from start until SSL/SSH handshake completed. See CURLINFO_APPCONNECT_TIME"}},
            //{CURLINFO_APPCONNECT_TIME_T, {"CURLINFO_APPCONNECT_TIME_T","Time from start until SSL/SSH handshake completed. See CURLINFO_APPCONNECT_TIME_T"}},
            {CURLINFO_PRETRANSFER_TIME,        {"CURLINFO_PRETRANSFER_TIME",        "Time from start until just before the transfer begins. See CURLINFO_PRETRANSFER_TIME"}},
            //{CURLINFO_PRETRANSFER_TIME_T, {"CURLINFO_PRETRANSFER_TIME_T","Time from start until just before the transfer begins. See CURLINFO_PRETRANSFER_TIME_T"}},
            {CURLINFO_STARTTRANSFER_TIME,      {"CURLINFO_STARTTRANSFER_TIME",      "Time from start until just when the first byte is received. See CURLINFO_STARTTRANSFER_TIME"}},
            //{CURLINFO_STARTTRANSFER_TIME_T, {"CURLINFO_STARTTRANSFER_TIME_T","Time from start until just when the first byte is received. See CURLINFO_STARTTRANSFER_TIME_T"}},
            {CURLINFO_REDIRECT_TIME,           {"CURLINFO_REDIRECT_TIME",           "Time taken for all redirect steps before the final transfer. See CURLINFO_REDIRECT_TIME"}},
            //{CURLINFO_REDIRECT_TIME_T, {"CURLINFO_REDIRECT_TIME_T","Time taken for all redirect steps before the final transfer. See CURLINFO_REDIRECT_TIME_T"}},
            {CURLINFO_REDIRECT_COUNT,          {"CURLINFO_REDIRECT_COUNT",          "Total number of redirects that were followed. See CURLINFO_REDIRECT_COUNT"}},
            {CURLINFO_REDIRECT_URL,            {"CURLINFO_REDIRECT_URL",            "URL a redirect would take you to, had you enabled redirects. See CURLINFO_REDIRECT_URL"}},
            {CURLINFO_SIZE_UPLOAD,             {"CURLINFO_SIZE_UPLOAD",             "(Deprecated) Number of bytes uploaded. See CURLINFO_SIZE_UPLOAD"}},
            //{CURLINFO_SIZE_UPLOAD_T, {"CURLINFO_SIZE_UPLOAD_T","Number of bytes uploaded. See CURLINFO_SIZE_UPLOAD_T"}},
            {CURLINFO_SIZE_DOWNLOAD,           {"CURLINFO_SIZE_DOWNLOAD",           "(Deprecated) Number of bytes downloaded. See CURLINFO_SIZE_DOWNLOAD"}},
            //{CURLINFO_SIZE_DOWNLOAD_T, {"CURLINFO_SIZE_DOWNLOAD_T","Number of bytes downloaded. See CURLINFO_SIZE_DOWNLOAD_T"}},
            {CURLINFO_SPEED_DOWNLOAD,          {"CURLINFO_SPEED_DOWNLOAD",          "(Deprecated) Average download speed. See CURLINFO_SPEED_DOWNLOAD"}},
            //{CURLINFO_SPEED_DOWNLOAD_T, {"CURLINFO_SPEED_DOWNLOAD_T","Average download speed. See CURLINFO_SPEED_DOWNLOAD_T"}},
            {CURLINFO_SPEED_UPLOAD,            {"CURLINFO_SPEED_UPLOAD",            "(Deprecated) Average upload speed. See CURLINFO_SPEED_UPLOAD"}},
            //{CURLINFO_SPEED_UPLOAD_T, {"CURLINFO_SPEED_UPLOAD_T","Average upload speed. See CURLINFO_SPEED_UPLOAD_T"}},
            {CURLINFO_HEADER_SIZE,             {"CURLINFO_HEADER_SIZE",             "Number of bytes of all headers received. See CURLINFO_HEADER_SIZE"}},
            {CURLINFO_REQUEST_SIZE,            {"CURLINFO_REQUEST_SIZE",            "Number of bytes sent in the issued HTTP requests. See CURLINFO_REQUEST_SIZE"}},
            {CURLINFO_SSL_VERIFYRESULT,        {"CURLINFO_SSL_VERIFYRESULT",        "Certificate verification result. See CURLINFO_SSL_VERIFYRESULT"}},
            // {CURLINFO_PROXY_SSL_VERIFYRESULT,  {"CURLINFO_PROXY_SSL_VERIFYRESULT",  "Proxy certificate verification result. See CURLINFO_PROXY_SSL_VERIFYRESULT"}}, // cURL 7.64
            {CURLINFO_SSL_ENGINES,             {"CURLINFO_SSL_ENGINES",             "A list of OpenSSL crypto engines. See CURLINFO_SSL_ENGINES"}},
            {CURLINFO_CONTENT_LENGTH_DOWNLOAD, {"CURLINFO_CONTENT_LENGTH_DOWNLOAD", "(Deprecated) Content length from the Content-Length header. See CURLINFO_CONTENT_LENGTH_DOWNLOAD"}},
            //{CURLINFO_CONTENT_LENGTH_DOWNLOAD_T, {"CURLINFO_CONTENT_LENGTH_DOWNLOAD_T","Content length from the Content-Length header. See CURLINFO_CONTENT_LENGTH_DOWNLOAD_T"}},
            {CURLINFO_CONTENT_LENGTH_UPLOAD,   {"CURLINFO_CONTENT_LENGTH_UPLOAD",   "(Deprecated) Upload size. See CURLINFO_CONTENT_LENGTH_UPLOAD"}},
            //{CURLINFO_CONTENT_LENGTH_UPLOAD_T, {"CURLINFO_CONTENT_LENGTH_UPLOAD_T","Upload size. See CURLINFO_CONTENT_LENGTH_UPLOAD_T"}},
            {CURLINFO_CONTENT_TYPE,            {"CURLINFO_CONTENT_TYPE",            "Content type from the Content-Type header. See CURLINFO_CONTENT_TYPE"}},
            //{CURLINFO_RETRY_AFTER, {"CURLINFO_RETRY_AFTER","The value from the from the Retry-After header. See CURLINFO_RETRY_AFTER"}},
            {CURLINFO_PRIVATE,                 {"CURLINFO_PRIVATE",                 "User's private data pointer. See CURLINFO_PRIVATE"}},
            {CURLINFO_HTTPAUTH_AVAIL,          {"CURLINFO_HTTPAUTH_AVAIL",          "Available HTTP authentication methods. See CURLINFO_HTTPAUTH_AVAIL"}},
            {CURLINFO_PROXYAUTH_AVAIL,         {"CURLINFO_PROXYAUTH_AVAIL",         "Available HTTP proxy authentication methods. See CURLINFO_PROXYAUTH_AVAIL"}},
            {CURLINFO_OS_ERRNO,                {"CURLINFO_OS_ERRNO",                "The errno from the last failure to connect. See CURLINFO_OS_ERRNO"}},
            {CURLINFO_NUM_CONNECTS,            {"CURLINFO_NUM_CONNECTS",            "Number of new successful connections used for previous transfer. See CURLINFO_NUM_CONNECTS"}},
            {CURLINFO_PRIMARY_IP,              {"CURLINFO_PRIMARY_IP",              "IP address of the last connection. See CURLINFO_PRIMARY_IP"}},
            // {CURLINFO_PRIMARY_PORT,            {"CURLINFO_PRIMARY_PORT",            "Port of the last connection. See CURLINFO_PRIMARY_PORT"}},  // cURL 7.64
            // {CURLINFO_LOCAL_IP,                {"CURLINFO_LOCAL_IP",                "Local-end IP address of last connection. See CURLINFO_LOCAL_IP"}},  // cURL 7.64
            // {CURLINFO_LOCAL_PORT,              {"CURLINFO_LOCAL_PORT",              "Local-end port of last connection. See CURLINFO_LOCAL_PORT"}},  // cURL 7.64
            {CURLINFO_COOKIELIST,              {"CURLINFO_COOKIELIST",              "List of all known cookies. See CURLINFO_COOKIELIST"}},
            {CURLINFO_LASTSOCKET,              {"CURLINFO_LASTSOCKET",              "Last socket used. See CURLINFO_LASTSOCKET"}},
            // {CURLINFO_ACTIVESOCKET,            {"CURLINFO_ACTIVESOCKET",            "The session's active socket. See CURLINFO_ACTIVESOCKET"}}, // cURL 7.64
            {CURLINFO_FTP_ENTRY_PATH,          {"CURLINFO_FTP_ENTRY_PATH",          "The entry path after logging in to an FTP server. See CURLINFO_FTP_ENTRY_PATH"}},
            {CURLINFO_CERTINFO,                {"CURLINFO_CERTINFO",                "Certificate chain. See CURLINFO_CERTINFO"}},
            // {CURLINFO_TLS_SSL_PTR,             {"CURLINFO_TLS_SSL_PTR",             "TLS session info that can be used for further processing. See CURLINFO_TLS_SSL_PTR"}}, // cURL 7.64
            //{CURLINFO_TLS_SESSION,             {"CURLINFO_TLS_SESSION",             "TLS session info that can be used for further processing. See CURLINFO_TLS_SESSION. Deprecated option, use CURLINFO_TLS_SSL_PTR instead!"}}, //breaks on centos 7. sbl 3/11/20
            {CURLINFO_CONDITION_UNMET,         {"CURLINFO_CONDITION_UNMET",         "Whether or not a time conditional was met. See CURLINFO_CONDITION_UNMET"}}
            // {CURLINFO_RTSP_SESSION_ID,         {"CURLINFO_RTSP_SESSION_ID",         "RTSP session ID. See CURLINFO_RTSP_SESSION_ID"}}, // Not on CentOS-6
            // {CURLINFO_RTSP_CLIENT_CSEQ,        {"CURLINFO_RTSP_CLIENT_CSEQ",        "RTSP CSeq that will next be used. See CURLINFO_RTSP_CLIENT_CSEQ"}},  // cURL 7.64
            // {CURLINFO_RTSP_SERVER_CSEQ,        {"CURLINFO_RTSP_SERVER_CSEQ",        "RTSP CSeq that will next be expected. See CURLINFO_RTSP_SERVER_CSEQ"}},  // cURL 7.64
            // {CURLINFO_RTSP_CSEQ_RECV,          {"CURLINFO_RTSP_CSEQ_RECV",          "RTSP CSeq last received. See CURLINFO_RTSP_CSEQ_RECV"}},  // cURL 7.64
            // {CURLINFO_PROTOCOL,                {"CURLINFO_PROTOCOL",                "The protocol used for the connection. (Added in 7.52.0) See CURLINFO_PROTOCOL"}}, // cURL 7.64
            // {CURLINFO_SCHEME,                  {"CURLINFO_SCHEME",                  "The scheme used for the connection. (Added in 7.52.0) See CURLINFO_SCHEME"}}, // cURL 7.64
    };


    /**
     * Helper function for probe_easy_handle() to handle String type
     * @param ss output goes in here
     * @param c_handle cURL easy handle to interrogate
     * @param kurl The CURLINFO feature to examine.
    */
    void curlValueAsString(stringstream &ss, CURL *c_handle, CURLINFO kurl) {
        char *strValue = NULL;
        curl_easy_getinfo(c_handle, kurl, &strValue);
        if (strValue)
            ss << CURL_MESSAGE_INFO.find(kurl)->second[0] << ": " << strValue << "  ("
               << CURL_MESSAGE_INFO.find(kurl)->second[1] << ")" << endl;
        else
            ss << CURL_MESSAGE_INFO.find(kurl)->second[0] << ": **MISSING** ("
               << CURL_MESSAGE_INFO.find(kurl)->second[1] << ")" << endl;
    }

    /**
     * Helper function for probe_easy_handle() to handle Long type
     * @param ss output goes in here
     * @param c_handle cURL easy handle to interrogate
     * @param kurl The CURLINFO feature to examine.
    */
    void curlValueAsLong(stringstream &ss, CURL *c_handle, CURLINFO kurl) {
        long lintValue;
        curl_easy_getinfo(c_handle, kurl, &lintValue);
        ss << CURL_MESSAGE_INFO.find(kurl)->second[0] << ": " << lintValue << "  ("
           << CURL_MESSAGE_INFO.find(kurl)->second[1] << ")" << endl;
    }

    /**
     * Helper function for probe_easy_handle() to handle Double type
     * @param ss output goes in here
     * @param c_handle cURL easy handle to interrogate
     * @param kurl The CURLINFO feature to examine.
    */
    void curlValueAsDouble(stringstream &ss, CURL *c_handle, CURLINFO kurl) {
        double dValue;
        curl_easy_getinfo(c_handle, kurl, &dValue);
        ss << CURL_MESSAGE_INFO.find(kurl)->second[0] << ": " << dValue << "  ("
           << CURL_MESSAGE_INFO.find(kurl)->second[1] << ")" << endl;
    }

    /**
     * Helper function for probe_easy_handle() to handle CurlOffT type
     * @param ss output goes in here
     * @param c_handle cURL easy handle to interrogate
     * @param kurl The CURLINFO feature to examine.
    */
    void curlValueAsCurlOffT(stringstream &ss, CURL *c_handle, CURLINFO kurl) {
        curl_off_t coft_value;
        curl_easy_getinfo(c_handle, kurl, &coft_value);
        ss << CURL_MESSAGE_INFO.find(kurl)->second[0] << ": " << coft_value << "  ("
           << CURL_MESSAGE_INFO.find(kurl)->second[1] << ")" << endl;
    }

    /**
     * Helper function for probe_easy_handle() to handle CurlStructList type
     * @param ss output goes in here
     * @param c_handle cURL easy handle to interrogate
     * @param kurl The CURLINFO feature to examine.
    */
    void curlValueAsCurlSList(stringstream &ss, CURL *c_handle, CURLINFO kurl) {
        struct curl_slist *engine_list;
        curl_easy_getinfo(c_handle, kurl, &engine_list);
        ss << CURL_MESSAGE_INFO.find(kurl)->second[0] << ": " << engine_list << "  ("
           << CURL_MESSAGE_INFO.find(kurl)->second[1] << ")" << endl;

        curl_slist_free_all(engine_list);
    }

    /**
    * Helper function for probe_easy_handle() to handle CurlSocket type
    * @param ss output goes in here
    * @param c_handle cURL easy handle to interrogate
    * @param kurl The CURLINFO feature to examine.
    */
    void curlValueAsCurlSocket(stringstream &ss, CURL *c_handle, CURLINFO kurl) {
        curl_socket_t *c_sock;
        curl_easy_getinfo(c_handle, kurl, &c_sock);
        ss << CURL_MESSAGE_INFO.find(kurl)->second[0] << ": " << c_sock << "  ("
           << CURL_MESSAGE_INFO.find(kurl)->second[1] << ")" << endl;
    }

    /**
    * Helper function for probe_easy_handle() to handle CurlCertInfo type
    * @param ss output goes in here
    * @param c_handle cURL easy handle to interrogate
    * @param kurl The CURLINFO feature to examine.
    */
    void curlValueAsCurlCertInfo(stringstream &ss, CURL *c_handle, CURLINFO kurl) {
        struct curl_certinfo *chainp;
        curl_easy_getinfo(c_handle, kurl, &chainp);
        ss << CURL_MESSAGE_INFO.find(kurl)->second[0] << ": " << chainp << "  ("
           << CURL_MESSAGE_INFO.find(kurl)->second[1] << ")" << endl;

    }

    /**
     * Helper function for probe_easy_handle() to handle CurlTlsSessionInfo type
     * @param ss output goes in here
     * @param c_handle cURL easy handle to interrogate
     * @param kurl The CURLINFO feature to examine.
     */
    void curlValueAsCurlTlsSessionInfo(stringstream &ss, CURL *c_handle, CURLINFO kurl) {
        struct curl_tlssessioninfo *session;
        curl_easy_getinfo(c_handle, kurl, &session);
        ss << CURL_MESSAGE_INFO.find(kurl)->second[0] << ": " << session << "  ("
           << CURL_MESSAGE_INFO.find(kurl)->second[1] << ")" << endl;

    }


    /**
     * Returns a string based on an extensive interrogation of the passed cURL easy handle, c_handle.
     * @param c_handle The cURL easy handle to probe
     * @return The string representation of this probing business.
     */
    string probe_easy_handle(CURL *c_handle) {
        stringstream ss;

        curlValueAsString(ss, c_handle, CURLINFO_EFFECTIVE_URL);
        curlValueAsLong(ss, c_handle, CURLINFO_RESPONSE_CODE);

        curlValueAsLong(ss, c_handle, CURLINFO_HTTP_CONNECTCODE);
        // curlValueAsLong(ss, c_handle, CURLINFO_HTTP_VERSION); // cURL 7.64
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
        // curlValueAsLong(ss, c_handle, CURLINFO_PROXY_SSL_VERIFYRESULT); // cURL 7.64
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
        // curlValueAsLong(ss, c_handle, CURLINFO_PRIMARY_PORT);  // cURL 7.64
        // curlValueAsString(ss, c_handle, CURLINFO_LOCAL_IP); // cURL 7.64
        // curlValueAsLong(ss, c_handle, CURLINFO_LOCAL_PORT); // cURL 7.64
        curlValueAsCurlSList(ss, c_handle, CURLINFO_COOKIELIST);
        curlValueAsLong(ss, c_handle, CURLINFO_LASTSOCKET);
        // curlValueAsCurlSocket(ss, c_handle, CURLINFO_ACTIVESOCKET); // cURL 7.64
        curlValueAsString(ss, c_handle, CURLINFO_FTP_ENTRY_PATH);
        curlValueAsCurlCertInfo(ss, c_handle, CURLINFO_CERTINFO);
        // curlValueAsCurlTlsSessionInfo(ss, c_handle, CURLINFO_TLS_SSL_PTR); // cURL 7.64
        //curlValueAsCurlTlsSessionInfo(ss, c_handle, CURLINFO_TLS_SESSION);  // cURL 7.64
        curlValueAsLong(ss, c_handle, CURLINFO_CONDITION_UNMET);
        // curlValueAsString(ss, c_handle, CURLINFO_RTSP_SESSION_ID); // cURL 7.64
        // curlValueAsLong(ss, c_handle, CURLINFO_RTSP_CLIENT_CSEQ); // cURL 7.64
        // curlValueAsLong(ss, c_handle, CURLINFO_RTSP_SERVER_CSEQ); // cURL 7.64
        // curlValueAsLong(ss, c_handle, CURLINFO_RTSP_CSEQ_RECV); // cURL 7.64
        // curlValueAsLong(ss, c_handle, CURLINFO_PROTOCOL);  // cURL 7.64
        // curlValueAsString(ss, c_handle, CURLINFO_SCHEME);
        return ss.str();
    }
#endif


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
            throw BESInternalError(string("HTTP Error setting URL: ").append(curl::error_message(res, d_errbuf)),
                                   __FILE__, __LINE__);

        // Pass all data to the 'write_data' function ------------------------------------------------------------------
        if (CURLE_OK != (res = curl_easy_setopt(d_handle, CURLOPT_WRITEFUNCTION, c_write_data)))
            throw BESInternalError(string("CURL Error: ").append(curl::error_message(res, d_errbuf)),
                                   __FILE__, __LINE__);

        // Pass this to write_data as the fourth argument --------------------------------------------------------------
        if (CURLE_OK != (res = curl_easy_setopt(d_handle, CURLOPT_WRITEDATA, reinterpret_cast<void *>(response_buff))))
            throw BESInternalError(
                    string("CURL Error setting chunk as data buffer: ").append(curl::error_message(res, d_errbuf)),
                    __FILE__, __LINE__);

        // Follow redirects --------------------------------------------------------------------------------------------
        if (CURLE_OK != (res = curl_easy_setopt(d_handle, CURLOPT_FOLLOWLOCATION, 1L)))
            throw BESInternalError(string("Error setting CURLOPT_FOLLOWLOCATION: ").append(
                    curl::error_message(res, d_errbuf)),
                                   __FILE__, __LINE__);


        // Use cookies -------------------------------------------------------------------------------------------------
        if (CURLE_OK != (res = curl_easy_setopt(d_handle, CURLOPT_COOKIEFILE, cookies_file.c_str())))
            throw BESInternalError(
                    string("Error setting CURLOPT_COOKIEFILE to '").append(cookies_file).append("' msg: ").append(
                            curl::error_message(res, d_errbuf)),
                    __FILE__, __LINE__);

        if (CURLE_OK != (res = curl_easy_setopt(d_handle, CURLOPT_COOKIEJAR, cookies_file.c_str())))
            throw BESInternalError(string("Error setting CURLOPT_COOKIEJAR: ").append(
                    curl::error_message(res, d_errbuf)),
                                   __FILE__, __LINE__);


        // Authenticate using best available ---------------------------------------------------------------------------
        if (CURLE_OK != (res = curl_easy_setopt(d_handle, CURLOPT_HTTPAUTH, (long) CURLAUTH_ANY)))
            throw BESInternalError(string("Error setting CURLOPT_HTTPAUTH to CURLAUTH_ANY msg: ").append(
                    curl::error_message(res, d_errbuf)),
                                   __FILE__, __LINE__);

#if 0
        if(debug) cout << "uid: " << uid << endl;
            if (CURLE_OK != (res = curl_easy_setopt(d_handle, CURLOPT_USERNAME, uid.c_str())))
                throw BESInternalError(string("Error setting CURLOPT_USERNAME: ").append(curl::error_message(res, d_errbuf)),
                                       __FILE__, __LINE__);

            if(debug) cout << "pw: " << pw << endl;
            if (CURLE_OK != (res = curl_easy_setopt(d_handle, CURLOPT_PASSWORD, pw.c_str())))
                throw BESInternalError(string("Error setting CURLOPT_PASSWORD: ").append(curl::error_message(res, d_errbuf)),
                                       __FILE__, __LINE__);
#else
        // Use .netrc for credentials ----------------------------------------------------------------------------------
        if (CURLE_OK != (res = curl_easy_setopt(d_handle, CURLOPT_NETRC, CURL_NETRC_OPTIONAL)))
            throw BESInternalError(string("Error setting CURLOPT_NETRC to CURL_NETRC_OPTIONAL: ").append(
                    curl::error_message(res, d_errbuf)),
                                   __FILE__, __LINE__);
#endif

        // Error Buffer ------------------------------------------------------------------------------------------------
        if (CURLE_OK != (res = curl_easy_setopt(d_handle, CURLOPT_ERRORBUFFER, d_errbuf)))
            throw BESInternalError(string("CURL Error: ").append(curl_easy_strerror(res)), __FILE__, __LINE__);


        return d_handle;
    }


    /**
     * Check the response for errors and such.
     * @param eh
     * @return
     */
    bool eval_get_response(CURL *eh) {
        long http_code = 0;
        CURLcode res = curl_easy_getinfo(eh, CURLINFO_RESPONSE_CODE, &http_code);
        if (CURLE_OK != res) {
            throw BESInternalError(
                    string("Error getting HTTP response code: ").append(curl::error_message(res, (char *) "")),
                    __FILE__, __LINE__);
        }
        char *last_url = 0;
        curl_easy_getinfo(eh, CURLINFO_EFFECTIVE_URL, &last_url);
        BESDEBUG(MODULE, "cURL - CURLINFO_EFFECTIVE_URL: "<< last_url << endl );
        // @TODO @FIXME Expand the list of handled status to at least include the 4** stuff for authentication so that something sensible can be done.
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
                oss << "curl_utils - HTTP status error: Expected an OK status, but got: " << http_code;
                if(BESDebug::IsSet(MODULE)) oss << " from (CURLINFO_EFFECTIVE_URL): " << last_url;
                throw BESInternalError(oss.str(), __FILE__, __LINE__);
            }
        }
    }

    string get_range_arg_string(const unsigned long long &offset, const unsigned long long &size)
    {
        ostringstream range;   // range-get needs a string arg for the range
        range << offset << "-" << offset + size - 1;
        return range.str();
    }

    /**
     * Execute the HTTP VERB from the passed cURL handle "c_handle" and retrieve the response.
     * @param c_handle The cURL easy handle to execute and read.
     */
    void read_data(CURL *c_handle) {

        unsigned int tries = 0;
        unsigned int retry_limit = 3;
        useconds_t retry_time = 1000;
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
                        string("read_data() - ERROR! Message: ").append(curl::error_message(curl_code, curlErrorBuf)),
                        __FILE__, __LINE__);
            }

            success = eval_get_response(c_handle);
            // if(debug) cout << curl::probe_easy_handle(c_handle) << endl;
            if (!success) {
                if (tries == retry_limit) {
                    throw BESInternalError(
                            string("Data transfer error: Number of re-tries to S3 exceeded: ").append(
                                    curl::error_message(curl_code, curlErrorBuf)), __FILE__, __LINE__);
                } else {
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

    /**
     * Derefernce the target URL and put the response in response_buf
     * @param target_url The URL to dereference.
     * @param response_buf The buffer into which to put the response.
     */
    void http_get(const std::string &target_url, char *response_buf) {
        char name[] = "/tmp/ngap_cookiesXXXXXX";
        string cookies = mktemp(name);
        if (cookies.empty())
            throw BESInternalError(string("Failed to make temporary file for HTTP cookies in module 'ngap' (").append(strerror(errno)).append(")"), __FILE__, __LINE__);

        try {
            CURL *c_handle = set_up_easy_handle(target_url, cookies, response_buf);
            read_data(c_handle);
            curl_easy_cleanup(c_handle);
            unlink(cookies.c_str());
        }
        catch(...) {
            unlink(cookies.c_str());
            throw;
        }
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

}
