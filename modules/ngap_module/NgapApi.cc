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



#include <cstdio>
#include <cstring>
#include <iostream>
#include <sstream>
#include <memory>
#include <time.h>
#include <curl/curl.h>

#include <util.h>
#include <debug.h>

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/filereadstream.h"

#include "BESError.h"
#include "BESNotFoundError.h"
#include "BESSyntaxUserError.h"
#include "BESDebug.h"
#include "BESUtil.h"
#include "TheBESKeys.h"
#include "CurlUtils.h"
#include "RemoteResource.h"

#include "NgapApi.h"
#include "NgapNames.h"
#include "NgapError.h"

using namespace std;

#define prolog string("NgapApi::").append(__func__).append("() - ")

namespace ngap {

    const string NGAP_PROVIDER_KEY("providers");
    const string NGAP_COLLECTIONS_KEY("collections");
    const string NGAP_GRANULES_KEY("granules");
    const string DEFAULT_CMR_ENDPOINT_URL("https://cmr.earthdata.nasa.gov");
    const string DEFAULT_CMR_SEARCH_ENDPOINT_PATH("/search/granules.umm_json_v1_4");

    const string CMR_PROVIDER("provider");
    const string CMR_ENTRY_TITLE("entry_title");
    const string CMR_GRANULE_UR("granule_ur");
    const string CMR_URL_TYPE_GET_DATA("GET DATA");

    const string RJ_TYPE_NAMES[] = {
            "kNullType",
            "kFalseType",
            "kTrueType",
            "kObjectType",
            "kArrayType",
            "kStringType",
            "kNumberType"
    };

    const string AMS_EXPIRES_HEADER_KEY("X-Amz-Expires");
    const string AWS_DATE_HEADER_KEY("X-Amz-Date");
    // const string AWS_DATE_FORMAT("%Y%m%dT%H%MS"); // 20200624T175046Z
    const string CLOUDFRONT_EXPIRES_HEADER_KEY("Expires");
    const string INGEST_TIME_KEY("ingest_time");
    const unsigned int REFRESH_THRESHOLD = 3600; // An hour


    NgapApi::NgapApi() : d_cmr_hostname(DEFAULT_CMR_ENDPOINT_URL), d_cmr_search_endpoint_path(DEFAULT_CMR_SEARCH_ENDPOINT_PATH) {
        bool found;
        string cmr_hostnamer;
        TheBESKeys::TheKeys()->get_value(NGAP_CMR_HOSTNAME_KEY, cmr_hostnamer, found);
        if (found) {
            d_cmr_hostname = cmr_hostnamer;
        }

        string cmr_search_endpoint_path;
        TheBESKeys::TheKeys()->get_value(NGAP_CMR_SEARCH_ENDPOINT_PATH_KEY, cmr_search_endpoint_path, found);
        if (found) {
            d_cmr_search_endpoint_path = cmr_search_endpoint_path;
        }


    }

    std::string NgapApi::get_cmr_search_endpoint_url(){
        return BESUtil::assemblePath(d_cmr_hostname , d_cmr_search_endpoint_path);
    }

