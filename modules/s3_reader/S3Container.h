// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of S3_module, A C++ module that can be loaded in to
// the OPeNDAP Back-End Server (BES) and is able to handle remote requests.

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


#ifndef S3Container_h_
#define S3Container_h_ 1

#include <string>
#include <ostream>
#include <memory>

#include "BESContainer.h"
#include "RemoteResource.h"

namespace s3 {

/** @brief Container representing a remote request
 *
 * The real name of a S3Container is the actual remote request. When the
 * access method is called the remote request is made, the response
 * saved to file if successful, and the target response returned as the real
 * container that a data handler would then open.
 *
 * @see S3ContainerStorage
 */

class S3Container: public BESContainer {

    std::shared_ptr<http::RemoteResource> d_dmrpp_rresource = nullptr;

    void initialize();
    void _duplicate(S3Container &copy_to);

public:
    S3Container() = default;
    S3Container(const std::string &sym_name, const std::string &real_name, const std::string &type);

    S3Container(const S3Container &copy_from) = delete;
    S3Container& operator=(const S3Container& other) = delete;

    ~S3Container() override = default ;

    // These three methods are abstract in the BESContainer parent class. jhrg 10/18/22
    BESContainer *ptr_duplicate() override;

    void filter_response(const std::map<std::string, std::string, std::less<>> &content_filters) const;

    std::string access() override;

    bool release() override;

    void dump(std::ostream &strm) const override;
};

} // namespace s3

#endif // S3Container_h_
