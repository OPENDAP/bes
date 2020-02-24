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

// Scope: public members of AccessCredentials
const string NgapS3Credentials::AWS_SESSION_TOKEN="aws_session_token";
const string NgapS3Credentials::AWS_TOKEN_EXPIRATION="aws_token_expiration";
const string NgapS3Credentials::BES_CONF_S3_ENDPOINT_KEY="NGAP.S3.distribution.endpoint.url";
const string NgapS3Credentials::BES_CONF_REFRESH_KEY="NGAP.S3.refresh.margin";
const string NgapS3Credentials::BES_CONF_URL_BASE="NGAP.s3.url.base";


/**
 * This code assumes that the credentials needed to authenticate the retrieval of the S3
 * credentials will be handled by the curl call via the ~/.netrc file of the BES user.
 *
 * @param distribution_api_endpoint The URL of the cumulus distribution api s3credentials endpoint
 */
void NgapS3Credentials::get_temporary_credentials() {

    string accessKeyId, secretAccessKey, sessionToken, expiration;

    BESDEBUG(MODULE,  " distribution_api_endpoint: " << distribution_api_endpoint << endl);

    rapidjson::Document d = curl::http_get_as_json(distribution_api_endpoint);
    BESDEBUG(MODULE, "S3 Credentials:"  << endl);

    rapidjson::Value& val = d[AWS_ACCESS_KEY_ID_KEY];
    accessKeyId = val.GetString();
    add(ID_KEY,accessKeyId);
    BESDEBUG(MODULE, AWS_ACCESS_KEY_ID_KEY << ":        "  << accessKeyId  << endl);

    val = d[AWS_SECRET_ACCESS_KEY_KEY];
    secretAccessKey = val.GetString();
    add(KEY_KEY,secretAccessKey);
    BESDEBUG(MODULE, AWS_SECRET_ACCESS_KEY_KEY << ":    "  << secretAccessKey << endl);

    val = d[AWS_SESSION_TOKEN_KEY];
    sessionToken = val.GetString();
    add(AWS_SESSION_TOKEN,sessionToken);
    BESDEBUG(MODULE, AWS_SESSION_TOKEN_KEY << ":       " << sessionToken  << endl);

    val = d[AWS_EXPIRATION_KEY];
    expiration = val.GetString();
    add(AWS_TOKEN_EXPIRATION,expiration);
    BESDEBUG(MODULE, AWS_EXPIRATION_KEY << ":         "  << expiration  << endl);

    // parse the time string into a something useful -------------------------------------------------------
    struct tm tm;
    // 2020-02-18 13:49:30+00:00
    strptime(expiration.c_str(), "%Y-%m-%d %H:%M:%S%z", &tm);
    d_expiration_time = mktime(&tm);  // t is now your desired time_t
    BESDEBUG(MODULE, "expiration(time_t): "  << d_expiration_time  << endl);

}



