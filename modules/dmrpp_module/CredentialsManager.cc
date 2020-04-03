// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of the BES

// Copyright (c) 2020 OPeNDAP, Inc.
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

#include "config.h"

#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <locale>
#include <string>
#include <sys/stat.h>

#include <WhiteList.h>
#include <TheBESKeys.h>
#include <kvp_utils.h>
#include <BESInternalError.h>
#include <BESDebug.h>

#include "CredentialsManager.h"
#include "NgapS3Credentials.h"

using namespace std;

#define MODULE "dmrpp:creds"

#define prolog std::string("CredentialsManager::").append(__func__).append("() - ")

/**
 * Our singleton instance
 */
CredentialsManager *CredentialsManager::theMngr=0;

// Scope: public members of CredentialsManager
const string CredentialsManager::ENV_ID_KEY="CMAC_ID";
const string CredentialsManager::ENV_ACCESS_KEY="CMAC_ACCESS_KEY";
const string CredentialsManager::ENV_REGION_KEY="CMAC_REGION";
//const string CredentialsManager::ENV_BUCKET_KEY="CMAC_BUCKET";
const string CredentialsManager::ENV_URL_KEY="CMAC_URL";
const string CredentialsManager::ENV_CREDS_KEY_VALUE="ENV_CREDS";
const string CredentialsManager::NETRC_FILE_KEY="BES.netrc.credentials";


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
    for (std::map<std::string, AccessCredentials *>::iterator it = creds.begin(); it != creds.end(); ++it) {
        delete it->second;
    }
    creds.clear();
}

/**
 * Really it's the default constructor for now.
 */
CredentialsManager::CredentialsManager(): ngaps3CredentialsLoaded(false){
    bool found;
    d_netrc_filename="";
    TheBESKeys::TheKeys()->get_value(NETRC_FILE_KEY,d_netrc_filename,found);
    if(found){
        BESDEBUG(MODULE, prolog << "Using netrc file: " << d_netrc_filename << endl);
    }
    else {
        BESDEBUG(MODULE, prolog << "Using ~/.netrc file." << endl);
    }
}

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
 * Check fr file existence
 * @param filename Name of file to check
 * @return true if file exists, false otherwise.
 */
bool file_exists(const string &filename) {
    struct stat buffer;
    return (stat (filename.c_str(), &buffer) == 0);
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
        err.append("file_is_secured() Unable to access file ");
        err.append(filename).append("  strerror: ").append(strerror(errno));
        throw BESInternalError(err, __FILE__, __LINE__);
    }

    mode_t perm = st.st_mode;
    bool status;
    status = (perm & S_IRUSR) && !(
            // (perm & S_IWUSR) || // We don't need to enforce user no write
            (perm & S_IXUSR) ||
            (perm & S_IRGRP) ||
            (perm & S_IWGRP) ||
            (perm & S_IXGRP) ||
            (perm & S_IROTH) ||
            (perm & S_IWOTH) ||
            (perm & S_IXOTH));
    BESDEBUG(MODULE, "file_is_secured() " << filename << " secured: " << (status ? "true" : "false") << endl);
    return status;
}

/**
 * This method loads credentials from a special file identified in the bes.conf chain
 * by the key "CredentialsManager.config". If the key is missing from the bes.conf chain
 * the method will return and no credentials will be loaded.
 *
 * The credentials are stored as a map of maps  where the key is the human readable name
 * of the credentials.
 * The map of maps is accomplished by the following formatting:
 *
 * cloudydap=url:https://s3.amazonaws.com/cloudydap/
 * cloudydap+=id:---------------------------
 * cloudydap+=key:**************************
 * cloudydap+=region:us-east-1
 * cloudydap+=bucket:cloudydap
 *
 * cloudyopendap=url:https://s3.amazonaws.com/cloudyopendap/
 * cloudyopendap+=id:---------------------------
 * cloudyopendap+=key:**************************
 * cloudyopendap+=region:us-east-1
 * cloudyopendap+=bucket:cloudyopendap
 *
 * cname_02=url:https://ssotherone.org/login
 * cname_02+=id:---------------------------
 * cname_02+=key:**************************
 * cname_02+=region:us-east-1
 * cname_02+=bucket:cloudyotherdap

 * @throws BESInternalError if the file specified by the "CredentialsManager.config"
 * key is missing.
 */
