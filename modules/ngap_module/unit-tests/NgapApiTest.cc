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

#include <BESError.h>
#include <BESDebug.h>
#include <BESUtil.h>
#include <BESCatalogList.h>
#include <TheBESKeys.h>
#include "test_config.h"

#include "RemoteHttpResource.h"
#include "NgapApi.h"
// #include "NgapNames.h"
// #include "NgapError.h"
// #include "rjson_utils.h"

using namespace std;
using namespace rapidjson;

static bool debug = false;
static bool Debug = false;
static bool bes_debug = false;

#undef DBG
#define DBG(x) do { if (debug) x; } while(false)

namespace ngap {

class NgapApiTest: public CppUnit::TestFixture {
private:

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

public:
    // Called once before everything gets tested
    NgapApiTest()
    {
    }

    // Called at the end of the test
    ~NgapApiTest()
    {
    }

    // Called before each test
    void setUp()
    {
        if(Debug) cerr << "setUp() - BEGIN" << endl;
        string bes_conf = BESUtil::assemblePath(TEST_BUILD_DIR,"bes.conf");
        if(Debug) cerr << "setUp() - Using BES configuration: " << bes_conf << endl;

        TheBESKeys::ConfigFile = bes_conf;

        if (bes_debug) BESDebug::SetUp("cerr,ngap");

        if (bes_debug) show_file(bes_conf);
        if(Debug) cerr << "setUp() - END" << endl;
    }

    // Called after each test
    void tearDown()
    {
    }

    void cmr_access_test() {
        string prolog = string(__func__) + "() - ";
        NgapApi ngapi;
        string provider_name;
        string collection_name;
        string granule_name;
        string data_access_url;

        if ( debug  ) {
            cout << endl;
        }
        provider_name = "GHRC_CLOUD";
        collection_name ="ADVANCED MICROWAVE SOUNDING UNIT-A (AMSU-A) SWATH FROM NOAA-15 V1";
        granule_name = "amsua15_2020.028_12915_1139_1324_WI.nc";

        string resty_path = "providers/"+provider_name+"/collections/"+collection_name+"/granules/"+granule_name;
        if (debug) cerr << prolog << "RestifiedPath: " << resty_path << endl;

        try {
            data_access_url = ngapi.convert_ngap_resty_path_to_data_access_url(resty_path);
        }
        catch(BESError e){
            cerr << "Caught BESError: " << e.get_message() << endl;
            CPPUNIT_ASSERT(false);
        }
        stringstream msg;

        string expected;
        // OLD value.
        // expected = "https://d1lpqa6z94hycl.cloudfront.net/ghrc-app-protected/amsua15sp__1/2020-01-28/amsua15_2020.028_12915_1139_1324_WI.nc";
        // New value as of 4/24/2020
        expected = "https://d1sd4up8kynpk2.cloudfront.net/ghrcw-protected/amsua15sp/amsu-a/noaa-15/data/nc/2020/0128/amsua15_2020.028_12915_1139_1324_WI.nc";

        if (debug) cerr << prolog << "TEST: Is the URL longer than the granule name? " << endl;
        CPPUNIT_ASSERT (data_access_url.length() > granule_name.length() );

        if (debug) cerr << prolog << "TEST: Does the URL end with the granule name? " << endl;
        bool endsWithGranuleName = data_access_url.substr(data_access_url.length()-granule_name.length(), granule_name.length()).compare(granule_name) == 0;
        CPPUNIT_ASSERT( endsWithGranuleName == true );

        if (debug) cerr << prolog << "TEST: Does the returned URL match the expected URL? " << endl;
        if (debug) cerr << prolog << "CMR returned DataAccessURL: " << data_access_url << endl;
        if (debug) cerr << prolog << "The expected DataAccessURL: " << expected << endl;
        CPPUNIT_ASSERT (expected == data_access_url);
    }

    CPPUNIT_TEST_SUITE( NgapApiTest );

    CPPUNIT_TEST(cmr_access_test);

    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(NgapApiTest);

} // namespace dmrpp

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
            test = ngap::NgapApiTest::suite()->getName().append("::").append(argv[i]);
            wasSuccessful = wasSuccessful && runner.run(test);
            ++i;
        }
    }

    return wasSuccessful ? 0 : 1;
}
