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

// Created by James Gallagher on 3/6/25.

#include "config.h"

#include <string>
#include <sstream>
#include <cstdint>
#include <curl/curl.h>

#include <sys/stat.h>

#include "AWS_SDK.h"
#include "test_config.h"
#include "BESInternalFatalError.h"

#include "modules/common/run_tests_cppunit.h"

using namespace std;

#define prolog std::string("AWS_SDK::").append(__func__).append("() - ")

namespace bes {
class AWS_SDK_Test : public CppUnit::TestFixture {
public:
    // Called once before everything gets tested
    AWS_SDK_Test() = default;

    // Called at the end of the test
    ~AWS_SDK_Test() override = default;

    // Called before each test
    void setUp() override {
        AWS_SDK::aws_library_initialize();
    }

    // Called after each test
    void tearDown() override {
        AWS_SDK::aws_library_shutdown();
    }

    static void get_s3_creds(string &id, string &secret) {
        id = getenv("CMAC_ID") ? getenv("CMAC_ID"): "";
        secret = getenv("CMAC_ACCESS_KEY")  ? getenv("CMAC_ACCESS_KEY"): "";
        CPPUNIT_ASSERT_MESSAGE("Neither the AWS ID nor Secret can be empty for these tests.",
                               !(id.empty() || secret.empty()));
    }

    static long get_file_size(const std::string &filename) {
        struct stat st{};
        if (stat(filename.c_str(), &st) == 0) {
            return st.st_size;
        }
        return -1; // Indicate error (file not found, etc.)
    }

    class file_wrapper {
        string d_filename;
        public:
        explicit file_wrapper(const std::string &filename) : d_filename(filename) {}
        ~file_wrapper() { remove(d_filename.c_str()); }
    };

    static bool is_url_signed_for_s3(const std::string &url) {
        return url.find("X-Amz-Algorithm=") != string::npos &&
            url.find("X-Amz-Credential=") != string::npos &&
            url.find("X-Amz-Signature=") != string::npos;
    }

    static std::string strip_signature_from_url(const std::string &signed_url) {
        string url(signed_url);
        size_t pos = url.find('?');
        if (pos != std::string::npos) {
            url.erase(pos);
        }
        return url;
    }

    /**
     * Helper function for testing pre-signed url behavior
     * Lightly modified from AWS SDK documentation https://docs.aws.amazon.com/sdk-for-cpp/v1/developer-guide/cpp_s3_code_examples.html
     */
    static size_t myCurlWriteBack(char *buffer, size_t size, size_t nitems, void *userdata) {
        Aws::StringStream *str = (Aws::StringStream *) userdata;

        if (nitems > 0) {
            str->write(buffer, size * nitems);
        }
        return size * nitems;
    }

    /**
     * Helper function for testing url object fetching via curl requests
     * Lightly modified from AWS SDK documentation https://docs.aws.amazon.com/sdk-for-cpp/v1/developer-guide/cpp_s3_code_examples.html
     */
    static bool url_request_returns_object(const Aws::String &url) {
        CURL *curl = curl_easy_init();
        CURLcode result;

        std::stringstream outWriteString;

        result = curl_easy_setopt(curl, CURLOPT_WRITEDATA, &outWriteString);

        if (result != CURLE_OK) {
            std::cerr << "Failed to set CURLOPT_WRITEDATA " << std::endl;
            return false;
        }

        result = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, myCurlWriteBack);

        if (result != CURLE_OK) {
            std::cerr << "Failed to set CURLOPT_WRITEFUNCTION" << std::endl;
            return false;
        }

        result = curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

        if (result != CURLE_OK) {
            std::cerr << "Failed to set CURLOPT_URL" << std::endl;
            return false;
        }

        result = curl_easy_perform(curl);

        if (result != CURLE_OK) {
            std::cerr << "Failed to perform CURL request" << std::endl;
            return false;
        }

        auto resultString = outWriteString.str();

        if (resultString.find("<?xml") == 0) {
            std::cerr << "Failed to get object, response:\n" << resultString << std::endl;
            return false;
        }

        return true;
    }

    static void test_throw_if_s3_client_uninitialized() {
        AWS_SDK aws_sdk;
        CPPUNIT_ASSERT_THROW_MESSAGE("s3 client must be initialized",
                                        aws_sdk.s3_head_exists("foo", "bar"), BESInternalFatalError);

    }

    static void test_s3_head_exists_yes() {
        AWS_SDK aws_sdk;
        string id;
        string secret;
        get_s3_creds(id, secret);
        aws_sdk.initialize_s3_client("us-east-1", id, secret);
        const string object = "/samples/chunked_twoD.h5";
        const string bucket = "cloudydap";
        const bool status = aws_sdk.s3_head_exists(bucket, object);
        CPPUNIT_ASSERT_MESSAGE("The object " + object + " should be in " + bucket, status == true);
    }

