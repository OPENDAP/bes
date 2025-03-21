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

#include <sys/stat.h>

#include "AWS_SDK.h"
#include "test_config.h"

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

    static void test_s3_head_yes() {
        AWS_SDK aws_sdk;
        string id;
        string secret;
        get_s3_creds(id, secret);
        aws_sdk.initialize("us-east-1", id, secret);
        const string object = "/samples/chunked_twoD.h5";
        const string bucket = "cloudydap";
        const bool status = aws_sdk.s3_head(bucket, object);
        CPPUNIT_ASSERT_MESSAGE("The object " + object + " should be in " + bucket, status == true);
    }

    static void test_s3_head_no() {
        AWS_SDK aws_sdk;
        string id;
        string secret;
        get_s3_creds(id, secret);
        aws_sdk.initialize("us-east-1", id, secret);
        const string object = "/samples/not_here";
        const string bucket = "cloudydap";
        const bool status = aws_sdk.s3_head(bucket, object);
        CPPUNIT_ASSERT_MESSAGE("The object " + object + " should not be in " + bucket, status == false);
        DBG(cerr << "AWS exception message: " << aws_sdk.get_aws_exception_message() << '\n');
        DBG(cerr << "HTTP Status code: " << aws_sdk.get_http_status_code() << '\n');
        CPPUNIT_ASSERT_MESSAGE("The HTTP Response code for object " + object + " in " + bucket + "should be 404",
                               aws_sdk.get_http_status_code() == 404);
    }

    static void test_s3_head_bad_creds() {
        AWS_SDK aws_sdk;
        aws_sdk.initialize("us-east-1", "foo", "bar");
        const string object = "/samples/chunked_twoD.h5";
        const string bucket = "cloudydap";
        const bool status = aws_sdk.s3_head(bucket, object);
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
        aws_sdk.initialize("us-east-1", id, secret);
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
        aws_sdk.initialize("us-east-1", id, secret);
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
        aws_sdk.initialize("us-east-1", "foo", "bar");
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
        aws_sdk.initialize("us-east-1", id, secret);
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

    CPPUNIT_TEST_SUITE(AWS_SDK_Test);

        CPPUNIT_TEST(test_s3_get_as_string);
        CPPUNIT_TEST(test_s3_get_as_string_not_there);
        CPPUNIT_TEST(test_s3_get_as_string_bad_creds);
        CPPUNIT_TEST(test_s3_get_as_file);

        CPPUNIT_TEST(test_s3_head_yes);
        CPPUNIT_TEST(test_s3_head_no);
        CPPUNIT_TEST(test_s3_head_bad_creds);

    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(AWS_SDK_Test);
} // namespace http

int main(int argc, char *argv[]) {
    return bes_run_tests<bes::AWS_SDK_Test>(argc, argv, "cerr,bes,http") ? 0 : 1;
}
