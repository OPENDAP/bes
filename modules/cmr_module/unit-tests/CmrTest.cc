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

#include "RemoteHttpResource.h"
#include "CmrApi.h"
#include "CmrError.h"

#define MODULE "cmr"

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


    void get_collection_years_test()
    {
        string prolog = string(__func__) + "() - ";

        string collection_name = "C179003030-ORNL_DAAC";
        string expected[] = {
                string("1984"),
                string("1985"),
                string("1986"),
                string("1987"),
                string("1988")
        };

        vector<string> years;
        try {
            CmrApi cmr;

            cmr.get_collection_years(collection_name,years);

            CPPUNIT_ASSERT(5 == years.size());

            stringstream msg;
            msg << prolog << "The collection '" << collection_name << "' spans " << years.size() << " years: ";
            for(size_t i=0; i<years.size(); i++){
                if(i>0) msg << ", ";
                msg << years[i];
            }
            BESDEBUG(MODULE,msg.str() << endl);

            for(size_t i=0; i<years.size(); i++){
                msg.str(std::string());
                msg << prolog << "Checking:  expected: "<< expected[i] << " received: " << years[i];
                BESDEBUG(MODULE,msg.str() << endl);
                CPPUNIT_ASSERT(expected[i] == years[i]);
            }

     }
        catch (BESError &be){
            string msg = "Caught BESError! Message: " + be.get_message() ;
            cerr << endl << msg << endl;
            CPPUNIT_ASSERT(!"Caught BESError");
        }


    }

    CPPUNIT_TEST_SUITE( CmrTest );

    CPPUNIT_TEST(get_collection_years_test);

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