    /**
     * @brief Converts an NGAP restified granule path into a CMR metadata query for the granule.
     *
     * The NGAP module's "restified" interface utilizes a google-esque set of
     * ordered key value pairs using the "/" character as field seperatror.
     *
     * The NGAP container the "restified_path" will follow the template:
     *
     *   provider/daac_name/datasets/collection_name/granules/granule_name(s?)
     *
     * Where "provider", "datasets", and "granules" are NGAP keys and
     * "ddac_name", "collection_name", and "granule_name" the their respective values.
     *
     * For example, "provider/GHRC_CLOUD/datasets/ACES_CONTINUOUS_DATA_V1/granules/aces1cont.nc"
     *
     * https://cmr.earthdata.nasa.gov/search/granules.umm_json_v1_4?
     *   provider=GHRC_CLOUD &entry_title=ACES_CONTINUOUS_DATA_V1 &native_id=aces1cont.nc
     *   provider=GHRC_CLOUD &entry_title=ACES CONTINUOUS DATA V1 &native_id=aces1cont_2002.191_v2.50.tar
     *   provider=GHRC_CLOUD &native_id=olslit77.nov_analog.hdf &pretty=true
     *
     * @param restified_path The name to decompose.
     */
    string NgapApi::convert_ngap_resty_path_to_data_access_url(
            const std::string &restified_path,
            const std::string &uid,
            const std::string &access_token
    ) {

        string data_access_url("");

        vector<string> tokens;
        BESUtil::tokenize(restified_path, tokens);
        if (tokens.empty()) {
            throw BESSyntaxUserError(string("The specified path '") + restified_path +
                                     "' does not conform to the NGAP request interface API.", __FILE__, __LINE__);
        }

        // Check to make sure all required tokens are present.
        if (tokens[0] != NGAP_PROVIDER_KEY || tokens[2] != NGAP_COLLECTIONS_KEY || tokens[4] != NGAP_GRANULES_KEY) {
            throw BESSyntaxUserError(string("The specified path '") + restified_path +
                                     "' does not conform to the NGAP request interface API.", __FILE__, __LINE__);
        }
        // Pick up the values of said tokens.
        string cmr_url = get_cmr_search_endpoint_url() + "?";

        char error_buffer[CURL_ERROR_SIZE];
        CURL *curl = curl::init(error_buffer);  // This may throw either Error or InternalErr
        char *esc_url_content;

        esc_url_content = curl_easy_escape(curl, tokens[1].c_str(), tokens[1].size());
        cmr_url += CMR_PROVIDER + "=" + esc_url_content + "&";
        curl_free(esc_url_content);

        esc_url_content = curl_easy_escape(curl, tokens[3].c_str(), tokens[3].size());
        cmr_url += CMR_ENTRY_TITLE + "=" + esc_url_content + "&";
        curl_free(esc_url_content);

        esc_url_content = curl_easy_escape(curl, tokens[5].c_str(), tokens[5].size());
        cmr_url += CMR_GRANULE_UR + "=" + esc_url_content;
        curl_free(esc_url_content);
        curl_easy_cleanup(curl);


        BESDEBUG(MODULE, prolog << "CMR Request URL: " << cmr_url << endl);
#if 1
        BESDEBUG(MODULE, prolog << "Building new RemoteResource." << endl);
        http::RemoteResource cmr_query(cmr_url, uid, access_token);
        cmr_query.retrieveResource();
        rapidjson::Document cmr_response = cmr_query.get_as_json();
#else
        rapidjson::Document cmr_response = ngap_curl::http_get_as_json(cmr_url);
#endif

        rapidjson::Value &val = cmr_response["hits"];
        int hits = val.GetInt();
        if (hits < 1) {
            throw BESNotFoundError(string("The specified path '").append(restified_path).append(
                    "' does not identify a granule in CMR."), __FILE__, __LINE__);
        }

        rapidjson::Value &items = cmr_response["items"];
        if (items.IsArray()) {
            stringstream ss;
            for (rapidjson::SizeType i = 0; i < items.Size(); i++) // Uses SizeType instead of size_t
                ss << "items[" << i << "]: " << RJ_TYPE_NAMES[items[i].GetType()] << endl;

            BESDEBUG(MODULE, prolog << "items size: " << items.Size() << endl << ss.str() << endl);

            rapidjson::Value &items_obj = items[0];
            rapidjson::GenericMemberIterator<false, rapidjson::UTF8<char>, rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>> mitr = items_obj.FindMember(
                    "umm");

            rapidjson::Value &umm = mitr->value;
            mitr = umm.FindMember("RelatedUrls");
            if (mitr == umm.MemberEnd()) {
                throw BESInternalError("Error! The umm/RelatedUrls object was not located!", __FILE__, __LINE__);
            }
            rapidjson::Value &related_urls = mitr->value;

            if (!related_urls.IsArray()) {
                throw BESNotFoundError("Error! The RelatedUrls object in the CMR response is not an array!", __FILE__,
                                       __LINE__);
            }

            BESDEBUG(MODULE, prolog << " Found RelatedUrls array in CMR response." << endl);

            bool noSubtype;
            for (rapidjson::SizeType i = 0; i < related_urls.Size() && data_access_url.empty(); i++) {
                rapidjson::Value &obj = related_urls[i];
                mitr = obj.FindMember("URL");
                if (mitr == obj.MemberEnd()) {
                    stringstream err;
                    err << "Error! The umm/RelatedUrls[" << i << "] does not contain the URL object";
                    throw BESInternalError(err.str(), __FILE__, __LINE__);
                }
                rapidjson::Value &r_url = mitr->value;

                mitr = obj.FindMember("Type");
                if (mitr == obj.MemberEnd()) {
                    stringstream err;
                    err << "Error! The umm/RelatedUrls[" << i << "] does not contain the Type object";
                    throw BESInternalError(err.str(), __FILE__, __LINE__);
                }
                rapidjson::Value &r_type = mitr->value;

                noSubtype = obj.FindMember("Subtype") == obj.MemberEnd();
#if 0
                mitr = obj.FindMember("Description");
                if(mitr == obj.MemberEnd()){
                    stringstream  err;
                    err << "Error! The umm/RelatedUrls[" << i << "] does not contain the Description object";
                    throw BESInternalError(err.str(), __FILE__, __LINE__);
                }
                rapidjson::Value& r_desc = mitr->value;
#endif
                BESDEBUG(MODULE, prolog << "RelatedUrl Object:" <<
                                        " URL: '" << r_url.GetString() << "'" <<
                                        " Type: '" << r_type.GetString() << "'" <<
                                        " SubType: '" << (noSubtype ? "Absent" : "Present") << "'" << endl);

                if ((r_type.GetString() == CMR_URL_TYPE_GET_DATA) && noSubtype) {
                    data_access_url = r_url.GetString();
                }
            }
        }

        if (data_access_url.empty()) {
            throw BESInternalError(string("ERROR! Failed to locate a data access URL for the path: ") + restified_path,
                                   __FILE__, __LINE__);
        }

        BESDEBUG(MODULE, prolog << "Following data_access_url redirects..." << endl);

        return data_access_url;
    }




