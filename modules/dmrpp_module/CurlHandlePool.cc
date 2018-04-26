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

#include <curl/curl.h>

#include "BESInternalError.h"

#include "CurlHandlePool.h"
#include "Chunk.h"

using namespace dmrpp;

#if 0
CurlHandlePool *CurlHandlePool::d_instance = 0;
#endif


#if 0
/**
 * @return Get a pointer to the curl handle pool
 */
CurlHandlePool *CurlHandlePool::get_curl_handle_pool()
{
    // TODO Make this thread safe. jhrg 4/24/18
    if (!d_instance)
    d_instance = new CurlHandlePool;

    return d_instance;
}
#endif

/**
 * Get a CURL easy handle to transfer data from \arg url into the given \arg chunk.
 *
 * @param url
 * @param chunk
 * @return A CURL easy handle configured to transfer data.
 */
CURL *
CurlHandlePool::get_easy_handle(Chunk *chunk)
{
    // Get the next available CurlHandlePool::easy_handle
    if (!d_easy_handle) {
        d_easy_handle = new CurlHandlePool::easy_handle();
    }
    else {
        if (d_easy_handle->d_in_use) throw BESInternalError("CURL handle in use", __FILE__, __LINE__);
    }

    // Once here, d_easy_handle holds a CURL* we can use.

    d_easy_handle->d_in_use = true;

    CURLcode res = curl_easy_setopt(d_easy_handle->d_handle, CURLOPT_URL, chunk->get_data_url().c_str());
    if (res != CURLE_OK) throw BESInternalError(string(curl_easy_strerror(res)), __FILE__, __LINE__);

    // get the offset to offset + size bytes
    if (CURLE_OK != curl_easy_setopt(d_easy_handle->d_handle, CURLOPT_RANGE, chunk->get_curl_range_arg_string().c_str()))
        throw BESInternalError(string("HTTP Error: ").append(d_easy_handle->d_error_buf), __FILE__, __LINE__);

    // Pass this to write_data as the fourth argument
    if (CURLE_OK != curl_easy_setopt(d_easy_handle->d_handle, CURLOPT_WRITEDATA, reinterpret_cast<void*>(chunk)))
        throw BESInternalError(string("CURL Error: ").append(d_easy_handle->d_error_buf), __FILE__, __LINE__);

    return d_easy_handle->d_handle;
}

void CurlHandlePool::release_handle(CURL *)
{
    d_easy_handle->d_in_use = false;
}

