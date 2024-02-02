// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of the BES component of the Hyrax Data Server.

// Copyright (c) 2018 OPeNDAP, Inc.
// Author: Nathan Potter <ndp@opendap.org>
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

#include <memory>
#include <iostream>
#include <thread>         // std::this_thread::sleep_for
#include <chrono>
#include <ctime>

#include "BESError.h"
#include "BESDebug.h"
#include "BESUtil.h"
#include "TheBESKeys.h"

#include "AllowedHosts.h"
#include "HttpNames.h"

#include "modules/common/run_tests_cppunit.h"
#include "test_config.h"

using namespace std;

#define prolog std::string("HttpUrlTest::").append(__func__).append("() - ")

namespace http {

class HttpUrlTest: public CppUnit::TestFixture {
private:
    void show_file(const string &filename) const  {
        ifstream t(filename.c_str());

        if (t.is_open()) {
            string file_content((istreambuf_iterator<char>(t)), istreambuf_iterator<char>());
            t.close();
            cerr << endl;
            cerr << "#############################################################################" << endl;
            cerr << "file: " << filename << endl;
            cerr << ". . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . ." << endl;
            cerr << file_content << endl;
            cerr << "#############################################################################" << endl;
        }
        else {
            cerr << prolog << "FAILED TO OPEN FILE: " << filename << endl;
        }
    }

public:
    string d_data_dir;

    // Called once before everything gets tested
    HttpUrlTest() = default;

    // Called at the end of the test
    ~HttpUrlTest() override = default;

    // Called before each test
    void setUp() override {
        DBG2(cerr << endl);
        DBG2(cerr << prolog << "BEGIN" << endl);

        d_data_dir = TEST_DATA_DIR;;
        DBG2(cerr << prolog << "data_dir: " << d_data_dir << endl);

        string bes_conf = BESUtil::assemblePath(TEST_BUILD_DIR,"bes.conf");
        DBG2(cerr << prolog << "setUp() - Using BES configuration: " << bes_conf << endl);
        DBG2(show_file(bes_conf));
        TheBESKeys::ConfigFile = bes_conf;

        DBG2(cerr << prolog << "END" << endl);
    }

    /**
     * Takes a time_t value dat_time and converts it into an X-Amz-Date header formatted string
     * @param date_time The time_t value to convert to an X-Amz_Date header formatted string.
     * @return an X-Amz-Date header formatted string from date_time
     */
    string get_amz_date(const time_t &date_time){
        DBG(cerr << prolog << "BEGIN" << endl);
        // string amz_date_format("%Y%m%dT%H%M%SZ"); // 20200808T032623Z
        struct tm dttm{};
        gmtime_r(&date_time, &dttm);

        vector<char> amz_date(32, 0);
        snprintf(amz_date.data(), 32, "%4d%02d%02dT%02d%02d%02dZ", dttm.tm_year + 1900,
                 dttm.tm_mon + 1, dttm.tm_mday, dttm.tm_hour, dttm.tm_min, dttm.tm_sec);
        DBG(cout << "Built amz_date: " << string(amz_date.data()) << " from time: " << date_time << endl);
        DBG(cerr << prolog << "END" << endl);
        return amz_date.data();
    }

/*##################################################################################################*/
/* TESTS BEGIN */
    string expired_source_url = "https://ghrcwuat-protected.s3.us-west-2.amazonaws.com/rss_demo/rssmif16d__7/f16_ssmis_20031229v7.nc?A-userid=hyrax&X-Amz-Algorithm=AWS4-HMAC-SHA256&X-Amz-Credential=ASIASF4N-AWS-Creds-00808%2Fus-west-2%2Fs3%2Faws4_request&X-Amz-Date=20200808T032623Z&X-Amz-Expires=86400&X-Amz-Security-Token=FwoGZXIvYXdzE-AWS-Sec-Token-MWRLIZGYvDx1ONzd0ffK8VtxO8JP7thrGIQ%3D%3D&X-Amz-SignedHeaders=host&X-Amz-Signature=260a7c4dd4-AWS-SIGGY-0c7a39ee899";
    string now_template_url = "https://ghrcwuat-protected.s3.us-west-2.amazonaws.com/rss_demo/rssmif16d__7/f16_ssmis_20031229v7.nc?A-userid=hyrax&X-Amz-Algorithm=AWS4-HMAC-SHA256&X-Amz-Credential=ASIASF4N-AWS-Creds-00808%2Fus-west-2%2Fs3%2Faws4_request&X-Amz-Date=AMAZON_DATE_VALUE&X-Amz-Expires=86400&X-Amz-Security-Token=FwoGZXIvYXdzE-AWS-Sec-Token-MWRLIZGYvDx1ONzd0ffK8VtxO8JP7thrGIQ%3D%3D&X-Amz-SignedHeaders=host&X-Amz-Signature=260a7c4dd4-AWS-SIGGY-0c7a39ee899";
    string amz_date_template = "AMAZON_DATE_VALUE";

