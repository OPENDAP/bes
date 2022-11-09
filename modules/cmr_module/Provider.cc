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

#include "config.h"

#include <utility>
#include <sstream>
#include "nlohmann/json.hpp"

#include "rjson_utils.h"

#include "CmrNames.h"
#include "Provider.h"

using namespace std;
using json = nlohmann::json;

#define prolog string("Provider::").append(__func__).append("() - ")

namespace cmr {

/*
   {
    "provider": {
      "contacts": [
        {
          "email": "michael.p.morahan@nasa.gov",
          "first_name": "Michael",
          "last_name": "Morahan",
          "phones": [],
          "role": "Default Contact"
        }
      ],
      "description_of_holdings": "EO European and Canadian missions Earth Science Data",
      "discovery_urls": [
        "https://fedeo-client.ceos.org/about/"
      ],
      "id": "E37E931C-A94A-3F3C-8FA1-206EB96B465C",
      "organization_name": "Federated EO missions support environment",
      "provider_id": "FEDEO",
      "provider_types": [
        "CMR"
      ],
      "rest_only": true
    }
 */

/*
    {
        "email": "michael.p.morahan@nasa.gov",
        "first_name": "Michael",
        "last_name": "Morahan",
        "phones": [],
        "role": "Default Contact"
    }

 */


string Provider::id(){
    return d_provider_json_obj[CMR_PROVIDER_ID_KEY].get<std::string>();
}

string Provider::description_of_holdings() {
    return d_provider_json_obj[CMR_PROVIDER_DESCRIPTION_OF_HOLDINGS_KEY].get<std::string>();
}

string Provider::organization_name() {
    return d_provider_json_obj[CMR_PROVIDER_ORGANIZATION_NAME_KEY].get<std::string>();
}

json Provider::contacts() {
    return d_provider_json_obj[CMR_PROVIDER_CONTACTS_KEY];
}

bool Provider::rest_only() {
    return d_provider_json_obj[CMR_PROVIDER_REST_ONLY_KEY];
}

string Provider::to_string() {
    stringstream msg;
    msg << "# # # # # # # # # # # # # # # # # # " << endl;
    msg << "# Provider" << endl;
    msg << "#              provider_id: " << id() << endl;
    msg << "#        organization_name: " << organization_name() << endl;
    msg << "#  description_of_holdings: " << description_of_holdings() << endl;
    msg << "#                 contacts: " << contacts().dump() << endl;
    msg << "#                rest_only: " << (rest_only()?"true":"false") << endl;
    msg << "# OPeNDAP collection count: " << d_opendap_collection_count << endl;
    msg << "#" << endl;
    return msg.str();
}

} // cmr