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

#include <sstream>
#include <algorithm>
#include <string>

#include "nlohmann/json.hpp"

#include "BESDebug.h"
#include "BESUtil.h"

#include "CmrNames.h"
#include "CmrApi.h"
#include "CmrInternalError.h"
#include "GranuleUMM.h"
#include "JsonUtils.h"


using namespace std;

#define prolog std::string("GranuleUMM::").append(__func__).append("() - ")


namespace cmr {

/**
 *
 * @param granule_umm_json The CMR
 */
GranuleUMM::GranuleUMM(const nlohmann::json& granule_umm_json)
{
    BESDEBUG(MODULE, prolog << "BEGIN" << endl << granule_umm_json.dump(2) << endl);
    setConceptId(granule_umm_json);
    setName(granule_umm_json);
    setSize(granule_umm_json);
    setDapServiceUrl(granule_umm_json);
    setDataGranuleUrl(granule_umm_json);
    setLastModifiedStr(granule_umm_json);
    setDescription(granule_umm_json);
}


void GranuleUMM::setName(const nlohmann::json& granule_umm_json)
{
    JsonUtils json;
    const auto &umm_obj = json.qc_get_object(CMR_UMM_UMM_KEY, granule_umm_json);
    this->d_name = json.get_str_if_present(CMR_UMM_GRANULE_UR_KEY, umm_obj);
}

/**
 * Not implemented because the .umm_json response does not contain an obvious candidate for Description.
 * @param go
 */
void GranuleUMM::setDescription(const nlohmann::json& /*granule_umm_json*/)
{
    // Not implemented because the .umm_json response does not contain an obvious candidate for Description.
}

void GranuleUMM::setConceptId(const nlohmann::json& granule_umm_json)
{
    JsonUtils json;
    const auto &meta_obj = json.qc_get_object(CMR_UMM_META_KEY, granule_umm_json);

    BESDEBUG(MODULE, prolog << "META OBJECT" << endl << json.probe_json(meta_obj) << endl);

    this->d_id = json.get_str_if_present(CMR_UMM_CONCEPT_ID_KEY, meta_obj);
}


/**
 * @brief Sets the granule size by insoecting the passed umm_json granule object.
 *
 * Try to locate the granule size from the granule.umm_json response. This is made challenging by how the
 * records are stored/organized in the reponse, or if objects that we expect to be available even are so.
 *
 * @param granule_obj
 */
void GranuleUMM::setSize(const nlohmann::json& granule_umm_json)
{
    JsonUtils json;
    const auto &umm_obj = json.qc_get_object(CMR_UMM_UMM_KEY, granule_umm_json);
    const auto &data_granule_obj = json.qc_get_object(CMR_UMM_DATA_GRANULE_KEY, umm_obj);
    BESDEBUG(MODULE, prolog << CMR_UMM_DATA_GRANULE_KEY << data_granule_obj.dump(2) << endl );
    const auto &arch_and_info_array = json.qc_get_array(CMR_UMM_ARCHIVE_AND_DIST_INFO_KEY, data_granule_obj);
    //
    // At this point we just look at the first entry in the arch_and_info_array.
    // What does more than a single entry mean? It means that using this method is not deterministic.
    // Consider this relevant JSON fragment:
    //
    //       "DataGranule" : {
    //        "ArchiveAndDistributionInformation" : [ {
    //          "SizeUnit" : "MB",
    //          "Size" : 29.323293685913086,
    //          "Checksum" : {
    //            "Value" : "b807626928f3176bd969664090ad4b05",
    //            "Algorithm" : "MD5"
    //          },
    //          "Name" : "saildrone-gen_4-baja_2018-sd1002-20180411T180000-20180611T055959-1_minutes-v1.nc"
    //        }, {
    //          "SizeUnit" : "MB",
    //          "Size" : 1.0967254638671875E-4,
    //          "Checksum" : {
    //            "Value" : "f617b33fb4ace5c26b044a418d22fcbf",
    //            "Algorithm" : "MD5"
    //          },
    //          "Name" : "saildrone-gen_4-baja_2018-sd1002-20180411T180000-20180611T055959-1_minutes-v1.nc.md5"
    //        } ],
    //        "DayNightFlag" : "Unspecified",
    //        "ProductionDateTime" : "2018-08-29T21:02:49.000Z"
    //      },
    //
    // This fragment contains two entries in the ArchiveAndDistributionInformation array. One entry for the native
    // netcdf file, and the other entry for its MD5 checksum. Assuming that the order is not fixed in some way
    // We would be lucky to get the right size for the granule from the first element. Nothing in this to
    // distinguish one entry as the correct one other than the name ending in .nc vs .nc.md5. That works for this
    // example, but I doubt it works for hdf5 files (.h5) apand I doubt there semantic constraints on the value of
    // Name in the ArchiveAndDistributionInformation.
    //
    for(const auto &entry : arch_and_info_array){
        BESDEBUG(MODULE, prolog << CMR_UMM_ARCHIVE_AND_DIST_INFO_KEY << entry.dump(2) << endl );
        string name = json.get_str_if_present(CMR_UMM_NAME_KEY,entry);
        BESDEBUG(MODULE, prolog << CMR_UMM_NAME_KEY << ": " << name << endl );

        // We want the granule and not its md5 hash, so we check for that.
        // There may be other entries in the array but *shrugs* what's a person to do?
        if(BESUtil::endsWith(name,".md5")) {
            BESDEBUG(MODULE, prolog << "Detected MD5 hash file: " << name << " SKIPPING." << endl);
        }
        else {
            d_size_orig = json.qc_double(CMR_UMM_SIZE_KEY, entry);
            BESDEBUG(MODULE, prolog << "d_size_orig: " << d_size_orig << endl );

            d_size_units_str = json.get_str_if_present(CMR_UMM_SIZE_UNIT_KEY, entry).c_str();
            std::transform(d_size_units_str.begin(), d_size_units_str.end(),d_size_units_str.begin(), ::toupper);
            BESDEBUG(MODULE, prolog << "d_size_units_str: " << d_size_units_str << endl );

            if(d_size_units_str.empty()){
                BESDEBUG(MODULE, prolog << "Size content is incomplete. size: " << d_size_str << " units: " << d_size_units_str << endl );
                return;
            }

            double size;
            size = d_size_orig;

            if(d_size_units_str == "KB"){
                size *= 1024;
            }
            if(d_size_units_str == "MB"){
                size *= 1024ULL*1024ULL;
            }
            else if (d_size_units_str=="GB"){
                size *= 1024ULL*1024ULL*1024ULL;
            }
            else if (d_size_units_str=="TB"){
                size *= 1024ULL*1024ULL*1024ULL*1024ULL;
            }
            d_size =  static_cast<uint64_t>(size);

            BESDEBUG(MODULE, prolog << "d_size: " << d_size << " bytes" << endl );
            break;
        }
    }
}


/**
  * Sets the last modified time of the granule as a string.
  * @param go
  */
void GranuleUMM::setLastModifiedStr(const nlohmann::json& granule_umm_json)
{
    JsonUtils json;
    const auto &umm_obj = json.qc_get_object(CMR_UMM_META_KEY, granule_umm_json);
    this->d_last_modified_time = json.get_str_if_present(CMR_UMM_REVISION_DATE_KEY, umm_obj);
}


/**
 * Sets the data access URL for the dataset granule.
 */
void GranuleUMM::setDataGranuleUrl(const nlohmann::json& granule_umm_json)
{
    JsonUtils json;
    const auto& umm_obj = json.qc_get_object(CMR_UMM_UMM_KEY, granule_umm_json);
    const auto& related_urls = json.qc_get_array(CMR_UMM_RELATED_URLS_KEY, umm_obj);
    for(auto &url_obj : related_urls){
        string url = json.get_str_if_present(CMR_UMM_URL_KEY, url_obj);
        string type = json.get_str_if_present(CMR_UMM_TYPE_KEY, url_obj);
        if(type == CMR_UMM_TYPE_GET_DATA_VALUE){
            this->d_data_access_url = url;
            return;
        }
    }
    stringstream msg;
    msg << "ERROR: Failed to locate Data Granule URL (";
    msg << CMR_UMM_RELATED_URLS_KEY << "). json: " << endl << related_urls.dump(2) << endl;
    BESDEBUG(MODULE, prolog << msg.str() << endl);
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
void GranuleUMM::setDapServiceUrl(const nlohmann::json& granule_umm_json)
{
    const std::string DAP2_HTML_SUFFIX(".html");
    const std::string DAP4_HTML_SUFFIX(".dmr.html");

    JsonUtils json;
    const auto& umm_obj = json.qc_get_object(CMR_UMM_UMM_KEY, granule_umm_json);
    const auto& related_urls = json.qc_get_array(CMR_UMM_RELATED_URLS_KEY, umm_obj);
    for(auto &related_url_obj : related_urls){
        string url = json.get_str_if_present(CMR_UMM_URL_KEY,related_url_obj);
        string type = json.get_str_if_present(CMR_UMM_TYPE_KEY,related_url_obj);
        string subtype = json.get_str_if_present(CMR_UMM_SUBTYPE_KEY,related_url_obj);
        bool is_dap_service = ((type == CMR_UMM_TYPE_USE_SERVICE_API_VALUE || type == CMR_UMM_TYPE_GET_DATA_VALUE)
                                && subtype == CMR_UMM_SUBTYPE_KEY_OPENDAP_DATA_VALUE);
        if(is_dap_service){
            // This next is a hack for bad CMR records in which the URL incorrectly references the DAP2 or
            // DAP4 Data Request form and not the unadorned Dataset URL.
            if(BESUtil::endsWith(url, DAP2_HTML_SUFFIX)){
                url = url.substr(0,url.length() - DAP2_HTML_SUFFIX.length());
            }
            else if(BESUtil::endsWith(url, DAP4_HTML_SUFFIX)){
                url = url.substr(0,url.length() - DAP4_HTML_SUFFIX.length());
            }
            d_dap_service_url = url;
            return;
        }
    }
    stringstream msg;
    msg << "ERROR: Failed to locate DAP service URL (";
    msg << CMR_UMM_RELATED_URLS_KEY << "). json: " << endl << related_urls.dump(2) << endl;
    BESDEBUG(MODULE, prolog << msg.str() << endl);
}

/**
 *
 * @param d_catalog_utils
 * @return
 */
bes::CatalogItem *GranuleUMM::getCatalogItem(BESCatalogUtils *d_catalog_utils)
{
    auto *item = new bes::CatalogItem();
    item->set_type(bes::CatalogItem::leaf);
    item->set_name(getName());
    item->set_lmt(getLastModifiedStr());
    item->set_size(getSize());
    item->set_description(getDescription());

    if(!getDapServiceUrl().empty()) {
        item->set_dap_service_url(getDapServiceUrl());
    }
    bool is_data = d_catalog_utils->is_data(item->get_name()) || !getDapServiceUrl().empty();
    item->set_is_data(is_data);

    return item;
}




} //namespace cmr
