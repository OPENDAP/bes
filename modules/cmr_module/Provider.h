// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of cmr_module, A C++ MODULE that can be loaded in to
// the OPeNDAP Back-End Server (BES) and is able to handle remote requests.

// Copyright (c) 2022 OPeNDAP, Inc.
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

#ifndef HYRAX_GIT_PROVIDER_H
#define HYRAX_GIT_PROVIDER_H

#include "config.h"

#include <string>
#include <utility>
#include <vector>

#include "nlohmann/json.hpp"

namespace cmr {

class Provider {
private:
    nlohmann::json d_provider_json_obj;
    unsigned long long d_opendap_collection_count{};
public:
    explicit Provider(nlohmann::json provider_obj): d_provider_json_obj(std::move(provider_obj)){}

    std::string id();
    std::string description_of_holdings();
    std::string organization_name();
    nlohmann::json contacts();
    bool rest_only();

    void set_opendap_collection_count(unsigned long long count){ d_opendap_collection_count = count; }
    unsigned long long get_opendap_collection_count(){ return d_opendap_collection_count; }
    std::string to_string();
};

} // cmr

#endif //HYRAX_GIT_PROVIDER_H
