// scrubT.C

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

#include <cstring>
#include <iostream>
#include <limits.h>

using std::cerr;
using std::cout;
using std::endl;
using std::string;

#include "BESError.h"
#include "BESScrub.h"
#include <unistd.h>

static bool debug = false;

#undef DBG
#define DBG(x)                                                                                                         \
    do {                                                                                                               \
        if (debug)                                                                                                     \
            (x);                                                                                                       \
    } while (false);

class connT : public TestFixture {
private:
public:
    connT() {}
    ~connT() {}

    void setUp() {}

    void tearDown() {}

    CPPUNIT_TEST_SUITE(connT);

    CPPUNIT_TEST(do_test);

    CPPUNIT_TEST_SUITE_END();

    void do_test() {
        cout << "*****************************************" << endl;
        cout << "Entered scrubT::run" << endl;

        try {
            cout << "*****************************************" << endl;
            cout << "Test command line length over 255 characters" << endl;
            char arg[512];
            memset(arg, 'a', 300);
            arg[300] = '\0';
            CPPUNIT_ASSERT(!BESScrub::command_line_arg_ok(arg));
        } catch (BESError &e) {
            cerr << e.get_message() << endl;
            CPPUNIT_ASSERT(!"scrub failed");
        }

        try {
            cout << "*****************************************" << endl;
            cout << "Test command line length ok" << endl;
            string arg = "anarg";
            CPPUNIT_ASSERT(BESScrub::command_line_arg_ok(arg));
        } catch (BESError &e) {
            cerr << e.get_message() << endl;
            CPPUNIT_ASSERT(!"scrub failed");
        }

        try {
            cout << "*****************************************" << endl;
            cout << "Test path name length over 255 characters" << endl;
            char path_name[512];
            memset(path_name, 'a', 300);
            path_name[300] = '\0';
            CPPUNIT_ASSERT(!BESScrub::pathname_ok(path_name, true));
        } catch (BESError &e) {
            cerr << e.get_message() << endl;
            CPPUNIT_ASSERT(!"scrub failed");
        }

        try {
            cout << "*****************************************" << endl;
            cout << "Test path name good" << endl;
            CPPUNIT_ASSERT(BESScrub::pathname_ok("/usr/local/something_goes_here-and-is-ok.txt", true));
        } catch (BESError &e) {
            cerr << e.get_message() << endl;
            CPPUNIT_ASSERT(!"scrub failed");
        }

        try {
            cout << "*****************************************" << endl;
            cout << "Test path name bad characters strict" << endl;
            CPPUNIT_ASSERT(!BESScrub::pathname_ok("*$^&;@/user/local/bin/ls", true));
        } catch (BESError &e) {
            cerr << e.get_message() << endl;
            CPPUNIT_ASSERT(!"scrub failed");
        }

        try {
            cout << "*****************************************" << endl;
            cout << "Test array size too big" << endl;
            CPPUNIT_ASSERT(!BESScrub::size_ok(4, UINT_MAX));
        } catch (BESError &e) {
            cerr << e.get_message() << endl;
            CPPUNIT_ASSERT(!"scrub failed");
        }

        try {
            cout << "*****************************************" << endl;
            cout << "Test array size ok" << endl;
            CPPUNIT_ASSERT(BESScrub::size_ok(4, 32));
        } catch (BESError &e) {
            cerr << e.get_message() << endl;
            CPPUNIT_ASSERT(!"scrub failed");
        }

        cout << "*****************************************" << endl;
        cout << "Returning from scrubT::run" << endl;
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION(connT);

int main(int argc, char *argv[]) {
    int option_char;
    while ((option_char = getopt(argc, argv, "dh")) != EOF)
        switch (option_char) {
        case 'd':
            debug = 1; // debug is a static global
            break;
        case 'h': { // help - show test names
            cerr << "Usage: connT has the following tests:" << endl;
            const std::vector<Test *> &tests = connT::suite()->getTests();
            unsigned int prefix_len = connT::suite()->getName().append("::").size();
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
            test = connT::suite()->getName().append("::").append(argv[i]);
            wasSuccessful = wasSuccessful && runner.run(test);
        }
    }

    return wasSuccessful ? 0 : 1;
}