    bool NgapApi::signed_url_is_expired(const std::map<std::string,std::string> &ngap_url_info)
    {
        bool is_expired;
        time_t now;
        time(&now);  /* get current time; same as: timer = time(NULL)  */
        BESDEBUG(MODULE, prolog << "now: " << now << endl);

        time_t expires = now;
        std::map<string,string>::const_iterator cfexpires_it = ngap_url_info.find(CLOUDFRONT_EXPIRES_HEADER_KEY);
        std::map<string,string>::const_iterator awsexpires_it = ngap_url_info.find(AMS_EXPIRES_HEADER_KEY);
        std::map<string,string>::const_iterator ingest_time_it = ngap_url_info.find(INGEST_TIME_KEY);

        if(cfexpires_it != ngap_url_info.end()){ // CloudFront expires header?
            expires = stoll(cfexpires_it->second);
            BESDEBUG(MODULE, prolog << "Using "<< CLOUDFRONT_EXPIRES_HEADER_KEY << ": " << expires << endl);
        }
        else if(awsexpires_it != ngap_url_info.end() && ingest_time_it != ngap_url_info.end()){
            // AWS Expires header?
            //
            // By default we'll use the time we read the probe response, ingest_time
            time_t start_time = stoll(ingest_time_it->second);
            // But if there's an AWS Date we'll parse that and compute the time
            // @TODO move to NgapApi::decompose_url() and add the result to the map
            std::map<string,string>::const_iterator awsdate_it = ngap_url_info.find(AWS_DATE_HEADER_KEY);
            if(awsdate_it != ngap_url_info.end()){
                string date = awsdate_it->second; // 20200624T175046Z
                string year = date.substr(0,4);
                string month = date.substr(4,2);
                string day = date.substr(6,2);
                string hour = date.substr(9,2);
                string minute = date.substr(11,2);
                string second = date.substr(13,2);

                BESDEBUG(MODULE, prolog << "date: "<< date <<
                                        " year: " << year << " month: " << month << " day: " << day <<
                                        " hour: " << hour << " minute: " << minute  << " second: " << second << endl);

                struct tm *ti = gmtime(&now);
                ti->tm_year = stoll(year);
                ti->tm_mon = stoll(month) - 1;
                ti->tm_mday = stoll(day);
                ti->tm_hour = stoll(hour);
                ti->tm_min = stoll(minute);
                ti->tm_sec = stoll(second);

                BESDEBUG(MODULE, prolog << "ti->tm_year: "<< ti->tm_year <<
                                        " ti->tm_mon: " << ti->tm_mon <<
                                        " ti->tm_mday: " << ti->tm_mday <<
                                        " ti->tm_hour: " << ti->tm_hour <<
                                        " ti->tm_min: " << ti->tm_min <<
                                        " ti->tm_sec: " << ti->tm_sec << endl);


               // start_time = mktime(&ti);
               //  BESDEBUG(MODULE, prolog << "AWS (computed) start_time: "<< start_time << endl);
            }
            expires = start_time + stoll(awsexpires_it->second);
            BESDEBUG(MODULE, prolog << "Using "<< AMS_EXPIRES_HEADER_KEY << ": " << awsexpires_it->second <<
                                    " (expires: " << expires << ")" << endl);
        }
        time_t remaining = expires - now;
        BESDEBUG(MODULE, prolog << "expires: " << expires <<
                                "  remaining: " << remaining <<
                                " threshold: " << REFRESH_THRESHOLD << endl);

        is_expired = remaining < REFRESH_THRESHOLD;
        BESDEBUG(MODULE, prolog << "is_expired: " << (is_expired?"true":"false") << endl);

        return is_expired;
    }



#if 0
    string NgapApi::convert_ngap_resty_path_to_data_access_url(string real_name){
    string data_access_url("");

    vector<string> tokens;
    BESUtil::tokenize(real_name,tokens);
    if( tokens[0]!= NGAP_PROVIDER_KEY || tokens[2]!=NGAP_DATASETS_KEY || tokens[4]!=NGAP_GRANULES_KEY){
        string err = (string) "The specified path " + real_name
                     + " does not conform to the NGAP request interface API.";
        throw BESSyntaxUserError(err, __FILE__, __LINE__);
    }

    string cmr_url = cmr_granule_search_endpoint_url + "?";
    cmr_url += CMR_PROVIDER + "=" + tokens[1] + "&";
    cmr_url += CMR_ENTRY_TITLE + "=" + tokens[3] + "&";
    cmr_url += CMR_GRANULE_UR + "=" + tokens[5] ;
    BESDEBUG( MODULE, prolog << "CMR Request URL: "<< cmr_url << endl );
    rapidjson::Document cmr_response = ngap_curl::http_get_as_json(cmr_url);

    rapidjson::Value& val = cmr_response["hits"];
    int hits = val.GetInt();
    if(hits < 1){
        string err = (string) "The specified path " + real_name
                     + " does not identify a thing we know about....";
        throw BESNotFoundError(err, __FILE__, __LINE__);
    }

    rapidjson::Value& items = cmr_response["items"];
    if(items.IsArray()){
        stringstream ss;
        for (rapidjson::SizeType i = 0; i < items.Size(); i++) // Uses SizeType instead of size_t
            ss << "items[" << i << "]: " << rjtype_names[items[i].GetType()] << endl;
        BESDEBUG(MODULE,prolog << "items size: " << items.Size() << endl << ss.str() << endl);

        rapidjson::Value& items_obj = items[0];
        rapidjson::GenericMemberIterator<false, rapidjson::UTF8<char>, rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>> mitr = items_obj.FindMember("umm");

        rapidjson::Value& umm = mitr->value;
        mitr  = umm.FindMember("RelatedUrls");
        rapidjson::Value& related_urls = mitr->value;

        if(!related_urls.IsArray()){
            string err = (string) "Error! The RelatedUrls object in the CMR response is not an array!";
            throw BESNotFoundError(err, __FILE__, __LINE__);
        }

        BESDEBUG(MODULE,prolog << " Found RelatedUrls array in CMR response." << endl);

        bool noSubtype;

        for (rapidjson::SizeType i = 0; i < related_urls.Size() && data_access_url.empty(); i++)  {
            rapidjson::Value& obj = related_urls[i];
            mitr = obj.FindMember("URL");
            rapidjson::Value& r_url = mitr->value;
            mitr = obj.FindMember("Type");
            rapidjson::Value& r_type = mitr->value;
            noSubtype = ((mitr = obj.FindMember("Subtype")) == obj.MemberEnd()) ? true : false;
            mitr = obj.FindMember("Description");
            rapidjson::Value& r_desc = mitr->value;
            BESDEBUG(MODULE,prolog << "RelatedUrl Object:" <<
                                   " URL: '" << r_url.GetString() << "'" <<
                                   " Type: '" << r_type.GetString() << "'" <<
                                   //" Subtype: '" << r_subtype.GetString() << "'" <<
                                   " Description: '" << r_desc.GetString() <<  "'" << endl);

            if( (r_type.GetString() == CMR_URL_TYPE_GET_DATA) && noSubtype ){
                data_access_url = r_url.GetString();
            }
        }

    }

    return data_access_url + ".dmrpp";
}
#endif

#if 0
    /**
     *
     */
        const rapidjson::Value&
        NgapApi::get_children(const rapidjson::Value& obj) {
            rapidjson::Value::ConstMemberIterator itr;

            itr = obj.FindMember("children");
            bool result  = itr != obj.MemberEnd();
            string msg = prolog + (result?"Located":"FAILED to locate") + " the value 'children' in the object.";
            BESDEBUG(MODULE, msg << endl);
            if(!result){
                throw NgapError(msg,__FILE__,__LINE__);
            }

            const rapidjson::Value& children = itr->value;
            result = children.IsArray();
            msg = prolog + "The value 'children' is" + (result?"":" NOT") + " an array.";
            BESDEBUG(MODULE, msg << endl);
            if(!result){
                throw NgapError(msg,__FILE__,__LINE__);
            }
            return children;
        }

