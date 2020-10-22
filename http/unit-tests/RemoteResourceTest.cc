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

#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>

#include <GetOpt.h>
#include <util.h>

#include <BESError.h>
#include <BESDebug.h>
#include <BESUtil.h>
#include <BESCatalogList.h>
#include <TheBESKeys.h>
#include "RemoteResource.h"
#include "HttpNames.h"

#include "test_config.h"

using namespace std;

static bool debug = false;
static bool Debug = false;
static bool bes_debug = false;
static bool purge_cache = false;

#undef DBG
#define DBG(x) do { if (debug) x; } while(false)
#define prolog std::string("RemoteResourceTest::").append(__func__).append("() - ")

namespace http {

class RemoteResourceTest: public CppUnit::TestFixture {
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
            stringstream msg;
            msg << prolog << "FAILED TO OPEN FILE: " << filename << endl;
            CPPUNIT_FAIL( msg.str());
        }
        return "";
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
    RemoteResourceTest()
    {
        d_data_dir = TEST_DATA_DIR;;
    }

    // Called at the end of the test
    ~RemoteResourceTest()
    {
    }

    // Called before each test
    void setUp()
    {
        if(debug) cerr << "data_dir: " << d_data_dir << endl;
        if(Debug) cerr << endl << prolog << "BEGIN" << endl;
        string bes_conf = BESUtil::assemblePath(TEST_BUILD_DIR,"bes.conf");
        if(Debug) cerr << prolog << "Using BES configuration: " << bes_conf << endl;
        if (bes_debug) show_file(bes_conf);
        TheBESKeys::ConfigFile = bes_conf;

        if (bes_debug) BESDebug::SetUp("cerr,rr,bes,http");


        if(purge_cache){
            if(Debug) cerr << prolog << "Purging cache!" << endl;
            string cache_dir;
            bool found;
            TheBESKeys::TheKeys()->get_value(HTTP_CACHE_DIR_KEY,cache_dir,found);
            if(found){
                if(Debug) cerr << prolog << HTTP_CACHE_DIR_KEY << ": " <<  cache_dir << endl;
                if(Debug) cerr << prolog << "Purging " << cache_dir << endl;
                string sys_cmd = "mkdir -p "+ cache_dir;
                system(sys_cmd.c_str());
                sys_cmd = "exec rm -rf "+ BESUtil::assemblePath(cache_dir,"/*");
                system(sys_cmd.c_str());
                if(Debug) cerr << prolog << cache_dir  << " has been purged." << endl;
            }
        }


        if(Debug) cerr << "setUp() - END" << endl;
    }

    // Called after each test
    void tearDown()
    {
    }


/*##################################################################################################*/
/* TESTS BEGIN */


    void get_http_url_test() {

        string url = "http://test.opendap.org/data/httpd_catalog/READTHIS";
        if(debug) cerr << prolog << "url: " << url << endl;
        http::RemoteResource rhr(url);
        try {
            rhr.retrieveResource();
            vector<string> *hdrs = rhr.getResponseHeaders();

            for(size_t i=0; i<hdrs->size() && debug ; i++){
                cerr << prolog << "hdr["<< i << "]: " << (*hdrs)[i] << endl;
            }
            string cache_filename = rhr.getCacheFileName();
            if(debug) cerr << prolog << "cache_filename: " << cache_filename << endl;
            string expected_content("This is a test. If this was not a test you would have known the answer.\n");
            if(debug) cerr << prolog << "expected_content string: " << expected_content << endl;
            string content = get_file_as_string(cache_filename);
            if(debug) cerr << prolog << "retrieved content: " << content << endl;
            CPPUNIT_ASSERT( content == expected_content );
        }
        catch (BESError &besE){
            cerr << "Caught BESError! message: " << besE.get_verbose_message() << " type: " << besE.get_bes_error_type() << endl;
            CPPUNIT_ASSERT(false);
        }
    }

