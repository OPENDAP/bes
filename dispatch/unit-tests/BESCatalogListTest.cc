// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of libdap, A C++ implementation of the OPeNDAP Data
// Access Protocol.

// Copyright (c) 2013 OPeNDAP, Inc.
// Author: Nathan David Potter <ndp@opendap.org>
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

#include <unistd.h>

#include "TheBESKeys.h"
#include "BESCatalogList.h"

#include "test_config.h"

static bool debug = false;

#undef DBG
#define DBG(x) do { if (debug) (x); } while(false);

using namespace std;
using namespace CppUnit;

class BESCatalogListTest: public CppUnit::TestFixture {

public:

    // Called once before everything gets tested
    BESCatalogListTest()
    {
    }

    // Called at the end of the test
    ~BESCatalogListTest()
    {
    }

    // Called before each test
    void setUp()
    {
        TheBESKeys::ConfigFile = string(TEST_BUILD_DIR).append("/bes.conf");
    }

    // Called after each test
    void tearDown()
    {
        TheBESKeys::ConfigFile = "";
    }

    CPPUNIT_TEST_SUITE( BESCatalogListTest );

    CPPUNIT_TEST(bclut_test);

    CPPUNIT_TEST_SUITE_END();

    void bclut_test()
    {

        try {
            DBG(cerr << endl);
            DBG(cerr << "bclut_test() - BEGIN." << endl);

            string defcat = BESCatalogList::TheCatalogList()->default_catalog_name();
            DBG(cerr << "bclut_test() - Default catalog is '" << defcat << "'" << endl);
            CPPUNIT_ASSERT(defcat == "default");

            int numCat = BESCatalogList::TheCatalogList()->num_catalogs();
            DBG(cerr << "bclut_test() - TheCatalogList()->num_catalogs(): " << numCat << endl);
            CPPUNIT_ASSERT(numCat == 1);

//            DBG(cerr << "bclut_test() - Calling  BESCatalogList::delete_instance()" << endl);
//            BESCatalogList::delete_instance();
//            DBG(cerr << "bclut_test() - Calling  BESCatalogList::initialize_instance()" << endl);
//            BESCatalogList::initialize_instance();

            defcat = BESCatalogList::TheCatalogList()->default_catalog_name();
            DBG(cerr << "bclut_test() - Default catalog is '" << defcat << "'" << endl);
            CPPUNIT_ASSERT(defcat == "default");

            numCat = BESCatalogList::TheCatalogList()->num_catalogs();
            DBG(cerr << "bclut_test() - TheCatalogList()->num_catalogs(): " << numCat << endl);
            CPPUNIT_ASSERT(numCat == 1);

            DBG(cerr << "bclut_test() - END." << endl);
            CPPUNIT_ASSERT(true);
        }
        catch (BESError &e) {
            cerr << "bclut_test() - Error: " << e.get_verbose_message() << endl;
            CPPUNIT_ASSERT(false);
        }

    }

};

// BindTest

CPPUNIT_TEST_SUITE_REGISTRATION(BESCatalogListTest);

int main(int argc, char*argv[])
{
    int option_char;
    while ((option_char = getopt(argc, argv, "dh")) != EOF)
        switch (option_char) {
        case 'd':
            debug = 1;  // debug is a static global
            break;

        case 'h': {     // help - show test names
            cerr << "Usage: BESCatalogListUnitTest has the following tests:" << endl;
            const std::vector<Test*> &tests = BESCatalogListTest::suite()->getTests();
            unsigned int prefix_len = BESCatalogListTest::suite()->getName().append("::").size();
            for (std::vector<Test*>::const_iterator i = tests.begin(), e = tests.end(); i != e; ++i) {
                cerr << (*i)->getName().replace(0, prefix_len, "") << endl;
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
    }
    else {
        int i = 0;
        while (i < argc) {
            if (debug) cerr << "Running " << argv[i] << endl;
            test = BESCatalogListTest::suite()->getName().append("::").append(argv[i]);
            wasSuccessful = wasSuccessful && runner.run(test);
            ++i;
        }
    }

    return wasSuccessful ? 0 : 1;
}

