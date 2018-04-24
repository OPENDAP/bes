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
#include <map>

#include <curl/curl.h>

namespace dmrpp {

/**
 * Maintain a pool of CURL handles. Reusing the CURL handles enables the
 * libcurl connection pooling scheme.
 */
class CurlHandlePool {

    /**
     * Bundle an easy handle and an 'in use' flag.
     */
    struct easy_handle {
        bool d_in_use;
        CURL *d_easy_handle;
        char d_error_buf[CURL_ERROR_SIZE];

        easy_handle() {
            d_easy_handle = curl_easy_init();
            d_in_use = false;
        }

        ~easy_handle() {
            curl_easy_cleanup(d_easy_handle);
        }
    };

    typedef std::multimap<std::string, easy_handle*>::iterator eh_iter;

    std::multimap<std::string, easy_handle*> d_easy_handles;

public:
    CurlHandlePool() { }

    ~CurlHandlePool() {
        // delete all of the easy_handle things FIXME
        for (eh_iter i = d_easy_handles.begin(), e = d_easy_handles.end(); i != e; ++i) {
            delete (*i).second; // calls curl_easy_cleanup()
        }
    }

    CURL *get_easy_handle(const std::string &url);

    void release_handle(const std::string &url, CURL *h);
};

} // namespace dmrpp

#endif // _HandlePool_h