    /**
     *
     */
        const rapidjson::Value&
        NgapApi::get_feed(const rapidjson::Document &ngap_doc){

            bool result = ngap_doc.IsObject();
            string msg = prolog + "Json document is" + (result?"":" NOT") + " an object.";
            BESDEBUG(MODULE, msg << endl);
            if(!result){
                throw NgapError(msg,__FILE__,__LINE__);
            }

            //################### feed
            rapidjson::Value::ConstMemberIterator itr = ngap_doc.FindMember("feed");
            result  = itr != ngap_doc.MemberEnd();
            msg = prolog + (result?"Located":"FAILED to locate") + " the value 'feed'.";
            BESDEBUG(MODULE, msg << endl);
            if(!result){
                throw NgapError(msg,__FILE__,__LINE__);
            }

            const rapidjson::Value& feed = itr->value;
            result  = feed.IsObject();
            msg = prolog + "The value 'feed' is" + (result?"":" NOT") + " an object.";
            BESDEBUG(MODULE, msg << endl);
            if(!result){
                throw NgapError(msg,__FILE__,__LINE__);
            }
            return feed;
        }

    /**
     *
     */
        const rapidjson::Value&
        NgapApi::get_entries(const rapidjson::Document &ngap_doc){
            bool result;
            string msg;

            const rapidjson::Value& feed = get_feed(ngap_doc);

            rapidjson::Value::ConstMemberIterator itr = feed.FindMember("entry");
            result  = itr != feed.MemberEnd();
            msg = prolog + (result?"Located":"FAILED to locate") + " the value 'entry'.";
            BESDEBUG(MODULE, msg << endl);
            if(!result){
                throw NgapError(msg,__FILE__,__LINE__);
            }

            const rapidjson::Value& entry = itr->value;
            result  = entry.IsArray();
            msg = prolog + "The value 'entry' is" + (result?"":" NOT") + " an Array.";
            BESDEBUG(MODULE, msg << endl);
            if(!result){
                throw NgapError(msg,__FILE__,__LINE__);
            }
            return entry;
        }

