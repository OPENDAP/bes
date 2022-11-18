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
 * CmrApi.h
 *
 *  Created on: July, 13 2018
 *      Author: ndp
 */

#ifndef MODULES_CMR_MODULE_CMRAPI_H_
#define MODULES_CMR_MODULE_CMRAPI_H_

#include <string>
#include <vector>
#include <memory>

#include "nlohmann/json.hpp"

#include "BESCatalogUtils.h"

#include "CmrNames.h"
#include "Provider.h"
#include "Collection.h"
#include "Granule.h"

namespace cmr {


class CmrApi {
private:
    std::string d_cmr_endpoint_url;
    std::string d_cmr_provider_search_endpoint_url;
    std::string d_cmr_providers_search_endpoint_url;
    std::string d_cmr_collections_search_endpoint_url;
    std::string d_cmr_granules_search_endpoint_url;

    const nlohmann::json &get_temporal_group(const nlohmann::json &cmr_doc);

    const nlohmann::json &get_year_group(const nlohmann::json &cmr_doc);

    const nlohmann::json &get_month_group(const std::string &for_year, const nlohmann::json &cmr_doc);

    const nlohmann::json &get_month(const std::string &target_month,
                                    const std::string &target_year,
                                    const nlohmann::json &cmr_doc);

    const nlohmann::json &get_day_group(const std::string &target_month,
                                                const std::string &target_year,
                                                const nlohmann::json &cmr_doc);

    const nlohmann::json &get_children(const nlohmann::json &jobj);

    const nlohmann::json &get_feed(const nlohmann::json &cmr_doc);

    const nlohmann::json& get_entries(const nlohmann::json &cmr_doc);

    void granule_search(const std::string &collection_name, const std::string &r_year, const std::string &r_month, const std::string &r_day, nlohmann::json &cmr_doc);

    void get_collections_worker(const std::string &provider_id, std::vector<cmr::Collection> &collections,
                         unsigned int page_size=CMR_MAX_PAGE_SIZE,
                         bool just_opendap=false );



public:
    CmrApi();

    void get_years(const std::string &collection_name, std::vector<std::string> &years_result);

    void get_months(const std::string &collection_name, const std::string &year, std::vector<std::string> &months_result);

    void get_days(const std::string& collection_name, const std::string& r_year, std::string r_month, std::vector<std::string> &days_result);

    void get_granule_ids(const std::string& collection_name,
                         const std::string& r_year,
                         const std::string &r_month,
                         const std::string &r_day,
                         std::vector<std::string> &granule_ids);


    void get_granules(const std::string& collection_name,
                      const std::string &r_year,
                      const std::string &r_month,
                      const std::string &r_day,
                      std::vector<cmr::Granule *> &granule_objs);


    void get_collection_ids(std::vector<std::string> &collection_ids);

    unsigned long granule_count(const std::string &collection_name,
                                const std::string &r_year,
                                const std::string &r_month,
                                const std::string &r_day);

    unsigned long granule_count_OLD(std::string collection_name,
                                    std::string r_year,
                                    std::string r_month,
                                    std::string r_day);


    cmr::Granule *get_granule(const std::string path);

    cmr::Granule *get_granule(const std::string& collection_name,
                              const std::string& r_year,
                              const std::string& r_month,
                              const std::string& r_day,
                              const std::string& granule_id);

    Provider get_provider(const std::string &provider_id);
    void get_providers(std::vector<cmr::Provider> &providers);
    void get_opendap_providers(std::vector<cmr::Provider> &providers);
    unsigned int get_opendap_collections_count(const std::string &provider_id);

    void get_collections(const std::string &provider_id, std::vector<cmr::Collection> &collections );
    void get_opendap_collections(const std::string &provider_id, std::vector<cmr::Collection> &collections );

};



} // namespace cmr

#endif /* MODULES_CMR_MODULE_CMRAPI_H_ */
