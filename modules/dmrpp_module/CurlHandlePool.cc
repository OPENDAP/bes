// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of the BES

// Copyright (c) 2018 OPeNDAP, Inc.
// Author: James Gallagher<jgallagher@opendap.org>
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

#include "config.h"

#include <string>
#include <locale>
#include <sstream>

#include <cstring>
#include <unistd.h>

#include <curl/curl.h>

#include "CurlUtils.h"
#include "HttpNames.h"

#include <time.h>

#include "util.h"   // long_to_string()

#include "BESLog.h"
#include "BESDebug.h"
#include "BESInternalError.h"
#include "BESForbiddenError.h"
#include "AllowedHosts.h"

#include "DmrppCommon.h"
#include "DmrppNames.h"
#include "awsv4.h"
#include "CurlHandlePool.h"
#include "Chunk.h"
#include "CredentialsManager.h"
#include "AccessCredentials.h"

#define KEEP_ALIVE 1   // Reuse libcurl easy handles (1) or not (0).
#define CURL_VERBOSE 0  // Logs curl info to the bes.log

#define prolog std::string("CurlHandlePool::").append(__func__).append("() - ")

#if 0
// Shutdown this block of unsed variables. ndp - 3/1/21
//static const int MAX_WAIT_MSECS = 30 * 1000; // Wait max. 30 seconds
//static const unsigned int retry_limit = 10; // Amazon's suggestion
//static const useconds_t uone_second = 1000 * 1000; // one second, in microseconds
//namespace dmrpp {
//const bool have_curl_multi_api = false;
//}
#endif

using namespace dmrpp;
using namespace std;

string pthread_error(unsigned int err){
    string error_msg;
    switch(err){
        case EINVAL:
            error_msg = "The mutex was either created with the "
                        "protocol attribute having the value "
                        "PTHREAD_PRIO_PROTECT and the calling "
                        "thread's priority is higher than the "
                        "mutex's current priority ceiling."
                        "OR The value specified by mutex does not "
                        "refer to an initialized mutex object.";
            break;

        case EBUSY:
            error_msg = "The mutex could not be acquired "
                        "because it was already locked.";
            break;

        case EAGAIN:
            error_msg = "The mutex could not be acquired because "
                        "the maximum number of recursive locks "
                        "for mutex has been exceeded.";
            break;

        case EDEADLK:
            error_msg = "The current thread already owns the mutex";
            break;

        case EPERM:
            error_msg = "The current thread does not own the mutex.";
            break;

        default:
            error_msg = "Unknown pthread error type.";
            break;
    }

    return error_msg;
}

Lock::Lock(pthread_mutex_t &lock) : m_mutex(lock) {
    int status = pthread_mutex_lock(&m_mutex);
    if (status != 0)
        throw BESInternalError(prolog + "Failed to acquire mutex lock. msg: " + pthread_error(status), __FILE__, __LINE__);
}

Lock::~Lock() {
    int status = pthread_mutex_unlock(&m_mutex);
    if (status != 0)
        ERROR_LOG(prolog + "Failed to release mutex lock. msg: " + pthread_error(status));
}

/**
 * @brief Build a string with hex info about stuff libcurl gets
 *
 * Unused, but might be useful someday.
 */
#if 0
static
string dump(const char *text, unsigned char *ptr, size_t size)
{
    size_t i;
    size_t c;
    unsigned int width=0x10;
 
    ostringstream oss;
    oss << text << ", " << std::setw(10) << (long)size << std::setbase(16) << (long)size << endl;

    for(i=0; i<size; i+= width) {
    oss << std::setw(4) << (long)i;
    // fprintf(stream, "%4.4lx: ", (long)i);

    /* show hex to the left */
    for(c = 0; c < width; c++) {
        if(i+c < size) {
        oss << std::setw(2) << ptr[i+c];
        //fprintf(stream, "%02x ", ptr[i+c]);
        }
        else {
        oss << "   ";
        // fputs("   ", stream);
        }
    }

    /* show data on the right */
    for(c = 0; (c < width) && (i+c < size); c++) {
        char x = (ptr[i+c] >= 0x20 && ptr[i+c] < 0x80) ? ptr[i+c] : '.';
        // fputc(x, stream);
        oss << std::setw(1) << x;
    }

    // fputc('\n', stream); /* newline */
    oss << endl;
    }

    return oss.str();
}
#endif

