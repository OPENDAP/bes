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
#if 0
#include <map>
#endif

#include <curl/curl.h>

#include "BESInternalError.h"

#include "DmrppUtil.h"

namespace dmrpp {

class Chunk;

/**
 * Wrapper for a single libcurl 'easy' handle. Expand to multiple handles later.
 *
 * @note This is a singleton
 */
class CurlHandlePool {

    /**
     * Bundle an easy handle and an 'in use' flag.
     */
    struct easy_handle {
        bool d_in_use;
        CURL *d_handle;
#if 0
        char d_error_buf[CURL_ERROR_SIZE];
#endif

        easy_handle()
        {
            d_handle = curl_easy_init();
            if (!d_handle) throw BESInternalError("Could not allocate CURL handle", __FILE__, __LINE__);

            CURLcode res;
#if 0
            CURLcode res = curl_easy_setopt(d_handle, CURLOPT_ERRORBUFFER, d_error_buf);
            if (res != CURLE_OK) throw BESInternalError(string(curl_easy_strerror(res)), __FILE__, __LINE__);
#endif
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

#if 0
    typedef std::multimap<std::string, easy_handle*>::iterator eh_iter;

    std::multimap<std::string, easy_handle*> d_easy_handles;
#endif

    unsigned int d_num_easy_handles;

    easy_handle *d_easy_handle;

public:

    CurlHandlePool() : d_easy_handle(0) { }

    ~CurlHandlePool() {
#if 0
        // delete all of the easy_handle things FIXME
        for (eh_iter i = d_easy_handles.begin(), e = d_easy_handles.end(); i != e; ++i) {
            delete (*i).second; // calls curl_easy_cleanup()
        }
#endif
        delete d_easy_handle;
    }

    CURL *get_easy_handle(Chunk *chunk);

    void release_handle(CURL *h);
};

} // namespace dmrpp

#endif // _HandlePool_h
