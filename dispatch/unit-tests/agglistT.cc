// agglistT.C

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2004-2009 University Corporation for Atmospheric Research
// Author: Patrick West <pwest@ucar.edu> and Jose Garcia <jgarcia@ucar.edu>
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
// You can contact University Corporation for Atmospheric Research at
// 3080 Center Green Drive, Boulder, CO 80301

// (c) COPYRIGHT University Corporation for Atmospheric Research 2004-2005
// Please read the full copyright statement in the file COPYRIGHT_UCAR.
//
// Authors:
//      pwest       Patrick West <pwest@ucar.edu>
//      jgarcia     Jose Garcia <jgarcia@ucar.edu>

#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/extensions/TestFactoryRegistry.h>

using namespace CppUnit;

#include <cstdlib>
#include <iostream>

using std::cerr;
using std::cout;
using std::endl;

#include "BESAggFactory.h"
#include "BESError.h"
#include "BESTextInfo.h"
#include "TestAggServer.h"
#include "TheBESKeys.h"
#include <test_config.h>
#include <unistd.h>

static bool debug = false;

#undef DBG
#define DBG(x)                                                                                                         \
    do {                                                                                                               \
        if (debug)                                                                                                     \
            (x);                                                                                                       \
    } while (false);

class agglistT : public TestFixture {
private:
public:
    agglistT() {}
    ~agglistT() {}

    static BESAggregationServer *h1(const string &name) { return new TestAggServer(name); }

    static BESAggregationServer *h2(const string &name) { return new TestAggServer(name); }

    static BESAggregationServer *h3(const string &name) { return new TestAggServer(name); }

    void setUp() {
        string bes_conf = (string)TEST_SRC_DIR + "/empty.ini";
        TheBESKeys::ConfigFile = bes_conf;
    }

    void tearDown() {}

    CPPUNIT_TEST_SUITE(agglistT);

    CPPUNIT_TEST(do_test);

    CPPUNIT_TEST_SUITE_END();

    void do_test() {
        cout << "*****************************************" << endl;
        cout << "Entered agglistT::run" << endl;

        cout << "*****************************************" << endl;
        cout << "Adding three handlers to the list" << endl;
        try {
            CPPUNIT_ASSERT(BESAggFactory::TheFactory()->add_handler("h1", agglistT::h1));
            CPPUNIT_ASSERT(BESAggFactory::TheFactory()->add_handler("h2", agglistT::h2));
            CPPUNIT_ASSERT(BESAggFactory::TheFactory()->add_handler("h3", agglistT::h3));
        } catch (BESError &e) {
            cerr << e.get_message() << endl;
            CPPUNIT_ASSERT(!"Failed to add aggregation servers to list");
        }

        cout << "*****************************************" << endl;
        cout << "Find the three handlers" << endl;
        try {
            BESAggregationServer *s = 0;
            s = BESAggFactory::TheFactory()->find_handler("h1");
            CPPUNIT_ASSERT(s);
            s = BESAggFactory::TheFactory()->find_handler("h2");
            CPPUNIT_ASSERT(s);
            s = BESAggFactory::TheFactory()->find_handler("h3");
            CPPUNIT_ASSERT(s);
        } catch (BESError &e) {
            cerr << e.get_message() << endl;
            CPPUNIT_ASSERT(!"Failed to find aggregation servers");
        }

        cout << "*****************************************" << endl;
        cout << "Remove handler h2" << endl;
        try {
            bool removed = BESAggFactory::TheFactory()->remove_handler("h2");
            CPPUNIT_ASSERT(removed);
        } catch (BESError &e) {
            cerr << e.get_message() << endl;
            CPPUNIT_ASSERT(!"Failed to remove aggregation server h2");
        }

        cout << "*****************************************" << endl;
        cout << "Find the two handlers" << endl;
        try {
            BESAggregationServer *s = 0;
            s = BESAggFactory::TheFactory()->find_handler("h1");
            CPPUNIT_ASSERT(s);
            s = BESAggFactory::TheFactory()->find_handler("h2");
            CPPUNIT_ASSERT(!s);
            s = BESAggFactory::TheFactory()->find_handler("h3");
            CPPUNIT_ASSERT(s);
        } catch (BESError &e) {
            cerr << "Failed to find aggregation servers" << endl;
            cerr << e.get_message() << endl;
        }

        cout << "*****************************************" << endl;
        cout << "Show handler names registered" << endl;
        string registered = BESAggFactory::TheFactory()->get_handler_names();
        string expected = "h1, h3";
        CPPUNIT_ASSERT(registered == expected);

        cout << "*****************************************" << endl;
        cout << "Returning from agglistT::run" << endl;
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION(agglistT);

int main(int argc, char *argv[]) {
    int option_char;
    while ((option_char = getopt(argc, argv, "dh")) != EOF)
        switch (option_char) {
        case 'd':
            debug = 1; // debug is a static global
            break;
        case 'h': { // help - show test names
            cerr << "Usage: agglistT has the following tests:" << endl;
            const std::vector<Test *> &tests = agglistT::suite()->getTests();
            unsigned int prefix_len = agglistT::suite()->getName().append("::").size();
            for (std::vector<Test *>::const_iterator i = tests.begin(), e = tests.end(); i != e; ++i) {
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
    } else {
        int i = 0;
        while (i < argc) {
            if (debug)
                cerr << "Running " << argv[i] << endl;
            test = agglistT::suite()->getName().append("::").append(argv[i]);
            wasSuccessful = wasSuccessful && runner.run(test);
        }
    }

    return wasSuccessful ? 0 : 1;
}
