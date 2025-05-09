// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of libdap, A C++ implementation of the OPeNDAP Data
// Access Protocol.

// Copyright (c) 2018 OPeNDAP, Inc.
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

#include "config.h"

#include <cstring>

#include "modules/common/run_tests_cppunit.h"

#include "awsv4.h"
#include "test_config.h"

#define USE_MKTIME 0

using namespace std;

namespace http {

class awsv4_test : public CppUnit::TestFixture {
private:
    string cm_config;
    time_t request_time;
    string aws_key_id;
    string aws_secret_key;
    string region;
    string serviceName;

    string fileToString(const string &fn) {
        ifstream is;
        is.open(fn.c_str(), ios::binary);
        if (!is || is.eof()) return "";

        // get length of file:
        is.seekg(0, ios::end);
        int length = is.tellg();

        // back to start
        is.seekg(0, ios::beg);

        // allocate memory:
        vector<char> buffer(length + 1);

        // read data as a block:
        is.read(buffer.data(), length);
        is.close();
        buffer[length] = '\0';

        return string(buffer.data());
    }

public:
    // Called once before everything gets tested
    awsv4_test() = default;

    // Called at the end of the test
    ~awsv4_test() override = default;

    // Called before each test
    void setUp() override {
        if (debug) cerr << endl;

        // Get the time value
        // AWSv4 examples are based on a request dat/time of:
        // define REQUEST_DATE "2015 08 30 T 12 36 00Z"
#if USE_MKTIME
        // Set timezone to GMT0
        setenv("TZ", "GMT0", true);

        // Populate time struct
        // Failure to initialize this struct _may_ cause mktime(3) to fail on linux. jhrg 1/19/24
        struct tm t_info = {};
        t_info.tm_year = 115;  // Years since 1900
        t_info.tm_mon = 7;    // August
        t_info.tm_mday = 30;
        t_info.tm_hour = 12;   // 1200 GMT
        t_info.tm_min = 36;
        t_info.tm_sec = 0;

        request_time = mktime(&t_info);
        if (request_time < 0) cerr << "mktime(3) Error: " << strerror(errno) << endl;
#else
        request_time = 1440938160;   // This is "2015 08 30 T 12 36 00Z". jhrg 1/19/24
#endif
        if (debug) cerr << "request_time: " << request_time << endl;
        aws_key_id = "AKIDEXAMPLE";
        aws_secret_key = "wJalrXUtnFEMI/K7MDENG+bPxRfiCYEXAMPLEKEY";

        region = "us-east-1";
        serviceName = "service";
    }

    void show_baseline(const string &name, const string &value) {
        cerr << "# BEGIN " << name << " -------------------------------------------------" << endl;
        cerr << value << endl;
        cerr << "# END " << name << " -------------------------------------------------" << endl << endl;
    }

    void load_test_baselines(const string &test_name, string &auth_header_baseline) {
        string test_file_base = string(TEST_SRC_DIR).append("/awsv4/")
                .append(test_name).append("/")
                .append(test_name);

        auth_header_baseline = fileToString(test_file_base + ".authz");
        if (debug) show_baseline("auth_header_baseline", auth_header_baseline);
    }

    void run_test(const string &test_name, const string &request_uri_str) {
        string auth_header_baseline;
        load_test_baselines(test_name, auth_header_baseline);

        auto request_uri = make_shared<http::url>(request_uri_str);
        string auth_header = AWSV4::compute_awsv4_signature(request_uri, request_time, aws_key_id,
                                                            aws_secret_key, region, serviceName);

        if (debug) show_baseline("auth_header", auth_header);

        CPPUNIT_ASSERT(auth_header == auth_header_baseline);
    }

    void get_unreserved() {
        string request_uri = "https://example.amazonaws.com/-._~0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
        if (debug) cerr << "get_unreserved() - request_uri: " << request_uri << endl;
        run_test("get-unreserved", request_uri);
    }

    void get_utf8() {
        string request_uri = "https://example.amazonaws.com/ሴ";
        if (debug) cerr << "get_utf8() - request_uri: " << request_uri << endl;
        run_test("get-utf8", request_uri);
    }

