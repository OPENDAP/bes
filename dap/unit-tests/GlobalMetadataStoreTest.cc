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
static const string c_mds_baseline = string(TEST_SRC_DIR) + "/mds_baseline";


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

        TheBESKeys::ConfigFile = (string) TEST_SRC_DIR + "/input-files/test.keys"; // intentionally empty file. jhrg 10/20/15

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
            bool stored = d_mds->store_dap2_response(d_test_dds, &DDS::print, d_test_dds->get_dataset_name() + ".dds_r");

            DBG(cerr << __func__ << " - stored: " << stored << endl);
            CPPUNIT_ASSERT(stored);

            // Now check the file
            string baseline_name = c_mds_baseline + "/" + c_mds_prefix + "SimpleTypes.dds_r";
            DBG(cerr << "Reading baseline: " << baseline_name << endl);
            string test_05_dds_baseline = read_test_baseline(baseline_name);

            string response_name = d_mds_dir + "/" + c_mds_prefix + "SimpleTypes.dds_r";
            // read_test_baseline() just reads stuff from a file - it will work for the response, too.
            DBG(cerr << "Reading response: " << response_name << endl);
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
            bool stored = d_mds->store_dap2_response(d_test_dds, &DDS::print_das, d_test_dds->get_dataset_name() + ".das_r");

            DBG(cerr << __func__ << " - stored: " << stored << endl);
            CPPUNIT_ASSERT(stored);

            // Now check the file
            string baseline_name = c_mds_baseline + "/" + c_mds_prefix + "SimpleTypes.das_r";
            DBG(cerr << "Reading baseline: " << baseline_name << endl);
            string test_05_dds_baseline = read_test_baseline(baseline_name);

            string response_name = d_mds_dir + "/" + c_mds_prefix + "SimpleTypes.das_r";
            // read_test_baseline() just reads stuff from a file - it will work for the response, too.
            DBG(cerr << "Reading response: " << response_name << endl);
            string stored_response = read_test_baseline(response_name);

            CPPUNIT_ASSERT(stored_response == test_05_dds_baseline);
        }
        catch (BESError &e) {
            CPPUNIT_FAIL(e.get_message());
        }

        DBG(cerr << __func__ << " - END" << endl);
    }
   // The first call reads values into the DDS, stores a copy in the cache and
    // returns the DDS. The second call reads the value from the cache.
    //
    // Use load_from_cache() (Private interface) to read the data.
#if 0
    void cache_and_read_a_response()
    {
        DBG(cerr << "cache_and_read_a_response() - BEGIN" << endl);

        cache = GlobalMetadataStore::get_instance(d_cache, c_mds_prefix, 1000);
        try {
            const string constraint = "test(\"bar\")";

            // This code is here to load the DataDDX response into the cache if it is not
            // there already. If it is there, it reads it from the cache.
            DDS *result = cache->get_or_cache_dataset(d_test_dds, constraint);

            CPPUNIT_ASSERT(result);
            int var_count = result->var_end() - result->var_begin();
            CPPUNIT_ASSERT(var_count == 1);

            //DDS *result2 = cache->get_or_cache_dataset(test_dds, "test(\"bar\")");
            string resource_id = cache->get_resource_id(d_test_dds, constraint);
            string cache_file_name = cache->get_hash_basename(resource_id);

            DBG(
                cerr << "cache_and_read_a_response() - resource_id: " << resource_id << ", cache_file_name: "
                    << cache_file_name << endl);

            DDS *result2 = cache->load_from_cache(resource_id, cache_file_name);
            // Better not be null!
            CPPUNIT_ASSERT(result2);
            result2->filename("function_result_SimpleTypes");

            // There are nine variables in test.05.ddx
            var_count = result2->var_end() - result2->var_begin();
            DBG(cerr << "cache_and_read_a_response() - var_count: " << var_count << endl);
            CPPUNIT_ASSERT(var_count == 1);

            ostringstream oss;
            DDS::Vars_iter i = result2->var_begin();
            while (i != result2->var_end()) {
                DBG(cerr << "Variable " << (*i)->name() << endl);
                // this will incrementally add the string rep of values to 'oss'
                (*i)->print_val(oss, "", false /*print declaration */);
                DBG(cerr << "Value " << oss.str() << endl);
                ++i;
            }

            CPPUNIT_ASSERT(oss.str().compare("{{0, 1, 2},{3, 4, 5},{6, 7, 8}}") == 0);
        }
        catch (Error &e) {
            CPPUNIT_FAIL(e.get_error_message());
        }

        DBG(cerr << "cache_and_read_a_response() - END" << endl);
    }

    // The first call reads values into the DDS, stores a copy in the cache and
    // returns the DDS. The second call reads the value from the cache.
    //
    // Use the public interface to read the data (cache_dataset()), but w/o a
    // constraint
    void cache_and_read_a_response2()
    {
        DBG(cerr << "cache_and_read_a_response() - BEGIN" << endl);

        cache = GlobalMetadataStore::get_instance(d_cache, c_mds_prefix, 1000);
        try {
            // This code is here to load the DataDDX response into the cache if it is not
            // there already. If it is there, it reads it from the cache.
            DDS *result = cache->get_or_cache_dataset(d_test_dds, "test(\"bar\")");

            CPPUNIT_ASSERT(result);
            int var_count = result->var_end() - result->var_begin();
            CPPUNIT_ASSERT(var_count == 1);

            DDS *result2 = cache->get_or_cache_dataset(d_test_dds, "test(\"bar\")");
            // Better not be null!
            CPPUNIT_ASSERT(result2);
            result2->filename("function_result_SimpleTypes");

            // There are nine variables in test.05.ddx
            var_count = result2->var_end() - result2->var_begin();
            DBG(cerr << "cache_and_read_a_response() - var_count: " << var_count << endl);
            CPPUNIT_ASSERT(var_count == 1);

            ostringstream oss;
            DDS::Vars_iter i = result2->var_begin();
            while (i != result2->var_end()) {
                DBG(cerr << "Variable " << (*i)->name() << endl);
                // this will incrementally add the string rep of values to 'oss'
                (*i)->print_val(oss, "", false /*print declaration */);
                DBG(cerr << "Value " << oss.str() << endl);
                ++i;
            }

            CPPUNIT_ASSERT(oss.str().compare("{{0, 1, 2},{3, 4, 5},{6, 7, 8}}") == 0);
        }
        catch (Error &e) {
            CPPUNIT_FAIL(e.get_error_message());
        }

        DBG(cerr << "cache_and_read_a_response() - END" << endl);
    }
#endif

    CPPUNIT_TEST_SUITE( GlobalMetadataStoreTest );

    CPPUNIT_TEST(ctor_test_1);
    CPPUNIT_TEST(ctor_test_2);
    CPPUNIT_TEST(cache_a_dap2_dds_response);
    CPPUNIT_TEST(cache_a_dap2_das_response);

#if 0
    CPPUNIT_TEST(cache_a_response);
    CPPUNIT_TEST(cache_and_read_a_response);
    CPPUNIT_TEST(cache_and_read_a_response2);
#endif
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
