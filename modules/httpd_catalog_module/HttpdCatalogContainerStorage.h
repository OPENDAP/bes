
// -*- mode: c++; c-basic-offset:4 -*-
//
// This file is part of httpd_catalog_module, A C++ module that can be loaded in to
// the OPeNDAP Back-End Server (BES) and is able to handle remote requests.
//
// Copyright (c) 2018 OPeNDAP, Inc.
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
//
// Authors:
//      ndp       Nathan Potter <ndp@opendap.org>

#ifndef I_HttpdCatalogContainerStorage_H_
#define I_HttpdCatalogContainerStorage_H_ 1

#include "BESContainerStorageVolatile.h"

class BESCatalogUtils;

namespace httpd_catalog {

/** @brief implementation of BESContainerStorageVolatile that represents a
 * list of remote requests
 *
 * Each of the containers stored in the HttpdCatalogContainerStorage represents
 * a remote request. When accessed the container will make the remote
 * request in order to create the target response.
 *
 * @see BESContainerStorageVolatile
 * @see HttpdCatalogContainer
 */
class HttpdCatalogContainerStorage: public BESContainerStorageVolatile {
public:
    HttpdCatalogContainerStorage(const std::string &n);
    virtual ~HttpdCatalogContainerStorage();

    virtual void add_container(const std::string &s_name, const std::string &r_name, const std::string &type);

    void dump(std::ostream &strm) const override;
};

} // namespace httpd_catalog

#endif // I_HttpdCatalogContainerStorage_H_
