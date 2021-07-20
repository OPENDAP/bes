// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of the BES component of the Hyrax Data Server.

// Copyright (c) 2020 OPeNDAP, Inc.
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

#include "config.h"

#include <memory>
#include <cstdio>
#include <cstring>
#include <iostream>

#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>

#include <unistd.h>
#include <util.h>

#include "BESError.h"
#include "BESDebug.h"
#include "BESUtil.h"
#include "BESCatalogList.h"
#include "TheBESKeys.h"
#include "BESContextManager.h"
#include "CurlUtils.h"
#include "HttpNames.h"

#include "test_config.h"

using namespace std;

static bool debug = false;
static bool Debug = false;
static bool bes_debug = false;
static bool purge_cache = false;

#undef DBG
#define DBG(x) do { if (debug) x; } while(false)

#define prolog std::string("CurlUtilsTest::").append(__func__).append("() - ")

namespace http {

    class CurlUtilsTest: public CppUnit::TestFixture {
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



    public:
        string d_data_dir;

        // Called once before everything gets tested
        CurlUtilsTest()
        {
            d_data_dir = TEST_DATA_DIR;;
            cerr << "data_dir: " << d_data_dir << endl;
        }

        // Called at the end of the test
        ~CurlUtilsTest()
        {
        }

        // Called before each test
        void setUp()
        {
            if(Debug || debug ) cerr << endl;
            if(Debug) cerr << "setUp() - BEGIN" << endl;
            string bes_conf = BESUtil::assemblePath(TEST_BUILD_DIR,"bes.conf");
            if(Debug) cerr << "setUp() - Using BES configuration: " << bes_conf << endl;
            if (bes_debug) show_file(bes_conf);
            TheBESKeys::ConfigFile = bes_conf;

            if (bes_debug) BESDebug::SetUp("cerr,bes,http,curl");

            if(Debug) cerr << "setUp() - END" << endl;
        }

        // Called after each test
        void tearDown()
        {
        }


/*##################################################################################################*/
/* TESTS BEGIN */

        void is_retryable_test() {
            if(debug) cerr << prolog << "BEGIN" << endl;
            bool isRetryable;

            try {
                string url = "http://test.opendap.org/data/httpd_catalog/READTHIS";
                isRetryable = curl::is_retryable(url);
                if(debug) cerr << prolog << "is_retryable('" << url << "'): " << (isRetryable ? "true" : "false") << endl;
                CPPUNIT_ASSERT(isRetryable);

                url = "https://ghrcwuat-protected.s3.us-west-2.amazonaws.com/rss_demo/rssmif16d__7/f16_ssmis_20031229v7.nc?A-userid=hyrax&X-Amz-Algorithm=AWS4-HMAC-SHA256&X-Amz-Credential=ASIASF4N-AWS-Creds-00808%2Fus-west-2%2Fs3%2Faws4_request&X-Amz-Date=20200808T032623Z&X-Amz-Expires=86400&X-Amz-Security-Token=FwoGZXIvYXdzE-AWS-Sec-Token-MWRLIZGYvDx1ONzd0ffK8VtxO8JP7thrGIQ%3D%3D&X-Amz-SignedHeaders=host&X-Amz-Signature=260a7c4dd4-AWS-SIGGY-0c7a39ee899";
                isRetryable = curl::is_retryable(url);
                if(debug) cerr << prolog << "is_retryable('" << url << "'): " << (isRetryable ? "true" : "false") << endl;
                CPPUNIT_ASSERT(!isRetryable);

                url = "https://d1jecqxxv88lkr.cloudfront.net/ghrcwuat-protected/rss_demo/rssmif16d__7/f16_ssmis_20040107v7.nc";
                isRetryable = curl::is_retryable(url);
                if(debug) cerr << prolog << "is_retryable('" << url << "'): " << (isRetryable ? "true" : "false") << endl;
                CPPUNIT_ASSERT(isRetryable);

                url = "https://data.ghrc.uat.earthdata.nasa.gov/login?code=8800da07f823dfce312ee85e44c9e89efdf6bd9d776b1cb8666029ba2c8d257e&state=%2Fghrcwuat%2Dprotected%2Frss_demo%2Frssmif16d__7%2Ff16_ssmis_20040107v7%2Enc";
                isRetryable = curl::is_retryable(url);
                if(debug) cerr << prolog << "is_retryable('" << url << "'): " << (isRetryable ? "true" : "false") << endl;
                CPPUNIT_ASSERT(!isRetryable);

                url = "https://data.ghrc.uat.earthdata.nasa.gov/login?code=46196589bfe26c4c298e1a74646b99005d20a022cabff6434a550283defa8153&state=%2Fghrcwuat%2Dprotected%2Frss_demo%2Frssmif16d__7%2Ff16_ssmis_20040115v7%2Enc";
                isRetryable = curl::is_retryable(url);
                if(debug) cerr << prolog << "is_retryable('" << url << "'): " << (isRetryable ? "true" : "false") << endl;
                CPPUNIT_ASSERT(!isRetryable);
            }
            catch (BESError &be){
                stringstream msg;
                msg << prolog << "ERROR! Caught BESError. Message: " << be.get_message() << endl;
                CPPUNIT_FAIL(msg.str());

            }
            catch( std::exception &se ){
                stringstream msg;
                msg << "CAUGHT std::exception message: " << se.what() << endl;
                cerr << msg.str();
                CPPUNIT_FAIL(msg.str());
            }
        if(debug)  cerr << prolog << "END" << endl;
        }


