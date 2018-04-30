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
#if 0
#include <map>
#endif

#include <curl/curl.h>

#include "BESInternalError.h"

#include "DmrppUtil.h"

namespace dmrpp {

class Chunk;

/**
 * Get a CURL easy handle, assign a URL and other values, use the handler, return
 * it to the pool. This class helps take advantage of libculr's built-in reuse
 * capabilities (connection keep-alive, DNS pooling, etc.).
 *
 * See d_max_handles below for the limit on the total number of easy handles.
 */
class CurlHandlePool {

    const static unsigned int d_max_handles = 5;

    /**
     * Bundle an easy handle and an 'in use' flag.
     */
    struct easy_handle {
        bool d_in_use;
        CURL *d_handle;

        easy_handle()
        {
            d_handle = curl_easy_init();
            if (!d_handle) throw BESInternalError("Could not allocate CURL handle", __FILE__, __LINE__);

            CURLcode res;

            // Pass all data to the 'write_data' function
            if (CURLE_OK != (res = curl_easy_setopt(d_handle, CURLOPT_WRITEFUNCTION, chunk_write_data)))
                throw BESInternalError(string("CURL Error: ").append(curl_easy_strerror(res)), __FILE__, __LINE__);

            /* enable TCP keep-alive for this transfer */
            if (CURLE_OK != (res = curl_easy_setopt(d_handle, CURLOPT_TCP_KEEPALIVE, 1L)))
                throw string("CURL Error: ").append(curl_easy_strerror(res));

            /* keep-alive idle time to 120 seconds */
            if (CURLE_OK != (res = curl_easy_setopt(d_handle, CURLOPT_TCP_KEEPIDLE, 120L)))
                throw string("CURL Error: ").append(curl_easy_strerror(res));

            /* interval time between keep-alive probes: 60 seconds */
            if (CURLE_OK != (res = curl_easy_setopt(d_handle, CURLOPT_TCP_KEEPINTVL, 60L)))
                throw string("CURL Error: ").append(curl_easy_strerror(res));

            d_in_use = false;
        }

        ~easy_handle()
        {
            curl_easy_cleanup(d_handle);
        }
    };

    std::vector<easy_handle*> d_available;
    std::vector<easy_handle*> d_in_use;

    easy_handle *d_easy_handle;

public:

    CurlHandlePool() : d_easy_handle(0)
    {
#if 0
        for (int i = 0; i < d_max_handles; ++i) {
            d_available.push_back(new CurlHandlePool::easy_handle());
        }
#endif

    }

    ~CurlHandlePool() {
        delete d_easy_handle;
    }

    CURL *get_easy_handle(Chunk *chunk);

    void release_handle(CURL *h);
};

} // namespace dmrpp

#endif // _HandlePool_h
