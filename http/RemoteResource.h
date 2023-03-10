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

#include <memory>
#include <string>
#include <vector>

#include "CurlUtils.h"
#include "rapidjson/document.h"

namespace http {

class url;

/**
 * This class encapsulates a remote resource available via HTTP GET. It will
 * retrieve the content of the resource and place it in a local disk cache
 * for rapid (subsequent) access. It can be configured to use a proxy server
 * for the outgoing requests.
 */
class RemoteResource {
private:
    friend class RemoteResourceTest;

    static std::string d_temp_file_dir;

    /// Resource URL that an instance of this class represents
    std::shared_ptr<http::url> d_url;

    /// Open file descriptor for the resource content
    int d_fd = 0;

    /// Protect the state of the object, not allowing some method calls before the resource is retrieved.
    bool d_initialized = false;

    /// User id associated with this request
    std::string d_uid;

    /// The BES type of the resource
    std::string d_type;

    /// The file name in which the content is stored.
    std::string d_filename;

    /// The basename of the URL or file::// path. Used to name the temp file in a way that is meaningful
    /// to libdap when it builds DDS, etc., objects.
    std::string d_basename;

    /// If true, d_filename is a temporary file and should be deleted when the object is destroyed.
    bool d_delete_file = false;

    /// The raw HTTP response headers returned by the request for the remote resource.
    std::vector<std::string> d_response_headers; // Response headers

    /// write the url content to a file, set the type, and rewind the file descriptor
    void get_url(int fd);

public:
    /// The default constructor is here to ease testing. Remove if not needed. jhrg 3/8/23
    RemoteResource() = default;
    RemoteResource(const RemoteResource &rhs) = delete;
    RemoteResource &operator=(const RemoteResource &rhs) = delete;
    explicit RemoteResource(std::shared_ptr<http::url> target_url, std::string uid = "");

    virtual ~RemoteResource();

    void retrieve_resource();

    /**
     * Returns the DAP type std::string of the RemoteHttpResource
     * @return Returns the DAP type std::string used by the BES Containers.
     */
    std::string get_type() const { return d_type; }

    /// Returns the file name in which the content of the URL has been stored.
    std::string get_filename() const {return d_filename; }

    /// Return the file descriptor to the open temp file
    int get_fd() const { return d_fd; }

    /// Set the file descriptor to the open temp file. Useful when
    void set_fd(int fd) { d_fd = fd; }
};

} /* namespace http */

#endif /*  _bes_http_REMOTE_HTTP_RESOURCE_H_ */
