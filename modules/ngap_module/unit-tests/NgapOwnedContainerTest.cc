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
auto const TEST_DATA_LOCATION = string("file://") + TEST_SRC_DIR;

#define TEST_NAME DBG(cerr << __PRETTY_FUNCTION__ << "()\n")
#define prolog string("NgapOwnedContainerTest::").append(__func__).append("() - ")

namespace ngap {

class NgapOwnedContainerTest: public CppUnit::TestFixture {
    string d_cache_dir = string(TEST_BUILD_DIR) + "/cache";

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
    }

    void configure_ngap_handler() const {
        NgapRequestHandler::d_use_dmrpp_cache = true;
        NgapRequestHandler::d_dmrpp_file_cache_dir = d_cache_dir;
        NgapRequestHandler::d_dmrpp_file_cache_size_mb = 100 * MEGABYTE; // MB
        NgapRequestHandler::d_dmrpp_file_cache_purge_size_mb = 20 * MEGABYTE; // MB
        NgapRequestHandler::d_dmrpp_file_cache.initialize(NgapRequestHandler::d_dmrpp_file_cache_dir,
                                                          NgapRequestHandler::d_dmrpp_file_cache_size_mb,
                                                          NgapRequestHandler::d_dmrpp_file_cache_purge_size_mb);
    }

    // Delete the cache dir after each test; really only needed for the
    // tests toward the end of the suite that test the FileCache.
    void tearDown() override {
        struct stat sb{0};
        if (stat(d_cache_dir.c_str(), &sb) == 0) {
            if (S_ISDIR(sb.st_mode)) {
                // remove the cache dir
                string cmd = "rm -rf " + d_cache_dir;
                int ret = system(cmd.c_str());
                if (ret != 0) {
                    CPPUNIT_FAIL("Failed to remove cache dir: " + d_cache_dir + '\n');
                }
            }
        }
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
        DBG(cerr << "Content length : " << content.size() << '\n');
        CPPUNIT_ASSERT_MESSAGE("The file should be > 1k", content.size() > 1'000);
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
        DBG(cerr << "Content length : " << content.size() << '\n');
        CPPUNIT_ASSERT_MESSAGE("The file should be > 16k", content.size() > 16'000);
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

        TEST_NAME;set_bes_keys();
        configure_ngap_handler();
        string dmrpp_string;
        NgapOwnedContainer container;
        container.set_real_name("/data/dmrpp/a2_local_twoD.h5");
        CPPUNIT_ASSERT_MESSAGE("The item should not be in the cache", !container.get_item_from_cache(dmrpp_string));
        CPPUNIT_ASSERT_MESSAGE("The item should empty", dmrpp_string.empty());
    }

    void test_cache_item() {
        TEST_NAME;
        set_bes_keys();
        configure_ngap_handler();
        string dmrpp_string = "cached DMR++";
        NgapOwnedContainer container;
        container.set_real_name("/data/dmrpp/a2_local_twoD.h5");
        CPPUNIT_ASSERT_MESSAGE("The item should be added to the cache", container.put_item_in_cache(dmrpp_string));
        string cached_value;
        CPPUNIT_ASSERT_MESSAGE("The item should be in the cache", container.get_item_from_cache(cached_value));
        CPPUNIT_ASSERT_MESSAGE("The item should be the same", cached_value == dmrpp_string);
    }

    void test_cache_item_stomp() {
        TEST_NAME;
        set_bes_keys();
        configure_ngap_handler();
        string dmrpp_string = "cached DMR++";
        NgapOwnedContainer container;
        container.set_real_name("/data/dmrpp/a2_local_twoD.h5");
        CPPUNIT_ASSERT_MESSAGE("The item should be added to the cache", container.put_item_in_cache(dmrpp_string));
        string cached_value;
        CPPUNIT_ASSERT_MESSAGE("The item should be in the cache", container.get_item_from_cache(cached_value));
        CPPUNIT_ASSERT_MESSAGE("The item should be the same", cached_value == dmrpp_string);

        // now 'stomp' on the cached item
        CPPUNIT_ASSERT_MESSAGE("The item should not be added to the cache", !container.put_item_in_cache(
                "Over-written cache item"));
        CPPUNIT_ASSERT_MESSAGE("The item should be in the cache", container.get_item_from_cache(cached_value));
        DBG(cerr << "Cached value: " << cached_value << '\n');
        CPPUNIT_ASSERT_MESSAGE("The item should be the same", cached_value == dmrpp_string);
    }

    void test_get_dmrpp_from_cache_or_remote_source() {
        TEST_NAME;
        set_bes_keys();
        configure_ngap_handler();

        string dmrpp_string;
        NgapOwnedContainer container;
        // The REST path will become data/d_int.h5
        container.set_real_name("collections/data/granules/d_int.h5");
        // Set the location of the data as a file:// URL for this test.
        container.set_data_source_location(TEST_DATA_LOCATION);
        CPPUNIT_ASSERT_MESSAGE("The DMR++ should be found", container.get_dmrpp_from_cache_or_remote_source(dmrpp_string));
        DBG(cerr << "DMR++: " << dmrpp_string << '\n');
        CPPUNIT_ASSERT_MESSAGE("The DMR++ should be in the string", !dmrpp_string.empty());
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

    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(NgapOwnedContainerTest);

} // namespace dmrpp

int main(int argc, char*argv[])
{
    bool status = bes_run_tests<ngap::NgapOwnedContainerTest>(argc, argv, "cerr,ngap,cache");

    return status ? 0 : 1;
}
