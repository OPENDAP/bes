// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of the OPeNDAP Back-End Server (BES)
// and creates an allowed hosts list of which systems that may be
// accessed by the server as part of its routine operation.

// Copyright (c) 2025 OPeNDAP, Inc.
// Author: James Gallagher <jgallagher@opendap.org>
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
//

// Created by James Gallagher on 3/4/25.

#include "config.h"

#include <string>
#include <exception>

#include <aws/core/Aws.h>
#include <aws/core/auth/AWSCredentialsProvider.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/GetObjectRequest.h>
#include <aws/s3/model/HeadObjectRequest.h>
#include <fstream>
#include <iostream>

#include "AWS_SDK.h"

//#include "BESInternalFatalError.h"

using namespace std;

namespace bes {

Aws::SDKOptions AWS_SDK::options;

#if 0

/**
 * @brief Get an S3 Client.
 * @param region AWS region string, e.g., 'us-east-1'
 * @return The AWS S3 Client object.
 */
Aws::S3::S3Client AWS_SDK::get_s3_client(const string &region) {
    Aws::S3::S3ClientConfiguration clientConfig;
    clientConfig.region = region; // Set your region

    // Create a shared pointer to a SimpleAWSCredentialsProvider using your key and secret.
    auto credentialsProvider = Aws::Auth::AWSCredentials(getenv("CMAC_ID"), getenv("CMAC_ACCESS_KEY"));

    // Construct the S3 client with the credentials provider and client configuration.
    // std::shared_ptr<S3EndpointProviderBase> is nullptr in the following call.
    return {credentialsProvider, nullptr, clientConfig};
}

#endif

/**
 * @brief Get an S3 Client.
 * @param region AWS region string.
 * @param aws_key The AWS Key ID
 * @param aws_secret_key The AWS Secret Key
 * @return The AWS S3 Client object
 */
Aws::S3::S3Client AWS_SDK::get_s3_client(const string &region, const string &aws_key, const string &aws_secret_key) {
    Aws::S3::S3ClientConfiguration clientConfig;
    clientConfig.region = region; // Set your region

    // Create a shared pointer to a SimpleAWSCredentialsProvider using your key and secret.
    auto credentialsProvider = Aws::Auth::AWSCredentials(aws_key, aws_secret_key);

    // Construct the S3 client with the credentials provider and client configuration.
    // std::shared_ptr<S3EndpointProviderBase> is nullptr in the following call.
    return {credentialsProvider, nullptr, clientConfig};
}

void AWS_SDK::ok() const {
    if (!d_is_s3_initialized) {
        // throw BESInternalFatalError("AWS_SDK object not initialized.", __FILE__, __LINE__);
        throw std::runtime_error("AWS_SDK::ok() called before initialization");
    }
}

/**
 *
 * @param bucket Name of the S3 bucket
 * @param key Object key in the bucket
 * @return True if the object exists and can be accessed, false otherwise
 */
bool AWS_SDK::s3_head(const string &bucket, const string &key) {
    ok();

    Aws::S3::Model::HeadObjectRequest head_request;
    head_request.SetBucket(bucket);
    head_request.SetKey(key);

    const auto head_outcome = d_get_s3_client.HeadObject(head_request);
    if (head_outcome.IsSuccess()) {
        return true;
    }
    const auto &error = head_outcome.GetError();
    const auto http_code = error.GetResponseCode(); // Aws::Http::HttpResponseCode is an enum. See cast below.
    d_aws_exception_message = error.GetMessage();
    d_aws_exception_name = error.GetExceptionName();
    d_http_status_code = static_cast<int>(http_code);

    return false;
}

/**
 *
 * @param bucket Name of the S3 bucket
 * @param key Object key in the bucket
 * @return Received data as a string or the empty string
 */
string AWS_SDK::s3_get_as_string(const string &bucket, const string &key) {
    ok();

    Aws::S3::Model::GetObjectRequest object_request;
    object_request.SetBucket(bucket);
    object_request.SetKey(key);

    auto get_object_outcome = d_get_s3_client.GetObject(object_request);
    if (get_object_outcome.IsSuccess()) {
        const auto &retrieved_file = get_object_outcome.GetResultWithOwnership().GetBody();
        stringstream file_contents;
        file_contents << retrieved_file.rdbuf();
        return {file_contents.str()};
    }
    const auto error = get_object_outcome.GetError();
    const auto httpCode = error.GetResponseCode(); // Aws::Http::HttpResponseCode
    d_aws_exception_message = error.GetMessage();
    d_aws_exception_name = error.GetExceptionName();
    d_http_status_code = static_cast<int>(httpCode);

    return {""};
}

/**
 *
 * @param bucket Name of the S3 bucket
 * @param key Object key in the bucket
 * @param filename Local file/path name for the received data
 * @return True if successful, false otherwise
 */
bool AWS_SDK::s3_get_as_file(const string &bucket, const string &key, const string &filename) {
    ok();

    Aws::S3::Model::GetObjectRequest object_request;
    object_request.SetBucket(bucket);
    object_request.SetKey(key);

    auto get_object_outcome = d_get_s3_client.GetObject(object_request);
    if (get_object_outcome.IsSuccess()) {
        const auto &retrieved_file = get_object_outcome.GetResultWithOwnership().GetBody();
        std::ofstream output_file(filename, std::ios::binary);
        output_file << retrieved_file.rdbuf();
        return true;
    }
    const auto error = get_object_outcome.GetError();
    const auto httpCode = error.GetResponseCode(); // Aws::Http::HttpResponseCode
    d_aws_exception_message = error.GetMessage();
    d_aws_exception_name = error.GetExceptionName();
    d_http_status_code = static_cast<int>(httpCode);

    return false;
}
}