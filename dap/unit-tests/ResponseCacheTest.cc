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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.

#include "config.h"

#include <dirent.h>

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
#include "BESDapResponseBuilder.h"
#include "testFile.h"
#include "test_config.h"

using namespace CppUnit;
using namespace std;
using namespace libdap;

int test_variable_sleep_interval = 0;

static bool debug = false;

#undef DBG
#define DBG(x) do { if (debug) (x); } while(false);

using namespace libdap;

class ResponseCacheTest: public TestFixture {
private:
	TestTypeFactory ttf;
	DDS *test_05_dds;
	//DDS test_06_dds;
	DDXParser dp;
	ConstraintEvaluator eval;
	BESDapResponseBuilder rb;

    string d_response_cache;
    BESDapResponseCache *cache;

public:
    ResponseCacheTest() : test_05_dds(0), /*test_06_dds(&ttf), */dp(&ttf),
		d_response_cache(string(TEST_SRC_DIR) + "/response_cache") {
    }

    ~ResponseCacheTest() {
    }

    void clean_cache(const string &directory, const string &prefix) {
        DIR *dip = opendir(directory.c_str());
        if (!dip)
            throw InternalErr(__FILE__, __LINE__, "Unable to open cache directory " + directory);

        struct dirent *dit;
        // go through the cache directory and collect all of the files that
        // start with the matching prefix
        while ((dit = readdir(dip)) != NULL) {
            string dirEntry = dit->d_name;
            if (dirEntry.compare(0, prefix.length(), prefix) == 0) {
            	unlink(string(directory + "/" + dit->d_name).c_str());
            }
        }

        closedir(dip);
    }

    void setUp() {
    	string cid;
    	test_05_dds = new DDS(&ttf);
    	dp.intern((string)TEST_SRC_DIR + "/input-files/test.05.ddx", test_05_dds, cid);
    	// cid == http://dods.coas.oregonstate.edu:8080/dods/dts/test.01.blob
    	DBG(cerr << "DDS Name: " << test_05_dds->get_dataset_name() << endl);
    	DBG(cerr << "Intern CID: " << cid << endl);
    }

    void tearDown() {
		clean_cache(d_response_cache, "rc");
		delete test_05_dds;
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

    	cache = new BESDapResponseCache(string(TEST_SRC_DIR) + "/never", "rc", 1000);
    	CPPUNIT_ASSERT(!cache->is_available());
    }

    // The directory 'response_cache' should exist so is_available() should be
    // true.
    void ctor_test_2() {
    	//cache = new ResponseCache(TEST_SRC_DIR + "response_cache", "rc", 1000);
    	cache = new BESDapResponseCache(d_response_cache, "rc", 1000);
    	CPPUNIT_ASSERT(cache->is_available());
    }

    // Because setup() and teardown() clean out the cache directory, there should
    // never be a cached item; calling read_cached_dataset() should return a
    // valid DDS with data and store a copy in the cache.
	void cache_a_response()
	{
		//cache = new ResponseCache(TEST_SRC_DIR + "response_cache", "rc", 1000);
		cache = new BESDapResponseCache(d_response_cache, "rc", 1000);
		string token;
		try {
			// TODO Could stat the cache file to make sure it's not already there.
			DDS *cache_dds = cache->read_cached_dataset(*test_05_dds, "", &rb, &eval, token);
			cache->unlock_and_close(token);

			DBG(cerr << "Cached response token: " << token << endl);
			CPPUNIT_ASSERT(cache_dds);
			CPPUNIT_ASSERT(token == d_response_cache + "/rc#SimpleTypes#");
			// TODO Stat the cache file to check it's size
			delete cache_dds;
		}
		catch (Error &e) {
			CPPUNIT_FAIL(e.get_error_message());
		}
    }

