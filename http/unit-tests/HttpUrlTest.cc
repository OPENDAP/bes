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
#include <cstdio>
#include <cstring>
#include <iostream>
#include <thread>         // std::this_thread::sleep_for
#include <chrono>
#include <time.h>

#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>

#include <GetOpt.h>
#include <util.h>

#include "BESError.h"
#include "BESDebug.h"
#include "BESUtil.h"
#include "BESCatalogList.h"
#include "TheBESKeys.h"

#include "AllowedHosts.h"
#include "EffectiveUrlCache.h"
#include "HttpNames.h"

#include "test_config.h"


using namespace std;

static bool debug = false;
static bool Debug = false;
static bool bes_debug = false;
static bool purge_cache = false;

#define prolog std::string("HttpUrlTest::").append(__func__).append("() - ")

namespace http {

class HttpUrlTest: public CppUnit::TestFixture {
private:

    /**
     *
     */
    void show_file(string filename)
    {
        ifstream t(filename.c_str());

        if (t.is_open()) {
            string file_content((istreambuf_iterator<char>(t)), istreambuf_iterator<char>());
            t.close();
            cerr << prolog << endl << "#############################################################################" << endl;
            cerr << prolog << "file: " << filename << endl;
            cerr << prolog <<         ". . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . " << endl;
            cerr << prolog << file_content << endl;
            cerr << prolog << "#############################################################################" << endl;
        }
        else {
            cerr << prolog << "FAILED TO OPEN FILE: " << filename << endl;
        }
    }

    std::string get_file_as_string(string filename)
    {
        ifstream t(filename.c_str());

        if (t.is_open()) {
            string file_content((istreambuf_iterator<char>(t)), istreambuf_iterator<char>());
            t.close();
            if(Debug) cerr << prolog << endl << "#############################################################################" << endl;
            if(Debug) cerr << prolog << "file: " << filename << endl;
            if(Debug) cerr << prolog <<         ". . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . " << endl;
            if(Debug) cerr << prolog << file_content << endl;
            if(Debug) cerr << prolog << "#############################################################################" << endl;
            return file_content;
        }
        else {
            cerr << prolog << "FAILED TO OPEN FILE: " << filename << endl;
            CPPUNIT_ASSERT(false);
            return "";
        }
    }

    /**
     *
     */
    string get_data_file_url(string name){
        string data_file = BESUtil::assemblePath(d_data_dir,name);
        if(debug) cerr << prolog << prolog << "data_file: " << data_file << endl;
        if(Debug) show_file(data_file);

        string data_file_url = "file://" + data_file;
        if(debug) cerr << prolog << prolog << "data_file_url: " << data_file_url << endl;
        return data_file_url;
    }



public:
    string d_data_dir;

    // Called once before everything gets tested
    HttpUrlTest()
    {
    }

    // Called at the end of the test
    ~HttpUrlTest()
    {
    }

    // Called before each test
    void setUp()
    {
        if(debug) cerr << endl;
        if(debug) cerr << prolog << "BEGIN" << endl;

        d_data_dir = TEST_DATA_DIR;;
        if(debug)  cerr << prolog << "data_dir: " << d_data_dir << endl;

        string bes_conf = BESUtil::assemblePath(TEST_BUILD_DIR,"bes.conf");
        if(Debug) cerr << prolog << "setUp() - Using BES configuration: " << bes_conf << endl;
        if (bes_debug) show_file(bes_conf);
        TheBESKeys::ConfigFile = bes_conf;

        if (bes_debug) BESDebug::SetUp("cerr,bes,http,ah");


        if(purge_cache){
            if(Debug) cerr << prolog << "Purging cache!" << endl;
            string cache_dir;
            bool found;
            TheBESKeys::TheKeys()->get_value(HTTP_CACHE_DIR_KEY,cache_dir,found);
            if(found){
                if(Debug) cerr << prolog << HTTP_CACHE_DIR_KEY << ": " <<  cache_dir << endl;
                if(Debug) cerr << prolog << "Purging " << cache_dir << endl;
                string cmd = "exec rm -r "+ BESUtil::assemblePath(cache_dir,"/*");
                system(cmd.c_str());
            }
        }


        if(debug) cerr << prolog << "END" << endl;
    }

    // Called after each test
    void tearDown()
    {
    }

