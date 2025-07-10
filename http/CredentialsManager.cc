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
#include <string>
#include <unordered_map>
#include <sys/stat.h>

#include "AllowedHosts.h"
#include "TheBESKeys.h"
#include "kvp_utils.h"
#include "BESInternalError.h"
#include "BESDebug.h"
#include "BESLog.h"
#include "HttpNames.h"

#include "CredentialsManager.h"

using namespace std;

#define prolog std::string("CredentialsManager::").append(__func__).append("() - ")

#define CREDS "creds"

namespace http {

// Class vocabulary
const char *CredentialsManager::ENV_ID_KEY = "CMAC_ID";
const char *CredentialsManager::ENV_ACCESS_KEY = "CMAC_ACCESS_KEY";
const char *CredentialsManager::ENV_REGION_KEY = "CMAC_REGION";
const char *CredentialsManager::ENV_URL_KEY = "CMAC_URL";

const char *CredentialsManager::USE_ENV_CREDS_KEY_VALUE = "ENV_CREDS";

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//
// Helper Functions
//
/**
 *  Get get the specified environment value. This function
 *  returns an empty string if the environment variable is
 *  not found, AND if the environment value is set but empty.
 * @param key The environment value to retrieve
 * @return The value of the environment variable,
 * or the empty string is not found.
 */
std::string get_env_value(const string &key) {
    string value;
    const char *cstr = getenv(key.c_str());
    if (cstr) {
        value.assign(cstr);
        BESDEBUG(CREDS, prolog << "From system environment - " << key << ": " << value << endl);
    } else {
        value.clear();
    }
    return value;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//
// class CredentialsManager
//

/**
 * @brief Returns the singleton instance of the CredentialsManager.
 * @return Returns the singleton instance of the CredentialsManager
 */
CredentialsManager *CredentialsManager::theCM() {
    static CredentialsManager theMngr;
    return &theMngr;
}

/**
 * Add the passed set of AccessCredentials to the collection, filed under key.
 * @param key The key (URL) to associated with these credentials
 * @param ac The credentials to use for access.
 */
void
CredentialsManager::add(const std::string &key, AccessCredentials *ac) {
    // This lock is a RAII implementation. It will block until the mutex is
    // available and the lock will be released when the instance is destroyed.
    std::lock_guard<std::recursive_mutex> lock_me(d_lock_mutex);

    creds.insert(std::pair<std::string, AccessCredentials *>(key, ac));
    BESDEBUG(HTTP_MODULE, prolog << "Added AccessCredentials to CredentialsManager.\n");
    BESDEBUG(CREDS, prolog << "Credentials: \n" << ac->to_json() << "\n");
}

/**
 * Retrieve the AccessCredentials, if any, associated with the passed url (key).
 * @param url The URL for which AccessCredentials are desired
 * @return If there are AccessCredentials associated with the URL/key then a pointer to
 * them will be returned. Otherwise, NULL.
 */
AccessCredentials *
CredentialsManager::get(const shared_ptr <http::url> &url) {
    // This lock is a RAII implementation. It will block until the mutex is
    // available and the lock will be released when the instance is destroyed.
    std::lock_guard<std::recursive_mutex> lock_me(d_lock_mutex);

    AccessCredentials *best_match = nullptr;
    std::string best_key;

    if (url->protocol() == HTTP_PROTOCOL || url->protocol() == HTTPS_PROTOCOL) {
        for (auto &item: creds) {
            const std::string &key = item.first;
            if ((url->str().rfind(key, 0) == 0) && (key.size() > best_key.size())) {
                // url starts with key
                best_key = key;
                best_match = item.second;
            }
        }
    }

    return best_match;
}

AccessCredentials *
CredentialsManager::get(const std::string &url) {
    // Check the protocol before locking the credential manager. jhrg 2/20/25
    const auto protocol = url.substr(0, url.find(':'));
    if (url.find(HTTP_PROTOCOL) != 0 && url.find(HTTPS_PROTOCOL) != 0)
        return nullptr;

    // This lock is a RAII implementation. It will block until the mutex is
    // available and the lock will be released when the instance is destroyed.
    std::lock_guard<std::recursive_mutex> lock_me(d_lock_mutex);

    AccessCredentials *best_match = nullptr;
    for (auto &item: creds) {
        std::string best_key;
        const std::string &key = item.first;
        if ((url.rfind(key, 0) == 0) && (key.size() > best_key.size())) {
            // url starts with key
            best_key = key;
            best_match = item.second;
        }
    }

    return best_match;
}


/**
 * Check for file existence
 * @param filename Name of file to check
 * @return true if file exists, false otherwise.
 */
bool file_exists(const string &filename) {
    struct stat buffer{};
    return (stat(filename.c_str(), &buffer) == 0);
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
    struct stat st{};
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
    BESDEBUG(HTTP_MODULE,
             prolog << "file_is_secured() " << filename << " secured: " << (status ? "true" : "false") << endl);
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
 * @todo FIXME The 'bucket' key is not used. jhrg 2/19/25
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
 *
 * @throws BESInternalError if the file specified by the "CredentialsManager.config"
 * key is missing or if the file is not secured (permissions 600).
 *
 * @note NEVER call this method directly. It is called using call_once() by the
 * singleton accessor.
 */
void CredentialsManager::load_credentials() {
    string config_file;
    bool found_key = true;
    TheBESKeys::TheKeys()->get_value(CATALOG_MANAGER_CREDENTIALS, config_file, found_key);
    if (!found_key) {
        BESDEBUG(HTTP_MODULE, prolog << "The BES key " << CATALOG_MANAGER_CREDENTIALS
                                     << " was not found in the BES configuration tree. No AccessCredentials were loaded"
                                     << endl);
        return;
    }

    // Does the configuration indicate that credentials will be submitted via the runtime environment?
    if (config_file == string(CredentialsManager::USE_ENV_CREDS_KEY_VALUE)) {
        // Apparently so...
        auto *accessCredentials = load_credentials_from_env();
        if (accessCredentials) {
            // So if we have them, we add them to CredentialsManager object that called this method
            // and then return without processing the configuration.
            string url = accessCredentials->get(AccessCredentials::URL_KEY);
            add(url, accessCredentials);
        }
        // Environment injected credentials override all other configuration credentials.
        // Since the value of CATALOG_MANAGER_CREDENTIALS is ENV_CREDS_VALUE, there is no
        // configuration file identified, so we simply return regardless of whether valid
        // credential information was found in the ENV.
        return;
    }

    if (!file_exists(config_file)) {
        string err{prolog + "CredentialsManager config file "};
        err += config_file + " was specified but is not present.\n";    // Bjarne says to use \n not endl. jhrg 8/25/23
        ERROR_LOG(err);
        BESDEBUG(HTTP_MODULE, err);
        return;
    }

    if (!file_is_secured(config_file)) {
        string err{prolog + "CredentialsManager config file "};
        err += config_file + " is not secured! Set the access permissions to -rw------- (600) and try again.\n";
        ERROR_LOG(err);
        BESDEBUG(HTTP_MODULE, err);
        return;
    }

    BESDEBUG(HTTP_MODULE, prolog << "The config file '" << config_file << "' is secured." << endl);

    unordered_map<string, vector<string> > keystore;

    kvp::load_keys(config_file, keystore);
    map<string, AccessCredentials *> credential_sets;
    AccessCredentials *accessCredentials = nullptr;
    for (const auto &key: keystore) {
        string creds_name = key.first;
        const vector<string> &credentials_entries = key.second;
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

        for (const auto &entry: credentials_entries) {
            size_t index = entry.find(':');
            if (index > 0) {
                string key_name = entry.substr(0, index);
                string value = entry.substr(index + 1);
                BESDEBUG(HTTP_MODULE, prolog << creds_name << ":" << key_name << "=" << value << endl);
                accessCredentials->add(key_name, value);
            }
        }
    }

    BESDEBUG(HTTP_MODULE, prolog << "Loaded " << credential_sets.size() << " AccessCredentials" << endl);
    vector<AccessCredentials *> bad_creds;

    for (const auto &acit: credential_sets) {
        accessCredentials = acit.second;
        string url = accessCredentials->get(AccessCredentials::URL_KEY);
        if (!url.empty()) {
            add(url, accessCredentials);
        } else {
            bad_creds.push_back(acit.second);
        }
    }

    if (!bad_creds.empty()) {
        stringstream err;
        err << "Encountered " << bad_creds.size() << " AccessCredentials "
            << " definitions missing an associated URL. offenders: ";

        for (auto &bc: bad_creds) {
            err << bc->name() << "  ";
            credential_sets.erase(bc->name());
            delete bc;
        }
        ERROR_LOG(err.str());
        BESDEBUG(HTTP_MODULE, err.str());
        return;
    }
    BESDEBUG(HTTP_MODULE, prolog << "Successfully ingested " << size() << " AccessCredentials" << endl);
}


/**
 * @brief Attempts to load Access Credentials from the environment variables.
 *
 * WARNING: This method assumes that the Manager is located and makes no
 * effort to lock or unlock the manager.
 *
 * @return A pointer to AccessCredentials of successful, nullptr otherwise.
 */
AccessCredentials *CredentialsManager::load_credentials_from_env() {
    std::lock_guard<std::recursive_mutex> lock_me(d_lock_mutex);

    string env_id{get_env_value(CredentialsManager::ENV_ID_KEY)};
    string env_access_key{get_env_value(CredentialsManager::ENV_ACCESS_KEY)};
    string env_region{get_env_value(CredentialsManager::ENV_REGION_KEY)};
    string env_url{get_env_value(CredentialsManager::ENV_URL_KEY)};

    // evaluates to true iff none of the strings are empty. - ndp 08/07/23
    if (!(env_url.empty() || env_id.empty() || env_access_key.empty() || env_region.empty())) {
        auto ac = make_unique<AccessCredentials>();
        ac->add(AccessCredentials::URL_KEY, env_url);
        ac->add(AccessCredentials::ID_KEY, env_id);
        ac->add(AccessCredentials::KEY_KEY, env_access_key);
        ac->add(AccessCredentials::REGION_KEY, env_region);
        return ac.release();
    }
    return nullptr;
}

} // namespace http
