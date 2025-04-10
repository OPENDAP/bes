// NgapOwnedContainer.cc

// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of ngap_module, A C++ module that can be loaded in to
// the OPeNDAP Back-End Server (BES) and is able to handle remote requests.

// Copyright (c) 2020, 2024 OPeNDAP, Inc.
// Author: Nathan Potter <ndp@opendap.org>
//         James Gallagher <jgallagher@opendap.org>
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
// Authors:
//      ndp       Nathan Potter <ndp@opendap.org>
//      jhrg      James Gallagher <jgallagher@opendap.org>

#include "config.h"

#include <sys/stat.h>
#include <unistd.h>

#include <sstream>
#include <string>

#include "BESStopWatch.h"
#include "BESUtil.h"
#include "CurlUtils.h"
#include "AWS_SDK.h"
#include "BESContextManager.h"
#include "HttpError.h"
#include "BESLog.h"
#include "TheBESKeys.h"
#include "BESSyntaxUserError.h"
#include "BESDebug.h"

#include "NgapRequestHandler.h"
#include "NgapOwnedContainer.h"
#include "NgapApi.h"
#include "NgapNames.h"

#define prolog std::string("NgapOwnedContainer::").append(__func__).append("() - ")
// CACHE_LOG is defined separately from INFO_LOG so that we can turn it off easily. jhrg 11/19/23
#define CACHE_LOG(x) INFO_LOG(x)

using namespace std;
using namespace bes;

