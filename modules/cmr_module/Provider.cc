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


/* Response from CMR ingest API:  https://cmr-host/ingest/providers

[{
    "provider-id": "LARC_ASDC",
    "short-name": "LARC_ASDC",
    "cmr-only": false,
    "small": false,
    "consortiums": "EOSDIS GEOSS"
}, {
    "provider-id": "USGS_EROS",
    "short-name": "USGS_EROS",
    "cmr-only": false,
    "small": false,
    "consortiums": "CEOS CWIC"
}, {
 ...
 }]

 */


/*
   Response from the new Provider api:
   https://cmr.uat.earthdata.nasa.gov/ingest/providers/provider_id

   {
  "MetadataSpecification": {
    "Name": "Provider",
    "Version": "1.0.0",
    "URL": "https://cdn.earthdata.nasa.gov/schemas/provider/v1.0.0"
  },
  "ProviderId": "GES_DISC",
  "DescriptionOfHolding": "None",
  "Organizations": [
    {
      "ShortName": "GES_DISC",
      "LongName": "GES_DISC",
      "URLValue": "https://disc.sci.gsfc.nasa.gov",
      "Roles": [
        "PUBLISHER"
      ]
    }
  ],
  "Administrators": [
    "cloeser1",
    "cdurbin",
    "ECHO_SYS",
    "gesdisc_test",
    "mmorahan",
    "sritz"
  ],
  "ContactPersons": [
    {
      "Roles": [
        "PROVIDER MANAGEMENT"
      ],
      "LastName": "Seiler",
      "FirstName": "Ed",
      "ContactInformation": {
        "Addresses": [
          {
            "City": "Greenbelt",
            "Country": "United States",
            "StateProvince": "MD",
            "PostalCode": "20771",
            "StreetAddresses": [
              "Building 32",
              "Goddard Space Flight Center"
            ]
          }
        ],
        "ContactMechanisms": [
          {
            "Type": "Email",
            "Value": "edward.j.seiler@nasa.gov"
          },
          {
            "Type": "Telephone",
            "Value": "301.614.5486"
          }
        ]
      }
    }
  ],
  "Consortiums": [
    "EOSDIS",
    "GEOSS"
  ]
}
 */


string Provider::id() const{
    JsonUtils json;
    return json.get_str_if_present(CMR_PROVIDER_ID_KEY, d_provider_json_obj);
}

string Provider::description_of_holding() const {
    JsonUtils json;
    return json.get_str_if_present(CMR_DESCRIPTION_OF_HOLDING_KEY, d_provider_json_obj);
}

void Provider::get_collections(std::map<std::string,unique_ptr<cmr::Collection>> &collections) const
{
    CmrApi cmrApi;
    cmrApi.get_collections(id(), collections);
}

void Provider::get_opendap_collections(std::map<std::string, std::unique_ptr<cmr::Collection>> &collections) const
{
    CmrApi cmrApi;
    cmrApi.get_opendap_collections(id(), collections);

}



string Provider::to_string(bool show_json) const {
    stringstream msg;
    msg << "# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #" << endl;
    msg << "# Provider" << endl;
    msg << "#              provider_id: " << id() << endl;
//    msg << "#        organization_name: " << organization_long_name() << endl;
    msg << "#  description_of_holding: " << description_of_holding() << endl;
//    msg << "#                 contacts: " << contacts().dump() << endl;
//    msg << "#                rest_only: " << (rest_only()?"true":"false") << endl;
    msg << "# OPeNDAP collection count: " << d_opendap_collection_count << endl;
    msg << "#" << endl;
    if(show_json) {
        msg << "# json: " << endl;
        msg << d_provider_json_obj.dump(4) << endl;
        msg << "#" << endl;
    }
    msg << "#" << endl;
    return msg.str();
}

} // cmr