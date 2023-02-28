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


#ifndef BuildDmrppContainer_h_
#define BuildDmrppContainer_h_ 1

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

class BuildDmrppContainer : public BESContainer {

private:
    http::RemoteResource *d_data_rresource;

    std::string d_real_name;         ///< The full name of the thing (filename, database table name, ...)

    // std::vector<std::string> d_collections;
    // std::vector<std::string> d_facets;

    virtual void initialize();

    bool inject_data_url();

protected:
    void _duplicate(BuildDmrppContainer &copy_to);

    BuildDmrppContainer() :
            BESContainer(), d_data_rresource(nullptr)
    {
    }

public:
    BuildDmrppContainer(const std::string &sym_name, const std::string &real_name, const std::string &type);

    BuildDmrppContainer(const BuildDmrppContainer &copy_from);

    // void get_granule_path(const std::string &path) const ;

    static bool signed_url_is_expired(std::map<std::string,std::string> url_info);

    virtual ~BuildDmrppContainer();

    virtual BESContainer * ptr_duplicate();

    virtual std::string access();

    virtual bool release();

    virtual void dump(std::ostream &strm) const;

    /** @brief retrieve the real name for this container, such as a
     * file name.
     *
     * @return real name, such as file name
     *//*
    std::string get_real_name() const
    {
        return d_real_name;
    }

    *//** @brief set the real name for this container, such as a file name
     * if reading a data file.
     *
     * @param real_name real name, such as the file name
     *//*
    void set_real_name(const std::string &real_name)
    {
        d_real_name = real_name;
    }*/
};

} // namespace ngap

#endif // BuildDmrppContainer_h_
