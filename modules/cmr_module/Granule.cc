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
 *  Created on: July, 13 2018
 *      Author: ndp
 */
#include "config.h"

#include <cstdlib>     /* atol */
#include <sstream>

#include "rjson_utils.h"
#include "BESDebug.h"

#include "CmrNames.h"
#include "CmrInternalError.h"
#include "CmrNotFoundError.h"
#include "Granule.h"
#include "CmrApi.h"


using namespace std;

#define prolog std::string("Granule::").append(__func__).append("() - ")


namespace cmr {
/**
 * granule.umm_json
{
  "hits": 1,
  "took": 399,
  "items": [
    {
      "meta": {
        "concept-type": "granule",
        "concept-id": "G1216079418-GES_DISC",
        "revision-id": 1,
        "native-id": "AIRH2SUP.006:AIRS.2002.11.16.240.L2.RetSup_H.v6.0.12.0.G14105054018.hdf",
        "provider-id": "GES_DISC",
        "format": "application/echo10+xml",
        "revision-date": "2016-05-05T14:45:25.846Z"
      },
      "umm": {
        "TemporalExtent": {
          "RangeDateTime": {
            "BeginningDateTime": "2002-11-16T23:59:26.000Z",
            "EndingDateTime": "2002-11-17T00:05:26.000Z"
          }
        },
        "GranuleUR": "AIRH2SUP.006:AIRS.2002.11.16.240.L2.RetSup_H.v6.0.12.0.G14105054018.hdf",
        "SpatialExtent": {
          "HorizontalSpatialDomain": {
            "Geometry": {
              "BoundingRectangles": [
                {
                  "WestBoundingCoordinate": -163.196578979492,
                  "EastBoundingCoordinate": -139.618240356445,
                  "NorthBoundingCoordinate": -18.9448909759521,
                  "SouthBoundingCoordinate": -42.5188293457031
                }
              ]
            }
          }
        },
        "ProviderDates": [
          {
            "Date": "2016-05-05T13:58:30.000Z",
            "Type": "Insert"
          },
          {
            "Date": "2016-05-05T13:58:30.000Z",
            "Type": "Update"
          }
        ],
        "CollectionReference": {
          "ShortName": "AIRH2SUP",
          "Version": "006"
        },
        "PGEVersionClass": {
          "PGEVersion": "6.0.12.0"
        },
        "RelatedUrls": [
          {
            "URL": "http://discnrt1.gesdisc.eosdis.nasa.gov/data/Aqua_AIRS_Level2/AIRH2SUP.006/2002/320/AIRS.2002.11.16.240.L2.RetSup_H.v6.0.12.0.G14105054018.hdf",
            "Type": "GET DATA"
          }
        ],
        "DataGranule": {
          "DayNightFlag": "Day",
          "Identifiers": [
            {
              "Identifier": "AIRS.2002.11.16.240.L2.RetSup_H.v6.0.12.0.G14105054018.hdf",
              "IdentifierType": "ProducerGranuleId"
            }
          ],
          "ProductionDateTime": "2014-04-15T09:40:21.000Z",
          "ArchiveAndDistributionInformation": [
            {
              "Name": "Not provided",
              "Size": 21.0902881622314,
              "SizeUnit": "MB"
            }
          ]
        },
        "MetadataSpecification": {
          "URL": "https://cdn.earthdata.nasa.gov/umm/granule/v1.6.4",
          "Name": "UMM-G",
          "Version": "1.6.4"
        }
      }
    }
  ]
}

 */
/** Builds Granule from granule.umm_json response from CMR.
 *
 * @param granule_json
 */
Granule::Granule(const nlohmann::json& granule_json)
{
    setId(granule_json);
    setName(granule_json);
    setSize(granule_json);
    setDapServiceUrl(granule_json);
    setDataGranuleUrl(granule_json);
    setMetadataAccessUrl(granule_json);
    setLastModifiedStr(granule_json);
}


void Granule::setName(const nlohmann::json& granule_json)
{
    JsonUtils json;
    d_name = json.get_str_if_present(CMR_V2_TITLE_KEY, granule_json);
}


void Granule::setId(const nlohmann::json& granule_json)
{
    JsonUtils json;
    d_id = json.get_str_if_present(CMR_GRANULE_ID_KEY, granule_json);
}


void Granule::setSize(const nlohmann::json& granule_json)
{
    JsonUtils json;
    d_size_str = json.get_str_if_present(CMR_GRANULE_SIZE_KEY, granule_json);
}


/**
  * Sets the last modified time of the granule as a string.
  * @param go
  */
void Granule::setLastModifiedStr(const nlohmann::json& granule_json)
{
    JsonUtils json;
    d_last_modified_time = json.get_str_if_present(CMR_GRANULE_LMT_KEY, granule_json);
}


/**
 * Internal method that retrieves the "links" array from the Granule's object.
 */
const nlohmann::json& Granule::get_links_array(const nlohmann::json& granule_json) const
{
    JsonUtils json;
   return json.qc_get_array(CMR_GRANULE_LINKS_KEY, granule_json);
}



/**
 * Sets the data access URL for the dataset granule.
 */
void Granule::setDataGranuleUrl(const nlohmann::json& go)
{
    const auto& links = get_links_array(go);
    for(auto &link : links){
        string rel = link[CMR_GRANULE_LINKS_REL].get<string>();
        if(rel == CMR_GRANULE_LINKS_REL_DATA_ACCESS){
            d_data_access_url = link[CMR_GRANULE_LINKS_HREF];
            return;
        }
    }
    stringstream msg;
    msg << "ERROR: Failed to locate granule data access link (";
    msg << CMR_GRANULE_LINKS_REL_DATA_ACCESS << "), :(";
    throw CmrInternalError(msg.str(), __FILE__, __LINE__);
}

/**
 * Sets the data access URL for the dataset granule.
 */
void Granule::setDapServiceUrl(const nlohmann::json& jo)
{
    BESDEBUG(MODULE, prolog << "JSON: " << endl << jo.dump(4) << endl);
    const auto& links = get_links_array(jo);
    for(auto &link : links){
        string rel = link[CMR_GRANULE_LINKS_REL].get<string>();
        if (rel == CMR_GRANULE_LINKS_REL_SERVICE) {
            d_dap_service_url = link[CMR_GRANULE_LINKS_HREF];
            const auto &title_itr = link.find(CMR_V2_TITLE_KEY);
            if(title_itr != link.end()){
                string title = title_itr.value().get<string>();
                transform(title.begin(), title.end(), title.begin(), ::toupper);
                if (title.find("OPENDAP") != string::npos) {
                    d_dap_service_url = link[CMR_GRANULE_LINKS_HREF];
                    return;
                }
            }
        }
    }
    stringstream msg;
    msg << "Failed to locate DAP service link (";
    msg << CMR_GRANULE_LINKS_REL_SERVICE << "), :(";
    BESDEBUG(MODULE, prolog << msg.str() << endl);
}


/**
 * Sets the metadata access URL for the dataset granule.
 */
void Granule::setMetadataAccessUrl(const nlohmann::json& go)
{
    const auto &links = get_links_array(go);
    for(auto &link : links){
        string rel = link[CMR_GRANULE_LINKS_REL].get<string>();
        if(rel == CMR_GRANULE_LINKS_REL_METADATA_ACCESS){
            d_metadata_access_url = link[CMR_GRANULE_LINKS_HREF].get<string>();
            return;
        }
    }
    stringstream msg;
    msg << "ERROR: Failed to locate granule metadata access link (";
    msg << CMR_GRANULE_LINKS_REL_METADATA_ACCESS << "), :(";
    throw CmrInternalError(msg.str(), __FILE__, __LINE__);
}




bes::CatalogItem *Granule::getCatalogItem(const BESCatalogUtils *d_catalog_utils) const
{
    auto *item = new bes::CatalogItem();
    item->set_type(bes::CatalogItem::leaf);
    item->set_name(getName());
    item->set_lmt(getLastModifiedStr());
    item->set_size(getSize());
    item->set_is_data(d_catalog_utils->is_data(item->get_name()));
    if(!getDapServiceUrl().empty()) {
        item->set_dap_service_url(getDapServiceUrl());
    }

    return item;
}



} //namespace cmr
