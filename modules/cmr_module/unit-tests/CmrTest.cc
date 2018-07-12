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
#include <iostream>

#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>
#include <curl/curl.h>

#include <XMLWriter.h>
#include <GetOpt.h>
#include <util.h>
#include <debug.h>

#include <BESError.h>
#include <BESDebug.h>
#include <BESUtil.h>
#include <TheBESKeys.h>
#include "test_config.h"

#include "../RemoteHttpResource.h"


using namespace libdap;
using namespace rapidjson;

static bool debug = false;
static bool bes_debug = false;

#undef DBG
#define DBG(x) do { if (debug) x; } while(false)

namespace cmr {

class CmrTest: public CppUnit::TestFixture {
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
    CmrTest()
    {
    }

    // Called at the end of the test
    ~CmrTest()
    {
    }

    // Called before each test
    void setUp()
    {
        string bes_conf = BESUtil::assemblePath(TEST_SRC_DIR,"CmrTest.ini");
        TheBESKeys::ConfigFile = bes_conf;

        if (bes_debug) BESDebug::SetUp("cerr,cmr");

        if (bes_debug) show_file(bes_conf);
    }

    // Called after each test
    void tearDown()
    {
    }

    void getJsonDoc(const string &url, rapidjson::Document &d){
        if(debug) cerr << endl << "Trying url: " << url << endl;

        try {
            cmr::RemoteHttpResource rhr(url);


            rhr.retrieveResource();
            FILE* fp = fopen(rhr.getCacheFileName().c_str(), "r"); // non-Windows use "r"
            char readBuffer[65536];
            rapidjson::FileReadStream frs(fp, readBuffer, sizeof(readBuffer));

            d.ParseStream(frs);


            CPPUNIT_ASSERT(true);
        }
        catch (BESError &be){
            cerr << be.get_message() << endl;
            CPPUNIT_ASSERT(false);
        }


    }
    void printJsonDoc(rapidjson::Document &d){
        StringBuffer buffer;
        rapidjson::PrettyWriter<StringBuffer> writer(buffer);
        d.Accept(writer);
        std::cout << buffer.GetString() << std::endl;
    }

    void get_collection_years(string collection_name){
        string url = "https://cmr.earthdata.nasa.gov/search/granules.json?concept_id="+collection_name +"&include_facets=v2";
        rapidjson::Document doc;
        getJsonDoc(url,doc);
        if(debug) printJsonDoc(doc);

        bool result = doc.IsObject();
        if(debug) cerr << "Json document is" << (result?"":" NOT") << " an object." << endl;
        CPPUNIT_ASSERT(result);

        rapidjson::Value::ConstMemberIterator itr = doc.FindMember("feed");
        result  = itr != doc.MemberEnd();
        if(debug) cerr << "" << (result?"Located":"FAILED to locate") << " the value 'feed'." << endl;
        CPPUNIT_ASSERT(result);

        const Value& feed = itr->value;
        result  = feed.IsObject();
        if(debug) cerr << "The value 'feed' is" << (result?"":" NOT") << " an object." << endl;
        CPPUNIT_ASSERT(result);

        itr = feed.FindMember("facets");
        result  = itr != feed.MemberEnd();
        if(debug) cerr << "" << (result?"Located":"FAILED to locate") << " the value 'facets'." << endl;
        CPPUNIT_ASSERT(result);

        const Value& facets = itr->value;
        result  = facets.IsObject();
        if(debug) cerr << "The value 'facets' is" << (result?"":" NOT") << " an object." << endl;
        CPPUNIT_ASSERT(result);

    }

    void test_one()
    {
        CPPUNIT_ASSERT(true);
        string collection_name = "C179003030-ORNL_DAAC";

        get_collection_years(collection_name);

    }

    CPPUNIT_TEST_SUITE( CmrTest );

    CPPUNIT_TEST(test_one);

    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(CmrTest);

} // namespace dmrpp

int main(int argc, char*argv[])
{
    CppUnit::TextTestRunner runner;
    runner.addTest(CppUnit::TestFactoryRegistry::getRegistry().makeTest());

    GetOpt getopt(argc, argv, "db");
    int option_char;
    while ((option_char = getopt()) != -1)
        switch (option_char) {
        case 'd':
            debug = true;  // debug is a static global
            break;
        case 'b':
            bes_debug = true;  // debug is a static global
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
            test = cmr::CmrTest::suite()->getName().append("::").append(argv[i]);
            wasSuccessful = wasSuccessful && runner.run(test);
            ++i;
        }
    }

    return wasSuccessful ? 0 : 1;
}