    void get_vanilla() {
        string request_uri = "https://example.amazonaws.com/";
        if (debug) cerr << "get_vanilla() - request_uri: " << request_uri << endl;
        run_test("get-vanilla", request_uri);
    }

    void get_vanilla_empty_query_key() {
        string request_uri = "https://example.amazonaws.com/?Param1=value1";
        if (debug) cerr << "get_vanilla_empty_query_key() - request_uri: " << request_uri << endl;
        run_test("get-vanilla-empty-query-key", request_uri);
    }

    void get_vanilla_query() {
        string request_uri = "https://example.amazonaws.com/";
        if (debug) cerr << "get_vanilla_query() - request_uri: " << request_uri << endl;
        run_test("get-vanilla-query", request_uri);
    }

    void get_vanilla_query_order_key() {
        string request_uri = "https://example.amazonaws.com/?Param1=value2&Param1=Value1";
        if (debug) cerr << "get_vanilla_query_order_key() - request_uri: " << request_uri << endl;
        run_test("get-vanilla-query-order-key", request_uri);
    }

    void get_vanilla_query_order_key_case() {
        string request_uri = "https://example.amazonaws.com/?Param2=value2&Param1=value1";
        if (debug) cerr << "get_vanilla_query_order_key_case() - request_uri: " << request_uri << endl;
        run_test("get-vanilla-query-order-key-case", request_uri);
    }


    void get_vanilla_query_order_value() {
        string request_uri = "https://example.amazonaws.com/?Param1=value2&Param1=value1";
        if (debug) cerr << "get_vanilla_query_order_value() - request_uri: " << request_uri << endl;
        run_test("get-vanilla-query-order-value", request_uri);
    }


    void get_vanilla_query_unreserved() {
        string request_uri = "https://example.amazonaws.com/?-._~0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                             "abcdefghijklmnopqrstuvwxyz=-._~0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                             "abcdefghijklmnopqrstuvwxyz";

        if (debug) cerr << "get_vanilla_query_unreserved() - request_uri: " << request_uri << endl;
        run_test("get-vanilla-query-unreserved", request_uri);
    }

    void get_vanilla_utf8_query() {
        string request_uri = "https://example.amazonaws.com/?ሴ=bar";
        if (debug) cerr << "get_vanilla_utf8_query() - request_uri: " << request_uri << endl;
        run_test("get-vanilla-utf8-query", request_uri);
    }

    void join_test() {
        vector<string> in = {"a", "b", "c"};
        CPPUNIT_ASSERT(AWSV4::join(in, ":") == "a:b:c");

        vector<string> in2 = {"a"};
        CPPUNIT_ASSERT(AWSV4::join(in2, ":") == "a");

        vector<string> in3 = {};
        CPPUNIT_ASSERT(AWSV4::join(in3, ":") == "");

        CPPUNIT_ASSERT(AWSV4::join(in, "") == "abc");
    }

    CPPUNIT_TEST_SUITE(awsv4_test);

    CPPUNIT_TEST(join_test);

    CPPUNIT_TEST(get_unreserved);

#if 0
    CPPUNIT_TEST(get_utf8); // UTF characters are not correctly escaped in canonical request
#endif

    CPPUNIT_TEST(get_vanilla);
    CPPUNIT_TEST(get_vanilla_empty_query_key);
    CPPUNIT_TEST(get_vanilla_query);

#if 0
    CPPUNIT_TEST(get_vanilla_query_order_key); // Order of our parameters is not modifed based on key string
    CPPUNIT_TEST(get_vanilla_query_order_key_case); // Order of our parameters is not modifed based on key case
    CPPUNIT_TEST(get_vanilla_query_order_value); // Order of our parameters is not modifed based on key value
    CPPUNIT_TEST(get_vanilla_utf8_query); // UTF characters are not correctly escaped in canonical request
#endif

    CPPUNIT_TEST(get_vanilla_query_unreserved);

    CPPUNIT_TEST_SUITE_END();

};

CPPUNIT_TEST_SUITE_REGISTRATION(awsv4_test);

} // namespace http

int main(int argc, char *argv[]) {
    return bes_run_tests<http::awsv4_test>(argc, argv, "dmrpp,dmrpp:creds") ? 0 : 1;
}
