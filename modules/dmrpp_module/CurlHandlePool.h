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

#include <pthread.h>

#include <curl/curl.h>

#include "url_impl.h"

namespace dmrpp {

class Chunk;

/**
 * RAII. Lock access to the get_easy_handle() and release_handle() methods.
 */
class Lock {
private:
    pthread_mutex_t &m_mutex;

    Lock();

    Lock(const Lock &rhs);

public:
    Lock(pthread_mutex_t &lock);

    virtual ~Lock();
};

/**
 * @brief Bundle a libcurl easy handle with other information.
 *
 * Provide an object that encapsulates a libcurl easy handle, a URL and
 * a DMR++ handler 'chunk.' This can be used with the libcurl 'easy' API
 * for serial data access or parallel (round robin) data transfers.
 */
class dmrpp_easy_handle {
    bool d_in_use;      ///< Is this easy_handle in use?
    std::shared_ptr<http::url> d_url;  ///< The libcurl handle reads from this URL.
    Chunk *d_chunk;     ///< This easy_handle reads the data for \arg chunk.
    char d_errbuf[CURL_ERROR_SIZE]; ///< raw error message info from libcurl
    CURL *d_handle;     ///< The libcurl handle object.
    curl_slist *d_request_headers; ///< Holds the list of authorization headers, if needed.

    friend class CurlHandlePool;

public:
    dmrpp_easy_handle();

    ~dmrpp_easy_handle();

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
    pthread_mutex_t d_get_easy_handle_mutex;

    friend class Lock;
    CurlHandlePool();

public:

    explicit CurlHandlePool(unsigned int max_handles);

    ~CurlHandlePool()
    {
        for (auto i = d_easy_handles.begin(), e = d_easy_handles.end(); i != e; ++i) {
            delete *i;
        }
    }

    /// @brief Get the number of handles in the pool.
    unsigned int get_max_handles() const
    { return d_max_easy_handles; }

    unsigned int get_handles_available() const
    {
        unsigned int n = 0;
        for (auto i = d_easy_handles.begin(), e = d_easy_handles.end(); i != e; ++i) {
            if (!(*i)->d_in_use) {
                n++;

            }
        }
        return n;
    }

    dmrpp_easy_handle *get_easy_handle(Chunk *chunk);

    void release_handle(dmrpp_easy_handle *h);

    void release_handle(Chunk *chunk);

    void release_all_handles();
};

/**
 * How a collection of dmrpp_easy_handles that are being used together on
 * a single logical transfer. By definition, if one of these fails, they all
 * fail, are stopped and the easy handles reset and returned to the pool.
 * This class is used to portect leaking handles when one thread of a
 * parallel transfer fails and an exception is thrown taking the flow of
 * control out of the handler to the command processor loop.
 */
class SwimLane {
    CurlHandlePool &d_pool;
    std::vector<dmrpp_easy_handle *> d_handles;
public:
    SwimLane(CurlHandlePool &pool) : d_pool(pool)
    {}

    SwimLane(CurlHandlePool &pool, dmrpp_easy_handle *h) : d_pool(pool)
    {
        d_handles.push_back(h);
    }

    virtual ~SwimLane()
    {
        for (auto i = d_handles.begin(), e = d_handles.end(); i != e; ++i) {
            d_pool.release_handle(*i);
        }
    }

    void add_handle(dmrpp_easy_handle *h)
    {
        d_handles.push_back(h);
    }
};

} // namespace dmrpp

#endif // _HandlePool_h
