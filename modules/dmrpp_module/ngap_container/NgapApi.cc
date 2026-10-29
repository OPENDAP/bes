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

#include <sstream>
#include <ctime>
#include <memory>

#include <curl/curl.h>
#include "rapidjson/document.h"
#include "rapidjson/error/en.h"

#include "BESNotFoundError.h"
#include "BESSyntaxUserError.h"
#include "BESInternalError.h"
#include "BESContextManager.h"
#include "BESDebug.h"
#include "BESStopWatch.h"
#include "BESUtil.h"
#include "TheBESKeys.h"
#include "CurlUtils.h"
#include "HttpError.h"

#include "NgapApi.h"
#include "NgapNames.h"
#include "NgapServiceChaining.h"

using namespace std;

#define prolog string("NgapApi::").append(__func__).append("() - ")

namespace ngap {

namespace {
/// @return True if 's' ends with 'suffix'. std::string::ends_with() is C++20.
bool ends_with(const string &s, const string &suffix) {
    return s.size() >= suffix.size() && s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
}

/**
 * @brief Get a string member of a umm/RelatedUrls element.
 * rapidjson only asserts its preconditions, and NDEBUG builds don't check them, so test
 * them here and throw instead (BUG-002).
 * @return The value of the member
 * @exception BESInternalError if 'obj' is not an object or the member is missing or not a string
 */
string get_string_member(const rapidjson::Value &obj, const char *name) {
    if (!obj.IsObject()) {
        throw BESInternalError("The umm/RelatedUrls element is not an object", __FILE__, __LINE__);
    }
    auto mitr = obj.FindMember(name);
    if (mitr == obj.MemberEnd()) {
        throw BESInternalError(string("The umm/RelatedUrls element does not contain the ") + name + " object",
                               __FILE__, __LINE__);
    }
    if (!mitr->value.IsString()) {
        throw BESInternalError(string("The umm/RelatedUrls element's ") + name + " is not a string",
                               __FILE__, __LINE__);
    }
    return mitr->value.GetString();
}

using curl_easy_handle = std::unique_ptr<CURL, decltype(&curl_easy_cleanup)>;

/// @return A new curl easy handle, used only for curl_easy_escape() and curl_easy_unescape().
curl_easy_handle make_curl_easy_handle() {
    curl_easy_handle ceh(curl_easy_init(), &curl_easy_cleanup);
    if (!ceh) {
        throw BESInternalError("Could not initialize a curl easy handle.", __FILE__, __LINE__);
    }
    return ceh;
}

/// @return 's' URL-encoded.
string url_escape(CURL *ceh, const string &s) {
    std::unique_ptr<char, decltype(&curl_free)> escaped(
            curl_easy_escape(ceh, s.c_str(), static_cast<int>(s.size())), &curl_free);
    if (!escaped) {
        throw BESInternalError("Could not URL-encode '" + s + "'.", __FILE__, __LINE__);
    }
    return {escaped.get()};
}

/// @return 's' URL-decoded.
string url_unescape(CURL *ceh, const string &s) {
    int out_len = 0;
    std::unique_ptr<char, decltype(&curl_free)> unescaped(
            curl_easy_unescape(ceh, s.c_str(), static_cast<int>(s.size()), &out_len), &curl_free);
    if (!unescaped) {
        throw BESInternalError("Could not URL-decode '" + s + "'.", __FILE__, __LINE__);
    }
    return {unescaped.get(), static_cast<size_t>(out_len)};
}
} // namespace

/**
 * @brief Get the CMR search endpoint URL using information from the BES Keys.
 * The BES keys are read on every call.
 * @return The URL used to access CMR.
 */
string NgapApi::get_cmr_search_endpoint_url() {
    const string cmr_hostname = TheBESKeys::read_string_key(NGAP_CMR_HOSTNAME_KEY, DEFAULT_CMR_ENDPOINT_URL);
    const string cmr_search_endpoint_path = TheBESKeys::read_string_key(NGAP_CMR_SEARCH_ENDPOINT_PATH_KEY,
                                                                        DEFAULT_CMR_SEARCH_ENDPOINT_PATH);
    return BESUtil::assemblePath(cmr_hostname, cmr_search_endpoint_path);
}

/**
 * @brief Converts an NGAP restified path into the corresponding CMR query URL.
 *
 * @param restified_path The restified path to convert
 * @return The CMR query URL that will return the granules.umm_json_v1_4 from CMR for the
 * granule specified in the restified path.
 */
string NgapApi::build_cmr_query_url_old_rpath_format(const string &restified_path) {

    // Make sure it starts with a '/' (see key strings above)
    string r_path = (restified_path[0] != '/' ? "/" : "") + restified_path;

    size_t provider_index = r_path.find(NGAP_PROVIDERS_KEY);
    if (provider_index == string::npos) {
        stringstream msg;
        msg << prolog << "The specified path '" << r_path << "'";
        msg << " does not contain the required path element '" << NGAP_PROVIDERS_KEY << "'";
        throw BESSyntaxUserError(msg.str(), __FILE__, __LINE__);
    }
    if (provider_index != 0) {
        stringstream msg;
        msg << prolog << "The specified path '" << r_path << "'";
        msg << " has the path element '" << NGAP_PROVIDERS_KEY << "' located in the incorrect position, expected 0.";
        throw BESSyntaxUserError(msg.str(), __FILE__, __LINE__);
    }
    provider_index += string(NGAP_PROVIDERS_KEY).size();

    bool use_collection_concept_id = false;
    size_t collection_index = r_path.find(NGAP_COLLECTIONS_KEY);
    if (collection_index == string::npos) {
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
    if (collection_index <= provider_index + 1) {  // The value of provider has to be at least 1 character
        stringstream msg;
        msg << prolog << "The specified path '" << r_path << "'";
        msg << " has the path element '" << (use_collection_concept_id ? NGAP_CONCEPTS_KEY : NGAP_COLLECTIONS_KEY)
            << "' located in the incorrect position (";
        msg << collection_index << ") expected at least " << provider_index + 1;
        throw BESSyntaxUserError(msg.str(), __FILE__, __LINE__);
    }
    string provider = r_path.substr(provider_index, collection_index - provider_index);
    collection_index += use_collection_concept_id ? string(NGAP_CONCEPTS_KEY).size() : string(
            NGAP_COLLECTIONS_KEY).size();

    size_t granule_index = r_path.find(NGAP_GRANULES_KEY);
    if (granule_index == string::npos) {
        stringstream msg;
        msg << prolog << "The specified path '" << r_path << "'";
        msg << " does not contain the required path element '" << NGAP_GRANULES_KEY << "'";
        throw BESSyntaxUserError(msg.str(), __FILE__, __LINE__);
    }
    if (granule_index <= collection_index + 1) { // The value of collection must have at least one character.
        stringstream msg;
        msg << prolog << "The specified path '" << r_path << "'";
        msg << " has the path element '" << NGAP_GRANULES_KEY << "' located in the incorrect position,";
        throw BESSyntaxUserError(msg.str(), __FILE__, __LINE__);
    }
    string collection = r_path.substr(collection_index, granule_index - collection_index);
    granule_index += string(NGAP_GRANULES_KEY).size();

    // The granule value is the path terminus, so it's every thing after the key
    string granule = r_path.substr(granule_index);

    // Build the CMR query URL for the dataset
    string cmr_url = get_cmr_search_endpoint_url() + "?";
    {
        // This easy-handle is only created so we can use the curl_easy_escape() on the token values
        auto ceh = make_curl_easy_handle();

        // Add provider
        cmr_url += string(CMR_PROVIDER).append("=").append(url_escape(ceh.get(), url_unescape(ceh.get(), provider))).append("&");

        const string escaped_collection = url_escape(ceh.get(), url_unescape(ceh.get(), collection));
        if (use_collection_concept_id) {
            // Add collection_concept_id
            cmr_url += string(CMR_COLLECTION_CONCEPT_ID).append("=").append(escaped_collection).append("&");
        } else {
            // Add entry_title
            cmr_url += string(CMR_ENTRY_TITLE).append("=").append(escaped_collection).append("&");
        }

        cmr_url += string(CMR_GRANULE_UR).append("=").append(url_escape(ceh.get(), url_unescape(ceh.get(), granule)));
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
 * @param restified_path The restified path to convert
 * @return The CMR query URL that will return the granules.umm_json_v1_4 from CMR for the
 * granule specified in the restified path.
 */
string NgapApi::build_cmr_query_url(const string &restified_path) {

    // Make sure it starts with a '/' (see key strings above)
    string r_path = (restified_path[0] != '/' ? "/" : "") + restified_path;

    size_t provider_index = r_path.find(NGAP_PROVIDERS_KEY);
    if (provider_index != string::npos) {
        return build_cmr_query_url_old_rpath_format(restified_path);
    }

    size_t collections_key_index = r_path.find(NGAP_COLLECTIONS_KEY);
    if (collections_key_index == string::npos) {
        stringstream msg;
        msg << prolog << "The specified path '" << r_path << "'";
        msg << " contains neither the '" << NGAP_COLLECTIONS_KEY << "'";
        msg << " nor the '" << NGAP_CONCEPTS_KEY << "'";
        msg << " one must be provided.";
        throw BESSyntaxUserError(msg.str(), __FILE__, __LINE__);
    }
    if (collections_key_index != 0) {  // The COLLECTIONS_KEY comes first
        stringstream msg;
        msg << prolog << "The specified path '" << r_path << "'";
        msg << " has the path element '" << NGAP_COLLECTIONS_KEY << "' located in the incorrect position,";
        msg << " expected at least " << provider_index + 1;
        throw BESSyntaxUserError(msg.str(), __FILE__, __LINE__);
    }
    // This is now the beginning of the collection_concept_id value.
    size_t collections_index = collections_key_index + string(NGAP_COLLECTIONS_KEY).size();

    size_t granules_key_index = r_path.find(NGAP_GRANULES_KEY);
    if (granules_key_index == string::npos) {
        stringstream msg;
        msg << prolog << "The specified path '" << r_path << "'";
        msg << " does not contain the required path element '" << NGAP_GRANULES_KEY << "'";
        throw BESSyntaxUserError(msg.str(), __FILE__, __LINE__);
    }

    // The collection key must precede the granules key in the path,
    // and the collection name must have at least one character.
    if (granules_key_index <= collections_index + 1) {
        stringstream msg;
        msg << prolog << "The specified path '" << r_path << "'";
        msg << " has the path element '" << NGAP_GRANULES_KEY << "' located in the incorrect position (";
        msg << granules_key_index << ") expected at least " << collections_index + 1;
        throw BESSyntaxUserError(msg.str(), __FILE__, __LINE__);
    }
    size_t granules_index = granules_key_index + string(NGAP_GRANULES_KEY).size();
    // The granule_name value is the path terminus, so it's every thing after the key
    string granule_name = r_path.substr(granules_index);

    // Now we need to work on the collection value to eliminate the optional parts.
    // This is the entire collections string including any optional components.
    string collection_name = r_path.substr(collections_index, granules_key_index - collections_index);

    // Since there may be optional parameters, we need to strip them off to get the collection_concept_id, since
    // we know that collection_concept_id will never contain a '/', and we know that the optional
    // part is separated from the collection_concept_id by a '/', we look for that and if we find it, we truncate
    // the value at that spot.
    size_t slash_pos = collection_name.find('/');
    if (slash_pos != string::npos) {
        collection_name = collection_name.substr(0, slash_pos);
    }
    BESDEBUG(MODULE, prolog << "Found collection_name (aka collection_concept_id): " << collection_name << endl);

    // Build the CMR query URL for the dataset
    string cmr_url = get_cmr_search_endpoint_url() + "?";

    {
        // This easy-handle is only created so we can use the curl_easy_escape() on the token values
        auto ceh = make_curl_easy_handle();

        cmr_url += string(CMR_COLLECTION_CONCEPT_ID).append("=").append(url_escape(ceh.get(), collection_name)).append("&");
        cmr_url += string(CMR_GRANULE_UR).append("=").append(url_escape(ceh.get(), granule_name));
    }

    return cmr_url;
}

string get_data_http_url(rapidjson::Value &obj) {
    const string candidate_url = get_string_member(obj, "URL");
    const string r_type = get_string_member(obj, "Type");

    bool noSubtype = obj.FindMember("Subtype") == obj.MemberEnd();

    if ((r_type == CMR_URL_TYPE_GET_DATA) && noSubtype) {
        // Added test that the URL does not end in 'xml' to avoid the LPDAAC .cmr.xml records. jhrg 5/22/24
        // The URL has to start with HTTP/S and cannot end in .xml or .dmrpp (FR-007). jhrg 6/2/25
        if ((candidate_url.find("https://") == 0 || candidate_url.find("http://") == 0)
            && !ends_with(candidate_url, ".xml")
            && !ends_with(candidate_url, ".dmrpp")) {
            return candidate_url;
        }
    }

    return {""};
}


string get_data_s3_url(rapidjson::Value &obj) {
    const string candidate_url = get_string_member(obj, "URL");
    const string r_type = get_string_member(obj, "Type");

    bool noSubtype = obj.FindMember("Subtype") == obj.MemberEnd();

    if ((r_type == CMR_URL_TYPE_GET_DATA) && noSubtype) {
        // The URL has to start with s3:// and cannot end in .xml or .dmrpp (FR-008)
        if (candidate_url.find("s3://") == 0
            && !ends_with(candidate_url, ".xml")
            && !ends_with(candidate_url, ".dmrpp")) {
            return candidate_url;
        }
    }

    return {""};
}

string get_s3credentials_url(rapidjson::Value &obj) {
    const string candidate_url = get_string_member(obj, "URL");
    const string r_type = get_string_member(obj, "Type");

    if (r_type == CMR_URL_TYPE_GET_S3CREDENTIALS) {
        // The URL has to start with HTTP/S and must end in s3credentials
        if ((candidate_url.find("https://") == 0 || candidate_url.find("http://") == 0)
            && ends_with(candidate_url, "s3credentials")) {
            return candidate_url;
        }
    }

    return {""};
}

/**
 * @brief  Locates the "GET DATA" URL, "GET DATA" s3 URL, and "USE SERVICE API" s3 credentials endpoint
 * URL for a granule in the granules.umm_json_v1_4 document.
 *
 * A single granule query is built by convert_restified_path_to_cmr_query_url() from the
 * NGAP API restified path. This method will parse the CMR response to the query and extract the
 * granule's data and s3 credentials urls and return them.
 *
 * @note This method uses heuristics to get each url for the granule from the CMR UMM-G JSON.
 *  For each url, the process it follows is look in the RelatedUrls array for an entry with the following contstraints:
 *  - For the `GET DATA` URL, a TYPE of Type 'GET DATA' with a URL that uses the https:// protocol 
 *    where that URL does not end in 'xml'. 
 *    The latter characteristic was added for records added by LPDAAC. jhrg 5/22/24
 *  - For the `GET DATA` S3 URL, a TYPE of Type 'GET DATA' with a URL that uses the s3:// protocol 
 *    where that URL does not end in 'xml'. 
 *  - For the `GET DATA` S3 URL, a TYPE of Type 'VIEW RELATED INFORMATION' with a URL that uses the https:// protocol 
 *    where that URL ends in 's3credentials'. 
 *
 * @param rest_path The REST path used to form the CMR query (only used for error messages)
 * @param cmr_granule_response The CMR response (granules.umm_json_v1_4) to evaluate
 * @return  A tuple of data urls for the granule: the "GET DATA" URL, "GET DATA" s3 URL, and "USE SERVICE API" s3 CREDENTIALS URL.
 */
NgapApi::DataAccessUrls NgapApi::get_urls_from_granules_umm_json_v1_4(const std::string &rest_path,
                                                                      const std::string &cmr_granule_json_string) {

    // rapidjson only asserts its preconditions, and NDEBUG builds don't check them, so each
    // access below is checked first. A malformed response is an internal error (BUG-002).
    rapidjson::Document cmr_granule_response;
    cmr_granule_response.Parse(cmr_granule_json_string.c_str());
    if (cmr_granule_response.HasParseError()) {
        throw BESInternalError(string("The CMR response for '") + rest_path + "' is not valid JSON: "
                               + rapidjson::GetParseError_En(cmr_granule_response.GetParseError())
                               + " (offset " + to_string(cmr_granule_response.GetErrorOffset()) + ")",
                               __FILE__, __LINE__);
    }
    if (!cmr_granule_response.IsObject()) {
        throw BESInternalError(string("The CMR response for '") + rest_path + "' is not a JSON object.",
                               __FILE__, __LINE__);
    }

    auto mitr = cmr_granule_response.FindMember("hits");
    if (mitr == cmr_granule_response.MemberEnd() || !mitr->value.IsInt()) {
        throw BESInternalError(string("The CMR response for '") + rest_path + "' has no integer 'hits' value.",
                               __FILE__, __LINE__);
    }
    int hits = mitr->value.GetInt();
    if (hits < 1) {
        throw BESNotFoundError(string("The specified path '") + rest_path
                               + "' does not identify a granule in CMR.", __FILE__, __LINE__);
    }

    mitr = cmr_granule_response.FindMember("items");
    if (mitr == cmr_granule_response.MemberEnd() || !mitr->value.IsArray()) {
        throw BESInternalError(string("ERROR! The CMR response did not contain the data URL information: ")
                               + rest_path, __FILE__, __LINE__);
    }
    rapidjson::Value &items = mitr->value;
    if (items.Empty() || !items[0].IsObject()) {
        throw BESInternalError(string("The CMR response for '") + rest_path + "' has no granule item.",
                               __FILE__, __LINE__);
    }

    // JSON is now vetted so that we know it has an array of one or more 'items'. jhrg 6/2/25
    rapidjson::Value &items_obj = items[0];
    mitr = items_obj.FindMember("umm");
    if (mitr == items_obj.MemberEnd() || !mitr->value.IsObject()) {
        throw BESInternalError(string("The CMR response for '") + rest_path + "' has no umm object.",
                               __FILE__, __LINE__);
    }
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

    // The first element of 'items' is now vetted so that we know it's an array of 'RelatedUrls'.
    string data_http_url;
    string data_s3_url;
    string tea_endpoint_url;
    for (rapidjson::SizeType i = 0; i < related_urls.Size(); i++) {
        if (data_http_url.empty()) {
            data_http_url = get_data_http_url(related_urls[i]);
        }
        if (data_s3_url.empty()) {
            data_s3_url = get_data_s3_url(related_urls[i]);
        }
        if (tea_endpoint_url.empty()) {
            tea_endpoint_url = get_s3credentials_url(related_urls[i]);
        }
        if (!data_http_url.empty() && !data_s3_url.empty() && !tea_endpoint_url.empty()) {
            break;
        }
    }

    // If we have enough information to get data later, our work here is done
    // For now, all we care about is a non-empty data_http_url. In the future this will likely change.
    if (!data_http_url.empty()) {
        return std::make_tuple(data_http_url, data_s3_url, tea_endpoint_url);
    }

    // If no valid related-URL is found, it's an error.
    throw BESInternalError(string("Failed to locate a data URL for the path: ") + rest_path,
                           __FILE__, __LINE__);
}

/**
 * @brief Remove a trailing '.dmrpp' from a data URL, if present.
 *
 * This is defensive. Under FR-007/FR-008, get_urls_from_granules_umm_json_v1_4() never
 * selects a data URL that ends in '.dmrpp', so this should not change the URL. It guards
 * against a future change to that filter; NgapOwnedContainer appends '.dmrpp' to the data
 * URLs to locate the DMR++ (FR-011).
 *
 * @param url value-result parameter; the URL to trim
 */
void NgapApi::strip_dmrpp_suffix(string &url) {
    const string suffix = ".dmrpp";
    if (ends_with(url, suffix)) {
        url.erase(url.size() - suffix.size());
    }
}

/**
 * @brief Converts an NGAP restified granule path into a CMR metadata query for the granule.
 *
 * The NGAP module's "restified" interface uses a google-esque set of
 * ordered key value pairs using the "/" character as field separator.
 *
 * The NGAP container the "restified_path" will follow the template:
 *
 *   provider/daac_name/datasets/collection_name/granules/granule_name(s?)
 *
 * Where "provider", "datasets", and "granules" are NGAP keys and
 * "ddac_name", "collection_name", and "granule_name" their respective values.
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
NgapApi::DataAccessUrls NgapApi::convert_ngap_resty_path_to_data_access_urls(const string &restified_path) {
    BESDEBUG(MODULE, prolog << "BEGIN" << endl);

    string cmr_query_url = build_cmr_query_url(restified_path);

    BESDEBUG(MODULE, prolog << "CMR Request URL: " << cmr_query_url << endl);

    string cmr_json_string;
    try {
        BES_DBG_PROFILE_TIMING(string("Request granule record from CMR - ") + cmr_query_url);
        // We know that CMR is an EDL authenticated situation, so we create those headers
        // (if available) and pass them into the http_get() call
        curl::http_get(cmr_query_url, cmr_json_string, curl::add_edl_auth_headers(nullptr));
    }
    catch (http::HttpError &http_error) {
        if (is_http_5xx(http_error.http_status()))
            add_service_chaining_message(http_error, prolog, "attempting to retrieve a CMR record.");
        else
            http_error.set_message(prolog + "Error while attempting to retrieve a CMR record. "
                                   + http_error.get_message());
        throw;
    }

    string data_access_url, data_s3_url, tea_endpoint_url;
    tie(data_access_url, data_s3_url, tea_endpoint_url) = get_urls_from_granules_umm_json_v1_4(restified_path, cmr_json_string);

    if (data_s3_url.empty() || tea_endpoint_url.empty()) {
        // Eventually we may remove the non-s3 access; we want to know about any unsupported cases before that happens.
        // Add a log warning that can be searched.
        INFO_LOG(prolog + string("SERVICE CHAIN WARNING - Data s3 url and/or s3credentials url not found - ") + cmr_query_url + " s3 url: " + data_s3_url + " s3credentials url: " + tea_endpoint_url);
    }

    // Check for existing .dmrpp and remove it if found at the end of the url. - kln 6/6/25
    strip_dmrpp_suffix(data_access_url);
    strip_dmrpp_suffix(data_s3_url);

    BESDEBUG(MODULE, prolog << "END (data_access_url: " << data_access_url << ", data_s3_url: " << data_s3_url << ", tea_endpoint_url: " << tea_endpoint_url << ")" << endl);

    return std::make_tuple(data_access_url, data_s3_url, tea_endpoint_url);
}

} // namespace ngap

