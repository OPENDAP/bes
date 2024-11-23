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

#ifndef _HandlePool_h
#define _HandlePool_h 1

#include <memory>
#include <string>
#include <vector>
#include <mutex>

#include <curl/curl.h>

#include "url_impl.h"

namespace dmrpp {

class Chunk;

/**
 * @brief Bundle a libcurl easy handle with other information.
 *
 * Provide an object that encapsulates a libcurl easy handle, a URL and
 * a DMR++ handler 'chunk.' This can be used with the libcurl 'easy' API
 * for serial data access or parallel (round robin) data transfers.
 */
class dmrpp_easy_handle {
    bool d_in_use = false;      ///< Is this easy_handle in use?
    std::shared_ptr<http::url> d_url;  ///< The libcurl handle reads from this URL.
    Chunk *d_chunk = nullptr;     ///< This easy_handle reads the data for \arg chunk.
#if 0
    //char d_errbuf[CURL_ERROR_SIZE]; ///< raw error message info from libcurl
#endif
    std::vector<char> d_errbuf = std::vector<char>(CURL_ERROR_SIZE, '\0'); ///< raw error message info from libcurl

    CURL *d_handle = nullptr;     ///< The libcurl handle object.
    curl_slist *d_request_headers = nullptr; ///< Holds the list of authorization headers, if needed.

    friend class CurlHandlePool;

public:
    dmrpp_easy_handle();

    ~dmrpp_easy_handle();

    void read_data();
};

/**
 * Get a CURL easy handle, assign a URL and other values, use the handler, return
 * it to the pool. This class helps take advantage of libcurl's built-in reuse
 * capabilities (connection keep-alive, DNS pooling, etc.).
 *
 * @note It may be that TCP Keep Alive is not supported in libcurl versions
 * prior to 7.25, which means CentOS 6 will not have support for this.
 *
 * See https://ec.haxx.se/libcurl-connectionreuse.html for more information.
 *
 * See d_max_easy_handles below for the limit on the total number of easy handles.
 */
class CurlHandlePool {
private:
    std::string d_cookies_filename;
    std::string d_hyrax_user_agent;
    unsigned long d_max_redirects = 3;
    std::string d_netrc_file;

    CURLSH *d_share = nullptr;

    static std::recursive_mutex d_share_mutex;
    
    static std::recursive_mutex d_cookie_mutex;
    static std::recursive_mutex d_dns_mutex;
    static std::recursive_mutex d_ssl_session_mutex;
    static std::recursive_mutex d_connect_mutex;

    static std::recursive_mutex d_mutex;    // Use this for the default case.

    static void lock_cb(CURL */*handle*/, curl_lock_data data, curl_lock_access /*access*/, void */*userptr*/) {
        switch (data) {
            case CURL_LOCK_DATA_SHARE: // CURL_LOCK_DATA_SHARE is used internally
                d_share_mutex.lock();
                break;

            case CURL_LOCK_DATA_COOKIE:
                d_cookie_mutex.lock();
                break;

            case CURL_LOCK_DATA_DNS:
                d_dns_mutex.lock();
                break;
            case CURL_LOCK_DATA_SSL_SESSION:
                d_ssl_session_mutex.lock();
                break;
            case CURL_LOCK_DATA_CONNECT:
                d_connect_mutex.lock();
                break;
            default:
                d_mutex.lock();
                break;
        }
    }

    static void unlock_cb(CURL */*handle*/, curl_lock_data data, void * /*userptr*/) {
        switch (data) {
            case CURL_LOCK_DATA_SHARE: // CURL_LOCK_DATA_SHARE is used internally
                d_share_mutex.unlock();
                break;

            case CURL_LOCK_DATA_COOKIE:
                d_cookie_mutex.unlock();
                break;

            case CURL_LOCK_DATA_DNS:
                d_dns_mutex.unlock();
                break;
            case CURL_LOCK_DATA_SSL_SESSION:
                d_ssl_session_mutex.unlock();
                break;
            case CURL_LOCK_DATA_CONNECT:
                d_connect_mutex.unlock();
                break;
            default:
                d_mutex.unlock();
                break;
        }
    }

public:
    CurlHandlePool() = default;
    ~CurlHandlePool();

    void initialize();
    dmrpp_easy_handle *get_easy_handle(Chunk *chunk);

    static void release_handle(dmrpp_easy_handle *h);
};

} // namespace dmrpp

#endif // _HandlePool_h
