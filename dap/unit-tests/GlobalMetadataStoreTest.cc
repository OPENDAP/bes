// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of libdap, A C++ implementation of the OPeNDAP Data
// Access Protocol.

// Copyright (c) 2002,2003 OPeNDAP, Inc.
// Author: James Gallagher <jgallagher@opendap.org>
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

#include <unistd.h>

#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>

#include <Array.h>
#include <Byte.h>
#include <DAS.h>
#include <DDS.h>
#include <DDXParserSAX2.h>

#include <GetOpt.h>
#include <GNURegex.h>
#include <util.h>
#include <debug.h>

#include <BaseTypeFactory.h>

#include "BESError.h"
#include "TheBESKeys.h"
#include "BESDebug.h"

#include "GlobalMetadataStore.h"

#include "test_utils.h"
#include "test_config.h"

static bool debug = false;
static bool bes_debug = false;
static bool clean = true;

#undef DBG
#define DBG(x) do { if (debug) (x); } while(false);

#define DEBUG_KEY "metadata_store,cache,cache2"

using namespace CppUnit;
using namespace std;
using namespace libdap;

namespace bes {

// Move this into the class when we goto C++-11
static const string c_mds_prefix = "mds_"; // used when cleaning the cache, etc.
static const string c_mds_name = "/mds";
static const string c_mds_baselines = string(TEST_SRC_DIR) + "/mds_baselines";


class GlobalMetadataStoreTest: public TestFixture {
private:
    DDS *d_test_dds;
    BaseTypeFactory d_btf;

    string d_mds_dir;
    GlobalMetadataStore *d_mds;

public:
    GlobalMetadataStoreTest() :
        d_test_dds(0), d_mds_dir(string(TEST_BUILD_DIR).append(c_mds_name)), d_mds(0)
    {
    }

    ~GlobalMetadataStoreTest()
    {
    }

    void setUp()
    {
        DBG(cerr << __func__ << " - BEGIN" << endl);

        if (bes_debug) BESDebug::SetUp(string("cerr,") + DEBUG_KEY);

        if (clean) clean_cache_dir(c_mds_name);

        // Contains BES Log parameters but not cache names
        TheBESKeys::ConfigFile = (string) TEST_SRC_DIR + "/bes.conf";

        DBG(cerr << __func__ << " - END" << endl);
    }

    void tearDown()
    {
        DBG(cerr << __func__ << " - BEGIN" << endl);

        delete d_test_dds;

        if (clean) clean_cache_dir(d_mds_dir);

        DBG(cerr << __func__ << " - END" << endl);
    }

#if 0
    bool re_match(Regex &r, const string &s)
    {
        DBG(cerr << "s.length(): " << s.length() << endl);
        int pos = r.match(s.c_str(), s.length());
        DBG(cerr << "r.match(s): " << pos << endl);
        return pos > 0 && static_cast<unsigned>(pos) == s.length();
    }

    bool re_match_binary(Regex &r, const string &s)
    {
        DBG(cerr << "s.length(): " << s.length() << endl);
        int pos = r.match(s.c_str(), s.length());
        DBG(cerr << "r.match(s): " << pos << endl);
        return pos > 0;
    }
#endif

    void ctor_test_1()
    {
        DBG(cerr << __func__ << " - BEGIN" << endl);

        try {
            // The call to get_instance should fail since the directory is named,
            // but does not exist and cannot be made.
            d_mds = GlobalMetadataStore::get_instance("/new", c_mds_prefix, 1000);
            DBG(cerr << "retrieved GlobalMetadataStore instance: " << d_mds << endl);
            CPPUNIT_FAIL("get_instance() Should not return when the non-existent directory cannot be created");
        }
        catch (BESError &e) {
            CPPUNIT_ASSERT(!e.get_message().empty());
        }

        DBG(cerr << __func__ << " - END" << endl);
    }