    /**
     * Takes a time_t value dat_time and converts it into an X-Amz-Date header formatted string
     * @param dat_time The time_t value to convert to an X-Amz_Date header formatted string.
     * @return an X-Amz-Date header formatted string from dat_time
     */
    string get_amz_date(const time_t &da_time){
        if(debug) cerr << prolog << "BEGIN" << endl;
        string amz_date_format("%Y%m%dT%H%M%SZ"); // "20200808T032623Z";
        struct tm *dttm;
        dttm = gmtime (&da_time);
        int value;

        stringstream amz_date;
        value = dttm->tm_year + 1900 ;
        amz_date << value;
        value = dttm->tm_mon + 1 ;
        amz_date << (value<10?"0":"") << value;
        value = dttm->tm_mday;
        amz_date << (value<10?"0":"") << value;

        amz_date << "T";

        value = dttm->tm_hour;
        amz_date << (value<10?"0":"") << value;
        value = dttm->tm_min;
        amz_date << (value<10?"0":"") << value;
        value = dttm->tm_sec;
        amz_date << (value<10?"0":"") << value;
        amz_date << "Z";

        if(debug) cout << "Built amz_date: " << amz_date.str() << " from time: " << da_time << endl;
        if(debug) cerr << prolog << "END" << endl;
        return amz_date.str();
    }

/*##################################################################################################*/
/* TESTS BEGIN */
    string expired_source_url = "https://ghrcwuat-protected.s3.us-west-2.amazonaws.com/rss_demo/rssmif16d__7/f16_ssmis_20031229v7.nc?A-userid=hyrax&X-Amz-Algorithm=AWS4-HMAC-SHA256&X-Amz-Credential=ASIASF4N-AWS-Creds-00808%2Fus-west-2%2Fs3%2Faws4_request&X-Amz-Date=20200808T032623Z&X-Amz-Expires=86400&X-Amz-Security-Token=FwoGZXIvYXdzE-AWS-Sec-Token-MWRLIZGYvDx1ONzd0ffK8VtxO8JP7thrGIQ%3D%3D&X-Amz-SignedHeaders=host&X-Amz-Signature=260a7c4dd4-AWS-SIGGY-0c7a39ee899";
    string now_template_url="https://ghrcwuat-protected.s3.us-west-2.amazonaws.com/rss_demo/rssmif16d__7/f16_ssmis_20031229v7.nc?A-userid=hyrax&X-Amz-Algorithm=AWS4-HMAC-SHA256&X-Amz-Credential=ASIASF4N-AWS-Creds-00808%2Fus-west-2%2Fs3%2Faws4_request&X-Amz-Date=AMAZON_DATE_VALUE&X-Amz-Expires=86400&X-Amz-Security-Token=FwoGZXIvYXdzE-AWS-Sec-Token-MWRLIZGYvDx1ONzd0ffK8VtxO8JP7thrGIQ%3D%3D&X-Amz-SignedHeaders=host&X-Amz-Signature=260a7c4dd4-AWS-SIGGY-0c7a39ee899";
    string amz_date_template="AMAZON_DATE_VALUE";

    void url_ingest_test(){
        if(debug) cerr << prolog << "BEGIN" << endl;
        http::url url(expired_source_url);
        CPPUNIT_ASSERT(url.protocol() == HTTPS_PROTOCOL);
        CPPUNIT_ASSERT(url.host() == "ghrcwuat-protected.s3.us-west-2.amazonaws.com");
        CPPUNIT_ASSERT(url.path() == "/rss_demo/rssmif16d__7/f16_ssmis_20031229v7.nc");

        CPPUNIT_ASSERT( url.query_parameter_value("A-userid") == "hyrax");
        CPPUNIT_ASSERT( url.query_parameter_value("X-Amz-Signature") == "260a7c4dd4-AWS-SIGGY-0c7a39ee899");
        // Verify the keys are case sensitive
        CPPUNIT_ASSERT( url.query_parameter_value("x-amz-signature") != "260a7c4dd4-AWS-SIGGY-0c7a39ee899");
        if(debug) cerr << prolog << "END" << endl;
    }