    static void test_s3_head_exists_no() {
        AWS_SDK aws_sdk;
        string id;
        string secret;
        get_s3_creds(id, secret);
        aws_sdk.initialize_s3_client("us-east-1", id, secret);
        const string object = "/samples/not_here";
        const string bucket = "cloudydap";
        const bool status = aws_sdk.s3_head_exists(bucket, object);
        CPPUNIT_ASSERT_MESSAGE("The object " + object + " should not be in " + bucket, status == false);
        DBG(cerr << "AWS exception message: " << aws_sdk.get_aws_exception_message() << '\n');
        DBG(cerr << "HTTP Status code: " << aws_sdk.get_http_status_code() << '\n');
        CPPUNIT_ASSERT_MESSAGE("The HTTP Response code for object " + object + " in " + bucket + "should be 404",
                               aws_sdk.get_http_status_code() == 404);
    }

    static void test_s3_head_exists_bad_creds() {
        AWS_SDK aws_sdk;
        aws_sdk.initialize_s3_client("us-east-1", "foo", "bar");
        const string object = "/samples/chunked_twoD.h5";
        const string bucket = "cloudydap";
        const bool status = aws_sdk.s3_head_exists(bucket, object);
        CPPUNIT_ASSERT_MESSAGE("The request for object " + object + " fail.", status == false);
        DBG(cerr << "AWS exception message: " << aws_sdk.get_aws_exception_message() << '\n');
        DBG(cerr << "HTTP Status code: " << aws_sdk.get_http_status_code() << '\n');
        CPPUNIT_ASSERT_MESSAGE("The HTTP Response code for object " + object + " in " + bucket + "should be 404",
                               aws_sdk.get_http_status_code() == 403);
    }

    static void test_s3_get_as_string() {
        AWS_SDK aws_sdk;
        string id;
        string secret;
        get_s3_creds(id, secret);
        aws_sdk.initialize_s3_client("us-east-1", id, secret);
        const string object =
                "/C2036877806-POCLOUD/20180101000000-OSISAF-L3C_GHRSST-SSTsubskin-GOES16-ssteqc_goes16_20180101_000000-v02.0-fv01.0.dmrpp";
        const string bucket = "cloudydap";
        const string dmrpp = aws_sdk.s3_get_as_string(bucket, object);
        CPPUNIT_ASSERT_MESSAGE("The object " + object + " should be in " + bucket, !dmrpp.empty());
    }

    static void test_s3_get_as_string_not_there() {
        AWS_SDK aws_sdk;
        string id;
        string secret;
        get_s3_creds(id, secret);
        aws_sdk.initialize_s3_client("us-east-1", id, secret);
        const string object = "/C2036877806-POCLOUD/foobar.baz";
        const string bucket = "cloudydap";
        const string dmrpp = aws_sdk.s3_get_as_string(bucket, object);
        CPPUNIT_ASSERT_MESSAGE("The object " + object + " should be notin " + bucket, dmrpp.empty());
        DBG(cerr << "AWS exception message: " << aws_sdk.get_aws_exception_message() << '\n');
        DBG(cerr << "HTTP Status code: " << aws_sdk.get_http_status_code() << '\n');
        CPPUNIT_ASSERT_MESSAGE("The HTTP Response code for object " + object + " in " + bucket + "should be 404",
                               aws_sdk.get_http_status_code() == 404);
    }

    static void test_s3_get_as_string_bad_creds() {
        AWS_SDK aws_sdk;
        aws_sdk.initialize_s3_client("us-east-1", "foo", "bar");
        const string object = "/C2036877806-POCLOUD/20180101000000-OSISAF-L3C_GHRSST-SSTsubskin-GOES16-ssteqc_goes16_20180101_000000-v02.0-fv01.0.dmrpp";
        const string bucket = "cloudydap";
        const string dmrpp = aws_sdk.s3_get_as_string(bucket, object);
        CPPUNIT_ASSERT_MESSAGE("The object " + object + " should be notin " + bucket, dmrpp.empty());
        DBG(cerr << "AWS exception message: " << aws_sdk.get_aws_exception_message() << '\n');
        DBG(cerr << "HTTP Status code: " << aws_sdk.get_http_status_code() << '\n');
        CPPUNIT_ASSERT_MESSAGE("The HTTP Response code for object " + object + " in " + bucket + "should be 403",
                               aws_sdk.get_http_status_code() == 403);
    }

    static void test_s3_get_as_file() {
        AWS_SDK aws_sdk;
        string id;
        string secret;
        get_s3_creds(id, secret);
        aws_sdk.initialize_s3_client("us-east-1", id, secret);
        const string object =
                "/C2036877806-POCLOUD/20180101000000-OSISAF-L3C_GHRSST-SSTsubskin-GOES16-ssteqc_goes16_20180101_000000-v02.0-fv01.0.dmrpp";
        const string bucket = "cloudydap";
        const string filename = string(TEST_BUILD_DIR) + "/temp_response_dmrpp";
        const bool status = aws_sdk.s3_get_as_file(bucket, object, filename);
        CPPUNIT_ASSERT_MESSAGE("The object " + object + " should be in " + bucket, status == true);
        const long file_size = get_file_size(filename);
        file_wrapper dmrpp_file(filename);  // remove the file on exit.
        DBG(cerr << "Response file size: " << file_size << '\n');
        CPPUNIT_ASSERT_MESSAGE("The HTTP Response code for object " + object + " in " + bucket + "should be 55585",
                               file_size == 55585);
    }