    void ctor_test_2()
    {
        DBG(cerr << __func__ << " - BEGIN" << endl);

        try {
            // This one should work and will make the directory
            d_mds = GlobalMetadataStore::get_instance(d_mds_dir, c_mds_prefix, 1000);
            DBG(cerr << "retrieved GlobalMetadataStore instance: " << d_mds << endl);
            CPPUNIT_ASSERT(d_mds);
        }
        catch (BESError &e) {
            CPPUNIT_FAIL("Caught exception: " + e.get_message());
        }

        DBG(cerr << __func__ << " - END" << endl);
    }

    // This test may fail if the -k option is used.
    void cache_a_dap2_dds_response()
    {
        DBG(cerr << __func__ << " - BEGIN" << endl);

        try {
            d_mds = GlobalMetadataStore::get_instance(d_mds_dir, c_mds_prefix, 1000);
            DBG(cerr << "cache_a_response() - Retrieved GlobalMetadataStore object: " << d_mds << endl);

            // Get a DDS to cache.
            d_test_dds = new DDS(&d_btf);
            DDXParser dp(&d_btf);
            string cid; // This is an unused value-result parameter. jhrg 5/10/16
            dp.intern(string(TEST_SRC_DIR) + "/input-files/test.05.ddx", d_test_dds, cid);

            // for these tests, set the filename to the dataset_name. ...keeps the cache names short
            d_test_dds->filename(d_test_dds->get_dataset_name());
            DBG(cerr << "DDS Name: " << d_test_dds->get_dataset_name() << endl);
            CPPUNIT_ASSERT(d_test_dds);

            // Store it - this will work if the the code is cleaning the cache.
#if 0
            bool stored = d_mds->store_dap2_response(d_test_dds, &DDS::print, d_test_dds->get_dataset_name() + ".dds_r");
#endif
            GlobalMetadataStore::StreamDDS write_the_dds_response(d_test_dds);
            bool stored = d_mds->store_dap2_response(d_test_dds, write_the_dds_response, d_test_dds->get_dataset_name() + ".dds_r");

            DBG(cerr << __func__ << " - stored: " << stored << endl);
            CPPUNIT_ASSERT(stored);

            // Now check the file
            string baseline_name = c_mds_baselines + "/" + c_mds_prefix + "SimpleTypes.dds_r";
            DBG(cerr << "Reading baseline: " << baseline_name << endl);
            CPPUNIT_ASSERT(access(baseline_name.c_str(), R_OK) == 0);

            string test_05_dds_baseline = read_test_baseline(baseline_name);

            string response_name = d_mds_dir + "/" + c_mds_prefix + "SimpleTypes.dds_r";
            // read_test_baseline() just reads stuff from a file - it will work for the response, too.
            DBG(cerr << "Reading response: " << response_name << endl);
            CPPUNIT_ASSERT(access(response_name.c_str(), R_OK) == 0);

            string stored_response = read_test_baseline(response_name);

            CPPUNIT_ASSERT(stored_response == test_05_dds_baseline);
        }
        catch (BESError &e) {
            CPPUNIT_FAIL(e.get_message());
        }

        DBG(cerr << __func__ << " - END" << endl);
    }