    void url_ingest_test_01() {
        DBG(cerr << prolog << "BEGIN" << endl);
        http::url url(expired_source_url);
        CPPUNIT_ASSERT(url.protocol() == HTTPS_PROTOCOL);
        CPPUNIT_ASSERT(url.host() == "ghrcwuat-protected.s3.us-west-2.amazonaws.com");
        CPPUNIT_ASSERT(url.path() == "/rss_demo/rssmif16d__7/f16_ssmis_20031229v7.nc");

        CPPUNIT_ASSERT( url.query_parameter_value("A-userid") == "hyrax");
        CPPUNIT_ASSERT( url.query_parameter_value("X-Amz-Signature") == "260a7c4dd4-AWS-SIGGY-0c7a39ee899");
        // Verify the keys are case-sensitive
        CPPUNIT_ASSERT( url.query_parameter_value("x-amz-signature") != "260a7c4dd4-AWS-SIGGY-0c7a39ee899");
        DBG(cerr << prolog << "END" << endl);
    }

    void url_ingest_test_02() {
        DBG(cerr << prolog << "BEGIN" << endl);
        http::url url("http://www.example.com/search?q=cats&q=dogs&q=birds&A-userid=A-userid");
        CPPUNIT_ASSERT(url.protocol() == HTTP_PROTOCOL);
        CPPUNIT_ASSERT(url.host() == "www.example.com");
        CPPUNIT_ASSERT(url.path() == "/search");

        CPPUNIT_ASSERT(url.query_parameter_value("A-userid") == "A-userid");
        CPPUNIT_ASSERT(url.query_parameter_value("q") == "cats");
        // Verify the keys are case-sensitive
        CPPUNIT_ASSERT(url.query_parameter_values_size("q") == 3);
        const auto &q_values = url.query_parameter_values("q");
        CPPUNIT_ASSERT_EQUAL_MESSAGE("expected cats", string("cats"), q_values[0]);
        CPPUNIT_ASSERT_EQUAL_MESSAGE("expected dogs", string("dogs"), q_values[1]);
        CPPUNIT_ASSERT_EQUAL_MESSAGE("expected birds", string("birds"), q_values[2]);
        CPPUNIT_ASSERT_EQUAL_MESSAGE("expected 3", 3UL, q_values.size());
        DBG(cerr << prolog << "END" << endl);
    }

    void file_url_test_01() {
        DBG(cerr << prolog << "BEGIN" << endl);

        string url_str = FILE_PROTOCOL "/etc/password";
        shared_ptr<http::url> file_url(new http::url(url_str,true));
        bool allowed = AllowedHosts::theHosts()->is_allowed(file_url);
        DBG(cerr << prolog << "url: " << file_url->str() << " is " << (allowed?"":"NOT ") << "allowed. " << endl);
        CPPUNIT_ASSERT(!allowed);

        DBG(cerr << prolog << "END" << endl);
    }

    void file_url_test_02() {
        DBG(cerr << prolog << "BEGIN" << endl);

        string url_str = FILE_PROTOCOL TEST_DATA_DIR;
        shared_ptr<http::url> file_url(new http::url(url_str,true));
        bool allowed = AllowedHosts::theHosts()->is_allowed(file_url);
        DBG(cerr << prolog << "url: " << file_url->str() << " is " << (allowed?"":"NOT ") << "allowed. " << endl);
        CPPUNIT_ASSERT(allowed);

        DBG(cerr << prolog << "END" << endl);
    }

    void file_url_test_03() {
        DBG(cerr << prolog << "BEGIN" << endl);

        string url_str = FILE_PROTOCOL TEST_DATA_DIR;
        http::url url(url_str,true);
        CPPUNIT_ASSERT_EQUAL_MESSAGE("Expected 'file://'", string("file://"), url.protocol());
        CPPUNIT_ASSERT_EQUAL_MESSAGE("Expected ''", string(""), url.host());
        CPPUNIT_ASSERT_EQUAL_MESSAGE(string("Expected ").append(TEST_DATA_DIR), string(TEST_DATA_DIR), url.path());
        CPPUNIT_ASSERT_EQUAL_MESSAGE("Expected 0", 0UL, url.d_query_kvp.size());

        DBG(cerr << prolog << "END" << endl);
    }

