// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of ngap_module, A C++ module that can be loaded in to
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


#ifndef NgapOwnedContainerStorage_h_
#define NgapOwnedContainerStorage_h_ 1

#include <string>

#include "BESContainerStorageVolatile.h"

class BESCatalogUtils;

namespace ngap {

/** @brief implementation of BESContainerStorageVolatile that represents a
 * resource managed by the NASA NGAP/EOSDIS cloud-based data system. These
 * resources are stored in S3 and must be transferred to the server before
 * they can be used. In the Hyrax server, these are DMR++ (i.e., XML) files,
 * not the actual data files. The data files are stored in S3 but are subset
 * directly from S3 using information in the (much smaller) DMR++ files
 *
 * @see BESContainerStorageVolatile
 * @see GatewayContainer
 */
class NgapOwnedContainerStorage: public BESContainerStorageVolatile {
public:
    // Inherit all the constructors of the base class. jhrg 6/4/25
    using BESContainerStorageVolatile::BESContainerStorageVolatile;
#if 0
      explicit NgapOwnedContainerStorage(const std::string &n) : BESContainerStorageVolatile(n) {}
#endif

    ~NgapOwnedContainerStorage() override = default;


    void add_container(const std::string &s_name, const std::string &r_name, const std::string &type) override;

    void dump(std::ostream &strm) const override;
};

} // namespace ngap

#endif // NgapOwnedContainerStorage_h_