namespace ngap {

// This data source location currently (8/10/24) is a S3 bucket where the DMR++ files are stored
// for the OPeNDAP-owned data used by the tests. jhrg 8/10/24
std::string NgapOwnedContainer::d_data_source_location = "cloudydap";
bool NgapOwnedContainer::d_use_opendap_bucket = true;
bool NgapOwnedContainer::d_inject_data_url = true;

/**
 * @brief Creates an instances of NgapOwnedContainer with symbolic name and real
 * name, which is the remote request.
 *
 * The real_name is the remote request URL.
 *
 * @param sym_name symbolic name representing this remote container
 * @param real_name The NGAP REST path.
 * @throws BESSyntaxUserError if the url does not validate
 * @see NgapUtils
 */
NgapOwnedContainer::NgapOwnedContainer(const string &sym_name, const string &real_name, const string &)
        : BESContainer(sym_name, real_name, "owned-ngap"), d_ngap_path(real_name) {
    NgapOwnedContainer::d_data_source_location
        = TheBESKeys::read_string_key(DATA_SOURCE_LOCATION, NgapOwnedContainer::d_data_source_location);
    NgapOwnedContainer::d_use_opendap_bucket
        = TheBESKeys::read_bool_key(USE_OPENDAP_BUCKET, NgapOwnedContainer::d_use_opendap_bucket);
    NgapOwnedContainer::d_inject_data_url
        = TheBESKeys::read_bool_key(NGAP_INJECT_DATA_URL_KEY, NgapOwnedContainer::d_inject_data_url);
}

/**
 * @brief Read data from a file descriptor into a string.
 * @param fd The file descriptor to read from.
 * @param content The string to read the data into.
 * @return True if all the bytes were read, false otherwise.
 */
bool NgapOwnedContainer::file_to_string(int fd, string &content) {
    // The file size is needed later; this doubles as a check that the file in open.
    struct stat statbuf = {};
    if (fstat(fd, &statbuf) < 0) {
        ERROR_LOG("NgapOwnedContainer::file_to_string() - failed to get file descriptor status\n");
        return false;
    }

    // read the data in 4k chunks
    vector<char> buffer(4096);
    ssize_t bytes_read;
    while ((bytes_read = read(fd, buffer.data(), buffer.size())) > 0) {
        content.append(buffer.data(), bytes_read);
    }

    // did we get it all
    if (statbuf.st_size != content.size()) {
        ERROR_LOG("NgapOwnedContainer::file_to_string() - failed to read all bytes from file cache\n");
        return false;
    }

    return true;
}

/**
 * @brief Set the real name of the container using the CMR or cache.
 *
 * This uses CMR to translate a REST path to a true NGAP URL for the granule.
 * Once this is done, the result is cached in an unordered_map keyed using the the
 * REST path and the UID. The REST path is initially the value of the
 * real_name property of the Container, but the action of performing the translation
 * changes the name of the real_name property to the true NGAP URL. The REST
 * path is stored in the d_ngap_path property of the NgapContainer instance.
 *
 * @note: The cache is global to the NgapRequestHandler class. This is a per-process
 * cache and it is not thread-safe.
 */
string NgapOwnedContainer::build_data_url_to_daac_bucket(const string &rest_path) {
    BES_MODULE_TIMING(prolog + rest_path);

    bool found;
    string uid = BESContextManager::TheManager()->get_context(EDL_UID_KEY, found);
    BESDEBUG(MODULE, prolog << "EDL_UID_KEY(" << EDL_UID_KEY << "): " << uid << endl);

    // If using the cache, look there. Note that the UID is part of the key to the cached data.
    string url_key = rest_path + ':' + uid;
    string data_url;
    if (NgapRequestHandler::d_use_cmr_cache) {
        if (NgapRequestHandler::d_cmr_mem_cache.get(url_key, data_url)) {
            CACHE_LOG(prolog + "CMR Cache hit, translated URL: " + data_url + '\n');
            return data_url;
        } else {
            CACHE_LOG(prolog + "CMR Cache miss, REST path: " + url_key + '\n');
        }
    }

    // Not cached or not using the cache; ask CMR. Throws on lookup failure, HTTP failure. jhrg 1/24/25
    data_url = NgapApi::convert_ngap_resty_path_to_data_access_url(rest_path);

    // If using the CMR cache, cache the response.
    if (NgapRequestHandler::d_use_cmr_cache) {
        NgapRequestHandler::d_cmr_mem_cache.put(url_key, data_url);
        CACHE_LOG(prolog + "CMR Cache put, translated URL: " + data_url + '\n');
    }

    return data_url;
}

/**
 * @brief Build a URL to the granule in the OPeNDAP S3 bucket
 * @param rest_path The REST path of the granule: '/collections/<ccid>/granules/<granule_id>'
 * @param data_source The protocol and host name part of the URL
 * @return The URL to the item in a S3 bucket
 */
string NgapOwnedContainer::build_dmrpp_url_to_owned_bucket(const string &rest_path, const string &data_source) {
    // The PATH part of a URL to the NGAP/DMR++ is an 'NGAP REST path' that has the form:
    // /collections/<ccid>/granules/<granule_id>. In our 'owned' S3 bucket, we use object
    // names of the form: /<ccid>/<granule_id>.dmrpp.
    BES_MODULE_TIMING(prolog + rest_path);

    auto parts = BESUtil::split(rest_path, '/');
    if (parts.size() != 4 || parts[0] != "collections" || parts[2] != "granules") {
        throw BESSyntaxUserError("Invalid NGAP path: " + rest_path, __FILE__, __LINE__);
    }

    string dmrpp_name = parts[1] + '/' + parts[3] + ".dmrpp";

    // http://<bucket_name>.s3.amazonaws.com/<object_key>
    // Chane so the first part is read from a configuration file.
    // That way it can be a file:// URL for testing, and later can be set
    // in other ways. jhrg 5/1/24
    string dmrpp_url_str = data_source + '/' + dmrpp_name;

    return dmrpp_url_str;
}

/**
 * @brief Parse the NGAP REST path and build the object key to the DMR++ in the OPeNDAP Bucket.
 * @param rest_path The NGAP REST path provided by the user
 * @return The object key to the DMR++
 */
string NgapOwnedContainer::build_dmrpp_object_key_in_owned_bucket(const string &rest_path) {
    // The PATH part of a URL to the NGAP/DMR++ is an 'NGAP REST path' that has the form:
    // /collections/<ccid>/granules/<granule_id>. In our 'owned' S3 bucket, we use object
    // names of the form: /<ccid>/<granule_id>.dmrpp.
    BES_MODULE_TIMING(prolog + rest_path);

    auto parts = BESUtil::split(rest_path, '/');
    if (parts.size() != 4 || parts[0] != "collections" || parts[2] != "granules") {
        throw BESSyntaxUserError("Invalid NGAP path: " + rest_path, __FILE__, __LINE__);
    }

    return {parts[1] + '/' + parts[3] + ".dmrpp"};
}

bool NgapOwnedContainer::get_item_from_dmrpp_cache(string &dmrpp_string) const {

    // Read the cache entry if it exists. jhrg 4/29/24
    if (NgapRequestHandler::d_dmrpp_mem_cache.get(get_real_name(), dmrpp_string)) {
        CACHE_LOG(prolog + "Memory Cache hit, DMR++: " + get_real_name() + '\n');
        return true;
    }
    else {
        CACHE_LOG(prolog + "Memory Cache miss, DMR++: " + get_real_name() + '\n');
    }

    // Before going over the network to get the DMR++, look in the FileCache.
    // If found, put it in the memory cache and return it as a string.

    FileCache::Item item;
    if (NgapRequestHandler::d_dmrpp_file_cache.get(FileCache::hash_key(get_real_name()), item)) { // got it
        // read data from the file into the string.
        CACHE_LOG(prolog + "File Cache hit, DMR++: " + get_real_name() + '\n');
        if (file_to_string(item.get_fd(), dmrpp_string)) {
            // put it in the memory cache
            NgapRequestHandler::d_dmrpp_mem_cache.put(get_real_name(), dmrpp_string);
            CACHE_LOG(prolog + "Memory Cache put, DMR++: " + get_real_name() + '\n');
            return true;
        }
        else {
            ERROR_LOG(prolog + "Failed to read DMR++ from file cache\n");
            return false;
        }
    }
    else {
        CACHE_LOG(prolog + "File Cache miss, DMR++: " + get_real_name() + '\n');
    }

    return false;
}

bool NgapOwnedContainer::put_item_in_dmrpp_cache(const std::string &dmrpp_string) const
{
    if (NgapRequestHandler::d_dmrpp_file_cache.put_data(FileCache::hash_key(get_real_name()), dmrpp_string)) {
        CACHE_LOG(prolog + "File Cache put, DMR++: " + get_real_name() + '\n');
    }
    else {
        // This might not be an error - put_data() records errors. jhrg 2/13/25
        CACHE_LOG(prolog + "Failed to put DMR++ in file cache\n");
        return false;
    }

    if (!NgapRequestHandler::d_dmrpp_file_cache.purge()) {
        ERROR_LOG(prolog + "Call to FileCache::purge() failed\n");
    }

    NgapRequestHandler::d_dmrpp_mem_cache.put(get_real_name(), dmrpp_string);
    CACHE_LOG(prolog + "Memory Cache put, DMR++: " + get_real_name() + '\n');

    return true;
}

/**
 * @brief Filter the cached resource. Each key in content_filters is replaced with its associated map value.
 *
 * WARNING: Does not lock cache. This method assumes that the process has already
 * acquired an exclusive lock on the cache file.
 *
 * WARNING: This method will overwrite the cached data with the filtered result.
 *
 * @param content_filters A map of key value pairs which define the filter operation. Each key found in the
 * resource will be replaced with its associated value.
 * @param content A reference to the C++ string to filter
 */
void NgapOwnedContainer::filter_response(const map <string, string, std::less<>> &content_filters, string &content) {
    for (const auto &filter: content_filters) {
        const unsigned int replace_count = BESUtil::replace_all(content, filter.first, filter.second);
        BESDEBUG(MODULE, prolog << "Replaced " << replace_count << " instance(s) of template(" << filter.first
                 << ") with " << filter.second << " in the DMR++" << endl);
    }
}

/**
 * Build the content filters if needed
 * @note If the filters are built, clear the content_filters value/result parameter first.
 * @param content_filters Value-result parameter
 * @return True if the filters were built, false otherwise
 */
bool NgapOwnedContainer::get_daac_content_filters(const string &data_url, map<string, string, std::less<>> &content_filters) {
    if (NgapOwnedContainer::d_inject_data_url) {
        // data_url was get_real_name(). jhrg 8/9/24
        const string missing_data_url_str = data_url + "_mvs.h5";
        const string href = R"(href=")";
        const string trusted_url_hack = R"(" dmrpp:trust="true")";
        const string data_access_url_key = href + DATA_ACCESS_URL_KEY + "\"";
        const string data_access_url_with_trusted_attr_str = href + data_url + trusted_url_hack;
        const string missing_data_access_url_key = href + MISSING_DATA_ACCESS_URL_KEY + "\"";
        const string missing_data_url_with_trusted_attr_str = href + missing_data_url_str + trusted_url_hack;