void CredentialsManager::load_credentials( ) {

    bool found_key = true;
    AccessCredentials *accessCredentials;
    map<string, AccessCredentials *> credential_sets;

    string config_file;
    TheBESKeys::TheKeys()->get_value(CATALOG_MANAGER_CREDENTIALS, config_file, found_key);
    if(!found_key){
        BESDEBUG(MODULE, "The BES key " << CATALOG_MANAGER_CREDENTIALS
        << " was not found in the BES configuration tree. No AccessCredentials were loaded" << endl);
        return;
    }

    // Does the configuration indicate that credentials will be submitted via the runtime environment?
    if(config_file == ENV_CREDS_KEY_VALUE){
        // Apparently so...
        accessCredentials = theCM()->load_credentials_from_env();
        if(accessCredentials){
            // So if we have them, we add them to theCM() and then return without processing the configuration.
            string url = accessCredentials->get(AccessCredentials::URL_KEY);
            theCM()->add(url,accessCredentials);
        }
        // Environment injected credentials override all other configuration credentials.
        // Since the value of CATALOG_MANAGER_CREDENTIALS is  ENV_CREDS_VALUE there is no
        // Configuration file identified, so wether or not valid credentials information was
        // found in the ENV we simply return.
        return;
    }

    theCM()->load_ngap_s3_credentials();

    if(!file_exists(config_file)){
        BESDEBUG(MODULE, "The file specified by the BES key " << CATALOG_MANAGER_CREDENTIALS
        << " does not exist. No Access Credentials were loaded." << endl);
        return;
    }

    if (!file_is_secured(config_file)) {
        string err;
        err.append("CredentialsManager config file ");
        err.append(config_file);
        err.append(" is not secured! ");
        err.append("Set the access permissions to -rw------- (600) and try again.");
        throw BESInternalError(err, __FILE__, __LINE__);
    }
    BESDEBUG(MODULE, "CredentialsManager config file '" << config_file << "' is secured." << endl);

    map <string, vector<string>> keystore;

    kvp::load_keys(config_file, keystore);

    for(map <string, vector<string>>::iterator it=keystore.begin(); it!=keystore.end(); it++) {
        string creds_name = it->first;
        vector<string> &credentials_entries = it->second;
        map<string, AccessCredentials *>::iterator mit;
        mit = credential_sets.find(creds_name);
        if (mit != credential_sets.end()) {  // New?
            // Nope.
            accessCredentials = mit->second;
        } else {
            // Make new one
            accessCredentials = new AccessCredentials(creds_name);
            credential_sets.insert(pair<string, AccessCredentials *>(creds_name, accessCredentials));
        }
        for (vector<string>::iterator jt = credentials_entries.begin(); jt != credentials_entries.end(); jt++) {
            string credentials_entry = *jt;
            int index = credentials_entry.find(":");
            if (index > 0) {
                string key_name = credentials_entry.substr(0, index);
                string value = credentials_entry.substr(index + 1);
                BESDEBUG(MODULE, creds_name << ":" << key_name << "=" << value << endl);
                accessCredentials->add(key_name, value);
            }
        }
    }
    BESDEBUG(MODULE, "CredentialsManager loaded " << credential_sets.size()  << " AccessCredentials" << endl);
    vector<AccessCredentials *> bad_creds;
    map<string,AccessCredentials *>::iterator acit;

    for (acit = credential_sets.begin(); acit != credential_sets.end(); acit++) {
        accessCredentials = acit->second;
        string url = accessCredentials->get(AccessCredentials::URL_KEY);
        if(url.length()){
            theCM()->add(url,accessCredentials);
        }
        else {
            bad_creds.push_back(acit->second);
        }
    }
    if(bad_creds.size()){
        stringstream ss;
        vector<AccessCredentials * >::iterator bc;

        ss << "Encountered " << bad_creds.size() <<  " AccessCredentials "
           << " definitions missing an associated URL. offenders: ";

        for (bc = bad_creds.begin(); bc != bad_creds.end(); bc++) {
            ss << (*bc)->name() << "  ";
            credential_sets.erase((*bc)->name());
            delete *bc;
        }
        throw BESInternalError( ss.str(), __FILE__, __LINE__);
    }
    BESDEBUG(MODULE, "CredentialsManager has successfully ingested " << theCM()->size()  << " AccessCredentials" << endl);

}


