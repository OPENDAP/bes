// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of cmr_module, A C++ module that can be loaded in to
// the OPeNDAP Back-End Server (BES) and is able to handle remote requests.

// Copyright (c) 2013 OPeNDAP, Inc.
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

#ifndef CMR_REMOTERESOURCE_H_
#define CMR_REMOTERESOURCE_H_

#include <curl/curl.h>
#include <curl/easy.h>

#include <string>
#include <vector>

//#include <libdap/util.h>
#include <libdap/InternalErr.h>
#include <libdap/RCReader.h>

namespace cmr {

/**
 * This class encapsulates a remote resource available via HTTP GET. It will
 * retrieve the content of the resource and place it in a local disk cache
 * for rapid (subsequent) access. It can be configure to use a proxy server
 * for the outgoing requests.
 */
class RemoteHttpResource {
private:
    /// Resource URL that an instance of this class represents
    std::string d_remoteResourceUrl;

    /**
     * Open file descriptor for the resource content (Returned from the cache).
     */
    int d_fd;

    /// Protect the state of the object, not allowing some method calls before the resource is retrieved.
    bool d_initialized;

    /// An pointer to a CURL object to use for any HTTP transactions.
    CURL *d_curl;

    /// @TODO This variable fails to accumulate error message content when curl has problems. FIX.
    char d_error_buffer[CURL_ERROR_SIZE]; // A human-readable message.

    /// The DAP type of the resource. See RemoteHttpResource::setType() for more.
    std::string d_type;

    /// The file name in which the content of the remote resource has been cached.
    std::string d_resourceCacheFileName;

    /// HTTP request headers added the curl HTTP GET request
    std::vector<std::string> *d_request_headers; // Request headers

    /// The HTTP response headers returned by the request for the remote resource.
    std::vector<std::string> *d_response_headers; // Response headers

    /// The HTTP response headers returned by the request for the remote resource.
    std::map<std::string,std::string> *d_http_response_headers; // Response headers

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

protected:
    RemoteHttpResource() :
        d_fd(0), d_initialized(false), d_curl(0), d_resourceCacheFileName(""), d_request_headers(0), d_response_headers(
            0), d_http_response_headers(0)
    {
    }

public:
    RemoteHttpResource(const std::string &url);
    virtual ~RemoteHttpResource();

    void retrieveResource();

    /**
     * Returns the DAP type std::string of the RemoteHttpResource
     * @return Returns the DAP type std::string used by the BES Containers.
     */
    std::string getType()
    {
        return d_type;
    }

    /**
     * Returns the (read-locked) cache file name on the local system in which the content of the remote
     * resource is stored. Deleting of the instance of this class will release the read-lock.
     */
    std::string getCacheFileName()
    {
        if (!d_initialized)
            throw libdap::Error(
                "RemoteHttpResource::getCacheFileName() - STATE ERROR: Remote Resource Has Not Been Retrieved.");
        return d_resourceCacheFileName;
    }

    std::string get_http_response_header(const std::string header_name);


    /**
     * Returns a std::vector of HTTP headers received along with the response from the request for the remote resource..
     */
    std::vector<std::string> *getResponseHeaders()
    {
        if (!d_initialized)
            throw libdap::Error(
                "RemoteHttpResource::getCacheFileName() - STATE ERROR: Remote Resource Has Not Been Retrieved.");
        return d_response_headers;
    }


};

} /* namespace cmr */

#endif /* CMR_REMOTERESOURCE_H_ */
