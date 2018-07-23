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
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>

#include <GetOpt.h>

#include "TheBESKeys.h"
#include "BESXMLInfo.h"
#include "CatalogNode.h"

#include "test_config.h"
#include "test_utils.h"

static bool debug = false;

#undef DBG
#define DBG(x) do { if (debug) (x); } while(false);

using namespace std;
using namespace CppUnit;
using namespace bes;

class CatalogNodeTest: public CppUnit::TestFixture {

    CatalogNode *d_node;

public:

    // Called once before everything gets tested
    CatalogNodeTest(): d_node(0)
    {
    }

    // Called at the end of the test
    ~CatalogNodeTest()
    {
    }

    // Called before each test
    void setUp()
    {
        TheBESKeys::ConfigFile = string(TEST_SRC_DIR).append("/bes.conf");

        d_node = new CatalogNode;
        d_node->set_name("/test");
        d_node->set_catalog_name("default");
        d_node->set_lmt("07-07-07T07:07:07");
    }

    // Called after each test
    void tearDown()
    {
        TheBESKeys::ConfigFile = "";

        delete d_node; d_node = 0;
    }

    CPPUNIT_TEST_SUITE( CatalogNodeTest );

    CPPUNIT_TEST(encode_node_test);

    CPPUNIT_TEST_SUITE_END();

    void encode_node_test()
    {
        try {
            auto_ptr<BESInfo> info(new BESXMLInfo);

            BESDataHandlerInterface dhi;
            info->begin_response("showNode", dhi);

            d_node->encode_node(info.get());

            // end the response object
            info->end_response();

            ostringstream oss;
            info->print(oss);

            string show_node_response = read_test_baseline(string(TEST_SRC_DIR) + "/catalog_test_baselines/show_node_test_1.txt");

            DBG(cerr << "baseline: " << show_node_response << endl);
            DBG(cerr << "response: " << oss.str() << endl);

            CPPUNIT_ASSERT(oss.str() == show_node_response);
        }
        catch (BESError &e) {
            cerr << "encode_node_test() - Error: " << e.get_verbose_message() << endl;
            CPPUNIT_ASSERT(false);
        }
    }

};

// BindTest

CPPUNIT_TEST_SUITE_REGISTRATION(CatalogNodeTest);

int main(int argc, char*argv[])
{

    int start = 0;
    GetOpt getopt(argc, argv, "dh");
    char option_char;
    while ((option_char = getopt()) != EOF)
        switch (option_char) {
        case 'd': {
            debug = 1;  // debug is a static global
            start = 1;
            break;
        }
        case 'h': {     // help - show test names
            cerr << "Usage: CatalogNodeTest has the following tests:" << endl;
            const std::vector<Test*> &tests = CatalogNodeTest::suite()->getTests();
            unsigned int prefix_len = CatalogNodeTest::suite()->getName().append("::").length();
            for (std::vector<Test*>::const_iterator i = tests.begin(), e = tests.end(); i != e; ++i) {
                cerr << (*i)->getName().replace(0, prefix_len, "") << endl;
            }
            break;
        }
        default:
            break;
        }

    CppUnit::TextTestRunner runner;
    runner.addTest(CppUnit::TestFactoryRegistry::getRegistry().makeTest());

    bool wasSuccessful = true;
    string test = "";
    int i = getopt.optind;
    if (i == argc) {
        // run them all
        wasSuccessful = runner.run("");
    }
    else {
        while (i < argc) {
            if (debug) cerr << "Running " << argv[i] << endl;
            test = CatalogNodeTest::suite()->getName().append("::").append(argv[i]);
            wasSuccessful = wasSuccessful && runner.run(test);
            ++i;
        }
    }

    return wasSuccessful ? 0 : 1;
}