    void url_is_expired_test() {
        if(debug) cerr << prolog << "BEGIN" << endl;
        string source_url;

        try {
            http::url old_url(expired_source_url);
            if(debug) cerr << prolog << "old_url: " << old_url.str() << endl;
            CPPUNIT_ASSERT( old_url.is_expired() );

            time_t now;
            time ( &now );
            string x_amz_date = get_amz_date(now);

            source_url = now_template_url;
            size_t index = source_url.find(amz_date_template);
            source_url.erase(index,amz_date_template.length());
            source_url.insert(index,x_amz_date);
            http::url now_url(source_url);
            if(debug) cerr << prolog << "now_url: " << now_url.str() << endl;
            CPPUNIT_ASSERT( !now_url.is_expired() );

        }
        catch (BESError be){
            stringstream msg;
            msg << "() - ERROR! Caught BESError. Message: " << be.get_message() << endl;
            CPPUNIT_FAIL(msg.str());

        }
        if(debug) cerr << prolog << "END" << endl;
    }


    void chrono_test(){
        if(debug) cerr << prolog << "BEGIN" << endl;
        long long nap_time = 2;
        std::time_t today_tt;
        std::chrono::system_clock::time_point today = std::chrono::system_clock::now();
        today_tt = std::chrono::system_clock::to_time_t ( today );
        if(debug) cerr  << prolog << "today is: " << ctime(&today_tt) << endl;

        {
            if(debug) cerr << prolog << "std::chrono::steady_clock" << endl;

            std::chrono::steady_clock::time_point cnow = std::chrono::steady_clock::now();
            long long now = cnow.time_since_epoch().count();
            auto now_secs = std::chrono::time_point_cast<std::chrono::seconds>(cnow);
            if(debug) cerr << prolog << "   now: " << now << " converted: " << now_secs.time_since_epoch().count()  << endl;

            std::this_thread::sleep_for (std::chrono::seconds(nap_time));

            auto lnow = std::chrono::steady_clock::now();
            long long later = lnow.time_since_epoch().count();
            auto later_secs = std::chrono::time_point_cast<std::chrono::seconds>(lnow);
            if(debug) cerr << prolog << " later: " << later << " converted: " << later_secs.time_since_epoch().count() << endl;
            if(debug) cerr << prolog << "  diff: " << later_secs.time_since_epoch().count() - now_secs.time_since_epoch().count() << endl;
        }
        {
            if(debug) cerr << prolog << "std::chrono::system_clock" << endl;
            auto cnow = std::chrono::system_clock::now();
            std::time_t now_tt = std::chrono::system_clock::to_time_t(cnow);
            long long now_count = cnow.time_since_epoch().count();
            auto n2 =  std::chrono::time_point_cast<std::chrono::seconds>(cnow);
            if(debug) cerr << prolog << "     now_count: " << now_count << " converted: " << n2.time_since_epoch().count() << endl;
            if(debug) cerr << prolog << "        now_tt: " << now_tt << endl;

            std::this_thread::sleep_for (std::chrono::seconds(nap_time));

            auto lnow = std::chrono::system_clock::now();
            std::time_t later_tt = std::chrono::system_clock::to_time_t(lnow);
            long long lnow_count = lnow.time_since_epoch().count();
            auto l2 =  std::chrono::time_point_cast<std::chrono::seconds>(lnow);
            if(debug) cerr << prolog << "    lnow_count: " << lnow_count << " converted: " << l2.time_since_epoch().count() << endl;
            if(debug) cerr << prolog << "      later_tt: " << later_tt << endl;
            if(debug) cerr << prolog << "          diff: " << lnow_count - now_count << endl;
            if(debug) cerr << prolog << "diff_converted: " << l2.time_since_epoch().count() - n2.time_since_epoch().count() << endl;
            if(debug) cerr << prolog << "       diff_tt: " << later_tt - now_tt << endl;
        }
        if(debug) cerr << prolog << "END" << endl;
    }


