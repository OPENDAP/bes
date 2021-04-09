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

#include "config.h"

#include <memory>
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/filereadstream.h"
#include <cstdio>
#include <cstring>
#include <iostream>

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
#include "HttpUtils.h"
#include "HttpNames.h"
#include "url_impl.h"
#include "RemoteResource.h"
#include "BESContainer.h"

#include "NgapApi.h"
#include "NgapContainer.h"
#include "NgapNames.h"
// #include "NgapError.h"
// #include "rjson_utils.h"

#include "test_config.h"

using namespace std;
using namespace rapidjson;

static bool debug = false;
static bool Debug = false;
static bool bes_debug = false;

#undef DBG
#define DBG(x) do { if (debug) x; } while(false)

#define prolog std::string("NgapContainerTest::").append(__func__).append("() - ")


class MockContainer : public ngap::NgapContainer {
private:
    void initialize() override {  }

public:

    MockContainer(const string &sym_name,
                  const string &real_name,
                  const string &type) {
        set_real_name(real_name);
    }
};

class NgapContainerTest: public CppUnit::TestFixture {
private:
    string d_data_dir;
    string d_temp_file;

    // char curl_error_buf[CURL_ERROR_SIZE];

    void show_file(string filename)
    {
        ifstream t(filename.c_str());

        if (t.is_open()) {
            string file_content((istreambuf_iterator<char>(t)), istreambuf_iterator<char>());
            t.close();
            cout << endl << "##################################################################" << endl;
            cout << "file: " << filename << endl;
            cout << ". . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . " << endl;
            cout << file_content << endl;
            cout << ". . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . " << endl;
        }
    }
    void purge_http_cache(){
        if(Debug) cerr << prolog << "Purging cache!" << endl;
        string cache_dir;
        bool found_dir;
        TheBESKeys::TheKeys()->get_value(HTTP_CACHE_DIR_KEY,cache_dir,found_dir);
        bool found_prefix;
        string cache_prefix;
        TheBESKeys::TheKeys()->get_value(HTTP_CACHE_PREFIX_KEY,cache_prefix,found_prefix);

        if(found_dir && found_prefix){
            if(Debug) cerr << prolog << HTTP_CACHE_DIR_KEY << ": " <<  cache_dir << endl;
            if(Debug) cerr << prolog << "Purging " << cache_dir << " of files with prefix: " << cache_prefix << endl;
            string sys_cmd = "mkdir -p "+ cache_dir;
            if(Debug) cerr << "Running system command: " << sys_cmd << endl;
            system(sys_cmd.c_str());

            sys_cmd = "exec rm -rf "+ BESUtil::assemblePath(cache_dir,cache_prefix);
            sys_cmd =  sys_cmd.append("*");
            if(Debug) cerr << "Running system command: " << sys_cmd << endl;
            system(sys_cmd.c_str());
            if(Debug) cerr << prolog << "The HTTP cache has been purged." << endl;
        }
    }

public:
    // Called once before everything gets tested
    NgapContainerTest()
    {
        d_data_dir = TEST_DATA_DIR;;
    }

    // Called at the end of the test
    ~NgapContainerTest()
    {
    }

    // Called before each test
    void setUp()
    {
        if(debug) cerr << endl;
        if(Debug) cerr << prolog << "BEGIN" << endl;
        string bes_conf = BESUtil::assemblePath(TEST_BUILD_DIR,"bes.conf");
        if(Debug) cerr << prolog << "Using BES configuration: " << bes_conf << endl;

        TheBESKeys::ConfigFile = bes_conf;

        if (bes_debug) BESDebug::SetUp("cerr,ngap,http");

        if (Debug) show_file(bes_conf);

        purge_http_cache();

        if(Debug) cerr << prolog << "END" << endl;
    }

    // Called after each test
    void tearDown()
    {
    }

    void show_vector(vector<string> v){
        cerr << "show_vector(): Found " << v.size() << " elements." << endl;
        // vector<string>::iterator it = v.begin();
        for(size_t i=0;  i < v.size(); i++){
            cerr << "show_vector:    v["<< i << "]: " << v[i] << endl;
        }
    }


    void compare_results(const string &granule_name, const string &data_access_url, const string &expected_data_access_url){
        if (debug) cerr << prolog << "TEST: Is the URL longer than the granule name? " << endl;
        CPPUNIT_ASSERT (data_access_url.length() > granule_name.length() );

        if (debug) cerr << prolog << "TEST: Does the URL end with the granule name? " << endl;
        bool endsWithGranuleName = data_access_url.substr(data_access_url.length()-granule_name.length(), granule_name.length()) == granule_name;
        CPPUNIT_ASSERT( endsWithGranuleName == true );

        if (debug) cerr << prolog << "TEST: Does the returned URL match the expected URL? " << endl;
        if (debug) cerr << prolog << "CMR returned DataAccessURL: " << data_access_url << endl;
        if (debug) cerr << prolog << "The expected DataAccessURL: " << expected_data_access_url << endl;
        CPPUNIT_ASSERT (expected_data_access_url == data_access_url);

    }


    /**
     * This test exercises the legacy 3 component restified path model
     * /providers/<provider_id>/collections/<entry_title>/granules/<granule_ur>
     */
    void access_test() {
        if(debug) cerr << prolog << "BEGIN" << endl;

        try {
            string sym_name = "";
            string restified_path = "http://";
            string type = "";

            MockContainer mc(sym_name,restified_path,type);

        }
        catch(BESError &be){
            stringstream msg;
            msg << prolog << "Caught BESError! Message: " << be.get_verbose_message();
            cerr << msg.str() << endl;
            CPPUNIT_FAIL(msg.str());
        }
        catch(std::exception &se){
            stringstream msg;
            msg << prolog << "Caught std::excpetion! Message: " << se.what();
            cerr << msg.str() << endl;
            CPPUNIT_FAIL(msg.str());
        }
        catch(...){
            CPPUNIT_FAIL(prolog + "Caught unknown exception.");
        }







        if(debug) cerr << prolog << "END" << endl;
    }



    CPPUNIT_TEST_SUITE( NgapContainerTest );

        CPPUNIT_TEST(access_test);

    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(NgapContainerTest);


int main(int argc, char*argv[])
{
    CppUnit::TextTestRunner runner;
    runner.addTest(CppUnit::TestFactoryRegistry::getRegistry().makeTest());

    GetOpt getopt(argc, argv, "dbD");
    int option_char;
    while ((option_char = getopt()) != -1)
        switch (option_char) {
        case 'd':
            debug = true;  // debug is a static global
            break;
        case 'D':
            Debug = true;  // Debug is a static global
            break;
        case 'b':
            bes_debug = true;  // debug is a static global
            break;
        default:
            break;
        }

    /*cerr << "    debug: " << (debug?"enabled":"disabled") << endl;
    cerr << "    Debug: " << (Debug?"enabled":"disabled") << endl;
    cerr << "bes_debug: " << (bes_debug?"enabled":"disabled") << endl;*/

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
            test = NgapContainerTest::suite()->getName().append("::").append(argv[i]);
            wasSuccessful = wasSuccessful && runner.run(test);
            ++i;
        }
    }

    return wasSuccessful ? 0 : 1;
}
