
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

#ifndef I_HttpdCatalogContainer_H_
#define I_HttpdCatalogContainer_H_ 1

#include <string>
#include <ostream>

#include <BESContainer.h>

namespace httpd_catalog {

class RemoteHttpResource;

/** @brief Container representing a remote request
 *
 * The real name of a HttpdCatalogContainer is the actual remote request. When the
 * access method is called the remote request is made, the response
 * saved to file if successful, and the target response returned as the real
 * container that a data handler would then open.
 *
 * @see GatewayContainerStorage
 */
class HttpdCatalogContainer: public BESContainer {
private:
    /// Remote resource for this container, nullptr means it has not yet been accessed
    RemoteHttpResource *d_remoteResource;

    HttpdCatalogContainer() :
        BESContainer(), d_remoteResource(0)
    {
    }

protected:
    void _duplicate(HttpdCatalogContainer &copy_to);

public:
    HttpdCatalogContainer(const std::string &sym_name, const std::string &real_name, const std::string &type);

    HttpdCatalogContainer(const HttpdCatalogContainer &copy_from);

    virtual ~HttpdCatalogContainer();

    virtual BESContainer * ptr_duplicate();

    virtual std::string access();

    virtual bool release();

    virtual void dump(std::ostream &strm) const;
};

} // namespace http_catalog

#endif // I_HttpdCatalogContainer_H_
