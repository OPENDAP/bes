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
#include "BESContextManager.h"

#include "HttpNames.h"
#include "EffectiveUrlCache.h"

#include "test_config.h"

using namespace std;

static bool debug = false;
static bool Debug = false;
static bool bes_debug = false;
static bool purge_cache = false;
static bool ngap_tests = false;
static std::string token;

#undef DBG
#define DBG(x) do { if (debug) x; } while(false)
#define prolog std::string("EffectiveUrlCacheTest::").append(__func__).append("() - ")

namespace http {

    class EffectiveUrlCacheTest: public CppUnit::TestFixture {
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
                cerr << endl << "#############################################################################" << endl;
                cerr << "file: " << filename << endl;
                cerr <<         ". . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . " << endl;
                cerr << file_content << endl;
                cerr << "#############################################################################" << endl;
            }
            else {
                cerr << "FAILED TO OPEN FILE: " << filename << endl;
            }
        }

        std::string get_file_as_string(string filename)
        {
            ifstream t(filename.c_str());

            if (t.is_open()) {
                string file_content((istreambuf_iterator<char>(t)), istreambuf_iterator<char>());
                t.close();
                if(Debug) cerr << endl << "#############################################################################" << endl;
                if(Debug) cerr << "file: " << filename << endl;
                if(Debug) cerr <<         ". . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . " << endl;
                if(Debug) cerr << file_content << endl;
                if(Debug) cerr << "#############################################################################" << endl;
                return file_content;
            }
            else {
                cerr << "FAILED TO OPEN FILE: " << filename << endl;
                CPPUNIT_ASSERT(false);
                return "";
            }
        }

        /**
         *
         */
        string get_data_file_url(string name){
            string data_file = BESUtil::assemblePath(d_data_dir,name);
            if(debug) cerr << prolog << "data_file: " << data_file << endl;
            if(Debug) show_file(data_file);

            string data_file_url = "file://" + data_file;
            if(debug) cerr << prolog << "data_file_url: " << data_file_url << endl;
            return data_file_url;
        }

        void purge_test_cache(){
            if(debug) cerr << prolog << "BEGIN" << endl;
            string cache_dir;
            bool found;
            TheBESKeys::TheKeys()->get_value(HTTP_CACHE_DIR_KEY,cache_dir,found);
            if(found){
                if(Debug) cerr << HTTP_CACHE_DIR_KEY << ": " <<  cache_dir << endl;
                if(Debug) cerr << "Purging " << cache_dir << endl;
                string cmd = "exec rm -r "+ BESUtil::assemblePath(cache_dir,"/*");
                system(cmd.c_str());
            }
            if(debug) cerr << prolog << "END" << endl;
        }

    public:
        string d_data_dir;

        // Called once before everything gets tested
        EffectiveUrlCacheTest()
        {
            d_data_dir = TEST_DATA_DIR;;
        }

        // Called at the end of the test
        ~EffectiveUrlCacheTest()
        {
        }

        // Called before each test
        void setUp()
        {
            if(debug) cerr << endl;
            if(Debug) cerr << prolog << "BEGIN" << endl;
            if(debug) cerr << prolog << "data_dir: " << d_data_dir << endl;
            string bes_conf = BESUtil::assemblePath(TEST_BUILD_DIR,"bes.conf");
            if(Debug) cerr << prolog << "Using BES configuration: " << bes_conf << endl;
            if (Debug) show_file(bes_conf);
            TheBESKeys::ConfigFile = bes_conf;

            if (bes_debug) BESDebug::SetUp("cerr,bes,euc,http");

            if(purge_cache) purge_test_cache();

            // Clear the cache for the next test.
            EffectiveUrlCache::TheCache()->delete_instance();

            if(!token.empty()){
                if(debug) cerr << "Setting BESContext " << EDL_AUTH_TOKEN_KEY<< " to: '"<< token << "'" << endl;
                BESContextManager::TheManager()->set_context(EDL_AUTH_TOKEN_KEY,token);
            }
            if(Debug) cerr << prolog << "END" << endl;
        }

        // Called after each test
        void tearDown()
        {
        }


