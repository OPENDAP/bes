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
 * CmrApi.cc
 *
 *  Created on: July, 13 2018
 *      Author: ndp
 */
#include <memory>
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/filereadstream.h"
#include <cstdio>
#include <cstring>
#include <iostream>
#include <sstream>


#include <util.h>
#include <debug.h>

#include <BESError.h>
#include <BESDebug.h>
#include <BESUtil.h>
#include <TheBESKeys.h>

#include "CmrApi.h"
#include "RemoteHttpResource.h"
#include "CmrError.h"
#include "rjson_utils.h"


#define MODULE "cmr"

using namespace std;

namespace cmr {

/**
 *
 */
const rapidjson::Value&
CmrApi::get_children(const rapidjson::Value& obj) {
    string prolog = string("CmrApi::") + __func__ + "() - ";
    rapidjson::Value::ConstMemberIterator itr;

    itr = obj.FindMember("children");
    bool result  = itr != obj.MemberEnd();
    string msg = prolog + (result?"Located":"FAILED to locate") + " the value 'children' in the object.";
    BESDEBUG(MODULE, msg << endl);
    if(!result){
        throw CmrError(msg,__FILE__,__LINE__);
    }

    const rapidjson::Value& children = itr->value;
    result = children.IsArray();
    msg = prolog + "The value 'children' is" + (result?"":" NOT") + " an array.";
    BESDEBUG(MODULE, msg << endl);
    if(!result){
        throw CmrError(msg,__FILE__,__LINE__);
    }
    return children;
}

/**
 *
 */
const rapidjson::Value&
CmrApi::get_feed(const rapidjson::Document &cmr_doc){
    string prolog = string("CmrApi::") + __func__ + "() - ";

    bool result = cmr_doc.IsObject();
    string msg = prolog + "Json document is" + (result?"":" NOT") + " an object.";
    BESDEBUG(MODULE, msg << endl);
    if(!result){
        throw CmrError(msg,__FILE__,__LINE__);
    }

    //################### feed
    rapidjson::Value::ConstMemberIterator itr = cmr_doc.FindMember("feed");
    result  = itr != cmr_doc.MemberEnd();
    msg = prolog + (result?"Located":"FAILED to locate") + " the value 'feed'.";
    BESDEBUG(MODULE, msg << endl);
    if(!result){
        throw CmrError(msg,__FILE__,__LINE__);
    }

    const rapidjson::Value& feed = itr->value;
    result  = feed.IsObject();
    msg = prolog + "The value 'feed' is" + (result?"":" NOT") + " an object.";
    BESDEBUG(MODULE, msg << endl);
    if(!result){
        throw CmrError(msg,__FILE__,__LINE__);
    }
    return feed;
}

/**
 *
 */
const rapidjson::Value&
CmrApi::get_entries(const rapidjson::Document &cmr_doc){
    string prolog = string("CmrApi::") + __func__ + "() - ";
    bool result;
    string msg;

    const rapidjson::Value& feed = get_feed(cmr_doc);

    rapidjson::Value::ConstMemberIterator itr = feed.FindMember("entry");
    result  = itr != feed.MemberEnd();
    msg = prolog + (result?"Located":"FAILED to locate") + " the value 'entry'.";
    BESDEBUG(MODULE, msg << endl);
    if(!result){
        throw CmrError(msg,__FILE__,__LINE__);
    }

    const rapidjson::Value& entry = itr->value;
    result  = entry.IsArray();
    msg = prolog + "The value 'entry' is" + (result?"":" NOT") + " an Array.";
    BESDEBUG(MODULE, msg << endl);
    if(!result){
        throw CmrError(msg,__FILE__,__LINE__);
    }
    return entry;
}

/**
 *
 */
const rapidjson::Value&
CmrApi::get_temporal_group(const rapidjson::Document &cmr_doc){
    string prolog = string("CmrApi::") + __func__ + "() - ";
    rjson_utils ru;

    bool result;
    string msg;
    const rapidjson::Value& feed = get_feed(cmr_doc);

    //################### facets
    rapidjson::Value::ConstMemberIterator  itr = feed.FindMember("facets");
    result  = itr != feed.MemberEnd();
    msg =  prolog + (result?"Located":"FAILED to locate") + " the value 'facets'." ;
    BESDEBUG(MODULE, msg << endl);
    if(!result){
        throw CmrError(msg,__FILE__,__LINE__);
    }

    const rapidjson::Value& facets_obj = itr->value;
    result  = facets_obj.IsObject();
    msg =  prolog + "The value 'facets' is" + (result?"":" NOT") + " an object.";
    BESDEBUG(MODULE, msg << endl);
    if(!result){
        throw CmrError(msg,__FILE__,__LINE__);
    }

    const rapidjson::Value& facets = get_children(facets_obj);
    for (rapidjson::SizeType i = 0; i < facets.Size(); i++) { // Uses SizeType instead of size_t
        const rapidjson::Value& facet = facets[i];

        string facet_title = ru.getStringValue(facet,"title");
        string temporal_title("Temporal");
        if(facet_title == temporal_title){
            msg = prolog + "Found Temporal object.";
            BESDEBUG(MODULE, msg << endl);
            return facet;
        }
        else {
            msg = prolog + "The child of 'facets' with title '"+facet_title+"' does not match 'Temporal'";
            BESDEBUG(MODULE, msg << endl);
        }
    }
    msg = prolog + "Failed to locate the Temporal facet.";
    BESDEBUG(MODULE, msg << endl);
    throw CmrError(msg,__FILE__,__LINE__);

} // CmrApi::get_temporal_group()

/**
 *
 */
const rapidjson::Value&
CmrApi::get_year_group(const rapidjson::Document &cmr_doc){
    string prolog = string("CmrApi::") + __func__ + "() - ";
    rjson_utils rju;
    string msg;

    const rapidjson::Value& temporal_group = get_temporal_group(cmr_doc);
    const rapidjson::Value& temporal_children = get_children(temporal_group);
    for (rapidjson::SizeType j = 0; j < temporal_children.Size(); j++) { // Uses SizeType instead of size_t
        const rapidjson::Value& temporal_child = temporal_children[j];

        string temporal_child_title = rju.getStringValue(temporal_child,"title");
        string year_title("Year");
        if(temporal_child_title == year_title){
            msg = prolog + "Found Year object.";
            BESDEBUG(MODULE, msg << endl);
            return temporal_child;
        }
        else {
            msg = prolog + "The child of 'Temporal' with title '"+temporal_child_title+"' does not match 'Year'";
            BESDEBUG(MODULE, msg << endl);
        }
    }
    msg = prolog + "Failed to locate the Year group.";
    BESDEBUG(MODULE, msg << endl);
    throw CmrError(msg,__FILE__,__LINE__);
}

/**
 *
 */
const rapidjson::Value&
CmrApi::get_month_group(const string r_year, const rapidjson::Document &cmr_doc){
    string prolog = string("CmrApi::") + __func__ + "() - ";
    rjson_utils rju;
    string msg;

    const rapidjson::Value& year_group = get_year_group(cmr_doc);
    const rapidjson::Value& years = get_children(year_group);
    for (rapidjson::SizeType i = 0; i < years.Size(); i++) { // Uses SizeType instead of size_t
        const rapidjson::Value& year_obj = years[i];

        string year_title = rju.getStringValue(year_obj,"title");
        if(r_year == year_title){
            msg = prolog + "Found Year object.";
            BESDEBUG(MODULE, msg << endl);

            const rapidjson::Value& year_children = get_children(year_obj);
            for (rapidjson::SizeType j = 0; j < year_children.Size(); j++) { // Uses SizeType instead of size_t
                const rapidjson::Value& child = year_children[i];
                string title = rju.getStringValue(child,"title");
                string month_title("Month");
                if(title == month_title){
                    msg = prolog + "Found Month object.";
                    BESDEBUG(MODULE, msg << endl);
                    return child;
                }
                else {
                    msg = prolog + "The child of 'Year' with title '"+title+"' does not match 'Month'";
                    BESDEBUG(MODULE, msg << endl);
                }
            }
        }
        else {
            msg = prolog + "The child of 'Year' group with title '"+year_title+"' does not match the requested year ("+r_year+")";
            BESDEBUG(MODULE, msg << endl);
        }
    }
    msg = prolog + "Failed to locate the Year group.";
    BESDEBUG(MODULE, msg << endl);
    throw CmrError(msg,__FILE__,__LINE__);
}

const rapidjson::Value&
CmrApi::get_month(const string r_month, const string r_year, const rapidjson::Document &cmr_doc){
    string prolog = string("CmrApi::") + __func__ + "() - ";
    rjson_utils rju;
    stringstream msg;

    const rapidjson::Value& month_group = get_month_group(r_year,cmr_doc);
    const rapidjson::Value& months = get_children(month_group);
    for (rapidjson::SizeType i = 0; i < months.Size(); i++) { // Uses SizeType instead of size_t
        const rapidjson::Value& month = months[i];
        string month_id = rju.getStringValue(month,"title");
        if(month_id == r_month){
            msg.str("");
            msg << prolog  << "Located requested month ("<<r_month << ")";
            BESDEBUG(MODULE, msg << endl);
            return month;
        }
        else {
            msg.str("");
            msg << prolog  << "The month titled '"<<month_id << "' does not match the requested month ("<< r_month <<")";
            BESDEBUG(MODULE, msg << endl);
        }
    }
    msg.str("");
    msg << prolog  << "Failed to locate request Year/Month.";
    BESDEBUG(MODULE, msg << endl);
    throw CmrError(msg.str(),__FILE__,__LINE__);
}

const rapidjson::Value&
CmrApi::get_day_group(const string r_month, const string r_year, const rapidjson::Document &cmr_doc){
    string prolog = string("CmrApi::") + __func__ + "() - ";
    rjson_utils rju;
    stringstream msg;

    const rapidjson::Value& month = get_month(r_month, r_year, cmr_doc);
    const rapidjson::Value& month_children = get_children(month);

    for (rapidjson::SizeType k = 0; k < month_children.Size(); k++) { // Uses SizeType instead of size_t
        const rapidjson::Value& object = month_children[k];
        string title = rju.getStringValue(object,"title");
        string day_group_title = "Day";
        if(title == day_group_title){
            msg.str("");
            msg << prolog  << "Located Day group for year: " << r_year << " month: "<< r_month;
            BESDEBUG(MODULE, msg << endl);
            return object;
        }
    }
    msg.str("");
    msg << prolog  << "Failed to locate requested Day  year: " << r_year << " month: "<< r_month;
    BESDEBUG(MODULE, msg << endl);
    throw CmrError(msg.str(),__FILE__,__LINE__);
}


/**
 * Queries CMR for the 'collection_name' and returns the span of years covered by the collection.
 *
 * @param collection_name The name of the collection to query.
 * @param collection_years A vector into which the years will be placed.
 */
void
CmrApi::get_years(string collection_name, vector<string> &years_result){
    string prolog = string("CmrApi::") + __func__ + "() - ";
    rjson_utils rju;
    // bool result;
    string msg;

    string url = BESUtil::assemblePath(cmr_search_endpoint_url,"granules.json") + "?concept_id="+collection_name +"&include_facets=v2";
    rapidjson::Document doc;
    rju.getJsonDoc(url,doc);

    const rapidjson::Value& year_group = get_year_group(doc);
    const rapidjson::Value& years = get_children(year_group);
    for (rapidjson::SizeType k = 0; k < years.Size(); k++) { // Uses SizeType instead of size_t
        const rapidjson::Value& year_obj = years[k];
        string year = rju.getStringValue(year_obj,"title");
        years_result.push_back(year);
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
CmrApi::get_months(string collection_name, string r_year, vector<string> &months_result){
    string prolog = string("CmrApi::") + __func__ + "() - ";
    rjson_utils rju;

    stringstream msg;

    string url = BESUtil::assemblePath(cmr_search_endpoint_url,"granules.json")
        + "?concept_id="+collection_name
        +"&include_facets=v2"
        +"&temporal_facet[0][year]="+r_year;

    rapidjson::Document doc;
    rju.getJsonDoc(url,doc);
    BESDEBUG(MODULE, prolog << "Got JSON Document: "<< endl << rju.jsonDocToString(doc) << endl);

    const rapidjson::Value& year_group = get_year_group(doc);
    const rapidjson::Value& years = get_children(year_group);
    if(years.Size() != 1){
        msg.str("");
        msg << prolog  << "We expected to get back one year (" << r_year << ") but we got back " << years.Size();
        BESDEBUG(MODULE, msg << endl);
        throw CmrError(msg.str(),__FILE__,__LINE__);
    }

    const rapidjson::Value& year = years[0];
    string year_title = rju.getStringValue(year,"title");
    if(year_title != r_year){
        msg.str("");
        msg << prolog  << "The returned year (" << year_title << ") does not match the requested year ("<< r_year << ")";
        BESDEBUG(MODULE, msg << endl);
        throw CmrError(msg.str(),__FILE__,__LINE__);
    }

    const rapidjson::Value& year_children = get_children(year);
    if(year_children.Size() != 1){
        msg.str("");
        msg << prolog  << "We expected to get back one child for the year (" << r_year << ") but we got back " << years.Size();
        BESDEBUG(MODULE, msg << endl);
        throw CmrError(msg.str(),__FILE__,__LINE__);
    }

    const rapidjson::Value& month_group = year_children[0];
    string title = rju.getStringValue(month_group,"title");
    if(title != string("Month")){
        msg.str("");
        msg << prolog  << "We expected to get back a Month object, but we did not.";
        BESDEBUG(MODULE, msg << endl);
        throw CmrError(msg.str(),__FILE__,__LINE__);
    }

    const rapidjson::Value& months = get_children(month_group);
    for (rapidjson::SizeType i = 0; i < months.Size(); i++) { // Uses SizeType instead of size_t
        const rapidjson::Value& month = months[i];
        string month_id = rju.getStringValue(month,"title");
        months_result.push_back(month_id);
    }
    return;

} // CmrApi::get_months()

/**
 * Creates a list of the valid days for the collection matching the year and month
 */
void
CmrApi::get_days(string collection_name, string r_year, string r_month, vector<string> &days_result){
    string prolog = string("CmrApi::") + __func__ + "() - ";
    rjson_utils rju;
    stringstream msg;

    string url = BESUtil::assemblePath(cmr_search_endpoint_url,"granules.json")
        + "?concept_id="+collection_name
        +"&include_facets=v2"
        +"&temporal_facet[0][year]="+r_year
        +"&temporal_facet[0][month]="+r_month;

    rapidjson::Document cmr_doc;
    rju.getJsonDoc(url,cmr_doc);
    BESDEBUG(MODULE, prolog << "Got JSON Document: "<< endl << rju.jsonDocToString(cmr_doc) << endl);

    const rapidjson::Value& day_group = get_day_group(r_month, r_year, cmr_doc);
    const rapidjson::Value& days = get_children(day_group);
    for (rapidjson::SizeType i = 0; i < days.Size(); i++) { // Uses SizeType instead of size_t
        const rapidjson::Value& day = days[i];
        string day_id = rju.getStringValue(day,"title");
        days_result.push_back(day_id);
    }
}



/**
 *
 */
void
CmrApi::get_granule_ids(string collection_name, string r_year, string r_month, string r_day, vector<string> &granules_ids){
    string prolog = string("CmrApi::") + __func__ + "() - ";
    rjson_utils rju;
    stringstream msg;
    rapidjson::Document cmr_doc;

    granule_search(collection_name, r_year, r_month, r_day, cmr_doc);

    const rapidjson::Value& entries = get_entries(cmr_doc);
    for (rapidjson::SizeType i = 0; i < entries.Size(); i++) { // Uses SizeType instead of size_t
        const rapidjson::Value& granule = entries[i];
        string day_id = rju.getStringValue(granule,"id");
        granules_ids.push_back(day_id);
    }

}


/**
 *
 */
unsigned long
CmrApi::granule_count(string collection_name, string r_year, string r_month, string r_day){
    string prolog = string("CmrApi::") + __func__ + "() - ";
    stringstream msg;
    rapidjson::Document cmr_doc;
    granule_search(collection_name, r_year, r_month, r_day, cmr_doc);
    const rapidjson::Value& entries = get_entries(cmr_doc);
    return entries.Size();
}

/**
 * Locates granules in the collection matching the year, month, and day. Any or all of
 * year, month, and day may be the empty string.
 */
void
CmrApi::granule_search(string collection_name, string r_year, string r_month, string r_day, rapidjson::Document &result_doc){
    string prolog = string("CmrApi::") + __func__ + "() - ";
    rjson_utils rju;

    string url = BESUtil::assemblePath(cmr_search_endpoint_url,"granules.json")
        + "?concept_id="+collection_name
        + "&include_facets=v2"
        + "&page_size=2000";

    if(!r_year.empty())
        url += "&temporal_facet[0][year]="+r_year;

    if(!r_month.empty())
        url += "&temporal_facet[0][month]="+r_month;

    if(!r_day.empty())
        url += "&temporal_facet[0][day]="+r_day;

    BESDEBUG(MODULE, prolog << "CMR Granule Search Request Url: : " << url << endl);
    rju.getJsonDoc(url,result_doc);
    BESDEBUG(MODULE, prolog << "Got JSON Document: "<< endl << rju.jsonDocToString(result_doc) << endl);
}



/**
 * Returns all of the Granules in the collection matching the date.
 */
void
CmrApi::get_granules(string collection_name, string r_year, string r_month, string r_day, vector<Granule *> &granules){
    string prolog = string("CmrApi::") + __func__ + "() - ";
    stringstream msg;
    rapidjson::Document cmr_doc;

    granule_search(collection_name, r_year, r_month, r_day, cmr_doc);

    const rapidjson::Value& entries = get_entries(cmr_doc);
    for (rapidjson::SizeType i = 0; i < entries.Size(); i++) { // Uses SizeType instead of size_t
        const rapidjson::Value& granule_obj = entries[i];
        // rapidjson::Value grnl(granule_obj,cmr_doc.GetAllocator());
        Granule *g = new Granule(granule_obj);
        granules.push_back(g);
    }

}



#define CMR_COLLECTIONS "CMR.Collections"
void
CmrApi::get_collection_ids(std::vector<std::string> &collection_ids){
    bool found = false;
    string key = CMR_COLLECTIONS;
    TheBESKeys::TheKeys()->get_values(CMR_COLLECTIONS, collection_ids, found);
    if(!found){
        throw BESInternalError(string("The '") +CMR_COLLECTIONS
            + "' field has not been configured.", __FILE__, __LINE__);
    }
}


} // namespace cmr

