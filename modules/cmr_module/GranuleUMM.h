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
#ifndef MODULES_CMR_MODULE_GRANULE_UMM_H_
#define MODULES_CMR_MODULE_GRANULE_UMM_H_

#include <string>
#include <vector>
#include "nlohmann/json.hpp"
#include "CatalogItem.h"
#include "BESCatalogUtils.h"


namespace cmr {

/**
 *
 * GranuleUMM
 *     Represents a CMR granule as returned by the granule.umm_json response.
 *
 * @TODO Make a better implementation for this that is more likely to be successful
 *   one thing we might want to consider is combining the two granule classes, Granule and GranuleUMM
 *   the Granule class ingests the granule.json response from CMR. This class, GranuleUMM ingests the
 *   granule.umm_json response from CMR. Different information is available in each response and combining them
 *   might allow us to more reliably retrieve things like the granule's size.
 */
class GranuleUMM {
private:

    std::string d_name;
    std::string d_id;
    std::string d_data_access_url;
    std::string d_dap_service_url;
    std::string d_metadata_access_url;
    double d_size_orig{};
    uint64_t d_size{};
    std::string d_size_str;
    std::string d_size_units_str;
    std::string d_last_modified_time;
    std::string d_description;

    void setName(const nlohmann::json& granule_umm_json);
    void setConceptId(const nlohmann::json& granule_umm_json);
    void setDataGranuleUrl(const nlohmann::json& granule_umm_json);
    void setDapServiceUrl(const nlohmann::json& granule_umm_json);
    void setSize(const nlohmann::json& granule_umm_json);
    void setLastModifiedStr(const nlohmann::json& granule_umm_json);
    void setDescription(const nlohmann::json& granule_umm_json);


public:
    explicit GranuleUMM(const nlohmann::json& granule_umm_json);

    std::string getName() const { return d_name; }
    std::string getConceptId() const { return d_id; }
    std::string getDataGranuleUrl() const { return d_data_access_url; }
    std::string getDapServiceUrl() const { return d_dap_service_url; }
    std::string getMetadataAccessUrl() const { return d_metadata_access_url; }
    std::string getSizeStr() const { return d_size_str; }
    std::string getLastModifiedStr() const { return d_last_modified_time; }
    uint64_t getSize() const { return d_size; }

    // For now we use getName() until a better option appears.
    std::string getDescription() const { return getName(); }

    bes::CatalogItem *getCatalogItem(BESCatalogUtils *d_catalog_utils);
};

} // namespace cmr

#endif /* MODULES_CMR_MODULE_GRANULE_UMM_H_ */



