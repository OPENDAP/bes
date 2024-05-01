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

#include "NgapNames.h"
#include "NgapOwnedContainer.h"
#include "NgapRequestHandler.h"

#include "run_tests_cppunit.h"
#include "test_config.h"

using namespace std;

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
        string content;
        string file_name = string(TEST_DATA_DIR) + "/chunked_enum.h5.dmrpp";
        int fd = open(file_name.c_str(), O_RDONLY);
        CPPUNIT_ASSERT_MESSAGE("The file should open", fd > 0);
        CPPUNIT_ASSERT_MESSAGE("The file should be read", NgapOwnedContainer::file_to_string(fd, content));
        CPPUNIT_ASSERT_MESSAGE("The file should be closed", close(fd) == 0);
        CPPUNIT_ASSERT_MESSAGE("The file should have content", !content.empty());
        DBG(cerr << "Content length : " << content.size() << '\n');
        CPPUNIT_ASSERT_MESSAGE("The file should be > 1k", content.size() > 1'000);
    }

    void test_file_to_string_bigger_than_buffer() {
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
        string content;
        string file_name = "NGAPApiTest.cc";    // ~16k while the buffer is 4k
        int fd = -1;
        CPPUNIT_ASSERT_MESSAGE("The file should open", fd < 0);
        CPPUNIT_ASSERT_MESSAGE("The function should return false", !NgapOwnedContainer::file_to_string(fd, content));
        CPPUNIT_ASSERT_MESSAGE("The string should be empty", content.empty());
    }

    void test_build_dmrpp_url_to_owned_bucket() {
        string rest_path = "collections/C1996541017-GHRC_DAAC/granules/amsua15_2020.028_12915_1139_1324_WI.nc";
        string expected = "https://dmrpp-sit-poc.s3.amazonaws.com/C1996541017-GHRC_DAAC/amsua15_2020.028_12915_1139_1324_WI.nc.dmrpp";
        CPPUNIT_ASSERT_MESSAGE("The URL should be built", NgapOwnedContainer::build_dmrpp_url_to_owned_bucket(rest_path) == expected);
    }

    void test_build_dmrpp_url_to_owned_bucket_bad_path() {
        string rest_path = "collections/C1996541017-GHRC_DAAC/granules/amsua15_2020.028_12915_1139_1324_WI.nc/extra";
        CPPUNIT_ASSERT_THROW_MESSAGE("The function should throw", NgapOwnedContainer::build_dmrpp_url_to_owned_bucket(rest_path), BESSyntaxUserError);
    }

    void test_build_dmrpp_url_to_owned_bucket_bad_path_2() {
        string rest_path = "C1996541017-GHRC_DAAC/granules/amsua15_2020.028_12915_1139_1324_WI.nc";
        CPPUNIT_ASSERT_THROW_MESSAGE("The function should throw", NgapOwnedContainer::build_dmrpp_url_to_owned_bucket(rest_path), BESSyntaxUserError);
    }

    void test_build_dmrpp_url_to_owned_bucket_bad_path_3() {
        string rest_path = "collections/C1996541017-GHRC_DAAC/amsua15_2020.028_12915_1139_1324_WI.nc";
        CPPUNIT_ASSERT_THROW_MESSAGE("The function should throw", NgapOwnedContainer::build_dmrpp_url_to_owned_bucket(rest_path), BESSyntaxUserError);
    }

    void test_build_dmrpp_url_to_owned_bucket_bad_path_4() {
        string rest_path = "C1996541017-GHRC_DAAC/amsua15_2020.028_12915_1139_1324_WI.nc";
        CPPUNIT_ASSERT_THROW_MESSAGE("The function should throw", NgapOwnedContainer::build_dmrpp_url_to_owned_bucket(rest_path), BESSyntaxUserError);
    }
    void test_build_dmrpp_url_to_owned_bucket_bad_path_5() {
        string rest_path = "";
        CPPUNIT_ASSERT_THROW_MESSAGE("The function should throw", NgapOwnedContainer::build_dmrpp_url_to_owned_bucket(rest_path), BESSyntaxUserError);
    }

    void test_item_in_cache() {
        set_bes_keys();
        configure_ngap_handler();
        string dmrpp_string;
        NgapOwnedContainer container;
        container.set_real_name("/data/dmrpp/a2_local_twoD.h5");
        CPPUNIT_ASSERT_MESSAGE("The item should not be in the cache", !container.item_in_cache(dmrpp_string));
        CPPUNIT_ASSERT_MESSAGE("The item should empty", dmrpp_string.empty());
    }

    void test_cache_item() {
        set_bes_keys();
        configure_ngap_handler();
        string dmrpp_string = "cached DMR++";
        NgapOwnedContainer container;
        container.set_real_name("/data/dmrpp/a2_local_twoD.h5");
        CPPUNIT_ASSERT_MESSAGE("The item should be added to the cache", container.cache_item(dmrpp_string));
        string cached_value;
        CPPUNIT_ASSERT_MESSAGE("The item should be in the cache", container.item_in_cache(cached_value));
        CPPUNIT_ASSERT_MESSAGE("The item should be the same", cached_value == dmrpp_string);
    }

