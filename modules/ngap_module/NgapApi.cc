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

#include "config.h"

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
#include "BESStopWatch.h"
#include "BESLog.h"
#include "TheBESKeys.h"
#include "CurlUtils.h"
#include "url_impl.h"
#include "RemoteResource.h"

#include "NgapApi.h"
#include "NgapNames.h"
#include "NgapError.h"

using namespace std;

#define prolog string("NgapApi::").append(__func__).append("() - ")

namespace ngap {

const unsigned int REFRESH_THRESHOLD = 3600; // An hour


NgapApi::NgapApi() : d_cmr_hostname(DEFAULT_CMR_ENDPOINT_URL), d_cmr_search_endpoint_path(DEFAULT_CMR_SEARCH_ENDPOINT_PATH) {
    bool found;
    string cmr_hostname;
    TheBESKeys::TheKeys()->get_value(NGAP_CMR_HOSTNAME_KEY, cmr_hostname, found);
    if (found) {
        d_cmr_hostname = cmr_hostname;
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
 * @brief Converts an NGAP restified path into the corresponding CMR query URL.
 *
 * @param restified_path The resitified path to convert
 * @return The CMR query URL that will return the granules.umm_json_v1_4 from CMR for the
 * granule specified in the restified path.
 */
std::string NgapApi::build_cmr_query_url_old_rpath_format(const std::string &restified_path) {

    // Make sure it starts with a '/' (see key strings above)
    string r_path = ( restified_path[0] != '/' ? "/" : "") + restified_path;

    size_t provider_index  = r_path.find(NGAP_PROVIDERS_KEY);
    if(provider_index == string::npos){
        stringstream msg;
        msg << prolog << "The specified path '" << r_path << "'";
        msg << " does not contain the required path element '" << NGAP_PROVIDERS_KEY << "'";
        throw BESSyntaxUserError(msg.str(), __FILE__, __LINE__);
    }
    if(provider_index != 0){
        stringstream msg;
        msg << prolog << "The specified path '" << r_path << "'";
        msg << " has the path element '" << NGAP_PROVIDERS_KEY << "' located in the incorrect position (";
        msg << provider_index << ") expected 0.";
        throw BESSyntaxUserError(msg.str(), __FILE__, __LINE__);
    }
    provider_index += string(NGAP_PROVIDERS_KEY).length();

    bool use_collection_concept_id = false;
    size_t collection_index  = r_path.find(NGAP_COLLECTIONS_KEY);
    if(collection_index == string::npos) {
        size_t concepts_index = r_path.find(NGAP_CONCEPTS_KEY);
        if (concepts_index == string::npos) {
            stringstream msg;
            msg << prolog << "The specified path '" << r_path << "'";
            msg << " contains neither the '" << NGAP_COLLECTIONS_KEY << "'";
            msg << " nor the '" << NGAP_CONCEPTS_KEY << "'";
            msg << " key, one must be provided.";
            throw BESSyntaxUserError(msg.str(), __FILE__, __LINE__);
        }
        collection_index = concepts_index;
        use_collection_concept_id = true;
    }
    if(collection_index <= provider_index+1){  // The value of provider has to be at least 1 character
        stringstream msg;
        msg << prolog << "The specified path '" << r_path << "'";
        msg << " has the path element '" << (use_collection_concept_id?NGAP_CONCEPTS_KEY:NGAP_COLLECTIONS_KEY) << "' located in the incorrect position (";
        msg << collection_index << ") expected at least " << provider_index+1;
        throw BESSyntaxUserError(msg.str(), __FILE__, __LINE__);
    }
    string provider = r_path.substr(provider_index,collection_index - provider_index);
    collection_index += use_collection_concept_id?string(NGAP_CONCEPTS_KEY).length():string(NGAP_COLLECTIONS_KEY).length();


    size_t granule_index  = r_path.find(NGAP_GRANULES_KEY);
    if(granule_index == string::npos){
        stringstream msg;
        msg << prolog << "The specified path '" << r_path << "'";
        msg << " does not contain the required path element '" << NGAP_GRANULES_KEY << "'";
        throw BESSyntaxUserError(msg.str(), __FILE__, __LINE__);
    }
    if(granule_index <= collection_index+1){ // The value of collection must have at least one character.
        stringstream msg;
        msg << prolog << "The specified path '" << r_path << "'";
        msg << " has the path element '" << NGAP_GRANULES_KEY << "' located in the incorrect position (";
        msg << granule_index << ") expected at least " << collection_index+1;
        throw BESSyntaxUserError(msg.str(), __FILE__, __LINE__);
    }
    string collection = r_path.substr(collection_index,granule_index - collection_index);
    granule_index += string(NGAP_GRANULES_KEY).length();

    // The granule value is the path terminus so it's every thing after the key
    string granule = r_path.substr(granule_index);

    // Build the CMR query URL for the dataset
    string cmr_url = get_cmr_search_endpoint_url() + "?";
    {
        // This easy handle is only created so we can use the curl_easy_escape() on the token values
        CURL *ceh = curl_easy_init();
        char *esc_url_content;

        // Add provider
        esc_url_content = curl_easy_escape(ceh, provider.c_str(), provider.size());
        cmr_url += string(CMR_PROVIDER).append("=").append(esc_url_content).append("&");
        curl_free(esc_url_content);

        esc_url_content = curl_easy_escape(ceh, collection.c_str(), collection.size());
        if(use_collection_concept_id){
            // Add collection_concept_id
            cmr_url += string(CMR_COLLECTION_CONCEPT_ID).append("=").append(esc_url_content).append("&");
        }
        else {
            // Add entry_title
            cmr_url += string(CMR_ENTRY_TITLE).append("=").append(esc_url_content).append("&");

        }
        curl_free(esc_url_content);

        esc_url_content = curl_easy_escape(ceh, granule.c_str(), granule.size());
        cmr_url += string(CMR_GRANULE_UR).append("=").append(esc_url_content);
        curl_free(esc_url_content);

        curl_easy_cleanup(ceh);
    }
    return cmr_url;
}

/**
 * @brief Converts an NGAP restified path into the corresponding CMR query URL.
 *
 * There are two mandatory and one optional query parameters in the URL
 *   MANDATORY: " /collections/UMM-C:{concept-id} "
 *   OPTIONAL:  "/UMM-C:{ShortName} '.' UMM-C:{Version} "
 *   MANDATORY: "/granules/UMM-G:{GranuleUR}"
 * Example:
 * https://opendap.earthdata.nasa.gov/collections/C1443727145-LAADS/MOD08_D3.v6.1/granules/MOD08_D3.A2020308.061.2020309092644.hdf.nc
 *
 * More Info Here: https://wiki.earthdata.nasa.gov/display/DUTRAIN/Feature+analysis%3A+Restified+URL+for+OPENDAP+Data+Access
 *
 * @param restified_path The resitified path to convert
 * @return The CMR query URL that will return the granules.umm_json_v1_4 from CMR for the
 * granule specified in the restified path.
 */
std::string NgapApi::build_cmr_query_url(const std::string &restified_path) {

    // Make sure it starts with a '/' (see key strings above)
    string r_path = ( restified_path[0] != '/' ? "/" : "") + restified_path;

    size_t provider_index  = r_path.find(NGAP_PROVIDERS_KEY);
    if(provider_index != string::npos){
        return build_cmr_query_url_old_rpath_format(restified_path);
    }

    size_t collections_key_index  = r_path.find(NGAP_COLLECTIONS_KEY);
    if(collections_key_index == string::npos) {
        stringstream msg;
        msg << prolog << "The specified path '" << r_path << "'";
        msg << " contains neither the '" << NGAP_COLLECTIONS_KEY << "'";
        msg << " nor the '" << NGAP_CONCEPTS_KEY << "'";
        msg << " one must be provided.";
        throw BESSyntaxUserError(msg.str(), __FILE__, __LINE__);
    }
    if(collections_key_index != 0){  // The COLLECTIONS_KEY comes first
        stringstream msg;
        msg << prolog << "The specified path '" << r_path << "'";
        msg << " has the path element '" << NGAP_COLLECTIONS_KEY << "' located in the incorrect position (";
        msg << collections_key_index << ") expected at least " << provider_index + 1;
        throw BESSyntaxUserError(msg.str(), __FILE__, __LINE__);
    }
    // This is now the beginning of the collection_concept_id value.
    size_t collections_index =  collections_key_index + string(NGAP_COLLECTIONS_KEY).length();

    size_t granules_key_index  = r_path.find(NGAP_GRANULES_KEY);
    if(granules_key_index == string::npos){
        stringstream msg;
        msg << prolog << "The specified path '" << r_path << "'";
        msg << " does not contain the required path element '" << NGAP_GRANULES_KEY << "'";
        throw BESSyntaxUserError(msg.str(), __FILE__, __LINE__);
    }

    // The collection key must precede the granules key in the path,
    // and the collection name must have at least one character.
    if(granules_key_index <= collections_index + 1){
        stringstream msg;
        msg << prolog << "The specified path '" << r_path << "'";
        msg << " has the path element '" << NGAP_GRANULES_KEY << "' located in the incorrect position (";
        msg << granules_key_index << ") expected at least " << collections_index + 1;
        throw BESSyntaxUserError(msg.str(), __FILE__, __LINE__);
    }
    size_t granules_index =  granules_key_index + string(NGAP_GRANULES_KEY).length();
    // The granule_name value is the path terminus so it's every thing after the key
    string granule_name = r_path.substr(granules_index);

    // Now we need to work on the collections value to eliminate the optional parts.
    // This is the entire collections string including any optional components.
    string collection_name = r_path.substr(collections_index, granules_key_index - collections_index);

    // Since there may be optional parameters we need to strip them off to get the collection_concept_id
    // And, since we know that collection_concept_id will never contain a '/', and we know that the optional
    // part is separated from the collection_concept_id by a '/' we look for that and of we find it we truncate
    // the value at that spot.
    string optional_part;
    size_t slash_pos  = collection_name.find('/');
    if(slash_pos != string::npos){
        optional_part = collection_name.substr(slash_pos);
        BESDEBUG(MODULE, prolog << "Found optional collections name component: " << optional_part << endl);
        collection_name = collection_name.substr(0,slash_pos);
    }
    BESDEBUG(MODULE, prolog << "Found collection_name (aka collection_concept_id): " << collection_name << endl);

    // Build the CMR query URL for the dataset
    string cmr_url = get_cmr_search_endpoint_url() + "?";
    {
        // This easy handle is only created so we can use the curl_easy_escape() on the token values
        CURL *ceh = curl_easy_init();
        char *esc_url_content;

        esc_url_content = curl_easy_escape(ceh, collection_name.c_str(), collection_name.size());
        cmr_url += string(CMR_COLLECTION_CONCEPT_ID).append("=").append(esc_url_content).append("&");
        curl_free(esc_url_content);

        esc_url_content = curl_easy_escape(ceh, granule_name.c_str(), granule_name.size());
        cmr_url += string(CMR_GRANULE_UR).append("=").append(esc_url_content);
        curl_free(esc_url_content);

        curl_easy_cleanup(ceh);
    }
    return cmr_url;
}

/**
 * @brief  Locates the "GET DATA" URL for a granule in the granules.umm_json_v1_4 document.
 *
 * A single granule query is built by convert_restified_path_to_cmr_query_url() from the
 * NGAP API restified path. This method will parse the CMR response to the query and extract the
 * granule's "GET DATA" URL and return it.
 * @param restified_path THe restified path used to form the CMR query
 * @param cmr_granules The CMR response (granules.umm_json_v1_4) to evaluate
 * @return  The "GET DATA" URL for the granule.
 */
std::string NgapApi::find_get_data_url_in_granules_umm_json_v1_4(const std::string &restified_path, rapidjson::Document &cmr_granule_response)
{

    string data_access_url;

    rapidjson::Value &val = cmr_granule_response["hits"];
    int hits = val.GetInt();
    if (hits < 1) {
        throw BESNotFoundError(string("The specified path '").append(restified_path).append(
                "' does not identify a granule in CMR."), __FILE__, __LINE__);
    }

    rapidjson::Value &items = cmr_granule_response["items"];
    if (items.IsArray()) {
        stringstream ss;
        if(BESDebug::IsSet(MODULE)){
            const string RJ_TYPE_NAMES[] = {string("kNullType"),string("kFalseType"),string("kTrueType"),
                    string("kObjectType"),string("kArrayType"),string("kStringType"),string("kNumberType")};
            for (rapidjson::SizeType i = 0; i < items.Size(); i++) // Uses SizeType instead of size_t
                ss << "items[" << i << "]: " << RJ_TYPE_NAMES[items[i].GetType()] << endl;
            BESDEBUG(MODULE, prolog << "items size: " << items.Size() << endl << ss.str() << endl);
        }

        rapidjson::Value &items_obj = items[0];
        //  rapidjson::GenericMemberIterator<false, rapidjson::UTF8<char>, rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>> mitr = items_obj.FindMember("umm");
        auto mitr = items_obj.FindMember("umm");

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

            BESDEBUG(MODULE, prolog << "RelatedUrl Object:" <<
                                    " URL: '" << r_url.GetString() << "'" <<
                                    " Type: '" << r_type.GetString() << "'" <<
                                    " SubType: '" << (noSubtype ? "Absent" : "Present") << "'" << endl);

            if ((r_type.GetString() == string(CMR_URL_TYPE_GET_DATA)) && noSubtype) {
                data_access_url = r_url.GetString();
            }
        }
    }

    if (data_access_url.empty()) {
        throw BESInternalError(string("ERROR! Failed to locate a data access URL for the path: ") + restified_path,
                               __FILE__, __LINE__);
    }

    return data_access_url;
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
            const std::string &uid
            ) {
        BESDEBUG(MODULE, prolog << "BEGIN" << endl);
        string data_access_url;

        string cmr_query_url = build_cmr_query_url(restified_path);

        BESDEBUG(MODULE, prolog << "CMR Request URL: " << cmr_query_url << endl);

        BESDEBUG(MODULE, prolog << "Building new RemoteResource." << endl);
        std::shared_ptr<http::url> cmr_query_url_ptr(new http::url(cmr_query_url));
        http::RemoteResource cmr_query(cmr_query_url_ptr, uid);
        {
            BESStopWatch besTimer;
            if (BESISDEBUG(MODULE) || BESDebug::IsSet(TIMING_LOG_KEY) || BESLog::TheLog()->is_verbose()){
                besTimer.start("CMR Query: " + cmr_query_url);
            }
            cmr_query.retrieveResource();
        }
        rapidjson::Document cmr_response = cmr_query.get_as_json();

        data_access_url = find_get_data_url_in_granules_umm_json_v1_4(restified_path, cmr_response);

        BESDEBUG(MODULE, prolog << "END (data_access_url: "<< data_access_url << ")" << endl);

        return data_access_url;
    }




    bool NgapApi::signed_url_is_expired(const http::url &signed_url)
    {
        bool is_expired;
        time_t now;
        time(&now);  /* get current time; same as: timer = time(NULL)  */
        BESDEBUG(MODULE, prolog << "now: " << now << endl);

        time_t expires = now;
        string cf_expires = signed_url.query_parameter_value(CLOUDFRONT_EXPIRES_HEADER_KEY);
        string aws_expires = signed_url.query_parameter_value(AMS_EXPIRES_HEADER_KEY);
        time_t ingest_time = signed_url.ingest_time();

        if(!cf_expires.empty()){ // CloudFront expires header?
            expires = stoll(cf_expires);
            BESDEBUG(MODULE, prolog << "Using "<< CLOUDFRONT_EXPIRES_HEADER_KEY << ": " << expires << endl);
        }
        else if(!aws_expires.empty()){
            // AWS Expires header?
            //
            // By default we'll use the time we made the URL object, ingest_time
            time_t start_time = ingest_time;
            // But if there's an AWS Date we'll parse that and compute the time
            // @TODO move to NgapApi::decompose_url() and add the result to the map
            string aws_date = signed_url.query_parameter_value(AWS_DATE_HEADER_KEY);
            if(!aws_date.empty()){
                string date = aws_date; // 20200624T175046Z
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
                ti->tm_year = stoll(year) - 1900;
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


                start_time = mktime(ti);
                BESDEBUG(MODULE, prolog << "AWS (computed) start_time: "<< start_time << endl);
            }
            expires = start_time + stoll(aws_expires);
            BESDEBUG(MODULE, prolog << "Using "<< AMS_EXPIRES_HEADER_KEY << ": " << aws_expires <<
                                    " (expires: " << expires << ")" << endl);
        }
        time_t remaining = expires - now;
        BESDEBUG(MODULE, prolog << "expires_time: " << expires <<
                                "  remaining_time: " << remaining <<
                                " refresh_threshold: " << REFRESH_THRESHOLD << endl);

        is_expired = remaining < REFRESH_THRESHOLD;
        BESDEBUG(MODULE, prolog << "is_expired: " << (is_expired?"true":"false") << endl);

        return is_expired;
    }

} // namespace ngap

