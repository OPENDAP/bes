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


/*
 * NgapApi.h
 *
 *  Created on: July, 13 2018
 *      Author: ndp
 */

#ifndef MODULES_NGAP_MODULE_NGAPAPI_H_
#define MODULES_NGAP_MODULE_NGAPAPI_H_

#include <string>
#include <vector>
#include <map>
#include "rapidjson/document.h"
#include "BESCatalogUtils.h"
#include "url_impl.h"

namespace ngap {

class NgapApi {
private:
    std::string d_cmr_hostname;
    std::string d_cmr_search_endpoint_path;

    std::string get_cmr_search_endpoint_url();
    std::string find_get_data_url_in_granules_umm_json_v1_4(const std::string &restified_path, rapidjson::Document &cmr_granule_response);
    std::string build_cmr_query_url(const std::string &restified_path);
    std::string build_cmr_query_url_old_rpath_format(const std::string &restified_path);

    friend class NgapApiTest;

public:

    NgapApi();

    std::string convert_ngap_resty_path_to_data_access_url(
            const std::string &restified_path,
            const std::string &uid="");

    static bool signed_url_is_expired(const http::url &signed_url) ;

#if 0
    void get_years(std::string collection_name, std::vector<std::string> &years_result);
    void get_months(std::string collection_name, std::string year, std::vector<std::string> &months_result);
    void get_days(std::string collection_name, std::string r_year, std::string r_month, std::vector<std::string> &days_result);
    void get_granule_ids(std::string collection_name, std::string r_year, std::string r_month, std::string r_day, std::vector<std::string> &granules_result);
    void get_granule_ids(std::string collection_name, std::string r_year, std::string r_month, std::vector<std::string> &granules_result);
    void get_granules(std::string collection_name, std::string r_year, std::string r_month, std::string r_day, std::vector<ngap::Granule *> &granules);
    void get_collection_ids(std::vector<std::string> &collection_ids);
    unsigned long granule_count(std::string collection_name,std:: string r_year, std::string r_month, std::string r_day);
    ngap::Granule *get_granule(const std::string path);
    ngap::Granule *get_granule(std::string collection_name, std::string r_year, std::string r_month, std::string r_day, std::string granule_id);
};
#endif
};

} // namespace ngap

#endif /* MODULES_NGAP_MODULE_NGAPAPI_H_ */
