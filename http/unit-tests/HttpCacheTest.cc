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
#include <future>
#include <thread>

#include "HttpCache.h"
#include "HttpNames.h"
#include "BESDebug.h"
#include "BESUtil.h"
#include "TheBESKeys.h"
#include "BESInternalError.h"

#include "test_config.h"

// Maybe the common testing code in modules should be moved up one level? jhrg 11/3/22
#include "modules/common/run_tests_cppunit.h"

using namespace std;

#define prolog std::string("HttpCacheTest::").append(__func__).append("() - ")

namespace http {

class HttpCacheTest : public CppUnit::TestFixture {
public:
    string d_data_dir = TEST_DATA_DIR;
    string d_build_dir = TEST_BUILD_DIR;

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

    void tearDown() override {
        DBG(cerr << endl);
        DBG(cerr << "tearDown() - BEGIN" << endl);

        // Reload the keys after every test.
        delete TheBESKeys::d_instance;
        TheBESKeys::d_instance = nullptr;

        DBG(cerr << "tearDown() - END" << endl);
    }

/*##################################################################################################*/
/* TESTS BEGIN */

    void get_http_cache_dir_from_config_test_1() {
        DBG(cerr << endl);
        DBG(cerr << prolog << "BEGIN" << endl);

        string cache_dir = get_http_cache_dir_from_config();
        DBG(cerr << prolog << "cache_dir: " << cache_dir << endl);

        CPPUNIT_ASSERT(cache_dir == d_build_dir + "/cache");

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

    void get_instance_test_1() {
        DBG(cerr << endl);
        DBG(cerr << prolog << "BEGIN" << endl);

        auto cache = HttpCache::get_instance();
        CPPUNIT_ASSERT_MESSAGE("Expected get_instance() to return a non-null pointer", cache != nullptr);

        DBG(cerr << prolog << "END" << endl);
    }

    void get_instance_test_2() {
        DBG(cerr << endl);
        DBG(cerr << prolog << "BEGIN" << endl);

        auto cache = HttpCache::get_instance();
        CPPUNIT_ASSERT_MESSAGE("Expected get_instance() to return a non-null pointer", cache != nullptr);
        CPPUNIT_ASSERT_MESSAGE("Expected the cache directory",
                               cache->get_cache_directory() == d_build_dir + "/cache");
        CPPUNIT_ASSERT_MESSAGE("Expected the cache prefix", cache->get_cache_file_prefix() == "hut_");

        DBG(cerr << prolog << "END" << endl);
    }

    void get_instance_test_3() {
        DBG(cerr << endl);
        DBG(cerr << prolog << "BEGIN" << endl);

        auto cache = HttpCache::get_instance();
        auto cache2 = HttpCache::get_instance();
        CPPUNIT_ASSERT_MESSAGE("Expected get_instance() to return the same pointer", cache == cache2);

        DBG(cerr << prolog << "END" << endl);
    }

    // Multiple threads can call get_instance() and get the same pointer.
    void get_instance_test_4() {
        DBG(cerr << endl);
        DBG(cerr << prolog << "BEGIN" << endl);

        vector<future<HttpCache*>> futures;

        for (size_t i = 0; i < 3; ++i) {
            futures.emplace_back(std::async(std::launch::async, []() {
                return HttpCache::get_instance();
            }));
        }

        DBG(cerr << "I'm doing my own work!" << endl);
        std::this_thread::sleep_for(std::chrono::seconds(1));
        DBG(cerr << "I'm done with my own work!" << endl);

        DBG(cerr << "Start querying" << endl);
        const auto c1 = futures[0].get();
        const auto c2 = futures[1].get();
        const auto c3 = futures[2].get();
        DBG(cerr << "Done querying" << endl);
        CPPUNIT_ASSERT_MESSAGE("Expected get_instance() to return the same pointer", c1 == c2);
        CPPUNIT_ASSERT_MESSAGE("Expected get_instance() to return the same pointer", c2 == c3);

        DBG(cerr << prolog << "END" << endl);
    }

    void get_cache_file_name_test_1() {
        DBG(cerr << endl);
        DBG(cerr << prolog << "BEGIN" << endl);

        auto cache = HttpCache::get_instance();
        string url = "http://www.opendap.org";
        string file_name = cache->get_cache_file_name("bob", url);
        const string prefix = d_build_dir + "/cache/hut_";
        const string hash = "ce3afbaa5fdeeb94ae3fb8e34571072c765f6922c6d5ebbed65da64b4b98ba90"; // sha256 hash of the url
        DBG(cerr << prolog << "file_name: " << file_name << endl);
        CPPUNIT_ASSERT_MESSAGE("Expected file name to use when caching data", file_name == prefix + "bob_" + hash);

        DBG(cerr << prolog << "END" << endl);
    }

    void get_cache_file_name_test_2() {
        DBG(cerr << endl);
        DBG(cerr << prolog << "BEGIN" << endl);

        auto cache = HttpCache::get_instance();
        string url = "http://www.opendap.org";
        string file_name = cache->get_cache_file_name("", url);
        const string prefix = d_build_dir + "/cache/hut_";
        const string hash = "ce3afbaa5fdeeb94ae3fb8e34571072c765f6922c6d5ebbed65da64b4b98ba90"; // sha256 hash of the url
        DBG(cerr << prolog << "file_name: " << file_name << endl);
        CPPUNIT_ASSERT_MESSAGE("Expected file name to use when caching data", file_name == prefix + hash);

        DBG(cerr << prolog << "END" << endl);
    }

    void get_cache_file_name_test_3() {
        DBG(cerr << endl);
        DBG(cerr << prolog << "BEGIN" << endl);

        auto cache = HttpCache::get_instance();
        string url = "http://www.opendap.org";
        string file_name = cache->get_cache_file_name("", url, false);
        const string prefix = d_build_dir + "/cache/hut_";
        DBG(cerr << prolog << "file_name: " << file_name << endl);
        CPPUNIT_ASSERT_MESSAGE("Expected file name to use when caching data", file_name == prefix + url);

        DBG(cerr << prolog << "END" << endl);
    }

    void get_cache_file_name_test_4() {
        DBG(cerr << endl);
        DBG(cerr << prolog << "BEGIN" << endl);

        auto cache = HttpCache::get_instance();
        string url = "http://www.opendap.org/opendap/data/file.nc.dap";
        string file_name = cache->get_cache_file_name("bob", url);
        const string prefix = d_build_dir + "/cache/hut_";
        const string hash = "54afe4b284eef6f956f840871ae27b30bc600bcb4f330c35a8c16f4404e56edb#file.nc.dap"; // 'sha256 hash' of the url
        DBG(cerr << prolog << "file_name: " << file_name << endl);
        CPPUNIT_ASSERT_MESSAGE("Expected file name to use when caching data", file_name == prefix + "bob_" + hash);

        DBG(cerr << prolog << "END" << endl);
    }

    // The HttpCache::get_cache_file_name() method does not include the query string in the name, but it
    // is used when forming the hash, so the two URLs in the test_4 and this test have different hash values.
    void get_cache_file_name_test_4_1() {
        DBG(cerr << endl);
        DBG(cerr << prolog << "BEGIN" << endl);

        auto cache = HttpCache::get_instance();
        string url = "http://www.opendap.org/opendap/data/file.nc.dap?dap4.ce=/stuff;/stuff/here;/stuff/here.there";
        string file_name = cache->get_cache_file_name("bob", url);
        const string prefix = d_build_dir + "/cache/hut_";
        const string hash = "49fbd7c6653d2291399c8018cfebe34a98e368ad34485f641d52f282915d9d55#file.nc.dap"; // 'sha256 hash' of the url
        DBG(cerr << prolog << "file_name: " << file_name << endl);
        CPPUNIT_ASSERT_MESSAGE("Expected file name to use when caching data", file_name == prefix + "bob_" + hash);

        DBG(cerr << prolog << "END" << endl);
    }

    // Multithreaded test.
    void get_cache_file_name_test_4_2() {
        DBG(cerr << endl);
        DBG(cerr << prolog << "BEGIN" << endl);

        auto cache = HttpCache::get_instance();
        string url = "http://www.opendap.org/opendap/data/file.nc.dap?dap4.ce=/stuff;/stuff/here;/stuff/here.there";

        vector<future<string>> futures;

        for (size_t i = 0; i < 3; ++i) {
            futures.emplace_back(std::async(std::launch::async, [cache](const string &url) {
                return cache->get_cache_file_name("bob", url);
            }, url));
        }

        DBG(cerr << "I'm doing my own work!" << endl);
        std::this_thread::sleep_for(std::chrono::seconds(1));
        DBG(cerr << "I'm done with my own work!" << endl);

        DBG(cerr << "Start querying" << endl);
        auto fn1 = futures[0].get();
        auto fn2 = futures[1].get();
        auto fn3 = futures[2].get();
        DBG(cerr << "Done querying" << endl);
        CPPUNIT_ASSERT_MESSAGE("Expected get_instance() to return the same string", fn1 == fn2);
        CPPUNIT_ASSERT_MESSAGE("Expected get_instance() to return the same string", fn2 == fn3);

        const string prefix = d_build_dir + "/cache/hut_";
        const string hash = "49fbd7c6653d2291399c8018cfebe34a98e368ad34485f641d52f282915d9d55#file.nc.dap"; // 'sha256 hash' of the url
        DBG(cerr << prolog << "file_name: " << fn1 << endl);
        CPPUNIT_ASSERT_MESSAGE("Expected file name to use when caching data", fn1 == prefix + "bob_" + hash);

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

        CPPUNIT_TEST(get_instance_test_1);
        CPPUNIT_TEST(get_instance_test_2);
        CPPUNIT_TEST(get_instance_test_3);
        CPPUNIT_TEST(get_instance_test_4);  // Multithreaded test

        CPPUNIT_TEST(get_cache_file_name_test_1);
        CPPUNIT_TEST(get_cache_file_name_test_2);
        CPPUNIT_TEST(get_cache_file_name_test_3);
        CPPUNIT_TEST(get_cache_file_name_test_4);
        CPPUNIT_TEST(get_cache_file_name_test_4_1);
        CPPUNIT_TEST(get_cache_file_name_test_4_2); // Multithreaded test

    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(HttpCacheTest);

} // namespace http

int main(int argc, char *argv[]) {
    return bes_run_tests<http::HttpCacheTest>(argc, argv, "http,curl") ? 0 : 1;
}
