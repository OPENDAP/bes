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

#include "test_config.h"

#include <chrono>
#include <memory>
#include <sstream>

#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/extensions/TestFactoryRegistry.h>

#include <unistd.h>

#include "BESDebug.h"
#include "BESLog.h"
#include "BESTimeoutError.h"
#include "TheBESKeys.h"

#include "RequestServiceTimer.h"

using namespace std;
using namespace std::chrono;

static bool debug = false;
static bool bes_debug = false;

#undef DBG
#define DBG(x)                                                                                                         \
    do {                                                                                                               \
        if (debug)                                                                                                     \
            x;                                                                                                         \
    } while (false)
#define prolog std::string("RequestTimerTest::").append(__func__).append("() - ")

namespace timer {

class RequestTimerTest : public CppUnit::TestFixture {
private:
public:
    // Called once before everything gets tested
    RequestTimerTest() = default;

    // Called at the end of the test
    ~RequestTimerTest() = default;

    // Called before each test
    void setUp() {
        if (debug)
            cerr << endl;
        if (bes_debug)
            BESDebug::SetUp("cerr,bes");

        TheBESKeys::ConfigFile = string(TEST_BUILD_DIR).append("/bes.conf");
        // TheBESKeys::TheKeys()->set_key("BES.LogName", "./bes.log", false);
    }

    // Called after each test
    /**
     *
     */
    void tearDown() {}

    /**
     *
     */
    void test_dump() {
        DBG(cerr << prolog << "BEGIN" << endl);
        RequestServiceTimer::TheTimer()->start(seconds{1});
        DBG(cerr << prolog << RequestServiceTimer::TheTimer()->dump(true) << endl);
        sleep(1);
        DBG(cerr << prolog << RequestServiceTimer::TheTimer()->dump(true) << endl);
        stringstream ss;
        RequestServiceTimer::TheTimer()->dump(ss);
        DBG(cerr << prolog << "dump(ostream): " << ss.str() << endl);
        DBG(cerr << prolog << "END" << endl);
    }

    /**
     *
     */
    void test_disable_timeout() {
        DBG(cerr << prolog << "BEGIN" << endl);
        try {
            seconds time_out{2};
            RequestServiceTimer::TheTimer()->start(time_out);
            DBG(cerr << prolog << RequestServiceTimer::TheTimer()->dump(true) << endl);
            sleep(1);
            RequestServiceTimer::TheTimer()->disable_timeout();
            sleep(2);
            CPPUNIT_ASSERT(RequestServiceTimer::TheTimer()->is_expired() == false);
            DBG(cerr << prolog << RequestServiceTimer::TheTimer()->dump(true) << endl);
        } catch (std::exception &se) {
            ostringstream msg;
            msg << prolog << "Caught std::exception! Message: " << se.what();
            cerr << msg.str() << endl;
            CPPUNIT_FAIL(msg.str());
        } catch (...) {
            CPPUNIT_FAIL(prolog + "Caught unknown exception.");
        }
        DBG(cerr << prolog << "END" << endl);
    }

    /**
     *
     */
    void test_no_timeout() {
        DBG(cerr << prolog << "BEGIN" << endl);
        try {
            seconds time_out{0};
            RequestServiceTimer::TheTimer()->start(time_out);
            DBG(cerr << prolog << RequestServiceTimer::TheTimer()->dump(true) << endl);
            sleep(1);
            CPPUNIT_ASSERT(RequestServiceTimer::TheTimer()->is_expired() == false);
            DBG(cerr << prolog << RequestServiceTimer::TheTimer()->dump(true) << endl);
        } catch (std::exception &se) {
            ostringstream msg;
            msg << prolog << "Caught std::exception! Message: " << se.what();
            cerr << msg.str() << endl;
            CPPUNIT_FAIL(msg.str());
        } catch (...) {
            CPPUNIT_FAIL(prolog + "Caught unknown exception.");
        }
        DBG(cerr << prolog << "END" << endl);
    }

    /**
     *
     */
    void test_is_expired() {
        DBG(cerr << prolog << "BEGIN" << endl);
        try {
            milliseconds time_out{10};
            RequestServiceTimer::TheTimer()->start(time_out);
            DBG(cerr << prolog << RequestServiceTimer::TheTimer()->dump(true) << endl);
            sleep(1);
            DBG(cerr << prolog << RequestServiceTimer::TheTimer()->dump(true) << endl);
            CPPUNIT_ASSERT(RequestServiceTimer::TheTimer()->is_expired() == true);
        } catch (std::exception &se) {
            ostringstream msg;
            msg << prolog << "Caught std::exception! Message: " << se.what();
            cerr << msg.str() << endl;
            CPPUNIT_FAIL(msg.str());
        } catch (...) {
            CPPUNIT_FAIL(prolog + "Caught unknown exception.");
        }
        DBG(cerr << prolog << "END" << endl);
    }

