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
#include <DDXParserSAX2.h>

#include <GetOpt.h>
#include <GNURegex.h>
#include <util.h>
#include <debug.h>

#include <test/TestTypeFactory.h>

#include "BESDapResponseCache.h"
#include "TheBESKeys.h"
#include "BESDebug.h"

#include "test_utils.h"
#include "test_config.h"

using namespace CppUnit;
using namespace std;
using namespace libdap;

int test_variable_sleep_interval = 0;

static bool debug = false;
static bool bes_debug = false;
static bool clean = true;
static const string c_cache_name = "/response_cache";

#undef DBG
#define DBG(x) do { if (debug) (x); } while(false);

using namespace libdap;

class ResponseCacheTest: public TestFixture {
private:
    DDXParser dp;
	TestTypeFactory ttf;
	DDS *test_dds;

	ConstraintEvaluator eval;

    string d_response_cache;
    BESDapResponseCache *cache;

public:
    ResponseCacheTest() : dp(&ttf), test_dds(0), d_response_cache(string(TEST_SRC_DIR) + c_cache_name), cache(0) {
    }

    ~ResponseCacheTest() {
    }

    void setUp() {
    	DBG(cerr << "setUp() - BEGIN" << endl);
    	if (bes_debug)
    	    BESDebug::SetUp("cerr,response_cache");

    	string cid; // This is an unused value-result parameter. jhrg 5/10/16
    	test_dds = new DDS(&ttf);
    	dp.intern((string)TEST_SRC_DIR + "/input-files/test.05.ddx", test_dds, cid);

    	// for these tests, set the filename to the dataset_name. ...keeps the cache names short
    	test_dds->filename(test_dds->get_dataset_name());

    	DBG(cerr << "DDS Name: " << test_dds->get_dataset_name() << endl);

    	if (clean)
    	    clean_cache_dir(d_response_cache);

        TheBESKeys::ConfigFile = (string) TEST_SRC_DIR + "/input-files/test.keys"; // empty file. jhrg 10/20/15

    	DBG(cerr << "setUp() - END" << endl);
    }

	void tearDown() {
		DBG(cerr << "tearDown() - BEGIN" << endl);

		delete test_dds;

		if (clean)
		    clean_cache_dir(d_response_cache);

		DBG(cerr << "tearDown() - END" << endl);
	}

    bool re_match(Regex &r, const string &s) {
        DBG(cerr << "s.length(): " << s.length() << endl);
        int pos = r.match(s.c_str(), s.length());
        DBG(cerr << "r.match(s): " << pos << endl);
        return pos > 0 && static_cast<unsigned> (pos) == s.length();
    }

    bool re_match_binary(Regex &r, const string &s) {
        DBG(cerr << "s.length(): " << s.length() << endl);
        int pos = r.match(s.c_str(), s.length());
        DBG(cerr << "r.match(s): " << pos << endl);
        return pos > 0;
    }

    // The directory 'never' does not exist; the cache won't be initialized,
    // so is_available() should be false
    void ctor_test_1() {
		DBG(cerr << "ctor_test_1() - BEGIN" << endl);

		string cacheDir = string(TEST_SRC_DIR) + "/never";
		string prefix = "rc";
		long size = 1000;

    	cache =  BESDapResponseCache::get_instance(cacheDir, prefix, size);
		DBG(cerr << "ctor_test_1() - retrieved BESDapResponseCache instance: " << cache << endl);

    	CPPUNIT_ASSERT(!cache);

		DBG(cerr << "ctor_test_1() - END" << endl);
    }

    // The directory 'd_response_cache' should exist so is_available() should be
    // true.
    void ctor_test_2() {
		DBG(cerr << "ctor_test_2() - BEGIN" << endl);

		string cacheDir = d_response_cache;
		string prefix = "rc";
		long size = 1000;
    	cache =  BESDapResponseCache::get_instance(cacheDir, prefix, size);
		DBG(cerr << "ctor_test_1() - retrieved BESDapResponseCache instance: "<< cache << endl);

    	CPPUNIT_ASSERT(cache);

		DBG(cerr << "ctor_test_2() - END" << endl);
    }