/**
 *
 * @return
 */
AccessCredentials *CredentialsManager::load_credentials_from_env( ) {

    AccessCredentials *ac = NULL;
    string env_url, env_id, env_access_key, env_region, env_bucket;

    // If we are in developer mode then we compile this section which
    // allows us to inject credentials via the system environment

    env_id.assign(        get_env_value(ENV_ID_KEY));
    env_access_key.assign(get_env_value(ENV_ACCESS_KEY));
    env_region.assign(    get_env_value(ENV_REGION_KEY));
    //env_bucket.assign(    get_env_value(ENV_BUCKET_KEY));
    env_url.assign(       get_env_value(ENV_URL_KEY));

    if(env_url.length() &&
            env_id.length() &&
            env_access_key.length() &&
            // env_bucket.length() &&
            env_region.length() ){
        ac = new AccessCredentials();
        ac->add(AccessCredentials::URL_KEY, env_url);
        ac->add(AccessCredentials::ID_KEY, env_id);
        ac->add(AccessCredentials::KEY_KEY, env_access_key);
        ac->add(AccessCredentials::REGION_KEY, env_region);
       // ac->add(AccessCredentials::BUCKET_KEY, env_bucket);
    }
    return ac;
}


std::string NGAP_S3_BASE_DEFAULT="https://";
/**
 * Read the BESKeys (from bes.conf chain) and if NgapS3Credentials::BES_CONF_S3_ENDPOINT_KEY is present builds
 * and adds to the CredentialsManager an instance of NgapS3Credentials based on the values found in the bes.conf chain.
 */
void  CredentialsManager::load_ngap_s3_credentials( ){
    string s3_distribution_endpoint_url;
    bool found;
    TheBESKeys::TheKeys()->get_value(NgapS3Credentials::BES_CONF_S3_ENDPOINT_KEY,s3_distribution_endpoint_url,found);
    if(found) {
        string value;

        long refresh_margin = 600;
        TheBESKeys::TheKeys()->get_value(NgapS3Credentials::BES_CONF_REFRESH_KEY, value, found);
        if (found) {
            refresh_margin = strtol(value.c_str(), 0, 10);
        }

        string s3_base_url = NGAP_S3_BASE_DEFAULT;
        TheBESKeys::TheKeys()->get_value(NgapS3Credentials::BES_CONF_URL_BASE, value, found);
        if (found) {
            s3_base_url = value;
        }

        NgapS3Credentials *nsc = new NgapS3Credentials(s3_distribution_endpoint_url, refresh_margin);
        nsc->add(NgapS3Credentials::URL_KEY, s3_base_url);
        nsc->name("NgapS3Credentials");

        CredentialsManager::theCM()->add(s3_base_url,nsc);
        CredentialsManager::theCM()->ngaps3CredentialsLoaded = true;

    }
    else {
        BESDEBUG(MODULE,
                "WARNING: The BES configuration did not contain an instance of " <<
                NgapS3Credentials::BES_CONF_S3_ENDPOINT_KEY <<
                " NGAP S3 Credentials NOT loaded." << endl);
    }
}

