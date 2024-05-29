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
// Authors:
//      ndp       Nathan Potter <ndp@opendap.org>


#include "config.h"

#include <sstream>
#include <fstream>
#include <string>
#include <iostream>

#include "rapidjson/document.h"

#include "BESInternalError.h"
#include "BESForbiddenError.h"
#include "BESSyntaxUserError.h"
#include "BESNotFoundError.h"
#include "BESTimeoutError.h"

#include "BESDebug.h"
#include "BESUtil.h"
#include "TheBESKeys.h"

#include "NgapNames.h"
#include "NgapCache.h"
#include "NgapUtils.h"
#include "curl_utils.h"
#include "RemoteHttpResource.h"

using namespace std;
using namespace ngap;

#define prolog std::string("RemoteHttpResource::").append(__func__).append("() - ")

namespace ngap {

    /**
     * Builds a RemoteHttpResource object associated with the passed \c url parameter.
     *
     * @param url Is a URL string that identifies the remote resource.
     */
    RemoteHttpResource::RemoteHttpResource(const std::string &url, const std::string &uid, const std::string &echo_token){

        d_fd = 0;
        d_initialized = false;

        d_uid = uid;
        d_echo_token = echo_token;

        d_curl = 0;
        d_resourceCacheFileName.clear();
        d_response_headers = new vector<string>();
        d_request_headers = new vector<string>();
        d_http_response_headers = new map<string, string>();

        if (url.empty()) {
            string err = "RemoteHttpResource(): Remote resource URL is empty";
            throw BESInternalError(err, __FILE__, __LINE__);
        }

        d_remoteResourceUrl = url;
        BESDEBUG(MODULE, prolog << "URL: " << d_remoteResourceUrl << endl);


        if(!d_uid.empty()){
            string client_id_hdr = "User-Id: " + d_uid;
            BESDEBUG(MODULE, prolog << client_id_hdr << endl);
            d_request_headers->push_back(client_id_hdr);
        }
        if(!d_echo_token.empty()){
            string echo_token_hdr = "Echo-Token: " + d_echo_token;
            BESDEBUG(MODULE, prolog << echo_token_hdr << endl);
            d_request_headers->push_back(echo_token_hdr);
        }

        // EXAMPLE: returned value parameter for CURL *
        //
        // CURL *www_lib_init(CURL **curl); // function type signature
        //
        // CURL *pvparam = 0;               // passed value parameter
        // result = www_lib_init(&pvparam); // the call to the method

        d_curl = ngap_curl::init(d_error_buffer);  // This may throw either Error or InternalErr

        ngap_curl::configureProxy(d_curl, d_remoteResourceUrl); // Configure the a proxy for this url (if appropriate).

        BESDEBUG(MODULE, prolog << "d_curl: " << d_curl << endl);
    }

    /**
     * Releases any memory resources and also any existing cache file locks for the cached resource.
     * ( Closes the file descriptor opened when retrieveResource() was called.)
     */
    RemoteHttpResource::~RemoteHttpResource() {
        BESDEBUG(MODULE, prolog << "BEGIN resourceURL: " << d_remoteResourceUrl << endl);

        delete d_response_headers;
        d_response_headers = 0;
        BESDEBUG(MODULE, prolog << "Deleted d_response_headers." << endl);

        delete d_request_headers;
        d_request_headers = 0;
        BESDEBUG(MODULE, prolog << "Deleted d_request_headers." << endl);

/*
        if (!d_resourceCacheFileName.empty()) {
            NgapCache *cache = NgapCache::get_instance();
            if (cache) {
                cache->unlock_and_close(d_resourceCacheFileName);
                BESDEBUG(MODULE, prolog << "Closed and unlocked " << d_resourceCacheFileName << endl);
                d_resourceCacheFileName.clear();
            }
        }
*/

        if (d_curl) {
            curl_easy_cleanup(d_curl);
            BESDEBUG(MODULE, prolog << "Called curl_easy_cleanup()." << endl);
        }
        d_curl = 0;

        BESDEBUG(MODULE, prolog << "Clearing resourceURL: " << d_remoteResourceUrl << endl);
        d_remoteResourceUrl.clear();
    }


