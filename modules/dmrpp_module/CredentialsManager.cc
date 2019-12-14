// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of the BES

// Copyright (c) 2019 OPeNDAP, Inc.
// Author: Nathan Potter<ndp@opendap.org>
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
// Created by ndp on 12/11/19.
//

#include <curl/multi.h>
#include <curl/curl.h>
#include <ctime>
#include <unistd.h>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <locale>
#include <string>
#include <sstream>
#include <sys/types.h>
#include <sys/stat.h>

#include "WhiteList.h"
#include "TheBESKeys.h"
#include "BESForbiddenError.h"
#include "BESInternalError.h"
#include "BESDebug.h"
#include "BESLog.h"
#include "util.h"   // libdap::long_to_string()

#include "config.h"
#include "Chunk.h"
#include "CurlHandlePool.h"
#include "awsv4.h"
#include "DmrppCommon.h"

#include "kvp_utils.h"
#include "CredentialsManager.h"

using namespace std;

#define MODULE "dmrpp:creds"

/**
 * Our singleton instance
 */
CredentialsManager *CredentialsManager::theMngr=0;

// Scope: public members of AccessCredentials
const string AccessCredentials::ID="id";
const string AccessCredentials::KEY="key";
const string AccessCredentials::REGION="region";
const string AccessCredentials::BUCKET="bucket";
const string AccessCredentials::URL="url";

// Scope: This file...
const string CM_CREDENTIALS="CM.credentials";
const string CM_CONFIG="CredentialsManager.config";

#if 0
// These are all from the "OLD" method where there was one set of credentials
// for everything.
const string ENV_AKID_KEY("AWS_ACCESS_KEY_ID");
const string CONFIG_AKID_KEY("DMRPP.AWS_ACCESS_KEY_ID");

const string ENV_SAK_KEY("AWS_SECRET_ACCESS_KEY");
const string CONFIG_SAK_KEY("DMRPP.AWS_SECRET_ACCESS_KEY");

const string ENV_REGION_KEY("AWS_REGION");
const string CONFIG_REGION_KEY("DMRPP.AWS_REGION");

const string ENV_S3_BUCKET_KEY("AWS_S3_BUCKET");
const string CONFIG_S3_BUCKET_KEY("DMRPP.AWS_S3_BUCKET");
#endif



/**
 *  Get get the specified environment value. This function
 *  returns an empty string if the environment variable is
 *  not found, AND if the environment value is set but empty.
 * @param key The environment value to retrieve
 * @return The value of the environment variable,
 * or the empty string is not found.
 */
std::string get_env_value(const string &key){
    string value;
    const char *cstr = getenv(key.c_str());
    if(cstr){
        value.assign(cstr);
        BESDEBUG(MODULE, __FILE__ << " " << __LINE__ << " From system environment - " << key << ": " << value << endl);
    }
    else {
        value.clear();
    }
    return value;
}

/**
 *  Get get the specified bes.conf configuration value. This function
 *  returns an empty string if the configuration variable is
 *  not found, AND if the environment value is set but empty.
 * @param key The configuration value to retrieve
 * @return The value of the configuration variable,
 * or the empty string is not found.
 */
std::string get_config_value(const string &key){
    string value;
    bool key_found=false;
    TheBESKeys::TheKeys()->get_value(key, value, key_found);
    if (key_found) {
        BESDEBUG(MODULE, __FILE__ << " " << __LINE__ << " Using " << key << " from TheBESKeys" << endl);
    }
    else {
        value.clear();
    }
    return value;
}

/**
 * Destructo
 */
CredentialsManager::~CredentialsManager() {
    for (std::map<std::string,AccessCredentials*>::iterator it=creds.begin(); it != creds.end(); ++it){
        delete it->second;
    }
    creds.clear();
    delete_instance();
}

/**
 * Really it's the default constructor for now.
 */
CredentialsManager::CredentialsManager(){}

/**
 *
 */
void CredentialsManager::initialize_instance()
{
    theMngr = new CredentialsManager;
#ifdef HAVE_ATEXIT
    atexit(delete_instance);
#endif

}

/**
 * Private static function can only be called by friends and pThreads code.
 */
void CredentialsManager::delete_instance()
{
    delete theMngr;
    theMngr = 0;
}


/**
 * Add the passed set of AccessCredentials to the collection, filed under key.
 * @param key The key (URL) to associated with these credentials
 * @param ac The credentials to use for access.
 */