#if CURL_VERBOSE
/**
 * @brief print verbose info from curl.
 *
 * Copied from the libcurl docs, then hacked to remove most of the output.
 */ 
static
int curl_trace(CURL */*handle*/, curl_infotype type, char *data, size_t /*size*/, void */*userp*/)
{
    string text = "";
    switch (type) {
        // print info
        case CURLINFO_TEXT:
        case CURLINFO_HEADER_OUT:
        case CURLINFO_HEADER_IN: {
            text = data;
            size_t pos;
            while ((pos = text.find('\n')) != string::npos)
                text = text.substr(0, pos);
            break;
        }

        // Do not build up 'text' for the data transfers
        case CURLINFO_DATA_OUT:
        case CURLINFO_SSL_DATA_OUT:
        case CURLINFO_DATA_IN:
        case CURLINFO_SSL_DATA_IN:
        default: /* in case a new one is introduced to shock us */
            break;
    }

    switch (type) {
        // print info
        case CURLINFO_TEXT:
            LOG("libcurl == Info: " << text << endl);
            break;

        case CURLINFO_HEADER_OUT:
            LOG("libcurl == Send header: " << text << endl);
            break;
        case CURLINFO_HEADER_IN:
            LOG("libcurl == Recv header: " << text << endl);
            break;

        // Only print these if we're desperate and the above code has been hacked to match
        case CURLINFO_DATA_OUT:
        case CURLINFO_SSL_DATA_OUT:
        case CURLINFO_DATA_IN:
        case CURLINFO_SSL_DATA_IN:
        default:
            break;
     }

     return 0;
}
#endif

 // FIXME - This code does not make a cURL handle that follows links and I think that's a bug!
dmrpp_easy_handle::dmrpp_easy_handle() : d_url(nullptr), d_request_headers(nullptr) {

    CURLcode res;

    d_handle = curl_easy_init();
    if (!d_handle) throw BESInternalError("Could not allocate CURL handle", __FILE__, __LINE__);

    curl::set_error_buffer(d_handle, d_errbuf);

    res =  curl_easy_setopt(d_handle, CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1_2);
    curl::eval_curl_easy_setopt_result(res, prolog, "CURLOPT_SSLVERSION", d_errbuf, __FILE__, __LINE__);


#if CURL_VERBOSE
    res = curl_easy_setopt(d_handle, CURLOPT_DEBUGFUNCTION, curl_trace);
    curl::check_setopt_result(res, prolog, "CURLOPT_DEBUGFUNCTION", d_errbuf, __FILE__, __LINE__);
   // Many tests fail with this option, but it's still useful to see how connections
   // are treated. jhrg 10/2/18
   res = curl_easy_setopt(d_handle, CURLOPT_VERBOSE, 1L);
   curl::check_setopt_result(res, prolog, "CURLOPT_VERBOSE", d_errbuf, __FILE__, __LINE__);
#endif

    res = curl_easy_setopt(d_handle, CURLOPT_HEADERFUNCTION, chunk_header_callback);
    curl::eval_curl_easy_setopt_result(res, prolog, "CURLOPT_HEADERFUNCTION", d_errbuf, __FILE__, __LINE__);

    // Pass all data to the 'write_data' function
    res = curl_easy_setopt(d_handle, CURLOPT_WRITEFUNCTION, chunk_write_data);
    curl::eval_curl_easy_setopt_result(res, prolog, "CURLOPT_WRITEFUNCTION", d_errbuf, __FILE__, __LINE__);

#ifdef CURLOPT_TCP_KEEPALIVE
    /* enable TCP keep-alive for this transfer */
    res = curl_easy_setopt(d_handle, CURLOPT_TCP_KEEPALIVE, 1L);
    curl::check_setopt_result(res, prolog, "CURLOPT_TCP_KEEPALIVE", d_errbuf, __FILE__, __LINE__);
#endif

#ifdef CURLOPT_TCP_KEEPIDLE
    /* keep-alive idle time to 120 seconds */
     res = curl_easy_setopt(d_handle, CURLOPT_TCP_KEEPIDLE, 120L);
    curl::check_setopt_result(res, prolog, "CURLOPT_TCP_KEEPIDLE", d_errbuf, __FILE__, __LINE__);
#endif

#ifdef CURLOPT_TCP_KEEPINTVL
    /* interval time between keep-alive probes: 120 seconds */
    res = curl_easy_setopt(d_handle, CURLOPT_TCP_KEEPINTVL, 120L)
    curl::check_setopt_result(res, prolog, "CURLOPT_TCP_KEEPINTVL", d_errbuf, __FILE__, __LINE__);
#endif

    d_in_use = false;
    d_chunk = 0;
}

