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


#ifndef NgapOwnedContainer_h_
#define NgapOwnedContainer_h_ 1

#include <string>
#include <map>
#include <memory>

#include "BESContainer.h"

namespace http {
class RemoteResource;
}

namespace ngap {

/**
 * @brief Container representing a data resource that is 'owned' by NGAP/EDC/NASA
 * but that is described by a DMR++ that OPeNDAP 'owns.'
 *
 * This container nominally stores the 'restified' URL to a NASA granule.
 * The container handles the operation needed to access a DMR++ file
 * that can _then_ be used to read data from that granule.
 *
 * This operation is to retrieve that DMR++ file and store it in
 * a cache as text (DMR++ files are XML).
 *
 * The NgapOwnedContainer::access() method performs this operation the first
 * time it is called. Subsequent calls to NgapOwnedContainer::access() will return
 * cached XML text. Like NgapContainer::access(), this object uses the container
 * attributes to indicate that access() may return a string that holds the
 * DMR++ XML text.
 *
 * @see NgapOwnedContainerStorage
 */
class NgapOwnedContainer: public BESContainer {

    std::string d_ngap_path;    // The (in)famous REST path
    std::string d_data_source_location = "https://s3.amazonaws.com/cloudydap"; // FIXME Remove hardcoded value. jhrg 5/17/24

    bool get_dmrpp_from_cache_or_remote_source(std::string &dmrpp_string) const;

    // I made these static so that they will be in the class' namespace but still
    // be easy to test in the unit tests. jhrg 4/29/24
    static bool file_to_string(int fd, std::string &content);
    static std::string build_dmrpp_url_to_owned_bucket(const std::string &rest_path, const std::string &data_source);
    static std::string build_dmrpp_url_to_daac_bucket(const std::string &rest_path);

    void set_real_name_using_cmr_or_cache();

    bool get_item_from_dmrpp_cache(std::string &dmrpp_string) const;
    bool put_item_in_dmrpp_cache(const std::string &dmrpp_string) const;

    friend class NgapOwnedContainerTest;

protected:
    void _duplicate(NgapOwnedContainer &copy_to) {
        copy_to.d_ngap_path = d_ngap_path;
        BESContainer::_duplicate(copy_to);
    }

public:
    NgapOwnedContainer() = default;
    NgapOwnedContainer(const NgapOwnedContainer &copy_from) = delete;
    NgapOwnedContainer(NgapOwnedContainer &&move_from) = delete;
    NgapOwnedContainer &operator=(const NgapOwnedContainer &rhs) = delete;
    NgapOwnedContainer &operator=(NgapOwnedContainer &&rhs) = delete;
    ~NgapOwnedContainer() override = default;

    /**
     * @brief Creates an instances of NgapOwnedContainer with symbolic name and real
     * name, which is the remote request.
     *
     * The real_name is the remote request URL.
     *
     * @param sym_name symbolic name representing this remote container
     * @param real_name The NGAP restified path.
     * @throws BESSyntaxUserError if the url does not validate
     * @see NgapUtils
     */
    NgapOwnedContainer(const std::string &sym_name, const std::string &real_name, const std::string &)
            : BESContainer(sym_name, real_name, "owned-ngap"), d_ngap_path(real_name) {}

    BESContainer *ptr_duplicate() override {
        auto container = std::make_unique<NgapOwnedContainer>();
        _duplicate(*container);
        return container.release();
    }

    void set_ngap_path(const std::string &ngap_path) { d_ngap_path = ngap_path; }
    std::string get_ngap_path() const { return d_ngap_path; }

    void set_data_source_location(const std::string &data_source_location) {
        d_data_source_location = data_source_location;
    }
    std::string get_data_source_location() const { return d_data_source_location; }

    std::string access() override;

    bool release() override {
        return true;
    }

    void dump(std::ostream &strm) const override;
};

} // namespace ngap

#endif // NgapOwnedContainer_h_