        void retrieve_effective_url_test(){
            if(debug) cerr << prolog << "BEGIN" << endl;
            shared_ptr<http::url> trusted_target_url(new http::url("http://test.opendap.org/opendap",true));
            shared_ptr<http::url> target_url(new http::url("http://test.opendap.org/opendap",false));
            string expected_url = "http://test.opendap.org/opendap/";
            EffectiveUrl *effective_url;

            try {
                if(debug) cerr << prolog << "   target_url: " << target_url->str() << endl;

                auto effective_url = curl::retrieve_effective_url(target_url);

                if(debug) cerr << prolog << "effective_url: " << effective_url->str() << endl;
                if(debug) cerr << prolog << " expected_url: " << expected_url << endl;
                CPPUNIT_ASSERT( effective_url->str() == expected_url );

                if(debug) cerr << prolog << "   target_url is " << (target_url->is_trusted()?"":"NOT ")<< "trusted." << endl;
                if(debug) cerr << prolog << "effective_url is " << (effective_url->is_trusted()?"":"NOT ")<< "trusted." << endl;
                CPPUNIT_ASSERT( effective_url->is_trusted() == target_url->is_trusted() );



                effective_url = curl::retrieve_effective_url(trusted_target_url);

                if(debug) cerr << prolog << "effective_url: " << effective_url->str() << endl;
                if(debug) cerr << prolog << " expected_url: " << expected_url << endl;
                CPPUNIT_ASSERT( effective_url->str() == expected_url );

                if(debug) cerr << prolog << "   target_url is " << (trusted_target_url->is_trusted()?"":"NOT ")<< "trusted." << endl;
                if(debug) cerr << prolog << "effective_url is " << (effective_url->is_trusted()?"":"NOT ")<< "trusted." << endl;
                CPPUNIT_ASSERT( effective_url->is_trusted() == trusted_target_url->is_trusted() );


            }
            catch(BESError &be){
                stringstream msg;
                msg << "Caught BESError! Message: " << be.get_message() << " file: " << be.get_file() << " line: " << be.get_line()<< endl;
                CPPUNIT_FAIL(msg.str());
            }
            catch( std::exception &se ){
                stringstream msg;
                msg << "CAUGHT std::exception message: " << se.what() << endl;
                cerr << msg.str();
                CPPUNIT_FAIL(msg.str());
            }
            if(debug) cerr << prolog << "END" << endl;
        }

        /**
         * struct curl_slist {  char *data;  struct curl_slist *next;};
         */
        void add_edl_auth_headers_test(){
            if(debug) cerr << prolog << "BEGIN" << endl;
            curl_slist *hdrs=NULL;
            curl_slist *temp=NULL;
            string tokens[]= {"big_bucky_ball","itsa_authy_token_time","echo_my_smo:kin_token"};
            BESContextManager::TheManager()->set_context(EDL_UID_KEY, tokens[0]);
            BESContextManager::TheManager()->set_context(EDL_AUTH_TOKEN_KEY, tokens[1]);
            BESContextManager::TheManager()->set_context(EDL_ECHO_TOKEN_KEY, tokens[2]);

            try {
                hdrs = curl::add_edl_auth_headers(hdrs);
                temp=hdrs;
                size_t index = 0;
                while(temp){
                    string value(temp->data);
                    if(debug) cerr << prolog << "header: " << value << endl;
                    size_t found = value.find(tokens[index]);
                    CPPUNIT_ASSERT( found != string::npos);
                    temp = temp->next;
                    index++;
                }
            }
            catch(BESError &be){
                stringstream msg;
                msg << "Caught BESError! Message: " << be.get_message() << " file: " << be.get_file() << " line: " << be.get_line()<< endl;
                CPPUNIT_FAIL(msg.str());
            }
            catch( std::exception &se ){
                stringstream msg;
                msg << "CAUGHT std::exception message: " << se.what() << endl;
                cerr << msg.str();
                CPPUNIT_FAIL(msg.str());
            }
            if(debug) cerr << prolog << "END" << endl;
        }



/* TESTS END */
/*##################################################################################################*/


    CPPUNIT_TEST_SUITE( CurlUtilsTest );

            CPPUNIT_TEST(is_retryable_test);
            CPPUNIT_TEST(retrieve_effective_url_test);
            CPPUNIT_TEST(add_edl_auth_headers_test);

        CPPUNIT_TEST_SUITE_END();
    };

    CPPUNIT_TEST_SUITE_REGISTRATION(CurlUtilsTest);

} // namespace httpd_catalog

int main(int argc, char*argv[])
{
    CppUnit::TextTestRunner runner;
    runner.addTest(CppUnit::TestFactoryRegistry::getRegistry().makeTest());

    int option_char;
    while ((option_char = getopt(argc, argv, "dbD")) != -1)
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
            default:
                break;
        }

    argc -= optind;
    argv += optind;

    bool wasSuccessful = true;
    string test = "";
    if (0 == argc) {
        // run them all
        wasSuccessful = runner.run("");
    }
    else {
        int i = 0;
        while (i < argc) {
            if (debug) cerr << "Running " << argv[i] << endl;
            test = http::CurlUtilsTest::suite()->getName().append("::").append(argv[i]);
            wasSuccessful = wasSuccessful && runner.run(test);
            ++i;
        }
    }

    return wasSuccessful ? 0 : 1;
}