dmrpp_easy_handle::~dmrpp_easy_handle() {
    if (d_handle) curl_easy_cleanup(d_handle);
    if (d_request_headers) curl_slist_free_all(d_request_headers);
}

/**
 * @brief This is the read_data() method for all transfers.
 *
 * This method is used by Chunk::read_data() which is used for all
 * data transfers by the dmrpp module classes (DmrppArray, D4Opaque,
 * and Common::read_atomic()). Whether a request is retired is
 * determined by curl::super_easy_perform().
 *
 * If either the super_easy_perform() (our concoction) or easy_perform()
 * throws, assume the transfer failed. The caller of this method must handle
 * all cleanup.
 */
void dmrpp_easy_handle::read_data() {
    // Treat HTTP/S requests specially; retry some kinds of failures.
    if (d_url->protocol() == HTTPS_PROTOCOL || d_url->protocol() == HTTP_PROTOCOL) {
        curl::super_easy_perform(d_handle);
    }
    else {
        CURLcode curl_code = curl_easy_perform(d_handle);
        if (CURLE_OK != curl_code) {
            string msg = prolog + "ERROR - Data transfer error: ";
            throw BESInternalError(msg.append(curl::error_message(curl_code, d_errbuf)), __FILE__, __LINE__);
        }
    }

    d_chunk->set_is_read(true);
}

#if 0
// This implmentation of the default constructor should have:
// a) Utilized the other constructor:
//        CurlHandlePool::CurlHandlePool() { CurlHandlePool(DmrppRequestHandler::d_max_transfer_threads); }
//    rather than duplicating the logic.
// b) Skipped because the only code that called it in the first place was DmrppRequestHandler::DmrppRequestHandler()
//    which is already owns DmrppRequestHandler::d_max_transfer_threads and can pass it in.
//
//
// Old default constructor. Duplicates logic.
//
CurlHandlePool::CurlHandlePool() {
    d_max_easy_handles = DmrppRequestHandler::d_max_transfer_threads;

    for (unsigned int i = 0; i < d_max_easy_handles; ++i) {
        d_easy_handles.push_back(new dmrpp_easy_handle());
    }
    unsigned int status = pthread_mutex_init(&d_get_easy_handle_mutex, 0);
    if (status != 0)
        throw BESInternalError("Could not initialize mutex in CurlHandlePool. msg: " + pthread_error(status), __FILE__, __LINE__);
}
//
// One alternate would be to do this for the default constructor:
CurlHandlePool::CurlHandlePool() {
    CurlHandlePool(8);
}
//
// - ndp 12/02/20
#endif