    /**
     *
     */
        const rapidjson::Value&
        NgapApi::get_temporal_group(const rapidjson::Document &ngap_doc){
            rjson_utils ru;

            bool result;
            string msg;
            const rapidjson::Value& feed = get_feed(ngap_doc);

            //################### facets
            rapidjson::Value::ConstMemberIterator  itr = feed.FindMember("facets");
            result  = itr != feed.MemberEnd();
            msg =  prolog + (result?"Located":"FAILED to locate") + " the value 'facets'." ;
            BESDEBUG(MODULE, msg << endl);
            if(!result){
                throw NgapError(msg,__FILE__,__LINE__);
            }

            const rapidjson::Value& facets_obj = itr->value;
            result  = facets_obj.IsObject();
            msg =  prolog + "The value 'facets' is" + (result?"":" NOT") + " an object.";
            BESDEBUG(MODULE, msg << endl);
            if(!result){
                throw NgapError(msg,__FILE__,__LINE__);
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
            throw NgapError(msg,__FILE__,__LINE__);

        } // NgapApi::get_temporal_group()

    /**
     *
     */
        const rapidjson::Value&
        NgapApi::get_year_group(const rapidjson::Document &ngap_doc){
            rjson_utils rju;
            string msg;

            const rapidjson::Value& temporal_group = get_temporal_group(ngap_doc);
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
            throw NgapError(msg,__FILE__,__LINE__);
        }

    /**
     *
     */
        const rapidjson::Value&
        NgapApi::get_month_group(const string r_year, const rapidjson::Document &ngap_doc){
            rjson_utils rju;
            string msg;

            const rapidjson::Value& year_group = get_year_group(ngap_doc);
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
            throw NgapError(msg,__FILE__,__LINE__);
        }

        const rapidjson::Value&
        NgapApi::get_month(const string r_month, const string r_year, const rapidjson::Document &ngap_doc){
            rjson_utils rju;
            stringstream msg;

            const rapidjson::Value& month_group = get_month_group(r_year,ngap_doc);
            const rapidjson::Value& months = get_children(month_group);
            for (rapidjson::SizeType i = 0; i < months.Size(); i++) { // Uses SizeType instead of size_t
                const rapidjson::Value& month = months[i];
                string month_id = rju.getStringValue(month,"title");
                if(month_id == r_month){
                    msg.str("");
                    msg << prolog  << "Located requested month ("<<r_month << ")";
                    BESDEBUG(MODULE, msg.str() << endl);
                    return month;
                }
                else {
                    msg.str("");
                    msg << prolog  << "The month titled '"<<month_id << "' does not match the requested month ("<< r_month <<")";
                    BESDEBUG(MODULE, msg.str() << endl);
                }
            }
            msg.str("");
            msg << prolog  << "Failed to locate request Year/Month.";
            BESDEBUG(MODULE, msg.str() << endl);
            throw NgapError(msg.str(),__FILE__,__LINE__);
        }

        const rapidjson::Value&
        NgapApi::get_day_group(const string r_month, const string r_year, const rapidjson::Document &ngap_doc){
            rjson_utils rju;
            stringstream msg;

            const rapidjson::Value& month = get_month(r_month, r_year, ngap_doc);
            const rapidjson::Value& month_children = get_children(month);

            for (rapidjson::SizeType k = 0; k < month_children.Size(); k++) { // Uses SizeType instead of size_t
                const rapidjson::Value& object = month_children[k];
                string title = rju.getStringValue(object,"title");
                string day_group_title = "Day";
                if(title == day_group_title){
                    msg.str("");
                    msg << prolog  << "Located Day group for year: " << r_year << " month: "<< r_month;
                    BESDEBUG(MODULE, msg.str() << endl);
                    return object;
                }
            }
            msg.str("");
            msg << prolog  << "Failed to locate requested Day  year: " << r_year << " month: "<< r_month;
            BESDEBUG(MODULE, msg.str() << endl);
            throw NgapError(msg.str(),__FILE__,__LINE__);
        }


    /**
     * Queries NGAP for the 'collection_name' and returns the span of years covered by the collection.
     *
     * @param collection_name The name of the collection to query.
     * @param collection_years A vector into which the years will be placed.
     */
        void
        NgapApi::get_years(string collection_name, vector<string> &years_result){
            rjson_utils rju;
            // bool result;
            string msg;

            string url = BESUtil::assemblePath(ngap_search_endpoint_url,"granules.json") + "?concept_id="+collection_name +"&include_facets=v2";
            rapidjson::Document doc;
            rju.getJsonDoc(url,doc);

            const rapidjson::Value& year_group = get_year_group(doc);
            const rapidjson::Value& years = get_children(year_group);
            for (rapidjson::SizeType k = 0; k < years.Size(); k++) { // Uses SizeType instead of size_t
                const rapidjson::Value& year_obj = years[k];
                string year = rju.getStringValue(year_obj,"title");
                years_result.push_back(year);
            }
        } // NgapApi::get_years()


    /**
     * Queries NGAP for the 'collection_name' and returns the span of years covered by the collection.
     *
     * https://cmr.earthdata.nasa.gov/search/granules.json?concept_id=C179003030-ORNL_DAAC&include_facets=v2&temporal_facet%5B0%5D%5Byear%5D=1985
     *
     * @param collection_name The name of the collection to query.
     * @param collection_years A vector into which the years will be placed.
     */
        void
        NgapApi::get_months(string collection_name, string r_year, vector<string> &months_result){
            rjson_utils rju;

            stringstream msg;

            string url = BESUtil::assemblePath(ngap_search_endpoint_url,"granules.json")
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
                BESDEBUG(MODULE, msg.str() << endl);
                throw NgapError(msg.str(),__FILE__,__LINE__);
            }

            const rapidjson::Value& year = years[0];
            string year_title = rju.getStringValue(year,"title");
            if(year_title != r_year){
                msg.str("");
                msg << prolog  << "The returned year (" << year_title << ") does not match the requested year ("<< r_year << ")";
                BESDEBUG(MODULE, msg.str() << endl);
                throw NgapError(msg.str(),__FILE__,__LINE__);
            }

            const rapidjson::Value& year_children = get_children(year);
            if(year_children.Size() != 1){
                msg.str("");
                msg << prolog  << "We expected to get back one child for the year (" << r_year << ") but we got back " << years.Size();
                BESDEBUG(MODULE, msg.str() << endl);
                throw NgapError(msg.str(),__FILE__,__LINE__);
            }

            const rapidjson::Value& month_group = year_children[0];
            string title = rju.getStringValue(month_group,"title");
            if(title != string("Month")){
                msg.str("");
                msg << prolog  << "We expected to get back a Month object, but we did not.";
                BESDEBUG(MODULE, msg.str() << endl);
                throw NgapError(msg.str(),__FILE__,__LINE__);
            }

            const rapidjson::Value& months = get_children(month_group);
            for (rapidjson::SizeType i = 0; i < months.Size(); i++) { // Uses SizeType instead of size_t
                const rapidjson::Value& month = months[i];
                string month_id = rju.getStringValue(month,"title");
                months_result.push_back(month_id);
            }
            return;

        } // NgapApi::get_months()

