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
#include <map>
#include <memory>

#include "BESContainer.h"

namespace http {
class RemoteResource;
}

namespace ngap {

enum RestifiedPathValues { cmrProvider, cmrDatasets, cmrGranuleUR };

/** @brief Container representing a remote request
 *
 * The real name of a NgapContainer is the actual remote request. When the
 * access method is called the remote request is made, the response
 * saved to file if successful, and the target response returned as the real
 * container that a data handler would then open.
 *
 * @see NgapContainerStorage
 */
class NgapContainer: public BESContainer {

private:
    // Make this shared so containers can be copied. jhrg 9/20/23
    std::shared_ptr<http::RemoteResource> d_dmrpp_rresource = nullptr;
    std::string d_ngap_path;    // The (in)famous restified path

    void set_real_name_using_cmr_or_cache();
    void cache_dmrpp_contents(std::shared_ptr<http::RemoteResource> &d_dmrpp_rresource);

    bool get_content_filters(std::map<std::string,std::string> &content_filters) const;
    void filter_response(const std::map<std::string, std::string> &content_filters) const;

    static bool inject_data_url();

protected:
    void _duplicate(NgapContainer &copy_to);

public:
    NgapContainer() = default;
    NgapContainer(const std::string &sym_name, const std::string &real_name, const std::string &type);
    NgapContainer(const NgapContainer &copy_from) = delete;

    ~NgapContainer() override;
    NgapContainer &operator=(const NgapContainer &rhs) = delete;

    BESContainer * ptr_duplicate() override;

    bool access_returns_cached_content() const; // hack jhrg 9/20/23
    std::string access() override;

    bool get_cached_dmrpp_string(std::string &dmrpp_string) const;
    bool is_dmrpp_cached() const;

    bool release() override;

    void dump(std::ostream &strm) const override;
};

} // namespace ngap

#endif // NgapContainer_h_
