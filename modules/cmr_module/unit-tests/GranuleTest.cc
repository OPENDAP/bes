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
#include <BESNotFoundError.h>
#include <BESDebug.h>
#include <BESUtil.h>
#include <TheBESKeys.h>
#include <BESCatalogList.h>
#include <CatalogNode.h>

#include "CmrNames.h"
#include "CmrApi.h"
#include "CmrCatalog.h"
#include "CmrInternalError.h"

using namespace std;

static bool debug = false;
static bool bes_debug = false;

#undef DBG
#define DBG(x) do { if (debug) x; } while(false)

namespace cmr {

class GranuleTest: public CppUnit::TestFixture {
private:
    const string ges_disc_collection_name_ = "C179003030-ORNL_DAAC";

    // char curl_error_buf[CURL_ERROR_SIZE];

    void show_file(string filename)
    {
        ifstream t(filename.c_str());

        if (t.is_open()) {
            string file_content((istreambuf_iterator<char>(t)), istreambuf_iterator<char>());
            t.close();
            cout << endl << "##################################################################" << endl;
            cout << "file: " << filename << endl;
            cout << ". . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . " << endl;
            cout << file_content << endl;
            cout << ". . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . " << endl;
        }
    }

public:
    // Called once before everything gets tested
    GranuleTest()
    {
    }

    // Called at the end of the test
    ~GranuleTest()
    {
    }

    // Called before each test
    void setUp()
    {
        string bes_conf = BESUtil::assemblePath(TEST_BUILD_DIR,"bes.conf");
        TheBESKeys::ConfigFile = bes_conf;
        BESCatalogList::TheCatalogList()->add_catalog(new cmr::CmrCatalog(CMR_CATALOG_NAME));

        if (bes_debug) BESDebug::SetUp("cerr,cmr");

        if (bes_debug) show_file(bes_conf);
    }

    // Called after each test
    void tearDown()
    {
    }


    void get_years_test() {
        string prolog = string(__func__) + "() - ";
        string expected[] = { string("1984"), string("1985"), string("1986"), string("1987"), string("1988") };
        unsigned long  expected_size = 5;
        stringstream msg;

        try {
            cmr::CmrCatalog catalog;
            bes::CatalogNode *node = catalog.get_node("ORNL_DAAC/C179003030-ORNL_DAAC");
            unsigned lcount = node->get_leaf_count();
            BESDEBUG(MODULE, prolog << "Checking expected leaves (0) vs received (" << lcount << ")" << endl);
            CPPUNIT_ASSERT( lcount==0 );

            unsigned ncount = node->get_node_count();
            BESDEBUG(MODULE, prolog << "Checking expected nodes ("<< expected_size << ") vs received (" << ncount << ")" << endl);
            CPPUNIT_ASSERT( ncount==expected_size );

            bes::CatalogNode::item_iter itr;
            size_t i=0;
            for (itr=node->nodes_begin(); itr!=node->nodes_end(); itr++, i++){
                string node_name = (*itr)->get_name();
                msg.str(std::string());
                msg << prolog << "Checking:  expected: " << expected[i]
                        << " received: " << node_name;
                BESDEBUG(MODULE, msg.str() << endl);
                CPPUNIT_ASSERT(expected[i] == node_name);
            }
        }
        catch (BESError &be) {
            string msg = "Caught BESError! Message: " + be.get_message();
            cerr << endl << msg << endl;
            CPPUNIT_ASSERT(!"Caught BESError");
        }
    }

    void get_months_test() {
        string prolog = string(__func__) + "() - ";
        stringstream msg;

        string collection_name = "C179003030-ORNL_DAAC";
        string expected[] = {
                string("01"),
                string("02"),
                string("03"),
                string("04"),
                string("05"),
                string("06"),
                string("07"),
                string("08"),
                string("09"),
                string("10"),
                string("11"),
                string("12") };
        unsigned long  expected_size = 12;
        vector<string> months;
        try {
            cmr::CmrCatalog catalog;
            bes::CatalogNode *node = catalog.get_node("/ORNL_DAAC/" +collection_name +"/1985");
            unsigned lcount = node->get_leaf_count();
            BESDEBUG(MODULE, prolog << "Checking expected leaves (0) vs received (" << lcount << ")" << endl);
            CPPUNIT_ASSERT( lcount==0 );

            unsigned ncount = node->get_node_count();
            BESDEBUG(MODULE, prolog << "Checking expected nodes ("<< expected_size << ") vs received (" << ncount << ")" << endl);
            CPPUNIT_ASSERT( ncount==expected_size );

            bes::CatalogNode::item_iter itr;
            size_t i=0;
            for (itr=node->nodes_begin(); itr!=node->nodes_end(); itr++, i++){
                string node_name = (*itr)->get_name();
                msg.str(std::string());
                msg << prolog << "Checking:  expected: " << expected[i]
                        << " received: " << node_name;
                BESDEBUG(MODULE, msg.str() << endl);
                CPPUNIT_ASSERT(expected[i] == node_name);
            }

        }
        catch (BESError &be) {
            string msg = "Caught BESError! Message: " + be.get_message();
            cerr << endl << msg << endl;
            CPPUNIT_ASSERT(!"Caught BESError");
        }

    }

