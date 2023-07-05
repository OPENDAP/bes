// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of the Hyrax data server.

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

#include <string>
#include <sstream>
#include <vector>

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/error/en.h"

#include "BESError.h"
#include "BESSyntaxUserError.h"
#include "BESDebug.h"

#include "CurlUtils.h"

#include "NgapS3Credentials.h"
#include "HttpNames.h"

using namespace std;

#define AWS_ACCESS_KEY_ID_KEY "accessKeyId"
#define AWS_SECRET_ACCESS_KEY_KEY "secretAccessKey"
#define AWS_SESSION_TOKEN_KEY "sessionToken"
#define AWS_EXPIRATION_KEY "expiration"

#define prolog std::string("NgapS3Credentials::").append(__func__).append("() - ")

namespace http {

// Scope: public members of AccessCredentials
const char *NgapS3Credentials::AWS_SESSION_TOKEN = "aws_session_token";
const char *NgapS3Credentials::AWS_TOKEN_EXPIRATION_KEY = "aws_token_expiration";
const char *NgapS3Credentials::BES_CONF_S3_ENDPOINT_KEY = "NGAP.S3.distribution.endpoint.url";
const char *NgapS3Credentials::BES_CONF_REFRESH_KEY = "NGAP.S3.refresh.margin";
const char *NgapS3Credentials::BES_CONF_URL_BASE_KEY = "NGAP.s3.url.base";

string NgapS3Credentials::get(const std::string &key) {
    if (needs_refresh()) {
        get_temporary_credentials();
    }
    return AccessCredentials::get(key);
}

/**
 * This code assumes that the credentials needed to authenticate the retrieval of the S3
 * credentials will be handled by the curl call via the ~/.netrc file of the BES user.
 *
 * @param distribution_api_endpoint The URL of the cumulus distribution api s3credentials endpoint
 */
void NgapS3Credentials::get_temporary_credentials() {

    string accessKeyId, secretAccessKey, sessionToken, expiration;

    BESDEBUG(HTTP_MODULE, prolog << "distribution_api_endpoint: " << distribution_api_endpoint << endl);

    vector<char> buf;
    curl::http_get(distribution_api_endpoint, buf);

    rapidjson::Document d;
    d.Parse(buf.data());
    if (d.HasParseError()) {
        ostringstream oss;
        oss << "Parse error: " << rapidjson::GetParseError_En(d.GetParseError()) << " at " << d.GetErrorOffset();
        BESDEBUG(HTTP_MODULE, prolog << oss.str() << endl);
        throw BESSyntaxUserError(oss.str(), __FILE__, __LINE__);
    }

    // rapidjson::Document d = curl::http_get_as_json(distribution_api_endpoint, buf);
    BESDEBUG(HTTP_MODULE, prolog << "S3 Credentials:" << endl);

    rapidjson::Value &val = d[AWS_ACCESS_KEY_ID_KEY];
    accessKeyId = val.GetString();
    add(ID_KEY, accessKeyId);
    BESDEBUG(HTTP_MODULE, prolog << AWS_ACCESS_KEY_ID_KEY << ":        " << accessKeyId << endl);

    val = d[AWS_SECRET_ACCESS_KEY_KEY];
    secretAccessKey = val.GetString();
    add(KEY_KEY, secretAccessKey);
    BESDEBUG(HTTP_MODULE, prolog << AWS_SECRET_ACCESS_KEY_KEY << ":    " << secretAccessKey << endl);

    val = d[AWS_SESSION_TOKEN_KEY];
    sessionToken = val.GetString();
    add(AWS_SESSION_TOKEN, sessionToken);
    BESDEBUG(HTTP_MODULE, prolog << AWS_SESSION_TOKEN_KEY << ":       " << sessionToken << endl);

    val = d[AWS_EXPIRATION_KEY];
    expiration = val.GetString();
    add(AWS_TOKEN_EXPIRATION_KEY, expiration);
    BESDEBUG(HTTP_MODULE, prolog << AWS_EXPIRATION_KEY << ":         " << expiration << endl);

    // parse the time string into a something useful -------------------------------------------------------
    struct tm tm{};
    // 2020-02-18 13:49:30+00:00
    strptime(expiration.c_str(), "%Y-%m-%d %H:%M:%S%z", &tm);
    d_expiration_time = mktime(&tm);  // t is now your desired time_t
    BESDEBUG(HTTP_MODULE, prolog << "expiration(time_t): " << d_expiration_time << endl);
}

#if 0
// How this might be used:

/**
 * Private method. Must be called inside code protected by a mutex.
 *
 * Read the BESKeys (from bes.conf chain) and if NgapS3Credentials::BES_CONF_S3_ENDPOINT_KEY is present builds
 * and adds to the CredentialsManager an instance of NgapS3Credentials based on the values found in the bes.conf chain.
 */
void CredentialsManager::load_ngap_s3_credentials() const {
    // This lock is a RAII implementation. It will block until the mutex is
    // available and the lock will be released when the instance is destroyed.
    // std::lock_guard<std::recursive_mutex> lock_me(d_lock_mutex);

    string s3_distribution_endpoint_url;
    bool found;
    TheBESKeys::TheKeys()->get_value(NgapS3Credentials::BES_CONF_S3_ENDPOINT_KEY, s3_distribution_endpoint_url, found);
    if (found) {
        string value;

        long refresh_margin = 600;
        TheBESKeys::TheKeys()->get_value(NgapS3Credentials::BES_CONF_REFRESH_KEY, value, found);
        if (found) {
            refresh_margin = strtol(value.c_str(), nullptr, 10);
        }

        string s3_base_url = NGAP_S3_BASE_DEFAULT;
        TheBESKeys::TheKeys()->get_value(NgapS3Credentials::BES_CONF_URL_BASE_KEY, value, found);
        if (found) {
            s3_base_url = value;
        }

        auto nsc = std::make_unique<NgapS3Credentials>(s3_distribution_endpoint_url, refresh_margin);
        nsc->add(AccessCredentials::URL_KEY, s3_base_url);
        nsc->name("NgapS3Credentials");

        CredentialsManager::theCM()->add(s3_base_url, nsc.release());
        CredentialsManager::theCM()->ngaps3CredentialsLoaded = true;
    }
    else {
        BESDEBUG(HTTP_MODULE, prolog << "WARNING: The BES configuration did not contain an instance of "
                                     << NgapS3Credentials::BES_CONF_S3_ENDPOINT_KEY
                                     << " NGAP S3 Credentials NOT loaded." << endl);
    }
}
#endif

} //namespace http
