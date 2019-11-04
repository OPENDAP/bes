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

#include <ConstraintEvaluator.h>
#include <DAS.h>
#include <DDS.h>

#include <GetOpt.h>
#include <GNURegex.h>
#include <util.h>
#include <mime_util.h>
#include <debug.h>

#include <DMR.h>
#include <D4Group.h>
#include <D4ParserSax2.h>
#include <test/D4TestTypeFactory.h>

#include "TheBESKeys.h"
#include "BESStoredDapResultCache.h"
#include "BESDapResponseBuilder.h"
#include "BESDebug.h"

#include "test_utils.h"
#include "test_config.h"

#define BES_DATA_ROOT "BES.Data.RootDirectory"
#define BES_CATALOG_ROOT "BES.Catalog.catalog.RootDirectory"

using namespace CppUnit;
using namespace std;
using namespace libdap;

int test_variable_sleep_interval = 0;

static bool debug = false;
static bool parser_debug = false;
static const string c_cache_name = "/dap4_result_cache";

#undef DBG
#define DBG(x) do { if (debug) (x); } while(false);

class StoredDap4ResultTest: public TestFixture {
private:
    D4TestTypeFactory *d4_ttf;
    D4BaseTypeFactory *d4_btf;
    DMR *test_01_dmr;
    D4ParserSax2 *d4parser;

    BESDapResponseBuilder responseBuilder;

    string d_data_root_dir;
    string d_stored_result_subdir;
    string d_response_cache;
    string d_baseline_local_id;
    string d_test01_dmr_value_baseline;

public:
    StoredDap4ResultTest() :
        d4_ttf(0), d4_btf(0), test_01_dmr(0), d4parser(0), d_data_root_dir(string(TEST_SRC_DIR)), d_stored_result_subdir(
            c_cache_name), d_response_cache(string(TEST_SRC_DIR) + c_cache_name), d_baseline_local_id(""), d_test01_dmr_value_baseline(
            "")
    {
    }

    ~StoredDap4ResultTest()
    {
    }

    void setUp()
    {
        DBG(cerr << "setUp() - BEGIN" << endl);
        DBG(BESDebug::SetUp("cerr,all"));

        d4parser = new D4ParserSax2();
        DBG(cerr << "Built D4ParserSax2() " << endl);

        d4_ttf = new D4TestTypeFactory();
        DBG(cerr << "Built D4TestTypeFactory() " << endl);

        d4_btf = new D4BaseTypeFactory();
        DBG(cerr << "Built D4BaseTypeFactory() " << endl);

        test_01_dmr = new DMR(d4_ttf);
        DBG(cerr << "Built DMR(D4TestTypeFactory *) " << endl);

        string dmr_filename = (string) TEST_SRC_DIR + "/input-files/test_01.dmr";
        DBG(cerr << "Parsing DMR file " << dmr_filename << endl);
        d4parser->intern(read_test_baseline(dmr_filename), test_01_dmr, parser_debug);
        DBG(cerr << "Parsed DMR from file " << dmr_filename << endl);

        d_baseline_local_id = "/cache_baseline_files/result_1688151760629011709.dap";

        // for these tests, set the filename to the dataset_name. ...keeps the cache names short
        test_01_dmr->set_filename(test_01_dmr->name());

        // cid == http://dods.coas.oregonstate.edu:8080/dods/dts/test.01.blob
        DBG(cerr << "DDS Name: " << test_01_dmr->name() << endl);

        d_test01_dmr_value_baseline =
            "{{255, 255, 255, 255},{255, 255, 255, 255},{255, 255, 255, 255}}"
                "{{32000, 32000, 32000, 32000},{32000, 32000, 32000, 32000},"
                "{32000, 32000, 32000, 32000}}{{123456789, 123456789, 123456789, 123456789},"
                "{123456789, 123456789, 123456789, 123456789},{123456789, 123456789, 123456789, 123456789}}"
                "{{64000, 64000, 64000, 64000},{64000, 64000, 64000, 64000},{64000, 64000, 64000, 64000}}"
                "{{4026531840, 4026531840, 4026531840, 4026531840},{4026531840, 4026531840, 4026531840, 4026531840},"
                "{4026531840, 4026531840, 4026531840, 4026531840}}"
                "{{99.999, 99.999, 99.999, 99.999},{99.999, 99.999, 99.999, 99.999},{99.999, 99.999, 99.999, 99.999}}"
                "{{99.999, 99.999, 99.999, 99.999},{99.999, 99.999, 99.999, 99.999},{99.999, 99.999, 99.999, 99.999}}"
                "{{\"Silly test string: 1\", \"Silly test string: 1\", \"Silly test string: 1\", \"Silly test string: 1\"},"
                "{\"Silly test string: 1\", \"Silly test string: 1\", \"Silly test string: 1\", \"Silly test string: 1\"},"
                "{\"Silly test string: 1\", \"Silly test string: 1\", \"Silly test string: 1\", \"Silly test string: 1\"}}"
                "{{\"http://dcz.gso.uri.edu/avhrr-archive/archive.html\", \"http://dcz.gso.uri.edu/avhrr-archive/archive.html\", "
                "\"http://dcz.gso.uri.edu/avhrr-archive/archive.html\", \"http://dcz.gso.uri.edu/avhrr-archive/archive.html\"},"
                "{\"http://dcz.gso.uri.edu/avhrr-archive/archive.html\", \"http://dcz.gso.uri.edu/avhrr-archive/archive.html\", "
                "\"http://dcz.gso.uri.edu/avhrr-archive/archive.html\", \"http://dcz.gso.uri.edu/avhrr-archive/archive.html\"},"
                "{\"http://dcz.gso.uri.edu/avhrr-archive/archive.html\", \"http://dcz.gso.uri.edu/avhrr-archive/archive.html\", "
                "\"http://dcz.gso.uri.edu/avhrr-archive/archive.html\", \"http://dcz.gso.uri.edu/avhrr-archive/archive.html\"}}"
                "{{123456789, 123456789, 123456789, 123456789, 123456789},{123456789, 123456789, 123456789, 123456789, 123456789},"
                "{123456789, 123456789, 123456789, 123456789, 123456789}}{{123456789, 123456789, 123456789, 123456789},{123456789, "
                "123456789, 123456789, 123456789},{123456789, 123456789, 123456789, 123456789},{123456789, 123456789, 123456789, "
                "123456789},{123456789, 123456789, 123456789, 123456789}}";

        clean_cache_dir(d_response_cache);

        DBG(cerr << "setUp() - END" << endl);
    }

