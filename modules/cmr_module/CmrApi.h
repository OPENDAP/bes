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
#include "rapidjson/document.h"
#include "BESCatalogUtils.h"

#include "Granule.h"

namespace cmr {


class CmrApi {
private:
    std::string cmr_search_endpoint_url;

    const rapidjson::Value& get_temporal_group(const rapidjson::Document &cmr_doc);
    const rapidjson::Value& get_year_group(const rapidjson::Document &cmr_doc);
    const rapidjson::Value& get_month_group(const string year, const rapidjson::Document &cmr_doc);
    const rapidjson::Value& get_month(const string r_month, const string r_year, const rapidjson::Document &cmr_doc);
    const rapidjson::Value& get_day_group(const string r_month, const string year, const rapidjson::Document &cmr_doc);
    const rapidjson::Value& get_children(const rapidjson::Value& obj);
    const rapidjson::Value& get_feed(const rapidjson::Document &cmr_doc);
    const rapidjson::Value& get_entries(const rapidjson::Document &cmr_doc);
    void  granule_search(string collection_name, string r_year, string r_month, string r_day,rapidjson::Document &result_doc);


public:
    CmrApi() : cmr_search_endpoint_url("https://cmr.earthdata.nasa.gov/search") {}

    void get_years(std::string collection_name, std::vector<std::string> &years_result);
    void get_months(std::string collection_name, std::string year, std::vector<std::string> &months_result);
    void get_days(std::string collection_name, std::string r_year, std::string r_month, std::vector<std::string> &days_result);
    void get_granule_ids(std::string collection_name, std::string r_year, std::string r_month, std::string r_day, std::vector<std::string> &granules_result);
    void get_granule_ids(std::string collection_name, std::string r_year, std::string r_month, std::vector<std::string> &granules_result);
    void get_granules(std::string collection_name, string r_year, string r_month, string r_day, std::vector<cmr::Granule *> &granules);
    void get_collection_ids(std::vector<std::string> &collection_ids);
    unsigned long granule_count(std::string collection_name,std:: string r_year, std::string r_month, std::string r_day);
    cmr::Granule *get_granule(const std::string path);
};



} // namespace cmr

#endif /* MODULES_CMR_MODULE_CMRAPI_H_ */
