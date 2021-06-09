// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of the BES http package, part of the Hyrax data server.

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

// Authors:
//      ndp       Nathan Potter <ndp@opendap.org>

#include "config.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <sstream>
#include <fstream>
#include <string>
#include <iostream>
#include <utility>

#include "rapidjson/document.h"

#include "BESInternalError.h"
#include "BESForbiddenError.h"
#include "BESSyntaxUserError.h"
#include "BESNotFoundError.h"
#include "BESTimeoutError.h"

#include "BESDebug.h"
#include "BESUtil.h"

#include "HttpCache.h"
#include "HttpUtils.h"
#include "CurlUtils.h"
#include "HttpNames.h"
#include "RemoteResource.h"
#include "TheBESKeys.h"
#include "BESStopWatch.h"
#include "BESLog.h"

using namespace std;

#define BES_CATALOG_ROOT_KEY "BES.Catalog.catalog.RootDirectory"

#define prolog std::string("RemoteResource::").append(__func__).append("() - ")
#define MODULE HTTP_MODULE

namespace http {

RemoteResource::RemoteResource(
        std::shared_ptr<http::url> target_url,
        const std::string &uid,
        long long expiredInterval)
        : d_remoteResourceUrl(std::move(target_url)){

    d_fd = 0;
    d_initialized = false;

    d_uid = uid;

    d_resourceCacheFileName.clear();
    d_response_headers = new vector<string>();
    d_http_response_headers = new map<string, string>();

    d_expires_interval = expiredInterval;


    if(d_remoteResourceUrl->protocol() == FILE_PROTOCOL){
        BESDEBUG(MODULE,prolog << "Found FILE protocol." << endl);
        d_resourceCacheFileName = d_remoteResourceUrl->path();
        while(BESUtil::endsWith(d_resourceCacheFileName,"/")){
            // Strip trailing slashes, because this about files, not directories
            d_resourceCacheFileName = d_resourceCacheFileName.substr(0,d_resourceCacheFileName.length()-1);
        }
        // Now we check that the data is in the BES_CATALOG_ROOT
        string catalog_root;
        bool found;
        TheBESKeys::TheKeys()->get_value(BES_CATALOG_ROOT_KEY,catalog_root,found );
        if(!found){
            throw BESInternalError( prolog + "ERROR - "+ BES_CATALOG_ROOT_KEY + "is not set",__FILE__,__LINE__);
        }
        if(d_resourceCacheFileName.find(catalog_root) !=0 ){
            d_resourceCacheFileName = BESUtil::pathConcat(catalog_root,d_resourceCacheFileName);
        }
        BESDEBUG(MODULE,"d_resourceCacheFileName: " << d_resourceCacheFileName << endl);
        d_initialized =true;
    }
    else if( d_remoteResourceUrl->protocol() == HTTPS_PROTOCOL || d_remoteResourceUrl->protocol() == HTTP_PROTOCOL ){
        BESDEBUG(MODULE, prolog << "URL: " << d_remoteResourceUrl->str() << endl);
#if 0

        if (!d_uid.empty()){
                string client_id_hdr = "User-Id: " + d_uid;
                BESDEBUG(MODULE, prolog << client_id_hdr << endl);
                d_request_headers.push_back(client_id_hdr);
            }
            if (!d_echo_token.empty()){
                string echo_token_hdr = "Echo-Token: " + d_echo_token;
                BESDEBUG(MODULE, prolog << echo_token_hdr << endl);
                d_request_headers.push_back(echo_token_hdr);
            }
#endif

    }
    else {
        string err = prolog + "Unsupported protocol: " + d_remoteResourceUrl->protocol();
        throw BESInternalError(err, __FILE__, __LINE__);
    }

    // BESDEBUG(MODULE, prolog << "d_curl: " << d_curl << endl);

}


#if 0
/**
     * Builds a RemoteHttpResource object associated with the passed url parameter.
     *
     * @param url Is a URL string that identifies the remote resource.
     */
    RemoteResource::RemoteResource(const std::string &url, const std::string &uid, const std::string &echo_token) {

        d_fd = 0;
        d_initialized = false;

        d_uid = uid;
        d_echo_token = echo_token;

        // d_curl = curl::init(url);

        d_resourceCacheFileName.clear();
        d_response_headers = new vector<string>();
        d_request_headers = new vector<string>();
        d_http_response_headers = new map<string, string>();

        if (url.empty()) {
            throw BESInternalError(prolog + "Remote resource URL is empty.", __FILE__, __LINE__);
        }

        if(url.find(FILE_PROTOCOL) == 0){
            d_resourceCacheFileName = url.substr(strlen(FILE_PROTOCOL));
            while(BESUtil::endsWith(d_resourceCacheFileName,"/")){
                // Strip trailing slashes, because this about files, not directories
                d_resourceCacheFileName = d_resourceCacheFileName.substr(0,d_resourceCacheFileName.length()-1);
            }
            // Now we check that the data is in the BES_CATALOG_ROOT
            string catalog_root;
            bool found;
            TheBESKeys::TheKeys()->get_value(BES_CATALOG_ROOT_KEY,catalog_root,found );
            if(!found){
                throw BESInternalError( prolog + "ERROR - "+ BES_CATALOG_ROOT_KEY + "is not set",__FILE__,__LINE__);
            }
            if(d_resourceCacheFileName.find(catalog_root) !=0 ){
                d_resourceCacheFileName = BESUtil::pathConcat(catalog_root,d_resourceCacheFileName);
            }
            d_initialized =true;
        }
        else if(url.find(HTTPS_PROTOCOL) == 0  || url.find(HTTP_PROTOCOL) == 0){
            d_remoteResourceUrl = url;
            BESDEBUG(MODULE, prolog << "URL: " << d_remoteResourceUrl << endl);

            if (!d_uid.empty()){
                string client_id_hdr = "User-Id: " + d_uid;
                BESDEBUG(MODULE, prolog << client_id_hdr << endl);
                d_request_headers->push_back(client_id_hdr);
            }
            if (!d_echo_token.empty()){
                string echo_token_hdr = "Echo-Token: " + d_echo_token;
                BESDEBUG(MODULE, prolog << echo_token_hdr << endl);
                d_request_headers->push_back(echo_token_hdr);
            }
        }
        else {
            string err = prolog + "Unsupported protocol: " + url;
            throw BESInternalError(err, __FILE__, __LINE__);
        }



        // BESDEBUG(MODULE, prolog << "d_curl: " << d_curl << endl);
    }
#endif


/**
 * Releases any memory resources and also any existing cache file locks for the cached resource.
 * ( Closes the file descriptor opened when retrieveResource() was called.)
 */
RemoteResource::~RemoteResource() {
    BESDEBUG(MODULE, prolog << "BEGIN resourceURL: " << d_remoteResourceUrl->str() << endl);

    delete d_response_headers;
    d_response_headers = 0;
    BESDEBUG(MODULE, prolog << "Deleted d_response_headers." << endl);

    if (!d_resourceCacheFileName.empty()) {
        HttpCache *cache = HttpCache::get_instance();
        if (cache) {
            cache->unlock_and_close(d_resourceCacheFileName);
            BESDEBUG(MODULE, prolog << "Closed and unlocked " << d_resourceCacheFileName << endl);
            d_resourceCacheFileName.clear();
        }
    }
    BESDEBUG(MODULE, prolog << "END" << endl);
}

/**
 * Returns the (read-locked) cache file name on the local system in which the content of the remote
 * resource is stored. Deleting of the instance of this class will release the read-lock.
 */
std::string RemoteResource::getCacheFileName() {
    if (!d_initialized) {
        throw BESInternalError(prolog + "STATE ERROR: Remote Resource " + d_remoteResourceUrl->str() +
                               " has Not Been Retrieved.", __FILE__, __LINE__);
    }
    return d_resourceCacheFileName;
}

/**
 * This method will check the cache for the resource. If it's not there then it will lock the cache and retrieve
 * the remote resource content using HTTP GET.
 *
 * When this method returns the RemoteResource object is fully initialized and the cache file name for the resource
 * is available along with an open file descriptor for the (now read-locked) cache file.
 */
void RemoteResource::retrieveResource() {
    std::map<std::string, std::string> content_filters;
    retrieveResource(content_filters);
}

/**
 * This method will check the cache for the resource. If it's not there then it will lock the cache and retrieve
 * the remote resource content using HTTP GET.
 *
 * When this method returns the RemoteHttpResource object is fully initialized and the cache file name for the resource
 * is available along with an open file descriptor for the (now read-locked) cache file.
 *
 * @param uid
 * @param template_key
 * @param replace_value
 */
void RemoteResource::retrieveResource(const std::map<std::string, std::string> &content_filters) {
    BESDEBUG(MODULE, prolog << "BEGIN   resourceURL: " << d_remoteResourceUrl->str() << endl);
    bool mangle = true;

    // TODO come back and visit this condition and determine if it is still needed jhrg/sbl 4.14.21
    if (d_initialized) {
        BESDEBUG(MODULE, prolog << "END  Already initialized." << endl);
        return;
    }
    // Get a pointer to the singleton cache instance for this process.
    HttpCache *cache = HttpCache::get_instance();
    if (!cache) {
        ostringstream oss;
        oss << prolog << "FAILED to get local cache. ";
        oss << "Unable to proceed with request for " << this->d_remoteResourceUrl->str();
        oss << " The server MUST have a valid HTTP cache configuration to operate." << endl;
        BESDEBUG(MODULE, oss.str());
        throw BESInternalError(oss.str(), __FILE__, __LINE__);
    }

    // Get the name of the file in the cache (either the code finds this file or
    // or it makes it).
    d_resourceCacheFileName = cache->get_cache_file_name(d_uid, d_remoteResourceUrl->str(), mangle);
    BESDEBUG(MODULE, prolog << "d_resourceCacheFileName: " << d_resourceCacheFileName << endl);

    // @TODO MAKE THIS RETRIEVE THE CACHED DATA TYPE IF THE CACHED RESPONSE IF FOUND
    //   We need to know the type of the resource. HTTP headers are the preferred  way to determine the type.
    //   Unfortunately, the current code losses both the HTTP headers sent from the request and the derived type
    //   to subsequent accesses of the cached object. Since we have to have a type, for now we just set the type
    //   from the url. If down below we DO an HTTP GET then the headers will be evaluated and the type set by setType()
    //   But really - we gotta fix this.
    http::get_type_from_url(d_remoteResourceUrl->str(), d_type);
    BESDEBUG(MODULE, prolog << "d_type: " << d_type << endl);

    try {
        if (cache->get_exclusive_lock(d_resourceCacheFileName, d_fd)) {
            BESDEBUG(MODULE,
                     prolog << "Remote resource is already in cache. cache_file_name: " << d_resourceCacheFileName
                            << endl);

            if (cached_resource_is_expired()) {
                BESDEBUG(MODULE, prolog << "EXISTS - UPDATING " << endl);
                update_file_and_headers(content_filters);
                cache->exclusive_to_shared_lock(d_fd);
            } else {
                BESDEBUG(MODULE, prolog << "EXISTS - LOADING " << endl);
                cache->exclusive_to_shared_lock(d_fd);
                load_hdrs_from_file();
            }
            d_initialized = true;
            return;
        } else {
            // Now we actually need to reach out across the interwebs and retrieve the remote resource and put it's
            // content into a local cache file, given that it's not in the cache.
            // First make an empty file and get an exclusive lock on it.
            if (cache->create_and_lock(d_resourceCacheFileName, d_fd)) {
                BESDEBUG(MODULE, prolog << "DOESN'T EXIST - CREATING " << endl);
                update_file_and_headers(content_filters);
            } else {
                BESDEBUG(MODULE, prolog << " WAS CREATED - LOADING " << endl);
                cache->get_read_lock(d_resourceCacheFileName, d_fd);
                load_hdrs_from_file();
            }
            d_initialized = true;
            return;
        }

        stringstream msg;
        msg << prolog + "Failed to acquire cache read lock for remote resource: '";
        msg << d_remoteResourceUrl->str() << endl;
        throw BESInternalError(msg.str(), __FILE__, __LINE__);

    }
    catch (BESError &besError) {
        BESDEBUG(MODULE, prolog << "Caught BESError. type: " << besError.get_bes_error_type() <<
                                " message: '" << besError.get_message() <<
                                "' file: " << besError.get_file() << " line: " << besError.get_line() <<
                                " Will unlock cache and re-throw." << endl);
        cache->unlock_cache();
        throw;
    }
    catch (...) {
        BESDEBUG(MODULE, prolog << "Caught unknown exception. Will unlock cache and re-throw." << endl);
        cache->unlock_cache();
        throw;
    }

} //end RemoteResource::retrieveResource()

/**
 * method for calling update_file_and_header(map<string,string>) with a black map
 */
void RemoteResource::update_file_and_headers(){
    std::map<std::string, std::string> content_filters;
    update_file_and_headers(content_filters);
}

/**
 * updates the file in the cache and the related headers file
 *
 * @param content_filters
 */
void RemoteResource::update_file_and_headers(const std::map<std::string, std::string> &content_filters){

    // Get a pointer to the singleton cache instance for this process.
    HttpCache *cache = HttpCache::get_instance();
    if (!cache) {
        ostringstream oss;
        oss << prolog << "FAILED to get local cache. ";
        oss << "Unable to proceed with request for " << this->d_remoteResourceUrl->str();
        oss << " The server MUST have a valid HTTP cache configuration to operate." << endl;
        BESDEBUG(MODULE, oss.str());
        throw BESInternalError(oss.str(), __FILE__, __LINE__);
    }

    // Write the remote resource to the cache file.
    try {
        writeResourceToFile(d_fd);
    }
    catch (...) {
        // If things went south then we need to dump the file because we'll end up with an empty/bogus file clogging the cache
        unlink(d_resourceCacheFileName.c_str());
        throw;
    }

    //  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
    // Filter the response file - If content_filters map is empty then nothing is done.
    filter_retrieved_resource(content_filters);

    // Write the headers to the appropriate cache file.
    string hdr_filename = d_resourceCacheFileName + ".hdrs";
    std::ofstream hdr_out(hdr_filename.c_str());
    try {
        for (size_t i = 0; i < this->d_response_headers->size(); i++) {
            hdr_out << (*d_response_headers)[i] << endl;
        }
    }
    catch (...) {
        // If this fails for any reason we:
        hdr_out.close(); // Close the stream
        unlink(hdr_filename.c_str()); // unlink the file
        unlink(d_resourceCacheFileName.c_str()); // unlink the primary cache file.
        throw;
    }

    // #########################################################################################################

    // Change the exclusive lock on the new file to a shared lock. This keeps
    // other processes from purging the new file and ensures that the reading
    // process can use it.
    cache->exclusive_to_shared_lock(d_fd);
    BESDEBUG(MODULE, prolog << "Converted exclusive cache lock to shared lock." << endl);

    // Now update the total cache size info and purge if needed. The new file's
    // name is passed into the purge method because this process cannot detect its
    // own lock on the file.
    unsigned long long size = cache->update_cache_info(d_resourceCacheFileName);
    BESDEBUG(MODULE, prolog << "Updated cache info" << endl);

    if (cache->cache_too_big(size)) {
        cache->update_and_purge(d_resourceCacheFileName);
        BESDEBUG(MODULE, prolog << "Updated and purged cache." << endl);
    }
    BESDEBUG(MODULE, prolog << "END" << endl);

    return;
} //end RemoteResource::update_file_and_headers()

/**
 * finds the header file of a previously specified file and retrieves the related headers file
 */
void RemoteResource::load_hdrs_from_file(){
    string hdr_filename = d_resourceCacheFileName + ".hdrs";
    std::ifstream hdr_ifs(hdr_filename.c_str());

    if(!hdr_ifs.is_open()){
        stringstream msg;
        msg << "ERROR. Internal state error. The headers file: " << hdr_filename << " could not be opened for reading.";
        BESDEBUG(MODULE, prolog << msg.str() << endl);
        throw BESInternalError(msg.str(), __FILE__, __LINE__);
    }

    BESDEBUG(MODULE, prolog << "Reading response headers from: " << hdr_filename << endl);
    for (std::string line; std::getline(hdr_ifs, line);) {
        (*d_response_headers).push_back(line);
        BESDEBUG(MODULE, prolog << "header:   " << line << endl);
    }
    ingest_http_headers_and_type();
} //end RemoteResource::load_hdrs_from_file()

/**
 * Checks if a cache resource is older than an hour
 *
 * @param filename - name of the resource to be checked
 * @param uid
 * @return true if the resource is over an hour old
 */
bool RemoteResource::cached_resource_is_expired(){
    BESDEBUG(MODULE, prolog << "BEGIN" << endl);

    struct stat statbuf;
    if (stat(d_resourceCacheFileName.c_str(), &statbuf) == -1){
        throw BESNotFoundError(strerror(errno), __FILE__, __LINE__);
    }//end if
    BESDEBUG(MODULE, prolog << "File exists" << endl);

    time_t cacheTime = statbuf.st_ctime;
    BESDEBUG(MODULE, prolog << "Cache file creation time: " << cacheTime << endl);
    time_t nowTime = time(0);
    BESDEBUG(MODULE, prolog << "Time now: " << nowTime << endl);
    double diffSeconds = difftime(nowTime,cacheTime);
    BESDEBUG(MODULE, prolog << "Time difference between cacheTime and nowTime: " << diffSeconds << endl);

    if (diffSeconds > d_expires_interval){
        BESDEBUG(MODULE, prolog << " refresh = TRUE " << endl);
        return true;
    }
    else{
        BESDEBUG(MODULE, prolog << " refresh = FALSE " << endl);
        return false;
    }
} //end RemoteResource::is_cache_resource_expired()

/**
 *
 * Retrieves the remote resource and write it the the open file associated with the open file
 * descriptor parameter 'fd'. In the process of caching the file a FILE * is fdopen'd from 'fd' and that is used buy
 * curl to write the content. At the end the stream is rewound and the FILE * pointer is returned.
 *
 * @param fd An open file descriptor the is associated with the target file.
 */
void RemoteResource::writeResourceToFile(int fd) {

    BESDEBUG(MODULE, prolog << "BEGIN" << endl);
    try {

        BESStopWatch besTimer;
        if (BESDebug::IsSet("rr") || BESDebug::IsSet(MODULE) || BESDebug::IsSet(TIMING_LOG_KEY) || BESLog::TheLog()->is_verbose()){
            besTimer.start(prolog + "source url: " + d_remoteResourceUrl->str());
        }

        int status = lseek(fd, 0, SEEK_SET);
        if (-1 == status)
            throw BESNotFoundError("Could not seek within the response file.", __FILE__, __LINE__);
        BESDEBUG(MODULE, prolog << "Reset file descriptor to start of file." << endl);

        status = ftruncate(fd, 0);
        if (-1 == status)
            throw BESInternalError("Could truncate the file prior to updating from remote. ", __FILE__, __LINE__);
        BESDEBUG(MODULE, prolog << "Truncated file, length is zero." << endl);

        BESDEBUG(MODULE, prolog << "Saving resource " << d_remoteResourceUrl << " to cache file " << d_resourceCacheFileName << endl);
        curl::http_get_and_write_resource(d_remoteResourceUrl, fd, d_response_headers); // Throws BESInternalError if there is a curl error.

        BESDEBUG(MODULE,  prolog << "Resource " << d_remoteResourceUrl->str() << " saved to cache file " << d_resourceCacheFileName << endl);

        // rewind the file
        // FIXME I think the idea here is that we have the file open and we should just keep
        //  reading from it. But the container mechanism works with file names, so we will
        //  likely have to open the file again. If that's true, lets remove this call. jhrg 3.2.18
        status = lseek(fd, 0, SEEK_SET);
        if (-1 == status)
            throw BESNotFoundError("Could not seek within the response file.", __FILE__, __LINE__);
        BESDEBUG(MODULE, prolog << "Reset file descriptor to start of file." << endl);

        // @TODO CACHE THE DATA TYPE OR THE HTTP HEADERS SO WHEN WE ARE RETRIEVING THE CACHED OBJECT WE CAN GET THE CORRECT TYPE
        ingest_http_headers_and_type();
    }
    catch (BESError &e) {
        throw;
    }
    BESDEBUG(MODULE, prolog << "END" << endl);
}

/**
 *
 */
void RemoteResource::ingest_http_headers_and_type() {
    BESDEBUG(MODULE, prolog << "BEGIN" << endl);

    const string colon_space = ": ";
    for (size_t i = 0; i < this->d_response_headers->size(); i++) {
        string header = (*d_response_headers)[i];
        BESDEBUG(MODULE, prolog << "Processing header " << header << endl);
        size_t colon_index = header.find(colon_space);
        if(colon_index == string::npos){
            BESDEBUG(MODULE, prolog << "Unable to locate the colon space \": \" delimiter in the header " <<
                                    "string: '" << header << "' SKIPPING!" << endl);
        }
        else {
            string key = BESUtil::lowercase(header.substr(0, colon_index));
            string value = header.substr(colon_index + colon_space.length());
            BESDEBUG(MODULE, prolog << "key: " << key << " value: " << value << endl);
            (*d_http_response_headers)[key] = value;
        }
    }
    BESDEBUG(MODULE, prolog << "Ingested " << d_http_response_headers->size() << " response headers." << endl);

    std::map<string, string>::iterator it;
    string type;

    // Try and figure out the file type first from the
    // Content-Disposition in the http header response.
    BESDEBUG(MODULE, prolog << "Checking Content-Disposition headers for type information." << endl);
    string content_disp_hdr;
    content_disp_hdr = get_http_response_header("content-disposition");
    if (!content_disp_hdr.empty()) {
        // Content disposition exists, grab the filename
        // attribute
        http::get_type_from_disposition(content_disp_hdr, type);
        BESDEBUG(MODULE,prolog << "Evaluated content-disposition '" << content_disp_hdr << "' matched type: \"" << type << "\"" << endl);
    }

    // still haven't figured out the type. Check the content-type
    // next, translate to the BES MODULE name. It's also possible
    // that even though Content-disposition was available, we could
    // not determine the type of the file.
    BESDEBUG(MODULE, prolog << "Checking Content-Type headers for type information." << endl);
    string content_type = get_http_response_header("content-type");
    if (type.empty() && !content_type.empty()) {
        http::get_type_from_content_type(content_type, type);
        BESDEBUG(MODULE,prolog << "Evaluated content-type '" << content_type << "' matched type \"" << type << "\"" << endl);
    }

    // still haven't figured out the type. Now check the actual URL
    // and see if we can't match the URL to a MODULE name
    BESDEBUG(MODULE, prolog << "Checking URL path for type information." << endl);
    if (type.empty()) {
        http::get_type_from_url(d_remoteResourceUrl->str(), type);
        BESDEBUG(MODULE, prolog << "Evaluated url '" << d_remoteResourceUrl->str() << "' matched type: \"" << type << "\"" << endl);
    }
    // still couldn't figure it out, punt
    if (type.empty()) {
        string err = prolog + "Unable to determine the type of data"
                     + " returned from '" + d_remoteResourceUrl->str() + "'  Setting type to 'unknown'";
        BESDEBUG(MODULE, err << endl);
        type = "unknown";
        //throw BESSyntaxUserError( err, __FILE__, __LINE__ ) ;
    }
    d_type = type;
    BESDEBUG(MODULE, prolog << "END (dataset type: " << d_type << ")" << endl);
}

/**
 * Returns the value of the requested HTTP response header.
 * Evaluation is case-insensitive.
 * If the requested header_name is not found the empty string is returned.
 */
std::string
RemoteResource::get_http_response_header(const std::string header_name) {
    string value("");
    std::map<string, string>::iterator it;
    it = d_http_response_headers->find(BESUtil::lowercase(header_name));
    if (it != d_http_response_headers->end())
        value = it->second;
    return value;
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
 */
void RemoteResource::filter_retrieved_resource(const std::map<std::string, std::string> &content_filters){

    // No filters?
    if(content_filters.empty()){
        // No problem...
        return;
    }
    string resource_content;
    {
        std::stringstream buffer;
        //  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
        // Read the cached file into a string object
        std::ifstream cr_istrm(d_resourceCacheFileName);
        if (!cr_istrm.is_open()) {
            string msg = "Could not open '" + d_resourceCacheFileName + "' to read cached response.";
            BESDEBUG(MODULE, prolog << msg << endl);
            throw BESInternalError(msg, __FILE__, __LINE__);
        }
        buffer << cr_istrm.rdbuf();

        // FIXME Do we need to make a copy here? Could we pass buffer.str() to replace_all??
        resource_content = buffer.str();
    } // cr_istrm is closed here.

    for (const auto& apair : content_filters) {
        unsigned int replace_count = BESUtil::replace_all(resource_content,apair.first, apair.second);
        BESDEBUG(MODULE, prolog << "Replaced " << replace_count << " instance(s) of template(" <<
        apair.first << ") with " << apair.second << " in cached RemoteResource" << endl);
    }


    //  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
    // Replace the contents of the cached file with the modified string.
    std::ofstream cr_ostrm(d_resourceCacheFileName);
    if (!cr_ostrm.is_open()) {
        string msg = "Could not open '" + d_resourceCacheFileName + "' to write modified cached response.";
        BESDEBUG(MODULE, prolog << msg << endl);
        throw BESInternalError(msg, __FILE__, __LINE__);
    }
    cr_ostrm << resource_content;

}

/**
 * Returns cache file content in a string..
 */
std::string RemoteResource::get_response_as_string() {

    if(!d_initialized){
        stringstream msg;
        msg << "ERROR. Internal state error. " << __PRETTY_FUNCTION__ << " was called prior to retrieving resource.";
        BESDEBUG(MODULE, prolog << msg.str() << endl);
        throw BESInternalError(msg.str(), __FILE__, __LINE__);
    }
    string cache_file = getCacheFileName();
    //  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
    // Set up cache file input stream.
    std::ifstream file_istream(cache_file, std::ofstream::in);

    // If the cache filename is not valid, the stream will not open. Empty is not valid.
    if(file_istream.is_open()){
        // If it's open we've got a valid input stream.
        BESDEBUG(MODULE, prolog << "Using cached file: " << cache_file << endl);
        std::stringstream buffer;
        buffer << file_istream.rdbuf();
        return buffer.str();
    }
    else {
        stringstream msg;
        msg << "ERROR. Failed to open cache file " << cache_file << " for reading.";
        BESDEBUG(MODULE, prolog << msg.str() << endl);
        throw BESInternalError(msg.str(), __FILE__, __LINE__);
    }

}

/**
 * @brief get_as_json() This function returns the cached resource parsed into a JSON document.
 *
 * @param target_url The URL to dereference.
 * @TODO Move this to ../curl_utils.cc (Requires moving the rapidjson lib too)
 * @return JSON document parsed from the response document returned by target_url
 */
rapidjson::Document RemoteResource::get_as_json() {
    string response = get_response_as_string();
    rapidjson::Document d;
    d.Parse(response.c_str());
    return d;
}

/**
 * Returns a std::vector of HTTP headers received along with the response from the request for the remote resource..
 */
vector<string> *RemoteResource::getResponseHeaders() {
    if (!d_initialized){
        throw BESInternalError(prolog +"STATE ERROR: Remote Resource Has Not Been Retrieved.",__FILE__,__LINE__);
    }
    return d_response_headers;
}


#if 0
void RemoteResource::setType(const vector<string> *resp_hdrs) {

        BESDEBUG(MODULE, prolog << "BEGIN" << endl);

        string type = "";

        // Try and figure out the file type first from the
        // Content-Disposition in the http header response.
        string disp;
        string ctype;

        if (resp_hdrs) {
            vector<string>::const_iterator i = resp_hdrs->begin();
            vector<string>::const_iterator e = resp_hdrs->end();
            for (; i != e; i++) {
                string hdr_line = (*i);

                BESDEBUG(MODULE, prolog << "Evaluating header: " << hdr_line << endl);

                hdr_line = BESUtil::lowercase(hdr_line);

                string colon_space = ": ";
                int index = hdr_line.find(colon_space);
                string hdr_name = hdr_line.substr(0, index);
                string hdr_value = hdr_line.substr(index + colon_space.length());

                BESDEBUG(MODULE, prolog << "hdr_name: '" << hdr_name << "'   hdr_value: '" << hdr_value  << "' " << endl);

                if (hdr_name.find("content-disposition") != string::npos) {
                    // Content disposition exists
                    BESDEBUG(MODULE, prolog << "Located content-disposition header." << endl);
                    disp = hdr_value;
                }
                if (hdr_name.find("content-type") != string::npos) {
                    BESDEBUG(MODULE, prolog << "Located content-type header." << endl);
                    ctype = hdr_value;
                }
            }
        }

        if (!disp.empty()) {
            // Content disposition exists, grab the filename
            // attribute
            HttpUtils::Get_type_from_disposition(disp, type);
            BESDEBUG(MODULE,prolog << "Evaluated content-disposition '" << disp << "' matched type: \""  << type << "\"" << endl);
        }

        // still haven't figured out the type. Check the content-type
        // next, translate to the BES MODULE name. It's also possible
        // that even though Content-disposition was available, we could
        // not determine the type of the file.
        if (type.empty() && !ctype.empty()) {
            HttpUtils::Get_type_from_content_type(ctype, type);
            BESDEBUG(MODULE,prolog << "Evaluated content-type '" << ctype << "' matched type \"" << type << "\"" << endl);
        }

        // still haven't figured out the type. Now check the actual URL
        // and see if we can't match the URL to a MODULE name
        if (type.empty()) {
            HttpUtils::Get_type_from_url(d_remoteResourceUrl, type);
            BESDEBUG(MODULE,prolog << "Evaluated url '" << d_remoteResourceUrl << "' matched type: \"" << type << "\"" << endl);
        }

        // still couldn't figure it out, punt
        if (type.empty()) {
            string err = prolog + "Unable to determine the type of data"
                         + " returned from '" + d_remoteResourceUrl + "'  Setting type to 'unknown'";
            BESDEBUG(MODULE, err << endl);
            type = "unknown";
            //throw BESSyntaxUserError( err, __FILE__, __LINE__ ) ;
        }

        // @TODO CACHE THE DATA TYPE OR THE HTTP HEADERS SO WHEN WE ARE RETRIEVING THE CACHED OBJECT WE CAN GET THE CORRECT TYPE

        d_type = type;
    }
#endif


} //  namespace http

