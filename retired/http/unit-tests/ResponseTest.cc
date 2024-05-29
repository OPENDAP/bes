// ResponseTest.cc

// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of the OPeNDAP Back-End Server (BES)
// and creates a set of allowed hosts that may be
// accessed by the server as part of it's routine operation.

// Copyright (c) 2024 OPeNDAP, Inc.
// Author: Nathan D. Potter <ndp@opendap.org>
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

// Authors:
//      ndp       Nathan D. Potter <ndp@opendap.org>

#include "config.h"

#include <memory>
#include <iostream>
#include <future>
#include <thread>

#include "http/unused/Response.h"
#include "HttpNames.h"
#include "BESDebug.h"
#include "BESUtil.h"
#include "TheBESKeys.h"
#include "BESInternalError.h"

#include "test_config.h"

// Maybe the common testing code in modules should be moved up one level? jhrg 11/3/22
#include "modules/common/run_tests_cppunit.h"

using namespace std;

#define prolog std::string("ResponseTest::").append(__func__).append("() - ")

namespace http {

class ResponseTest : public CppUnit::TestFixture {
public:
    string d_data_dir = TEST_DATA_DIR;
    string d_build_dir = TEST_BUILD_DIR;

    // Called once before everything gets tested
    ResponseTest() = default;

    // Called at the end of the test
    ~ResponseTest() override = default;

    // Called before each test
    void setUp() override {
        DBG(cerr << endl);
        DBG(cerr << "setUp() - BEGIN" << endl);

        string bes_conf = BESUtil::assemblePath(TEST_BUILD_DIR, "bes.conf");
        DBG(cerr << "setUp() - Using BES configuration: " << bes_conf << endl);
        DBG2(show_file(bes_conf));
        TheBESKeys::ConfigFile = bes_conf;
        TheBESKeys::TheKeys()->reload_keys();

        DBG(cerr << "setUp() - END" << endl);
    }

/*##################################################################################################*/
/* TESTS BEGIN */

    void copy_hdrs_test(){
        auto *target = new vector<string>();
        DBG(cerr << prolog << "target: " << (void *)&target << "\n");

        {
            http::Response http_response;
            http_response.headers().emplace_back("foo: bar");
            http_response.headers().emplace_back("yankee: doodle");
            http_response.headers().emplace_back("old: mcdonald");

            for (const auto &hdr: http_response.headers()) {
                DBG(cerr << prolog << "http_response.headers(" << (void *) &(http_response.headers()) << ") - " << hdr << "\n");
            }
            *target = http_response.headers();
            CPPUNIT_ASSERT( &(http_response.headers()) != target);
        }

        for(const auto &hdr: *target){
            DBG(cerr << prolog << "target(" << (void *)target << ") - " << hdr << "\n");
        }

        delete target;
    }

    void who_owns_it_test(){
        string buf = "Eno";

        DBG(cerr << prolog << "buf(" << (void *)&buf << "): '" << buf << "'\n");
        {
            http::Response http_response;
            //http_response.body(buf);
            DBG(cerr << prolog << "http_response.body()(" << (void *)&http_response.body() << "): '" << http_response.body() << "'\n");
            http_response.body() = "My Life In The Bush Of Ghosts";
            DBG(cerr << prolog << "http_response.body()(" << (void *)&http_response.body() << "): '" << http_response.body() << "'\n");
            DBG(cerr << prolog << "buf(" << (void *)&buf << "): '" << buf << "'\n");
        }
        DBG(cerr << prolog << "buf(" << (void *)&buf << "): '" << buf << "'\n");
    }

    void response_test(){
        http::Response r;
        r.origin_url("file:///foo");
        r.redirect_url("file:///foo/bar/");
        r.curl_code(CURLE_OK);
        r.status(302);
        //r.headers().emplace_back("foo: bar");
        //r.headers().emplace_back("yankee: doodle");
        //r.headers().emplace_back("old: mcdonald");
        r.body() = string("Here Come The Warm Jets");
        DBG(cerr << prolog << r.dump() <<  "'\n");
    }

/* TESTS END */
/*##################################################################################################*/

    CPPUNIT_TEST_SUITE(ResponseTest);

        CPPUNIT_TEST(response_test);
        CPPUNIT_TEST(copy_hdrs_test);
        CPPUNIT_TEST(who_owns_it_test);

    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(ResponseTest);

} // namespace http

int main(int argc, char *argv[]) {
    return bes_run_tests<http::ResponseTest>(argc, argv, "http,curl") ? 0 : 1;
}