    // This test may fail if the -k option is used.
    void cache_a_dap2_das_response()
    {
        DBG(cerr << __func__ << " - BEGIN" << endl);

        try {
            d_mds = GlobalMetadataStore::get_instance(d_mds_dir, c_mds_prefix, 1000);
            DBG(cerr << "cache_a_response() - Retrieved GlobalMetadataStore object: " << d_mds << endl);

            // Get a DDS to cache.
            d_test_dds = new DDS(&d_btf);
            DDXParser dp(&d_btf);
            string cid; // This is an unused value-result parameter. jhrg 5/10/16
            dp.intern(string(TEST_SRC_DIR) + "/input-files/test.05.ddx", d_test_dds, cid);

            // for these tests, set the filename to the dataset_name. ...keeps the cache names short
            d_test_dds->filename(d_test_dds->get_dataset_name());
            DBG(cerr << "DDS Name: " << d_test_dds->get_dataset_name() << endl);
            CPPUNIT_ASSERT(d_test_dds);

            // Store it - this will work if the the code is cleaning the cache.
#if 0
            bool stored = d_mds->store_dap2_response(d_test_dds, &DDS::print_das, d_test_dds->get_dataset_name() + ".das_r");
#endif
            GlobalMetadataStore::StreamDAS write_the_das_response(d_test_dds);
            bool stored = d_mds->store_dap2_response(d_test_dds, write_the_das_response, d_test_dds->get_dataset_name() + ".das_r");

            DBG(cerr << __func__ << " - stored: " << stored << endl);
            CPPUNIT_ASSERT(stored);

            // Now check the file
            string baseline_name = c_mds_baselines + "/" + c_mds_prefix + "SimpleTypes.das_r";
            DBG(cerr << "Reading baseline: " << baseline_name << endl);
            CPPUNIT_ASSERT(access(baseline_name.c_str(), R_OK) == 0);

            string test_05_dds_baseline = read_test_baseline(baseline_name);

            string response_name = d_mds_dir + "/" + c_mds_prefix + "SimpleTypes.das_r";
            // read_test_baseline() just reads stuff from a file - it will work for the response, too.
            DBG(cerr << "Reading response: " << response_name << endl);
            CPPUNIT_ASSERT(access(response_name.c_str(), R_OK) == 0);

            string stored_response = read_test_baseline(response_name);

            CPPUNIT_ASSERT(stored_response == test_05_dds_baseline);
        }
        catch (BESError &e) {
            CPPUNIT_FAIL(e.get_message());
        }

        DBG(cerr << __func__ << " - END" << endl);
    }

    void add_object_test()
    {
        DBG(cerr << __func__ << " - BEGIN" << endl);

        try {
            d_mds = GlobalMetadataStore::get_instance(d_mds_dir, c_mds_prefix, 1000);
            DBG(cerr << __func__ << " - Retrieved GlobalMetadataStore object: " << d_mds << endl);

            // Get a DDS to cache.
            d_test_dds = new DDS(&d_btf);
            DDXParser dp(&d_btf);
            string cid; // This is an unused value-result parameter. jhrg 5/10/16
            dp.intern(string(TEST_SRC_DIR) + "/input-files/test.05.ddx", d_test_dds, cid);

            // for these tests, set the filename to the dataset_name. ...keeps the cache names short
            d_test_dds->filename(d_test_dds->get_dataset_name());
            DBG(cerr << "DDS Name: " << d_test_dds->get_dataset_name() << endl);
            CPPUNIT_ASSERT(d_test_dds);

            // Store it - this will work if the the code is cleaning the cache.
            bool stored = d_mds->add_object(d_test_dds, d_test_dds->get_dataset_name());

            DBG(cerr << __func__ << " - stored: " << stored << endl);
            CPPUNIT_ASSERT(stored);

            // look for the files
            string dds_cache_name = d_mds->get_cache_file_name(d_mds->get_hash(d_test_dds->get_dataset_name().append("dds_r")), false /*mangle*/);
            DBG(cerr << __func__ << " - dds_cache_name: " << dds_cache_name << endl);
            CPPUNIT_ASSERT(access(dds_cache_name.c_str(), R_OK) == 0);

            string das_cache_name = d_mds->get_cache_file_name(d_mds->get_hash(d_test_dds->get_dataset_name().append("das_r")), false /*mangle*/);
            DBG(cerr << __func__ << " - das_cache_name: " << das_cache_name << endl);
            CPPUNIT_ASSERT(access(das_cache_name.c_str(), R_OK) == 0);
        }
        catch (BESError &e) {
            CPPUNIT_FAIL(e.get_message());
        }

        DBG(cerr << __func__ << " - END" << endl);
    }