void
CredentialsManager::add(const std::string &key, AccessCredentials *ac){
    creds.insert(std::pair<std::string,AccessCredentials *>(key, ac));
    BESDEBUG(MODULE, "Added AccessCredentials to CredentialsManager. credentials: " << endl <<  ac->to_json() << endl);
}

/**
 * Retrieve the AccessCredentials, if any, associated with the passed url (key).
 * @param url The URL for which AccessCredentials are desired
 * @return If there are AccessCredentials associated with the URL/key then a point to
 * them will be returned. Otherwise, NULL.
 */
AccessCredentials*
CredentialsManager::get(const std::string &url){
    AccessCredentials *best_match = NULL;
    std::string best_key("");

    if(url.find("http://") == 0 || url.find("https://") == 0) {
        for (std::map<std::string, AccessCredentials *>::iterator it = creds.begin(); it != creds.end(); ++it) {
            std::string key = it->first;
            if (url.rfind(key, 0) == 0) {
                // url starts with key
                if (key.length() > best_key.length()) {
                    best_key = key;
                    best_match = it->second;
                }
            }
        }
    }

    return best_match;
}

/**
 * Uses stat() to check that the passed file has the
 * permissions 600 (rw-------). An exception is thrown
 * if the file does not exist.
 *
 *      modeval[0] = (perm & S_IRUSR) ? 'r' : '-';
        modeval[1] = (perm & S_IWUSR) ? 'w' : '-';
        modeval[2] = (perm & S_IXUSR) ? 'x' : '-';
        modeval[3] = (perm & S_IRGRP) ? 'r' : '-';
        modeval[4] = (perm & S_IWGRP) ? 'w' : '-';
        modeval[5] = (perm & S_IXGRP) ? 'x' : '-';
        modeval[6] = (perm & S_IROTH) ? 'r' : '-';
        modeval[7] = (perm & S_IWOTH) ? 'w' : '-';
        modeval[8] = (perm & S_IXOTH) ? 'x' : '-';
        modeval[9] = '\0';

 * @param filename
 * @return True if the passed file exists and has the permissions 600
 * (rw-------)
 */
bool file_is_secured(const string &filename) {
    struct stat st;
    if (stat(filename.c_str(), &st) != 0) {
        string err;
        err.append("file_is_locked() Unable to access file ");
        err.append(filename).append("  strerror: ").append(strerror(errno));
        throw BESInternalError(err, __FILE__, __LINE__);
    }

    mode_t perm = st.st_mode;
    bool status;
    status = (perm & S_IRUSR) && (perm & S_IWUSR) && !(
            (perm & S_IXUSR) ||
            (perm & S_IRGRP) ||
            (perm & S_IWGRP) ||
            (perm & S_IXGRP) ||
            (perm & S_IROTH) ||
            (perm & S_IWOTH) ||
            (perm & S_IXOTH));
    BESDEBUG(MODULE, "file_is_locked() " << filename << " locked: " << (status ? "true" : "false") << endl);
    return status;
}

/**
 * This method loads credentials from a special file identified in the bes.conf chain
 * by the key "CredentialsManager.config". If the key is missing from the bes.conf chain
 * the method will return and no credentials will be loaded.
 *
 * @throws BESInternalError if the file specified by the "CredentialsManager.config"
 * key is missing.
 */
void CredentialsManager::load_credentials_NEW( ) {
    bool found = true;

    map<string, AccessCredentials *> credential_sets;
    AccessCredentials *accessCredentials;

    vector < string > credentials_entries;

    string config_file;
    TheBESKeys::TheKeys()->get_value(CM_CONFIG, config_file, found);
    if (found) {
        if (file_is_secured(config_file)) {
            BESDEBUG(MODULE, "CredentialsManager config file '" << config_file << "' is secured." << endl);

            std::map <std::string, std::vector<std::string>> keystore;
            load_keys(config_file, keystore);

        } else {
            string err;
            err.append("CredentialsManager config file ");
            err.append(config_file);
            err.append(" is not secured! ");
            err.append("Set the access permissions to -rw------- (600) and try again.");
            throw BESInternalError(err, __FILE__, __LINE__);
        }
    }
}