    /**
     * Creates a list of the valid days for the collection matching the year and month
     */
        void
        NgapApi::get_days(string collection_name, string r_year, string r_month, vector<string> &days_result){
            rjson_utils rju;
            stringstream msg;

            string url = BESUtil::assemblePath(ngap_search_endpoint_url,"granules.json")
                         + "?concept_id="+collection_name
                         +"&include_facets=v2"
                         +"&temporal_facet[0][year]="+r_year
                         +"&temporal_facet[0][month]="+r_month;

            rapidjson::Document ngap_doc;
            rju.getJsonDoc(url,ngap_doc);
            BESDEBUG(MODULE, prolog << "Got JSON Document: "<< endl << rju.jsonDocToString(ngap_doc) << endl);

            const rapidjson::Value& day_group = get_day_group(r_month, r_year, ngap_doc);
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
        NgapApi::get_granule_ids(string collection_name, string r_year, string r_month, string r_day, vector<string> &granules_ids){
            rjson_utils rju;
            stringstream msg;
            rapidjson::Document ngap_doc;

            granule_search(collection_name, r_year, r_month, r_day, ngap_doc);

            const rapidjson::Value& entries = get_entries(ngap_doc);
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
        NgapApi::granule_count(string collection_name, string r_year, string r_month, string r_day){
            stringstream msg;
            rapidjson::Document ngap_doc;
            granule_search(collection_name, r_year, r_month, r_day, ngap_doc);
            const rapidjson::Value& entries = get_entries(ngap_doc);
            return entries.Size();
        }

    /**
     * Locates granules in the collection matching the year, month, and day. Any or all of
     * year, month, and day may be the empty string.
     */
        void
        NgapApi::granule_search(string collection_name, string r_year, string r_month, string r_day, rapidjson::Document &result_doc){
            rjson_utils rju;

            string url = BESUtil::assemblePath(ngap_search_endpoint_url,"granules.json")
                         + "?concept_id="+collection_name
                         + "&include_facets=v2"
                         + "&page_size=2000";

            if(!r_year.empty())
                url += "&temporal_facet[0][year]="+r_year;

            if(!r_month.empty())
                url += "&temporal_facet[0][month]="+r_month;

            if(!r_day.empty())
                url += "&temporal_facet[0][day]="+r_day;

            BESDEBUG(MODULE, prolog << "ngap Granule Search Request Url: : " << url << endl);
            rju.getJsonDoc(url,result_doc);
            BESDEBUG(MODULE, prolog << "Got JSON Document: "<< endl << rju.jsonDocToString(result_doc) << endl);
        }



    /**
     * Returns all of the Granules in the collection matching the date.
     */
        void
        NgapApi::get_granules(string collection_name, string r_year, string r_month, string r_day, vector<Granule *> &granules){
            stringstream msg;
            rapidjson::Document ngap_doc;

            granule_search(collection_name, r_year, r_month, r_day, ngap_doc);

            const rapidjson::Value& entries = get_entries(ngap_doc);
            for (rapidjson::SizeType i = 0; i < entries.Size(); i++) { // Uses SizeType instead of size_t
                const rapidjson::Value& granule_obj = entries[i];
                // rapidjson::Value grnl(granule_obj, ngap_doc.GetAllocator());
                Granule *g = new Granule(granule_obj);
                granules.push_back(g);
            }

        }


        void
        NgapApi::get_collection_ids(std::vector<std::string> &collection_ids){
            bool found = false;
            string key = NGAP_COLLECTIONS;
            TheBESKeys::TheKeys()->get_values(NGAP_COLLECTIONS, collection_ids, found);
            if(!found){
                throw BESInternalError(string("The '") +NGAP_COLLECTIONS
                                       + "' field has not been configured.", __FILE__, __LINE__);
            }
        }

    /**
     * Returns all of the Granules in the collection matching the date.
     */
        Granule* NgapApi::get_granule(string collection_name, string r_year, string r_month, string r_day, string granule_id)
        {
            vector<Granule *> granules;
            Granule *result = 0;

            get_granules(collection_name, r_year, r_month, r_day, granules);
            for(size_t i=0; i<granules.size() ;i++){
                string id = granules[i]->getName();
                BESDEBUG(MODULE, prolog << "Comparing granule id: " << granule_id << " to collection member id: " << id << endl);
                if( id == granule_id){
                    result = granules[i];
                }
                else {
                    delete granules[i];
                    granules[i] = 0;
                }
            }
            return result;
        }
#endif

} // namespace ngap

