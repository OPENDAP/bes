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

#include "BESInternalError.h"

#include "CurlHandlePool.h"

using namespace dmrpp;

CURL *
CurlHandlePool::get_easy_handle(const std::string &url)
{
    // look in the map for the 'url.' If found, use it, else make a new handle,
    // add it to the map and return it.
    eh_iter i = d_easy_handles.find(url);
    if (i != d_easy_handles.end()) {
        return (*i).second->d_easy_handle;
    }

    CurlHandlePool::easy_handle *handle = new CurlHandlePool::easy_handle();
    if (!handle->d_easy_handle) {
        throw BESInternalError("Could not allocate CURL handle", __FILE__, __LINE__);
    }

    handle->d_in_use = true;

    // Set the URL and other parameters that are invariant for the same URL.

    CURLcode res = curl_easy_setopt(handle->d_easy_handle, CURLOPT_URL, url.c_str());
    if (res != CURLE_OK) throw BESInternalError(string(curl_easy_strerror(res)), __FILE__, __LINE__);

    res = curl_easy_setopt(handle->d_easy_handle, CURLOPT_ERRORBUFFER, handle.d_error_buf);
    if (res != CURLE_OK) throw BESInternalError(string(curl_easy_strerror(res)), __FILE__, __LINE__);

#if 0
    // get the offset to offset + size bytes
    if (CURLE_OK != curl_easy_setopt(curl, CURLOPT_RANGE, range.c_str() /*"0-199"*/))
    throw BESError(string("HTTP Error: ").append(buf), BES_INTERNAL_ERROR, __FILE__, __LINE__);
#endif

    // Pass all data to the 'write_data' function
    if (CURLE_OK != curl_easy_setopt(handle->d_easy_handle, CURLOPT_WRITEFUNCTION, chunk_write_data))
        throw BESInternalError(string("HTTP Error: ").append(buf), __FILE__, __LINE__);

    // Pass this to write_data as the fourth argument
    if (CURLE_OK != curl_easy_setopt(handle->d_easy_handle, CURLOPT_WRITEDATA, reinterpret_cast<void*>(chunk)))
        throw BESInternalError(string("HTTP Error: ").append(buf), __FILE__, __LINE__);

    d_easy_handles.insert(make_pair<string, CurlHandlePool::easy_handle*>(url, handle));

    return handle->d_easy_handle;
}


void CurlHandlePool::release_handle(const std::string &url, CURL *h)
{
    // FIXME
}

