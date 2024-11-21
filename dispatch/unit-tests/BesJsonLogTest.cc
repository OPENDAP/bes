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

#include "config.h"

#include <string>
#include <vector>
#include <sstream>

#include <unistd.h>

#include "BESInternalError.h"
#include "BESInternalFatalError.h"
#include "BESNotFoundError.h"
#include "TheBESKeys.h"

#include "modules/common/run_tests_cppunit.h"
#include "test_config.h"

#include "BesJsonLog.h"

#undef DBG
#define DBG(x) do { if (debug) x; } while(false)
#define prolog std::string("BesJsonLogTest::").append(__func__).append("() - ")

static auto BESKeys_LOG_NAME_KEY = "BES.LogName";

using namespace std;


class BesJsonLogTest : public CppUnit::TestFixture {
private:


public:
    // Called once before everything gets tested
    BesJsonLogTest() = default;

    // Called at the end of the test
    ~BesJsonLogTest() override = default;

    string log_file_name;

    // Called before each test
    void setUp() override {
		DBG(cerr << "\n");
        TheBESKeys::ConfigFile = string(TEST_BUILD_DIR).append("/bes.conf");
        DBG(cerr << prolog << "Using TheBESKeys::ConfigFile: " << TheBESKeys::ConfigFile << "\n");

        bool found;
        TheBESKeys::TheKeys()->get_value(BESKeys_LOG_NAME_KEY, log_file_name, found);

        if(!found) {
            throw new BESInternalError("Failed to locate a BES log file.", __FILE__, __LINE__);
        }
        DBG(cerr << prolog << "BES Log File Set To " << log_file_name << "\n");
    }

    // Called after each test
    void tearDown() override {
    }

    void enable_verbose_logging(){
        bool vstate = BesJsonLog::TheLog()->is_verbose();
        DBG(cerr << prolog << "The verbose log is: " << (vstate?"enabled":"disabled") << "\n");
        if(!vstate){
          	BesJsonLog::TheLog()->verbose_on();
			DBG(cerr << prolog << "The verbose log is: " << (vstate?"enabled":"disabled") << "\n");
		}
    }

    // Test that we can open the files and find all the vars in the DMR files.
    void request_log_test_1() {
        nlohmann::json j_obj;
        j_obj["user_id"] = "foobar";
        j_obj["source"] = prolog;
        DBG(cerr << prolog << "Sending JSON object to Request Log: " << j_obj.dump() << "\n");
        BesJsonLog::TheLog()->request(j_obj);
    }

    void request_log_test_2() {
        nlohmann::json j_obj;
        j_obj["user_id"] = "foobar";
        j_obj["source"] = prolog + "Testing JSON_REQUEST_LOG macro.";
        DBG(cerr << prolog << "Sending JSON object to Request Log: " << j_obj.dump() << "\n");
        JSON_REQUEST_LOG(j_obj);
    }

    void info_log_test_1() {
        string msg = prolog + "This is a test. If it had not been a test you would have known the answers.";
        DBG(cerr << prolog << "Sending string to INFO Log: " << msg << "\n");
        BesJsonLog::TheLog()->info(msg);
    }

    void info_log_test_2() {
        string msg = prolog + "Testing the JSON_INFO_LOG macro.";
        DBG(cerr << prolog << "Sending string to INFO Log: " << msg << "\n");
        JSON_INFO_LOG(msg);
    }

    void error_log_test_1() {
        string msg = prolog + "This is a test of the error log.";
        DBG(cerr << prolog << "Sending string to ERROR Log: " << msg << "\n");
        BesJsonLog::TheLog()->error(msg);
    }

    void error_log_test_2() {
        string msg = prolog + "Testing the JSON_ERROR_LOG macro.";
        DBG(cerr << prolog << "Sending string to ERROR Log: " << msg << "\n");
        JSON_ERROR_LOG(msg);
    }

    void verbose_log_test_1() {
        string msg = prolog + "This is a test of the verbose log.";
        enable_verbose_logging();
        DBG(cerr << prolog << "Sending string to VERBOSE Log: " << msg << "\n");
        BesJsonLog::TheLog()->verbose(msg);
    }

    void verbose_log_test_2() {
        string msg = prolog + "Testing the JSON_VERBOSE_LOG macro.";
        enable_verbose_logging();
        DBG(cerr << prolog << "Sending string to VERBOSE Log: " << msg << "\n");
        JSON_VERBOSE_LOG(msg);
    }

    void special_chars_log_test_1() {
        string msg = prolog + "This is a test.\t\\If it had no\"t been a test you would\" have known \n\nthe answers.";
        DBG(cerr << prolog << "Sending string to INFO Log: " << msg << "\n");
        BesJsonLog::TheLog()->info(msg);
    }


    CPPUNIT_TEST_SUITE(BesJsonLogTest);

    CPPUNIT_TEST(request_log_test_1);
    CPPUNIT_TEST(request_log_test_2);

    CPPUNIT_TEST(info_log_test_1);
    CPPUNIT_TEST(info_log_test_2);

    CPPUNIT_TEST(error_log_test_1);
    CPPUNIT_TEST(error_log_test_2);

    CPPUNIT_TEST(verbose_log_test_1);
    CPPUNIT_TEST(verbose_log_test_2);

    CPPUNIT_TEST(special_chars_log_test_1);

    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(BesJsonLogTest);


int main(int argc, char *argv[]) {
    return bes_run_tests<BesJsonLogTest>(argc, argv, "cerr,json_log") ? 0: 1;
}
