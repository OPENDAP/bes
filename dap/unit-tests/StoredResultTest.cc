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

#include "TheBESKeys.h"
#include "BESStoredDapResultCache.h"
#include "BESDapResponseBuilder.h"
#include "BESDebug.h"


#include "testFile.h"
#include "test_config.h"

#define BES_DATA_ROOT "BES.Data.RootDirectory"
#define BES_CATALOG_ROOT "BES.Catalog.catalog.RootDirectory"




using namespace CppUnit;
using namespace std;
using namespace libdap;

int test_variable_sleep_interval = 0;

static bool debug = false;

#undef DBG
#define DBG(x) do { if (debug) (x); } while(false);

using namespace libdap;

class StoredResultTest: public TestFixture {
private:
	TestTypeFactory ttf;
	DDS *test_05_dds;
	//DDS test_06_dds;
	DDXParser dp;
	ConstraintEvaluator eval;
	BESDapResponseBuilder rb;

    string d_data_root_dir;
    string d_stored_result_subdir;
	string d_response_cache;
    BESStoredDapResultCache *cache;

public:
    StoredResultTest() : test_05_dds(0),
    					 dp(&ttf),
    					 d_data_root_dir(string(TEST_SRC_DIR)),
    					 d_stored_result_subdir("/response_cache"),
    					 d_response_cache(string(TEST_SRC_DIR)+"/response_cache"),
    					 cache(0) {
    }

    ~StoredResultTest() {
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
		DBG(cerr << "setUp() - BEGIN" << endl);
		DBG(BESDebug::SetUp("cerr,all"));

    	string cid;
    	test_05_dds = new DDS(&ttf);
    	dp.intern((string)TEST_SRC_DIR + "/input-files/test.05.ddx", test_05_dds, cid);

    	// for these tests, set the filename to the dataset_name. ...keeps the cache names short
    	test_05_dds->filename(test_05_dds->get_dataset_name());

    	// cid == http://dods.coas.oregonstate.edu:8080/dods/dts/test.01.blob
    	DBG(cerr << "DDS Name: " << test_05_dds->get_dataset_name() << endl);
    	DBG(cerr << "Intern CID: " << cid << endl);


		DBG(cerr << "setUp() - END" << endl);
    }