    void get_dds_response_test() {
        DBG(cerr << __func__ << " - BEGIN" << endl);

         try {
             d_mds = GlobalMetadataStore::get_instance(d_mds_dir, c_mds_prefix, 1000);
             DBG(cerr << __func__ << " - Retrieved GlobalMetadataStore object: " << d_mds << endl);

             // Get a DDS to cache.
             d_test_dds = new DDS(&d_btf);
             DDXParser dp(&d_btf);
             string cid; // This is an unused value-result parameter. jhrg 5/10/16
             dp.intern(string(TEST_SRC_DIR) + "/input-files/test.05.ddx", d_test_dds, cid);

             // for these tests, set the filename to the dataset_name. ...keeps the cache names short
             d_test_dds->filename(d_test_dds->get_dataset_name());
             DBG(cerr << "DDS Name: " << d_test_dds->get_dataset_name() << endl);
             CPPUNIT_ASSERT(d_test_dds);

             // Store it - this will work if the the code is cleaning the cache.
             bool stored = d_mds->add_object(d_test_dds, d_test_dds->get_dataset_name());

             DBG(cerr << __func__ << " - stored: " << stored << endl);
             CPPUNIT_ASSERT(stored);

             // Now lets read the object from the cache
             ostringstream oss;
             d_mds->get_dds_response(d_test_dds->get_dataset_name(), oss);
             DBG(cerr << "DDS response: " << endl << oss.str() << endl);

             string baseline_name = c_mds_baselines + "/" + c_mds_prefix + "SimpleTypes.dds_r";
             DBG(cerr << "Reading baseline: " << baseline_name << endl);
             CPPUNIT_ASSERT(access(baseline_name.c_str(), R_OK) == 0);

             string test_05_dds_baseline = read_test_baseline(baseline_name);

             CPPUNIT_ASSERT(test_05_dds_baseline == oss.str());
         }
         catch (BESError &e) {
             CPPUNIT_FAIL(e.get_message());
         }

         DBG(cerr << __func__ << " - END" << endl);
    }

    void get_das_response_test() {
        DBG(cerr << __func__ << " - BEGIN" << endl);

         try {
             d_mds = GlobalMetadataStore::get_instance(d_mds_dir, c_mds_prefix, 1000);
             DBG(cerr << __func__ << " - Retrieved GlobalMetadataStore object: " << d_mds << endl);

             // Get a DDS to cache.
             d_test_dds = new DDS(&d_btf);
             DDXParser dp(&d_btf);
             string cid; // This is an unused value-result parameter. jhrg 5/10/16
             dp.intern(string(TEST_SRC_DIR) + "/input-files/test.05.ddx", d_test_dds, cid);

             // for these tests, set the filename to the dataset_name. ...keeps the cache names short
             d_test_dds->filename(d_test_dds->get_dataset_name());
             DBG(cerr << "DDS Name: " << d_test_dds->get_dataset_name() << endl);
             CPPUNIT_ASSERT(d_test_dds);

             // Store it - this will work if the the code is cleaning the cache.
             bool stored = d_mds->add_object(d_test_dds, d_test_dds->get_dataset_name());

             DBG(cerr << __func__ << " - stored: " << stored << endl);
             CPPUNIT_ASSERT(stored);

             // Now lets read the object from the cache
             ostringstream oss;
             d_mds->get_das_response(d_test_dds->get_dataset_name(), oss);
             DBG(cerr << "DAS response: " << endl << oss.str() << endl);

             string baseline_name = c_mds_baselines + "/" + c_mds_prefix + "SimpleTypes.das_r";
             DBG(cerr << "Reading baseline: " << baseline_name << endl);
             CPPUNIT_ASSERT(access(baseline_name.c_str(), R_OK) == 0);

             string test_05_das_baseline = read_test_baseline(baseline_name);

             CPPUNIT_ASSERT(test_05_das_baseline == oss.str());
         }
         catch (BESError &e) {
             CPPUNIT_FAIL(e.get_message());
         }

         DBG(cerr << __func__ << " - END" << endl);
    }

