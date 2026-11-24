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

// Tests that 'error' log records end with the request URL path and CE fields,
// and that other record types are unchanged. TASK-005 (HYRAX-2334)

#include "test_config.h"

#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/extensions/TestFactoryRegistry.h>

#include <unistd.h>

#include "BESDebug.h"
#include "BESLog.h"
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
#define prolog std::string("BESLogTest::").append(__func__).append("() - ")

// Not *.log so it cannot collide with the automake test driver's BESLogTest.log.
static const string test_log_name = "./beslog_test.txt";

namespace bes_log_test {

class BESLogTest : public CppUnit::TestFixture {

    static vector<string> read_log_lines() {
        ifstream ifs(test_log_name);
        vector<string> lines;
        string line;
        while (getline(ifs, line))
            lines.push_back(line);
        return lines;
    }

    // Return the last line in the log that contains marker.
    static string find_line(const string &marker) {
        string found;
        for (const auto &line : read_log_lines()) {
            if (line.find(marker) != string::npos)
                found = line;
        }
        return found;
    }

    static vector<string> split(const string &line) {
        vector<string> fields;
        size_t start = 0;
        size_t pos;
        while ((pos = line.find(BESLog::mark, start)) != string::npos) {
            fields.push_back(line.substr(start, pos - start));
            start = pos + BESLog::mark.size();
        }
        fields.push_back(line.substr(start));
        return fields;
    }

    static bool ends_with(const string &s, const string &suffix) {
        return s.size() >= suffix.size() && s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
    }

public:
    BESLogTest() = default;
    ~BESLogTest() override = default;

    void setUp() override {
        if (debug)
            cerr << endl;
        if (bes_debug)
            BESDebug::SetUp("cerr,bes");

        // BESLog is a singleton that reads BES.LogName when first used, so this must
        // be set before any test touches the log.
        TheBESKeys::ConfigFile = string(TEST_BUILD_DIR).append("/bes.conf");
        TheBESKeys::TheKeys()->set_key("BES.LogName", test_log_name);
        BESLog::TheLog()->reset_request_url_and_ce();
    }

    void test_sanitize_empty() { CPPUNIT_ASSERT_EQUAL(string("-"), BESLog::sanitize_log_field("")); }

    void test_sanitize_plain() {
        CPPUNIT_ASSERT_EQUAL(string("/opendap/data/nc/fnoc1.nc.dap"),
                             BESLog::sanitize_log_field("/opendap/data/nc/fnoc1.nc.dap"));
    }

    void test_sanitize_crlf() { CPPUNIT_ASSERT_EQUAL(string("a  b c"), BESLog::sanitize_log_field("a\r\nb\nc")); }

    void test_sanitize_mark() {
        CPPUNIT_ASSERT_EQUAL(string("u|&amp;|v|&amp;|w"), BESLog::sanitize_log_field("u|&|v|&|w"));
    }

    void test_error_record_fields() {
        CPPUNIT_ASSERT_EQUAL(string("boom|&|/opendap/x.nc.dap|&|u;v"),
                             BESLog::error_record_fields("boom", "/opendap/x.nc.dap", "u;v"));
    }

    void test_error_record_fields_empty() {
        CPPUNIT_ASSERT_EQUAL(string("boom|&|-|&|-"), BESLog::error_record_fields("boom", "", ""));
    }

    void test_error_record_fields_trailing_newline() {
        CPPUNIT_ASSERT_EQUAL(string("boom|&|-|&|-"), BESLog::error_record_fields("boom\n", "", ""));
        CPPUNIT_ASSERT_EQUAL(string("boom|&|-|&|-"), BESLog::error_record_fields("boom\r\n\n", "", ""));
    }

    void test_error_record_fields_sanitizes() {
        CPPUNIT_ASSERT_EQUAL(string("boom|&|/a b|&|x|&amp;|y"),
                             BESLog::error_record_fields("boom", "/a\nb", "x|&|y"));
    }