/*##################################################################################################*/
/* TESTS BEGIN */
        void is_cache_disabled_test() {
            if(debug) cerr << prolog << "EffectiveUrlCache::TheCache()->is_enabled(): "
                           << (EffectiveUrlCache::TheCache()->is_enabled()?"true":"false") << endl;
            CPPUNIT_ASSERT( !EffectiveUrlCache::TheCache()->is_enabled() );



            string src_url_00 = "http://started_here.com";
            http::EffectiveUrl *effective_url_00 = new http::EffectiveUrl("https://ended_here.com");

            EffectiveUrlCache::TheCache()->add(src_url_00,effective_url_00);
            CPPUNIT_ASSERT( EffectiveUrlCache::TheCache()->d_effective_urls.size() == 1);


            // This one does not add the URL or even check it because it _should_ be matching the skip regex.
            http::EffectiveUrl *result_url =EffectiveUrlCache::TheCache()->get_effective_url(src_url_00);
            CPPUNIT_ASSERT( !result_url );


        }


        void skip_regex_test(){
            if(debug) cerr << prolog << "BEGIN" << endl;
            string source_url;
            string value;
            http::EffectiveUrl *result_url;
            try {

                // This one does not add the URL or even check it because it _should_ be matching the skip regex
                // in the bes.conf
                string src_url_03 = "https://foobar.com/opendap/data/nc/fnoc1.nc?dap4.ce=u;v";
                result_url =EffectiveUrlCache::TheCache()->get_effective_url(src_url_03);
                CPPUNIT_ASSERT( EffectiveUrlCache::TheCache()->d_effective_urls.size() == 0);
                CPPUNIT_ASSERT( !result_url );

            }
            catch (BESError be){
                stringstream msg;
                msg << __func__ << "() - ERROR! Caught BESError. Message: " << be.get_message() << endl;
                CPPUNIT_FAIL(msg.str());

            }
        }


        void cache_test_00() {
            if(debug) cerr << prolog << "BEGIN" << endl;
            string source_url;
            string value;
            http::EffectiveUrl *result_url;
            try {
                // The cache is disabled in bes.conf so we need to turn it on.
                EffectiveUrlCache::TheCache()->d_enabled = true;

                string src_url_00 = "https://d1jecqxxv88lkr.cloudfront.net/ghrcwuat-protected/rss_demo/rssmif16d__7/f16_ssmis_20040107v7.nc";
                auto *effective_url_00 = new http::EffectiveUrl("https://ghrcwuat-protected.s3.us-west-2.amazonaws.com/rss_demo/rssmif16d__7/f16_ssmis_20031229v7.nc?A-userid=hyrax&X-Amz-Algorithm=AWS4-HMAC-SHA256&X-Amz-Credential=ASIASF4N-AWS-Creds-00808%2Fus-west-2%2Fs3%2Faws4_request&X-Amz-Date=20200808T032623Z&X-Amz-Expires=86400&X-Amz-Security-Token=FwoGZXIvYXdzE-AWS-Sec-Token-MWRLIZGYvDx1ONzd0ffK8VtxO8JP7thrGIQ%3D%3D&X-Amz-SignedHeaders=host&X-Amz-Signature=260a7c4dd4-AWS-SIGGY-0c7a39ee899");
                EffectiveUrlCache::TheCache()->add(src_url_00,effective_url_00);
                CPPUNIT_ASSERT( EffectiveUrlCache::TheCache()->d_effective_urls.size() == 1);

                string src_url_01 = "http://test.opendap.org/data/httpd_catalog/READTHIS";
                auto *effective_url_01 = new http::EffectiveUrl("https://test.opendap.org/data/httpd_catalog/READTHIS");
                EffectiveUrlCache::TheCache()->add(src_url_01,effective_url_01);
                CPPUNIT_ASSERT( EffectiveUrlCache::TheCache()->d_effective_urls.size() == 2);

                // This one actually does the thing
                string src_url_02 = "http://test.opendap.org/opendap";
                auto *effective_url_02 = new http::EffectiveUrl("http://test.opendap.org/opendap/");

                if(debug) cerr << prolog << "Retrieving effective URL for: " << src_url_02 << endl;
                result_url = EffectiveUrlCache::TheCache()->get_effective_url(src_url_02);
                CPPUNIT_ASSERT( EffectiveUrlCache::TheCache()->d_effective_urls.size() == 3);

                if(debug) cerr << prolog << "EffectiveUrlCache::TheCache()->get_effective_url() returned: " <<
                               (result_url?result_url->str():"NULL") << endl;
                CPPUNIT_ASSERT(result_url);
                CPPUNIT_ASSERT(result_url->str() == effective_url_02->str());

            }
            catch (BESError be){
                stringstream msg;
                msg << __func__ << "() - ERROR! Caught BESError. Message: " << be.get_message() << endl;
                CPPUNIT_FAIL(msg.str());

            }
            if(debug) cerr << prolog << "END" << endl;
        }
        void euc_ghrc_tea_url_test() {
            if(!ngap_tests){
                if(debug) cerr << prolog << "SKIPPING." << endl;
                return;
            }
            if(debug) cerr << prolog << "BEGIN" << endl;
            string source_url;
            string value;
            http::EffectiveUrl *result_url;
            try {
                // The cache is disabled in bes.conf so we need to turn it on.
                EffectiveUrlCache::TheCache()->d_enabled = true;
                string thing1 = "https://d1jecqxxv88lkr.cloudfront.net/ghrcwuat-protected/rss_demo/rssmif16d__7/f16_ssmis_20031026v7.nc";
                string thing1_out_of_region_effective_url_prefix = "https://d1jecqxxv88lkr.cloudfront.net/s3";
                string thing1_in_region_effective_url_prefix = "https://ghrcwuat-protected.s3.us-west-2.amazonaws.com/";

                if(debug) cerr << prolog << "Retrieving effective URL for: " << thing1 << endl;
                result_url = EffectiveUrlCache::TheCache()->get_effective_url(thing1);
                CPPUNIT_ASSERT( EffectiveUrlCache::TheCache()->d_effective_urls.size() == 1);

                if(debug) cerr << prolog << "EffectiveUrlCache::TheCache()->get_effective_url() returned: " <<
                               (result_url?result_url->str():"NULL") << endl;
                CPPUNIT_ASSERT(result_url);

                CPPUNIT_ASSERT(
                        result_url->str().rfind(thing1_in_region_effective_url_prefix, 0) == 0 ||
                        result_url->str().rfind(thing1_out_of_region_effective_url_prefix, 0) == 0
                );

                result_url = EffectiveUrlCache::TheCache()->get_effective_url(thing1);

            }
            catch (BESError be){
                stringstream msg;
                msg << __func__ << "() - ERROR! Caught BESError. Message: " << be.get_message() << endl;
                CPPUNIT_FAIL(msg.str());

            }
            if(debug) cerr << prolog << "END" << endl;
        }

        void euc_harmony_url_test() {
            if(!ngap_tests){
                if(debug) cerr << prolog << "SKIPPING." << endl;
                return;
            }
            if(debug) cerr << prolog << "BEGIN" << endl;
            string source_url;
            string value;
            http::EffectiveUrl *result_url;
            try {
                // The cache is disabled in bes.conf so we need to turn it on.
                EffectiveUrlCache::TheCache()->d_enabled = true;
                string thing1 = "https://harmony.uat.earthdata.nasa.gov/service-results/harmony-uat-staging/public/"
                                "sds/staged/ATL03_20200714235814_03000802_003_01.h5";
                string thing1_out_of_region_effective_url_prefix = "https://djpip0737hawz.cloudfront.net/s3";
                string thing1_in_region_effective_url_prefix = "https://harmony-uat-staging.s3.us-west-2.amazonaws.com/public/";

                if(debug) cerr << prolog << "Retrieving effective URL for: " << thing1 << endl;
                result_url = EffectiveUrlCache::TheCache()->get_effective_url(thing1);
                CPPUNIT_ASSERT( EffectiveUrlCache::TheCache()->d_effective_urls.size() == 1);

                if(debug) cerr << prolog << "EffectiveUrlCache::TheCache()->get_effective_url() returned: " <<
                               (result_url?result_url->str():"NULL") << endl;
                CPPUNIT_ASSERT(result_url);

                CPPUNIT_ASSERT(
                        result_url->str().rfind(thing1_in_region_effective_url_prefix, 0) == 0 ||
                                result_url->str().rfind(thing1_out_of_region_effective_url_prefix, 0) == 0
                );


                result_url = EffectiveUrlCache::TheCache()->get_effective_url(thing1);


            }
            catch (BESError be){
                stringstream msg;
                msg << __func__ << "() - ERROR! Caught BESError. Message: " << be.get_message() << endl;
                CPPUNIT_FAIL(msg.str());

            }
            if(debug) cerr << prolog << "END" << endl;
        }