    void tearDown()
    {
        DBG(cerr << "tearDown() - BEGIN" << endl);

        //clean_cache(d_response_cache, "result_");
        clean_cache_dir(d_response_cache);

        delete d4parser;
        delete test_01_dmr;
        delete d4_ttf;

        DBG(cerr << "tearDown() - END" << endl);
    }

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

    // Because setup() and teardown() clean out the cache directory, there should
    // never be a cached item; calling read_cached_dataset() should return a
    // valid DDS with data and store a copy in the cache.
    void cache_a_dap4_response()
    {
        DBG(cerr << "**** cache_a_dap4_response() - BEGIN" << endl);

        BESStoredDapResultCache *cache = BESStoredDapResultCache::get_instance(d_data_root_dir, d_stored_result_subdir,
            "result_", 1000);

        DBG(cerr << "cache_a_dap4_response() - Got the instance of BESStoredResultCache object: ");
        if (cache) {
            DBG(cerr << *cache << endl);
        }
        else {
            DBG(cerr << "NULL" << endl);
        }

        string stored_result_local_id;
        try {
            // TODO Could stat the cache file to make sure it's not already there.
            DBG(cerr << "cache_a_response() - caching a dataset... " << endl);

            // Mark all the stuff in the dataset so that everything gets written down!
            test_01_dmr->root()->set_send_p(true);

            // Write it down
            stored_result_local_id = cache->store_dap4_result(*test_01_dmr, "", &responseBuilder);

            DBG(cerr << "cache_a_dap4_response() -  baseline_local_id: " << name_path(d_baseline_local_id) << endl);
            DBG(cerr << "cache_a_dap4_response() - Cached response id: " << name_path(stored_result_local_id) << endl);

            CPPUNIT_ASSERT(name_path(stored_result_local_id) == name_path(d_baseline_local_id));
            // TODO Stat the cache file to check it's size
            cache->delete_instance();
        }
        catch (Error &e) {
            CPPUNIT_FAIL(e.get_error_message());
        }
        DBG(cerr << "**** cache_a_dap4_response() - END" << endl);

    }