    void test_setters() {
        BESLog::TheLog()->set_request_url("/opendap/x.nc.dmr");
        BESLog::TheLog()->set_request_ce("");
        CPPUNIT_ASSERT_EQUAL(string("/opendap/x.nc.dmr"), BESLog::TheLog()->get_request_url());
        CPPUNIT_ASSERT_EQUAL(string("-"), BESLog::TheLog()->get_request_ce());

        BESLog::TheLog()->reset_request_url_and_ce();
        CPPUNIT_ASSERT_EQUAL(string("-"), BESLog::TheLog()->get_request_url());
        CPPUNIT_ASSERT_EQUAL(string("-"), BESLog::TheLog()->get_request_ce());
    }

    void test_error_log_has_url_and_ce() {
        BESLog::TheLog()->set_request_url("/opendap/data/nc/fnoc1.nc.dap");
        BESLog::TheLog()->set_request_ce("dap4.ce=/u;/v");
        ERROR_LOG("error_marker_1");

        const string line = find_line("error_marker_1");
        DBG(cerr << prolog << "line: " << line << endl);
        const auto fields = split(line);
        CPPUNIT_ASSERT_EQUAL_MESSAGE(line, size_t(8), fields.size());
        CPPUNIT_ASSERT_EQUAL(string("error"), fields[4]);
        CPPUNIT_ASSERT_EQUAL(string("error_marker_1"), fields[5]);
        CPPUNIT_ASSERT_EQUAL(string("/opendap/data/nc/fnoc1.nc.dap"), fields[6]);
        CPPUNIT_ASSERT_EQUAL(string("dap4.ce=/u;/v"), fields[7]);
    }

    void test_error_log_defaults() {
        ERROR_LOG("error_marker_2\n");

        const string line = find_line("error_marker_2");
        DBG(cerr << prolog << "line: " << line << endl);
        CPPUNIT_ASSERT_MESSAGE(line, ends_with(line, "|&|error|&|error_marker_2|&|-|&|-"));
    }

    void test_info_log_unchanged() {
        BESLog::TheLog()->set_request_url("/opendap/data/nc/fnoc1.nc.dap");
        BESLog::TheLog()->set_request_ce("u");
        INFO_LOG("info_marker_1");

        const string line = find_line("info_marker_1");
        DBG(cerr << prolog << "line: " << line << endl);
        CPPUNIT_ASSERT_MESSAGE(line, ends_with(line, "|&|info|&|info_marker_1"));
        CPPUNIT_ASSERT_EQUAL_MESSAGE(line, size_t(6), split(line).size());
    }

    CPPUNIT_TEST_SUITE(BESLogTest);

    CPPUNIT_TEST(test_sanitize_empty);
    CPPUNIT_TEST(test_sanitize_plain);
    CPPUNIT_TEST(test_sanitize_crlf);
    CPPUNIT_TEST(test_sanitize_mark);
    CPPUNIT_TEST(test_error_record_fields);
    CPPUNIT_TEST(test_error_record_fields_empty);
    CPPUNIT_TEST(test_error_record_fields_trailing_newline);
    CPPUNIT_TEST(test_error_record_fields_sanitizes);
    CPPUNIT_TEST(test_setters);
    CPPUNIT_TEST(test_error_log_has_url_and_ce);
    CPPUNIT_TEST(test_error_log_defaults);
    CPPUNIT_TEST(test_info_log_unchanged);

    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(BESLogTest);

} // namespace bes_log_test

int main(int argc, char *argv[]) {
    // Start from an empty log; BESLog opens it in append mode.
    std::remove(test_log_name.c_str());

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
            test = bes_log_test::BESLogTest::suite()->getName().append("::").append(argv[i]);
            wasSuccessful = wasSuccessful && runner.run(test);
            ++i;
        }
    }

    return wasSuccessful ? 0 : 1;
}
