// CmrContainerStorage.h

// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of cnr_module, A C++ module that can be loaded in to
// the OPeNDAP Back-End Server (BES) and is able to handle remote requests.

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

// Authors:
//      ndp       Nathan Potter <ndp@opendap.org>

#ifndef CmrContainerStorage_h_
#define CmrContainerStorage_h_ 1

#include "BESContainerStorageVolatile.h"

class BESCatalogUtils;

//namespace cmr {

/** @brief implementation of BESContainerStorageVolatile that represents a
 * list of remote requests
 *
 * Each of the containers stored in the GatewayContainerStorage represents
 * a remote request. When accessed the container will make the remote
 * request in order to create the target response.
 *
 * @see BESContainerStorageVolatile
 * @see GatewayContainer
 */
class CmrContainerStorage: public BESContainerStorageVolatile {
public:
    CmrContainerStorage(const std::string &n);
    virtual ~CmrContainerStorage();

    virtual void add_container(const std::string &s_name, const std::string &r_name, const std::string &type);

    virtual void dump(std::ostream &strm) const;
};

//} // namespace gateway

#endif // CmrContainerStorage_h_
