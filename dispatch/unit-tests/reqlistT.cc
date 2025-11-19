// reqlistT.C

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

#include <iostream>

using std::cerr;
using std::cout;
using std::endl;
using std::string;

#include "BESRequestHandlerList.h"
#include "TestRequestHandler.h"
#include <unistd.h>

static bool debug = false;

#undef DBG
#define DBG(x)                                                                                                         \
    do {                                                                                                               \
        if (debug)                                                                                                     \
            (x);                                                                                                       \
    } while (false);

class reqlistT : public TestFixture {
private:
public:
    reqlistT() = default;
    ~reqlistT() = default;

    void setUp() {}

    void tearDown() {}

    CPPUNIT_TEST_SUITE(reqlistT);

    CPPUNIT_TEST(do_test);

    CPPUNIT_TEST_SUITE_END();

    void do_test() {
        BESRequestHandler *rh = nullptr;

        cout << "*****************************************" << endl;
        cout << "Entered reqlistT::run" << endl;

        cout << "*****************************************" << endl;
        cout << "add the 5 request handlers" << endl;
        BESRequestHandlerList *rhl = BESRequestHandlerList::TheList();
        char num[10];
        for (int i = 0; i < 5; i++) {
            sprintf(num, "req%d", i);
            cout << "    adding " << num << endl;
            rh = new TestRequestHandler(num);
            CPPUNIT_ASSERT(rhl->add_handler(num, rh));
        }

        cout << "*****************************************" << endl;
        cout << "try to add req3 again" << endl;
        rh = new TestRequestHandler("req3");
        CPPUNIT_ASSERT(rhl->add_handler("req3", rh) == false);

        cout << "*****************************************" << endl;
        cout << "finding the handlers" << endl;
        for (int i = 4; i >= 0; i--) {
            sprintf(num, "req%d", i);
            cout << "    finding " << num << endl;
            rh = rhl->find_handler(num);
            CPPUNIT_ASSERT(rh);
            CPPUNIT_ASSERT(rh->get_name() == num);
        }

        cout << "*****************************************" << endl;
        cout << "find handler that doesn't exist" << endl;
        rh = rhl->find_handler("not_there");
        CPPUNIT_ASSERT(!rh);

        cout << "*****************************************" << endl;
        cout << "removing req2" << endl;
        rh = rhl->remove_handler("req2");
        CPPUNIT_ASSERT(rh);
        CPPUNIT_ASSERT(rh->get_name() == "req2");

        rh = rhl->find_handler("req2");
        CPPUNIT_ASSERT(!rh);

        cout << "*****************************************" << endl;
        cout << "add req2 back" << endl;
        rh = new TestRequestHandler("req2");
        CPPUNIT_ASSERT(rhl->add_handler("req2", rh));

        rh = rhl->find_handler("req2");
        CPPUNIT_ASSERT(rh);
        CPPUNIT_ASSERT(rh->get_name() == "req2");

        cout << "*****************************************" << endl;
        cout << "Iterating through handler list" << endl;
        BESRequestHandlerList::Handler_citer h = rhl->get_first_handler();
        BESRequestHandlerList::Handler_citer hl = rhl->get_last_handler();
        int num_handlers = 0;
        for (; h != hl; h++) {
            rh = (*h).second;
            char sb[10];
            sprintf(sb, "req%d", num_handlers);
            string n = rh->get_name();
            CPPUNIT_ASSERT(n == sb);
            num_handlers++;
        }
        CPPUNIT_ASSERT(num_handlers == 5);

        cout << "*****************************************" << endl;
        cout << "Returning from reqlistT::run" << endl;
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION(reqlistT);

int main(int argc, char *argv[]) {
    int option_char;
    while ((option_char = getopt(argc, argv, "dh")) != EOF)
        switch (option_char) {
        case 'd':
            debug = true; // debug is a static global
            break;
        case 'h': { // help - show test names
            cerr << "Usage: reqlistT has the following tests:" << endl;
            const std::vector<Test *> &tests = reqlistT::suite()->getTests();
            unsigned int prefix_len = reqlistT::suite()->getName().append("::").size();
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
            test = reqlistT::suite()->getName().append("::").append(argv[i]);
            wasSuccessful = wasSuccessful && runner.run(test);
        }
    }

    return wasSuccessful ? 0 : 1;
}
