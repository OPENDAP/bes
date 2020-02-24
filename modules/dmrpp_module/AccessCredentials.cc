//
// Created by ndp on 2/12/20.
//

#include "CredentialsManager.h"
#include <BESDebug.h>
#include <BESInternalError.h>
#include <kvp_utils.h>
#include <TheBESKeys.h>
#include <WhiteList.h>
#include <sys/stat.h>
#include <string>
#include <locale>
#include <sstream>
#include <iomanip>
#include <cstring>
#include "config.h"
#include "AccessCredentials.h"

using std::string;


// Scope: public members of AccessCredentials
const string AccessCredentials::ID_KEY="id";
const string AccessCredentials::KEY_KEY="key";
const string AccessCredentials::REGION_KEY="region";
//const string AccessCredentials::BUCKET_KEY="bucket";
const string AccessCredentials::URL_KEY="url";

/**
 * Retrieves the value of key
 * @param key The key value to retrieve
 * @return The value of the key, empty string if the key does not exist.
 */
std::string
AccessCredentials::get(const std::string &key){
    std::__1::map<std::string, std::string>::iterator it;
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
    kvp.insert(std::__1::pair<std::string, std::string>(key, value));
}

/**
 *
 * @return
 */
bool AccessCredentials::isS3Cred(){
    if(!s3_tested){
        is_s3 = get(URL_KEY).length()>0 &&
                get(ID_KEY).length()>0 &&
                get(KEY_KEY).length()>0 &&
                get(REGION_KEY).length()>0; //&&
                //get(BUCKET_KEY).length()>0;
        s3_tested = true;
    }
    return is_s3;
}

std::__1::string AccessCredentials::to_json(){
    std::__1::stringstream ss;
    ss << "{" << std::__1::endl << "  \"AccessCredentials\": { " << std::__1::endl;
    ss << "    \"name\": \"" << d_config_name << "\"," << std::__1::endl;
    for (std::__1::map<std::__1::string, std::__1::string>::iterator it = kvp.begin(); it != kvp.end(); ++it) {
        std::string key = it->first;
        std::string value = it->second;

        if(it!=kvp.begin())
            ss << ", " << std::__1::endl ;

        ss << "    \"" << it->first << "\": \"" << it->second << "\"";
    }
    ss << std::__1::endl << "  }" << std::__1::endl << "}" << std::__1::endl;
    return ss.str();
}