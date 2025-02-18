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
#include "BESStopWatch.h"
#include "BESNotFoundError.h"
#include "BESDebug.h"
#include "BESUtil.h"
#include "TheBESKeys.h"
#include "BESLog.h"
#include "CmrApi.h"
#include "CmrNames.h"
#include "CmrInternalError.h"
#include "CmrNotFoundError.h"
#include "JsonUtils.h"

using std::string;
using json = nlohmann::json;

#define prolog string("CmrApi::").append(__func__).append("() - ")

namespace cmr {

std::string truth(bool t){ if(t){return "true";} return "false"; }


CmrApi::CmrApi() {
    bool found;
    string cmr_endpoint_url;
    TheBESKeys::TheKeys()->get_value(CMR_HOST_URL_KEY, cmr_endpoint_url, found);
    if (found) {
        d_cmr_endpoint_url = cmr_endpoint_url;
    }
    BESDEBUG(MODULE, prolog << "d_cmr_endpoint_url: " << d_cmr_endpoint_url << endl);

    d_cmr_providers_search_endpoint_url = BESUtil::assemblePath(d_cmr_endpoint_url,
                                                                CMR_PROVIDERS_SEARCH_ENDPOINT);
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

/**
* Internal method that retrieves the "links" array from the Granule's object.
*/
const nlohmann::json& CmrApi::get_related_urls_array(const nlohmann::json& json_obj) const
{
    JsonUtils json;
    return json.qc_get_array(CMR_UMM_RELATED_URLS_KEY,json_obj);
}


/**
  * Locates and QC's the child object named "children" (aka CMR_V2_CHILDREN_KEY)
  * @param jobj
  * @return The "children" (CMR_V2_CHILDREN_KEY) array of objects.
  */
const nlohmann::json &CmrApi::get_children(const nlohmann::json &jobj) const
{
    JsonUtils json;
    BESDEBUG(MODULE, prolog << json.probe_json(jobj) << endl);
    bool result = jobj.is_null();
    if(result){
        string msg;
        msg.append("ERROR: Json document is NULL. json: ").append(jobj.dump());
        BESDEBUG(MODULE, prolog <<  msg << "\n");
        throw CmrInternalError(msg, __FILE__, __LINE__);
    }

    result = jobj.is_object();
    if(!result){
        string msg;
        msg.append("ERROR: Json document is NOT an object. json: ").append(jobj.dump());
        BESDEBUG(MODULE, prolog <<  msg << "\n");
        throw CmrInternalError(msg, __FILE__, __LINE__);
    }

    const auto &has_children_j = jobj[CMR_V2_HAS_CHILDREN_KEY];
    if(!has_children_j.get<bool>()){
        string msg(prolog);
        msg.append("This json object does not have a child property of type ").append(CMR_V2_HAS_CHILDREN_KEY);
        msg.append(".  json: ").append(jobj.dump());
        BESDEBUG(MODULE, msg << "\n");
        INFO_LOG(msg);
    }

    return json.qc_get_array(CMR_V2_CHILDREN_KEY,jobj);
}

/**
 *
 * @param cmr_doc
 * @return
 */
const nlohmann::json &CmrApi::get_feed(const nlohmann::json &cmr_doc) const
{
    JsonUtils json;
    return json.qc_get_object(CMR_V2_FEED_KEY, cmr_doc);
}

/**
 *
 * @param cmr_doc
 * @return
 */
const json& CmrApi::get_entries(const json &cmr_doc) const
{

    JsonUtils json;
    BESDEBUG(MODULE, prolog << "cmr_doc" << endl << cmr_doc.dump(2) << endl);
    const auto &feed = get_feed(cmr_doc);
    return json.qc_get_array(CMR_V2_ENTRY_KEY,feed);
}


/**
 *
 * @param cmr_doc
 * @return
 */
const json& CmrApi::get_items(const json &cmr_doc) const
{
    JsonUtils json;
    BESDEBUG(MODULE, prolog << "cmr_doc" << endl << cmr_doc.dump(2) << endl);
    return json.qc_get_array(CMR_UMM_ITEMS_KEY,cmr_doc);
}


/**
 *
 * @param cmr_doc
 * @return
 */
const nlohmann::json &CmrApi::get_temporal_group(const nlohmann::json &cmr_doc) const
{
    JsonUtils json;
    const auto &feed = get_feed(cmr_doc);

    const auto &facets_obj = json.qc_get_object(CMR_V2_FACETS_KEY, feed);

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
 *
 * @param cmr_doc
 * @return
 */
const nlohmann::json &CmrApi::get_year_group(const nlohmann::json &cmr_doc) const
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
 * @param cmr_doc
 * @return
 */
const nlohmann::json &CmrApi::get_years(const nlohmann::json &cmr_doc) const
{
    auto &year_group = get_year_group(cmr_doc);
    if (year_group[CMR_V2_HAS_CHILDREN_KEY]) {
        return get_children(year_group);
    }
    throw CmrNotFoundError("The Year object had no children.", __FILE__, __LINE__);
}

/**
 *
 * @param target_year
 * @param cmr_doc
 * @return
 */
const nlohmann::json &CmrApi::get_year(const std::string &target_year, const nlohmann::json &cmr_doc) const
{
    const auto &years = get_years(cmr_doc);
    for( auto &year:years) {
        string year_title = year[CMR_V2_TITLE_KEY].get<string>();
        if (target_year == year_title) {
            BESDEBUG(MODULE, prolog + "Found matching Year object. target_year: " << target_year << endl);
            return year;
        }
        BESDEBUG(MODULE, prolog + "The current year: " << year_title <<
        " does not match the target_year of: " << target_year << endl);
    }
    throw CmrNotFoundError("The list of years did not contain on the matched: "+target_year, __FILE__, __LINE__);
}

/**
 *
 * @param target_year
 * @param cmr_doc
 * @return
 */
const nlohmann::json &CmrApi::get_month_group(const std::string &target_year, const nlohmann::json &cmr_doc) const
{


    auto &year = get_year(target_year,cmr_doc);
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

#if 0
    auto &year_group = get_year_group(cmr_doc);
    if(year_group[CMR_V2_HAS_CHILDREN_KEY]){
        auto &years = get_children(year_group);
        for( auto &year:years){
            string year_title = year[CMR_V2_TITLE_KEY].get<string>();
            if(target_year == year_title){
                BESDEBUG(MODULE, prolog + "Found matching Year object. target_year: " << target_year << endl);
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
#endif

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
 const {


    auto &month_group = get_month_group(target_year, cmr_doc);
    auto &months = get_children(month_group);
    for (auto &month: months){
        string month_id = month[CMR_V2_TITLE_KEY].get<string>();
        if(month_id == target_month){
            stringstream msg;
            msg << prolog  << "Located requested month ("<< target_month << ")";
            BESDEBUG(MODULE, msg.str() << endl);
            return month;
        }
        else {
            stringstream msg;
            msg << prolog  << "The month titled '"<<month_id << "' does not match the requested month ("<< target_month <<")";
            BESDEBUG(MODULE, msg.str() << endl);
        }
    }
    stringstream msg;
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
const {

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
void CmrApi::get_years(const string &collection_name, vector<string> &years_result) const
{
    JsonUtils json;
    stringstream cmr_query_url;

    cmr_query_url << d_cmr_granules_search_endpoint_url;
    cmr_query_url << "?concept_id=" + collection_name << "&";
    cmr_query_url << "include_facets=v2&page_size="<<CMR_MAX_PAGE_SIZE;

    BESDEBUG(MODULE, prolog << "CMR Query URL: "<< cmr_query_url.str() << endl);

    const auto &cmr_doc = json.get_as_json(cmr_query_url.str());

    const auto &year_group = get_year_group(cmr_doc);
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
                   vector<string> &months_result) const{
    JsonUtils json;
    stringstream msg;
    stringstream cmr_query_url;

    cmr_query_url << d_cmr_granules_search_endpoint_url << "?" ;
    cmr_query_url << "concept_id=" << collection_name << "&";
    cmr_query_url << "include_facets=v2&";
    cmr_query_url << http::url_encode("temporal_facet[0][year]") << "=" << r_year;
    BESDEBUG(MODULE, prolog << "CMR Query URL: "<< cmr_query_url.str() << endl);

    const auto &cmr_doc = json.get_as_json(cmr_query_url.str());
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




/**
 *
 * @param collection_concept_id
 * @param r_year
 * @param r_month
 * @param days_result
 */
void CmrApi::get_days(const string &collection_concept_id,
                      const string& r_year,
                      const string &r_month,
                      vector<string> &days_result) const
{
    stringstream cmr_query_url;
    cmr_query_url << d_cmr_granules_search_endpoint_url << "?";

    stringstream cmr_query_string;
    cmr_query_string << "concept_id=" << collection_concept_id << "&";
    cmr_query_string << "include_facets=v2" << "&";
    cmr_query_string << http::url_encode("temporal_facet[0][year]") << "=" << r_year << "&";
    cmr_query_string << http::url_encode("temporal_facet[0][month]") << "=" << r_month;
    cmr_query_url << cmr_query_string.str();
    BESDEBUG(MODULE, prolog << "CMR Query URL: " << cmr_query_url.str() << endl);

    JsonUtils json;
    const auto &cmr_doc = json.get_as_json(cmr_query_url.str());

    const auto &day_group = get_day_group(r_month, r_year, cmr_doc);
    const auto &days = get_children(day_group);
    for ( const auto &day : days ){
        string day_title = json.get_str_if_present(CMR_V2_TITLE_KEY,day);
        if(day_title.empty())
            day_title = "MISSING DAY TITLE";

        days_result.push_back(day_title);
    }
}// CmrApi::get_days()






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
                             std::vector<std::string> &granule_ids) const
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
                                    const string &r_day) const
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
void CmrApi::granule_search(const std::string &collection_name,
                            const std::string &r_year,
                            const std::string &r_month,
                            const std::string &r_day,
                            nlohmann::json &cmr_doc) const
{

    JsonUtils json;

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

    cmr_doc = json.get_as_json(cmr_query_url.str());
    BESDEBUG(MODULE, prolog << "Got JSON Document: "<< endl << cmr_doc.dump(4) << endl);
}



/**
 * Locates granules in the collection matching the year, month, and day. Any or all of
 * year, month, and day may be the empty string.
 */
void CmrApi::granule_umm_search(const std::string &collection_name,
                                const std::string &r_year,
                                const std::string &r_month,
                                const std::string &r_day,
                                nlohmann::json &cmr_doc)
const {

    JsonUtils json;

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

    cmr_doc = json.get_as_json(cmr_query_url.str());
    BESDEBUG(MODULE, prolog << "Got JSON Document: "<< endl << cmr_doc.dump(4) << endl);
}



/**
 * Returns all of the Granules in the collection matching the date.
 */
void CmrApi::get_granules(const std::string& collection_name,
                          const std::string &r_year,
                          const std::string &r_month,
                          const std::string &r_day,
                          std::vector<unique_ptr<cmr::Granule>> &granule_objs)
const {
    stringstream msg;
    json cmr_doc;

    granule_search(collection_name, r_year, r_month, r_day, cmr_doc);

    const auto& granules = get_entries(cmr_doc);
    for ( auto &granule : granules){
        auto g = unique_ptr<Granule>(new Granule(granule));
        granule_objs.emplace_back(std::move(g));
    }
}


/**
 * Returns all of the GranuleUMMs in the collection matching the date.
 */
void CmrApi::get_granules_umm(const std::string& collection_name,
                          const std::string &r_year,
                          const std::string &r_month,
                          const std::string &r_day,
                          std::vector<unique_ptr<cmr::GranuleUMM>> &granule_objs) const
{
    stringstream msg;
    json cmr_doc;

    granule_umm_search(collection_name, r_year, r_month, r_day, cmr_doc);
    const auto& granules = get_items(cmr_doc);
    for ( auto &granule : granules){
        auto g = unique_ptr<GranuleUMM>(new GranuleUMM(granule));
        granule_objs.emplace_back(std::move(g));
    }
}





void CmrApi::get_collection_ids(std::vector<std::string> &collection_ids) const
{
    bool found = false;
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
unique_ptr<Granule> CmrApi::get_granule(const string& collection_name,
                                  const string& r_year,
                                  const string& r_month,
                                  const string& r_day,
                                  const string& r_granule_ur)
const {
    // @TODO If this code is supposed to get a single granule, and it has the granule_concept_id
    //   this should be making a direct cmr query for just that granule.
    std::vector<unique_ptr<cmr::Granule>> granules;
    unique_ptr<Granule> result;

    get_granules(collection_name, r_year, r_month, r_day, granules);
    for(auto & granule : granules){
        string id = granule->getName();
        BESDEBUG(MODULE, prolog << "Comparing granule_ur: '" << r_granule_ur << "' to collection member id: " << id << endl);
        if( id == r_granule_ur){
            result = std::move(granule);
        }
    }
    return result;
}

/**
 * Uses the "new" CMR providers API to retrieve all of the Providers information using, specifically,
 * the <cmr_host>/search/providers endpoint and NOT the <cmr_host>/ingest/providers endpoint. This is because the
 * ingest/providers endpoint returns a metadata free list of the providers, and then to get the metadata
 * for each provider a separate HTTP request must be made.
 *
 * The <cmr_host>/search/providers endpoint returns the metadata for all of the providers, so a single HTTP request
 *
 * @param providers The vector into which each Providers unique_ptr object will be placed.
 */
void CmrApi::get_providers(vector<unique_ptr<cmr::Provider> > &providers) const
{
    JsonUtils json;
    BESStopWatch bsw;
    bsw.start(prolog);

    const auto &cmr_doc = json.get_as_json(d_cmr_providers_search_endpoint_url);
    unsigned int hits = cmr_doc["hits"];
    BESDEBUG(MODULE, prolog << "hits: " << hits << endl);
    if (hits == 0){
        return;
    }
    for (const auto &provider_json : cmr_doc["items"]) {
        if(provider_json.type() != nlohmann::detail::value_t::null) {
            auto prvdr = std::make_unique<Provider>(provider_json);
            if(prvdr!=nullptr)
                providers.emplace_back(std::move(prvdr));
        }
    }
}


/**
 * Gets the list of all of the providers and then weeds out the ones that have no collections with OPeNDAP
 * data access URLs
 * @param opendap_providers
 */
void CmrApi::get_opendap_providers(map<string, unique_ptr<cmr::Provider>> &opendap_providers) const
{
    vector<unique_ptr<cmr::Provider>> all_providers;
    BESStopWatch bsw;
    bsw.start(prolog);
    get_providers(all_providers);
    for(auto &provider: all_providers){
        BESDEBUG(MODULE, prolog << "Processing PROVIDER: " << provider->id() << endl);
        auto hits = get_opendap_collections_count(provider->id());
        if (hits > 0){
            provider->set_opendap_collection_count(hits);
            opendap_providers.emplace(provider->id(), std::move(provider));
        }
    }
}

/**
 * Determines the number of collections published by the "provider" that are tagged as having an opendap URL
 * The collections search endpoint query uses the provider_id and the query parameter "has_opendap_url=true"
 * to acheive this.
 * @param provider_id
 * @return
 */
unsigned long int CmrApi::get_opendap_collections_count(const string &provider_id) const
{
    JsonUtils json;
    stringstream cmr_query_url;
    cmr_query_url << d_cmr_collections_search_endpoint_url;
    cmr_query_url << "?has_opendap_url=true&page_size=0";
    cmr_query_url << "&provider=" << provider_id;
    BESDEBUG(MODULE, prolog << "cmr_query_url: " << cmr_query_url.str() << endl);
    const auto &cmr_doc = json.get_as_json(cmr_query_url.str());

    unsigned long int hits = json.qc_integer(CMR_HITS_KEY,cmr_doc);
    BESDEBUG(MODULE, prolog << CMR_HITS_KEY <<  ": " << hits << endl);
    return hits;
}



void CmrApi::get_collections_worker( const std::string &provider_id,
                                    std::map<std::string, std::unique_ptr<cmr::Collection>> &collections,
                                    unsigned int page_size,
                                    bool just_opendap)
const {
    unsigned int page_num=1;
    JsonUtils json;
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

    const auto &cmr_doc = json.get_as_json(cmr_query_url);

    unsigned int hits = cmr_doc["hits"];
    BESDEBUG(MODULE, prolog << "hits: " << hits << endl);
    if (hits == 0){
        return;
    }
    for (const auto &collection_json : cmr_doc["items"]) {
        auto collection = unique_ptr<Collection>(new Collection(collection_json));
        collections.emplace(collection->id(), std::move(collection));
    }
    BESDEBUG(MODULE, prolog << "collections.size(): " << collections.size() << endl);

    while (collections.size() < hits){
        page_num++;
        cmr_query_url = cmr_query_url_base + "page_num=" + to_string(page_num);
        BESDEBUG(MODULE, prolog << "cmr_query_url: " << cmr_query_url << endl);
        const auto &cmr_collection_doc = json.get_as_json(cmr_query_url);

        for (const auto &collection_json : cmr_collection_doc["items"]) {
            auto collection = unique_ptr<Collection>(new Collection(collection_json));
            collections.emplace(collection->id(), std::move(collection));
        }
        BESDEBUG(MODULE, prolog << "collections.size(): " << collections.size() << endl);
    }

}



void CmrApi::get_opendap_collections(const std::string &provider_id,
                                     std::map<std::string,std::unique_ptr<cmr::Collection>> &collections) const{
    get_collections_worker(provider_id,collections, CMR_MAX_PAGE_SIZE, true);
}
void CmrApi::get_collections(const std::string &provider_id,
                             std::map<std::string,std::unique_ptr<cmr::Collection>> &collections) const{
    get_collections_worker(provider_id,collections, CMR_MAX_PAGE_SIZE, false);
}






} // namespace cmr