        content_filters.clear();
        content_filters.insert(pair<string, string>(data_access_url_key, data_access_url_with_trusted_attr_str));
        content_filters.insert(pair<string, string>(missing_data_access_url_key, missing_data_url_with_trusted_attr_str));
        return true;
    }

    return false;
}

/**
 * @brief Get the content filters for the OPeNDAP-owned DMR++.
 * Build filters to use the BESUtils::replace_all() method to insert the 'dmrpp:trust="true"'
 * XML attribute. Adding this means that we do not have to add the strange cloudfront host names
 * to the BES AllowedHosts list.
 * @param content_filters Value-result parameter that is the map of values to replace in the DMR++
 * @return True if the filters were built, false otherwise
 */
bool NgapOwnedContainer::get_opendap_content_filters(map<string, string, std::less<>> &content_filters) {
    if (NgapOwnedContainer::d_inject_data_url) {    // Hmmm, this is a bit of a hack. jhrg 8/22/24

        const string version_attribute = "dmrpp:version";
        const string trusted_attribute = R"(dmrpp:trust="true" )";
        // The 'trust' attribute is inserted _before_ the 'version' attribute. jhrg 8/22/24
        const string trusted_and_version = trusted_attribute + version_attribute;

        content_filters.clear();
        content_filters.insert(pair<string, string>(version_attribute, trusted_and_version));

        return true;
    }

    return false;
}