    static void test_s3_generate_presigned_object_url() {
        AWS_SDK aws_sdk;
        string id;
        string secret;
        get_s3_creds(id, secret);
        aws_sdk.initialize_s3_client("us-east-1", id, secret);
        const string object = "/samples/chunked_twoD.h5";
        const string bucket = "cloudydap";
        const uint64_t expiration_seconds = 60;
        const Aws::String url = aws_sdk.s3_generate_presigned_object_url(bucket, object, expiration_seconds);
        CPPUNIT_ASSERT_MESSAGE("The url should be signed for s3: " + url, is_url_signed_for_s3(url));

        CPPUNIT_ASSERT_MESSAGE("Presigned url should return an object " + url, url_request_returns_object(url));

        std::string unsigned_url = strip_signature_from_url(url);
        CPPUNIT_ASSERT_MESSAGE("The url should not be signed for s3: " + unsigned_url, !is_url_signed_for_s3(unsigned_url));

        CPPUNIT_ASSERT_MESSAGE("Unsigned url should not return an object " + unsigned_url, !url_request_returns_object(unsigned_url));
    }

    static void test_s3_generate_presigned_object_url_not_there() {
        AWS_SDK aws_sdk;
        string id;
        string secret;
        get_s3_creds(id, secret);
        aws_sdk.initialize_s3_client("us-east-1", id, secret);
        const string object = "/foo";
        const string bucket = "cloudydap";
        const uint64_t expiration_seconds = 60;
        const Aws::String url = aws_sdk.s3_generate_presigned_object_url(bucket, object, expiration_seconds);
        CPPUNIT_ASSERT_MESSAGE("The url should be signed for s3 even though the object doesn't exist: " + url, is_url_signed_for_s3(url));

        CPPUNIT_ASSERT_MESSAGE("No object returned for signed url when object does not exist " + url, !url_request_returns_object(url));
    }

    static void test_s3_generate_presigned_object_url_bad_creds() {
        AWS_SDK aws_sdk;
        aws_sdk.initialize_s3_client("us-east-1", "foo", "bar");
        const string object = "/samples/chunked_twoD.h5";
        const string bucket = "cloudydap";
        const uint64_t expiration_seconds = 60;
        const Aws::String url = aws_sdk.s3_generate_presigned_object_url(bucket, object, expiration_seconds);
        CPPUNIT_ASSERT_MESSAGE("The url should be signed for s3 even though the credentials are bad: " + url, is_url_signed_for_s3(url));

        CPPUNIT_ASSERT_MESSAGE("No object returned for signed url when credentials are bad " + url, !url_request_returns_object(url));
    }

    static void test_s3_generate_presigned_object_url_expiration() {
        AWS_SDK aws_sdk;
        string id;
        string secret;
        get_s3_creds(id, secret);
        aws_sdk.initialize_s3_client("us-east-1", id, secret);
        const string object = "/samples/chunked_twoD.h5";
        const string bucket = "cloudydap";
        const uint64_t expiration_seconds = 3;
        const Aws::String url = aws_sdk.s3_generate_presigned_object_url(bucket, object, expiration_seconds);
        CPPUNIT_ASSERT_MESSAGE("Presigned url should return an object " + url, url_request_returns_object(url));

        sleep(expiration_seconds); // Bad form to add extra time to unit tests, but we need to know that we _can_ expire a signed url, and we need enough time for the response to have been returned for the first fetch

        CPPUNIT_ASSERT_MESSAGE("Presigned url should not return an object if it has expired" + url, !url_request_returns_object(url));
    }

    CPPUNIT_TEST_SUITE(AWS_SDK_Test);

        CPPUNIT_TEST(test_throw_if_s3_client_uninitialized);

        CPPUNIT_TEST(test_s3_get_as_string);
        CPPUNIT_TEST(test_s3_get_as_string_not_there);
        CPPUNIT_TEST(test_s3_get_as_string_bad_creds);
        CPPUNIT_TEST(test_s3_get_as_file);

        CPPUNIT_TEST(test_s3_head_exists_yes);
        CPPUNIT_TEST(test_s3_head_exists_no);
        CPPUNIT_TEST(test_s3_head_exists_bad_creds);

        CPPUNIT_TEST(test_s3_generate_presigned_object_url);
        CPPUNIT_TEST(test_s3_generate_presigned_object_url_not_there);
        CPPUNIT_TEST(test_s3_generate_presigned_object_url_bad_creds);
        CPPUNIT_TEST(test_s3_generate_presigned_object_url_expiration);

    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(AWS_SDK_Test);
} // namespace http

int main(int argc, char *argv[]) {
    return bes_run_tests<bes::AWS_SDK_Test>(argc, argv, "cerr,bes,http") ? 0 : 1;
}
