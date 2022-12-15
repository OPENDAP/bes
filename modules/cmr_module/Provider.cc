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

#include "JsonUtils.h"

#include "CmrNames.h"
#include "CmrApi.h"
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


string Provider::id() const{
    JsonUtils json;
    return json.get_str_if_present(CMR_PROVIDER_ID_KEY,d_provider_json_obj);
}

string Provider::description_of_holdings() const {
    JsonUtils json;
    return json.get_str_if_present(CMR_PROVIDER_DESCRIPTION_OF_HOLDINGS_KEY,d_provider_json_obj);
}

string Provider::organization_name() const {
    JsonUtils json;
    return json.get_str_if_present(CMR_PROVIDER_ORGANIZATION_NAME_KEY,d_provider_json_obj);
}

json Provider::contacts() const {
    JsonUtils json;
    return json.get_str_if_present(CMR_PROVIDER_CONTACTS_KEY,d_provider_json_obj);
}

bool Provider::rest_only() const {
    JsonUtils json;
    return json.qc_boolean(CMR_PROVIDER_REST_ONLY_KEY,d_provider_json_obj);
}
void Provider::get_collections(std::vector<unique_ptr<cmr::Collection>> &collections)
{
    CmrApi cmrApi;
    cmrApi.get_collections(id(), collections);
}

void Provider::get_opendap_collections(std::vector<unique_ptr<cmr::Collection>> &collections) const
{
    CmrApi cmrApi;
    cmrApi.get_opendap_collections(id(), collections);

}



string Provider::to_string() {
    stringstream msg;
    msg << "# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #" << endl;
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