    /**
     * Returns the (read-locked) cache file name on the local system in which the content of the remote
     * resource is stored. Deleting of the instance of this class will release the read-lock.
     */
    std::string RemoteHttpResource::getCacheFileName() {
        if (!d_initialized) {
            throw BESInternalError(prolog + "STATE ERROR: Remote Resource " + d_remoteResourceUrl +
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
    void RemoteHttpResource::retrieveResource() {
        string template_key;
        string replace_value;
        retrieveResource(template_key,replace_value);
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
    void RemoteHttpResource::retrieveResource(const string &template_key, const string &replace_value) {
        BESDEBUG(MODULE, prolog << "BEGIN   resourceURL: " << d_remoteResourceUrl << endl);

        if (d_initialized) {
            BESDEBUG(MODULE, prolog << "END  Already initialized." << endl);
            return;
        }
        // Get a pointer to the singleton cache instance for this process.
        NgapCache *cache = NgapCache::get_instance();
        if (!cache) {
            ostringstream oss;
            oss << __func__ << "() - FAILED to get local cache."
                               " Unable to proceed with request for " << this->d_remoteResourceUrl
                << " The ngap_module MUST have a valid cache configuration to operate." << endl;
            BESDEBUG(MODULE, oss.str());
            throw BESInternalError(oss.str(), __FILE__, __LINE__);
        }

        // Get the name of the file in the cache (either the code finds this file or
        // or it makes it).
        d_resourceCacheFileName = cache->get_cache_file_name(d_uid,d_remoteResourceUrl);
        BESDEBUG(MODULE, prolog << "d_resourceCacheFileName: " << d_resourceCacheFileName << endl);

        // @TODO MAKE THIS RETRIEVE THE CACHED DATA TYPE IF THE CACHED RESPONSE IF FOUND
        // We need to know the type of the resource. HTTP headers are the preferred  way to determine the type.
        // Unfortunately, the current code losses both the HTTP headers sent from the request and the derived type
        // to subsequent accesses of the cached object. Since we have to have a type, for now we just set the type
        // from the url. If down below we DO an HTTP GET then the headers will be evaluated and the type set by setType()
        // But really - we gotta fix this.
        NgapUtils::Get_type_from_url(d_remoteResourceUrl, d_type);
        BESDEBUG(MODULE, prolog << "d_type: " << d_type << endl);

        try {
            if (cache->get_read_lock(d_resourceCacheFileName, d_fd)) {
                BESDEBUG(MODULE,
                         prolog << "Remote resource is already in cache. cache_file_name: " << d_resourceCacheFileName
                                << endl);

                // #########################################################################################################
                // I think in this if() is where we need to load the headers from the cache if we have them.
                string hdr_filename = cache->get_cache_file_name(d_uid,d_remoteResourceUrl) + ".hdrs";
                std::ifstream hdr_ifs(hdr_filename.c_str());
                try {
                    BESDEBUG(MODULE, prolog << "Reading response headers from: " << hdr_filename << endl);
                    for (std::string line; std::getline(hdr_ifs, line);) {
                        (*d_response_headers).push_back(line);
                        BESDEBUG(MODULE, prolog << "header:   " << line << endl);
                    }
                }
                catch (...) {
                    hdr_ifs.close();
                    throw;
                }
                ingest_http_headers_and_type();
                d_initialized = true;
                return;
                // #########################################################################################################
            }

            // Now we actually need to reach out across the interwebs and retrieve the remote resource and put it's
            // content into a local cache file, given that it's not in the cache.
            // First make an empty file and get an exclusive lock on it.
            if (cache->create_and_lock(d_resourceCacheFileName, d_fd)) {

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
                // If we are filtering the response (for example to inject data URL into a dmr++ file),
                // The file is locked and we have the information required to make the substitution.
                // This is controlled by:
                //  - The template_key string must not be empty.
                if(!template_key.empty()){
                        unsigned int count = filter_retrieved_resource(template_key, replace_value);
                        BESDEBUG(MODULE, prolog << "Replaced " << count <<
                        " instance(s) of template(" <<
                        template_key << ") with "<< replace_value << " in cached RemoteResource" << endl);
                }

                //  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
                // I think right here is where I would be able to cache the data type/response headers. While I have
                // the exclusive lock I could open another cache file for metadata and write to it.
                {
                    string hdr_filename = cache->get_cache_file_name(d_uid,d_remoteResourceUrl) + ".hdrs";
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
                d_initialized = true;
                return;
            } else {
                if (cache->get_read_lock(d_resourceCacheFileName, d_fd)) {
                    BESDEBUG(MODULE,
                             prolog << "Remote resource is in cache. cache_file_name: " << d_resourceCacheFileName
                                    << endl);
                    d_initialized = true;
                    return;
                }
            }

            string msg = prolog + "Failed to acquire cache read lock for remote resource: '";
            msg += d_remoteResourceUrl + "\n";
            throw BESInternalError(msg, __FILE__, __LINE__);

        }
        catch (...) {
            BESDEBUG(MODULE,
                     "RemoteHttpResource::retrieveResource() - Caught exception, unlocking cache and re-throw."
                             << endl);
            cache->unlock_cache();
            throw;
        }

    }

    /**
     *
     * Retrieves the remote resource and write it the the open file associated with the open file
     * descriptor parameter 'fd'. In the process of caching the file a FILE * is fdopen'd from 'fd' and that is used buy
     * curl to write the content. At the end the stream is rewound and the FILE * pointer is returned.
     *
     * @param fd An open file descriptor the is associated with the target file.
     */
    void RemoteHttpResource::writeResourceToFile(int fd) {
        BESDEBUG(MODULE, prolog << "BEGIN" << endl);

        int status = -1;
        try {
            BESDEBUG(MODULE, prolog << "Saving resource " << d_remoteResourceUrl << " to cache file " << d_resourceCacheFileName << endl);
            status = ngap_curl::read_url(d_curl, d_remoteResourceUrl, fd, d_response_headers,
                            d_request_headers, d_error_buffer); // Throws BESInternalError if there is a curl error.

            if (status >= 400) {
                BESDEBUG(MODULE, prolog << "ERROR: HTTP request returned status of: " << status << endl);
                // delete resp_hdrs; resp_hdrs = 0;
                stringstream msg;
                msg << prolog << "Error while reading the URL: \"" <<  d_remoteResourceUrl << "\", ";;
                for(unsigned int i=0; i<d_request_headers->size() ;i++){
                    msg << "reqhdr[" << i << "]: \"" << (*d_request_headers)[i] << "\", ";
                }
                msg <<    "The HTTP request returned a status of " << status << " which means '" <<
                    ngap_curl::http_status_to_string(status) << "'" << endl;
                BESDEBUG(MODULE, prolog << "ERROR: HTTP request returned status: " << status << " message: " << msg.str() << endl);
                switch(status) {
                    case 400:
                        throw BESSyntaxUserError(msg.str(), __FILE__, __LINE__);
                    case 401:
                    case 402:
                    case 403:
                        throw BESForbiddenError(msg.str(), __FILE__, __LINE__);
                    case 404:
                        throw BESNotFoundError(msg.str(), __FILE__, __LINE__);
                    case 408:
                        throw BESTimeoutError(msg.str(), __FILE__, __LINE__);
                    default:
                        throw BESInternalError(msg.str(), __FILE__, __LINE__);
                }
            }
            BESDEBUG(MODULE,  prolog << "Resource " << d_remoteResourceUrl << " saved to cache file " << d_resourceCacheFileName << endl);

            // rewind the file
            // FIXME I think the idea here is that we have the file open and we should just keep
            // reading from it. But the container mechanism works with file names, so we will
            // likely have to open the file again. If that's true, lets remove this call. jhrg 3.2.18
            int status = lseek(fd, 0, SEEK_SET);
            if (-1 == status)
                throw BESError("Could not seek within the response.", BES_NOT_FOUND_ERROR, __FILE__, __LINE__);
            BESDEBUG(MODULE, prolog << "Reset file descriptor." << endl);

            ingest_http_headers_and_type();
        }
        catch (BESError &e) {
            throw;
        }
        BESDEBUG(MODULE, prolog << "END" << endl);
    }


    void RemoteHttpResource::ingest_http_headers_and_type() {
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
                string value = header.substr(colon_index + colon_space.size());
                BESDEBUG(MODULE, prolog << "key: " << key << " value: " << value << endl);
                (*d_http_response_headers)[key] = value;
            }
        }
        std::map<string, string>::iterator it;
        string type;

        // Try and figure out the file type first from the
        // Content-Disposition in the http header response.

        string content_disp_hdr;
        content_disp_hdr = get_http_response_header("content-disposition");
        if (!content_disp_hdr.empty()) {
            // Content disposition exists, grab the filename
            // attribute
            NgapUtils::Get_type_from_disposition(content_disp_hdr, type);
            BESDEBUG(MODULE,prolog << "Evaluated content-disposition '" << content_disp_hdr << "' matched type: \"" << type << "\"" << endl);
        }

        // still haven't figured out the type. Check the content-type
        // next, translate to the BES MODULE name. It's also possible
        // that even though Content-disposition was available, we could
        // not determine the type of the file.
        string content_type = get_http_response_header("content-type");
        if (type.empty() && !content_type.empty()) {
            NgapUtils::Get_type_from_content_type(content_type, type);
            BESDEBUG(MODULE,prolog << "Evaluated content-type '" << content_type << "' matched type \"" << type << "\"" << endl);
        }

        // still haven't figured out the type. Now check the actual URL
        // and see if we can't match the URL to a MODULE name
        if (type.empty()) {
            NgapUtils::Get_type_from_url(d_remoteResourceUrl, type);
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
        d_type = type;
        BESDEBUG(MODULE, prolog << "END (dataset type: " << d_type << ")" << endl);
    }

    /**
     * Returns the value of the requested HTTP response header.
     * Evaluation is case-insensitive.
     * If the requested header_name is not found the empty string is returned.
     */
    std::string
    RemoteHttpResource::get_http_response_header(const std::string header_name) {
        string value("");
        std::map<string, string>::iterator it;
        it = d_http_response_headers->find(BESUtil::lowercase(header_name));
        if (it != d_http_response_headers->end())
            value = it->second;
        return value;
    }


    /**
     * Filter the cache and replaces all occurances of template_str with update_str.
     *
     * WARNING: Does not lock cache. This method assumes that the process has already
     * acquired an exclusive lock on the cache file.
     *
     * @param template_str
     * @param update_str
     * @return
     */
    unsigned int RemoteHttpResource::filter_retrieved_resource(const std::string &template_str, const std::string &update_str){
        unsigned int replace_count = 0;

        //  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
        // Read the dmr++ file into a string object
        std::ifstream cr_istrm(d_resourceCacheFileName);
        if (!cr_istrm.is_open()) {
            string msg = "Could not open '" + d_resourceCacheFileName + "' to read cached response.";
            BESDEBUG(MODULE, prolog << msg << endl);
            throw BESInternalError(msg, __FILE__, __LINE__);
        }
        std::stringstream buffer;
        buffer << cr_istrm.rdbuf();
        string resource_content(buffer.str());

        //  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
        // Replace all occurrences of the dmr++ href attr key.
        int startIndex = 0;
        while ((startIndex = resource_content.find(template_str)) != -1) {
            resource_content.erase(startIndex, template_str.size());
            resource_content.insert(startIndex, update_str);
            replace_count++;
        }

        //  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
        // Replace the contents of the cached dmr++ file with the modified string.
        std::ofstream cr_ostrm(d_resourceCacheFileName);
        if (!cr_ostrm.is_open()) {
            string msg = "Could not open '" + d_resourceCacheFileName + "' to write modified cached response.";
            BESDEBUG(MODULE, prolog << msg << endl);
            throw BESInternalError(msg, __FILE__, __LINE__);
        }
        cr_ostrm << resource_content;

        return replace_count;
    }

    /**
     * Returns cache file content in a string..
     */
    std::string RemoteHttpResource::get_response_as_string(){

        if(!d_initialized){
            stringstream msg;
            msg << "ERROR. Internal state error. " << __PRETTY_FUNCTION__ << " was called prior to retrieving resource.";
            BESDEBUG(MODULE, prolog << msg.str() << endl);
            throw BESInternalError(msg.str(), __FILE__, __LINE__);
        }
        string cache_file = getCacheFileName();
        //  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
        // Set up dmr input stream.
        // If no valid dmr input file is provided the code tries to find a dmr in the mds.
        std::ifstream dmr_istream(cache_file, std::ofstream::in);

        // If the dmr_filename is not valid, the stream will not open. Empty is not valid.
        if(dmr_istream.is_open()){
            // If it's open we've got a valid filename.
            BESDEBUG(MODULE, prolog << "Using cached file: " << cache_file << endl);
            std::ifstream t(cache_file);
            std::stringstream buffer;
            buffer << t.rdbuf();
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
    rapidjson::Document RemoteHttpResource::get_as_json() {
        string response = get_response_as_string();
        rapidjson::Document d;
        d.Parse(response.c_str());
        return d;
    }



}

