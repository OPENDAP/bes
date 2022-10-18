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


#ifndef NgapContainer_h_
#define NgapContainer_h_ 1

#include <string>
#include <ostream>

#include "BESContainer.h"
#include "RemoteResource.h"


namespace ngap {



/** @brief Container representing a remote request
 *
 * The real name of a NgapContainer is the actual remote request. When the
 * access method is called the remote request is made, the response
 * saved to file if successful, and the target response returned as the real
 * container that a data handler would then open.
 *
 * @see NgapContainerStorage
 */
enum RestifiedPathValues { cmrProvider, cmrDatasets, cmrGranuleUR };

class NgapContainer: public BESContainer {

private:
    http::RemoteResource *d_dmrpp_rresource;

    // std::vector<std::string> d_collections;
    // std::vector<std::string> d_facets;

    virtual void initialize();

    bool inject_data_url();


protected:
    void _duplicate(NgapContainer &copy_to);

    NgapContainer() :
            BESContainer(), d_dmrpp_rresource(nullptr)
    {
    }

public:
    NgapContainer(const std::string &sym_name, const std::string &real_name, const std::string &type);

    NgapContainer(const NgapContainer &copy_from);

    // void get_granule_path(const std::string &path) const ;

    static bool signed_url_is_expired(std::map<std::string,std::string> url_info);

    virtual ~NgapContainer();

    virtual BESContainer * ptr_duplicate();

    virtual std::string access();

    virtual bool release();

    virtual void dump(std::ostream &strm) const;
};

} // namespace ngap

#endif // NgapContainer_h_
