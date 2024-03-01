// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of the BES component of the Hyrax Data Server.

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

#include "config.h"

#include <memory>
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include <cstring>
#include <iostream>

#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>

#include <unistd.h>

#include "BESError.h"
#include "BESDebug.h"
#include "BESUtil.h"
#include "TheBESKeys.h"
#include "HttpUtils.h"
#include "HttpNames.h"
#include "url_impl.h"

#include "test_config.h"

using namespace std;
using namespace rapidjson;

static bool debug = false;
static bool Debug = false;
static bool bes_debug = false;

#undef DBG
#define DBG(x) do { if (debug) x; } while(false)

namespace http {

    class HttpUtilsTest: public CppUnit::TestFixture {
    private:

        // char curl_error_buf[CURL_ERROR_SIZE];

        void show_file(string filename)
        {
            ifstream t(filename.c_str());

            if (t.is_open()) {
                string file_content((istreambuf_iterator<char>(t)), istreambuf_iterator<char>());
                t.close();
                cout << endl << "##################################################################" << endl;
                cout << "file: " << filename << endl;
                cout << ". . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . " << endl;
                cout << file_content << endl;
                cout << ". . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . " << endl;
            }
        }

    public:
        // Called once before everything gets tested
        HttpUtilsTest()
        {
        }

        // Called at the end of the test
        ~HttpUtilsTest()
        {
        }

        // Called before each test
        void setUp()
        {
            if(Debug) cerr << "setUp() - BEGIN" << endl;
            string bes_conf = BESUtil::assemblePath(TEST_BUILD_DIR,"bes.conf");
            if(Debug) cerr << "setUp() - Using BES configuration: " << bes_conf << endl;

            TheBESKeys::ConfigFile = bes_conf;

            if (bes_debug) BESDebug::SetUp("cerr,ngap,http");

            if (bes_debug) show_file(bes_conf);
            if(Debug) cerr << "setUp() - END" << endl;
        }

        // Called after each test
        void tearDown()
        {
        }

        void show_vector(vector<string> v){
            cerr << "show_vector(): Found " << v.size() << " elements." << endl;
            for(size_t i=0;  i < v.size(); i++){
                cerr << "show_vector:    v["<< i << "]: " << v[i] << endl;
            }
        }
        void tokenize_test() {
            string s1 = "//foo/bar/baz.nc";
            vector<string> tokens;
            BESUtil::tokenize(s1,tokens);
            if(debug){
                show_vector(tokens);
            }

            tokens.clear();
            string s2 = "granules.umm_json_v1_4?provider=GHRC_CLOUD&entry_title=ADVANCED%20MICROWAVE%20SOUNDING%20UNIT-A%20%28AMSU-A%29%20SWATH%20FROM%20NOAA-15%20V1&granule_ur=amsua15_2020.028_12915_1139_1324_WI.nc";
            BESUtil::tokenize(s2,tokens);
            if(debug){
                show_vector(tokens);
            }


        }