    void get_ngap_ghrc_tea_url_test() {
        string url = "https://d1jecqxxv88lkr.cloudfront.net/ghrcwuat-protected/rss_demo/rssmif16d__7/f16_ssmis_20031026v7.nc.dmrpp";
        if(debug) cerr << prolog << "url: " << url << endl;
        http::RemoteResource rhr(url);
        try {
            rhr.retrieveResource();
            vector<string> *hdrs = rhr.getResponseHeaders();
            for(size_t i=0; i<hdrs->size() && debug ; i++){
                cerr << prolog << "hdr["<< i << "]: " << (*hdrs)[i] << endl;
            }
            string cache_filename = rhr.getCacheFileName();
            if(debug) cerr << prolog << "cache_filename: " << cache_filename << endl;
            //string expected_content("This is a test. If this was not a test you would have known the answer.\n");
            //if(debug) cerr << prolog << "expected_content string: " << expected_content << endl;
            string content = get_file_as_string(cache_filename);
            if(debug) cerr << prolog << "retrieved content: " << content << endl;
            // CPPUNIT_ASSERT( content == expected_content );
        }
        catch (BESError &besE){
            stringstream msg;
            msg << "Caught BESError! message: " << besE.get_verbose_message() << " type: " << besE.get_bes_error_type() << endl;
            cerr << msg.str();
            CPPUNIT_FAIL(msg.str());
        }

    }



    void get_ngap_harmony_url_test() {

        string url = "https://harmony.uat.earthdata.nasa.gov/service-results/harmony-uat-staging/public/"
                        "sds/staged/ATL03_20200714235814_03000802_003_01.h5.dmrpp";

        if(debug) cerr << prolog << "url: " << url << endl;
        http::RemoteResource rhr(url);
        try {
            rhr.retrieveResource();
            vector<string> *hdrs = rhr.getResponseHeaders();
            for(size_t i=0; i<hdrs->size() && debug ; i++){
                cerr << prolog << "hdr["<< i << "]: " << (*hdrs)[i] << endl;
            }
            string cache_filename = rhr.getCacheFileName();
            if(debug) cerr << prolog << "cache_filename: " << cache_filename << endl;
            string content = get_file_as_string(cache_filename);
            if(debug) cerr << prolog << "retrieved content: " << content << endl;
            // CPPUNIT_ASSERT( content == expected_content );
        }
        catch (BESError &besE){
            stringstream msg;
            msg << "Caught BESError! message: " << besE.get_verbose_message() << " type: " << besE.get_bes_error_type() << endl;
            cerr << msg.str();
            CPPUNIT_FAIL(msg.str());
        }
    }

        /**
     *
     */
    void get_file_url_test() {
        bool debug_state = debug;
        debug = true;
        if(debug) cerr << endl;

        string data_file_url = get_data_file_url("test_file");
        http::RemoteResource rhr(data_file_url);
        try {
            rhr.retrieveResource();
            vector<string> *hdrs = rhr.getResponseHeaders();

            for(size_t i=0; i<hdrs->size() && debug ; i++){
                cerr << prolog << "hdr["<< i << "]: " << (*hdrs)[i] << endl;
            }
            string cache_filename = rhr.getCacheFileName();
            if(debug) cerr << prolog << "cache_filename: " << cache_filename << endl;

            string expected("This a TEST. Move Along...\n");
            string retrieved = get_file_as_string(cache_filename);
            if(debug) cerr << prolog << "expected_content: '" << expected << "'" << endl;
            if(debug) cerr << prolog << "retrieved_content: '" << retrieved << "'" << endl;
            CPPUNIT_ASSERT( retrieved == expected );
        }
        catch (BESError &besE){
            stringstream msg;
            msg << prolog << "Caught BESError! message: '" << besE.get_message() << "' bes_error_type: " << besE.get_bes_error_type() << endl;
            CPPUNIT_FAIL(msg.str());
        }
#if 0
        catch (libdap::Error &le){
            cerr << "Caught libdap::Error! message: " << le.get_error_message() << " code: "<< le.get_error_code() << endl;
            CPPUNIT_ASSERT(false);
        }
#endif
        debug = debug_state;
    }

/* TESTS END */
/*##################################################################################################*/


    CPPUNIT_TEST_SUITE( RemoteResourceTest );

    CPPUNIT_TEST(get_ngap_ghrc_tea_url_test);
    CPPUNIT_TEST(get_ngap_harmony_url_test);
    CPPUNIT_TEST(get_http_url_test);
    CPPUNIT_TEST(get_file_url_test);

    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(RemoteResourceTest);

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
        case 'P':
            purge_cache = true;  // debug is a static global
            cerr << "purge_cache enabled" << endl;
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
            test = http::RemoteResourceTest::suite()->getName().append("::").append(argv[i]);
            wasSuccessful = wasSuccessful && runner.run(test);
            ++i;
        }
    }

    return wasSuccessful ? 0 : 1;
}
