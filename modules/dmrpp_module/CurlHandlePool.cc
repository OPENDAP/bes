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
#include <sstream>
#include <ctime>

#include <curl/curl.h>

#include "CurlUtils.h"
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

#define CURL_VERBOSE 0  // Logs curl info to the bes.log

#define prolog std::string("CurlHandlePool::").append(__func__).append("() - ")

using namespace dmrpp;
using namespace http;
using namespace std;

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
    d_chunk = nullptr;
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

CurlHandlePool::CurlHandlePool(unsigned int max_handles) : d_max_easy_handles(max_handles) {
    for (unsigned int i = 0; i < d_max_easy_handles; ++i) {
        d_easy_handles.push_back(new dmrpp_easy_handle());
    }
}

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
    // Here we check to make sure that we are only going to
    // access an approved location with this easy_handle
    // TODO I don't think this belongs here. jhrg 5/13/22
    string reason = "The requested resource does not match any of the AllowedHost rules.";
    if (!http::AllowedHosts::theHosts()->is_allowed(chunk->get_data_url(), reason)) {
        stringstream ss;
        ss << "ERROR! The chunk url "<< chunk->get_data_url()->str() << " was rejected because: " << reason;
        throw BESForbiddenError(ss.str(), __FILE__, __LINE__);
    }

    std::lock_guard<std::recursive_mutex> lock_me(d_get_easy_handle_mutex);

    dmrpp_easy_handle *handle = nullptr;
    for (auto & d_easy_handle : d_easy_handles) {
        if (!d_easy_handle->d_in_use) {
            handle = d_easy_handle;
            break;
        }
    }

    if (handle) {
        try {
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
            curl::eval_curl_easy_setopt_result(res, prolog, "CURLOPT_FOLLOWLOCATION", handle->d_errbuf, __FILE__,
                                               __LINE__);

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
                curl::eval_curl_easy_setopt_result(res, prolog, "CURLOPT_NETRC_FILE", handle->d_errbuf, __FILE__,
                                                   __LINE__);
            }

            // TODO Code between here and below may have been turned into a method in AccessCredentials. jhrg 11/2/22
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


                handle->d_request_headers = curl::append_http_header((curl_slist *) 0, "Authorization", auth_header);
                handle->d_request_headers = curl::append_http_header(handle->d_request_headers, "x-amz-content-sha256",
                                                                     "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
                handle->d_request_headers = curl::append_http_header(handle->d_request_headers, "x-amz-date",
                                                                     AWSV4::ISO8601_date(request_time));
                // TODO here. jhrg 11/2/22
                res = curl_easy_setopt(handle->d_handle, CURLOPT_HTTPHEADER, handle->d_request_headers);
                curl::eval_curl_easy_setopt_result(res, prolog, "CURLOPT_HTTPHEADER", handle->d_errbuf, __FILE__,
                                                   __LINE__);
            }
        }
        catch (...){
            release_handle(handle,true);
            return nullptr;
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
void CurlHandlePool::release_handle(dmrpp_easy_handle *handle, bool replace) {
    // In get_easy_handle, it's possible that d_in_use could be false and d_chunk
    // could not be set to 0 (because a separate thread could be running these
    // methods). In that case, the thread running get_easy_handle could set d_chunk,
    // and then this thread could clear it (... unlikely, but an optimizing compiler is
    // free to reorder statements so long as they don't alter the function's behavior).
    // Timing tests indicate this lock does not cost anything that can be measured.
    // jhrg 8/21/18
    std::lock_guard<std::recursive_mutex> lock_me(d_get_easy_handle_mutex);

    // TODO Add a call to curl reset() here. jhrg 9/23/20
    //   I stuck it in the "replace" block below. ndp 08/07/23

    if(replace) {
        int i = 0;
        for (auto & d_easy_handle : d_easy_handles) {
            if (d_easy_handle == handle) {
                BESDEBUG("dmrpp:5", "Found a handle match for the " << i << "the easy handle." << endl);
                curl_easy_reset(handle->d_handle);
                delete handle;
                d_easy_handle = new dmrpp_easy_handle();
                break;
            }
            i++;
        }
    }
    else {
        handle->d_url = nullptr;
        handle->d_chunk = nullptr;
        handle->d_in_use = false;
    }
}

/**
 * @brief Release the handle associated with a given chunk
 * This is intended for use in error clean up code.
 * @param chunk Find the handle for this chunk and release it.
 */
void CurlHandlePool::release_handle(const Chunk *chunk) {
    for (auto & d_easy_handle : d_easy_handles) {
        if (d_easy_handle->d_chunk == chunk) {
            release_handle(d_easy_handle);
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
    for (auto & d_easy_handle : d_easy_handles) {
        release_handle(d_easy_handle);
    }
}
