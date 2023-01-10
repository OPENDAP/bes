// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of the BES component of the Hyrax Data Server.

// Copyright (c) 2023 OPeNDAP, Inc.
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

#include <memory>
#include <iostream>

#include "HttpCache.h"
#include "HttpNames.h"
#include "BESDebug.h"
#include "BESUtil.h"
#include "TheBESKeys.h"
#include "BESInternalError.h"

#include "test_config.h"

// Maybe the common testing code in modules should be moved up one level? jhrg 11/3/22
#include "modules/run_tests_cppunit.h"

using namespace std;

#define prolog std::string("HttpCacheTest::").append(__func__).append("() - ")

namespace http {

class HttpCacheTest : public CppUnit::TestFixture {
public:
    string d_data_dir = TEST_DATA_DIR;

    // Called once before everything gets tested
    HttpCacheTest() = default;

    // Called at the end of the test
    ~HttpCacheTest() override = default;

    // Called before each test
    void setUp() override {
        DBG(cerr << endl);
        DBG(cerr << "setUp() - BEGIN" << endl);
        string bes_conf = BESUtil::assemblePath(TEST_BUILD_DIR, "bes.conf");
        DBG(cerr << "setUp() - Using BES configuration: " << bes_conf << endl);
        DBG2(show_file(bes_conf));
        TheBESKeys::ConfigFile = bes_conf;

        DBG(cerr << "setUp() - END" << endl);
    }

/*##################################################################################################*/
/* TESTS BEGIN */

    void get_http_cache_dir_from_config_test_1() {
        DBG(cerr << endl);
        DBG(cerr << prolog << "BEGIN" << endl);

        string cache_dir = get_http_cache_dir_from_config();
        DBG(cerr << prolog << "cache_dir: " << cache_dir << endl);

        CPPUNIT_ASSERT(cache_dir == "/Users/jimg/src/opendap/hyrax/bes/http/unit-tests/cache");

        DBG(cerr << prolog << "END" << endl);
    }
    void get_http_cache_dir_from_config_test_2() {
        DBG(cerr << endl);
        DBG(cerr << prolog << "BEGIN" << endl);

        // This comes close to testing the case where the key is not set, but it is
        // not perfect since the key is set in the bes.conf file.
        TheBESKeys::TheKeys()->delete_key(HTTP_CACHE_DIR_KEY);

        string cache_dir;
        CPPUNIT_ASSERT_THROW_MESSAGE("Expected get_http_cache_dir_from_config() to throw an exception",
                                     cache_dir = get_http_cache_dir_from_config(), BESInternalError);
        DBG(cerr << prolog << "cache_dir: " << cache_dir << endl);

        DBG(cerr << prolog << "END" << endl);
    }

    void get_http_cache_prefix_from_config_test_1() {
        DBG(cerr << endl);
        DBG(cerr << prolog << "BEGIN" << endl);

        string prefix = get_http_cache_prefix_from_config();
        DBG(cerr << prolog << "prefix: " << prefix << endl);

        CPPUNIT_ASSERT(prefix == "hut_");

        DBG(cerr << prolog << "END" << endl);
    }

    void get_http_cache_prefix_from_config_test_2() {
        DBG(cerr << endl);
        DBG(cerr << prolog << "BEGIN" << endl);

        // This comes close to testing the case where the key is not set, but it is
        // not perfect since the key is set in the bes.conf file.
        TheBESKeys::TheKeys()->delete_key(HTTP_CACHE_PREFIX_KEY);

        string prefix;
        CPPUNIT_ASSERT_THROW_MESSAGE("Expected get_http_cache_prefix_from_config() to throw an exception",
                                     prefix = get_http_cache_prefix_from_config(), BESInternalError);
        DBG(cerr << prolog << "prefix: " << prefix << endl);

        DBG(cerr << prolog << "END" << endl);
    }

    void get_http_cache_size_from_config_test_1() {
        DBG(cerr << endl);
        DBG(cerr << prolog << "BEGIN" << endl);

        unsigned long cache_size = get_http_cache_size_from_config();
        DBG(cerr << prolog << "cache_size: " << cache_size << endl);

        CPPUNIT_ASSERT(cache_size == 500);

        DBG(cerr << prolog << "END" << endl);
    }

    void get_http_cache_size_from_config_test_2() {
        DBG(cerr << endl);
        DBG(cerr << prolog << "BEGIN" << endl);

        // This comes close to testing the case where the key is not set, but it is
        // not perfect since the key is set in the bes.conf file.
        TheBESKeys::TheKeys()->delete_key(HTTP_CACHE_SIZE_KEY);

        unsigned long cache_size;
        CPPUNIT_ASSERT_THROW_MESSAGE("Expected get_http_cache_size_from_config() to throw an exception",
                                     cache_size = get_http_cache_size_from_config(), BESInternalError);
        DBG(cerr << prolog << "cache_size: " << cache_size << endl);

        DBG(cerr << prolog << "END" << endl);
    }

    void get_http_cache_exp_time_from_config_test_1() {
        DBG(cerr << endl);
        DBG(cerr << prolog << "BEGIN" << endl);

        unsigned long exp_time = get_http_cache_exp_time_from_config();
        DBG(cerr << prolog << "exp_time: " << exp_time << endl);

        CPPUNIT_ASSERT(exp_time == 300);

        DBG(cerr << prolog << "END" << endl);
    }

    // This one does not throw if the the key is not present; it has a default value.
    void get_http_cache_exp_time_from_config_test_2() {
        DBG(cerr << endl);
        DBG(cerr << prolog << "BEGIN" << endl);

        // This comes close to testing the case where the key is not set, but it is
        // not perfect since the key is set in the bes.conf file.
        TheBESKeys::TheKeys()->delete_key(HTTP_CACHE_EXPIRES_TIME_KEY);

        unsigned long exp_time = get_http_cache_exp_time_from_config();
        CPPUNIT_ASSERT(exp_time == REMOTE_RESOURCE_DEFAULT_EXPIRED_INTERVAL);
        DBG(cerr << prolog << "exp_time: " << exp_time << endl);

        DBG(cerr << prolog << "END" << endl);
    }

/* TESTS END */
/*##################################################################################################*/

    CPPUNIT_TEST_SUITE(HttpCacheTest);

        CPPUNIT_TEST(get_http_cache_dir_from_config_test_1);
        CPPUNIT_TEST(get_http_cache_dir_from_config_test_2);
        CPPUNIT_TEST(get_http_cache_prefix_from_config_test_1);
        CPPUNIT_TEST(get_http_cache_prefix_from_config_test_2);
        CPPUNIT_TEST(get_http_cache_size_from_config_test_1);
        CPPUNIT_TEST(get_http_cache_size_from_config_test_2);
        CPPUNIT_TEST(get_http_cache_exp_time_from_config_test_1);
        CPPUNIT_TEST(get_http_cache_exp_time_from_config_test_2);

    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(HttpCacheTest);

} // namespace http

int main(int argc, char *argv[]) {
    return bes_run_tests<http::HttpCacheTest>(argc, argv, "bes,http,curl") ? 0 : 1;
}