    // The first call reads values into the DDS, stores a copy in the cache and
    // returns the DDS. The second call reads the value from the cache.
    void cache_and_read_a_dap4_response()
    {
        DBG(cerr << "**** cache_and_read_a_dap4_response() - BEGIN" << endl);

        BESStoredDapResultCache *cache = BESStoredDapResultCache::get_instance(d_data_root_dir, d_stored_result_subdir,
            "result_", 1000);

        DBG(cerr << "cache_and_read_a_dap4_response() - Got BESStoredDapResultCache instance." << endl);
        string stored_result_local_id;
        try {
            // TODO Could stat the cache file to make sure it's not already there.
            DBG(cerr << "cache_and_read_a_dap4_response() - caching a dataset... " << endl);

            // Mark all the stuff in the dataset so that everything gets written down!
            test_01_dmr->root()->set_send_p(true);

            // Write it down
            stored_result_local_id = cache->store_dap4_result(*test_01_dmr, "", &responseBuilder);

            DBG(
                cerr << "cache_and_read_a_dap4_response() - Cached response id: " << name_path(stored_result_local_id)
                    << endl);
            CPPUNIT_ASSERT(name_path(stored_result_local_id) == name_path(d_baseline_local_id));

            // DDS *get_cached_dap2_data_ddx(const string &cache_file_name, BaseTypeFactory *factory, const string &dataset)
            // Force read from the cache file

            string cacheFileName = d_data_root_dir + stored_result_local_id;
            DBG(
                cerr << "cache_and_read_a_dap4_response() - Reading stored DAP4 dataset. cache filename: "
                    << cacheFileName << endl);

            DMR *cached_data = cache->get_cached_dap4_data(cacheFileName, d4_btf, "test.01");

            DBG(cerr << "cache_and_read_a_dap4_response() - Stored DAP4 dataset has been read." << endl);

            int response_element_count = cached_data->root()->element_count(true);
            DBG(cerr << "cache_and_read_a_dap4_response() - response_element_count: " << response_element_count << endl);

            CPPUNIT_ASSERT(cached_data);
            // There are nine variables in test.05.ddx
            CPPUNIT_ASSERT(response_element_count == 11);

            ostringstream oss;
            Constructor::Vars_iter i = cached_data->root()->var_begin();
            while (i != cached_data->root()->var_end()) {
                DBG(cerr << "Variable " << (*i)->name() << endl);
                // this will incrementally add thr string rep of values to 'oss'
                (*i)->print_val(oss, "", false /*print declaration */);
                DBG(cerr << "response_value: " << oss.str() << endl);
                ++i;
            }

            DBG(
                cerr << "cache_and_read_a_dap4_response() - d_test01_dmr_value_baseline: "
                    << d_test01_dmr_value_baseline << endl);

            // In this regex the value of <number> in the DAP2 Str variable (Silly test string: <number>)
            // is a any single digit. The *Test classes implement a counter and return strings where
            // <number> is 1, 2, ..., and running several of the tests here in a row will get a range of
            // values for <number>.
            CPPUNIT_ASSERT(d_test01_dmr_value_baseline == oss.str());

            delete cached_data;
            cached_data = 0;
            // cache->delete_instance();
            cache->delete_instance();
        }
        catch (Error &e) {
            CPPUNIT_FAIL(e.get_error_message());
        }
        DBG(cerr << "**** cache_and_read_a_dap4_response() - END" << endl);

    }

CPPUNIT_TEST_SUITE( StoredDap4ResultTest );

    CPPUNIT_TEST(cache_a_dap4_response);
    CPPUNIT_TEST(cache_and_read_a_dap4_response);

    CPPUNIT_TEST_SUITE_END()
    ;
};

CPPUNIT_TEST_SUITE_REGISTRATION(StoredDap4ResultTest);

int main(int argc, char*argv[])
{

    GetOpt getopt(argc, argv, "dh");
    int option_char;
    while ((option_char = getopt()) != EOF)
        switch (option_char) {
        case 'd':
            debug = 1;  // debug is a static global
            break;
        case 'h': {     // help - show test names
            cerr << "Usage: StoredDap4ResultTest has the following tests:" << endl;
            const std::vector<Test*> &tests = StoredDap4ResultTest::suite()->getTests();
            unsigned int prefix_len = StoredDap4ResultTest::suite()->getName().append("::").length();
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
            test = StoredDap4ResultTest::suite()->getName().append("::").append(argv[i]);
            wasSuccessful = wasSuccessful && runner.run(test);
	    ++i;
        }
    }

    return wasSuccessful ? 0 : 1;
}