/**
 * @brief Look in the OPeNDAP S3 bucket for the object key
 * @param aws_sdk Use this AWS_SDK object
 * @param bucket The bucket to check
 * @param object_key The object_key to search for
 * @return True if a HEAD request returns success, false otherwise
 */
bool NgapOwnedContainer::dmrpp_probe_opendap_bucket(AWS_SDK &aws_sdk, const string &bucket, const string &object_key) const {
    bool success = aws_sdk.s3_head(bucket, object_key);
    return success;
}

/**
 * @brief Read the DMR++ from the OPeNDAP S3 bucket
 * @param dmrpp_string value-result parameter for the DMR++ doc as a string
 * @return True if the document was found, false otherwise
 * @exception Throw xxx on a 50x response from HTTP.
 */
bool NgapOwnedContainer::dmrpp_read_from_opendap_bucket(string &dmrpp_string) const {
    BES_MODULE_TIMING(prolog + get_real_name());
    bool dmrpp_read = false;

    bes::AWS_SDK aws_sdk;
    // FIXME Replace this hack. jhrg 3/12/25
    const string aws_key = getenv("CMAC_ID");
    const string aws_secret_key = getenv("CMAC_ACCESS_KEY");
    const string aws_region = getenv("CMAC_REGION");
    // FIXME END
    aws_sdk.initialize(aws_region, aws_key, aws_secret_key);
    const string object_key = build_dmrpp_object_key_in_owned_bucket(get_real_name());

    // Use an HTTP HEAD request to see if the object key exists in the OPeNDAP bucket.
    // get_data_source_location() returns the bucket.
    if (!dmrpp_probe_opendap_bucket(aws_sdk, d_data_source_location, object_key)) {
        return false;
    }

    // Once here, we know the object_key exists and can be accessed.
    dmrpp_string = aws_sdk.s3_get_as_string(d_data_source_location, object_key);

    const auto http_status = aws_sdk.get_http_status_code();
    if (http_status == 200) {
        map<string, string, std::less<> > content_filters;
        if (!get_opendap_content_filters(content_filters)) {
            throw BESInternalError("Could not build opendap content filters for DMR++", __FILE__, __LINE__);
        }
        filter_response(content_filters, dmrpp_string);
        INFO_LOG(prolog + "Found the DMRpp in the OPeNDAP bucket for: " + object_key);
        dmrpp_read = true;
    }
    else {
        switch (http_status) {
            case 400:
            case 401:
            case 403:
                ERROR_LOG(prolog + "Looked in the OPeNDAP bucket for the DMRpp for: " + get_real_name()
                    + " but got HTTP Status: " + std::to_string(aws_sdk.get_http_status_code()));
                dmrpp_string.clear(); // ...because S3 puts an error message in the string. jhrg 8/9/24
                dmrpp_read = false;
                break;
            case 404:
                INFO_LOG(prolog + "Looked in the OPeNDAP bucket for the DMRpp for: " + get_real_name()
                    + " but got HTTP Status: " + std::to_string(aws_sdk.get_http_status_code()));
                dmrpp_string.clear();
                dmrpp_read = false;
                break;
            default:
                throw http::HttpError(aws_sdk.get_aws_exception_message(), __FILE__, __LINE__);
        }
    }

    return dmrpp_read;
}

/**
 * @brief Read the DMR++ from a DAAC S3 bucket
 * @param dmrpp_string value-result parameter for the DMR++ doc as a string
 * @exception http::HttpError if the granule is not found
 */