        void decompose_aws_signed_request_url_test(){
            string prolog = string(__func__) + "() - ";

            std::map<std::string,std::string> url_info;
            std::map<std::string,std::string>::iterator it;

            if(debug ) cout << endl;

            string url;
            string key;
            string expected_value;
            string value;

            url = "https://ghrcw-protected.s3.us-west-2.amazonaws.com/rss_demo/rssmif16d__7/f16_ssmis_20200512v7.nc?"
                  "A-userid=hyrax"
                  "&X-Amz-Algorithm=AWS4-HMAC-SHA256"
                  "&X-Amz-Credential=SomeBigMessyAwfulEncodedEscapeBunchOfCryptoPhaffing"
                  "&X-Amz-Date=20200621T161744Z"
                  "&X-Amz-Expires=86400"
                  "&X-Amz-Security-Token=FwoGZXIvYXdzENL%2F%2F%2F%2F%2F%2F%2F%2F%2F%2FwEaDKmu"
                  "SomeBigMessyAwfulEncodedEscapeBunchOfCryptoPhaffingSomeBigMessyAwfulEncodedEscapeBunchOfCryptoPhaffing"
                  "SomeBigMessyAwfulEncodedEscapeBunchOfCryptoPhaffingSomeBigMessyAwfulEncodedEscapeBunchOfCryptoPhaffing"
                  "SomeBigMessyAwfulEncodedEscapeBunchOfCryptoPhaffingSomeBigMessyAwfulEncodedEscapeBunchOfCryptoPhaffing"
                  "SomeBigMessyAwfulEncodedEscapeBunchOfCryptoPhaffingSomeBigMessyAwfulEncodedEscapeBunchOfCryptoPhaffing"
                  "&X-Amz-SignedHeaders=host"
                  "&X-Amz-Signature=SomeBigMessyAwfulEncodedEscapeBunchOfCryptoPhaffing";

            if (debug) cerr << prolog << "Processing URL: " << url << endl;
            http::url target_url(url);

            value = target_url.protocol();
            expected_value = HTTPS_PROTOCOL;
            if(debug) cerr << prolog << "target_url.protocol(): " << value << " expected: " << expected_value << endl;
            CPPUNIT_ASSERT( value == expected_value );

            value = target_url.host();
            expected_value = "ghrcw-protected.s3.us-west-2.amazonaws.com";
            if(debug) cerr << prolog << "target_url.host(): " << value << " expected: " << expected_value << endl;
            CPPUNIT_ASSERT( value == expected_value );

            value = target_url.path();
            expected_value = "/rss_demo/rssmif16d__7/f16_ssmis_20200512v7.nc";
            if(debug) cerr << prolog << "target_url.path(): " << value << " expected: " << expected_value << endl;
            CPPUNIT_ASSERT( value == expected_value );

            key = "A-userid";
            expected_value = "hyrax";
            value = target_url.query_parameter_value(key);
            if(debug) cerr << prolog << "key: " << key << " value: " << value << " expected: " << expected_value << endl;
            CPPUNIT_ASSERT( value == expected_value );

            key = "X-Amz-Algorithm";
            expected_value = "AWS4-HMAC-SHA256";
            value = target_url.query_parameter_value(key);
            if(debug) cerr << prolog << "key: " << key << " value: " << value << " expected: " << expected_value << endl;
            CPPUNIT_ASSERT( value == expected_value );

            key = "X-Amz-Credential";
            expected_value = "SomeBigMessyAwfulEncodedEscapeBunchOfCryptoPhaffing";
            value = target_url.query_parameter_value(key);
            if(debug) cerr << prolog << "key: " << key << " value: " << value << " expected: " << expected_value << endl;
            CPPUNIT_ASSERT( value == expected_value );

            key = "X-Amz-Date";
            expected_value = "20200621T161744Z";
            value = target_url.query_parameter_value(key);
            if(debug) cerr << prolog << "key: " << key << " value: " << value << " expected: " << expected_value << endl;
            CPPUNIT_ASSERT( value == expected_value );

            key = "X-Amz-Expires";
            expected_value = "86400";
            value = target_url.query_parameter_value(key);
            if(debug) cerr << prolog << "key: " << key << " value: " << value << " expected: " << expected_value << endl;
            CPPUNIT_ASSERT( value == expected_value );

            key = "X-Amz-Security-Token";
            expected_value = "FwoGZXIvYXdzENL%2F%2F%2F%2F%2F%2F%2F%2F%2F%2FwEaDKmu"
                             "SomeBigMessyAwfulEncodedEscapeBunchOfCryptoPhaffingSomeBigMessyAwfulEncodedEscapeBunchOfCryptoPhaffing"
                             "SomeBigMessyAwfulEncodedEscapeBunchOfCryptoPhaffingSomeBigMessyAwfulEncodedEscapeBunchOfCryptoPhaffing"
                             "SomeBigMessyAwfulEncodedEscapeBunchOfCryptoPhaffingSomeBigMessyAwfulEncodedEscapeBunchOfCryptoPhaffing"
                             "SomeBigMessyAwfulEncodedEscapeBunchOfCryptoPhaffingSomeBigMessyAwfulEncodedEscapeBunchOfCryptoPhaffing";
            value = target_url.query_parameter_value(key);
            if(debug) cerr << prolog << "key: " << key << " value: " << value << " expected: " << expected_value << endl;
            CPPUNIT_ASSERT( value == expected_value );

            key = "X-Amz-SignedHeaders";
            expected_value = "host";
            value = target_url.query_parameter_value(key);
            if(debug) cerr << prolog << "key: " << key << " value: " << value << " expected: " << expected_value << endl;
            CPPUNIT_ASSERT( value == expected_value );

            key = "X-Amz-Signature";
            expected_value = "SomeBigMessyAwfulEncodedEscapeBunchOfCryptoPhaffing";
            value = target_url.query_parameter_value(key);
            if(debug) cerr << prolog << "key: " << key << " value: " << value << " expected: " << expected_value << endl;
            CPPUNIT_ASSERT( value == expected_value );

        }


        void decompose_simple_url_test(){
            string prolog = string(__func__) + "() - ";
            std::map<std::string,std::string> url_info;
            std::map<std::string,std::string>::iterator it;

            if(debug ) cout << endl;

            string url;
            string key;
            string expected_value;
            string value;

            url = "https://d1sd4up8kynpk2.cloudfront.net/s3-2dbad80ed80161e4b685a0385c322d93/rss_demo/rssmif16d__7/f16_ssmis_20200512v7.nc?"
                  "RequestId=yU6NwaRaSZBwQ0xexo5Ufv7aL0MeANMMM7oeB96NfuJzrfjVNmW9eQ=="
                  "&Expires=1592946176";

            http::url target_url(url);

            value = target_url.protocol();
            expected_value = HTTPS_PROTOCOL;
            if(debug) cerr << prolog << "target_url.protocol(): " << value << " expected: " << expected_value << endl;
            CPPUNIT_ASSERT( value == expected_value );

            value = target_url.host();
            expected_value = "d1sd4up8kynpk2.cloudfront.net";
            if(debug) cerr << prolog << "target_url.host(): " << value << " expected: " << expected_value << endl;
            CPPUNIT_ASSERT( value == expected_value );

            value = target_url.path();
            expected_value = "/s3-2dbad80ed80161e4b685a0385c322d93/rss_demo/rssmif16d__7/f16_ssmis_20200512v7.nc";
            if(debug) cerr << prolog << "target_url.path(): " << value << " expected: " << expected_value << endl;
            CPPUNIT_ASSERT( value == expected_value );

            key = "RequestId";
            expected_value = "yU6NwaRaSZBwQ0xexo5Ufv7aL0MeANMMM7oeB96NfuJzrfjVNmW9eQ==";
            value = target_url.query_parameter_value(key);
            if(debug) cerr << prolog << "key: " << key << " value: " << value << " expected: " << expected_value << endl;
            CPPUNIT_ASSERT( value == expected_value );

            key = "Expires";
            expected_value = "1592946176";
            value = target_url.query_parameter_value(key);
            if(debug) cerr << prolog << "key: " << key << " value: " << value << " expected: " << expected_value << endl;
            CPPUNIT_ASSERT( value == expected_value );

        }