    /**
    * without file protocol, with leading slash
    */
    void file_url_no_protocol_with_leading_slash() {
        DBG(cerr << prolog << "BEGIN" << endl);

        string url_str =  "/etc/password";
        string expected = FILE_PROTOCOL TEST_DATA_DIR + url_str;
        shared_ptr<http::url> file_url(new http::url(url_str,true));
        DBG(cerr << prolog << " created: " << file_url->str() << endl);
        DBG(cerr << prolog << "expected: " << expected << endl);
        CPPUNIT_ASSERT(file_url->str() == expected);

        DBG(cerr << prolog << "END" << endl);
    }

    /**
     * without file protocol no leading slash
     */
    void file_url_no_protocol_without_leading_slash() {
        DBG(cerr << prolog << "BEGIN" << endl);

        string url_str =  "etc/password";
        string expected = FILE_PROTOCOL TEST_DATA_DIR "/" + url_str;
        shared_ptr<http::url> file_url(new http::url(url_str,true));
        DBG(cerr << prolog << " created: " << file_url->str() << endl);
        DBG(cerr << prolog << "expected: " << expected << endl);
        CPPUNIT_ASSERT(file_url->str() == expected);

        DBG(cerr << prolog << "END" << endl);
    }

    /**
     * Test copy constructor including trusted attribute
     */
    void copy_constructor_trusted_test(){
        DBG(cerr << prolog << "BEGIN" << endl);
        http::url test_url("https://foo.com/barney/was/knots",true);

        CPPUNIT_ASSERT(test_url.protocol() == HTTPS_PROTOCOL);
        CPPUNIT_ASSERT(test_url.host() == "foo.com");
        CPPUNIT_ASSERT(test_url.path() == "/barney/was/knots");
        CPPUNIT_ASSERT(test_url.is_trusted());

        http::url copy_url(test_url);
        CPPUNIT_ASSERT(copy_url.protocol() == HTTPS_PROTOCOL);
        CPPUNIT_ASSERT(copy_url.host() == "foo.com");
        CPPUNIT_ASSERT(copy_url.path() == "/barney/was/knots");
        CPPUNIT_ASSERT(copy_url.is_trusted());

        DBG(cerr << prolog << "END" << endl);
    }

    void copy_constructor_untrusted_test() {
        DBG(cerr << prolog << "BEGIN" << endl);
        http::url test_url("https://foo.com/barney/was/knots",false);

        CPPUNIT_ASSERT(test_url.protocol() == HTTPS_PROTOCOL);
        CPPUNIT_ASSERT(test_url.host() == "foo.com");
        CPPUNIT_ASSERT(test_url.path() == "/barney/was/knots");
        CPPUNIT_ASSERT(!test_url.is_trusted());

        http::url copy_url(test_url);
        CPPUNIT_ASSERT(copy_url.protocol() == HTTPS_PROTOCOL);
        CPPUNIT_ASSERT(copy_url.host() == "foo.com");
        CPPUNIT_ASSERT(copy_url.path() == "/barney/was/knots");
        CPPUNIT_ASSERT(!copy_url.is_trusted());

        // Test copy constructor including trusted attribute.
        DBG(cerr << prolog << "END" << endl);
    }


    void url_is_expired_test() {
        DBG(cerr << prolog << "BEGIN" << endl);
        try {
            http::url old_url(expired_source_url);
            DBG(cerr << prolog << "old_url: " << old_url.str() << endl);
            CPPUNIT_ASSERT_EQUAL_MESSAGE("Expected true", true, old_url.is_expired());

            time_t now;
            time(&now);
            string x_amz_date = get_amz_date(now);
            DBG(cerr << prolog << "Now as an AMZ date: " << x_amz_date << endl);

            string source_url = now_template_url;
            size_t index = source_url.find(amz_date_template);
            source_url.erase(index,amz_date_template.size());
            source_url.insert(index,x_amz_date);
            http::url now_url(source_url);
            DBG(cerr << prolog << "now_url: " << now_url.str() << endl);
            CPPUNIT_ASSERT_EQUAL_MESSAGE("Expected false", false, now_url.is_expired());
        }
        catch (const BESError &be){
            stringstream msg;
            msg << "() - ERROR! Caught BESError. Message: " << be.get_message() << endl;
            CPPUNIT_FAIL(msg.str());
        }
        DBG(cerr << prolog << "END" << endl);
    }