	// The first call reads values into the DDS, stores a copy in the cache and
	// returns the DDS. The second call reads the value from the cache.
	void cache_and_read_a_response()
	{
		//cache = new ResponseCache(TEST_SRC_DIR + "response_cache", "rc", 1000);
		cache = new BESDapResponseCache(d_response_cache, "rc", 1000);
		string token;
		try {
			DDS *cache_dds = cache->read_cached_dataset(*test_05_dds, "", &rb, &eval, token);
			cache->unlock_and_close(token);

			DBG(cerr << "Cached response token: " << token << endl);
			CPPUNIT_ASSERT(cache_dds);
			CPPUNIT_ASSERT(token == d_response_cache + "/rc#SimpleTypes#");
			delete cache_dds; cache_dds = 0;

			// DDS *get_cached_data_ddx(const string &cache_file_name, BaseTypeFactory *factory, const string &dataset)
			// Force read from the cache file
			cache_dds = cache->get_cached_data_ddx(token, &ttf, "test.05");
			// The code cannot unlock the file because get_cached_data_ddx()
			// does not lock the cached item.
			//cache->unlock_and_close(token);

			CPPUNIT_ASSERT(cache_dds);
			CPPUNIT_ASSERT(token == d_response_cache + "/rc#SimpleTypes#");
			// There are nine variables in test.05.ddx
			CPPUNIT_ASSERT(cache_dds->var_end() - cache_dds->var_begin() == 9);

			ostringstream oss;
			DDS::Vars_iter i = cache_dds->var_begin();
			while (i != cache_dds->var_end()) {
				DBG(cerr << "Variable " << (*i)->name() << endl);
				// this will incrementally add thr string rep of values to 'oss'
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
			delete cache_dds; cache_dds = 0;
		}
		catch (Error &e) {
			CPPUNIT_FAIL(e.get_error_message());
		}
    }

	// The first call reads values into the DDS, stores a copy in the cache and
	// returns the DDS. The second call reads the value from the cache.
	void cache_and_read_a_response2()
	{
		//cache = new ResponseCache(TEST_SRC_DIR + "response_cache", "rc", 1000);
		cache = new BESDapResponseCache(d_response_cache, "rc", 1000);
		string token;
		try {
			// This loads a DDS in the cache and returns it.
			DDS *cache_dds = cache->read_cached_dataset(*test_05_dds, "", &rb, &eval, token);
			cache->unlock_and_close(token);

			DBG(cerr << "Cached response token: " << token << endl);
			CPPUNIT_ASSERT(cache_dds);
			CPPUNIT_ASSERT(token == d_response_cache + "/rc#SimpleTypes#");
			delete cache_dds; cache_dds = 0;

			// This reads the dataset from the cache, but unlike the previous test,
			// does so using the public interface.
			cache_dds = cache->read_cached_dataset(*test_05_dds, "", &rb, &eval, token);
			cache->unlock_and_close(token);

			CPPUNIT_ASSERT(cache_dds);
			CPPUNIT_ASSERT(token == d_response_cache + "/rc#SimpleTypes#");
			// There are nine variables in test.05.ddx
			CPPUNIT_ASSERT(cache_dds->var_end() - cache_dds->var_begin() == 9);

			ostringstream oss;
			DDS::Vars_iter i = cache_dds->var_begin();
			while (i != cache_dds->var_end()) {
				DBG(cerr << "Variable " << (*i)->name() << endl);
				// this will incrementally add the string rep of values to 'oss'
				(*i)->print_val(oss, "", false /*print declaration */);
				DBG(cerr << "Value " << oss.str() << endl);
				++i;
			}

			Regex regex("2551234567894026531840320006400099.99999.999\"Silly test string: [0-9]\"\"http://dcz.gso.uri.edu/avhrr-archive/archive.html\"");
			CPPUNIT_ASSERT(re_match(regex, oss.str()));
			delete cache_dds; cache_dds = 0;
		}
		catch (Error &e) {
			CPPUNIT_FAIL(e.get_error_message());
		}
    }

	// Test caching a response where a CE is applied to the DDS. The CE here is 'b,u'
	void cache_and_read_a_response3()
	{
		//cache = new ResponseCache(TEST_SRC_DIR + "response_cache", "rc", 1000);
		cache = new BESDapResponseCache(d_response_cache, "rc", 1000);
		string token;
		try {
			// This loads a DDS in the cache and returns it.
			DDS *cache_dds = cache->read_cached_dataset(*test_05_dds, "b,u", &rb, &eval, token);
			cache->unlock_and_close(token);

			DBG(cerr << "Cached response token: " << token << endl);
			CPPUNIT_ASSERT(cache_dds);
			CPPUNIT_ASSERT(token == d_response_cache + "/rc#SimpleTypes#b#u");
			ostringstream oss;
			DDS::Vars_iter i = cache_dds->var_begin();
			while (i != cache_dds->var_end()) {
				DBG(cerr << "Variable " << (*i)->name() << endl);
				if ((*i)->send_p()) {
					(*i)->print_val(oss, "", false /*print declaration */);
					DBG(cerr << "Value " << oss.str() << endl);
				}
				++i;
			}

			CPPUNIT_ASSERT(oss.str() == "255\"http://dcz.gso.uri.edu/avhrr-archive/archive.html\"");
			delete cache_dds; cache_dds = 0;
			oss.str("");

			cache_dds = cache->read_cached_dataset(*test_05_dds, "b,u", &rb, &eval, token);
			cache->unlock_and_close(token);

			CPPUNIT_ASSERT(cache_dds);
			CPPUNIT_ASSERT(token == d_response_cache + "/rc#SimpleTypes#b#u");
			// There are nine variables in test.05.ddx but two in the CE used here and
			// the response cached was constrained.
			CPPUNIT_ASSERT(cache_dds->var_end() - cache_dds->var_begin() == 2);

			i = cache_dds->var_begin();
			while (i != cache_dds->var_end()) {
				DBG(cerr << "Variable " << (*i)->name() << endl);
				if ((*i)->send_p()) {
					(*i)->print_val(oss, "", false /*print declaration */);
					DBG(cerr << "Value " << oss.str() << endl);
				}
				++i;
			}

			CPPUNIT_ASSERT(oss.str() == "255\"http://dcz.gso.uri.edu/avhrr-archive/archive.html\"");
			delete cache_dds; cache_dds = 0;
		}
		catch (Error &e) {
			CPPUNIT_FAIL(e.get_error_message());
		}

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

    GetOpt getopt(argc, argv, "d");
    char option_char;
    while ((option_char = getopt()) != EOF)
        switch (option_char) {
        case 'd':
            debug = 1;  // debug is a static global
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
