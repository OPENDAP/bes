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

#include "BESUtil.h"
#include "TheBESKeys.h"
#include "BESContextManager.h"
#include "BESInternalError.h"
#include "BESSyntaxUserError.h"

#include "NgapOwnedContainer.h"
#include "NgapRequestHandler.h"

#include "run_tests_cppunit.h"
#include "test_config.h"

using namespace std;

auto const DMRPP_LOCATION = "https://dmrpp-sit-poc.s3.amazonaws.com";
auto const DMRPP_TEST_BUCKET_OPENDAP_AWS = "https://s3.amazonaws.com/cloudydap";
auto const TEST_DATA_LOCATION = string("file://") + TEST_SRC_DIR;

#define TEST_NAME DBG(cerr << __PRETTY_FUNCTION__ << "()\n")
#define prolog string("NgapOwnedContainerTest::").append(__func__).append("() - ")

namespace ngap {

class NgapOwnedContainerTest: public CppUnit::TestFixture {
    string d_cache_dir = string(TEST_BUILD_DIR) + "/owned-cache";

public:
    // Called once before everything gets tested
    NgapOwnedContainerTest() = default;
    ~NgapOwnedContainerTest() override = default;
    NgapOwnedContainerTest(const NgapOwnedContainerTest &src) = delete;
    const NgapOwnedContainerTest &operator=(const NgapOwnedContainerTest & rhs) = delete;

    void set_bes_keys() const {
        TheBESKeys::TheKeys()->set_key("BES.LogName", "./bes.log");
        TheBESKeys::TheKeys()->set_key("BES.Catalog.catalog.RootDirectory", "/tmp"); // any dir that exists will do
        TheBESKeys::TheKeys()->set_key("BES.Catalog.catalog.TypeMatch", "any-value:will-do");
        TheBESKeys::TheKeys()->set_key("AllowedHosts", ".*");
        // Use the env var creds for real access to DMR++ in S3. This works
        // only for a given URL. On my machine that is https://s3.amazonaws.com/cloudydap.
        // jhrg 5/17/24
        TheBESKeys::TheKeys()->set_key("CredentialsManager.config", "ENV_CREDS");
    }

    void configure_ngap_handler() const {
        NgapRequestHandler::d_use_dmrpp_cache = true;
        NgapRequestHandler::d_dmrpp_file_cache_dir = d_cache_dir;
        NgapRequestHandler::d_dmrpp_file_cache_size_mb = 100 * MEGABYTE; // MB
        NgapRequestHandler::d_dmrpp_file_cache_purge_size_mb = 20 * MEGABYTE; // MB
        NgapRequestHandler::d_dmrpp_file_cache.initialize(NgapRequestHandler::d_dmrpp_file_cache_dir,
                                                          NgapRequestHandler::d_dmrpp_file_cache_size_mb,
                                                          NgapRequestHandler::d_dmrpp_file_cache_purge_size_mb);
        NgapRequestHandler::d_dmrpp_mem_cache.initialize(100, 20);
    }

    void setUp() override {
        set_bes_keys();
        configure_ngap_handler();
    }

    // Delete the cache dir after each test; really only needed for the
    // tests toward the end of the suite that test the FileCache.
    void tearDown() override {
        NgapRequestHandler::d_dmrpp_file_cache.clear();
        NgapRequestHandler::d_dmrpp_mem_cache.clear();
    }

