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
#include <algorithm>
#include <string>



#include "nlohmann/json.hpp"
#include "rjson_utils.h"


#include "BESDebug.h"
#include "BESUtil.h"

#include "CmrNames.h"
#include "CmrApi.h"
#include "CmrInternalError.h"
#include "CmrNotFoundError.h"
#include "GranuleUMM.h"


using namespace std;

#define prolog std::string("GranuleUMM::").append(__func__).append("() - ")


namespace cmr {


GranuleUMM::GranuleUMM(const nlohmann::json& granule_json)
{
    BESDEBUG(MODULE, prolog << "BEGIN" << endl << granule_json.dump(2) << endl);
    setConceptId(granule_json);
    setName(granule_json);
    setSize(granule_json);
    setDapServiceUrl(granule_json);
    setDataGranuleUrl(granule_json);
    setLastModifiedStr(granule_json);
    setDescription(granule_json);
}


void GranuleUMM::setName(const nlohmann::json& j_obj) {
    CmrApi cmrApi;
    const auto &umm_obj = cmrApi.qc_get_object(CMR_UMM_UMM_KEY, j_obj);
    this->d_name = cmrApi.get_str_if_present(CMR_UMM_GRANULE_UR_KEY, umm_obj);
}

void GranuleUMM::setDescription(const nlohmann::json& go){
}

void GranuleUMM::setConceptId(const nlohmann::json& j_obj)
{
    CmrApi cmrApi;
    const auto &meta_obj = cmrApi.qc_get_object(CMR_UMM_META_KEY, j_obj);

    BESDEBUG(MODULE, prolog << "META OBJECT" << endl << cmrApi.probe_json(meta_obj) << endl);

    this->d_id = cmrApi.get_str_if_present(CMR_UMM_CONCEPT_ID_KEY, meta_obj);
}


void GranuleUMM::setSize(const nlohmann::json& granule_obj)
{
    CmrApi cmrApi;
    const auto &umm_obj = cmrApi.qc_get_object(CMR_UMM_UMM_KEY, granule_obj);
    const auto &data_granule_obj = cmrApi.qc_get_object(CMR_UMM_DATA_GRANULE_KEY, umm_obj);
    BESDEBUG(MODULE, prolog << CMR_UMM_DATA_GRANULE_KEY << data_granule_obj.dump(2) << endl );
    const auto &arch_and_info_array = cmrApi.qc_get_array(CMR_UMM_ARCHIVE_AND_DIST_INFO_KEY, data_granule_obj);
    for(const auto &entry : arch_and_info_array){
        BESDEBUG(MODULE, prolog << CMR_UMM_ARCHIVE_AND_DIST_INFO_KEY << entry.dump(2) << endl );
        d_size_orig = cmrApi.qc_double(CMR_UMM_SIZE_KEY, entry);

        d_size_units_str = cmrApi.get_str_if_present(CMR_UMM_SIZE_UNIT_KEY, entry).c_str();
        std::transform(d_size_units_str.begin(), d_size_units_str.end(),d_size_units_str.begin(), ::toupper);

        if(d_size_units_str.empty()){
            BESDEBUG(MODULE, prolog << "Size content is incomplete. size: " << d_size_str << " units: " << d_size_units_str << endl );
            return;
        }
        BESDEBUG(MODULE, prolog << "d_size_orig: " << d_size_orig << endl );

        double size;
        size = d_size_orig;

        if(d_size_units_str == "KB"){
            size *= 1024;
        }
        if(d_size_units_str == "MB"){
            size *= 1024*1024;
        }
        else if (d_size_units_str=="GB"){
            size *= 1024*1024*1024;
        }
        else if (d_size_units_str=="TB"){
            size *= 1024*1024*1024*1024;
        }
        d_size = size;
        BESDEBUG(MODULE, prolog << "d_size: " << d_size << endl );

        // We just look at the first entry in the arch_and_info_array. What does more than a single entry mean?
        return ;
    }
}


/**
  * Sets the last modified time of the granule as a string.
  * @param go
  */
void GranuleUMM::setLastModifiedStr(const nlohmann::json& j_obj)
{
    CmrApi cmrApi;
    const auto &umm_obj = cmrApi.qc_get_object(CMR_UMM_META_KEY, j_obj);
    this->d_last_modified_time = cmrApi.get_str_if_present(CMR_UMM_REVISION_DATE_KEY, umm_obj);
}


/**
 * Sets the data access URL for the dataset granule.
 */
void GranuleUMM::setDataGranuleUrl(const nlohmann::json& go)
{
    CmrApi cmrApi;
    const auto& umm_obj = cmrApi.qc_get_object(CMR_UMM_UMM_KEY, go);
    const auto& related_urls = cmrApi.qc_get_array(CMR_UMM_RELATED_URLS_KEY, umm_obj);
    for(auto &url_obj : related_urls){
        string url = cmrApi.get_str_if_present(CMR_UMM_URL_KEY, url_obj);
        string type = cmrApi.get_str_if_present(CMR_UMM_TYPE_KEY, url_obj);
        if(type == CMR_UMM_TYPE_GET_DATA_VALUE){
            this->d_data_access_url = url;
            return;
        }
    }
    stringstream msg;
    msg << "ERROR: Failed to locate Data Granule URL (";
    msg << CMR_UMM_RELATED_URLS_KEY << "). json: " << endl << related_urls.dump(2) << endl;
    BESDEBUG(MODULE, prolog << msg.str() << endl);
    // throw CmrNotFoundError(msg.str(), __FILE__, __LINE__);
}
/**
 * Sets the DAP Service URL for the dataset granule.
 *  Some JSON:
   {
      "Description": "OPeNDAP request URL",
      "Subtype": "OPENDAP DATA",
      "Type": "USE SERVICE API",
      "URL": "https://opendap.earthdata.nasa.gov/collections/C2205121485-POCLOUD/granules/cyg.ddmi.s20211111-000000-e20211111-235959.l2.wind-mss-cdr.a11.d11"
    },
    {
      "Description": "OPeNDAP request URL",
      "Subtype": "OPENDAP DATA",
      "Type": "USE SERVICE API",
      "URL": "https://opendap.earthdata.nasa.gov/collections/C2205121485-POCLOUD/granules/cyg.ddmi.s20211111-000000-e20211111-235959.l2.wind-mss-cdr.a11.d11"
    },
 *
 *
 * @param jo
 */
void GranuleUMM::setDapServiceUrl(const nlohmann::json& jo)
{
    const std::string HTML_SUFFIX(".html");

    CmrApi cmrApi;
    const auto& umm_obj = cmrApi.qc_get_object(CMR_UMM_UMM_KEY, jo);
    const auto& related_urls = cmrApi.qc_get_array(CMR_UMM_RELATED_URLS_KEY, umm_obj);
    for(auto &related_url_obj : related_urls){
        string url = cmrApi.get_str_if_present(CMR_UMM_URL_KEY,related_url_obj);
        string type = cmrApi.get_str_if_present(CMR_UMM_TYPE_KEY,related_url_obj);
        string subtype = cmrApi.get_str_if_present(CMR_UMM_SUBTYPE_KEY,related_url_obj);
        if(type == CMR_UMM_TYPE_USE_SERVICE_API_VALUE || type == CMR_UMM_TYPE_GET_DATA_VALUE){
            if(subtype == CMR_UMM_SUBTYPE_KEY_OPENDAP_DATA_VALUE){

                if(BESUtil::endsWith(url, HTML_SUFFIX)){
                    url = url.substr(0,url.length() - HTML_SUFFIX.length());
                }
                this->d_dap_service_url = url;
                return;
            }
        }
    }
    stringstream msg;
    msg << "ERROR: Failed to locate DAP service URL (";
    msg << CMR_UMM_RELATED_URLS_KEY << "). json: " << endl << related_urls.dump(2) << endl;
    BESDEBUG(MODULE, prolog << msg.str() << endl);
    // throw CmrNotFoundError(msg.str(), __FILE__, __LINE__);
}

/**
 *
 * @param d_catalog_utils
 * @return
 */
bes::CatalogItem *GranuleUMM::getCatalogItem(BESCatalogUtils *d_catalog_utils){
    auto *item = new bes::CatalogItem();
    item->set_type(bes::CatalogItem::leaf);
    item->set_name(getName());
    item->set_lmt(getLastModifiedStr());
    item->set_size(getSize());
    item->set_description(getDescription());

    // item->set_is_data(d_catalog_utils->is_data(item->get_name()));
    if(!getDapServiceUrl().empty()) {
        item->set_dap_service_url(getDapServiceUrl());
    }
    bool is_data = d_catalog_utils->is_data(item->get_name()) || !getDapServiceUrl().empty();
    item->set_is_data(is_data);

    return item;
}




} //namespace cmr