/**
 * This method loads access credentials from the bes.conf chain.
 * The credentials are stored as a list of lists under the key
 * "CM.credentials". The list of lists is accomplished by the
 * following formatting:
 *   CM.credentials+=cloudydap:url:https://s3.amazonaws.com/cloudydap/
 *   CM.credentials+=cloudydap:id:---------------------------
 *   CM.credentials+=cloudydap:key:**************************
 *   CM.credentials+=cloudydap:region:us-east-1
 *   CM.credentials+=cloudydap:bucket:cloudydap
 *
 *   CM.credentials+=cloudyopendap:url:https://s3.amazonaws.com/cloudyopendap/
 *   CM.credentials+=cloudyopendap:id:---------------------------
 *   CM.credentials+=cloudyopendap:key:**************************
 *   CM.credentials+=cloudyopendap:region:us-east-1
 *   CM.credentials+=cloudyopendap:bucket:cloudyopendap
 *
 *   CM.credentials+=cname_02:url:https://ssotherone.org/login
 *   CM.credentials+=cname_02:id:---------------------------
 *   CM.credentials+=cname_02:key:**************************
 *   CM.credentials+=cname_02:region:us-east-1
 *   CM.credentials+=cname_02:bucket:cloudyotherdap
 *
 *   The sub keys shown here (url,id,key,bucket) are all public const strings in the
 *   AccessCredentials class, however any sub key may be used.
 */
void CredentialsManager::load_credentials( ){
    bool found = true;

    map<string,AccessCredentials *> credential_sets;
    AccessCredentials *accessCredentials;

    vector<string> credentials_entries;

    TheBESKeys::TheKeys()->get_values(CM_CREDENTIALS, credentials_entries,  found);
    if (found) {
        vector<string>::iterator it;
         for (it = credentials_entries.begin(); it != credentials_entries.end(); it++) {
            string credentials_entry = *it;
            accessCredentials = NULL;
            int index = credentials_entry.find(":");
            if (index > 0) {
                string config_name = credentials_entry.substr(0, index);
                string remainder = credentials_entry.substr(index + 1);
                BESDEBUG(MODULE,
                         "config_name: '" << config_name << "'  remainder: " << remainder << endl);

                map<string,AccessCredentials *>::iterator mit;

                mit = credential_sets.find(config_name);
                if (mit != credential_sets.end()) {  // New?
                    accessCredentials = mit->second;
                }
                else { // Nope.
                    accessCredentials = new AccessCredentials(config_name);
                    credential_sets.insert(pair<string, AccessCredentials *>(config_name,accessCredentials));
                }
                index = remainder.find(":");

                if (index > 0) {
                    string key = remainder.substr(0, index);
                    string value = remainder.substr(index + 1);
                    accessCredentials->add(key,value);
                } else {
                    throw BESInternalError(
                            string("The configuration entry for the ")
                            + CM_CREDENTIALS
                            + " was incorrectly formatted. entry: "
                            + credentials_entry, __FILE__, __LINE__);
                }
            } else {
                throw BESInternalError(
                        string("The configuration entry for the ")
                        + CM_CREDENTIALS
                        + " was incorrectly formatted. entry: "
                        + credentials_entry, __FILE__, __LINE__);
            }
        }
        vector<string> pitch;
        map<string,AccessCredentials *>::iterator acit;

        for (acit = credential_sets.begin(); acit != credential_sets.end(); acit++) {
            accessCredentials = acit->second;
            string url = accessCredentials->get(AccessCredentials::URL);
            if(url.length()){
                theCM()->add(url,accessCredentials);
            }
            else {
                pitch.push_back(acit->first);
            }
        }
        if(pitch.size()){
            stringstream ss;
            vector<string>::iterator pt;

            ss << "Encountered " << pitch.size() <<  CM_CREDENTIALS
               << " definitions missing an associated URL. offenders: ";

            for (pt = credentials_entries.begin(); pt != credentials_entries.end(); pt++)
                ss << *pt << "  ";

            throw BESInternalError( ss.str(), __FILE__, __LINE__);

        }
    }

}

#if 0 // DISABLED OLD way
/**
 * Load the AccessCredentials.
 */
