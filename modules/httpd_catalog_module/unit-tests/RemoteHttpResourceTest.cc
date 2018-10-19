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
#include "test_config.h"

#include "../RemoteHttpResource.h"
#include "../RemoteHttpResourceCache.h"
#include "../HttpdDirScraper.h"
#include "../HttpdCatalogNames.h"


using namespace std;

static bool debug = false;
static bool Debug = false;
static bool bes_debug = false;
static bool purge_cache = false;

#undef DBG
#define DBG(x) do { if (debug) x; } while(false)

namespace httpd_catalog {

class RemoteHttpResourceTest: public CppUnit::TestFixture {
private:

    // char curl_error_buf[CURL_ERROR_SIZE];

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
        if(debug) cerr << "data_file: " << data_file << endl;
        if(Debug) show_file(data_file);

        string data_file_url = "file://" + data_file;
        if(debug) cerr << "data_file_url: " << data_file_url << endl;
        return data_file_url;
    }



public:
    string d_data_dir;
    // Called once before everything gets tested
    RemoteHttpResourceTest()
    {
        d_data_dir = TEST_DATA_DIR;;
        cerr << "data_dir: " << d_data_dir << endl;
    }

    // Called at the end of the test
    ~RemoteHttpResourceTest()
    {
    }

    // Called before each test
    void setUp()
    {
        if(Debug) cerr << endl << "setUp() - BEGIN" << endl;
        string bes_conf = BESUtil::assemblePath(TEST_BUILD_DIR,"bes.conf");
        if(Debug) cerr << "setUp() - Using BES configuration: " << bes_conf << endl;

        TheBESKeys::ConfigFile = bes_conf;

        if (bes_debug) BESDebug::SetUp("cerr,cmr");

        if (bes_debug) show_file(bes_conf);

        if(purge_cache){
            if(Debug) cerr << "Purging cache!" << endl;
            string cache_dir;
            bool found;
            TheBESKeys::TheKeys()->get_value(RemoteHttpResourceCache::DIR_KEY,cache_dir,found);
            if(found){
                if(Debug) cerr << RemoteHttpResourceCache::DIR_KEY << ": " <<  cache_dir << endl;
                if(Debug) cerr << "Purging " << cache_dir << endl;
                string cmd = "exec rm -r "+ BESUtil::assemblePath(cache_dir,"/*");
                system(cmd.c_str());
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

        string url = "https://www.opendap.org/pub/";
        if(debug) cerr << __func__ << "() - url: " << url << endl;
        RemoteHttpResource rhr(url);
        try {
            rhr.retrieveResource();
            vector<string> hdrs;
            rhr.getResponseHeaders(hdrs);

            for(size_t i=0; i<hdrs.size() && debug ; i++){
                cerr << __func__ << "() - hdr["<< i << "]: " << hdrs[i] << endl;
            }
            string cache_filename = rhr.getCacheFileName();
            if(debug) cerr <<  __func__ << "() - cache_filename: " << cache_filename << endl;
            show_file(cache_filename);
        }
        catch (BESError &besE){
            cerr << "Caught BESError! message: " << besE.get_verbose_message() << " type: " << besE.get_bes_error_type() << endl;
            CPPUNIT_ASSERT(false);
        }
        catch (libdap::Error &le){
            cerr << "Caught libdap::Error! message: " << le.get_error_message() << " code: "<< le.get_error_code() << endl;
            CPPUNIT_ASSERT(false);
        }
    }

    /**
     *
     */
    void get_file_url_test() {
        if(debug) cerr << endl;

        string data_file_url = get_data_file_url("test_file");
        RemoteHttpResource rhr(data_file_url);
        try {
            rhr.retrieveResource();
            vector<string> hdrs;
            rhr.getResponseHeaders(hdrs);

            for(size_t i=0; i<hdrs.size() && debug ; i++){
                cerr <<  __func__ << "() - hdr["<< i << "]: " << hdrs[i] << endl;
            }
            string cache_filename = rhr.getCacheFileName();
            if(debug) cerr <<  __func__ << "() - cache_filename: " << cache_filename << endl;

            string target("This a TEST. Move Along...\n");

            string content = get_file_as_string(cache_filename);
            CPPUNIT_ASSERT( !content.compare(target) );
        }
        catch (BESError &besE){
            cerr << "Caught BESError! message: " << besE.get_verbose_message() << " type: " << besE.get_bes_error_type() << endl;
            CPPUNIT_ASSERT(false);
        }
        catch (libdap::Error &le){
            cerr << "Caught libdap::Error! message: " << le.get_error_message() << " code: "<< le.get_error_code() << endl;
            CPPUNIT_ASSERT(false);
        }
    }

/* TESTS END */
/*##################################################################################################*/


    CPPUNIT_TEST_SUITE( RemoteHttpResourceTest );

    // CPPUNIT_TEST(get_http_url_test);
    CPPUNIT_TEST(get_file_url_test);

    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(RemoteHttpResourceTest);

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
            test = httpd_catalog::RemoteHttpResourceTest::suite()->getName().append("::").append(argv[i]);
            wasSuccessful = wasSuccessful && runner.run(test);
            ++i;
        }
    }

    return wasSuccessful ? 0 : 1;
}