    /**
     *
     */
    void test_throw_if_timeout_expired() {
        DBG(cerr << prolog << "BEGIN" << endl);
        try {
            milliseconds time_out{10};
            RequestServiceTimer::TheTimer()->start(time_out);
            try {
                RequestServiceTimer::TheTimer()->throw_if_timeout_expired("<This request should NOT be expired.>",
                                                                          __FILE__, __LINE__);
            } catch (BESTimeoutError btoe) {
                CPPUNIT_FAIL(prolog + "Timeout expired prematurely.");
            }
            DBG(cerr << prolog << RequestServiceTimer::TheTimer()->dump(true) << endl);
            sleep(1);
            DBG(cerr << prolog << RequestServiceTimer::TheTimer()->dump(true) << endl);

            CPPUNIT_ASSERT(RequestServiceTimer::TheTimer()->is_expired() == true);
            try {
                RequestServiceTimer::TheTimer()->throw_if_timeout_expired("<This request should be expired.>", __FILE__,
                                                                          __LINE__);
            } catch (BESTimeoutError btoe) {
                DBG(cerr << prolog
                         << "RequestServiceTimer::TheTimer()->throw_if_timeout_expired() threw a "
                            "BESTimeoutError."
                         << endl
                         << "Message: " << btoe.get_message() << endl);
                CPPUNIT_ASSERT("RequestServiceTimer::TheTimer()->throw_if_timeout_expired() threw a BESTimeoutError "
                               "as expected.");
            }

        } catch (std::exception &se) {
            ostringstream msg;
            msg << prolog << "Caught std::exception! Message: " << se.what();
            cerr << msg.str() << endl;
            CPPUNIT_FAIL(msg.str());
        } catch (...) {
            CPPUNIT_FAIL(prolog + "Caught unknown exception.");
        }
        DBG(cerr << endl << prolog << "END" << endl);
    }

    /**
     *
     */
    void test_restart_1() {
        DBG(cerr << prolog << "BEGIN" << endl);
        try {
            milliseconds time_out{100};
            RequestServiceTimer::TheTimer()->start(time_out);
            DBG(cerr << prolog << RequestServiceTimer::TheTimer()->dump(true) << endl);
            CPPUNIT_ASSERT(RequestServiceTimer::TheTimer()->is_expired() == false);
            sleep(1);
            DBG(cerr << prolog << RequestServiceTimer::TheTimer()->dump(true) << endl);
            CPPUNIT_ASSERT(RequestServiceTimer::TheTimer()->is_expired() == true);

            time_out = seconds{2};
            RequestServiceTimer::TheTimer()->start(time_out);
            CPPUNIT_ASSERT(RequestServiceTimer::TheTimer()->is_expired() == false);
            DBG(cerr << prolog << RequestServiceTimer::TheTimer()->dump(true) << endl);
            sleep(1);
            DBG(cerr << prolog << RequestServiceTimer::TheTimer()->dump(true) << endl);
            CPPUNIT_ASSERT(RequestServiceTimer::TheTimer()->is_expired() == false);

        } catch (std::exception &se) {
            ostringstream msg;
            msg << prolog << "Caught std::exception! Message: " << se.what();
            cerr << msg.str() << endl;
            CPPUNIT_FAIL(msg.str());
        } catch (...) {
            CPPUNIT_FAIL(prolog + "Caught unknown exception.");
        }
        DBG(cerr << prolog << "END" << endl);
    }

    void test_restart_2() {
        DBG(cerr << prolog << "BEGIN" << endl);
        try {
            milliseconds time_out{100};
            RequestServiceTimer::TheTimer()->start(time_out);
            DBG(cerr << prolog << RequestServiceTimer::TheTimer()->dump(true) << endl);
            sleep(1);
            DBG(cerr << prolog << RequestServiceTimer::TheTimer()->dump(true) << endl);
            CPPUNIT_ASSERT(RequestServiceTimer::TheTimer()->is_expired() == true);

            time_out = seconds{2};
            RequestServiceTimer::TheTimer()->start(time_out);
            CPPUNIT_ASSERT(RequestServiceTimer::TheTimer()->is_expired() == false);
            DBG(cerr << prolog << RequestServiceTimer::TheTimer()->dump(true) << endl);
            sleep(1);
            DBG(cerr << prolog << RequestServiceTimer::TheTimer()->dump(true) << endl);
            CPPUNIT_ASSERT(RequestServiceTimer::TheTimer()->is_expired() == false);

        } catch (std::exception &se) {
            ostringstream msg;
            msg << prolog << "Caught std::exception! Message: " << se.what();
            cerr << msg.str() << endl;
            CPPUNIT_FAIL(msg.str());
        } catch (...) {
            CPPUNIT_FAIL(prolog + "Caught unknown exception.");
        }
        DBG(cerr << prolog << "END" << endl);
    }

    CPPUNIT_TEST_SUITE(RequestTimerTest);

    CPPUNIT_TEST(test_dump);
    CPPUNIT_TEST(test_disable_timeout);
    CPPUNIT_TEST(test_no_timeout);
    CPPUNIT_TEST(test_is_expired);
    CPPUNIT_TEST(test_throw_if_timeout_expired);
    CPPUNIT_TEST(test_restart_1);
    CPPUNIT_TEST(test_restart_2);
    // CPPUNIT_TEST_EXCEPTION(test_ingest_chunk_dimension_sizes_4, BESError);

    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(RequestTimerTest);

} // namespace timer

int main(int argc, char *argv[]) {
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
            test = timer::RequestTimerTest::suite()->getName().append("::").append(argv[i]);
            wasSuccessful = wasSuccessful && runner.run(test);
            ++i;
        }
    }

    return wasSuccessful ? 0 : 1;
}
