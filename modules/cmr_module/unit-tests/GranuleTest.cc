// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of the BES component of the Hyrax Data Server.

// Copyright (c) 2018 OPeNDAP, Inc.
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

#include "test_config.h"

#include <memory>
#include <cstdio>
#include <cstring>
#include <iostream>

#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>

#include <unistd.h>
#include <libdap/util.h>

#include <BESError.h>
#include <BESDebug.h>
#include <BESUtil.h>
#include <TheBESKeys.h>
#include <BESCatalogList.h>
#include <CatalogNode.h>

#include "CmrNames.h"
#include "CmrApi.h"
#include "CmrCatalog.h"

#include "run_tests_cppunit.h"

using namespace std;

namespace cmr {

class GranuleTest: public CppUnit::TestFixture {
    const string ges_disc_collection_name_ = "C1276812863-GES_DISC";

public:
    // Called once before everything gets tested
    GranuleTest() = default;

    // Called at the end of the test
    ~GranuleTest() override = default;

    // Called before each test
    void setUp() override
    {
        string bes_conf = BESUtil::assemblePath(TEST_BUILD_DIR,"bes.conf");
        TheBESKeys::ConfigFile = bes_conf;
        BESCatalogList::TheCatalogList()->add_catalog(new cmr::CmrCatalog(CMR_CATALOG_NAME));

        if (debug2) show_file(bes_conf);
    }

    // Called after each test
    void tearDown() override {
    }

    void get_years_test() {
        // Just look at the first five nodes. jhrg 10/17/25

        try {
            const cmr::CmrCatalog catalog;
            bes::CatalogNode *node = catalog.get_node("/GES_DISC/" + ges_disc_collection_name_);
            const unsigned lcount = node->get_leaf_count();
            CPPUNIT_ASSERT_MESSAGE("Checking expected leaves (0) vs received (" + to_string(lcount) + ")",
                lcount == 0);

            const unsigned expected_size = 46;
            const unsigned ncount = node->get_node_count();
            CPPUNIT_ASSERT_MESSAGE("Checking expected nodes (" + to_string(expected_size) + ") vs received (" + to_string(ncount) + ")",
                ncount >= expected_size);

            // Here we know there are at least 46 values in node. jhrg 10/17/25
            const vector<string> expected = { "1980", "1981", "1982", "1983","1984" };
            auto itr = node->nodes_begin();
            for (const auto & i : expected) {
                string node_name = (*itr)->get_name();
                CPPUNIT_ASSERT_MESSAGE("Checking:  expected: " + i + " received: " + node_name,
                    i == node_name);
                ++itr;
            }
        }
        catch (BESError &be) {
            string msg = "Caught BESError! Message: " + be.get_message();
            cerr << endl << msg << endl;
            CPPUNIT_ASSERT(!"Caught BESError");
        }
    }

    void get_months_test() {
        vector<string> expected = {
            "01",
            "02",
            "03",
            "04",
            "05",
            "06",
            "07",
            "08",
            "09",
            "10",
            "11",
            "12"
        };
        try {
            const unsigned expected_size = 12;
            cmr::CmrCatalog catalog;
            bes::CatalogNode *node = catalog.get_node("/GES_DISC/" + ges_disc_collection_name_ + "/1985");
            const unsigned lcount = node->get_leaf_count();
            CPPUNIT_ASSERT_MESSAGE("Checking expected leaves (0) vs received (" + to_string(lcount) + ")",
                lcount == 0);

            const unsigned ncount = node->get_node_count();
            CPPUNIT_ASSERT_MESSAGE("Checking expected nodes (" + to_string(expected_size) + ") vs received (" + to_string(ncount) + ")",
                ncount == expected_size);

            bes::CatalogNode::item_iter itr;
            size_t i=0;
            for (itr=node->nodes_begin(); itr!=node->nodes_end(); itr++, i++){
                string node_name = (*itr)->get_name();
                CPPUNIT_ASSERT_MESSAGE("Checking:  expected: " + expected[i] + " received: " + node_name,
                    expected[i] == node_name);
            }

        }
        catch (const BESError &be) {
            CPPUNIT_FAIL("Caught BESError! Message: " + be.get_message());
        }

    }

    void get_days_test() {
        string prolog = string(__func__) + "() - ";
        string node_path = "GES_DISC/" + ges_disc_collection_name_ + "/1985/03";
        vector<string> expected = {
                "01","02","03","04","05","06","07","08","09","10",
                "11","12","13","14","15","16","17","18","19","20",
                "21","22","23","24","25","26","27","28","29","30",
                "31"
        };
        unsigned long  expected_size = 31;
        try {
            cmr::CmrCatalog catalog;
            bes::CatalogNode *node = catalog.get_node(node_path);
            const unsigned lcount = node->get_leaf_count();
            CPPUNIT_ASSERT_MESSAGE("Checking expected leaves (0) vs received (" + to_string(lcount) + ")",
                lcount == 0);

            const unsigned ncount = node->get_node_count();
            CPPUNIT_ASSERT_MESSAGE("Checking expected nodes (" + to_string(expected_size) + ") vs received (" + to_string(ncount) + ")",
                ncount == expected_size);

            bes::CatalogNode::item_iter itr;
            size_t i=0;
            for (itr=node->nodes_begin(); itr!=node->nodes_end(); itr++, i++){
                string node_name = (*itr)->get_name();
                CPPUNIT_ASSERT_MESSAGE("Checking:  expected: " + expected[i] + " received: " + node_name,
                    expected[i] == node_name);
            }


        }
        catch (const BESError &be) {
            CPPUNIT_FAIL("Caught BESError! Message: " + be.get_message());
        }

    }


    void get_granules_test() {
        const string node_path = "GES_DISC/" + ges_disc_collection_name_ + "/1985/03/13";
        vector<string> expected = {
                "M2T1NXSLV.5.12.4:MERRA2_100.tavg1_2d_slv_Nx.19850313.nc4"
        };
        vector<string> days;
        try {
            cmr::CmrCatalog catalog;
            bes::CatalogNode *node = catalog.get_node(node_path);

            const unsigned expected_size = 1;
            const unsigned lcount = node->get_leaf_count();
            CPPUNIT_ASSERT_MESSAGE("Checking expected leaves (" + to_string(expected_size) + ") vs received (" + to_string(lcount) + ")",
                lcount == expected_size);

            const unsigned ncount = node->get_node_count();
            CPPUNIT_ASSERT_MESSAGE("Checking expected nodes (0) vs received (" + to_string(ncount) + ")",
                ncount == 0);

            bes::CatalogNode::item_iter itr;
            size_t i=0;
            for (itr=node->leaves_begin(); itr!=node->leaves_end(); itr++, i++){
                string leaf_name = (*itr)->get_name();
                CPPUNIT_ASSERT_MESSAGE("Checking:  expected: " + expected[i] + " received: " + leaf_name,
                    expected[i] == leaf_name);
            }
        }
        catch (const BESError &be) {
            CPPUNIT_FAIL("Caught BESError! Message: " + be.get_message());
        }
    }


    CPPUNIT_TEST_SUITE( GranuleTest );

    CPPUNIT_TEST(get_years_test);
    CPPUNIT_TEST(get_months_test);
    CPPUNIT_TEST(get_days_test);
    CPPUNIT_TEST(get_granules_test);

    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(GranuleTest);

} // namespace dmrpp

int main(int argc, char*argv[]) {
    return bes_run_tests<cmr::GranuleTest>(argc, argv, "cerr,cmr") ? 0 : 1;
}
