// replistT.C

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

#include "BESReporterList.h"
#include "TestReporter.h"
#include <unistd.h>

static bool debug = false;

#undef DBG
#define DBG(x)                                                                                                         \
    do {                                                                                                               \
        if (debug)                                                                                                     \
            (x);                                                                                                       \
    } while (false);

class replistT : public TestFixture {
private:
public:
    replistT() = default;
    ~replistT() = default;

    void setUp() {}

    void tearDown() {}

    CPPUNIT_TEST_SUITE(replistT);

    CPPUNIT_TEST(do_test);

    CPPUNIT_TEST_SUITE_END();

    void do_test() {
        cout << "*****************************************" << endl;
        cout << "Entered replistT::run" << endl;

        cout << "*****************************************" << endl;
        cout << "add the 5 reporters" << endl;
        BESReporterList *rl = BESReporterList::TheList();
        char num[10];
        for (int i = 0; i < 5; i++) {
            sprintf(num, "rep%d", i);
            CPPUNIT_ASSERT(rl->add_reporter(num, new TestReporter(num)));
        }

        cout << "*****************************************" << endl;
        cout << "try to add rep3 again" << endl;
        TestReporter *r = new TestReporter("rep3");
        CPPUNIT_ASSERT(rl->add_reporter("rep3", r) == false);

        cout << "*****************************************" << endl;
        cout << "finding the reporters" << endl;
        for (int i = 4; i >= 0; i--) {
            sprintf(num, "rep%d", i);
            cout << "    looking for " << num << endl;
            r = (TestReporter *)rl->find_reporter(num);
            CPPUNIT_ASSERT(r);
            CPPUNIT_ASSERT(r->get_name() == num);
        }

        cout << "*****************************************" << endl;
        cout << "finding non-existing reporter" << endl;
        r = (TestReporter *)rl->find_reporter("notthere");
        CPPUNIT_ASSERT(!r);

        cout << "*****************************************" << endl;
        cout << "removing rep2" << endl;
        r = (TestReporter *)rl->remove_reporter("rep2");
        CPPUNIT_ASSERT(r);
        CPPUNIT_ASSERT(r->get_name() == "rep2");

        cout << "*****************************************" << endl;
        cout << "find rep2" << endl;
        r = (TestReporter *)rl->find_reporter("rep2");
        CPPUNIT_ASSERT(!r);

        cout << "*****************************************" << endl;
        cout << "add rep2 again" << endl;
        r = new TestReporter("rep2");
        CPPUNIT_ASSERT(rl->add_reporter("rep2", r));

        cout << "*****************************************" << endl;
        cout << "find rep2" << endl;
        r = (TestReporter *)rl->find_reporter("rep2");
        CPPUNIT_ASSERT(r);
        CPPUNIT_ASSERT(r->get_name() == "rep2");

        cout << "*****************************************" << endl;
        cout << "report" << endl;
        BESDataHandlerInterface dhi;
        rl->report(dhi);
        for (int i = 4; i >= 0; i--) {
            sprintf(num, "rep%d", i);
            cout << "    " << num << " reported?" << endl;
            CPPUNIT_ASSERT(dhi.data[num] == num);
        }

        cout << "*****************************************" << endl;
        cout << "Returning from replistT::run" << endl;
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION(replistT);

int main(int argc, char *argv[]) {
    int option_char;
    while ((option_char = getopt(argc, argv, "dh")) != EOF)
        switch (option_char) {
        case 'd':
            debug = true; // debug is a static global
            break;
        case 'h': { // help - show test names
            cerr << "Usage: replistT has the following tests:" << endl;
            const std::vector<Test *> &tests = replistT::suite()->getTests();
            unsigned int prefix_len = replistT::suite()->getName().append("::").size();
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
            test = replistT::suite()->getName().append("::").append(argv[i]);
            wasSuccessful = wasSuccessful && runner.run(test);
        }
    }

    return wasSuccessful ? 0 : 1;
}