    void remove_object_test()
    {
        DBG(cerr << __func__ << " - BEGIN" << endl);

        try {
            d_mds = GlobalMetadataStore::get_instance(d_mds_dir, c_mds_prefix, 1000);
            DBG(cerr << __func__ << " - Retrieved GlobalMetadataStore object: " << d_mds << endl);

            // Get a DDS to cache.
            d_test_dds = new DDS(&d_btf);
            DDXParser dp(&d_btf);
            string cid; // This is an unused value-result parameter. jhrg 5/10/16
            dp.intern(string(TEST_SRC_DIR) + "/input-files/test.05.ddx", d_test_dds, cid);

            // for these tests, set the filename to the dataset_name. ...keeps the cache names short
            d_test_dds->filename(d_test_dds->get_dataset_name());
            DBG(cerr << "DDS Name: " << d_test_dds->get_dataset_name() << endl);
            CPPUNIT_ASSERT(d_test_dds);

            // Store it - this will work if the the code is cleaning the cache.
            bool stored = d_mds->add_object(d_test_dds, d_test_dds->get_dataset_name());

            DBG(cerr << __func__ << " - stored: " << stored << endl);
            CPPUNIT_ASSERT(stored);

            // look for the files
            string dds_cache_name = d_mds->get_cache_file_name(d_mds->get_hash(d_test_dds->get_dataset_name().append("dds_r")), false /*mangle*/);
            DBG(cerr << __func__ << " - dds_cache_name: " << dds_cache_name << endl);
            CPPUNIT_ASSERT(access(dds_cache_name.c_str(), R_OK) == 0);

            string das_cache_name = d_mds->get_cache_file_name(d_mds->get_hash(d_test_dds->get_dataset_name().append("das_r")), false /*mangle*/);
            DBG(cerr << __func__ << " - das_cache_name: " << das_cache_name << endl);
            CPPUNIT_ASSERT(access(das_cache_name.c_str(), R_OK) == 0);

            bool removed = d_mds->remove_object(d_test_dds->get_dataset_name());
            CPPUNIT_ASSERT(removed);

            CPPUNIT_ASSERT(access(dds_cache_name.c_str(), R_OK) != 0);
            CPPUNIT_ASSERT(access(das_cache_name.c_str(), R_OK) != 0);

        }
        catch (BESError &e) {
            CPPUNIT_FAIL(e.get_message());
        }

        DBG(cerr << __func__ << " - END" << endl);
    }

    CPPUNIT_TEST_SUITE( GlobalMetadataStoreTest );

    CPPUNIT_TEST(ctor_test_1);
    CPPUNIT_TEST(ctor_test_2);
    CPPUNIT_TEST(cache_a_dap2_dds_response);
    CPPUNIT_TEST(cache_a_dap2_das_response);
    CPPUNIT_TEST(add_object_test);
    CPPUNIT_TEST(get_dds_response_test);
    CPPUNIT_TEST(get_das_response_test);
    CPPUNIT_TEST(remove_object_test);

    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(GlobalMetadataStoreTest);

}

int main(int argc, char*argv[])
{

    GetOpt getopt(argc, argv, "dbkh");
    int option_char;
    while ((option_char = getopt()) != -1)
        switch (option_char) {
        case 'd':
            debug = 1;  // debug is a static global
            break;
        case 'b':
            bes_debug = true;  // bes_debug is a static global
            cerr << "##### BES DEBUG is ON" << endl;
            break;
        case 'k':   // -k turns off cleaning the metadata store dir
            clean = false;
            break;
        case 'h': {     // help - show test names
            cerr << "Usage: GlobalMetadataStoreTest has the following tests:" << endl;
            const std::vector<Test*> &tests = bes::GlobalMetadataStoreTest::suite()->getTests();
            unsigned int prefix_len = bes::GlobalMetadataStoreTest::suite()->getName().append("::").length();
            for (std::vector<Test*>::const_iterator i = tests.begin(), e = tests.end(); i != e; ++i) {
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
            test = bes::GlobalMetadataStoreTest::suite()->getName().append("::").append(argv[i]);
            wasSuccessful = wasSuccessful && runner.run(test);

            ++i;
        }
    }

    return wasSuccessful ? 0 : 1;
}
