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

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>

#include <cppunit/extensions/HelperMacros.h>

// rapidjson
#include <stringbuffer.h>
#include <writer.h>
#include <document.h>

#include <BESRegex.h>
#include <BESDebug.h>

#include "history_utils.h"

#include "modules/common/run_tests_cppunit.h"
#include "test_config.h"

using namespace std;
using namespace fonc_history_util;

#define prolog std::string("HistoryUtilsTest::").append(__func__).append("() - ")

class HistoryUtilsTest: public CppUnit::TestFixture {

private:
    string d_tmpDir = string(TEST_BUILD_DIR) + "/tmp";
    string date_time_regex = R"([0-9]{4}-[0-9]{2}-[0-9]{2} [0-9]{2}:[0-9]{2}:[0-9]{2})";
public:

    // Called once before everything gets tested
    HistoryUtilsTest() = default;

    // Called at the end of the test
    ~HistoryUtilsTest() = default;

    // These tests don't define specializations of void setUp() OR void tearDown().
    // jhrg 2/25/22

    void test_get_time_now() {
        string date_time = get_time_now();
        //  "%Y-%m-%d %H:%M:%S"
        BESRegex r(date_time_regex);
        DBG(cerr << "Test_get_time_now: " << date_time << ", match: " << r.match(date_time) << endl);
        CPPUNIT_ASSERT_MESSAGE("Time string did not match regex", r.match(date_time) > 0);
    }

    void test_create_cf_history_txt_1() {
        const string url = { R"(https://machine.domain.tld/place/stuff.ext)" };
        string cf_history = create_cf_history_txt(url);

        // 2022-02-25 14:17:26 Hyrax https://machine.domain.tld/place/stuff.ext
        BESRegex r(string(date_time_regex).append(" Hyrax ").append(url));
        DBG(cerr << "test_create_cf_history_txt_1: " << cf_history << ", match: " << r.match(cf_history) << endl);

        CPPUNIT_ASSERT_MESSAGE("New CF History did not match", r.match(cf_history) > 0);
    }

    void test_create_cf_history_txt_2() {
        const string url = { "" };
        string cf_history = create_cf_history_txt(url);

        // 2022-02-25 14:17:26 Hyrax
        BESRegex r(string(date_time_regex).append(" Hyrax ").append(url));
        DBG(cerr << "test_create_cf_history_txt_2: " << cf_history << ", match: " << r.match(cf_history) << endl);

        CPPUNIT_ASSERT_MESSAGE("New CF History did not match with an empty request url", r.match(cf_history) > 0);
    }

    void test_create_json_history_obj_1() {
        const string url = { R"(https://machine.domain.tld/place/stuff.ext)" };

        rapidjson::StringBuffer buffer;
        rapidjson::Writer <rapidjson::StringBuffer> writer(buffer);
        create_json_history_obj(url, writer);

        //  {"$schema":"https://harmony.earthdata.nasa.gov/schemas/history/0.1.0/history-0.1.0.json",
        //  "date_time":"2022-02-25 15:09:05","program":"hyrax","version":"1.16.3",
        //  "parameters":[{"request_url":"https://machine.domain.tld/place/stuff.ext"}]}
        //
        // In this regex I use .+ to make sure that the request_url is not an empty string.
        // jhrg 2/25/22
        string json_regex = R"(\{"\$schema":".*","date_time":".*",.*,"parameters":\[\{"request_url":".+"\}\]\})";
        DBG(cerr << "test_create_json_history_obj_1: ---" << json_regex << "---" << endl);
        BESRegex r(json_regex);

        DBG(cerr << "test_create_json_history_obj_1: " << buffer.GetString() << ", match: " << r.match(buffer.GetString()) << endl);

        CPPUNIT_ASSERT_MESSAGE("New CF History did not match", r.match(buffer.GetString()) > 0);
    }

    void test_create_json_history_obj_2() {
        const string url = "";

        rapidjson::StringBuffer buffer;
        rapidjson::Writer <rapidjson::StringBuffer> writer(buffer);
        create_json_history_obj(url, writer);

        //  {"$schema":"https://harmony.earthdata.nasa.gov/schemas/history/0.1.0/history-0.1.0.json",
        //  "date_time":"2022-02-25 15:09:05","program":"hyrax","version":"1.16.3",
        //  "parameters":[{"request_url":""}]}
        //
        // In this regex request_url is an empty string.
        // jhrg 2/25/22
        string json_regex = R"(\{"\$schema":".*","date_time":".*",.*,"parameters":\[\{"request_url":""\}\]\})";
        DBG(cerr << "test_create_json_history_obj_2: ---" << json_regex << "---" << endl);
        BESRegex r(json_regex);

        DBG(cerr << "test_create_json_history_obj_2: " << buffer.GetString() << ", match: " << r.match(buffer.GetString()) << endl);

        CPPUNIT_ASSERT_MESSAGE("New CF History did not match", r.match(buffer.GetString()) > 0);
    }

    void json_append_entry_to_array_test()
    {
        DBG(cerr << prolog << "BEGIN" << endl);

        string target_array = R"([ {"thing1":"one_fish"}, {"thing2":"two_fish"} ])";
        string new_entry = R"({"thing3":"red_fish"})";
        string expected = R"([{"thing1":"one_fish"},{"thing2":"two_fish"},{"thing3":"red_fish"}])";
        string result = json_append_entry_to_array(target_array,new_entry);

        DBG(cerr << prolog << "target_array: " << target_array << endl);
        DBG(cerr << prolog << "new_entry: " << new_entry << endl);
        DBG(cerr << prolog << "result: " << result << endl);

        CPPUNIT_ASSERT( result == expected );

        new_entry = R"({"thing4":"blue_fish"})";
        expected = R"([{"thing1":"one_fish"},{"thing2":"two_fish"},{"thing3":"red_fish"},{"thing4":"blue_fish"}])";

        result = json_append_entry_to_array(result,new_entry);
        DBG(cerr << prolog << "new_entry: " << new_entry << endl);
        DBG(cerr << prolog << "result: " << result << endl);
        CPPUNIT_ASSERT( result == expected );

        DBG(cerr << prolog << "END" << endl);
    }

    CPPUNIT_TEST_SUITE( HistoryUtilsTest );

    CPPUNIT_TEST(test_get_time_now);
    CPPUNIT_TEST(test_create_cf_history_txt_1);
    CPPUNIT_TEST(test_create_cf_history_txt_2);
    CPPUNIT_TEST(test_create_json_history_obj_1);
    CPPUNIT_TEST(test_create_json_history_obj_2);


    CPPUNIT_TEST(json_append_entry_to_array_test);

    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(HistoryUtilsTest);

int main(int argc, char *argv[])
{
    return bes_run_tests<HistoryUtilsTest>(argc, argv, "cerr,fonc") ? 0: 1;
}

