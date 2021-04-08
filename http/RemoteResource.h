// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of the BES http package, part of the Hyrax data server.

// Copyright (c) 2020 OPeNDAP, Inc.
// Author: Nathan Potter <ndp@opendap.org>
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

// Authors:
//      ndp       Nathan Potter <ndp@opendap.org>

#ifndef  _bes_http_REMOTE_HTTP_RESOURCE_H_
#define  _bes_http_REMOTE_HTTP_RESOURCE_H_ 1

#include <curl/curl.h>
#include <curl/easy.h>

#include <memory>
#include <string>
#include <vector>

#if 0
#include "InternalErr.h"
#include "RCReader.h"
#endif

#include "url_impl.h"
#include "RemoteResource.h"
#include "rapidjson/document.h"
#include "HttpCache.h"

namespace http {

/**
 * This class encapsulates a remote resource available via HTTP GET. It will
 * retrieve the content of the resource and place it in a local disk cache
 * for rapid (subsequent) access. It can be configure to use a proxy server
 * for the outgoing requests.
 */
    class RemoteResource {
    private:
        friend class RemoteResourceTest;
        /// Resource URL that an instance of this class represents
        // std::string d_remoteResourceUrl;
        std::shared_ptr<http::url> d_remoteResourceUrl;

        /// Open file descriptor for the resource content (Returned from the cache).
        int d_fd;

        /// Protect the state of the object, not allowing some method calls before the resource is retrieved.
        bool d_initialized;

        /// User id associated with this request
        std::string d_uid;

        /// The DAP type of the resource. See RemoteHttpResource::setType() for more.
        std::string d_type;

        /// The file name in which the content of the remote resource has been cached.
        std::string d_resourceCacheFileName;

        /// HTTP request headers added the curl HTTP GET request
        // std::vector<std::string> d_request_headers; // Request headers not used, maybe later

        /// The raw HTTP response headers returned by the request for the remote resource.
        std::vector<std::string> *d_response_headers; // Response headers

        /// The HTTP response headers returned by the request for the remote resource and parsed into KVP
        std::map<std::string, std::string> *d_http_response_headers; // Response headers

        /// The interval before a cache resource needs to be refreshed
        long long d_expires_interval;

#if 0
        // FIXME Not impl. jhrg 8/7/20
        /**
         * Determines the type of the remote resource. Looks at HTTP headers, and failing that compares the
         * basename in the resource URL to the data handlers TypeMatch.
         */
        void setType(const std::vector<std::string> *resp_hdrs);
#endif
        /**
         * Makes the curl call to write the resource to a file, determines DAP type of the content, and rewinds
         * the file descriptor.
         */
        void writeResourceToFile(int fd);

        /**
         * Ingests the HTTP headers into a queryable map. Once completed, determines the type of the remote resource.
         * Looks at HTTP headers, and failing that compares the basename in the resource URL to the data handlers TypeMatch.
         */
        void ingest_http_headers_and_type();

        /**
        * @brief Filter the cache and replaces all occurances each key in content_filters key with its associated value.
        *
        * WARNING: Does not lock cache. This method assumes that the process has already
        * acquired an exclusive lock on the cache file.
        *
        * @param template_str
        * @param update_str
        * @return
        */
        void filter_retrieved_resource(const std::map<std::string, std::string> &content_filters);

        /**
         * Checks if a cache resource is older than an hour
         *
         */
        bool cached_resource_is_expired();

        /**
         * method for calling update_file_and_header(map<string,string>) with a black map
         */
        void update_file_and_headers();

        /**
         * updates the file in the cache and the related headers file
         *
         * @param content_filters
         */
        void update_file_and_headers(const std::map<std::string, std::string> &content_filters);

        /**
         * finds the header file of a previously specified file and retrieves the related headers file
         */
        void load_hdrs_from_file();

    protected:
        RemoteResource() :
                d_remoteResourceUrl(), d_fd(0), d_initialized(false), d_resourceCacheFileName(""),
                d_response_headers(0), d_http_response_headers(0), d_expires_interval(HttpCache::getCacheExpiresTime()) {
        }

    public:
        // RemoteResource(const std::string &url, const std::string &uid = "", const std::string &echo_token = "");
        //RemoteResource(const std::string &url, const std::string &uid = "", long long expires_interval = HttpCache::getCacheExpiresTime());

        RemoteResource(std::shared_ptr<http::url> target_url, const std::string &uid = "", long long expires_interval = HttpCache::getCacheExpiresTime());

        virtual ~RemoteResource();

        void retrieveResource();
        void retrieveResource(const std::map<std::string, std::string> &content_filters);

        /**
         * Returns the DAP type std::string of the RemoteHttpResource
         * @return Returns the DAP type std::string used by the BES Containers.
         */
        std::string getType() {
            return d_type;
        }

        /**
         * Returns the (read-locked) cache file name on the local system in which the content of the remote
         * resource is stored. Deleting of the instance of this class will release the read-lock.
         */
        std::string getCacheFileName();

        std::string get_http_response_header(const std::string header_name);


        /**
         * Returns a std::vector of HTTP headers received along with the response from the request for the remote resource..
         */
        std::vector<std::string> *getResponseHeaders();


        /**
         * Returns cache file content in a string..
         */
        std::string get_response_as_string();

        rapidjson::Document get_as_json();

    };

} /* namespace http */

#endif /*  _bes_http_REMOTE_HTTP_RESOURCE_H_ */
