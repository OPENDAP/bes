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

//#include <cstdio>

#include <pthread.h>
#include <vector>

#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>

#include "TheBESKeys.h"
#include "BESCatalogList.h"
#include <GetOpt.h>

static bool debug = false;

#undef DBG
#define DBG(x) do { if (debug) (x); } while(false);

using namespace std;
using namespace CppUnit;

class BESCatalogListUnitTest: public CppUnit::TestFixture {

public:

    // Called once before everything gets tested
    BESCatalogListUnitTest()
    {
    }

    // Called at the end of the test
    ~BESCatalogListUnitTest()
    {
    }

    // Called before each test
    void setup()
    {
    }

    // Called after each test
    void tearDown()
    {
    }

    CPPUNIT_TEST_SUITE( BESCatalogListUnitTest );

    CPPUNIT_TEST(bclut_test);

    CPPUNIT_TEST_SUITE_END();

    void bclut_test()
    {

        try {
            DBG(cerr << endl);
            DBG(cerr << "bclut_test() - BEGIN." << endl);

            string defcat = BESCatalogList::TheCatalogList()->default_catalog_name();
            DBG(cerr << "bclut_test() - Default catalog is '" << defcat << "'" << endl);
            CPPUNIT_ASSERT(defcat == "catalog");

            int numCat = BESCatalogList::TheCatalogList()->num_catalogs();
            DBG(cerr << "bclut_test() - TheCatalogList()->num_catalogs(): " << numCat << endl);
            CPPUNIT_ASSERT(numCat == 0);

            DBG(cerr << "bclut_test() - Calling  BESCatalogList::delete_instance()" << endl);
            BESCatalogList::delete_instance();
            DBG(cerr << "bclut_test() - Calling  BESCatalogList::initialize_instance()" << endl);
            BESCatalogList::initialize_instance();

            defcat = BESCatalogList::TheCatalogList()->default_catalog_name();
            DBG(cerr << "bclut_test() - Default catalog is '" << defcat << "'" << endl);
            CPPUNIT_ASSERT(defcat == "catalog");

            numCat = BESCatalogList::TheCatalogList()->num_catalogs();
            DBG(cerr << "bclut_test() - TheCatalogList()->num_catalogs(): " << numCat << endl);
            CPPUNIT_ASSERT(numCat == 0);

            DBG(cerr << "bclut_test() - END." << endl);
            CPPUNIT_ASSERT(true);
        }
        catch (BESError &e) {
            cerr << "bclut_test() - ERROR: " << e.get_message() << endl;
            CPPUNIT_ASSERT(false);
        }

    }

};

// BindTest

CPPUNIT_TEST_SUITE_REGISTRATION(BESCatalogListUnitTest);

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
            cerr << "Usage: BESCatalogListUnitTest has the following tests:" << endl;
            const std::vector<Test*> &tests = BESCatalogListUnitTest::suite()->getTests();
            unsigned int prefix_len = BESCatalogListUnitTest::suite()->getName().append("::").length();
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
            test = BESCatalogListUnitTest::suite()->getName().append("::").append(argv[i]);
            wasSuccessful = wasSuccessful && runner.run(test);
            ++i;
        }
    }

    return wasSuccessful ? 0 : 1;
}