#if 0
        string get_amz_date(const time_t &da_time){
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

            if(debug) cout << "amz_date: " << amz_date.str() << " len: " << amz_date.str().length() << endl;
            return amz_date.str();
        }
#endif

/* TESTS END */
/*##################################################################################################*/


    CPPUNIT_TEST_SUITE( EffectiveUrlCacheTest );

            CPPUNIT_TEST(is_cache_disabled_test);
            CPPUNIT_TEST(cache_test_00);
            CPPUNIT_TEST(skip_regex_test);
            CPPUNIT_TEST(euc_ghrc_tea_url_test);
            CPPUNIT_TEST(euc_harmony_url_test);

    CPPUNIT_TEST_SUITE_END();
};

    CPPUNIT_TEST_SUITE_REGISTRATION(EffectiveUrlCacheTest);

} // namespace httpd_catalog

int main(int argc, char*argv[])
{
    CppUnit::TextTestRunner runner;
    runner.addTest(CppUnit::TestFactoryRegistry::getRegistry().makeTest());

    GetOpt getopt(argc, argv, "dbDPt:N");
    int option_char;
    while ((option_char = getopt()) != -1)
        switch (option_char) {
            case 'd':
                debug = true;  // debug is a static global
                cerr << "debug enabled" << endl;
                break;
            case 'D':
                Debug = true;  // Debug is a static global
                cerr << "Debug enabled" << endl;
                break;
            case 'b':
                bes_debug = true;  // debug is a static global
                cerr << "bes_debug enabled" << endl;
                break;
            case 'N':
                ngap_tests = true; // token is a static global
                cerr << "NGAP Tests Enabled." << token << endl;
                break;
            case 'P':
                purge_cache = true;  // purge_cache is a static global
                cerr << "purge_cache enabled" << endl;
                break;
            case 't':
                token = getopt.optarg; // token is a static global
                cerr << "Authorization header value: " << token << endl;
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
            test = http::EffectiveUrlCacheTest::suite()->getName().append("::").append(argv[i]);
            wasSuccessful = wasSuccessful && runner.run(test);
            ++i;
        }
    }

    return wasSuccessful ? 0 : 1;
}
