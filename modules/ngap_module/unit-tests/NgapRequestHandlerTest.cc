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
#include "NgapRequestHandler.h"

#include "test_config.h"

#include "run_tests_cppunit.h"

using namespace std;

#define prolog string("NgapRequestHandlerTest::").append(__func__).append("() - ")

namespace ngap {

class NgapRequestHandlerTest: public CppUnit::TestFixture {
    unique_ptr<NgapRequestHandler> d_rh = make_unique<NgapRequestHandler>("test");

public:
    // Called once before everything gets tested
    NgapRequestHandlerTest() = default;
    ~NgapRequestHandlerTest() override = default;
    NgapRequestHandlerTest(const NgapRequestHandlerTest &src) = delete;
    const NgapRequestHandlerTest &operator=(const NgapRequestHandlerTest & rhs) = delete;

    // setUp; Called before each test; not used.

    // tearDown; Called after each test; not used.

    void test_compiled_default_cache_keys() {
        // Test the values without any conf file loaded.
        CPPUNIT_ASSERT_MESSAGE("Expected the CMR cache to not be used by default", !NgapRequestHandler::d_use_cmr_cache);
        CPPUNIT_ASSERT_MESSAGE("Expected the DMR++ cache to not be used by default", !NgapRequestHandler::d_use_dmrpp_cache);

        CPPUNIT_ASSERT_MESSAGE("Expected the CMR cache threshold to be 100", NgapRequestHandler::d_cmr_cache_size_items == 100);
        CPPUNIT_ASSERT_MESSAGE("Expected the DMR++ cache threshold to be 100", NgapRequestHandler::d_dmrpp_mem_cache_size_items == 100);
        CPPUNIT_ASSERT_MESSAGE("Expected the CMR cache space to be 20", NgapRequestHandler::d_cmr_cache_purge_items == 20);
        CPPUNIT_ASSERT_MESSAGE("Expected the DMR++ cache space to be 20", NgapRequestHandler::d_dmrpp_mem_cache_purge_items == 20);
    }

    void test_cmr_cache_disabled() {
        TheBESKeys::TheKeys()->reload_keys(BESUtil::assemblePath(TEST_BUILD_DIR, "bes.cache.conf"));

        TheBESKeys::TheKeys()->set_key("NGAP.UseCMRCache", "false");

        d_rh = make_unique<NgapRequestHandler>("test_no_cmr_cache");

        CPPUNIT_ASSERT_MESSAGE("Expected the CMR cache to be false", !NgapRequestHandler::d_use_cmr_cache);
        CPPUNIT_ASSERT_MESSAGE("Expected the DMR++ cache to be used by default", NgapRequestHandler::d_use_dmrpp_cache);

        CPPUNIT_ASSERT_MESSAGE("Expected the CMR cache threshold to be 100", NgapRequestHandler::d_cmr_cache_size_items == 100);
        CPPUNIT_ASSERT_MESSAGE("Expected the DMR++ cache threshold to be 100", NgapRequestHandler::d_dmrpp_mem_cache_size_items == 100);
        CPPUNIT_ASSERT_MESSAGE("Expected the CMR cache space to be 20", NgapRequestHandler::d_cmr_cache_purge_items == 20);
        CPPUNIT_ASSERT_MESSAGE("Expected the DMR++ cache space to be 20", NgapRequestHandler::d_dmrpp_mem_cache_purge_items == 20);
    }

    void test_dmrpp_cache_disabled() {
        TheBESKeys::TheKeys()->reload_keys(BESUtil::assemblePath(TEST_BUILD_DIR, "bes.cache.conf"));
        TheBESKeys::TheKeys()->set_key("NGAP.UseDMRppCache", "false");  // NGAP.UseDMRppCache

        d_rh = make_unique<NgapRequestHandler>("test_no_dmrpp_cache");

        CPPUNIT_ASSERT_MESSAGE("Expected the CMR cache to be used by default", NgapRequestHandler::d_use_cmr_cache);
        CPPUNIT_ASSERT_MESSAGE("Expected the DMR++ cache to be false", !NgapRequestHandler::d_use_dmrpp_cache);

        // These will still have the default values
        CPPUNIT_ASSERT_MESSAGE("Expected the CMR cache threshold to be 100", NgapRequestHandler::d_cmr_cache_size_items == 100);
        CPPUNIT_ASSERT_MESSAGE("Expected the DMR++ cache threshold to be 100", NgapRequestHandler::d_dmrpp_mem_cache_size_items == 100);
        CPPUNIT_ASSERT_MESSAGE("Expected the CMR cache space to be 20", NgapRequestHandler::d_cmr_cache_purge_items == 20);
        CPPUNIT_ASSERT_MESSAGE("Expected the DMR++ cache space to be 20", NgapRequestHandler::d_dmrpp_mem_cache_purge_items == 20);
    }

    void test_custom_cache_params() {
        TheBESKeys::TheKeys()->reload_keys(BESUtil::assemblePath(TEST_BUILD_DIR, "bes.cache.conf"));

        TheBESKeys::TheKeys()->set_key("NGAP.CMRCacheSize.Items", "17");
        TheBESKeys::TheKeys()->set_key("NGAP.CMRCachePurge.Items", "7");

        TheBESKeys::TheKeys()->set_key("NGAP.DMRppCacheSize.Items", "17");
        TheBESKeys::TheKeys()->set_key("NGAP.DMRppCachePurge.Items", "7");

        d_rh = make_unique<NgapRequestHandler>("test_no_dmrpp_cache");

        CPPUNIT_ASSERT_MESSAGE("Expected the CMR cache to be used by default", NgapRequestHandler::d_use_cmr_cache);
        CPPUNIT_ASSERT_MESSAGE("Expected the DMR++ cache to be used by default", NgapRequestHandler::d_use_dmrpp_cache);

        // These will still have the default values
        CPPUNIT_ASSERT_MESSAGE("Expected the CMR cache threshold to be 17", NgapRequestHandler::d_cmr_cache_size_items == 17);
        CPPUNIT_ASSERT_MESSAGE("Expected the DMR++ cache threshold to be 17", NgapRequestHandler::d_dmrpp_mem_cache_size_items == 17);
        CPPUNIT_ASSERT_MESSAGE("Expected the CMR cache space to be 7", NgapRequestHandler::d_cmr_cache_purge_items == 7);
        CPPUNIT_ASSERT_MESSAGE("Expected the DMR++ cache space to be 7", NgapRequestHandler::d_dmrpp_mem_cache_purge_items == 7);
    }

    CPPUNIT_TEST_SUITE( NgapRequestHandlerTest );

        CPPUNIT_TEST(test_compiled_default_cache_keys);

        CPPUNIT_TEST(test_dmrpp_cache_disabled);
        CPPUNIT_TEST(test_cmr_cache_disabled);

        CPPUNIT_TEST(test_custom_cache_params);

    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(NgapRequestHandlerTest);

} // namespace dmrpp


int main(int argc, char*argv[])
{
    bool status = bes_run_tests<ngap::NgapRequestHandlerTest>(argc, argv, "cerr,ngap,cache");

    return status ? 0 : 1;
}