    void tearDown() {
		DBG(cerr << "tearDown() - BEGIN" << endl);

		clean_cache(d_response_cache, "result_");
		delete test_05_dds;
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




    // The directory 'd_response_cache' should exist so get_instance() should return a valid object (non-null)
    void path_construction_test() {
		DBG(cerr << "**** path_construction_test() - BEGIN" << endl);

		string newPath = BESStoredDapResultCache::assemblePath("test/path","response/subdir");

    	CPPUNIT_ASSERT(newPath == "test/path/response/subdir");

	    newPath = BESStoredDapResultCache::assemblePath("test/path","/response/subdir");

    	CPPUNIT_ASSERT(newPath == "test/path/response/subdir");

		newPath = BESStoredDapResultCache::assemblePath("test/path/","/response/subdir");

    	CPPUNIT_ASSERT(newPath == "test/path/response/subdir");

		newPath = BESStoredDapResultCache::assemblePath("test/path//","/response/subdir");

    	CPPUNIT_ASSERT(newPath == "test/path/response/subdir");

		newPath = BESStoredDapResultCache::assemblePath("test/path//","//response/subdir");

    	CPPUNIT_ASSERT(newPath == "test/path/response/subdir");

		DBG(cerr << "**** path_construction_test() - END" << endl);

		cache->delete_instance();

    }

    // The directory 'never' does not exist; the cache won't be initialized,
    // so get_instance() should return a null;
    void ctor_test_1() {
		DBG(cerr << "**** ctor_test_1() - BEGIN" << endl);

		string cacheDir = string(TEST_SRC_DIR) + "/never";
		string prefix = "result_";
		long size = 1000;

    	cache =  BESStoredDapResultCache::get_instance(cacheDir, d_stored_result_subdir, prefix, size);
		DBG(cerr << "ctor_test_1() - retrieved BESStoredResultCache instance: "<< endl);
		if(cache){
			DBG(cerr << *cache << endl);
		}
		else {
			DBG(cerr << "NULL" << endl);
		}

    	CPPUNIT_ASSERT(!cache);

    	// We DO NOT delete the instance because there isn't one, RIGHT?
    	//cache->delete_instance();

		DBG(cerr << "**** ctor_test_1() - END" << endl);

    }

    // The directory 'd_response_cache' should exist so get_instance() should return a valid object (non-null)
    void ctor_test_2() {
		DBG(cerr << "**** ctor_test_2() - BEGIN" << endl);

		string cacheDir = d_response_cache;
		string prefix = "result_";
		long size = 1000;
    	cache =  BESStoredDapResultCache::get_instance(d_data_root_dir, d_stored_result_subdir, prefix, size);
		DBG(cerr << "ctor_test_1() - retrieved BESStoredResultCache instance: "<< endl);
		if(cache){
			DBG(cerr << *cache << endl);
		}
		else {
			DBG(cerr << "NULL" << endl);
		}

    	CPPUNIT_ASSERT(cache);
		DBG(cerr << "**** ctor_test_2() - END" << endl);

		cache->delete_instance();

    }


    // Because setup() and teardown() clean out the cache directory, there should
    // never be a cached item; calling read_cached_dataset() should return a
    // valid DDS with data and store a copy in the cache.
	void cache_a_response()
	{
		DBG(cerr << "**** cache_a_response() - BEGIN" << endl);

		cache = BESStoredDapResultCache::get_instance(d_data_root_dir, d_stored_result_subdir, "result_", 1000);

		DBG(cerr << "cache_a_response() - Got the instance of BESStoredResultCache object: " );
		if(cache){
			DBG(cerr << *cache << endl);
		}
		else {
			DBG(cerr << "NULL" << endl);
		}

		string stored_result_local_id;
		try {
			// TODO Could stat the cache file to make sure it's not already there.
			DBG(cerr << "cache_a_response() - caching a dataset... " << endl);
			stored_result_local_id = cache->store_dap2_result(*test_05_dds, "", &rb, &eval);

			DBG(cerr << "Cached response id: " << stored_result_local_id << endl);
			CPPUNIT_ASSERT(stored_result_local_id == d_stored_result_subdir + "/result_17566926586167953453");
			// TODO Stat the cache file to check it's size
			cache->delete_instance();
		}
		catch (Error &e) {
			CPPUNIT_FAIL(e.get_error_message());
		}
		DBG(cerr << "**** cache_a_response() - END" << endl);

    }

	// The first call reads values into the DDS, stores a copy in the cache and
	// returns the DDS. The second call reads the value from the cache.
	void cache_and_read_a_response()
	{
		DBG(cerr << "**** cache_and_read_a_response() - BEGIN" << endl);

		cache = BESStoredDapResultCache::get_instance(d_data_root_dir, d_stored_result_subdir, "result_", 1000);
		string stored_result_local_id;
		try {
			DBG(cerr << "cache_and_read_a_response() - Storing DAP2 dataset." << endl);
			stored_result_local_id = cache->store_dap2_result(*test_05_dds, "", &rb, &eval);
			DBG(cerr << "Cached response id: " << stored_result_local_id << endl);


			// DDS *get_cached_data_ddx(const string &cache_file_name, BaseTypeFactory *factory, const string &dataset)
			// Force read from the cache file
			DBG(cerr << "cache_and_read_a_response() - Reading stored DAP2 dataset." << endl);
			DDS *cache_dds = cache->get_cached_data_ddx(d_data_root_dir+stored_result_local_id, &ttf, "test.05");
			// The code cannot unlock the file because get_cached_data_ddx()
			// does not lock the cached item.
			//cache->unlock_and_close(token);
			DBG(cerr << "cache_and_read_a_response() - Stored DAP2 dataset read." << endl);

			CPPUNIT_ASSERT(cache_dds);
			CPPUNIT_ASSERT(stored_result_local_id == d_stored_result_subdir + "/result_17566926586167953453");
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
	    	// cache->delete_instance();
			cache->delete_instance();

		}
		catch (Error &e) {
			CPPUNIT_FAIL(e.get_error_message());
		}
		DBG(cerr << "**** cache_and_read_a_response() - END" << endl);

    }


	// The first call reads values into the DDS, stores a copy in the cache and
	// returns the DDS. The second call reads the value from the cache.
	void cache_and_read_a_response2()
	{
		DBG(cerr << "**** cache_and_read_a_response2() - BEGIN" << endl);

		cache = BESStoredDapResultCache::get_instance(d_data_root_dir, d_stored_result_subdir, "result_", 1000);
		string stored_result_local_id;
		try {
			DBG(cerr << "cache_and_read_a_response2() - Storing DAP2 dataset." << endl);
			stored_result_local_id = cache->store_dap2_result(*test_05_dds, "", &rb, &eval);
			DBG(cerr << "cache_and_read_a_response2() - Cached response local id: " << stored_result_local_id << endl);
			CPPUNIT_ASSERT(stored_result_local_id == d_stored_result_subdir + "/result_17566926586167953453");


			// DDS *get_cached_data_ddx(const string &cache_file_name, BaseTypeFactory *factory, const string &dataset)
			// Force read from the cache file
			DBG(cerr << "cache_and_read_a_response2() - Reading stored DAP2 dataset." << endl);
			DDS *cache_dds = cache->get_cached_data_ddx(d_data_root_dir+stored_result_local_id, &ttf, "test.05");
			// The code cannot unlock the file because get_cached_data_ddx()
			// does not lock the cached item.
			//cache->unlock_and_close(token);
			DBG(cerr << "cache_and_read_a_response2() - Stored DAP2 dataset read." << endl);

			CPPUNIT_ASSERT(cache_dds);
			CPPUNIT_ASSERT(stored_result_local_id == d_stored_result_subdir + "/result_17566926586167953453");
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



			DBG(cerr << "cache_and_read_a_response2() - Storing the SAME DAP2 dataset." << endl);
			stored_result_local_id = cache->store_dap2_result(*test_05_dds, "", &rb, &eval);
			DBG(cerr << "cache_and_read_a_response2() - Cached response local id: " << stored_result_local_id << endl);
			CPPUNIT_ASSERT(stored_result_local_id == d_stored_result_subdir + "/result_17566926586167953453");


			cache->delete_instance();

		}
		catch (Error &e) {
			CPPUNIT_FAIL(e.get_error_message());
		}
		DBG(cerr << "**** cache_and_read_a_response2() - END" << endl);

    }



	void configureFromBesKeysAndStoreResult()
	{
		DBG(cerr << "**** configureFromBesKeysAndStoreResult() - BEGIN" << endl);

    	DBG(cerr << "Configuring BES Keys."<< endl);

        TheBESKeys::ConfigFile = (string)TEST_SRC_DIR + "/input-files/test.keys";
        TheBESKeys::TheKeys()->set_key(BES_CATALOG_ROOT,  (string)TEST_SRC_DIR);
        TheBESKeys::TheKeys()->set_key( BESStoredDapResultCache::SUBDIR_KEY,  d_stored_result_subdir);
        TheBESKeys::TheKeys()->set_key( BESStoredDapResultCache::PREFIX_KEY,  "my_result_");
        TheBESKeys::TheKeys()->set_key( BESStoredDapResultCache::SIZE_KEY,    "1100");

		cache = BESStoredDapResultCache::get_instance();

		CPPUNIT_ASSERT(cache);

		string stored_result_local_id;
		try {
			stored_result_local_id = cache->store_dap2_result(*test_05_dds, "", &rb, &eval);
			//cache->unlock_and_close(stored_result_local_id);

			DBG(cerr << "Cached response id: " << stored_result_local_id << endl);
			CPPUNIT_ASSERT("/"+stored_result_local_id == d_stored_result_subdir + "/my_result_17566926586167953453");

			// DDS *get_cached_data_ddx(const string &cache_file_name, BaseTypeFactory *factory, const string &dataset)
			// Force read from the cache file
			DDS *cache_dds = cache->get_cached_data_ddx(d_data_root_dir+"/"+stored_result_local_id, &ttf, "test.05");
			// The code cannot unlock the file because get_cached_data_ddx()
			// does not lock the cached item.
			// cache->unlock_and_close(stored_result_local_id);

			CPPUNIT_ASSERT(cache_dds);
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

			cache->delete_instance();

		}
		catch (Error &e) {
			CPPUNIT_FAIL(e.get_error_message());
		}
		DBG(cerr << "**** cache_and_read_a_response3() - END" << endl);


    }
    CPPUNIT_TEST_SUITE( StoredResultTest );

    CPPUNIT_TEST(path_construction_test);
    CPPUNIT_TEST(ctor_test_1);
    CPPUNIT_TEST(ctor_test_2);
    CPPUNIT_TEST(cache_a_response);
    CPPUNIT_TEST(cache_and_read_a_response);
    CPPUNIT_TEST(cache_and_read_a_response2);
    CPPUNIT_TEST(configureFromBesKeysAndStoreResult);

    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(StoredResultTest);

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
            test = string("StoredResultTest::") + argv[i++];

            wasSuccessful = wasSuccessful && runner.run(test);
        }
    }

    return wasSuccessful ? 0 : 1;
}
