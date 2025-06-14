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

namespace http {
class url;
}

namespace ngap {

class NgapApi {
private:
    static std::string get_cmr_search_endpoint_url();
    static std::string find_get_data_url_in_granules_umm_json_v1_4(const std::string &rest_path,
                                                                   rapidjson::Document &cmr_granule_response);
    static std::string build_cmr_query_url(const std::string &restified_path);
    static std::string build_cmr_query_url_old_rpath_format(const std::string &restified_path);

    friend class NgapApiTest;

public:
    NgapApi() = default;
    ~NgapApi() = default;
    NgapApi(const NgapApi &other) = delete;
    NgapApi &operator=(const NgapApi &other) = delete;

    static std::string convert_ngap_resty_path_to_data_access_url(const std::string &restified_path);

#if 0
      static bool signed_url_is_expired(const http::url &signed_url) ;
#endif
};

} // namespace ngap

#endif /* MODULES_NGAP_MODULE_NGAPAPI_H_ */
