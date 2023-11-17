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

/**
 * @brief Container representing a remote request to information stored in
 * the NASA NGAP/EOSDIS cloud-based data management system.
 *
 * This container nominally stores the 'restified' URL to a NASA granule.
 * The container handles the two operations needed to access a DMR++ file
 * that can _then_ be used to read data from that granule.
 *
 * THe first operation is to ask the CMR subsystem to translate the restified
 * path to a true URL that references the actual data granule in S3. We
 * assume that the DMR++ for that granule is 'next to' the granule and is
 * found by appending '.dmrpp' to the granule URL.
 *
 * The second operation is to then retrieve that DMR++ file and store it in
 * a cache as text (DMR++ files are XML).
 *
 * The NgapContainer::access() method performs the two operations the first
 * time it is called. Subsequent calls to NgapContainer::access() will return
 * cached XML text and not the filename of the DMR++ local file.
 *
 * @note in the future, we may want to store the DMR++ _only_ as a string.
 * jhrg 10/16/23
 *
 * @see NgapContainerStorage
 */
class NgapContainer: public BESContainer {

    std::string d_ngap_path;    // The (in)famous restified path

    void set_real_name_using_cmr_or_cache();
    bool get_dmrpp_from_cache_or_remote_source(std::string &dmrpp_string) const;

    bool get_content_filters(std::map<std::string, std::string, std::less<>> &content_filters) const;
    void filter_response(const std::map<std::string, std::string, std::less<>> &content_filters, std::string &content) const;

    static bool inject_data_url();

    friend class NgapContainerTest;

protected:
    void _duplicate(NgapContainer &copy_to);

public:
    NgapContainer() = default;
    NgapContainer(const NgapContainer &copy_from) = delete;
    ~NgapContainer() override = default;

    NgapContainer &operator=(const NgapContainer &rhs) = delete;

    /**
     * @brief Creates an instances of NgapContainer with symbolic name and real
     * name, which is the remote request.
     *
     * The real_name is the remote request URL.
     *
     * @param sym_name symbolic name representing this remote container
     * @param real_name The NGAP restified path.
     * @throws BESSyntaxUserError if the url does not validate
     * @see NgapUtils
     */
    NgapContainer(const std::string &sym_name, const std::string &real_name, const std::string &)
            : BESContainer(sym_name, real_name, "ngap"), d_ngap_path(real_name) {}

    BESContainer * ptr_duplicate() override;

    void set_ngap_path(const std::string &ngap_path) { d_ngap_path = ngap_path; }
    std::string get_ngap_path() const { return d_ngap_path; }

    std::string access() override;

    bool release() override {
        return true;
    }

    void dump(std::ostream &strm) const override;
};

} // namespace ngap

#endif // NgapContainer_h_
