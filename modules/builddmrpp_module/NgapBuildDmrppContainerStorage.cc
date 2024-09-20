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

#include "config.h"

#include <string>

#include "RemoteResource.h"
#include "NgapBuildDmrppContainerStorage.h"
#include "NgapBuildDmrppContainer.h"

using namespace std;

namespace builddmrpp {

/** @brief adds a container with the provided information
 *
 * @param s_name symbolic name for the container
 * @param r_name the remote request url
 * @param type ignored. The type of the target response is determined by the
 * request response, or could be passed in
 */
void NgapBuildDmrppContainerStorage::add_container(const string &s_name, const string &r_name, const string &type)
{
    auto c = make_unique<NgapBuildDmrppContainer>(s_name, r_name, "h5");

    // These line causes the Build DMR++ Container to access the contained resource.
    // That generally means the remote resource is retrieved and a temporary file is created.
    // jhrg 6/12/23
    (void) c->access();

    BESContainerStorageVolatile::add_container(c.release());
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance along with information about
 * each of the GatewayContainers already stored.
 *
 * @param strm C++ i/o stream to dump the information to
 */
void NgapBuildDmrppContainerStorage::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << "NgapBuildDmrppContainerStorage::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();
    BESContainerStorageVolatile::dump(strm);
    BESIndent::UnIndent();
}

} // namespace builddmrpp