    void file_url_test_01(){
        if(debug) cerr << prolog << "BEGIN" << endl;

        string url_str = FILE_PROTOCOL "/etc/password";
        shared_ptr<http::url> file_url(new http::url(url_str,true));
        bool allowed = AllowedHosts::theHosts()->is_allowed(file_url);
        if(debug) cerr << prolog << "url: " << file_url->str() << " is " << (allowed?"":"NOT ") << "allowed. " << endl;
        CPPUNIT_ASSERT(!allowed);


        url_str = FILE_PROTOCOL TEST_DATA_DIR;
        file_url = shared_ptr<http::url>(new http::url(url_str,true));
        allowed = AllowedHosts::theHosts()->is_allowed(file_url);
        if(debug) cerr << prolog << "url: " << file_url->str() << " is " << (allowed?"":"NOT ") << "allowed. " << endl;
        CPPUNIT_ASSERT(allowed);

        if(debug) cerr << prolog << "END" << endl;
    }
    /**
    * without file protocol, with leading slash
    */
    void file_url_no_protocol_with_leading_slash(){
        if(debug) cerr << prolog << "BEGIN" << endl;
        string url_str =  "/etc/password";
        string expected = FILE_PROTOCOL TEST_DATA_DIR + url_str;
        shared_ptr<http::url> file_url(new http::url(url_str,true));
        if(debug) cerr << prolog << " created: " << file_url->str() << endl;
        if(debug) cerr << prolog << "expected: " << expected << endl;
        CPPUNIT_ASSERT(file_url->str() == expected);

        if(debug) cerr << prolog << "END" << endl;
    }


    /**
     * without file protocol no leading slash
     */
    void file_url_no_protocol_without_leading_slash(){
        if(debug) cerr << prolog << "BEGIN" << endl;
        string url_str =  "etc/password";
        string expected = FILE_PROTOCOL TEST_DATA_DIR "/" + url_str;
        shared_ptr<http::url> file_url(new http::url(url_str,true));
        if(debug) cerr << prolog << " created: " << file_url->str() << endl;
        if(debug) cerr << prolog << "expected: " << expected << endl;
        CPPUNIT_ASSERT(file_url->str() == expected);

        if(debug) cerr << prolog << "END" << endl;
    }

    /**
     * Test copy constructor including trusted attribute
     */
    void copy_constructor_trusted_test(){
        if(debug) cerr << prolog << "BEGIN" << endl;
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

        // Test copy constructir including trusted attribute.
        if(debug) cerr << prolog << "END" << endl;
    }

    void copy_constructor_untrusted_test(){
        if(debug) cerr << prolog << "BEGIN" << endl;
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

        // Test copy constructir including trusted attribute.
        if(debug) cerr << prolog << "END" << endl;
    }

/* TESTS END */
/*##################################################################################################*/


CPPUNIT_TEST_SUITE( HttpUrlTest );

        CPPUNIT_TEST(file_url_test_01);
        CPPUNIT_TEST(file_url_no_protocol_with_leading_slash);
        CPPUNIT_TEST(file_url_no_protocol_without_leading_slash);
        CPPUNIT_TEST(copy_constructor_trusted_test);
        CPPUNIT_TEST(copy_constructor_untrusted_test);
        CPPUNIT_TEST(url_is_expired_test);
        CPPUNIT_TEST(url_ingest_test);
        CPPUNIT_TEST(chrono_test);

    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(HttpUrlTest);

} // namespace httpd_catalog

int main(int argc, char*argv[])
{
    CppUnit::TextTestRunner runner;
    runner.addTest(CppUnit::TestFactoryRegistry::getRegistry().makeTest());

    GetOpt getopt(argc, argv, "dbDP");
    int option_char;
    while ((option_char = getopt()) != -1)
        switch (option_char) {
            case 'd':
                debug = true;  // debug is a static global
                cerr << prolog << "debug enabled" << endl;
                break;
            case 'D':
                Debug = true;  // Debug is a static global
                cerr << prolog << "Debug enabled" << endl;
                break;
            case 'b':
                bes_debug = true;  // debug is a static global
                cerr << prolog << "bes_debug enabled" << endl;
                break;
            case 'P':
                purge_cache = true;  // purge_cache is a static global
                cerr << prolog << "purge_cache enabled" << endl;
                break;
            default:
                break;
        }

    bool wasSuccessful = true;
    string test = "";
    int i = getopt.optind;
    if (i == argc) {
        // run them all
        wasSuccessful = runner.run("");
    }
    else {
        while (i < argc) {
            if (debug) cerr << "Running " << argv[i] << endl;
            test = http::HttpUrlTest::suite()->getName().append("::").append(argv[i]);
            wasSuccessful = wasSuccessful && runner.run(test);
            ++i;
        }
    }

    return wasSuccessful ? 0 : 1;
}