void NgapOwnedContainer::dmrpp_read_from_daac_bucket(string &dmrpp_string) const {
    BES_MODULE_TIMING(prolog + get_real_name());
    // This code may ask CMR and will throw exceptions that mention CMR on error. jhrg 1/24/25
    string data_url = build_data_url_to_daac_bucket(get_real_name());
    string dmrpp_url_str = data_url + ".dmrpp"; // This is the URL to the DMR++ in the DAAC-owned bucket. jhrg 8/9/24
    INFO_LOG(prolog + "Look in the DAAC-bucket for the DMRpp for: " + dmrpp_url_str);

    try {
        curl::http_get(dmrpp_url_str, dmrpp_string);

        // filter the DMRPP from the DAAC's bucket to replace the template href with the data_url
        map <string, string, std::less<>> content_filters;
        if (!get_daac_content_filters(data_url, content_filters)) {
            throw BESInternalError("Could not build content filters for DMR++", __FILE__, __LINE__);
        }
        filter_response(content_filters, dmrpp_string);
        INFO_LOG(prolog + "Found the DMRpp in the DAAC-bucket for: " + dmrpp_url_str);
    }
    catch (http::HttpError &http_error) {
        http_error.set_message(http_error.get_message() + "NgapOwnedContainer::dmrpp_read_from_daac_bucket() failed to read the DMR++ from S3.");
        throw;
    }
}

/**
 * @brief Get the DMR++ from a remote source or a cache
 *
 * This method will try to read a DMR++ from an S3 bucket if that DMR++ cannot be found in the
 * DMR++ cache. If the DMR++ cannot be read from a S3 bucket, it will throw an exception. The
 * method returns false if the DMR++ was read but for some reason could not be cached.
 *
 * @param dmrpp_string Value-result parameter that will contain the DMR++ as a string
 *
 * @return True if the DMR++ was found and no caching issues were encountered, false if there
 * was a failure of the caching system. Error messages are logged if there is a caching issue.
 * Note that if this methods cannot get the DMR++ from the remote source, it will throw an exception.
 *
 * @exception http::HttpError if there is a problem making the remote request if one is needed.
 */
bool NgapOwnedContainer::get_dmrpp_from_cache_or_remote_source(string &dmrpp_string) const {
    BES_MODULE_TIMING(prolog + get_real_name());

    // If the DMR++ is cached, return it. NB: This cache holds OPeNDAP- and DAAC-owned DMR++ documents.
    if (NgapRequestHandler::d_use_dmrpp_cache && get_item_from_dmrpp_cache(dmrpp_string)) {
        return true;
    }

    // Else, the DMR++ is neither in the memory cache nor the file cache.
    // Read it from S3, etc., and filter it. Put it in the memory cache
    bool dmrpp_read = false;

    // If the server is set up to try the OPeNDAP bucket, look there first.
    if (NgapOwnedContainer::d_use_opendap_bucket) {
        // If we get the DMR++ from the OPeNDAP bucket, set dmrpp_read to true so
        // we don't also try the DAAC bucket.
        dmrpp_read = dmrpp_read_from_opendap_bucket(dmrpp_string);
    }

    // Try the DAAC bucket if either the OPeNDAP bucket is not used or the OPeNDAP bucket failed
    if (!dmrpp_read) {
        dmrpp_read_from_daac_bucket(dmrpp_string);
    }

    // if we get here, the DMR++ has been pulled over the network. Put it in both caches.
    // The memory cache is for use by this process, the file cache for other processes/VMs
    if (NgapRequestHandler::d_use_dmrpp_cache && !put_item_in_dmrpp_cache(dmrpp_string)) {
        return false;
    }

    return true;
}

/**
 * @brief Get the DMR++ from a remote source or a local cache
 *
 * @note The Container::access() methods are called by the framework when it
 * runs execute_commands() and then, often, a second time in the RequestHandler
 * code when it is looking for data.
 *
 * @return The DMR++ as a string.
 * @throws BESError if there is a problem making the remote request
 */
string NgapOwnedContainer::access() {

    string dmrpp_string;

    // Get the DMR++ from the S3 bucket or the cache.
    // get_dmrpp...() returns false for various caching errors, but throws if it cannot
    // get the remote DMR++. jhrg 4/29/24
    get_dmrpp_from_cache_or_remote_source(dmrpp_string);

    set_attributes("as-string");    // This means access() returns a string. jhrg 10/19/23
    // Originally, this was either hard-coded (as it is now) or was set using the 'extension'
    // on the URL. But it's always a DMR++. jhrg 11/16/23
    set_container_type("dmrpp");

    return dmrpp_string;
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance along with information about
 * this container.
 *
 * @param strm C++ i/o stream to dump the information to
 */
void NgapOwnedContainer::dump(ostream &strm) const {
    strm << BESIndent::LMarg << "NgapOwnedContainer::dump - (" << (void *) this << ")\n";
    BESIndent::Indent();
    BESContainer::dump(strm);
    BESIndent::UnIndent();
}

} // namespace ngap
