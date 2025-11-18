// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of libdap, A C++ implementation of the OPeNDAP Data
// Access Protocol.

// Copyright (c) 2018 OPeNDAP, Inc.
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

#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/extensions/TestFactoryRegistry.h>

#include <unistd.h>

#include "BESTextInfo.h"
#include "BESXMLInfo.h"
#include "CatalogNode.h"
#include "TheBESKeys.h"

#include "test_config.h"
#include "test_utils.h"

static bool debug = false;

#undef DBG
#define DBG(x)                                                                                                         \
    do {                                                                                                               \
        if (debug)                                                                                                     \
            (x);                                                                                                       \
    } while (false);

using namespace std;
using namespace CppUnit;
using namespace bes;

class CatalogNodeTest : public CppUnit::TestFixture {

    CatalogNode *d_node;

public:
    // Called once before everything gets tested
    CatalogNodeTest() : d_node(nullptr) {}

    // Called at the end of the test
    ~CatalogNodeTest() = default;

    // Called before each test
    void setUp() {
        TheBESKeys::ConfigFile = string(TEST_BUILD_DIR).append("/bes.conf");

        d_node = new CatalogNode;
        d_node->set_name("/test");
        d_node->set_catalog_name("default");
        d_node->set_lmt("07-07-07T07:07:07");
    }

    // Called after each test
    void tearDown() {
        TheBESKeys::ConfigFile = "";

        delete d_node;
        d_node = nullptr;
    }

    CPPUNIT_TEST_SUITE(CatalogNodeTest);

    CPPUNIT_TEST(encode_node_test);
    CPPUNIT_TEST(encode_node_text_test);

    CPPUNIT_TEST_SUITE_END();

    void encode_node_test() {
        try {
            unique_ptr<BESInfo> info(new BESXMLInfo);

            BESDataHandlerInterface dhi;
            info->begin_response("showNode", dhi);

            d_node->encode_node(info.get());

            // end the response object
            info->end_response();

            ostringstream oss;
            info->print(oss);

            string show_node_response =
                read_test_baseline(string(TEST_SRC_DIR) + "/catalog_test_baselines/show_node_test_1.txt");

            DBG(cerr << "baseline: " << show_node_response << endl);
            DBG(cerr << "response: " << oss.str() << endl);

            CPPUNIT_ASSERT(oss.str() == show_node_response);
        } catch (BESError &e) {
            cerr << "encode_node_test() - Error: " << e.get_verbose_message() << endl;
            CPPUNIT_ASSERT(false);
        }
    }

    // This is the same test as encode_node_test but using a BESTextInfo object.
    void encode_node_text_test() {
        try {
            unique_ptr<BESInfo> info(new BESTextInfo);

            BESDataHandlerInterface dhi;
            info->begin_response("showNode", dhi);

            d_node->encode_node(info.get());

            // end the response object
            info->end_response();

            ostringstream oss;
            info->print(oss);

            string show_node_response =
                read_test_baseline(string(TEST_SRC_DIR) + "/catalog_test_baselines/show_node_test_2.txt");

            DBG(cerr << "baseline: '" << show_node_response << "'" << endl);
            DBG(cerr << "response: '" << oss.str() << "'" << endl);

            CPPUNIT_ASSERT(oss.str() == show_node_response);
        } catch (BESError &e) {
            cerr << "encode_node_test() - Error: " << e.get_verbose_message() << endl;
            CPPUNIT_ASSERT(false);
        }
    }
};

// BindTest

CPPUNIT_TEST_SUITE_REGISTRATION(CatalogNodeTest);

int main(int argc, char *argv[]) {
    int option_char;
    while ((option_char = getopt(argc, argv, "dh")) != EOF)
        switch (option_char) {
        case 'd':
            debug = true; // debug is a static global
            break;

        case 'h': { // help - show test names
            cerr << "Usage: CatalogNodeTest has the following tests:" << endl;
            const std::vector<Test *> &tests = CatalogNodeTest::suite()->getTests();
            unsigned int prefix_len = CatalogNodeTest::suite()->getName().append("::").size();
            for (auto test : tests) {
                cerr << test->getName().replace(0, prefix_len, "") << endl;
            }
            break;
        }
        default:
            break;
        }

    argc -= optind;
    argv += optind;

    CppUnit::TextTestRunner runner;
    runner.addTest(CppUnit::TestFactoryRegistry::getRegistry().makeTest());

    bool wasSuccessful = true;
    string test = "";
    if (0 == argc) {
        // run them all
        wasSuccessful = runner.run("");
    } else {
        int i = 0;
        while (i < argc) {
            if (debug)
                cerr << "Running " << argv[i] << endl;
            test = CatalogNodeTest::suite()->getName().append("::").append(argv[i]);
            wasSuccessful = wasSuccessful && runner.run(test);
            ++i;
        }
    }

    return wasSuccessful ? 0 : 1;
}
