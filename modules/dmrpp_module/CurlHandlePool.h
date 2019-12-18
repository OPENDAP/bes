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

#include <string>
#include <vector>

#include <pthread.h>

#include <curl/curl.h>

namespace dmrpp {

class Chunk;

/**
 * RAII. Lock access to the get_easy_handle() and release_handle() methods.
 */
class Lock {
private:
    pthread_mutex_t& m_mutex;

    Lock();
    Lock(const Lock &rhs);

public:
    Lock(pthread_mutex_t &lock);
    virtual ~Lock();
};

/**
 * @brief Bundle a libcurl easy handle to other information.
 *
 * Provide an object that encapsulates a libcurl easy handle, a URL and
 * a DMR++ handler 'chunk.' This can be used with the libcurl 'easy' API
 * for serial data access or with the 'multi' API and a libcurl multi
 * handle for parallel (round robin) data transfers.
 */
class dmrpp_easy_handle {
    bool d_in_use;      ///< Is this easy_handle in use?
    std::string d_url;  ///< The libcurl handle reads from this URL.
    Chunk *d_chunk;     ///< This easy_handle reads the data for \arg chunk.
    char d_errbuf[CURL_ERROR_SIZE]; ///< raw error message info from libcurl
    CURL *d_handle;     ///< The libcurl handle object.
    struct curl_slist *d_headers; ///< Holds the list of authorization headers, if needed.

    friend class CurlHandlePool;
    friend class dmrpp_multi_handle;

public:
    dmrpp_easy_handle();
    ~dmrpp_easy_handle();

    void read_data();
};


/**
 * @brief Encapsulate a libcurl multi handle.
 */
class dmrpp_multi_handle {
    // This struct can be a vector<dmrpp_easy_handle*> or a CURLM *, depending
    // on whether the curl lib support the Multi API. ...commonly known as the
    // 'pointer to an implementation' pattern which has the unfortunate acronym
    // 'pimpl.' jhrg 8/27/18
    struct multi_handle;

    multi_handle *p_impl;

public:
    dmrpp_multi_handle();

    ~dmrpp_multi_handle();

    void add_easy_handle(dmrpp_easy_handle *eh);

    void read_data();
};

/**
 * Get a CURL easy handle, assign a URL and other values, use the handler, return
 * it to the pool. This class helps take advantage of libculr's built-in reuse
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
    unsigned int d_max_easy_handles;

    std::vector<dmrpp_easy_handle *> d_easy_handles;

    dmrpp_multi_handle *d_multi_handle;

    pthread_mutex_t d_get_easy_handle_mutex;

    friend class Lock;

public:
    CurlHandlePool();

    ~CurlHandlePool()
    {
        for (auto d_easy_handle : d_easy_handles) {
            delete d_easy_handle;
        }

        delete d_multi_handle;
    }

    unsigned int get_max_handles() const
    {
        return d_max_easy_handles;
    }

    dmrpp_multi_handle *get_multi_handle()
    {
        return d_multi_handle;
    }

    dmrpp_easy_handle *get_easy_handle(Chunk *chunk);

    void release_handle(dmrpp_easy_handle *h);
};

} // namespace dmrpp

#endif // _HandlePool_h
