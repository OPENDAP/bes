// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2026 OPeNDAP, Inc.
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

// Tests that BES_COMMAND_TIMING and BES_MODULE_TIMING write timing log records
// only in developer builds (NDEBUG not defined). TASK-001

#include "test_config.h"

#include <cstdio>
#include <fstream>
#include <sstream>

#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/extensions/TestFactoryRegistry.h>

#include <unistd.h>

#include "BESDataHandlerInterface.h"
#include "BESDataNames.h"
#include "BESDebug.h"
#include "BESLog.h"
#include "BESStopWatch.h"
#include "TheBESKeys.h"

using namespace std;

static bool debug = false;
static bool bes_debug = false;

#undef DBG
#define DBG(x)                                                                                                         \
    do {                                                                                                               \
        if (debug)                                                                                                     \
            x;                                                                                                         \
    } while (false)
#define prolog std::string("BESStopWatchTest::").append(__func__).append("() - ")

// Not *.log so it cannot collide with the automake test driver's BESStopWatchTest.log.
static const string timing_log_name = "./stopwatch_timing_test.txt";

namespace bes_timing_test {

class BESStopWatchTest : public CppUnit::TestFixture {

    static string read_log() {
        ifstream ifs(timing_log_name);
        stringstream ss;
        ss << ifs.rdbuf();
        return ss.str();
    }

public:
    BESStopWatchTest() = default;
    ~BESStopWatchTest() override = default;

    void setUp() override {
        if (debug)
            cerr << endl;
        if (bes_debug)
            BESDebug::SetUp("cerr,bes,timing");

        // BESLog is a singleton that reads BES.LogName when first used, so this must
        // be set before any test touches the log.
        TheBESKeys::ConfigFile = string(TEST_BUILD_DIR).append("/bes.conf");
        TheBESKeys::TheKeys()->set_key("BES.LogName", timing_log_name);
    }

    void test_module_timing() {
        const string msg = "test_module_timing_marker";
        {
            BES_MODULE_TIMING(msg);
        }
        const string log = read_log();
        DBG(cerr << prolog << "log: " << log << endl);
#ifndef NDEBUG
        CPPUNIT_ASSERT_MESSAGE("Developer builds should log module timing",
                               log.find("Module timing: " + msg) != string::npos);
#else
        CPPUNIT_ASSERT_MESSAGE("Production builds should not log module timing",
                               log.find("Module timing:") == string::npos);
#endif
    }

    void test_command_timing() {
        const string msg = "test_command_timing_marker ";
        BESDataHandlerInterface dhi;
        BESDataHandlerInterface *dhi_ptr = &dhi;
        dhi.data[LOG_INFO] = "log_info_marker";
        {
            BES_COMMAND_TIMING(msg, dhi_ptr);
        }
        const string log = read_log();
        DBG(cerr << prolog << "log: " << log << endl);
#ifndef NDEBUG
        CPPUNIT_ASSERT_MESSAGE("Developer builds should log command timing",
                               log.find("Command timing: " + msg + "log_info_marker") != string::npos);
#else
        CPPUNIT_ASSERT_MESSAGE("Production builds should not log command timing",
                               log.find("Command timing:") == string::npos);
#endif
    }

    CPPUNIT_TEST_SUITE(BESStopWatchTest);

    CPPUNIT_TEST(test_module_timing);
    CPPUNIT_TEST(test_command_timing);

    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(BESStopWatchTest);

} // namespace bes_timing_test

int main(int argc, char *argv[]) {
    // Start from an empty log; BESLog opens it in append mode.
    std::remove(timing_log_name.c_str());

    CppUnit::TextTestRunner runner;
    runner.addTest(CppUnit::TestFactoryRegistry::getRegistry().makeTest());

    int option_char;
    while ((option_char = getopt(argc, argv, "dD")) != -1) {
        switch (option_char) {
        case 'd':
            debug = true; // debug is a static global
            break;
        case 'D':
            debug = true;     // debug is a static global
            bes_debug = true; // debug is a static global
            break;
        default:
            break;
        }
    }
    argc -= optind;
    argv += optind;

    bool wasSuccessful = true;
    string test;
    if (0 == argc) {
        // run them all
        wasSuccessful = runner.run("");
    } else {
        int i = 0;
        while (i < argc) {
            if (debug)
                cerr << "Running " << argv[i] << endl;
            test = bes_timing_test::BESStopWatchTest::suite()->getName().append("::").append(argv[i]);
            wasSuccessful = wasSuccessful && runner.run(test);
            ++i;
        }
    }

    return wasSuccessful ? 0 : 1;
}