    void get_days_test() {
        string prolog = string(__func__) + "() - ";
        stringstream msg;

        //string collection_name = "C179003030-ORNL_DAAC";
        //string collection_name = "C1276812863-GES_DISC";
        string node_path="GES_DISC/" + ges_disc_collection_name_ + "/1985/03";
        string expected[] = {
                string("01"),string("02"),string("03"),string("04"),string("05"),string("06"),string("07"),string("08"),string("09"),string("10"),
                string("11"),string("12"),string("13"),string("14"),string("15"),string("16"),string("17"),string("18"),string("19"),string("20"),
                string("21"),string("22"),string("23"),string("24"),string("25"),string("26"),string("27"),string("28"),string("29"),string("30"),
                string("31")
        };
        unsigned long  expected_size = 31;
        vector<string> days;
        try {
            cmr::CmrCatalog catalog;
            bes::CatalogNode *node = catalog.get_node(node_path);
            unsigned lcount = node->get_leaf_count();
            BESDEBUG(MODULE, prolog << "Checking expected leaves (0) vs received (" << lcount << ")" << endl);
            CPPUNIT_ASSERT( lcount==0 );

            unsigned ncount = node->get_node_count();
            BESDEBUG(MODULE, prolog << "Checking expected nodes ("<< expected_size << ") vs received (" << ncount << ")" << endl);
            CPPUNIT_ASSERT( ncount==expected_size );

            bes::CatalogNode::item_iter itr;
            size_t i=0;
            for (itr=node->nodes_begin(); itr!=node->nodes_end(); itr++, i++){
                string node_name = (*itr)->get_name();
                msg.str(std::string());
                msg << prolog << "Checking:  expected: " << expected[i]
                        << " received: " << node_name;
                BESDEBUG(MODULE, msg.str() << endl);
                CPPUNIT_ASSERT(expected[i] == node_name);
            }


        }
        catch (BESError &be) {
            string msg = "Caught BESError! Message: " + be.get_message();
            cerr << endl << msg << endl;
            CPPUNIT_ASSERT(!"Caught BESError");
        }

    }


    void get_granules_test() {
        string prolog = string(__func__) + "() - ";
        stringstream msg;

        //string collection_name = "C179003030-ORNL_DAAC";
        //string collection_name = "C1276812863-GES_DISC";
        string node_path="GES_DISC/" + ges_disc_collection_name_ + "/1985/03/13";
        string expected[] = {
                string("M2T1NXSLV.5.12.4:MERRA2_100.tavg1_2d_slv_Nx.19850313.nc4")
        };
        unsigned long  expected_size = 1;
        vector<string> days;
        try {
            cmr::CmrCatalog catalog;
            bes::CatalogNode *node = catalog.get_node(node_path);

            unsigned lcount = node->get_leaf_count();
            BESDEBUG(MODULE, prolog << "Checking expected leaves (" << expected_size << ") vs received (" << lcount << ")" << endl);
            CPPUNIT_ASSERT( lcount==expected_size );

            unsigned ncount = node->get_node_count();
            BESDEBUG(MODULE, prolog << "Checking expected nodes (0) vs received (" << ncount << ")" << endl);
            CPPUNIT_ASSERT( ncount==0 );

            bes::CatalogNode::item_iter itr;
            size_t i=0;
            for (itr=node->leaves_begin(); itr!=node->leaves_end(); itr++, i++){
                string leaf_name = (*itr)->get_name();
                msg.str(std::string());
                msg << prolog << "Checking:  expected: " << expected[i]
                        << " received: " << leaf_name;
                BESDEBUG(MODULE, msg.str() << endl);
                CPPUNIT_ASSERT(expected[i] == leaf_name);
            }
        }
        catch (BESError &be) {
            string msg = "Caught BESError! Message: " + be.get_message();
            cerr << endl << msg << endl;
            CPPUNIT_ASSERT(!"Caught BESError");
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

int main(int argc, char*argv[])
{
    CppUnit::TextTestRunner runner;
    runner.addTest(CppUnit::TestFactoryRegistry::getRegistry().makeTest());

    int option_char;
    while ((option_char = getopt(argc, argv, "db")) != -1)
        switch (option_char) {
        case 'd':
            debug = true;  // debug is a static global
            break;
        case 'b':
            bes_debug = true;  // debug is a static global
            break;
        default:
            break;
        }

    argc -= optind;
    argv += optind;

    bool wasSuccessful = true;
    string test = "";
    if (0 == argc) {
        // run them all
        wasSuccessful = runner.run("");
    }
    else {
        int i = 0;
        while (i < argc) {
            if (debug) cerr << "Running " << argv[i] << endl;
            test = cmr::GranuleTest::suite()->getName().append("::").append(argv[i]);
            wasSuccessful = wasSuccessful && runner.run(test);
            ++i;
        }
    }

    return wasSuccessful ? 0 : 1;
}
