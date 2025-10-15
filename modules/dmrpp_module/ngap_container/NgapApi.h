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
#include <tuple>

#include "rapidjson/document.h"

namespace http {
class url;
}

namespace ngap {

/**
 * @class NgapApi
 * @brief This class provides an API interface for handling NGAP (NASA General Application Platform) procedures.
 * @note This would be better implemented as a set of functions in a namespace. jhrg 10/8/25
 */
class NgapApi {
    static bool append_hyrax_edl_client_id(std::string &cmr_url);

    static std::string get_cmr_search_endpoint_url();

    typedef std::tuple<std::string, std::string, std::string> DataAccessUrls;
    static DataAccessUrls get_urls_from_granules_umm_json_v1_4(const std::string &rest_path, rapidjson::Document &cmr_granule_response);
    static std::string build_cmr_query_url(const std::string &restified_path);
    static std::string build_cmr_query_url_old_rpath_format(const std::string &restified_path);

    friend class NgapApiTest;

public:
    NgapApi() = default;
    ~NgapApi() = default;
    NgapApi(const NgapApi &other) = delete;
    NgapApi &operator=(const NgapApi &other) = delete;

    static DataAccessUrls convert_ngap_resty_path_to_data_access_urls(const std::string &restified_path);
};

} // namespace ngap

#endif /* MODULES_NGAP_MODULE_NGAPAPI_H_ */