        /*
        void signed_url_is_expired_test(){
            string prolog = string(__func__) + "() - ";

            string url;
            std::map<std::string,std::string> url_info;
            bool is_expired;


            url = "https://ghrcw-protected.s3.us-west-2.amazonaws.com/rss_demo/rssmif16d__7/f16_ssmis_20200512v7.nc?"
                  "A-userid=hyrax"
                  "&X-Amz-Algorithm=AWS4-HMAC-SHA256"
                  "&X-Amz-Credential=SomeBigMessyAwfulEncodedEscapeBunchOfCryptoPhaffing"
                  "&X-Amz-Date=20200621T161744Z"
                  "&X-Amz-Expires=86400"
                  "&X-Amz-Security-Token=FwoGZXIvYXdzENL%2F%2F%2F%2F%2F%2F%2F%2F%2F%2FwEaDKmu"
                  "SomeBigMessyAwfulEncodedEscapeBunchOfCryptoPhaffingSomeBigMessyAwfulEncodedEscapeBunchOfCryptoPhaffing"
                  "SomeBigMessyAwfulEncodedEscapeBunchOfCryptoPhaffingSomeBigMessyAwfulEncodedEscapeBunchOfCryptoPhaffing"
                  "SomeBigMessyAwfulEncodedEscapeBunchOfCryptoPhaffingSomeBigMessyAwfulEncodedEscapeBunchOfCryptoPhaffing"
                  "SomeBigMessyAwfulEncodedEscapeBunchOfCryptoPhaffingSomeBigMessyAwfulEncodedEscapeBunchOfCryptoPhaffing"
                  "&X-Amz-SignedHeaders=host"
                  "&X-Amz-Signature=SomeBigMessyAwfulEncodedEscapeBunchOfCryptoPhaffing";

            http::url signed_url(url);

            time_t now;
            time(&now);
            stringstream ingest_time;
            time_t then = now - 82810; // 23 hours and 10 seconds ago.

            signed_url.set_ingest_time(then);
            is_expired = NgapApi::signed_url_is_expired(signed_url);
            CPPUNIT_ASSERT(is_expired == true );

        }
         */

    CPPUNIT_TEST_SUITE( HttpUtilsTest );

            CPPUNIT_TEST(decompose_simple_url_test);
            CPPUNIT_TEST(decompose_aws_signed_request_url_test);
            // CPPUNIT_TEST(signed_url_is_expired_test);
            CPPUNIT_TEST(tokenize_test);

        CPPUNIT_TEST_SUITE_END();
    };

    CPPUNIT_TEST_SUITE_REGISTRATION(HttpUtilsTest);

} // namespace dmrpp

int main(int argc, char*argv[])
{
    CppUnit::TextTestRunner runner;
    runner.addTest(CppUnit::TestFactoryRegistry::getRegistry().makeTest());

    int option_char;
    while ((option_char = getopt(argc, argv, "dbD")) != -1)
        switch (option_char) {
            case 'd':
                debug = true;  // debug is a static global
                break;
            case 'D':
                Debug = true;  // Debug is a static global
                break;
            case 'b':
                bes_debug = true;  // debug is a static global
                break;
            default:
                break;
        }

    argc -= optind;
    argv += optind;

    /*cerr << "    debug: " << (debug?"enabled":"disabled") << endl;
    cerr << "    Debug: " << (Debug?"enabled":"disabled") << endl;
    cerr << "bes_debug: " << (bes_debug?"enabled":"disabled") << endl;*/

    bool wasSuccessful = true;
    string test = "";
    if (0 == argc) {
        // run them all
        wasSuccessful = runner.run("");
    }
    else {
        int i = 0;
        while (i < argc) {
            if (debug) cerr << "Running " << argv[i] << endl;
            test = http::HttpUtilsTest::suite()->getName().append("::").append(argv[i]);
            wasSuccessful = wasSuccessful && runner.run(test);
            ++i;
        }
    }

    return wasSuccessful ? 0 : 1;
}