    void test_file_to_string() {
        TEST_NAME;
        string content;
        string file_name = string(TEST_SRC_DIR) + "/data/chunked_enum.h5.dmrpp";
        int fd = open(file_name.c_str(), O_RDONLY);
        CPPUNIT_ASSERT_MESSAGE("The file should open", fd > 0);
        CPPUNIT_ASSERT_MESSAGE("The file should be read", NgapOwnedContainer::file_to_string(fd, content));
        CPPUNIT_ASSERT_MESSAGE("The file should be closed", close(fd) == 0);
        CPPUNIT_ASSERT_MESSAGE("The file should have content", !content.empty());
        DBG2(cerr << "Content length : " << content.size() << '\n');
        CPPUNIT_ASSERT_MESSAGE("The file should be > 1k (was" + to_string(content.size()) + ").", content.size() > 1'000);
    }

    void test_file_to_string_bigger_than_buffer() {
        TEST_NAME;
        string content;
        string file_name = "NGAPApiTest.cc";    // ~16k while the buffer is 4k
        int fd = open(file_name.c_str(), O_RDONLY);
        CPPUNIT_ASSERT_MESSAGE("The file should open", fd > 0);
        CPPUNIT_ASSERT_MESSAGE("The file should be read", NgapOwnedContainer::file_to_string(fd, content));
        CPPUNIT_ASSERT_MESSAGE("The file should be closed", close(fd) == 0);
        CPPUNIT_ASSERT_MESSAGE("The file should have content", !content.empty());
        DBG2(cerr << "Content length : " << content.size() << '\n');
        CPPUNIT_ASSERT_MESSAGE("The file should be > 16k (was" + to_string(content.size()) + ").", content.size() > 16'000);
    }

    void test_file_to_string_file_not_open() {
        TEST_NAME;
        string content;
        string file_name = "NGAPApiTest.cc";    // ~16k while the buffer is 4k
        int fd = -1;
        CPPUNIT_ASSERT_MESSAGE("The file should open", fd < 0);
        CPPUNIT_ASSERT_MESSAGE("The function should return false", !NgapOwnedContainer::file_to_string(fd, content));
        CPPUNIT_ASSERT_MESSAGE("The string should be empty", content.empty());
    }

    void test_build_dmrpp_url_to_owned_bucket() {
        TEST_NAME;
        string rest_path = "collections/C1996541017-GHRC_DAAC/granules/amsua15_2020.028_12915_1139_1324_WI.nc";
        string expected = "https://dmrpp-sit-poc.s3.amazonaws.com/C1996541017-GHRC_DAAC/amsua15_2020.028_12915_1139_1324_WI.nc.dmrpp";
        string actual = NgapOwnedContainer::build_dmrpp_url_to_owned_bucket(rest_path, DMRPP_LOCATION);
        CPPUNIT_ASSERT_MESSAGE("The URL should be built (got: " + actual + " expected: " + expected + ").",
                               actual == expected);
    }

    void test_build_dmrpp_url_to_owned_bucket_bad_path() {
        TEST_NAME;
        string rest_path = "collections/C1996541017-GHRC_DAAC/granules/amsua15_2020.028_12915_1139_1324_WI.nc/extra";
        CPPUNIT_ASSERT_THROW_MESSAGE("The function should throw",
                                     NgapOwnedContainer::build_dmrpp_url_to_owned_bucket(rest_path, DMRPP_LOCATION), BESSyntaxUserError);
    }

    void test_build_dmrpp_url_to_owned_bucket_bad_path_2() {
        TEST_NAME;
        string rest_path = "C1996541017-GHRC_DAAC/granules/amsua15_2020.028_12915_1139_1324_WI.nc";
        CPPUNIT_ASSERT_THROW_MESSAGE("The function should throw",
                                     NgapOwnedContainer::build_dmrpp_url_to_owned_bucket(rest_path, DMRPP_LOCATION), BESSyntaxUserError);
    }

    void test_build_dmrpp_url_to_owned_bucket_bad_path_3() {
        TEST_NAME;
        string rest_path = "collections/C1996541017-GHRC_DAAC/amsua15_2020.028_12915_1139_1324_WI.nc";
        CPPUNIT_ASSERT_THROW_MESSAGE("The function should throw",
                                     NgapOwnedContainer::build_dmrpp_url_to_owned_bucket(rest_path, DMRPP_LOCATION), BESSyntaxUserError);
    }

    void test_build_dmrpp_url_to_owned_bucket_bad_path_4() {
        TEST_NAME;
        string rest_path = "C1996541017-GHRC_DAAC/amsua15_2020.028_12915_1139_1324_WI.nc";
        CPPUNIT_ASSERT_THROW_MESSAGE("The function should throw",
                                     NgapOwnedContainer::build_dmrpp_url_to_owned_bucket(rest_path, DMRPP_LOCATION), BESSyntaxUserError);
    }
    void test_build_dmrpp_url_to_owned_bucket_bad_path_5() {
        TEST_NAME;
        string rest_path = "";
        CPPUNIT_ASSERT_THROW_MESSAGE("The function should throw",
                                     NgapOwnedContainer::build_dmrpp_url_to_owned_bucket(rest_path, DMRPP_LOCATION), BESSyntaxUserError);
    }

    void test_item_in_cache() {
        TEST_NAME;
        string dmrpp_string;
        NgapOwnedContainer container;
        container.set_real_name("/data/dmrpp/a2_local_twoD.h5");
        CPPUNIT_ASSERT_MESSAGE("The item should not be in the cache", !container.get_item_from_dmrpp_cache(dmrpp_string));
        CPPUNIT_ASSERT_MESSAGE("The item should empty", dmrpp_string.empty());
    }

    void test_cache_item() {
        TEST_NAME;
        string dmrpp_string = "cached DMR++";
        NgapOwnedContainer container;
        container.set_real_name("/data/dmrpp/a2_local_twoD.h5");
        CPPUNIT_ASSERT_MESSAGE("The item should be added to the cache", container.put_item_in_dmrpp_cache(dmrpp_string));
        string cached_value;
        CPPUNIT_ASSERT_MESSAGE("The item should be in the cache", container.get_item_from_dmrpp_cache(cached_value));
        CPPUNIT_ASSERT_MESSAGE("The item should be the same", cached_value == dmrpp_string);
    }

    void test_cache_item_stomp() {
        TEST_NAME;
        string dmrpp_string = "cached DMR++";
        NgapOwnedContainer container;
        container.set_real_name("/data/dmrpp/a2_local_twoD.h5");
        CPPUNIT_ASSERT_MESSAGE("The item should be added to the cache", container.put_item_in_dmrpp_cache(dmrpp_string));
        string cached_value;
        CPPUNIT_ASSERT_MESSAGE("The item should be in the cache", container.get_item_from_dmrpp_cache(cached_value));
        CPPUNIT_ASSERT_MESSAGE("The item should be the same", cached_value == dmrpp_string);

        // now 'stomp' on the cached item
        CPPUNIT_ASSERT_MESSAGE("The item should not be added to the cache", !container.put_item_in_dmrpp_cache(
                "Over-written cache item"));
        CPPUNIT_ASSERT_MESSAGE("The item should be in the cache", container.get_item_from_dmrpp_cache(cached_value));
        DBG2(cerr << "Cached value: " << cached_value << '\n');
        CPPUNIT_ASSERT_MESSAGE("The item should be the same", cached_value == dmrpp_string);
    }

    void test_get_dmrpp_from_cache_or_remote_source() {
        TEST_NAME;
        string dmrpp_string;
        NgapOwnedContainer container;
        // The REST path will become data/d_int.h5
        container.set_real_name("collections/data/granules/d_int.h5");
        // Set the location of the data as a file:// URL for this test.
        container.set_data_source_location(TEST_DATA_LOCATION);
        CPPUNIT_ASSERT_MESSAGE("The DMR++ should be found", container.get_dmrpp_from_cache_or_remote_source(dmrpp_string));
        DBG2(cerr << "DMR++: " << dmrpp_string << '\n');
        CPPUNIT_ASSERT_MESSAGE("The DMR++ should be in the string", !dmrpp_string.empty());
    }

    void test_get_dmrpp_from_cache_or_remote_source_cache_consistency() {
        TEST_NAME;
        string dmrpp_string;
        NgapOwnedContainer container;
        // The REST path will become data/d_int.h5
        string real_name = "collections/data/granules/d_int.h5";
        container.set_real_name(real_name);
        // Set the location of the data as a file:// URL for this test.
        container.set_data_source_location(TEST_DATA_LOCATION);

        string cache_value;
        int status = container.get_item_from_dmrpp_cache(cache_value);
        CPPUNIT_ASSERT_MESSAGE("The DMR++ should not be in the cache (found: " + cache_value + ").", !status);

        CPPUNIT_ASSERT_MESSAGE("The DMR++ should be found", container.get_dmrpp_from_cache_or_remote_source(dmrpp_string));
        DBG2(cerr << "DMR++: " << dmrpp_string << '\n');
        CPPUNIT_ASSERT_MESSAGE("The DMR++ should be in the string", !dmrpp_string.empty());

        CPPUNIT_ASSERT_MESSAGE("The DMR++ should be in the cache", container.get_item_from_dmrpp_cache(cache_value));
        CPPUNIT_ASSERT_MESSAGE("The DMR++ should be in the cached value", !cache_value.empty());
    }

    void test_get_dmrpp_from_cache_or_remote_source_test_both_caches() {
        TEST_NAME;
        string dmrpp_string;
        NgapOwnedContainer container;
        // The REST path will become data/d_int.h5
        string real_name = "collections/data/granules/d_int.h5";
        container.set_real_name(real_name);
        // Set the location of the data as a file:// URL for this test.
        container.set_data_source_location(TEST_DATA_LOCATION);

        string key = FileCache::hash_key(real_name);
        FileCache::Item item;
        bool result = NgapRequestHandler::d_dmrpp_file_cache.get(key, item, LOCK_SH);
        CPPUNIT_ASSERT_MESSAGE("The DMR++ should not be in the file cache.", !result);

        string cache_value;
        result = NgapRequestHandler::d_dmrpp_mem_cache.get(real_name, cache_value);
        CPPUNIT_ASSERT_MESSAGE("The DMR++ should not be in the memory cache.", !result);

        CPPUNIT_ASSERT_MESSAGE("The DMR++ should be found", container.get_dmrpp_from_cache_or_remote_source(dmrpp_string));
        DBG2(cerr << "DMR++: " << dmrpp_string << '\n');
        CPPUNIT_ASSERT_MESSAGE("The DMR++ should be in the string", !dmrpp_string.empty());

        result = NgapRequestHandler::d_dmrpp_file_cache.get(key, item, LOCK_SH);
        CPPUNIT_ASSERT_MESSAGE("The DMR++ should not be in the file cache.", result);

        result = NgapRequestHandler::d_dmrpp_mem_cache.get(real_name, cache_value);
        CPPUNIT_ASSERT_MESSAGE("The DMR++ should not be in the memory cache.", result);
    }

    void test_get_dmrpp_from_cache_or_remote_source_test_cache_use() {
        TEST_NAME;
        string dmrpp_string;
        NgapOwnedContainer container;
        // The REST path will become data/d_int.h5
        container.set_real_name("collections/data/granules/d_int.h5");
        // Set the location of the data as a file:// URL for this test.
        container.set_data_source_location(TEST_DATA_LOCATION);

        string cached_value;
        int status = container.get_item_from_dmrpp_cache(cached_value);
        CPPUNIT_ASSERT_MESSAGE("The DMR++ should not be in the cache (found: " + cached_value + ").", !status);
        CPPUNIT_ASSERT_MESSAGE("The DMR++ should not be in the cached value", cached_value.empty());

        CPPUNIT_ASSERT_MESSAGE("The DMR++ should be found", container.get_dmrpp_from_cache_or_remote_source(dmrpp_string));
        DBG2(cerr << "DMR++: " << dmrpp_string << '\n');
        CPPUNIT_ASSERT_MESSAGE("The DMR++ should be in the string", !dmrpp_string.empty());

        CPPUNIT_ASSERT_MESSAGE("The DMR++ should be in the cache", container.get_dmrpp_from_cache_or_remote_source(cached_value));
        DBG2(cerr << "DMR++: " << cached_value << '\n');
        CPPUNIT_ASSERT_MESSAGE("The DMR++ should be in the cached value", !cached_value.empty());
    }

    void test_access() {
        TEST_NAME;

        NgapOwnedContainer container;
        // The REST path will become data/d_int.h5
        container.set_real_name("collections/data/granules/d_int.h5");
        // Set the location of the data as a file:// URL for this test.
        container.set_data_source_location(TEST_DATA_LOCATION);

        string dmrpp = container.access();
        DBG2(cerr << "DMR++: " << dmrpp << '\n');
        CPPUNIT_ASSERT_MESSAGE("The DMR++ should be in the string", !dmrpp.empty());

        string attrs = container.get_attributes();
        CPPUNIT_ASSERT_MESSAGE("The container attributes should be 'as-string'", attrs == "as-string");

        CPPUNIT_ASSERT_MESSAGE("The container type should be 'dmrpp'", container.get_container_type() == "dmrpp");
    }

    void test_access_s3() {
        TEST_NAME;

        if (getenv("CMAC_URL") == nullptr) {
            DBG(cerr << "Skipping test_access_s3 because AWS_ACCESS_KEY_ID is not set.\n");
            return;
        }

        NgapOwnedContainer container;
        // The REST path will become ngap_owned/d_int.h5. This exists in the S3 bucket
        // s3-module-test-bucket that DMRPP_TEST_BUCKET_OPENDAP_AWS points toward. jhrg 5/17/24
        container.set_real_name("collections/ngap_owned/granules/d_int.h5");
        // Set the location of the data as a file:// URL for this test.
        container.set_data_source_location(DMRPP_TEST_BUCKET_OPENDAP_AWS);

        string dmrpp = container.access();
        DBG2(cerr << "DMR++: " << dmrpp << '\n');
        CPPUNIT_ASSERT_MESSAGE("The response should not be empty", !dmrpp.empty());
        string dmrpp_str = R"(dmrpp:href="https://s3.amazonaws.com/cloudydap/ngap_owned/d_int.h5")";
        CPPUNIT_ASSERT_MESSAGE("The response should be a DMR++ XML document", dmrpp.find(dmrpp_str) != string::npos);

        string attrs = container.get_attributes();
        CPPUNIT_ASSERT_MESSAGE("The container attributes should be 'as-string'", attrs == "as-string");

        CPPUNIT_ASSERT_MESSAGE("The container type should be 'dmrpp'", container.get_container_type() == "dmrpp");
    }

    CPPUNIT_TEST_SUITE( NgapOwnedContainerTest );

    CPPUNIT_TEST(test_file_to_string);
    CPPUNIT_TEST(test_file_to_string_bigger_than_buffer);
    CPPUNIT_TEST(test_file_to_string_file_not_open);

    CPPUNIT_TEST(test_build_dmrpp_url_to_owned_bucket);
    CPPUNIT_TEST(test_build_dmrpp_url_to_owned_bucket_bad_path);
    CPPUNIT_TEST(test_build_dmrpp_url_to_owned_bucket_bad_path_2);
    CPPUNIT_TEST(test_build_dmrpp_url_to_owned_bucket_bad_path_3);
    CPPUNIT_TEST(test_build_dmrpp_url_to_owned_bucket_bad_path_4);
    CPPUNIT_TEST(test_build_dmrpp_url_to_owned_bucket_bad_path_5);

    CPPUNIT_TEST(test_item_in_cache);
    CPPUNIT_TEST(test_cache_item);
    CPPUNIT_TEST(test_cache_item_stomp);

    CPPUNIT_TEST(test_get_dmrpp_from_cache_or_remote_source);
    CPPUNIT_TEST(test_get_dmrpp_from_cache_or_remote_source_cache_consistency);
    CPPUNIT_TEST(test_get_dmrpp_from_cache_or_remote_source_test_both_caches);
    CPPUNIT_TEST(test_get_dmrpp_from_cache_or_remote_source_test_cache_use);

    CPPUNIT_TEST(test_access);
    CPPUNIT_TEST(test_access_s3);

    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(NgapOwnedContainerTest);

} // namespace dmrpp

int main(int argc, char*argv[])
{
    bool status = bes_run_tests<ngap::NgapOwnedContainerTest>(argc, argv, "cerr,ngap,cache");

    return status ? 0 : 1;
}
