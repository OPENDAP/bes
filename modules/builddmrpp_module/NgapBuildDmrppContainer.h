// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of builddmrpp_module, A C++ module that can be loaded in to
// the OPeNDAP Back-End Server (BES) and is able to handle remote requests.

// Copyright (c) 2023 OPeNDAP, Inc.
// Author: Daniel Holloway <dholloway@opendap.org>
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
//      dan       Daniel Holloway <dholloway@opendap.org>

#ifndef NgapBuildDmrppContainer_h_
#define NgapBuildDmrppContainer_h_ 1

#include <string>
#include <ostream>
#include <memory>

#include "BESContainer.h"

namespace http {
class RemoteResource;
}

namespace builddmrpp {

/** @brief Container representing a remote request
 *
 * The real name of a NgapBuildDmrppContainer is the actual remote request. When the
 * access method is called the remote request is made, the response
 * saved to file if successful, and the target response returned as the real
 * container that a data handler would then open.
 *
 * @see NgapBuildDmrppContainerStorage
 */
class NgapBuildDmrppContainer : public BESContainer {

private:
    std::shared_ptr<http::RemoteResource> d_data_rresource = nullptr;
    std::string d_real_name;         ///< The full name of the thing (filename, database table name, ...)

    void initialize();

protected:
    void _duplicate(NgapBuildDmrppContainer &copy_to);

public:
    NgapBuildDmrppContainer() = default;
    NgapBuildDmrppContainer(const std::string &sym_name, const std::string &real_name, const std::string &type);
    NgapBuildDmrppContainer(const NgapBuildDmrppContainer &copy_from);
    ~NgapBuildDmrppContainer() override = default;

    BESContainer *ptr_duplicate() override;

    std::string access() override;
    bool release() override { return true; }

    void dump(std::ostream &strm) const override;
};

} // namespace builddmrpp

#endif // NgapBuildDmrppContainer_h_
