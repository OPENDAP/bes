// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of the OPeNDAP Back-End Server (BES)
// and provides unit-test capabilities for the kvp_utils
//
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
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/extensions/TestFactoryRegistry.h>

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <streambuf>
#include <string>

#include <BESCatalog.h>
#include <BESCatalogDirectory.h>
#include <BESCatalogList.h>
#include <BESCatalogUtils.h>
#include <unistd.h>

#include <BESDebug.h>
#include <BESError.h>
#include <BESInternalError.h>
#include <BESInternalFatalError.h>
#include <TheBESKeys.h>

#include "kvp_utils.h"
#include "test_config.h"

using namespace std;
using namespace CppUnit;

static bool debug = false;
static bool bes_debug = false;

const string catalog_root_dir = BESUtil::assemblePath(TEST_SRC_DIR, "catalog_test");

#undef DBG
#define DBG(x)                                                                                                         \
    do {                                                                                                               \
        if (debug)                                                                                                     \
            (x);                                                                                                       \
    } while (false);

class KeyValuePairTest : public TestFixture {
private:
    unordered_map<string, vector<string>> keystore;

    void show_file(string filename) {
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
    KeyValuePairTest() = default;

    ~KeyValuePairTest() = default;

    void setUp() { string bes_dir = string(TEST_SRC_DIR).append("/bes/kvp_test.ini"); }

    void tearDown() {
        // BESCatalogList::TheCatalogList()->deref_catalog(BES_DEFAULT_CATALOG);
    }

    CPPUNIT_TEST_SUITE(KeyValuePairTest);

    CPPUNIT_TEST(load_bad_keys);
    CPPUNIT_TEST(get_key_value);
    CPPUNIT_TEST(get_multi_valued_key);
    CPPUNIT_TEST(get_single_val_from_multi_val_key);
    CPPUNIT_TEST(check_for_missing_key);
    CPPUNIT_TEST(get_key_with_empty_value);
    CPPUNIT_TEST(check_empty_value_from_include);
    CPPUNIT_TEST(get_keys_from_regexp_include);
    CPPUNIT_TEST(get_those_keys);

    CPPUNIT_TEST_SUITE_END();

    void load_bad_keys() {
        if (debug)
            cout << endl << "******  load_bad_keys() ******" << endl;
        keystore.clear();
        string keys_file = string(TEST_SRC_DIR).append("/bad_keys1.ini");
        if (debug)
            cout << "load_bad_keys() - Bad file, not enough equal signs" << endl;
        if (debug)
            cout << "load_bad_keys() - Loading Keys File: " << keys_file << endl;

        try {
            kvp::load_keys(keys_file, keystore);
            CPPUNIT_FAIL("load_bad_keys() The load_keys() call should have failed but it did not.");
        } catch (BESError &e) {
            if (debug)
                cout << "load_bad_keys() - Unable to load keys, this is expected. Message: ";
            if (debug)
                cout << e.get_message() << endl;
        }
    }
    void load_good_keys(const string &filename) {
        keystore.clear();
        string keys_file = string(TEST_SRC_DIR).append(filename);
        if (debug)
            cout << "load_good_keys() - Loading Keys File: " << keys_file << endl;

        try {
            kvp::load_keys(keys_file, keystore);
            if (debug)
                cout << "load_good_keys() - Read " << keystore.size() << " keys from " << keys_file << endl;
        } catch (BESError &e) {
            cerr << "load_good_keys() - ERROR: " << e.get_message() << endl;
            CPPUNIT_FAIL("load_good_keys() - The call to load_keys() was unable to create the keys. message: " +
                         e.get_message());
        }
    }

    void get_key_value() {
        if (debug)
            cout << endl << "******  get_key_value() ******" << endl;
        load_good_keys("/keys_test.ini");
        if (debug)
            cout << "get_key_value() - Keystore has " << keystore.size() << " keys" << endl;
        string ret = "";
        for (int i = 1; i < 5; i++) {
            char key[32];
            sprintf(key, "BES.KEY%d", i);
            char val[32];
            sprintf(val, "val%d", i);
            if (debug)
                cout << "get_key_value() - Looking for " << key << " with value " << val << endl;
            ret = "";
            auto it = keystore.find(key);
            if (it != keystore.end()) {
                vector<string> &ret_vals = it->second;
                CPPUNIT_ASSERT(!ret_vals.empty());
                CPPUNIT_ASSERT(!ret_vals[0].empty());
                CPPUNIT_ASSERT(ret_vals[0] == val);
            } else {
                CPPUNIT_FAIL(string("get_key_value() - Key '") + key + "' not found");
            }
        }
        if (debug)
            cout << "get_key_value() - Alright. Alright. Aright..." << endl;
    }

    void get_multi_valued_key() {
        if (debug)
            cout << endl << "******  get_multi_valued_key() ******" << endl;
        load_good_keys("/keys_test.ini");
        if (debug)
            cout << "get_multi_valued_key() - Check for a multi value key" << endl;

        string key = "BES.KEY.MULTI";
        auto it = keystore.find(key);
        if (it != keystore.end()) {
            vector<string> &ret_vals = it->second;
            if (debug)
                cout << "get_multi_valued_key() - Found " << ret_vals.size() << " values for key: '" << key << "'"
                     << endl;
            CPPUNIT_ASSERT(ret_vals.size() == 5);

            for (int i = 0; i < 5; i++) {
                char val[32];
                sprintf(val, "val_multi_%d", i);
                if (debug)
                    cout << "get_multi_valued_key() - expected value '" << val << "' read value: '" << ret_vals[i]
                         << "'" << endl;
                CPPUNIT_ASSERT(ret_vals[i] == val);
            }
        } else {
            CPPUNIT_FAIL(string("get_multi_valued_key() - Key '") + key + "' not found");
        }
        if (debug)
            cout << "get_multi_valued_key() - Alright. Alright. Aright..." << endl;
    }

    void get_single_val_from_multi_val_key() {
        if (debug)
            cout << endl << "******  get_single_val_from_multi_val_key() ******" << endl;
        load_good_keys("/keys_test.ini");
        if (debug)
            cout << "get_single_val_from_multi_val_key() - Get single value from a multi-value key" << endl;

        string key = "BES.KEY.MULTI";
        auto it = keystore.find(key);
        if (it != keystore.end()) {
            vector<string> &ret_vals = it->second;
            if (debug)
                cout << "get_multi_valued_key() - Found " << ret_vals.size() << " values for key: '" << key << "'"
                     << endl;
            CPPUNIT_ASSERT(ret_vals.size() == 5);

            string expected_value = "val_multi_0";
            CPPUNIT_ASSERT(ret_vals[0] == expected_value);
        } else {
            CPPUNIT_FAIL(string("get_single_val_from_multi_val_key() - Expected key '") + key + "' not found");
        }
        if (debug)
            cout << "get_single_val_from_multi_val_key() - Alright. Alright. Aright..." << endl;
    }

    void check_for_missing_key() {
        if (debug)
            cout << endl << "******  check_for_missing_key() ******" << endl;
        load_good_keys("/keys_test.ini");
        if (debug)
            cout << "check_for_missing_key() - Looking for non existant key" << endl;

        string key = "BES.NOTFOUND";
        auto it = keystore.find(key);
        if (it != keystore.end()) {
            CPPUNIT_FAIL(string("check_for_missing_key() - OOPS! Key '") + key + "' was found! ");
        }
        if (debug)
            cout << "check_for_missing_key() - Alright. Alright. Aright..." << endl;
    }

    void get_key_with_empty_value() {
        if (debug)
            cout << endl << "******  get_key_with_empty_value() ******" << endl;
        load_good_keys("/keys_test.ini");
        if (debug)
            cout << "get_key_with_empty_value() - Looking for key with empty value" << endl;

        string key = "BES.KEY5";
        auto it = keystore.find(key);
        if (it != keystore.end()) {
            CPPUNIT_ASSERT(it->second.empty());
        } else {
            CPPUNIT_FAIL(string("get_key_with_empty_value() - Expected key '") + key + "' not found");
        }
        if (debug)
            cout << "get_key_with_empty_value() - Alright. Alright. Aright..." << endl;
    }

    void check_empty_value_from_include() {
        if (debug)
            cout << endl << "******  get_key_with_empty_value() ******" << endl;
        load_good_keys("/keys_test.ini");
        if (debug)
            cout << "check_empty_value_from_include() - Looking for key with empty value in included file" << endl;

        string key = "BES.KEY6";
        auto it = keystore.find(key);
        if (it != keystore.end()) {
            CPPUNIT_ASSERT(it->second.empty());
        } else {
            CPPUNIT_FAIL(string("check_empty_value_from_include() - Expected key '") + key + "' not found");
        }
        if (debug)
            cout << "check_empty_value_from_include() - Alright. Alright. Aright..." << endl;
    }

    void get_keys_from_regexp_include() {
        if (debug)
            cout << endl << "******  get_keys_from_regexp_include() ******" << endl;
        load_good_keys("/keys_test.ini");
        if (debug)
            cout << "get_keys_from_regexp_include() - Getting keys from reg exp included file" << endl;

        string key = "BES.KEY.MI";
        auto it = keystore.find(key);
        if (it != keystore.end()) {
            vector<string> values = it->second;

            CPPUNIT_ASSERT(values.size() == 3);
            CPPUNIT_ASSERT(values[0] == "val_multi_2");
            CPPUNIT_ASSERT(values[1] == "val_multi_1");
            CPPUNIT_ASSERT(values[2] == "val_multi_3");
        } else {
            CPPUNIT_FAIL(string("get_key_with_empty_value() - Expected key '") + key + "' not found");
        }

        if (debug)
            cout << "get_keys_from_regexp_include() - Alright. Alright. Aright..." << endl;
    }

    void get_those_keys() {
        if (debug)
            cout << endl << "******  get_those_keys() ******" << endl;
        load_good_keys("/keys_test.ini");
        if (debug)
            cout << "get_those_keys() - Looking many single valued keys" << endl;

        for (int i = 1; i < 7; i++) {
            stringstream ss;
            string key;
            ss << "BES.KEY" << i;
            key = ss.str();
            string val;
            if (i == 5 || i == 6) {
                val = "";
            } else {
                ss.str(std::string());
                ss << "val" << i;
                val = ss.str();
            }
            if (debug)
                cout << "get_those_keys() - Looking for " << key << " with value " << val << endl;
            auto it = keystore.find(key);
            if (it != keystore.end()) {
                vector<string> values = it->second;
                if (val.size() == 0) {
                    CPPUNIT_ASSERT(values.size() == 0);
                } else {
                    CPPUNIT_ASSERT(values[0] == val);
                }
            } else {
                CPPUNIT_FAIL(string("get_those_keys() - Expected key '") + key + "' not found");
            }
        }
        if (debug)
            cout << "get_those_keys() - Alright. Alright. Aright..." << endl;
    }

#if 0

   void template (){
        if(debug) cout << endl << "******  template() ******"  << endl;
        load_good_keys("/keys_test.ini");
        map<string, vector<string>>::iterator it;
        if(debug) cout << "template() - Looking for key with empty value" << endl;


        if(debug) cout << "template() - Alright. Alright. Aright..." << endl;

    }

#endif
};

CPPUNIT_TEST_SUITE_REGISTRATION(KeyValuePairTest);

int main(int argc, char *argv[]) {
    int option_char;
    while ((option_char = getopt(argc, argv, "dhb")) != EOF)
        switch (option_char) {
        case 'd':
            debug = true; // debug is a static global
            break;
        case 'b':
            bes_debug = true; // debug is a static global
            break;
        case 'h': { // help - show test names
            cerr << "Usage: plistT has the following tests:" << endl;
            const vector<Test *> &tests = KeyValuePairTest::suite()->getTests();
            unsigned int prefix_len = KeyValuePairTest::suite()->getName().append("::").size();
            for (auto test : tests) {
                cerr << test->getName().replace(0, prefix_len, "") << endl;
            }
            break;
        }
        default:
            break;
        }

    argc -= optind;
    argv += optind;

    CppUnit::TextTestRunner runner;
    runner.addTest(CppUnit::TestFactoryRegistry::getRegistry().makeTest());

    bool wasSuccessful = true;
    string test = "";
    if (0 == argc) {
        // run them all
        wasSuccessful = runner.run("");
    } else {
        int i = 0;
        while (i < argc) {
            if (debug)
                cerr << "Running " << argv[i] << endl;
            test = KeyValuePairTest::suite()->getName().append("::").append(argv[i]);
            wasSuccessful = wasSuccessful && runner.run(test);
        }
    }

    return wasSuccessful ? 0 : 1;
}
