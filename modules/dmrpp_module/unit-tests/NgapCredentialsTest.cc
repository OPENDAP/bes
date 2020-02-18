// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of libdap, A C++ implementation of the OPeNDAP Data
// Access Protocol.

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

#include <memory>

#include <stdlib.h>
#include <unistd.h>
#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cstdio>

#include <GetOpt.h>
#include <util.h>
#include <debug.h>

#include <curl/curl.h>
#include "xml2json/include/xml2json.hpp"

#include "xml2json/include/rapidjson/document.h"
#include "xml2json/include/rapidjson/writer.h"

#if HAVE_CURL_MULTI_H
#include <curl/multi.h>
#endif

#include "BESContextManager.h"
#include "BESError.h"
#include "BESInternalError.h"
#include "BESDebug.h"
#include "TheBESKeys.h"

#include "test_config.h"
#include "../curl_utils.h"


#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

using namespace libdap;

static bool debug = false;
static bool bes_debug = false;

#undef DBG
#define DBG(x) do { if (debug) x; } while(false)

namespace dmrpp {

    /**
     * @brief http_get_as_json() This function de-references the target_url and parses the response into a JSON document.
     *
     * @param target_url The URL to dereference.
     * @return JSON document parsed from the response document returned by target_url
     */ // @TODO @FIXME Move this to ../curl_utils.cc (Requires moving the rapidjson lib too)
    rapidjson::Document http_get_as_json(const std::string &target_url){

        // @TODO @FIXME Make the size of this buffer a configuration setting, or pass it in, something....
        char response_buf[1024 * 1024];

        curl::http_get(target_url, response_buf);
        rapidjson::Document d;
        d.Parse(response_buf);
        return d;
    }

    class NgapCredentialsTest : public CppUnit::TestFixture {

    private:

    public:

        // Called once before everything gets tested
        NgapCredentialsTest() = default;

        // Called at the end of the test
        ~NgapCredentialsTest() = default;

        // Called before each test
        void setUp() override {
            if (debug) cout << endl;
            if (bes_debug) BESDebug::SetUp("cerr,dmrpp");

            TheBESKeys::ConfigFile = string(TEST_BUILD_DIR).append("/bes.conf");

            curl_global_init(CURL_GLOBAL_ALL);

        }

        // Called after each test
        void tearDown() override {
            curl_global_cleanup();
        }


#define AWS_ACCESS_KEY_ID_KEY "accessKeyId"
#define AWS_SECRET_ACCESS_KEY_KEY "secretAccessKey"
#define AWS_SESSION_TOKEN_KEY "sessionToken"
#define AWS_EXPIRATION_KEY "expiration"

        void get_s3_creds() {
            if(debug) cout << endl;
            string distribution_api_endpoint = "https://d33imu0z1ajyhj.cloudfront.net/s3credentials";
            string fnoc1_dds = "http://test.opendap.org/opendap/data/nc/fnoc1.nc.dds";
            string local_fnoc1="http://localhost:8080/opendap/data/nc/fnoc1.nc.dds";
            string target_url = distribution_api_endpoint;

            string accessKeyId, secretAccessKey, sessionToken, expiration;

            if(debug) cout << "Target URL: " << target_url<< endl;
            try {
                rapidjson::Document d = http_get_as_json(target_url);
                if(debug) cout << "S3 Credentials:"  << endl;

                rapidjson::Value& val = d[AWS_ACCESS_KEY_ID_KEY];
                accessKeyId = val.GetString();
                if(debug) cout << "    " << AWS_ACCESS_KEY_ID_KEY << ":     "  << accessKeyId << endl;

                val = d[AWS_SECRET_ACCESS_KEY_KEY];
                secretAccessKey = val.GetString();
                if(debug) cout << "    "  << AWS_SECRET_ACCESS_KEY_KEY << ": "  << secretAccessKey << endl;

                val = d[AWS_SESSION_TOKEN_KEY];
                sessionToken = val.GetString();
                if(debug) cout << "    "  << AWS_SESSION_TOKEN_KEY << ":    " << sessionToken  << endl;

                val = d[AWS_EXPIRATION_KEY];
                expiration = val.GetString();
                if(debug) cout << "    "  << AWS_EXPIRATION_KEY << ":      "  << expiration << endl;

            }
            catch (BESError e) {
                cerr << "Caught BESError. Message: " << e.get_message() << "  ";
                cerr << "[" << e.get_file() << ":" << e.get_line() << "]" << endl;
                CPPUNIT_ASSERT(false);
            }
        }


        CPPUNIT_TEST_SUITE(NgapCredentialsTest);
        CPPUNIT_TEST(get_s3_creds);
        CPPUNIT_TEST_SUITE_END();

    };

    CPPUNIT_TEST_SUITE_REGISTRATION(NgapCredentialsTest);


} // namespace dmrpp

int main(int argc, char *argv[]) {
    CppUnit::TextTestRunner runner;
    runner.addTest(CppUnit::TestFactoryRegistry::getRegistry().makeTest());

    GetOpt getopt(argc, argv, "c:dbu:p:");
    int option_char;
    while ((option_char = getopt()) != -1)
        switch (option_char) {
            case 'd':
                debug = true;  // debug is a static global
                break;
            case 'b':
                debug = true;  // debug is a static global
                bes_debug = true;  // debug is a static global
                break;
            default:
                break;
        }

    bool wasSuccessful = true;
    int i = getopt.optind;




    if (i == argc) {
        // run them all
        wasSuccessful = runner.run("");
    } else {
        while (i < argc) {
            if (debug) cerr << "Running " << argv[i] << endl;
            string test = dmrpp::NgapCredentialsTest::suite()->getName().append("::").append(argv[i]);
            wasSuccessful = wasSuccessful && runner.run(test);
            ++i;
        }
    }

    return wasSuccessful ? 0 : 1;
}