    // Because setup() and teardown() clean out the cache directory, there should
    // never be a cached item; calling read_cached_dataset() should return a
    // valid DDS with data and store a copy in the cache.
	void cache_a_response()
	{
		DBG(cerr << "cache_a_response() - BEGIN" << endl);
		cache = BESDapResponseCache::get_instance(d_response_cache, "rc", 1000);

		DBG(cerr << "cache_a_response() - Retrieved BESDapResponseCache object: " << cache << endl);

		string token;
		try {
            CPPUNIT_ASSERT(test_dds);

			DBG(cerr << "cache_a_response() - caching a dataset... " << endl);
			token = cache->cache_dataset(&test_dds, "", &eval);
			DBG(cerr << "cache_a_response() - unlocking and closing cache... token: " << token << endl);
			cache->unlock_and_close(token);

            CPPUNIT_ASSERT(test_dds);
		}
		catch (Error &e) {
			CPPUNIT_FAIL(e.get_error_message());
		}
		DBG(cerr << "cache_a_response() - END" << endl);

    }

	// The first call reads values into the DDS, stores a copy in the cache and
	// returns the DDS. The second call reads the value from the cache.
	//
	// Use load_from_cache() (Private interface) to read the data.
	void cache_and_read_a_response()
	{
		DBG(cerr << "cache_and_read_a_response() - BEGIN" << endl);

		cache = BESDapResponseCache::get_instance(d_response_cache, "rc", 1000);
		string token;
		try {
		    // This code is here to load the DataDDX response into the cache if it is not
		    // there already. If it is there, it reads it from the cache.
		    token = cache->cache_dataset(&test_dds, "", &eval);

			DBG(cerr << "Cached response token: " << token << endl);

			CPPUNIT_ASSERT(test_dds);
			int var_count = test_dds->var_end() - test_dds->var_begin();
			CPPUNIT_ASSERT(var_count == 9);

            bool ret = cache->load_from_cache("test.05", test_dds->filename()+"#", token, &test_dds);

            // True if it worked
            CPPUNIT_ASSERT(ret);

            // Better not be null!
			CPPUNIT_ASSERT(test_dds);

			// There are nine variables in test.05.ddx
			var_count = test_dds->var_end() - test_dds->var_begin() ;
	        DBG(cerr << "cache_and_read_a_response() - var_count: "<< var_count << endl);
			CPPUNIT_ASSERT(var_count == 9);

			ostringstream oss;
			DDS::Vars_iter i = test_dds->var_begin();
			while (i != test_dds->var_end()) {
				DBG(cerr << "Variable " << (*i)->name() << endl);
				// this will incrementally add the string rep of values to 'oss'
				(*i)->print_val(oss, "", false /*print declaration */);
				DBG(cerr << "Value " << oss.str() << endl);
				++i;
			}

			// In this regex the value of <number> in the DAP2 Str variable (Silly test string: <number>)
			// is a any single digit. The *Test classes implement a counter and return strings where
			// <number> is 1, 2, ..., and running several of the tests here in a row will get a range of
			// values for <number>.
			Regex regex("2551234567894026531840320006400099.99999.999\"Silly test string: [0-9]\"\"http://dcz.gso.uri.edu/avhrr-archive/archive.html\"");
			CPPUNIT_ASSERT(re_match(regex, oss.str()));
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
		DBG(cerr << "cache_and_read_a_response2() - BEGIN" << endl);

		cache = BESDapResponseCache::get_instance(d_response_cache, "rc", 1000);
		string token;
		try {
			// This loads a DDS in the cache and returns it.
			token = cache->cache_dataset(&test_dds, "", &eval);

			DBG(cerr << "Cached response token: " << token << endl);
			CPPUNIT_ASSERT(test_dds);

			// This reads the dataset from the cache, but unlike the previous test,
			// does so using the public interface.
			token = cache->cache_dataset(&test_dds, "", &eval);

			CPPUNIT_ASSERT(test_dds);
			// There are nine variables in test.05.ddx
			CPPUNIT_ASSERT(test_dds->var_end() - test_dds->var_begin() == 9);

			ostringstream oss;
			DDS::Vars_iter i = test_dds->var_begin();
			while (i != test_dds->var_end()) {
				DBG(cerr << "Variable " << (*i)->name() << endl);
				// this will incrementally add the string rep of values to 'oss'
				(*i)->print_val(oss, "", false /*print declaration */);
				DBG(cerr << "Value " << oss.str() << endl);
				++i;
			}

			Regex regex("2551234567894026531840320006400099.99999.999\"Silly test string: [0-9]\"\"http://dcz.gso.uri.edu/avhrr-archive/archive.html\"");
			CPPUNIT_ASSERT(re_match(regex, oss.str()));
		}
		catch (Error &e) {
			CPPUNIT_FAIL(e.get_error_message());
		}

		DBG(cerr << "cache_and_read_a_response2() - END" << endl);
    }

	// Test caching a response where a CE is applied to the DDS. The CE here is 'b,u'
    //
    // Use the public interface to read the data (cache_dataset()), but this time
    // include a constraint.
	void cache_and_read_a_response3()
	{
		DBG(cerr << "cache_and_read_a_response3() - BEGIN" << endl);

		cache = BESDapResponseCache::get_instance(d_response_cache, "rc", 1000);
		string token;
		try {
			// This loads a DDS in the cache and returns it.
			token = cache->cache_dataset(&test_dds, "b,u", &eval);

			DBG(cerr << "Cached response token: " << token << endl);
			CPPUNIT_ASSERT(test_dds);
			ostringstream oss;
			DDS::Vars_iter i = test_dds->var_begin();
			while (i != test_dds->var_end()) {
				DBG(cerr << "Variable " << (*i)->name() << endl);
				if ((*i)->send_p()) {
					(*i)->print_val(oss, "", false /*print declaration */);
					DBG(cerr << "Value " << oss.str() << endl);
				}
				++i;
			}

			CPPUNIT_ASSERT(oss.str() == "255\"http://dcz.gso.uri.edu/avhrr-archive/archive.html\"");
			oss.str("");

			token = cache->cache_dataset(&test_dds, "b,u", &eval);
			cache->unlock_and_close(token);

			CPPUNIT_ASSERT(test_dds);
			// There are nine variables in test.05.ddx but two in the CE used here and
			// the response cached was constrained.
			CPPUNIT_ASSERT(test_dds->var_end() - test_dds->var_begin() == 2);

			i = test_dds->var_begin();
			while (i != test_dds->var_end()) {
				DBG(cerr << "Variable " << (*i)->name() << endl);
				if ((*i)->send_p()) {
					(*i)->print_val(oss, "", false /*print declaration */);
					DBG(cerr << "Value " << oss.str() << endl);
				}
				++i;
			}

			CPPUNIT_ASSERT(oss.str() == "255\"http://dcz.gso.uri.edu/avhrr-archive/archive.html\"");

		}
		catch (Error &e) {
			CPPUNIT_FAIL(e.get_error_message());
		}

		DBG(cerr << "cache_and_read_a_response3() - END" << endl);
    }

	CPPUNIT_TEST_SUITE( ResponseCacheTest );

    CPPUNIT_TEST(ctor_test_1);
    CPPUNIT_TEST(ctor_test_2);
    CPPUNIT_TEST(cache_a_response);
    CPPUNIT_TEST(cache_and_read_a_response);
    CPPUNIT_TEST(cache_and_read_a_response2);
    CPPUNIT_TEST(cache_and_read_a_response3);

    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(ResponseCacheTest);

int main(int argc, char*argv[]) {
    CppUnit::TextTestRunner runner;
    runner.addTest(CppUnit::TestFactoryRegistry::getRegistry().makeTest());

    GetOpt getopt(argc, argv, "dbk");
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
        case 'k':   // -k turns off cleaning the response_cache dir
            clean = false;
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
            test = string("ResponseCacheTest::") + argv[i++];

            wasSuccessful = wasSuccessful && runner.run(test);
        }
    }

    return wasSuccessful ? 0 : 1;
}