#if 0
    void test_inject_data_url_default() {
        NgapContainer container;
        CPPUNIT_ASSERT_MESSAGE("The default value should be false", !container.inject_data_url());
    }

    void test_inject_data_url_set() {
        TheBESKeys::TheKeys()->set_key(NGAP_INJECT_DATA_URL_KEY, "true");
        NgapContainer container;
        CPPUNIT_ASSERT_MESSAGE("The default value should be true", container.inject_data_url());
    }

    void test_get_content_filters_default() {
        TheBESKeys::TheKeys()->set_key(NGAP_INJECT_DATA_URL_KEY, "false"); // clear this to the default
        NgapContainer container;
        map<string, string, std::less<>> content_filters;
        bool do_content_filtering = container.get_content_filters(content_filters);
        CPPUNIT_ASSERT_MESSAGE("The without setting the key, 'do_content_filtering' should be false", !do_content_filtering);
        CPPUNIT_ASSERT_MESSAGE("The content_filters v/r parameter should be empty (unaltered)", content_filters.empty());
    }

    void test_get_content_filters_set() {
        TheBESKeys::TheKeys()->set_key(NGAP_INJECT_DATA_URL_KEY, "true"); // clear this to the default
        map<string, string, std::less<>> content_filters;
        NgapContainer container;
        container.set_real_name("https://foo/bar.h5");

        bool do_content_filtering = container.get_content_filters(content_filters);
        CPPUNIT_ASSERT_MESSAGE("The key is set, 'do_content_filtering' should be tru", do_content_filtering);
        CPPUNIT_ASSERT_MESSAGE("The content_filters v/r parameter should have the filter values", !content_filters.empty());

        if (debug)
            for (auto const &p: content_filters) { cerr << "{" << p.first << ": " << p.second << "}\n"; }

        CPPUNIT_ASSERT_MESSAGE("The content_filters two key/value pairs", content_filters.size() == 2);
        string key_1 = R"(href="OPeNDAP_DMRpp_DATA_ACCESS_URL")";
        CPPUNIT_ASSERT_MESSAGE("The content_filters missing a value", content_filters.find(key_1) != content_filters.end());
        string key_2 = R"(href="OPeNDAP_DMRpp_MISSING_DATA_ACCESS_URL")";
        CPPUNIT_ASSERT_MESSAGE("The content_filters missing a value", content_filters.find(key_2) != content_filters.end());

        CPPUNIT_ASSERT_MESSAGE("The content_filters incorrect value", content_filters[key_1] == R"(href="https://foo/bar.h5" dmrpp:trust="true")");
        CPPUNIT_ASSERT_MESSAGE("The content_filters incorrect value", content_filters[key_2] == R"(href="https://foo/bar.h5.missing" dmrpp:trust="true")");
    }

    // This tests if a map the is not empty is first cleared before values are added.
    void test_get_content_filters_reused_map() {
        TheBESKeys::TheKeys()->set_key(NGAP_INJECT_DATA_URL_KEY, "true"); // clear this to the default
        map<string, string, std::less<>> content_filters;
        // make the content_filters 'used'
        content_filters["foo"] = "bar";
        content_filters["baz"] = "clyde";
        NgapContainer container;
        container.set_real_name("https://foo/bar.h5");

        bool do_content_filtering = container.get_content_filters(content_filters);
        CPPUNIT_ASSERT_MESSAGE("The key is set, 'do_content_filtering' should be true", do_content_filtering);
        CPPUNIT_ASSERT_MESSAGE("The content_filters v/r parameter should have the filter values", !content_filters.empty());

        if (debug)
            for (auto const &p: content_filters) { cerr << "{" << p.first << ": " << p.second << "}\n"; }

        CPPUNIT_ASSERT_MESSAGE("The content_filters two key/value pairs", content_filters.size() == 2);
        string key_1 = R"(href="OPeNDAP_DMRpp_DATA_ACCESS_URL")";
        CPPUNIT_ASSERT_MESSAGE("The content_filters missing a value", content_filters.find(key_1) != content_filters.end());
        string key_2 = R"(href="OPeNDAP_DMRpp_MISSING_DATA_ACCESS_URL")";
        CPPUNIT_ASSERT_MESSAGE("The content_filters missing a value", content_filters.find(key_2) != content_filters.end());

        CPPUNIT_ASSERT_MESSAGE("The content_filters incorrect value", content_filters[key_1] == R"(href="https://foo/bar.h5" dmrpp:trust="true")");
        CPPUNIT_ASSERT_MESSAGE("The content_filters incorrect value", content_filters[key_2] == R"(href="https://foo/bar.h5.missing" dmrpp:trust="true")");
    }

    void test_filter_response() {
        TheBESKeys::TheKeys()->set_key(NGAP_INJECT_DATA_URL_KEY, "true"); // clear this to the default
        map<string, string, std::less<>> content_filters;
        NgapContainer container;
        container.set_real_name("https://foo/bar.h5");

        bool do_content_filtering = container.get_content_filters(content_filters);
        CPPUNIT_ASSERT_MESSAGE("The key is set, 'do_content_filtering' should be true", do_content_filtering);

        string xml_content = R"(<Dataset name="foo" href="OPeNDAP_DMRpp_DATA_ACCESS_URL" href="OPeNDAP_DMRpp_MISSING_DATA_ACCESS_URL">)";
        DBG(cerr << "Before filtering: " << xml_content << '\n');
        container.filter_response(content_filters, xml_content);
        DBG(cerr << "... after filtering: " << xml_content << '\n');
        CPPUNIT_ASSERT_MESSAGE("Value1 not replaced", xml_content.find(R"(href="https://foo/bar.h5" dmrpp:trust="true")") != string::npos);
        CPPUNIT_ASSERT_MESSAGE("Value2 not replaced", xml_content.find(R"(href="https://foo/bar.h5.missing" dmrpp:trust="true")") != string::npos);
    }

    void test_set_real_name_using_cmr_or_cache_using_cmr() {
        const string provider_name = "GHRC_DAAC";
        const string collection_concept_id ="C1996541017-GHRC_DAAC";
        const string granule_name = "amsua15_2020.028_12915_1139_1324_WI.nc";

        const string resty_path = "providers/" + provider_name + "/concepts/" + collection_concept_id + "/granules/" + granule_name;

        set_bes_keys();

        NgapRequestHandler::d_use_cmr_cache = true;

        NgapContainer container;
        container.set_real_name(resty_path);
        container.set_real_name_using_cmr_or_cache();

        const string expected = "https://data.ghrc.earthdata.nasa.gov/ghrcw-protected/amsua15sp__1/amsu-a/noaa-15/data/nc/2020/0128/amsua15_2020.028_12915_1139_1324_WI.nc";
        CPPUNIT_ASSERT_MESSAGE("Expected URL not returned", container.get_real_name() == expected);
    }

    void test_set_real_name_using_cmr_or_cache_using_cache() {
        const string provider_name = "GHRC_DAAC";
        const string collection_concept_id ="C1996541017-GHRC_DAAC";
        const string granule_name = "amsua15_2020.028_12915_1139_1324_WI.nc";

        const string resty_path = "providers/" + provider_name + "/concepts/" + collection_concept_id + "/granules/" + granule_name;

        set_bes_keys();

        NgapRequestHandler::d_use_cmr_cache = true;

        const string uid_value = "bugsbunny";
        BESContextManager::TheManager()->set_context("uid", uid_value);

        // this ctor sets ngap_path, needed by set_real_name_using_cmr_or_cache().
        NgapContainer container("c", resty_path, "ngap");
        container.set_real_name_using_cmr_or_cache();

        const string expected = "https://data.ghrc.earthdata.nasa.gov/ghrcw-protected/amsua15sp__1/amsu-a/noaa-15/data/nc/2020/0128/amsua15_2020.028_12915_1139_1324_WI.nc";
        string cache_value;
        bool found = NgapRequestHandler::d_cmr_mem_cache.get(resty_path + ":" + uid_value, cache_value);

        CPPUNIT_ASSERT_MESSAGE("Expected URL from CMR not cached", found);
        CPPUNIT_ASSERT_MESSAGE("Expected URL from CMR not cached", cache_value == expected);
    }

    void test_set_real_name_using_cmr_or_cache_using_cache_default_ctor() {
        const string provider_name = "GHRC_DAAC";
        const string collection_concept_id ="C1996541017-GHRC_DAAC";
        const string granule_name = "amsua15_2020.028_12915_1139_1324_WI.nc";

        const string resty_path = "providers/" + provider_name + "/concepts/" + collection_concept_id + "/granules/" + granule_name;

        set_bes_keys();

        NgapRequestHandler::d_use_cmr_cache = true;

        const string uid_value = "bugsbunny";
        BESContextManager::TheManager()->set_context("uid", uid_value);

        // this ctor does not set ngap_path, so use the setter.
        NgapContainer container;
        container.set_ngap_path(resty_path);
        container.set_real_name(resty_path);
        container.set_real_name_using_cmr_or_cache();

        const string expected = "https://data.ghrc.earthdata.nasa.gov/ghrcw-protected/amsua15sp__1/amsu-a/noaa-15/data/nc/2020/0128/amsua15_2020.028_12915_1139_1324_WI.nc";
        string cache_value;
        bool found = NgapRequestHandler::d_cmr_mem_cache.get(resty_path + ":" + uid_value, cache_value);

        CPPUNIT_ASSERT_MESSAGE("Expected URL from CMR not cached", found);
        CPPUNIT_ASSERT_MESSAGE("Expected URL from CMR not cached", cache_value == expected);
    }

    void test_access() {
        const string provider_name = "GHRC_DAAC";
        const string collection_concept_id ="C1996541017-GHRC_DAAC";
        const string granule_name = "amsua15_2020.028_12915_1139_1324_WI.nc";

        const string resty_path = "providers/" + provider_name + "/concepts/" + collection_concept_id + "/granules/" + granule_name;

        set_bes_keys();

        NgapRequestHandler::d_use_cmr_cache = true;

        configure_ngap_handler();

        const string uid_value = "bugsbunny";
        BESContextManager::TheManager()->set_context("uid", uid_value);
        const string token_value = "Bearer XYZ";
        BESContextManager::TheManager()->set_context("edl_auth_token", token_value);

        NgapContainer container;
        container.set_ngap_path(resty_path);
        container.set_real_name(resty_path);
        string file_name;
        CPPUNIT_ASSERT_THROW_MESSAGE("Expected NGAP to balk, requiring auth", file_name = container.access(), BESError);
    }

    void test_get_dmrpp_from_cache_or_remote_source() {
        const string real_path = "http://test.opendap.org/opendap/data/dmrpp/a2_local_twoD.h5";

        set_bes_keys();

        configure_ngap_handler();

        NgapContainer container;
        container.set_real_name(real_path);
        string dmrpp_string;
        CPPUNIT_ASSERT_MESSAGE("Getting the DMRPP should work",
                               container.get_dmrpp_from_cache_or_remote_source(dmrpp_string));
        CPPUNIT_ASSERT_MESSAGE("The file name should be set", !dmrpp_string.empty());
        CPPUNIT_ASSERT_MESSAGE("The file name should be the cache file name",
                               dmrpp_string.find("<dmrpp:chunkDimensionSizes>50 50</dmrpp:chunkDimensionSizes>") != string::npos);
    }

    void test_get_dmrpp_from_cache_or_remote_source_twice() {
        const string real_path = "http://test.opendap.org/opendap/data/dmrpp/a2_local_twoD.h5";

        set_bes_keys();

        configure_ngap_handler();

        NgapContainer container;
        container.set_real_name(real_path);
        string dmrpp_string;
        CPPUNIT_ASSERT_MESSAGE("Getting the DMRPP should work",
                               container.get_dmrpp_from_cache_or_remote_source(dmrpp_string));
        CPPUNIT_ASSERT_MESSAGE("The file name should be set", !dmrpp_string.empty());
        CPPUNIT_ASSERT_MESSAGE("The file name should be the cache file name",
                               dmrpp_string.find("<dmrpp:chunkDimensionSizes>50 50</dmrpp:chunkDimensionSizes>") != string::npos);

        CPPUNIT_ASSERT_MESSAGE("Getting the DMRPP should work",
                               container.get_dmrpp_from_cache_or_remote_source(dmrpp_string));
        CPPUNIT_ASSERT_MESSAGE("The file name should be set", !dmrpp_string.empty());
        CPPUNIT_ASSERT_MESSAGE("The file name should be the cache file name",
                               dmrpp_string.find("<dmrpp:chunkDimensionSizes>50 50</dmrpp:chunkDimensionSizes>") != string::npos);
    }

    void test_get_dmrpp_from_cache_or_remote_source_two_containers() {
        const string real_path = "http://test.opendap.org/opendap/data/dmrpp/a2_local_twoD.h5";

        set_bes_keys();

        configure_ngap_handler();

        NgapContainer container;
        container.set_real_name(real_path);
        string dmrpp_string;
        CPPUNIT_ASSERT_MESSAGE("Getting the DMRPP should work",
                               container.get_dmrpp_from_cache_or_remote_source(dmrpp_string));
        CPPUNIT_ASSERT_MESSAGE("The file name should be set", !dmrpp_string.empty());
        CPPUNIT_ASSERT_MESSAGE("The file name should be the cache file name",
                               dmrpp_string.find("<dmrpp:chunkDimensionSizes>50 50</dmrpp:chunkDimensionSizes>") != string::npos);

        NgapContainer container2;
        container2.set_real_name(real_path);
        string dmrpp_string2;
        CPPUNIT_ASSERT_MESSAGE("Getting the DMRPP should work",
                               container2.get_dmrpp_from_cache_or_remote_source(dmrpp_string2));
        CPPUNIT_ASSERT_MESSAGE("The file name should be set", !dmrpp_string2.empty());
        CPPUNIT_ASSERT_MESSAGE("The file name should be the cache file name",
                               dmrpp_string2.find("<dmrpp:chunkDimensionSizes>50 50</dmrpp:chunkDimensionSizes>") != string::npos);
    }

    void test_get_dmrpp_from_cache_or_remote_source_clean_mem_cache() {
        const string real_path = "http://test.opendap.org/opendap/data/dmrpp/a2_local_twoD.h5";

        set_bes_keys();

        configure_ngap_handler();

        NgapContainer container;
        container.set_real_name(real_path);
        string dmrpp_string;
        CPPUNIT_ASSERT_MESSAGE("Getting the DMRPP should work",
                               container.get_dmrpp_from_cache_or_remote_source(dmrpp_string));
        CPPUNIT_ASSERT_MESSAGE("The file name should be set", !dmrpp_string.empty());
        CPPUNIT_ASSERT_MESSAGE("The file name should be the cache file name",
                               dmrpp_string.find("<dmrpp:chunkDimensionSizes>50 50</dmrpp:chunkDimensionSizes>") != string::npos);

        NgapRequestHandler::d_dmrpp_mem_cache.clear();

        NgapContainer container2;
        container2.set_real_name(real_path);
        string dmrpp_string2;
        CPPUNIT_ASSERT_MESSAGE("Getting the DMRPP should work",
                               container2.get_dmrpp_from_cache_or_remote_source(dmrpp_string2));
        CPPUNIT_ASSERT_MESSAGE("The file name should be set", !dmrpp_string2.empty());
        CPPUNIT_ASSERT_MESSAGE("The file name should be the cache file name",
                               dmrpp_string2.find("<dmrpp:chunkDimensionSizes>50 50</dmrpp:chunkDimensionSizes>") != string::npos);
    }

    // This version tests that when a DMR++ is not in the mem cache and is pulled
    // from the file cache, it is put in the memory cache so the next access will
    // find it in the mem cache
    void test_get_dmrpp_from_cache_or_remote_source_clean_mem_cache_2() {
        const string real_path = "http://test.opendap.org/opendap/data/dmrpp/a2_local_twoD.h5";

        set_bes_keys();


        NgapContainer container;
        container.set_real_name(real_path);
        string dmrpp_string;
        CPPUNIT_ASSERT_MESSAGE("Getting the DMRPP should work",
                               container.get_dmrpp_from_cache_or_remote_source(dmrpp_string));
        CPPUNIT_ASSERT_MESSAGE("The file name should be set", !dmrpp_string.empty());
        CPPUNIT_ASSERT_MESSAGE("The file name should be the cache file name",
                               dmrpp_string.find("<dmrpp:chunkDimensionSizes>50 50</dmrpp:chunkDimensionSizes>") != string::npos);

        NgapRequestHandler::d_dmrpp_mem_cache.clear();

        NgapContainer container2;
        container2.set_real_name(real_path);
        string dmrpp_string2;
        CPPUNIT_ASSERT_MESSAGE("Getting the DMRPP should work",
                               container2.get_dmrpp_from_cache_or_remote_source(dmrpp_string2));
        CPPUNIT_ASSERT_MESSAGE("The file name should be set", !dmrpp_string2.empty());
        CPPUNIT_ASSERT_MESSAGE("The file name should be the cache file name",
                               dmrpp_string2.find("<dmrpp:chunkDimensionSizes>50 50</dmrpp:chunkDimensionSizes>") != string::npos);

        NgapContainer container3;
        container3.set_real_name(real_path);
        string dmrpp_string3;
        CPPUNIT_ASSERT_MESSAGE("Getting the DMRPP should work",
                               container3.get_dmrpp_from_cache_or_remote_source(dmrpp_string3));
        CPPUNIT_ASSERT_MESSAGE("The file name should be set", !dmrpp_string3.empty());
        CPPUNIT_ASSERT_MESSAGE("The file name should be the cache file name",
                               dmrpp_string3.find("<dmrpp:chunkDimensionSizes>50 50</dmrpp:chunkDimensionSizes>") != string::npos);
    }

    void test_get_dmrpp_from_cache_or_remote_source_twice_no_cache() {
        const string real_path = "http://test.opendap.org/opendap/data/dmrpp/a2_local_twoD.h5";

        set_bes_keys();

        NgapRequestHandler::d_use_dmrpp_cache = false;

        NgapContainer container;
        container.set_real_name(real_path);
        string dmrpp_string;
        CPPUNIT_ASSERT_MESSAGE("Getting the DMRPP should work",
                               container.get_dmrpp_from_cache_or_remote_source(dmrpp_string));
        CPPUNIT_ASSERT_MESSAGE("The file name should be set", !dmrpp_string.empty());
        CPPUNIT_ASSERT_MESSAGE("The file name should be the cache file name",
                               dmrpp_string.find("<dmrpp:chunkDimensionSizes>50 50</dmrpp:chunkDimensionSizes>") != string::npos);

        CPPUNIT_ASSERT_MESSAGE("Getting the DMRPP should work",
                               container.get_dmrpp_from_cache_or_remote_source(dmrpp_string));
        CPPUNIT_ASSERT_MESSAGE("The file name should be set", !dmrpp_string.empty());
        CPPUNIT_ASSERT_MESSAGE("The file name should be the cache file name",
                               dmrpp_string.find("<dmrpp:chunkDimensionSizes>50 50</dmrpp:chunkDimensionSizes>") != string::npos);
    }
#endif

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



#if 0
    CPPUNIT_TEST(test_inject_data_url_default);
    CPPUNIT_TEST(test_inject_data_url_set);
    CPPUNIT_TEST(test_get_content_filters_default);
    CPPUNIT_TEST(test_get_content_filters_set);
    CPPUNIT_TEST(test_get_content_filters_reused_map);
    CPPUNIT_TEST(test_filter_response);

    CPPUNIT_TEST(test_set_real_name_using_cmr_or_cache_using_cmr);
    CPPUNIT_TEST(test_set_real_name_using_cmr_or_cache_using_cache);
    CPPUNIT_TEST(test_set_real_name_using_cmr_or_cache_using_cache_default_ctor);

    CPPUNIT_TEST(test_access);
    CPPUNIT_TEST(test_get_dmrpp_from_cache_or_remote_source);
    CPPUNIT_TEST(test_get_dmrpp_from_cache_or_remote_source_twice);
    CPPUNIT_TEST(test_get_dmrpp_from_cache_or_remote_source_two_containers);
    CPPUNIT_TEST(test_get_dmrpp_from_cache_or_remote_source_twice_no_cache);
    CPPUNIT_TEST(test_get_dmrpp_from_cache_or_remote_source_clean_mem_cache_2);
    CPPUNIT_TEST(test_get_dmrpp_from_cache_or_remote_source_clean_mem_cache);
#endif

    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(NgapOwnedContainerTest);

} // namespace dmrpp

int main(int argc, char*argv[])
{
    bool status = bes_run_tests<ngap::NgapOwnedContainerTest>(argc, argv, "cerr,ngap,cache");

    return status ? 0 : 1;
}
