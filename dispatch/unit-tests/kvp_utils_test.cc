// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of the OPeNDAP Back-End Server (BES)
// and embodies a whitelist of remote system that may be
// accessed by the server as part of it's routine operation.

// Copyright (c) 2020 OPeNDAP, Inc.
// Author: Nathan D. Potter <ndp@opendap.org>
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

#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>

#include <string>
#include <iostream>
#include <cstdlib>
#include <fstream>
#include <streambuf>

#include <GetOpt.h>
#include <BESCatalog.h>
#include <BESCatalogDirectory.h>
#include <BESCatalogList.h>
#include <BESCatalogUtils.h>

#include <BESError.h>
#include <BESInternalFatalError.h>
#include <BESInternalError.h>
#include <TheBESKeys.h>
#include <BESDebug.h>

#include "test_config.h"
#include "kvp_utils.h"

using namespace std;
using namespace CppUnit;

static bool debug = false;
static bool bes_debug = false;

const string catalog_root_dir = BESUtil::assemblePath(TEST_SRC_DIR,"catalog_test");

#undef DBG
#define DBG(x) do { if (debug) (x); } while(false);

class KeyValuePairTest: public TestFixture {
private:
    string kvp_file;
    map<string, vector<string> > keystore;

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
    KeyValuePairTest()
    {
    }

    ~KeyValuePairTest()
    {
    }

    void setUp()
    {
        string bes_dir = string(TEST_SRC_DIR).append("/bes/kvp_test.ini");
    }

    void tearDown()
    {
        // BESCatalogList::TheCatalogList()->deref_catalog(BES_DEFAULT_CATALOG);
    }

    CPPUNIT_TEST_SUITE( KeyValuePairTest );

    CPPUNIT_TEST(load_bad_keys);
    //CPPUNIT_TEST(load_good_keys);
    CPPUNIT_TEST(get_key_value);
    CPPUNIT_TEST(get_multi_valued_key);

    CPPUNIT_TEST_SUITE_END();

    void load_bad_keys()
    {
        keystore.clear();
        string keys_file = string(TEST_SRC_DIR).append("/bad_keys1.ini");

        if(debug) cout << "*****************************************" << endl;
        if(debug) cout << "load_bad_keys() - Bad file, not enough equal signs" << endl;
        if(debug) cout << "load_bad_keys() - Loading Keys File: " << keys_file << endl;

        try {
            load_keys(keys_file,keystore);
            CPPUNIT_FAIL("load_bad_keys() The load_keys() call should have failed but it did not.");
        }
        catch (BESError &e) {
            if(debug) cout << "load_bad_keys() - Unable to create BESKeys, expected. Message: ";
            if(debug) cout << e.get_message() << endl;
        }

    }
    void load_good_keys() {
        keystore.clear();
        string keys_file = string(TEST_SRC_DIR).append("/keys_test.ini");

        if(debug) cout << "*****************************************" << endl;
        if(debug) cout << "load_good_keys() - Loading Keys File: " << keys_file << endl;

        try {
            load_keys(keys_file, keystore);
            if(debug) cout << "load_good_keys() - Read " << keystore.size() << " keys from " << keys_file << endl ;
        }
        catch (BESError &e) {
            cerr << "load_good_keys() - ERROR: " << e.get_message() << endl;
            CPPUNIT_FAIL("load_good_keys() - The call to load_keys() was unable to create the keys. message: " + e.get_message());
        }

    }

    void get_key_value() {
        load_good_keys();
        map<string, vector<string>>::iterator it;
        if(debug) cout << "*****************************************" << endl;
        if(debug) cout << "get_key_value() - Keystore has "<< keystore.size() <<  " keys" << endl ;
        string ret = "";
        for (int i = 1; i < 5; i++) {
            char key[32];
            sprintf(key, "BES.KEY%d", i);
            char val[32];
            sprintf(val, "val%d", i);
            if(debug) cout << "get_key_value() - Looking for " << key << " with value " << val << endl;
            ret = "";
            it=keystore.find(key);
            if(it!=keystore.end()){
                vector<string> &ret_vals = it->second;
                CPPUNIT_ASSERT(!ret_vals.empty());
                CPPUNIT_ASSERT(!ret_vals[0].empty());
                CPPUNIT_ASSERT(ret_vals[0] == val);
            }
            else {
                CPPUNIT_FAIL(string("get_key_value() - Key '")+key+"' not found");
            }
        }
    }

    void get_multi_valued_key() {
        load_good_keys();
        map<string, vector<string>>::iterator it;
        if(debug) cout << "*****************************************" << endl;
        if(debug) cout << "get_multi_valued_key() - Check for a multi value key" << endl;

        string key = "BES.KEY.MULTI";
        it=keystore.find(key);
        if(it!=keystore.end()){
            vector<string> &ret_vals = it->second;
            if(debug) cout << "get_multi_valued_key() - Found " << ret_vals.size() << " values for key: '" << key << "'" << endl;
            CPPUNIT_ASSERT(ret_vals.size() == 5);

            for (int i = 0; i < 5; i++) {
                char val[32];
                sprintf(val, "val_multi_%d", i);
                if(debug) cout << "get_multi_valued_key() - expected value '" << val << "' read value: '" << ret_vals[i] << "'" << endl;
                CPPUNIT_ASSERT(ret_vals[i] == val);
            }
        }
        else {
            CPPUNIT_FAIL(string("get_multi_valued_key() - Key '")+key+"' not found");
        }
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION(KeyValuePairTest);

int main(int argc, char*argv[])
{
    GetOpt getopt(argc, argv, "dhb");
    int option_char;
    while ((option_char = getopt()) != EOF)
        switch (option_char) {
            case 'd':
                debug = 1;  // debug is a static global
                break;
            case 'b':
                bes_debug = 1;  // debug is a static global
                break;
            case 'h': {     // help - show test names
                cerr << "Usage: plistT has the following tests:" << endl;
                const vector<Test*> &tests = KeyValuePairTest::suite()->getTests();
                unsigned int prefix_len = KeyValuePairTest::suite()->getName().append("::").length();
                for (vector<Test*>::const_iterator i = tests.begin(), e = tests.end(); i != e; ++i) {
                    cerr << (*i)->getName().replace(0, prefix_len, "") << endl;
                }
                break;
            }
            default:
                break;
        }

    CppUnit::TextTestRunner runner;
    runner.addTest(CppUnit::TestFactoryRegistry::getRegistry().makeTest());

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
            test = KeyValuePairTest::suite()->getName().append("::").append(argv[i]);
            wasSuccessful = wasSuccessful && runner.run(test);
        }
    }

    return wasSuccessful ? 0 : 1;
}

