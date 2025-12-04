// debugT.C

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

#include "config.h"

#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/extensions/TestFactoryRegistry.h>

using namespace CppUnit;

#include <sys/time.h> // for gettimeofday
#include <unistd.h>   // for access

#include <cstdlib>
#include <iostream>
#include <sstream>

using std::cerr;
using std::cout;
using std::endl;
using std::ostringstream;
using std::string;

#include "BESDebug.h"
#include "BESError.h"
#include "BESUtil.h"
#include "TheBESKeys.h"
#include "test_config.h"

string DebugArgs;
static bool debug = false;

class debugT : public TestFixture {

public:
    debugT() = default;
    ~debugT() override = default;

    void setUp() override {
        string bes_conf = (string)TEST_SRC_DIR + "/bes.conf";
        TheBESKeys::ConfigFile = bes_conf;
    }

    CPPUNIT_TEST_SUITE(debugT);

    CPPUNIT_TEST(do_test);
    // CPPUNIT_TEST(profile_is_set);

    CPPUNIT_TEST_SUITE_END();

    void compare_debug(string result, string expected) {
        if (!expected.empty()) {
            string::size_type lb = result.find("[");
            CPPUNIT_ASSERT(lb != string::npos);
            string::size_type rb = result.rfind("]");
            CPPUNIT_ASSERT(rb != string::npos);
            result = result.substr(rb + 2);
        }
        CPPUNIT_ASSERT(result == expected);
    }

    // This test is here to help profile the debug code. jhrg 4/12/23
    void profile_is_set() {
        struct timeval start_usage;
        gettimeofday(&start_usage, nullptr);
        for (int i = 0; i < 10000000; ++i)
            BESDebug::IsSet("nc");

        struct timeval end_usage;
        gettimeofday(&end_usage, nullptr);

        cout << "start: " << start_usage.tv_sec << " " << start_usage.tv_usec << endl;
        cout << "end: " << end_usage.tv_sec << " " << end_usage.tv_usec << endl;
        cout << "diff: "
             << (end_usage.tv_sec * 1000000 + end_usage.tv_usec) - (start_usage.tv_sec * 1000000 + start_usage.tv_usec)
             << "us" << endl;
    }

    void do_test() {
        cout << "*****************************************" << endl;
        cout << "Entered debugT::run" << endl;

        char mypid[12];
        BESUtil::fastpidconverter(mypid, 10);

        if (!DebugArgs.empty()) {
            cout << "*****************************************" << endl;
            cout << "trying " << DebugArgs << endl;
            try {
                BESDebug::SetUp(DebugArgs);
            } catch (BESError &e) {
                cerr << e.get_message() << endl;
                CPPUNIT_ASSERT(!"Failed to set up Debug");
            }
        } else {
            try {
                cout << "*****************************************" << endl;
                cout << "Setting up with bad file name /bad/dir/debug" << endl;
                BESDebug::SetUp("/bad/dir/debug,nc");
                CPPUNIT_ASSERT(!"Successfully set up with bad file");
            } catch (BESError &e) {
                cout << "Unable to set up debug ... good" << endl;
                cout << e.get_message() << endl;
            }

            try {
                cout << "*****************************************" << endl;
                cout << "Setting up myfile.debug,nc,cdf,hdf4" << endl;
                BESDebug::SetUp("myfile.debug,nc,cdf,hdf4");
                int result = access("myfile.debug", W_OK | R_OK);
                CPPUNIT_ASSERT(result != -1);
                CPPUNIT_ASSERT(BESDebug::IsSet("nc"));
                CPPUNIT_ASSERT(BESDebug::IsSet("cdf"));
                CPPUNIT_ASSERT(BESDebug::IsSet("hdf4"));

                BESDebug::SetStrm(nullptr, false);
                result = remove("myfile.debug");
                CPPUNIT_ASSERT(result != -1);
            } catch (BESError &e) {
                CPPUNIT_ASSERT(!"Unable to set up debug");
            }

            try {
                cout << "*****************************************" << endl;
                cout << "Setting up cerr,ff,-cdf" << endl;
                BESDebug::SetUp("cerr,ff,-cdf");
                CPPUNIT_ASSERT(BESDebug::IsSet("ff"));
                CPPUNIT_ASSERT(!BESDebug::IsSet("cdf"));
            } catch (BESError &e) {
                CPPUNIT_ASSERT(!"Unable to set up debug");
            }

            cout << "*****************************************" << endl;
            cout << "try debugging to nc" << endl;
            ostringstream nc;
            BESDebug::SetStrm(&nc, false);
            string debug_str = "Testing nc debug";
            BESDEBUG("nc", debug_str);

            cerr << "source: " << debug_str << "  result: " << nc.str() << endl;
            compare_debug(nc.str(), debug_str);

            cout << "*****************************************" << endl;
            cout << "try debugging to hdf4" << endl;
            ostringstream hdf4;
            BESDebug::SetStrm(&hdf4, false);
            debug_str = "Testing hdf4 debug";
            BESDEBUG("hdf4", debug_str);
            cerr << "source: " << debug_str << "  result: " << hdf4.str() << endl;
            compare_debug(hdf4.str(), debug_str);

            cout << "*****************************************" << endl;
            cout << "try debugging to ff" << endl;
            ostringstream ff;
            BESDebug::SetStrm(&ff, false);
            debug_str = "Testing ff debug";
            BESDEBUG("ff", debug_str);
            cerr << "source: " << debug_str << "  result: " << ff.str() << endl;
            compare_debug(ff.str(), debug_str);

            cout << "*****************************************" << endl;
            cout << "turn off ff and try debugging to ff again" << endl;
            BESDebug::Set("ff", false);
            CPPUNIT_ASSERT(!BESDebug::IsSet("ff"));
            ostringstream ff2;
            BESDebug::SetStrm(&ff2, false);
            debug_str = "should not produce debug output";
            BESDEBUG("ff", debug_str);
            cerr << "source: " << debug_str << "  result: " << ff2.str() << endl;
            compare_debug(ff2.str(), "");

            cout << "*****************************************" << endl;
            cout << "try debugging to cdf" << endl;
            ostringstream cdf;
            BESDebug::SetStrm(&cdf, false);
            debug_str = "should not produce debug output";
            BESDEBUG("cdf", debug_str);
            cerr << "source: " << debug_str << "  result: " << cdf.str() << endl;
            compare_debug(cdf.str(), "");
        }

        cout << "*****************************************" << endl;
        cout << "display debug help" << endl;
        BESDebug::Help(cout);

        cout << "*****************************************" << endl;
        cout << "Returning from debugT::run" << endl;
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION(debugT);

int main(int argc, char *argv[]) {
    int option_char;
    while ((option_char = getopt(argc, argv, "dh")) != EOF)
        switch (option_char) {
        case 'd':
            debug = true; // debug is a static global
            break;
        case 'h': { // help - show test names
            cerr << "Usage: debugT has the following tests:" << endl;
            const std::vector<Test *> &tests = debugT::suite()->getTests();
            unsigned int prefix_len = debugT::suite()->getName().append("::").size();
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
            test = debugT::suite()->getName().append("::").append(argv[i]);
            wasSuccessful = wasSuccessful && runner.run(test);
        }
    }

    return wasSuccessful ? 0 : 1;
}