void CredentialsManager::load_credentials_OLD() {
    string aws_akid, aws_sak, aws_region, aws_s3_bucket;


#ifndef NDEBUG

    // If we are in developer mode then we compile this section which
    // allows us to inject credentials via the system environment

    aws_akid.assign(     get_env_value(ENV_AKID_KEY));
    aws_sak.assign(      get_env_value(ENV_SAK_KEY));
    aws_region.assign(   get_env_value(ENV_REGION_KEY));
    aws_s3_bucket.assign(get_env_value(ENV_S3_BUCKET_KEY));

    BESDEBUG(MODULE, __FILE__ << " " << __LINE__
                              << " From ENV aws_akid: '" << aws_akid << "' "
                              << "aws_sak: '" << aws_sak << "' "
                              << "aws_region: '" << aws_region << "' "
                              << "aws_s3_bucket: '" << aws_s3_bucket << "' "
                              << endl);

#endif

    // In production mode this is the single point of ingest for credentials.
    // Developer mode enables the piece above which allows the environment to
    // overrule the configuration

    if(aws_akid.length()){
        BESDEBUG(MODULE, __FILE__ << " " << __LINE__ << " Using " << ENV_AKID_KEY << " from the environment." << endl);
    }
    else {
        aws_akid.assign(get_config_value(CONFIG_AKID_KEY));
    }

    if(aws_sak.length()){
        BESDEBUG(MODULE, __FILE__ << " " << __LINE__ << " Using " << ENV_SAK_KEY << " from the environment." << endl);
    }
    else {
        aws_sak.assign(get_config_value(CONFIG_SAK_KEY));
    }

    if(aws_region.length()){
        BESDEBUG(MODULE, __FILE__ << " " << __LINE__ << " Using " << ENV_REGION_KEY << " from the environment." << endl);
    }
    else {
        aws_region.assign(get_config_value(CONFIG_REGION_KEY));
    }

    if(aws_s3_bucket.length()){
        BESDEBUG(MODULE, __FILE__ << " " << __LINE__ << " Using " << ENV_S3_BUCKET_KEY << " from the environment." << endl);
    }
    else {
        aws_s3_bucket.assign(get_config_value(CONFIG_S3_BUCKET_KEY));
    }
    BESDEBUG(MODULE, __FILE__ << " " << __LINE__
                              << " END aws_akid: '" << aws_akid << "' "
                              << "aws_sak: '" << aws_sak << "' "
                              << "aws_region: '" << aws_region << "' "
                              << "aws_s3_bucket: '" << aws_s3_bucket << "' "
                              << endl);
    AccessCredentials *ac = new AccessCredentials("single_user", aws_akid,aws_sak,aws_region,aws_s3_bucket);
    theCM()->add("https://", ac);
}
#endif

/*****************************************************************************************************/
/*****************************************************************************************************/
/*****************************************************************************************************/
/*****************************************************************************************************/
// AccessCredentials methods follow.

/**
 *
 * @param name The human-readable name associated these this AccessCredentials
 * @param id The user id of the credentials
 * @param key The password/secret_key what have you
 */
AccessCredentials::AccessCredentials(
        const std::string config_name,
        const std::string &id,
        const std::string &key) : AccessCredentials(config_name){
    add(ID,id);
    add(KEY,key);
}

/**
 *
 * @param name The human-readable name associated these this AccessCredentials
 * @param id The user id of the credentials
 * @param key The password/secret_key what have you
 * @param region The AWS region to be logging into.
 * @param bucket The S3 bucket name that these credentials can access.
 */
AccessCredentials::AccessCredentials(
        const std::string config_name,
        const std::string &id,
        const std::string &key,
        const std::string &region,
        const std::string &bucket)
        : AccessCredentials(config_name, id, key) {
    add(REGION,region);
    add(BUCKET,bucket);
}

/**
 * Retrieves the value of key
 * @param key The key value to retrieve
 * @return The value of the key, empty string if the key does not exist.
 */
std::string
AccessCredentials::get(const std::string &key){
    std::map<std::string, std::string>::iterator it;
    std::string value("");
    it = kvp.find(key);
    if (it != kvp.end())
        value = it->second;
    return value;
}

/**
 *
 * @param key
 * @param value
 */
void
AccessCredentials::add(
        const std::string &key,
        const std::string &value){
    kvp.insert(std::pair<std::string, std::string>(key, value));
}

/**
 *
 * @return
 */
bool AccessCredentials::isS3Cred(){
    if(!s3_tested){
        is_s3 = get(ID).length()>0 &&
                get(KEY).length()>0 &&
                get(REGION).length()>0 &&
                get(BUCKET).length()>0;
        s3_tested = true;
    }
    return is_s3;
}

string AccessCredentials::to_json(){
    stringstream ss;
    ss << "{" << endl << "  \"AccessCredentials\": { " << endl;
    ss << "    \"name\": \"" << conf_name << "\"," << endl;
    for (std::map<string, string>::iterator it = kvp.begin(); it != kvp.end(); ++it) {
        std::string key = it->first;
        std::string value = it->second;

        if(it!=kvp.begin())
            ss << ", " << endl ;

        ss << "    \"" << it->first << "\": \"" << it->second << "\"";
    }
    ss << endl << "  }" << endl << "}" << endl;
    return ss.str();

}
