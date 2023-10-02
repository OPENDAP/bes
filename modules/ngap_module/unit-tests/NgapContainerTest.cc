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

#include <libdap/util.h>

#include "BESUtil.h"
#include "TheBESKeys.h"
#include "BESContextManager.h"

#include "NgapNames.h"
#include "NgapContainer.h"

#include "test_config.h"

#include "run_tests_cppunit.h"

using namespace std;

#define prolog string("NgapContainerTest::").append(__func__).append("() - ")

namespace ngap {

class NgapContainerTest: public CppUnit::TestFixture {

public:
    // Called once before everything gets tested
    NgapContainerTest() = default;
    ~NgapContainerTest() override = default;
    NgapContainerTest(const NgapContainerTest &src) = delete;
    const NgapContainerTest &operator=(const NgapContainerTest & rhs) = delete;

    // setUp; Called before each test; not used.

    // tearDown; Called after each test; not used.

    void test_inject_data_url_default() {
        NgapContainer container;
        CPPUNIT_ASSERT_MESSAGE("The default value should be false", !container.inject_data_url());
    }

    void test_inject_data_url_set() {
        NgapContainer container;
        TheBESKeys::TheKeys()->set_key(NGAP_INJECT_DATA_URL_KEY, "true");
        CPPUNIT_ASSERT_MESSAGE("The default value should be true", container.inject_data_url());
    }

    void test_get_content_filters_default() {
        TheBESKeys::TheKeys()->set_key(NGAP_INJECT_DATA_URL_KEY, "false"); // clear this to the default
        map<string, string, std::less<>> content_filters;
        NgapContainer container;
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

        TheBESKeys::TheKeys()->set_key("BES.LogName", "./bes.log");
        TheBESKeys::TheKeys()->set_key("BES.Catalog.catalog.RootDirectory", "/tmp"); // any dir that exists will do
        TheBESKeys::TheKeys()->set_key("BES.Catalog.catalog.TypeMatch", "any-value:will-do");
        TheBESKeys::TheKeys()->set_key("AllowedHosts", ".*");

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

        TheBESKeys::TheKeys()->set_key("BES.LogName", "./bes.log");
        TheBESKeys::TheKeys()->set_key("BES.Catalog.catalog.RootDirectory", "/tmp"); // any dir that exists will do
        TheBESKeys::TheKeys()->set_key("BES.Catalog.catalog.TypeMatch", "any-value:will-do");
        TheBESKeys::TheKeys()->set_key("AllowedHosts", ".*");

        const string uid_value = "jhrguat";
        BESContextManager::TheManager()->set_context("uid", uid_value);

        // this ctor sets ngap_path, needed by set_real_name_using_cmr_or_cache().
        NgapContainer container("c", resty_path, "ngap");
        container.set_real_name_using_cmr_or_cache();

        const string expected = "https://data.ghrc.earthdata.nasa.gov/ghrcw-protected/amsua15sp__1/amsu-a/noaa-15/data/nc/2020/0128/amsua15_2020.028_12915_1139_1324_WI.nc";
        string cache_value;
        bool found = container.get_cmr_cache(resty_path + "." + uid_value, cache_value);

        CPPUNIT_ASSERT_MESSAGE("Expected URL from CMR not cached", found);
        CPPUNIT_ASSERT_MESSAGE("Expected URL from CMR not cached", cache_value == expected);
    }

    void test_set_real_name_using_cmr_or_cache_using_cache_default_ctor() {
        const string provider_name = "GHRC_DAAC";
        const string collection_concept_id ="C1996541017-GHRC_DAAC";
        const string granule_name = "amsua15_2020.028_12915_1139_1324_WI.nc";

        const string resty_path = "providers/" + provider_name + "/concepts/" + collection_concept_id + "/granules/" + granule_name;

        TheBESKeys::TheKeys()->set_key("BES.LogName", "./bes.log");
        TheBESKeys::TheKeys()->set_key("BES.Catalog.catalog.RootDirectory", "/tmp"); // any dir that exists will do
        TheBESKeys::TheKeys()->set_key("BES.Catalog.catalog.TypeMatch", "any-value:will-do");
        TheBESKeys::TheKeys()->set_key("AllowedHosts", ".*");

        const string uid_value = "jhrguat";
        BESContextManager::TheManager()->set_context("uid", uid_value);

        // this ctor does not set ngap_path, so use the setter.
        NgapContainer container;
        container.set_ngap_path(resty_path);
        container.set_real_name(resty_path);
        container.set_real_name_using_cmr_or_cache();

        const string expected = "https://data.ghrc.earthdata.nasa.gov/ghrcw-protected/amsua15sp__1/amsu-a/noaa-15/data/nc/2020/0128/amsua15_2020.028_12915_1139_1324_WI.nc";
        string cache_value;
        bool found = container.get_cmr_cache(resty_path + "." + uid_value, cache_value);

        CPPUNIT_ASSERT_MESSAGE("Expected URL from CMR not cached", found);
        CPPUNIT_ASSERT_MESSAGE("Expected URL from CMR not cached", cache_value == expected);
    }

    CPPUNIT_TEST_SUITE( NgapContainerTest );

    CPPUNIT_TEST(test_inject_data_url_default);
    CPPUNIT_TEST(test_inject_data_url_set);
    CPPUNIT_TEST(test_get_content_filters_default);
    CPPUNIT_TEST(test_get_content_filters_set);
    CPPUNIT_TEST(test_get_content_filters_reused_map);
    CPPUNIT_TEST(test_filter_response);

    CPPUNIT_TEST(test_set_real_name_using_cmr_or_cache_using_cmr);
    CPPUNIT_TEST(test_set_real_name_using_cmr_or_cache_using_cache);
    CPPUNIT_TEST(test_set_real_name_using_cmr_or_cache_using_cache_default_ctor);

    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(NgapContainerTest);

} // namespace dmrpp


int main(int argc, char*argv[])
{
    bool status = bes_run_tests<ngap::NgapContainerTest>(argc, argv, "cerr,ngap,cache");

    return status ? 0 : 1;
}
