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

/*
 * CmrApi.cc
 *
 *  Created on: July, 13 2018
 *      Author: ndp
 */
#include <memory>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <sstream>

#include "nlohmann/json.hpp"

#include <libdap/util.h>
#include <libdap/debug.h>

#include "HttpUtils.h"

#include "BESError.h"
#include "BESInternalError.h"
#include "BESSyntaxUserError.h"
#include "BESNotFoundError.h"
#include "BESDebug.h"
#include "BESUtil.h"
#include "TheBESKeys.h"

#include "CmrApi.h"
#include "CmrNames.h"
#include "CmrInternalError.h"
#include "CmrNotFoundError.h"
#include "rjson_utils.h"

using std::string;
using json = nlohmann::json;

#define prolog string("CmrApi::").append(__func__).append("() - ")

namespace cmr {

std::string truth(bool t){ if(t){return "true";} return "false"; }

std::string CmrApi::probe_json(const nlohmann::json &j)
{
    string hdr0("#########################################################################################");
    string hdr1("#- -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -");

    stringstream msg;
    msg << endl;
    msg << hdr0 << endl;
    msg << j.dump(2) << endl;
    msg << hdr1 << endl;
    msg << "           j.is_null(): " << truth(j.is_null()) << endl;
    msg << "         j.is_object(): " << truth(j.is_object()) << endl;
    msg << "          j.is_array(): " << truth(j.is_array()) << endl;
    msg << endl;
    msg << "      j.is_discarded(): " << truth(j.is_discarded()) << endl;
    msg << "         j.is_string(): " << truth(j.is_string()) << endl;
    msg << "     j.is_structured(): " << truth(j.is_structured()) << endl;
    msg << "         j.is_binary(): " << truth(j.is_binary()) << endl;
    msg << "        j.is_boolean(): " << truth(j.is_boolean()) << endl;
    msg << "         j.is_number(): " << truth(j.is_number()) << endl;
    msg << "   j.is_number_float(): " << truth(j.is_number_float()) << endl;
    msg << " j.is_number_integer(): " << truth(j.is_number_integer()) << endl;
    msg << "j.is_number_unsigned(): " << truth(j.is_number_unsigned()) << endl;
    msg << "      j.is_primitive(): " << truth(j.is_primitive()) << endl;
    msg << "              j.size(): " << j.size()<< endl;
    msg << "             j.empty(): " << truth(j.empty()) << endl;

    msg << hdr0 << endl;
    return msg.str();
}


CmrApi::CmrApi() : d_cmr_endpoint_url(DEFAULT_CMR_HOST_URL) {
    bool found;
    string cmr_endpoint_url;
    TheBESKeys::TheKeys()->get_value(CMR_HOST_URL_KEY, cmr_endpoint_url, found);
    if (found) {
        d_cmr_endpoint_url = cmr_endpoint_url;
    }
    BESDEBUG(MODULE, prolog << "d_cmr_endpoint_url: " << d_cmr_endpoint_url << endl);

    d_cmr_providers_search_endpoint_url = BESUtil::assemblePath(d_cmr_endpoint_url,
                                                                CMR_PROVIDERS_LEGACY_API_ENDPOINT);
    BESDEBUG(MODULE,
             prolog << "d_cmr_providers_search_endpoint_url: " << d_cmr_providers_search_endpoint_url << endl);

    d_cmr_collections_search_endpoint_url = BESUtil::assemblePath(d_cmr_endpoint_url,
                                                                  CMR_COLLECTIONS_SEARCH_API_ENDPOINT);
    BESDEBUG(MODULE,
             prolog << "d_cmr_collections_search_endpoint_url: " << d_cmr_collections_search_endpoint_url << endl);

    d_cmr_granules_search_endpoint_url = BESUtil::assemblePath(d_cmr_endpoint_url,
                                                               CMR_GRANULES_SEARCH_API_ENDPOINT);
    BESDEBUG(MODULE,
             prolog << "d_cmr_granules_search_endpoint_url: " << d_cmr_granules_search_endpoint_url << endl);

    d_cmr_granules_umm_search_endpoint_url = BESUtil::assemblePath(d_cmr_endpoint_url,
                                                               CMR_GRANULES_SEARCH_API_UMM_ENDPOINT);
    BESDEBUG(MODULE,
             prolog << "d_cmr_granules_umm_search_endpoint_url: " << d_cmr_granules_umm_search_endpoint_url << endl);
}

std::string CmrApi::get_str_if_present(std::string key, const nlohmann::json& json_obj)
{
    string value;

    BESDEBUG(MODULE, prolog << "Key: '" << key << "' JSON: " << endl << json_obj.dump(2) << endl);
    // Check input for object.
    bool result = json_obj.is_object();
    string msg0 = prolog + "Json document is" + (result?"":" NOT") + " an object.";
    BESDEBUG(MODULE, msg0 << endl);
    if(!result){
        return value;
    }

    const auto &key_itr = json_obj.find(key);
    if(key_itr == json_obj.end()){
        stringstream msg;
        msg << prolog;
        msg << "Ouch! Unable to locate the '" << key;
        msg << "' child of json: " << endl << json_obj.dump(2) << endl;
        BESDEBUG(MODULE, msg.str() << endl);
        return value;
    }

    auto &string_obj = json_obj[key];
    if(string_obj.is_null()){
        stringstream msg;
        msg << prolog;
        msg << "Failed to locate the '" << key;
        msg << "' child of json: " << endl << json_obj.dump(2) << endl;
        BESDEBUG(MODULE, msg.str() << endl);
        return value;
    }
    if(!string_obj.is_string()){
        stringstream msg;
        msg << prolog;
        msg << "The child element called '" << key;
        msg << "' is not a string. json: " << endl << json_obj.dump(2) << endl;
        BESDEBUG(MODULE, msg.str() << endl);
        return value;
    }
    return string_obj.get<string>();


}


const nlohmann::json& CmrApi::qc_get_array(std::string key, const nlohmann::json& json_obj)
{
    BESDEBUG(MODULE, prolog << "Key: '" << key << "' JSON: " << endl << json_obj.dump(2) << endl);
    // Check input for object.
    bool result = json_obj.is_object();
    string msg0 = prolog + "Json document is" + (result?"":" NOT") + " an object.";
    BESDEBUG(MODULE, msg0 << endl);
    if(!result){
        throw CmrInternalError(msg0, __FILE__, __LINE__);
    }

    const auto &key_itr = json_obj.find(key);
    if(key_itr == json_obj.end()){
        stringstream msg;
        msg << prolog;
        msg << "Ouch! Unable to locate the '" << key;
        msg << "' child of json: " << endl << json_obj.dump(2) << endl;
        BESDEBUG(MODULE, msg.str() << endl);
        throw CmrNotFoundError(msg.str(), __FILE__, __LINE__);
    }

    auto &array_obj = json_obj[key];
    if(array_obj.is_null()){
        stringstream msg;
        msg << prolog;
        msg << "Ouch! Unable to locate the '" << key;
        msg << "' child of json: " << endl << json_obj.dump(2) << endl;
        BESDEBUG(MODULE, msg.str() << endl);
        throw CmrNotFoundError(msg.str(), __FILE__, __LINE__);
    }
    if(!array_obj.is_array()){
        stringstream msg;
        msg << prolog;
        msg << "ERROR: The child element called '" << key;
        msg << "' is not an array. json: " << endl << json_obj.dump(2) << endl;
        BESDEBUG(MODULE, msg.str() << endl);
        throw CmrInternalError(msg.str(), __FILE__, __LINE__);
    }
    return array_obj;
}
const nlohmann::json& CmrApi::qc_get_object(std::string key, const nlohmann::json& json_obj)
{
    BESDEBUG(MODULE, prolog << "Key: '" << key << "' JSON: " << endl << json_obj.dump(2) << endl);

    // Check input for object.
    bool result = json_obj.is_object();
    string msg0 = prolog + "Json document is" + (result?"":" NOT") + " an object.";
    BESDEBUG(MODULE, msg0 << endl);
    if(!result){
        throw CmrInternalError(msg0, __FILE__, __LINE__);
    }

    const auto &key_itr = json_obj.find(key);
    if(key_itr == json_obj.end()){
        stringstream msg;
        msg << prolog;
        msg << "Ouch! Unable to locate the '" << key;
        msg << "' child of json: " << endl << json_obj.dump(2) << endl;
        BESDEBUG(MODULE, msg.str() << endl);
        throw CmrNotFoundError(msg.str(), __FILE__, __LINE__);
    }

    auto &child_obj = json_obj[key];

    if(child_obj.is_null()){
        stringstream msg;
        msg << prolog;
        msg << "Ouch! Unable to locate the '" << key;
        msg << "' child of json: " << endl << child_obj.dump(2) << endl;
        BESDEBUG(MODULE, msg.str() << endl);
        throw CmrNotFoundError(msg.str(), __FILE__, __LINE__);
    }
    if(!child_obj.is_object()){
        stringstream msg;
        msg << prolog;
        msg << "ERROR: The child element called '" << key;
        msg << "' is not an object. json: " << endl << child_obj.dump(2) << endl;
        BESDEBUG(MODULE, msg.str() << endl);
        throw CmrInternalError(msg.str(), __FILE__, __LINE__);
    }
    return child_obj;
}

/**
* Internal method that retrieves the "links" array from the Granule's object.
*/
const nlohmann::json& CmrApi::get_related_urls_array(const nlohmann::json& json_obj)
{
    return qc_get_array(CMR_UMM_RELATED_URLS_KEY,json_obj);
}


/**
  * Locates and QC's the child object named "children" (aka CMR_V2_CHILDREN_KEY)
  * @param jobj
  * @return The "children" (CMR_V2_CHILDREN_KEY) array of objects.
  */
const nlohmann::json &CmrApi::get_children(const nlohmann::json &jobj)
{
    BESDEBUG(MODULE, prolog << probe_json(jobj) << endl);
    bool result = jobj.is_null();
    if(jobj.is_null()){
        stringstream msg;
        msg <<  "ERROR: Json document is NULL: " << endl << jobj.dump(2) << endl;
        BESDEBUG(MODULE, prolog <<  msg.str() << endl);
        throw CmrInternalError(msg.str(), __FILE__, __LINE__);
    }

    result = jobj.is_object();
    if(!result){
        stringstream msg;
        msg <<  "ERROR: Json document is NOT an object. json: " << endl << jobj.dump(2) << endl;
        BESDEBUG(MODULE, prolog <<  msg.str() << endl);
        throw CmrInternalError(msg.str(), __FILE__, __LINE__);
    }

    const auto &has_children_j = jobj[CMR_V2_HAS_CHILDREN_KEY];
    if(!has_children_j.get<bool>()){
        stringstream msg;
        msg << prolog;
        msg << "This json object does not have children. json: " << jobj.dump(2) << endl;
        BESDEBUG(MODULE, msg.str() << endl);
        throw CmrNotFoundError(msg.str(), __FILE__, __LINE__);
    }

    return  qc_get_array(CMR_V2_CHILDREN_KEY,jobj);
}

/**
 *
 * @param cmr_doc
 * @return
 */
const nlohmann::json &CmrApi::get_feed(const nlohmann::json &cmr_doc)
{
    return qc_get_object(CMR_V2_FEED_KEY, cmr_doc);
}

/**
 *
 */
const json& CmrApi::get_entries(const json &cmr_doc)
{
    bool result;

    const auto &feed = get_feed(cmr_doc);
    return qc_get_array(CMR_V2_ENTRY_KEY,feed);
}



const json& CmrApi::get_items(const json &cmr_doc)
{
    bool result;

    BESDEBUG(MODULE, prolog << "cmr_doc" << endl << cmr_doc.dump(2) << endl);
    return qc_get_array(CMR_UMM_ITEMS_KEY,cmr_doc);
}


/**
 *
 */
const nlohmann::json &CmrApi::get_temporal_group(const nlohmann::json &cmr_doc)
{
    string msg0;
    const auto &feed = get_feed(cmr_doc);

    const auto &facets_obj = qc_get_object(CMR_V2_FACETS_KEY, feed);

    const auto &facets_array = get_children(facets_obj);
    for(const auto &facet:facets_array){
        string facet_title = facet[CMR_V2_TITLE_KEY].get<string>();
        if(facet_title == CMR_V2_TEMPORAL_TITLE_VALUE){
            string msg = prolog + "Found Temporal object.";
            BESDEBUG(MODULE, msg << endl);
            return facet;
        }
        else {
            string msg = prolog + "The child of 'facets' with title '"+facet_title+"' does not match "+ CMR_V2_TEMPORAL_TITLE_VALUE;
            BESDEBUG(MODULE, msg << endl);
        }

    }
    stringstream msg;
    msg << "Failed to locate the Temporal facet in : " << endl << cmr_doc.dump(2) << endl;
    BESDEBUG(MODULE, prolog << msg.str() << endl);
    throw BESNotFoundError(msg.str(), __FILE__, __LINE__);
} // CmrApi::get_temporal_group()




/**


const json &CmrApi::get_year_group(const json &cmr_doc)
{
    auto &feed = cmr_doc[CMR_V2_FEED_KEY];
    auto &facets = feed[CMR_V2_FACETS_KEY];
    if(facets[CMR_V2_HAS_CHILDREN_KEY].get<bool>()) {
        for (auto &facet: facets[CMR_V2_CHILDREN_KEY]) {
            if (facet[CMR_V2_TITLE_KEY].get<std::string>() == CMR_V2_TEMPORAL_FACET_TITLE_VALUE) {
                if (facet[CMR_V2_HAS_CHILDREN_KEY].get<bool>()) {
                    for (auto &temporal_child: facet[CMR_V2_CHILDREN_KEY]) {
                        if (temporal_child[CMR_V2_TITLE_KEY].get<std::string>() == CMR_V2_YEAR_TITLE_VALUE) {
                            return temporal_child; // It's the Year group!
                        }
                    }
                }
            }
        }
    }

}
**/



/**
 *
 * @param cmr_doc
 * @return
 */
const nlohmann::json &CmrApi::get_year_group(const nlohmann::json &cmr_doc)
{

    const auto &temporal_group = get_temporal_group(cmr_doc);
    const auto &temporal_children = get_children(temporal_group);
    for(const auto &temporal_child : temporal_children){
        string temporal_child_title = temporal_child[CMR_V2_TITLE_KEY].get<string>();
        if ( temporal_child_title == CMR_V2_YEAR_TITLE_VALUE ){
            string msg = prolog + "Found Year object.";
            BESDEBUG(MODULE, msg << endl);
            return temporal_child;
        }
        else {
            string msg = prolog + "The child of 'Temporal' with title '"+temporal_child_title+"' does not match 'Year'";
            BESDEBUG(MODULE, msg << endl);
        }
    }
    string msg = prolog + "Failed to locate the Year group.";
    BESDEBUG(MODULE, msg << endl);
    throw CmrInternalError(msg, __FILE__, __LINE__);
}


/**
 *
 * @param target_year
 * @param cmr_doc
 * @return
 */
const nlohmann::json &CmrApi::get_month_group(const std::string &target_year, const nlohmann::json &cmr_doc){

    auto &year_group = get_year_group(cmr_doc);
    if(year_group[CMR_V2_HAS_CHILDREN_KEY]){
        auto &years = get_children(year_group);
        for( auto &year:years){
            string year_title = year[CMR_V2_TITLE_KEY].get<string>();
            if(target_year == year_title){
                BESDEBUG(MODULE, prolog + "Found Year object." << endl);
                auto &months = get_children(year);
                for(auto &month: months){
                    string title = month[CMR_V2_TITLE_KEY];
                    if( title == CMR_V2_MONTH_TITLE_VALUE ){
                        BESDEBUG(MODULE, prolog + "Found Month object." << endl);
                        return month;
                    }
                    else {
                        stringstream msg;
                        msg << prolog << "The child of '" << CMR_V2_YEAR_TITLE_VALUE << "' with title '"+title+"' does not match 'Month'";
                        BESDEBUG(MODULE, msg.str() << endl);
                    }
                }
            }
        }
    }
    string msg = prolog + "Failed to locate the Month group.";
    BESDEBUG(MODULE, msg << endl);
    throw CmrInternalError(msg, __FILE__, __LINE__);
}



/**
 *
 *
 * @param target_month
 * @param target_year
 * @param cmr_doc
 * @return
 */
const nlohmann::json &
CmrApi::get_month(const std::string &target_month,
                  const std::string &target_year,
                  const nlohmann::json &cmr_doc)
{
    stringstream msg;

    auto &month_group = get_month_group(target_year, cmr_doc);
    auto &months = get_children(month_group);
    for (auto &month: months){
        string month_id = month[CMR_V2_TITLE_KEY].get<string>();
        if(month_id == target_month){
            msg.str("");
            msg << prolog  << "Located requested month ("<< target_month << ")";
            BESDEBUG(MODULE, msg.str() << endl);
            return month;
        }
        else {
            msg.str("");
            msg << prolog  << "The month titled '"<<month_id << "' does not match the requested month ("<< target_month <<")";
            BESDEBUG(MODULE, msg.str() << endl);
        }
    }
    msg.str("");
    msg << prolog  << "Failed to locate request Year/Month.";
    BESDEBUG(MODULE, msg.str() << endl);
    throw CmrInternalError(msg.str(), __FILE__, __LINE__);

}





/**
 *
 * @param target_month
 * @param target_year
 * @param cmr_doc
 * @return
 */
const nlohmann::json &
CmrApi::get_day_group(const std::string &target_month,
                        const std::string &target_year,
                        const nlohmann::json &cmr_doc)
{
    const auto &month = get_month( target_month, target_year,cmr_doc);
    const auto &m_kids = get_children(month);
    for ( const auto &m_kid : m_kids ) {
        string title = m_kid[CMR_V2_TITLE_KEY].get<string>();
        if(title == CMR_V2_DAY_TITLE_VALUE){
            stringstream msg;
            msg << prolog  << "Located Day group for year: " << target_year << " month: "<< target_month;
            BESDEBUG(MODULE, msg.str() << endl);
            return m_kid;
        }
    }
    stringstream msg;
    msg << prolog  << "Failed to locate requested Day, year: " << target_year << " month: "<< target_month;
    BESDEBUG(MODULE, msg.str() << endl);
    throw CmrInternalError(msg.str(), __FILE__, __LINE__);
}



/**
 * Queries CMR for the 'collection_name' and returns the span of years covered by the collection.
 *
 * @param collection_name The name of the collection to query.
 * @param collection_years A vector into which the years will be placed.
 */
void CmrApi::get_years(const string &collection_name, vector<string> &years_result)
{
    rjson_utils rju;
    // bool result;
    string msg;

    stringstream cmr_query_url;
    cmr_query_url << d_cmr_granules_search_endpoint_url;
    cmr_query_url << "?concept_id=" + collection_name << "&";
    cmr_query_url << "include_facets=v2&page_size="<<CMR_MAX_PAGE_SIZE;

    BESDEBUG(MODULE, prolog << "CMR Query URL: "<< cmr_query_url.str() << endl);

    json cmr_doc = rju.get_as_json(cmr_query_url.str());

    const json &year_group = get_year_group(cmr_doc);
    if(year_group[CMR_V2_HAS_CHILDREN_KEY].get<bool>()) {
        for (const auto &year_obj: year_group[CMR_V2_CHILDREN_KEY]) {
            years_result.emplace_back(year_obj[CMR_V2_TITLE_KEY].get<string>());
        }
    }
} // CmrApi::get_years()



/**
 * Queries CMR for the 'collection_name' and returns the span of years covered by the collection.
 *
 * https://cmr.earthdata.nasa.gov/search/granules.json?concept_id=C179003030-ORNL_DAAC&include_facets=v2&temporal_facet%5B0%5D%5Byear%5D=1985
 *
 * @param collection_name The name of the collection to query.
 * @param collection_years A vector into which the years will be placed.
 */
void
CmrApi::get_months(const string &collection_name,
                   const string &r_year,
                   vector<string> &months_result){
    rjson_utils rju;

    stringstream msg;

    stringstream cmr_query_url;
    cmr_query_url << d_cmr_granules_search_endpoint_url << "?" ;
    cmr_query_url << "concept_id=" << collection_name << "&";
    cmr_query_url << "include_facets=v2&";
    cmr_query_url << http::url_encode("temporal_facet[0][year]") << "=" << r_year;
    BESDEBUG(MODULE, prolog << "CMR Query URL: "<< cmr_query_url.str() << endl);

    json cmr_doc = rju.get_as_json(cmr_query_url.str());
    BESDEBUG(MODULE, prolog << "Got JSON Document: "<< endl << cmr_doc.dump(2) << endl);

    const auto &year_group = get_year_group(cmr_doc);
    const auto &years = get_children(year_group);
    if(years.size() != 1){
        msg.str("");
        msg << prolog  << "We expected to get back one year (" << r_year << ") but we got back " << years.size();
        BESDEBUG(MODULE, msg.str() << endl);
        throw CmrInternalError(msg.str(), __FILE__, __LINE__);
    }

    const auto &year = years[0];

    string year_title = year[CMR_V2_TITLE_KEY].get<string>();
    if(year_title != r_year){
        msg.str("");
        msg << prolog  << "The returned year (" << year_title << ") does not match the requested year ("<< r_year << ")";
        BESDEBUG(MODULE, msg.str() << endl);
        throw CmrInternalError(msg.str(), __FILE__, __LINE__);
    }

    const auto &year_children = get_children(year);
    if(year_children.size() != 1){
        msg.str("");
        msg << prolog  << "We expected to get back one child for the year (" << r_year << ") but we got back " << years.size();
        BESDEBUG(MODULE, msg.str() << endl);
        throw CmrInternalError(msg.str(), __FILE__, __LINE__);
    }

    const auto &month_group = year_children[0];
    string month_title = month_group[CMR_V2_TITLE_KEY].get<string>();
    if(month_title != CMR_V2_MONTH_TITLE_VALUE){
        msg.str("");
        msg << prolog  << "We expected to get back a Month object, but we did not.";
        BESDEBUG(MODULE, msg.str() << endl);
        throw CmrInternalError(msg.str(), __FILE__, __LINE__);
    }

    const auto &months = get_children(month_group);
    for (const auto &month: months) { // Uses SizeType instead of size_t
        months_result.push_back(month[CMR_V2_TITLE_KEY].get<string>());
    }

} // CmrApi::get_months()




void CmrApi::get_days(const string &collection_concept_id,
                      const string& r_year, string r_month, vector<string> &days_result)
{
    stringstream cmr_query_url, cmr_query_string;
    cmr_query_url << d_cmr_granules_search_endpoint_url << "?";

    cmr_query_string << "concept_id=" << collection_concept_id << "&";
    cmr_query_string << "include_facets=v2" << "&";
    cmr_query_string << http::url_encode("temporal_facet[0][year]") << "=" << r_year << "&";
    cmr_query_string << http::url_encode("temporal_facet[0][month]") << "=" << r_month;
    cmr_query_url << cmr_query_string.str();
    BESDEBUG(MODULE, prolog << "CMR Query URL: " << cmr_query_url.str() << endl);

    rjson_utils ju;
    json cmr_doc = ju.get_as_json(cmr_query_url.str());

    auto &day_group = get_day_group(r_month, r_year, cmr_doc);
    auto &days = get_children(day_group);
    for ( const auto &day : days ){
        string day_title = day[CMR_V2_TITLE_KEY].get<string>();
        days_result.push_back(day_title);
    }
}






/**
 *
 * @param collection_name
 * @param r_year
 * @param r_month
 * @param r_day
 * @param granule_ids
 */
void CmrApi::get_granule_ids(const std::string& collection_name,
                             const std::string& r_year,
                             const std::string &r_month,
                             const std::string &r_day,
                             std::vector<std::string> &granule_ids)
{
    json cmr_doc;

    granule_search(collection_name, r_year, r_month, r_day, cmr_doc);

    const auto &granules = get_entries(cmr_doc);
    for( const auto &granule : granules){
        string day_id = granule[CMR_GRANULE_ID_KEY].get<string>();
        granule_ids.push_back(day_id);
    }
}




/**
 *
 * @param collection_name
 * @param r_year
 * @param r_month
 * @param r_day
 * @return
 */
unsigned long CmrApi::granule_count(const string &collection_name,
                                    const string &r_year,
                                    const string &r_month,
                                    const string &r_day)
{
    stringstream msg;
    json cmr_doc;
    granule_search(collection_name, r_year, r_month, r_day, cmr_doc);
    const auto &entries = get_entries(cmr_doc);
    return entries.size();
}




/**
 * Locates granules in the collection matching the year, month, and day. Any or all of
 * year, month, and day may be the empty string.
 */
void CmrApi::granule_search(const std::string &collection_name, const std::string &r_year, const std::string &r_month, const std::string &r_day, nlohmann::json &cmr_doc)
{

    rjson_utils rju;

    stringstream cmr_query_url;
    cmr_query_url <<  d_cmr_granules_search_endpoint_url << "?";
    cmr_query_url << "concept_id=" << collection_name << "&";
    cmr_query_url << "include_facets=v2&";
    cmr_query_url << "page_size=" << CMR_MAX_PAGE_SIZE << "&";

    if(!r_year.empty()) {
        cmr_query_url << http::url_encode("temporal_facet[0][year]") << "=" << r_year<< "&";
    }
    if(!r_month.empty()) {
        cmr_query_url << http::url_encode("temporal_facet[0][month]") << "=" << r_month<< "&";
    }
    if(!r_day.empty()) {
        cmr_query_url << http::url_encode("temporal_facet[0][day]") << "=" << r_day;
    }

    BESDEBUG(MODULE, prolog << "CMR Granule Search Request Url: " << cmr_query_url.str() << endl);

    cmr_doc = rju.get_as_json(cmr_query_url.str());
    BESDEBUG(MODULE, prolog << "Got JSON Document: "<< endl << cmr_doc.dump(4) << endl);
}



/**
 * Locates granules in the collection matching the year, month, and day. Any or all of
 * year, month, and day may be the empty string.
 */
void CmrApi::granule_umm_search(const std::string &collection_name, const std::string &r_year, const std::string &r_month, const std::string &r_day, nlohmann::json &cmr_doc)
{

    rjson_utils rju;

    stringstream cmr_query_url;
    cmr_query_url <<  d_cmr_granules_umm_search_endpoint_url << "?";
    cmr_query_url << "concept_id=" << collection_name << "&";
    cmr_query_url << "page_size=" << CMR_MAX_PAGE_SIZE << "&";

    if(!r_year.empty()) {
        cmr_query_url << http::url_encode("temporal_facet[0][year]") << "=" << r_year<< "&";
    }
    if(!r_month.empty()) {
        cmr_query_url << http::url_encode("temporal_facet[0][month]") << "=" << r_month<< "&";
    }
    if(!r_day.empty()) {
        cmr_query_url << http::url_encode("temporal_facet[0][day]") << "=" << r_day;
    }

    BESDEBUG(MODULE, prolog << "CMR Granule Search Request Url: " << cmr_query_url.str() << endl);

    cmr_doc = rju.get_as_json(cmr_query_url.str());
    BESDEBUG(MODULE, prolog << "Got JSON Document: "<< endl << cmr_doc.dump(4) << endl);
}



/**
 * Returns all of the Granules in the collection matching the date.
 */
void CmrApi::get_granules(const std::string& collection_name,
                          const std::string &r_year,
                          const std::string &r_month,
                          const std::string &r_day,
                          std::vector<cmr::Granule *> &granule_objs)
{
    stringstream msg;
    json cmr_doc;

    granule_search(collection_name, r_year, r_month, r_day, cmr_doc);

    const auto& granules = get_entries(cmr_doc);
    for ( auto &granule : granules){
        auto *g = new Granule(granule);
        granule_objs.push_back(g);
    }
}


/**
 * Returns all of the GranuleUMMs in the collection matching the date.
 */
void CmrApi::get_granules_umm(const std::string& collection_name,
                          const std::string &r_year,
                          const std::string &r_month,
                          const std::string &r_day,
                          std::vector<cmr::GranuleUMM *> &granule_objs)
{
    stringstream msg;
    json cmr_doc;

    granule_umm_search(collection_name, r_year, r_month, r_day, cmr_doc);
    const auto& granules = get_items(cmr_doc);
    for ( auto &granule : granules){
        auto *g = new GranuleUMM(granule);
        granule_objs.push_back(g);
    }
}





void CmrApi::get_collection_ids(std::vector<std::string> &collection_ids)
{
    bool found = false;
    string key = CMR_COLLECTIONS_KEY;
    TheBESKeys::TheKeys()->get_values(CMR_COLLECTIONS_KEY, collection_ids, found);
    if(!found){
        throw BESInternalError(string("The '") + CMR_COLLECTIONS_KEY
                               + "' field has not been configured.", __FILE__, __LINE__);
    }
}


/**
  * Returns the requested granule by searching the collection at the supplied  date and sifting
  * the result.
  *
  * @param collection_name
  * @param r_year
  * @param r_month
  * @param r_day
  * @param granule_id
  * @return
  */
cmr::Granule* CmrApi::get_granule(const string& collection_name,
                                  const string& r_year,
                                  const string& r_month,
                                  const string& r_day,
                                  const string& r_granule_concept_id)
{
    // @TODO If this code is supposed to get a single granule, and it has the granule_concept_id
    //   this should be making a direct cmr query for just that granule.
    vector<Granule *> granules;
    Granule *result = nullptr;

    get_granules(collection_name, r_year, r_month, r_day, granules);
    for(auto & granule : granules){
        string id = granule->getName();
        BESDEBUG(MODULE, prolog << "Comparing r_granule_concept_id: '" << r_granule_concept_id << "' to collection member id: " << id << endl);
        if( id == r_granule_concept_id){
            result = granule;
        }
        else {
            // This deletes the granule in the granules vector because granule is a reference.
            delete granule;
            granule = nullptr;
        }
    }
    return result;
}


/**
 * https://cmr.earthdata.nasa.gov/legacy-services/rest/providers/LPDAAC_ECS.json
 * @param provider_id
 * @return
 */
Provider CmrApi::get_provider(const std::string &provider_id)
{
    rjson_utils ju;

    string cmr_query_url = BESUtil::pathConcat(d_cmr_providers_search_endpoint_url,provider_id);
    cmr_query_url += ".json";
    BESDEBUG(MODULE, prolog << "CMR Providers Search Request Url: : " << cmr_query_url << endl);

    json cmr_doc = ju.get_as_json(cmr_query_url);

    // We know that this CMR query returns a single anonymous json object, which
    // in turn contains a single provider object (really...)

    // Grab the internal provider object...
    auto provider_json = cmr_doc[CMR_PROVIDER_KEY];
    // And then make a new provider.
    Provider provider(provider_json);

    return provider;
}


void CmrApi::get_providers(vector<cmr::Provider> &providers)
{
    rjson_utils ju;

    stringstream cmr_query_url;
    cmr_query_url << d_cmr_providers_search_endpoint_url << ".json?page_size=" << CMR_MAX_PAGE_SIZE;
    BESDEBUG(MODULE, prolog << "CMR Providers Search Request Url: : " << cmr_query_url.str() << endl);

    json cmr_doc = ju.get_as_json(cmr_query_url.str());

    // We know that this CMR query returns an array of anonymous json objects, each of which
    // contains a single provider object (really...)

    // So we iterate over the anonymous objects
    for (auto &obj : cmr_doc){
        // And the grab the internal provider object...
        auto provider_json = obj[CMR_PROVIDER_KEY];
        // And then make a new provider.
        Provider provider(provider_json);
        //BESDEBUG(MODULE, prolog << provider.dump() << endl);
        providers.emplace_back(std::move(provider));
    }

}

void CmrApi::get_opendap_providers(vector<cmr::Provider> &providers){
    vector<cmr::Provider> all_providers;
    get_providers(all_providers);
    for(auto &provider: all_providers){
        BESDEBUG(MODULE, prolog << "PROVIDER: " << provider.id() << endl);
        unsigned int hits = get_opendap_collections_count(provider.id());
        provider.set_opendap_collection_count(hits);
        if (hits > 0){
            providers.emplace_back(provider);
        }
    }
}

unsigned int CmrApi::get_opendap_collections_count(const string &provider_id)
{
    rjson_utils ju;
    stringstream cmr_query_url;
    cmr_query_url << d_cmr_collections_search_endpoint_url;
    cmr_query_url << "?has_opendap_url=true&page_size=0";
    cmr_query_url << "&provider=" << provider_id;
    BESDEBUG(MODULE, prolog << "cmr_query_url: " << cmr_query_url.str() << endl);
    json cmr_doc = ju.get_as_json(cmr_query_url.str());
//        BESDEBUG(MODULE, prolog << cmr_doc.dump() << endl);
    unsigned int hits = cmr_doc["hits"];
    BESDEBUG(MODULE, prolog << "HITS: " << hits << endl);
    return hits;
}


void CmrApi::get_collections_worker(const std::string &provider_id, std::vector<cmr::Collection> &collections,
                             unsigned int page_size,
                             bool just_opendap)
{
    unsigned int page_num=1;
    rjson_utils ju;
    string cmr_query_url_base;
    cmr_query_url_base = d_cmr_collections_search_endpoint_url + "?";
    cmr_query_url_base += "provider=" + provider_id + "&";
    if(just_opendap){
        cmr_query_url_base += "has_opendap_url=true&";
    }
    cmr_query_url_base += "page_size=" + to_string(page_size) + "&";
    BESDEBUG(MODULE, prolog << "cmr_query_url_base: " << cmr_query_url_base << endl);

    string cmr_query_url = cmr_query_url_base + "page_num=" + to_string(page_num);
    BESDEBUG(MODULE, prolog << "cmr_query_url: " << cmr_query_url << endl);

    json cmr_doc = ju.get_as_json(cmr_query_url);
//      BESDEBUG(MODULE, prolog << cmr_doc.dump() << endl);
    unsigned int hits = cmr_doc["hits"];
    BESDEBUG(MODULE, prolog << "hits: " << hits << endl);
    if (hits == 0){
        return;
    }
    for (auto &collection_json : cmr_doc["items"]) {
        Collection collection(collection_json);
        collections.emplace_back(collection);
    }
    BESDEBUG(MODULE, prolog << "collections.size(): " << collections.size() << endl);

    while (collections.size() < hits){
        page_num++;
        cmr_query_url = cmr_query_url_base + "page_num=" + to_string(page_num);
        BESDEBUG(MODULE, prolog << "cmr_query_url: " << cmr_query_url << endl);
        cmr_doc = ju.get_as_json(cmr_query_url);
//      BESDEBUG(MODULE, prolog << cmr_doc.dump() << endl);
        for (auto &collection_json : cmr_doc["items"]) {
            Collection collection(collection_json);
            collections.emplace_back(collection);
        }
        BESDEBUG(MODULE, prolog << "collections.size(): " << collections.size() << endl);
    }

}



void CmrApi::get_opendap_collections(const std::string &provider_id, std::vector<cmr::Collection> &collections){
    get_collections_worker(provider_id,collections, CMR_MAX_PAGE_SIZE, true);
}
void CmrApi::get_collections(const std::string &provider_id, std::vector<cmr::Collection> &collections){
    get_collections_worker(provider_id,collections, CMR_MAX_PAGE_SIZE, false);
}






} // namespace cmr