CurlHandlePool::CurlHandlePool(unsigned int max_handles) : d_max_easy_handles(max_handles) {
    for (unsigned int i = 0; i < d_max_easy_handles; ++i) {
        d_easy_handles.push_back(new dmrpp_easy_handle());
    }

    unsigned int status = pthread_mutex_init(&d_get_easy_handle_mutex, 0);
    if (status != 0)
        throw BESInternalError("Could not initialize mutex in CurlHandlePool. msg: " + pthread_error(status), __FILE__, __LINE__);
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
#if 0
static struct curl_slist *append_http_header(curl_slist *slist, const string &header, const string &value) {
    string full_header = header;
    full_header.append(" ").append(value);

    struct curl_slist *temp = curl_slist_append(slist, full_header.c_str());
    return temp;
}
#endif


/**
 * Get a CURL easy handle to transfer data from \arg url into the given \arg chunk.
 *
 * @note This method and release_handle() use the same lock to prevent the handle's
 * chunk pointer from being cleared by another thread after a thread running this
 * method has set it. However, there's no protection against calling this when no
 * more handles are available. If that happens a thread calling release_handle()
 * will block until this code returns (and this code will return NULL).
 *
 * @param chunk Use this Chunk to set a libcurl easy handle so that it
 * will fetch the Chunk's data.
 * @return A CURL easy handle configured to transfer data, or null if
 * there are no more handles in the pool.
 */
dmrpp_easy_handle *
CurlHandlePool::get_easy_handle(Chunk *chunk) {
    // Here we check to make sure that the we are only going to
    // access an approved location with this easy_handle
    if (!http::AllowedHosts::theHosts()->is_allowed(chunk->get_data_url())) {
        string msg = "ERROR!! The chunk url " + chunk->get_data_url()->str() + " does not match any of the AllowedHost rules. ";
        throw BESForbiddenError(msg, __FILE__, __LINE__);
    }

    Lock lock(d_get_easy_handle_mutex); // RAII

    dmrpp_easy_handle *handle = 0;
    for (auto i = d_easy_handles.begin(), e = d_easy_handles.end(); i != e; ++i) {
        if (!(*i)->d_in_use) {
            handle = *i;
            break;
        }
    }

    if (handle) {
        // Once here, d_easy_handle holds a CURL* we can use.
        handle->d_in_use = true;
        handle->d_url = chunk->get_data_url();

        handle->d_chunk = chunk;

        CURLcode res = curl_easy_setopt(handle->d_handle, CURLOPT_URL, chunk->get_data_url()->str().c_str());
        curl::eval_curl_easy_setopt_result(res, prolog, "CURLOPT_URL", handle->d_errbuf, __FILE__, __LINE__);

        // get the offset to offset + size bytes
        res = curl_easy_setopt(handle->d_handle, CURLOPT_RANGE, chunk->get_curl_range_arg_string().c_str());
        curl::eval_curl_easy_setopt_result(res, prolog, "CURLOPT_RANGE", handle->d_errbuf, __FILE__, __LINE__);

        // Pass this to chunk_header_callback as the fourth argument
        res = curl_easy_setopt(handle->d_handle, CURLOPT_HEADERDATA, reinterpret_cast<void *>(chunk));
        curl::eval_curl_easy_setopt_result(res, prolog, "CURLOPT_HEADERDATA", handle->d_errbuf, __FILE__, __LINE__);

        // Pass this to chunk_write_data as the fourth argument
        res = curl_easy_setopt(handle->d_handle, CURLOPT_WRITEDATA, reinterpret_cast<void *>(chunk));
        curl::eval_curl_easy_setopt_result(res, prolog, "CURLOPT_WRITEDATA", handle->d_errbuf, __FILE__, __LINE__);

        // store the easy_handle so that we can call release_handle in multi_handle::read_data()
        res = curl_easy_setopt(handle->d_handle, CURLOPT_PRIVATE, reinterpret_cast<void *>(handle));
        curl::eval_curl_easy_setopt_result(res, prolog, "CURLOPT_PRIVATE", handle->d_errbuf, __FILE__, __LINE__);

        // Enabled cookies
        res = curl_easy_setopt(handle->d_handle, CURLOPT_COOKIEFILE, curl::get_cookie_filename().c_str());
        curl::eval_curl_easy_setopt_result(res, prolog, "CURLOPT_COOKIEFILE", handle->d_errbuf, __FILE__, __LINE__);

        res = curl_easy_setopt(handle->d_handle, CURLOPT_COOKIEJAR, curl::get_cookie_filename().c_str());
        curl::eval_curl_easy_setopt_result(res, prolog, "CURLOPT_COOKIEJAR", handle->d_errbuf, __FILE__, __LINE__);

        // Follow 302 (redirect) responses
        res = curl_easy_setopt(handle->d_handle, CURLOPT_FOLLOWLOCATION, 1);
        curl::eval_curl_easy_setopt_result(res, prolog, "CURLOPT_FOLLOWLOCATION", handle->d_errbuf, __FILE__, __LINE__);

        res = curl_easy_setopt(handle->d_handle, CURLOPT_MAXREDIRS, curl::max_redirects());
        curl::eval_curl_easy_setopt_result(res, prolog, "CURLOPT_MAXREDIRS", handle->d_errbuf, __FILE__, __LINE__);

        // Set the user agent something otherwise TEA will never redirect to URS.
        res = curl_easy_setopt(handle->d_handle, CURLOPT_USERAGENT, curl::hyrax_user_agent().c_str());
        curl::eval_curl_easy_setopt_result(res, prolog, "CURLOPT_USERAGENT", handle->d_errbuf, __FILE__, __LINE__);

        // This means libcurl will use Basic, Digest, GSS Negotiate, or NTLM,
        // choosing the the 'safest' one supported by the server.
        // This requires curl 7.10.6 which is still in pre-release. 07/25/03 jhrg
        res = curl_easy_setopt(handle->d_handle, CURLOPT_HTTPAUTH, (long) CURLAUTH_ANY);
        curl::eval_curl_easy_setopt_result(res, prolog, "CURLOPT_HTTPAUTH", handle->d_errbuf, __FILE__, __LINE__);

        // Enable using the .netrc credentials file.
        res = curl_easy_setopt(handle->d_handle, CURLOPT_NETRC, CURL_NETRC_OPTIONAL);
        curl::eval_curl_easy_setopt_result(res, prolog, "CURLOPT_NETRC", handle->d_errbuf, __FILE__, __LINE__);

        // If the configuration specifies a particular .netrc credentials file, use it.
        // TODO move this operation into constructor and stash the value.
        string netrc_file = curl::get_netrc_filename();
        if (!netrc_file.empty()) {
            res = curl_easy_setopt(handle->d_handle, CURLOPT_NETRC_FILE, netrc_file.c_str());
            curl::eval_curl_easy_setopt_result(res, prolog, "CURLOPT_NETRC_FILE", handle->d_errbuf, __FILE__, __LINE__);
        }
        //VERBOSE(__FILE__ << "::get_easy_handle() is using the netrc file '"
        //<< ((!netrc_file.empty()) ? netrc_file : "~/.netrc") << "'" << endl);

        AccessCredentials *credentials = CredentialsManager::theCM()->get(handle->d_url);
        if (credentials && credentials->is_s3_cred()) {
            BESDEBUG(DMRPP_CURL,
                     prolog << "Got AccessCredentials instance: " << endl << credentials->to_json() << endl);
            // If there are available credentials, and they are S3 credentials then we need to sign
            // the request
            const std::time_t request_time = std::time(0);

            const std::string auth_header =
                    AWSV4::compute_awsv4_signature(
                            handle->d_url,
                            request_time,
                            credentials->get(AccessCredentials::ID_KEY),
                            credentials->get(AccessCredentials::KEY_KEY),
                            credentials->get(AccessCredentials::REGION_KEY),
                            "s3");


            handle->d_request_headers = curl::append_http_header((curl_slist *)0, "Authorization", auth_header);
            handle->d_request_headers = curl::append_http_header(handle->d_request_headers, "x-amz-content-sha256",
                                                  "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
            handle->d_request_headers = curl::append_http_header(handle->d_request_headers, "x-amz-date", AWSV4::ISO8601_date(request_time));
#if 0

            // passing nullptr for the first call allocates the curl_slist
            // The following code builds the slist that holds the headers. This slist is freed
            // once the URL is dereferenced in dmrpp_easy_handle::read_data(). jhrg 11/26/19
            handle->d_request_headers = append_http_header(0, "Authorization:", auth_header);
            if (!handle->d_request_headers)
                throw BESInternalError(
                        string("CURL Error setting Authorization header: ").append(
                                curl::error_message(res, handle->d_errbuf)), __FILE__, __LINE__);

            // We pre-compute the sha256 hash of a null message body
            curl_slist *temp = append_http_header(handle->d_request_headers, "x-amz-content-sha256:",
                                                  "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
            if (!temp)
                throw BESInternalError(
                        string("CURL Error setting x-amz-content-sha256: ").append(
                                curl::error_message(res, handle->d_errbuf)),
                        __FILE__, __LINE__);
            handle->d_request_headers = temp;

            temp = append_http_header(handle->d_request_headers, "x-amz-date:", AWSV4::ISO8601_date(request_time));
            if (!temp)
                throw BESInternalError(
                        string("CURL Error setting x-amz-date header: ").append(
                                curl::error_message(res, handle->d_errbuf)),
                        __FILE__, __LINE__);
            handle->d_request_headers = temp;
#endif

            // handle->d_request_headers = curl::add_edl_auth_headers(handle->d_request_headers);

            res = curl_easy_setopt(handle->d_handle, CURLOPT_HTTPHEADER, handle->d_request_headers);
            curl::eval_curl_easy_setopt_result(res, prolog, "CURLOPT_HTTPHEADER", handle->d_errbuf, __FILE__, __LINE__);
        }
    }

    return handle;
}

/**
 * Release a DMR++ easy_handle. This returns the handle to the pool of handles
 * that can be used for serial transfers or, with multi curl, for parallel transfers.
 *
 * @param handle
 */
void CurlHandlePool::release_handle(dmrpp_easy_handle *handle) {
    // In get_easy_handle, it's possible that d_in_use could be false and d_chunk
    // could not be set to 0 (because a separate thread could be running these
    // methods). In that case, the thread running get_easy_handle could set d_chunk,
    // and then this thread could clear it (... unlikely, but an optimizing compiler is
    // free to reorder statements so long as they don't alter the function's behavior).
    // Timing tests indicate this lock does not cost anything that can be measured.
    // jhrg 8/21/18
    Lock lock(d_get_easy_handle_mutex);

    // TODO Add a call to curl reset() here. jhrg 9/23/20

#if KEEP_ALIVE
    handle->d_url = nullptr;
    handle->d_chunk = 0;
    handle->d_in_use = false;
#else
    // This is to test the effect of libcurl Keep Alive support
    // Find the handle; erase from the vector; delete; allocate a new handle and push it back on
    for (std::vector<dmrpp_easy_handle *>::iterator i = d_easy_handles.begin(), e = d_easy_handles.end(); i != e; ++i) {
        if (*i == handle) {
            BESDEBUG("dmrpp:5", "Found a handle match for the " << i - d_easy_handles.begin() << "th easy handle." << endl);
            delete handle;
            *i = new dmrpp_easy_handle();
            break;
        }
    }
#endif
}

/**
 * @brief Release the handle associated with a given chunk
 * This is intended for use in error clean up code.
 * @param chunk Find the handle for this chunk and release it.
 */
void CurlHandlePool::release_handle(Chunk *chunk) {
    for (std::vector<dmrpp_easy_handle *>::iterator i = d_easy_handles.begin(), e = d_easy_handles.end(); i != e; ++i) {
        if ((*i)->d_chunk == chunk) {
            release_handle(*i);
            break;
        }
    }
}

/**
 * @breif release all outstanding curl handles
 * If one access in a multi-transfer fails because of an error such as
 * Access Denied, end the entire process and free all curl handles. This
 * is different from an Internal Server Error response, which should be
 * retried without ending the other accesses.
 */
void CurlHandlePool::release_all_handles() {
    for (std::vector<dmrpp_easy_handle *>::iterator i = d_easy_handles.begin(), e = d_easy_handles.end(); i != e; ++i) {
        release_handle(*i);
    }
}
