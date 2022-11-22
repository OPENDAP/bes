// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of cmr_module, A C++ MODULE that can be loaded in to
// the OPeNDAP Back-End Server (BES) and is able to handle remote requests.

// Copyright (c) 2015 OPeNDAP, Inc.
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

/*
 * Granule.h
 *
 *  Created on: July, 14 2018
 *      Author: ndp
 */
#ifndef MODULES_CMR_MODULE_GRANULE_H_
#define MODULES_CMR_MODULE_GRANULE_H_

#include <string>
#include <vector>
#include "nlohmann/json.hpp"
#include "CatalogItem.h"
#include "BESCatalogUtils.h"


namespace cmr {

class Granule {
private:

    std::string d_name;
    std::string d_id;
    std::string d_data_access_url;
    std::string d_metadata_access_url;
    std::string d_size_str;
    std::string d_last_modified_time;

    void setName(const nlohmann::json& jobj);
    void setId(const nlohmann::json& go);
    void setDataAccessUrl(const nlohmann::json& go);
    void setMetadataAccessUrl(const nlohmann::json& granule_obj);
    void setSize(const nlohmann::json& j_obj);
    void setLastModifiedStr(const nlohmann::json& go);
    const nlohmann::json& get_links_array(const nlohmann::json& go);

public:
    Granule(const nlohmann::json& granule_json);

    std::string getName(){ return d_name; }
    std::string getId(){ return d_id; }
    std::string getDataAccessUrl() { return d_data_access_url; }
    std::string getMetadataAccessUrl(){ return d_metadata_access_url; }
    std::string getSizeStr(){ return d_size_str; }
    std::string getLastModifiedStr() { return d_last_modified_time; }
    float getSize(){ return atof(getSizeStr().c_str())*1024*1204; }

    bes::CatalogItem *getCatalogItem(BESCatalogUtils *d_catalog_utils);
};

} // namespace cmr

#endif /* MODULES_CMR_MODULE_GRANULE_H_ */



