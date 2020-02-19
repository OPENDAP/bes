//
// Created by ndp on 2/12/20.
//

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

#include <BESError.h>
#include <BESDebug.h>

#include "curl_utils.h"

#include "NgapS3Credentials.h"
using std::string;
using std::endl;

#define AWS_ACCESS_KEY_ID_KEY "accessKeyId"
#define AWS_SECRET_ACCESS_KEY_KEY "secretAccessKey"
#define AWS_SESSION_TOKEN_KEY "sessionToken"
#define AWS_EXPIRATION_KEY "expiration"


#define MODULE "ngap"

void NgapS3Credentials::get_temporary_credentials(const string &distribution_api_endpoint) {

    string accessKeyId, secretAccessKey, sessionToken, expiration;

    BESDEBUG(MODULE,  " distribution_api_endpoint: " << distribution_api_endpoint << endl);

    rapidjson::Document d = curl::http_get_as_json(distribution_api_endpoint);
    BESDEBUG(MODULE, "S3 Credentials:"  << endl);

    rapidjson::Value& val = d[AWS_ACCESS_KEY_ID_KEY];
    accessKeyId = val.GetString();
    add(AccessCredentials::ID_KEY,accessKeyId);
    BESDEBUG(MODULE, AWS_ACCESS_KEY_ID_KEY << ":        "  << accessKeyId  << endl);

    val = d[AWS_SECRET_ACCESS_KEY_KEY];
    secretAccessKey = val.GetString();
    add(AccessCredentials::KEY_KEY,secretAccessKey);
    BESDEBUG(MODULE, AWS_SECRET_ACCESS_KEY_KEY << ":    "  << secretAccessKey << endl);

    val = d[AWS_SESSION_TOKEN_KEY];
    sessionToken = val.GetString();
    add("AWS_SESSION_TOKEN",sessionToken);
    BESDEBUG(MODULE, AWS_SESSION_TOKEN_KEY << ":       " << sessionToken  << endl);

    val = d[AWS_EXPIRATION_KEY];
    expiration = val.GetString();
    add("AWS_TOKEN_EXPIRATION",expiration);
    BESDEBUG(MODULE, AWS_EXPIRATION_KEY << ":         "  << expiration  << endl);


    // parse the time string into a something useful -------------------------------------------------------
    struct tm tm;
    // 2020-02-18 13:49:30+00:00
    strptime(expiration.c_str(), "%Y-%m-%d %H:%M:%S%z", &tm);
    d_expiration_time = mktime(&tm);  // t is now your desired time_t
    BESDEBUG(MODULE, "expiration(time_t): "  << d_expiration_time  << endl);

}