    /**
     * @brief THis test is run only when -D is used
     * This test sleeps for four seconds and exercises C++ chrono, but it
     * does not test http::url.
     */
    void chrono_test() {
        if (!debug2)
            return ;

        DBG2(cerr << prolog << "BEGIN" << endl);
        long long nap_time = 2;
        std::time_t today_tt;
        std::chrono::system_clock::time_point today = std::chrono::system_clock::now();
        today_tt = std::chrono::system_clock::to_time_t ( today );
        DBG2(cerr  << prolog << "today is: " << ctime(&today_tt) << endl);

        {
            DBG2(cerr << prolog << "std::chrono::steady_clock" << endl);

            std::chrono::steady_clock::time_point cnow = std::chrono::steady_clock::now();
            long long now = cnow.time_since_epoch().count();
            auto now_secs = std::chrono::time_point_cast<std::chrono::seconds>(cnow);
            DBG2(cerr << prolog << "   now: " << now << " converted: " << now_secs.time_since_epoch().count()  << endl);

            std::this_thread::sleep_for (std::chrono::seconds(nap_time));

            auto lnow = std::chrono::steady_clock::now();
            long long later = lnow.time_since_epoch().count();
            auto later_secs = std::chrono::time_point_cast<std::chrono::seconds>(lnow);
            DBG2(cerr << prolog << " later: " << later << " converted: " << later_secs.time_since_epoch().count() << endl);
            DBG2(cerr << prolog << "  diff: " << later_secs.time_since_epoch().count() - now_secs.time_since_epoch().count() << endl);
        }
        {
            DBG2(cerr << prolog << "std::chrono::system_clock" << endl);
            auto cnow = std::chrono::system_clock::now();
            std::time_t now_tt = std::chrono::system_clock::to_time_t(cnow);
            long long now_count = cnow.time_since_epoch().count();
            auto n2 =  std::chrono::time_point_cast<std::chrono::seconds>(cnow);
            DBG2(cerr << prolog << "     now_count: " << now_count << " converted: " << n2.time_since_epoch().count() << endl);
            DBG2(cerr << prolog << "        now_tt: " << now_tt << endl);

            std::this_thread::sleep_for (std::chrono::seconds(nap_time));

            auto lnow = std::chrono::system_clock::now();
            std::time_t later_tt = std::chrono::system_clock::to_time_t(lnow);
            long long lnow_count = lnow.time_since_epoch().count();
            auto l2 =  std::chrono::time_point_cast<std::chrono::seconds>(lnow);
            DBG2(cerr << prolog << "    lnow_count: " << lnow_count << " converted: " << l2.time_since_epoch().count() << endl);
            DBG2(cerr << prolog << "      later_tt: " << later_tt << endl);
            DBG2(cerr << prolog << "          diff: " << lnow_count - now_count << endl);
            DBG2(cerr << prolog << "diff_converted: " << l2.time_since_epoch().count() - n2.time_since_epoch().count() << endl);
            DBG2(cerr << prolog << "       diff_tt: " << later_tt - now_tt << endl);
        }

        DBG2(cerr << prolog << "END" << endl);
    }

/* TESTS END */
/*##################################################################################################*/

    CPPUNIT_TEST_SUITE( HttpUrlTest );

        CPPUNIT_TEST(url_ingest_test_01);
        CPPUNIT_TEST(url_ingest_test_02);

        CPPUNIT_TEST(file_url_test_01);
        CPPUNIT_TEST(file_url_test_02);
        CPPUNIT_TEST(file_url_test_03);

        CPPUNIT_TEST(file_url_no_protocol_with_leading_slash);
        CPPUNIT_TEST(file_url_no_protocol_without_leading_slash);

        CPPUNIT_TEST(copy_constructor_trusted_test);
        CPPUNIT_TEST(copy_constructor_untrusted_test);

        CPPUNIT_TEST(url_is_expired_test);
        
        // This test is run only with the '-D' option
        CPPUNIT_TEST(chrono_test);

    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(HttpUrlTest);

} // namespace httpd_catalog

int main(int argc, char *argv[]) {
    return bes_run_tests<http::HttpUrlTest>(argc, argv, "bes,bes,http,ah,curl") ? 0: 1;
}
