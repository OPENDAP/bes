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
using std::map;
using std::pair;
using std::stringstream;
using std::endl;


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
string
AccessCredentials::get(const string &key){
    map<string, string>::iterator it;
    string value("");
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
        const string &key,
        const string &value){
    kvp.insert(pair<string, string>(key, value));
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

string AccessCredentials::to_json(){
    stringstream ss;
    ss << "{" << endl << "  \"AccessCredentials\": { " << endl;
    ss << "    \"name\": \"" << d_config_name << "\"," << endl;
    for (map<string, string>::iterator it = kvp.begin(); it != kvp.end(); ++it) {
        string key = it->first;
        string value = it->second;

        if(it!=kvp.begin())
            ss << ", " << endl ;

        ss << "    \"" << it->first << "\": \"" << it->second << "\"";
    }
    ss << endl << "  }" << endl << "}" << endl;
    return ss.str();
}