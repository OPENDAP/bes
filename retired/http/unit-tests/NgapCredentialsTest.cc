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

#include "config.h"

#include <memory>

#include <unistd.h>
#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cstdio>
#include <time.h>

#include <unistd.h>
#include <libdap/util.h>
#include <libdap/debug.h>

#include <curl/curl.h>

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

#if HAVE_CURL_MULTI_H
#include <curl/multi.h>
#endif

#include "BESContextManager.h"
#include "BESError.h"
#include "BESInternalError.h"
#include "BESDebug.h"
#include "TheBESKeys.h"

#include "test_config.h"

#include "CurlUtils.h"
#include "NgapS3Credentials.h"

using namespace libdap;
using namespace std;

static bool debug = false;
static bool bes_debug = false;

#undef DBG
#define DBG(x) do { if (debug) x; } while(false)

namespace http {

class NgapCredentialsTest : public CppUnit::TestFixture {

private:
    string distribution_api_endpoint = "https://d33imu0z1ajyhj.cloudfront.net/s3credentials";

public:

    // Called once before everything gets tested
    NgapCredentialsTest() = default;

    // Called at the end of the test
    ~NgapCredentialsTest() = default;

    // Called before each test
    void setUp() override {
        if (debug) cout << endl;
        if (bes_debug) BESDebug::SetUp("cerr,dmrpp,ngap,http,curl");

        TheBESKeys::ConfigFile = string(TEST_BUILD_DIR).append("/bes.conf");

        curl_global_init(CURL_GLOBAL_ALL);
    }

    // Called after each test
    void tearDown() override {
        curl_global_cleanup();
    }

    void test_ngap_creds_object() {
        try {
            NgapS3Credentials nsc(distribution_api_endpoint, 600);
            //nsc.get_temporary_credentials();



            if (debug)
                cout << "                  NgapS3Credentials::ID: " << nsc.get(NgapS3Credentials::ID_KEY) << endl;
            if (debug)
                cout << "                 NgapS3Credentials::KEY: " << nsc.get(NgapS3Credentials::KEY_KEY) << endl;
            //if(debug) cout << "            NgapS3Credentials::BUCKET: " << nsc.get(NgapS3Credentials::BUCKET_KEY) << endl;
            if (debug)
                cout << "   NgapS3Credentials::AWS_SESSION_TOKEN: " << nsc.get(NgapS3Credentials::AWS_SESSION_TOKEN)
                     << endl;
            if (debug)
                cout << "NgapS3Credentials::AWS_TOKEN_EXPIRATION: " << nsc.get(NgapS3Credentials::AWS_TOKEN_EXPIRATION_KEY)
                     << endl;

            time_t now = time(0);
            if (debug) cout << "         now: " << now << endl;
            if (debug) cout << "     expires: " << nsc.expires() << endl;
            if (debug) cout << "needsRefresh: " << (nsc.needs_refresh() ? "true" : "false") << endl;

            time_t diff = nsc.expires() - now;
            if (debug)
                cout << "AWS credentials expire in " << diff << " seconds. (" << diff / 60.0 << " minutes, "
                     << diff / 60.0 / 60.0 << " hours)" << endl;

        }
        catch (BESError e) {
            cerr << "Caught BESError. Message: " << e.get_message() << "  ";
            cerr << "[" << e.get_file() << ":" << e.get_line() << "]" << endl;
            CPPUNIT_ASSERT(false);
        }
    }


CPPUNIT_TEST_SUITE(NgapCredentialsTest);
        CPPUNIT_TEST(test_ngap_creds_object);
    CPPUNIT_TEST_SUITE_END();

};

CPPUNIT_TEST_SUITE_REGISTRATION(NgapCredentialsTest);


} // namespace http

int main(int argc, char *argv[]) {
    CppUnit::TextTestRunner runner;
    runner.addTest(CppUnit::TestFactoryRegistry::getRegistry().makeTest());

    int option_char;
    while ((option_char = getopt(argc, argv, "c:dbu:p:")) != -1)
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

    argc -= optind;
    argv += optind;

    bool wasSuccessful = true;
    if (0 == argc) {
        // run them all
        wasSuccessful = runner.run("");
    }
    else {
        int i = 0;
        while (i < argc) {
            if (debug) cerr << "Running " << argv[i] << endl;
            string test = http::NgapCredentialsTest::suite()->getName().append("::").append(argv[i]);
            wasSuccessful = wasSuccessful && runner.run(test);
            ++i;
        }
    }

    return wasSuccessful ? 0 : 1;
}
