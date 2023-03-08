// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of the BES http package, part of the Hyrax data server.

// Copyright (c) 2020,2023 OPeNDAP, Inc.
// Authors: Nathan Potter <ndp@opendap.org>,
// James Gallagher <jgallagher@opendap.org>
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

#ifndef  _remote_resource2_h_
#define  _remote_resource2_h_ 1

#include <curl/curl.h>
#include <curl/easy.h>

#include <memory>
#include <string>
#include <vector>

#include "url_impl.h"
#include "RemoteResource2.h"

namespace http {

/**
 * This class encapsulates a remote resource available via HTTP GET. It will
 * retrieve the content of the resource and place it in a local disk cache
 * for rapid (subsequent) access. It can be configured to use a proxy server
 * for the outgoing requests.
 */
class RemoteResource2 {
private:
    friend class RemoteResource2Test;

    /// Resource URL that an instance of this class represents
    std::shared_ptr<http::url> d_remoteResourceUrl;

    /// Open file descriptor for the resource content (Returned from the cache).
    int d_fd = 0;

    /// Protect the state of the object, not allowing some method calls before the resource is retrieved.
    bool d_initialized = false;

    /// User id associated with this request
    std::string d_uid;

    /// The DAP type of the resource. See RemoteHttpResource::setType() for more.
    std::string d_type;

    /// The raw HTTP response headers returned by the request for the remote resource.
    std::vector<std::string> d_response_headers; // Response headers

    /**
     * Makes the curl call to write the resource to a file, determines DAP type of the content, and rewinds
     * the file descriptor.
     */
    void writeResourceToFile(int fd);

    /**
    * @brief Filter the cache and replaces all occurrences each key in content_filters key with its associated value.
    *
    * WARNING: Does not lock cache. This method assumes that the process has already
    * acquired an exclusive lock on the cache file.
    *
    * @param template_str
    * @param update_str
    * @return
    */
    void filter_retrieved_resource(const std::map<std::string, std::string> &content_filters) const;

public:
    RemoteResource2() = default;

    explicit RemoteResource2(std::shared_ptr<http::url> target_url, std::string uid = "");

    virtual ~RemoteResource2() = default;

    void retrieveResource();
    void retrieveResource(const std::map<std::string, std::string> &content_filters);

    /**
     * Returns the DAP type std::string of the RemoteHttpResource
     * @return Returns the DAP type std::string used by the BES Containers.
     */
    std::string getType() const {
        return d_type;
    }

    /**
     * Returns cache file content in a string..
     */
    std::string get_response_as_string() const;
};

} /* namespace http */

#endif /* _remote_resource2_h_ */
