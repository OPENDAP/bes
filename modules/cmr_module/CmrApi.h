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
#include <map>
#include <memory>

#include "nlohmann/json.hpp"

#include "BESCatalogUtils.h"

#include "CmrNames.h"
#include "Provider.h"
#include "Collection.h"
#include "Granule.h"
#include "GranuleUMM.h"

namespace cmr {


class CmrApi {
private:
    std::string d_cmr_endpoint_url{DEFAULT_CMR_HOST_URL};
    std::string d_cmr_provider_search_endpoint_url;
    std::string d_cmr_providers_search_endpoint_url;
    std::string d_cmr_collections_search_endpoint_url;
    std::string d_cmr_granules_search_endpoint_url;
    std::string d_cmr_granules_umm_search_endpoint_url;
    const nlohmann::json &get_temporal_group(const nlohmann::json &cmr_doc) const;

    const nlohmann::json &get_year_group(const nlohmann::json &cmr_doc) const;
    const nlohmann::json &get_years(const nlohmann::json &cmr_doc) const;
    const nlohmann::json &get_year(const std::string &target_year, const nlohmann::json &cmr_doc) const;


    const nlohmann::json &get_month_group(const std::string &for_year, const nlohmann::json &cmr_doc) const;

    const nlohmann::json &get_month(const std::string &target_month,
                                    const std::string &target_year,
                                    const nlohmann::json &cmr_doc) const;

    const nlohmann::json &get_day_group(const std::string &target_month,
                                                const std::string &target_year,
                                                const nlohmann::json &cmr_doc) const;

    const nlohmann::basic_json<> &get_items(const nlohmann::basic_json<> &cmr_doc) const;

    const nlohmann::json &get_children(const nlohmann::json &jobj) const;

    const nlohmann::json &get_feed(const nlohmann::json &cmr_doc) const;

    const nlohmann::json& get_entries(const nlohmann::json &cmr_doc) const;

    void granule_search(const std::string &collection_name,
                        const std::string &r_year,
                        const std::string &r_month,
                        const std::string &r_day,
                        nlohmann::json &cmr_doc) const;

    void granule_umm_search(const std::string &collection_name,
                            const std::string &r_year,
                            const std::string &r_month,
                            const std::string &r_day,
                            nlohmann::json &cmr_doc) const;

    void get_collections_worker(const std::string &provider_id,
                                std::map<std::string, std::unique_ptr<cmr::Collection>> &collections,
                                unsigned int page_size=CMR_MAX_PAGE_SIZE,
                                bool just_opendap=false ) const;

public:
    CmrApi();

    const nlohmann::json& get_related_urls_array(const nlohmann::json& go) const;


    void get_years(const std::string &collection_name,
                   std::vector<std::string> &years_result) const;

    void get_months(const std::string &collection_name,
                    const std::string &year,
                    std::vector<std::string> &months_result) const;

    void get_days(const std::string& collection_name,
                  const std::string& r_year,
                  const std::string &r_month,
                  std::vector<std::string> &days_result) const;

    void get_granule_ids(const std::string& collection_name,
                         const std::string& r_year,
                         const std::string &r_month,
                         const std::string &r_day,
                         std::vector<std::string> &granule_ids) const;

    void get_granules(const std::string& collection_name,
                      const std::string &r_year,
                      const std::string &r_month,
                      const std::string &r_day,
                      std::vector<std::unique_ptr<cmr::Granule>> &granule_objs) const;

    void get_granules_umm(const std::string& collection_name,
                              const std::string &r_year,
                              const std::string &r_month,
                              const std::string &r_day,
                              std::vector<std::unique_ptr<cmr::GranuleUMM>> &granule_objs) const;

    void get_collection_ids(std::vector<std::string> &collection_ids) const;

    unsigned long granule_count(const std::string &collection_name,
                                const std::string &r_year,
                                const std::string &r_month,
                                const std::string &r_day) const;


    cmr::Granule *get_granule(const std::string path) const;

    std::unique_ptr<Granule> get_granule( const std::string& collection_name,
                                          const std::string& r_year,
                                          const std::string& r_month,
                                          const std::string& r_day,
                                          const std::string& granule_id) const;

    void get_providers( std::vector<std::unique_ptr<cmr::Provider> > &providers) const;
    void get_opendap_providers(std::map< std::string, std::unique_ptr<cmr::Provider> > &providers) const;
    unsigned long int get_opendap_collections_count(const std::string &provider_id) const;

    void get_collections(const std::string &provider_id, std::map< std::string, std::unique_ptr<cmr::Collection> > &collections ) const;
    void get_opendap_collections(const std::string &provider_id, std::map<std::string, std::unique_ptr<cmr::Collection> > &collections ) const;

};



} // namespace cmr

#endif /* MODULES_CMR_MODULE_CMRAPI_H_ */
