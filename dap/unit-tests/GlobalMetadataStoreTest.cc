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
#include <fcntl.h>
#include <sys/stat.h>

#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>
#include <unistd.h>
#include <regex>

#include <libdap/Array.h>
#include <libdap/Byte.h>
#include <libdap/DAS.h>
#include <libdap/DDS.h>
#include <libdap/DDXParserSAX2.h>
#include <libdap/DMR.h>
#include <libdap/D4ParserSax2.h>

#include <libdap/util.h>
#include <libdap/debug.h>

#include <libdap/BaseTypeFactory.h>
#include <libdap/D4BaseTypeFactory.h>

#include "TheBESKeys.h"
#include "BESContextManager.h"
#include "BESFileContainer.h"
#include "BESRequestHandlerList.h"
#include "BESRequestHandler.h"
#include "BESDebug.h"
#include "BESInternalError.h"
#include "BESNotFoundError.h"
#include "BESRegex.h"
#include "GlobalMetadataStore.h"

#include "test_utils.h"
#include "test_config.h"

static bool debug = false;
static bool bes_debug = false;
static bool clean = true;

#undef DBG
#define DBG(x) do { if (debug) (x); } while(false);

#define DEBUG_KEY "metadata_store,cache"

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

    DMR *d_test_dmr;
    D4BaseTypeFactory d_d4f;

    string d_mds_dir;
    GlobalMetadataStore *d_mds;

    // BESRequestHandler *besRH = BESRequestHandlerList::TheList()->find_handler(type);
    BESRequestHandler d_handler;

    string tmp;

    /**
     * This is like SetUp() but is run selectively by tests. It won't work
     * for all of the tests (like test_ctor_1, where get_instance() should not
     * return).
     */
    void init_dds_and_mds()
    {
        try {
            // Stock code to get the d_test_dds and d_mds objects used by many
            // of the tests.
            d_mds = GlobalMetadataStore::get_instance(d_mds_dir, c_mds_prefix, 1000);
            DBG(cerr << "Retrieved GlobalMetadataStore object: " << d_mds << endl);

            // Get a DDS to cache.
            string file_name = string(TEST_BUILD_DIR).append("/input-files/sequence_1.dds");

            // Get a DDS to cache.
            d_test_dds = new DDS(&d_btf);
            DDXParser dp(&d_btf);
            string cid; // This is an unused value-result parameter. jhrg 5/10/16
            dp.intern(string(TEST_SRC_DIR).append("/input-files/test.05.ddx"), d_test_dds, cid);

            // for these tests, set the filename to the dataset_name. ...keeps the cache names short
            d_test_dds->filename(d_test_dds->get_dataset_name());
            DBG(cerr << "DDS Name: " << d_test_dds->get_dataset_name() << endl);
            CPPUNIT_ASSERT(d_test_dds);
        }
        catch (BESError &e) {
            CPPUNIT_FAIL(e.get_message());
        }
        catch (Error &e) {
            CPPUNIT_FAIL(e.get_error_message());
        }
        catch (std::exception &e) {
            CPPUNIT_FAIL(e.what());
        }
    }

    /**
     * SetUp()-like method to build a DMR for testing.
     */
    void init_dmr_and_mds()
    {
        try {
            // Stock code to get the d_test_dds and d_mds objects used by many
            // of the tests.
            d_mds = GlobalMetadataStore::get_instance(d_mds_dir, c_mds_prefix, 1000);
            DBG(cerr << "Retrieved GlobalMetadataStore object: " << d_mds << endl);

            // Get a DMR to cache.
            string file_name = string(TEST_SRC_DIR).append("/input-files/test_01.dmr");

            d_test_dmr = new DMR(&d_d4f);
            D4ParserSax2 dp;
            DBG(cerr << "DMR file to be parsed: " << file_name << endl);
            fstream in(file_name.c_str(), ios::in|ios::binary);
            dp.intern(in, d_test_dmr);

            DBG(cerr << "DMR Name: " << d_test_dmr->name() << endl);
            CPPUNIT_ASSERT(d_test_dmr);
        }
        catch (BESError &e) {
            CPPUNIT_FAIL(e.get_message());
        }
        catch (Error &e) {
            CPPUNIT_FAIL(e.get_error_message());
        }
        catch (std::exception &e) {
            CPPUNIT_FAIL(e.what());
        }
    }

#if 0
    void init_dmrpp_and_mds()
    {
        try {
            // Stock code to get the d_test_dds and d_mds objects used by many
            // of the tests.
            d_mds = GlobalMetadataStore::get_instance(d_mds_dir, c_mds_prefix, 1000);
            DBG(cerr << "Retrieved GlobalMetadataStore object: " << d_mds << endl);

            // Get a DMRpp to cache.
            string file_name = string(TEST_SRC_DIR).append("/input-files/chunked_fourD.h5.dmrpp");

            DmrppTypeFactory dmrpp_factory;
            d_test_dmr = new DMRpp(&dmrpp_factory);
            DmrppParserSax2 dp;
            DBG(cerr << "DMRpp file to be parsed: " << file_name << endl);
            fstream in(file_name.c_str(), ios::in|ios::binary);
            dp.intern(in, d_test_dmr);

            DBG(cerr << "DMRpp Name: " << d_test_dmr->name() << endl);
            CPPUNIT_ASSERT(d_test_dmr);
        }
        catch (BESError &e) {
            CPPUNIT_FAIL(e.get_message());
        }
        catch (Error &e) {
            CPPUNIT_FAIL(e.get_error_message());
        }
        catch (std::exception &e) {
            CPPUNIT_FAIL(e.what());
        }
    }
#endif

public:
    GlobalMetadataStoreTest() :
        d_test_dds(0), d_test_dmr(0), d_mds_dir(string(TEST_BUILD_DIR).append(c_mds_name)), d_mds(0),
	d_handler("test_handler"), tmp(string(TEST_BUILD_DIR) + "/tmp")
    {
    	BESRequestHandlerList::TheList()->add_handler("test_handler", &d_handler);
    	mkdir(tmp.c_str(),0755);
    }

    ~GlobalMetadataStoreTest()
    {
    	rmdir(tmp.c_str());
    }

    void setUp()
    {
        DBG(cerr << __func__ << " - BEGIN" << endl);

        if (bes_debug) BESDebug::SetUp(string("cerr,").append(DEBUG_KEY));

        if (clean) clean_cache_dir(c_mds_name);

        // Contains BES Log parameters but not cache names
        TheBESKeys::ConfigFile = string(TEST_BUILD_DIR).append("/bes.conf");
        BESContextManager::TheManager()->set_context("xml:base", "http://localhost/");

        DBG(cerr << __func__ << " - END" << endl);
    }

    void tearDown()
    {
        DBG(cerr << __func__ << " - BEGIN" << endl);

        delete d_test_dds; d_test_dds = 0;

        delete d_test_dmr; d_test_dmr = 0;

        d_mds->delete_instance();

        if (clean) clean_cache_dir(d_mds_dir);

        DBG(cerr << __func__ << " - END" << endl);
    }

    void ctor_test_1()
    {
        DBG(cerr << __func__ << " - BEGIN" << endl);

        try {
            // The call to get_instance should fail since the directory is named,
            // but does not exist and cannot be made.
        		// Check to see if the test is being run by the super user or built in root.
        	    if (access("/", W_OK) != 0) {
				d_mds = GlobalMetadataStore::get_instance("/new", c_mds_prefix, 1000);
				DBG(cerr << "retrieved GlobalMetadataStore instance: " << d_mds << endl);
				CPPUNIT_FAIL("get_instance() Should not return when the non-existent directory cannot be created");
         	}
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
            CPPUNIT_ASSERT(!d_mds->is_unlimited());

            d_mds->update_and_purge("no_name"); //cheesy test - use -b to read BES debug info
        }
        catch (BESError &e) {
            CPPUNIT_FAIL("Caught exception: " + e.get_message());
        }

        DBG(cerr << __func__ << " - END" << endl);
    }

    void ctor_test_3()
    {
        DBG(cerr << __func__ << " - BEGIN" << endl);

        try {
            // This one should work and will make the directory
            d_mds = GlobalMetadataStore::get_instance(d_mds_dir, c_mds_prefix, 0);
            DBG(cerr << "retrieved GlobalMetadataStore instance: " << d_mds << endl);
            CPPUNIT_ASSERT(d_mds);
            CPPUNIT_ASSERT(d_mds->is_unlimited());

            d_mds->update_and_purge("no_name");
        }
        catch (BESError &e) {
            CPPUNIT_FAIL("Caught exception: " + e.get_message());
        }

        DBG(cerr << __func__ << " - END" << endl);
    }

    void get_hash_test()
    {
        d_mds = GlobalMetadataStore::get_instance(d_mds_dir, c_mds_prefix, 0);
        DBG(cerr << "retrieved GlobalMetadataStore instance: " << d_mds << endl);
        CPPUNIT_ASSERT(d_mds);

        CPPUNIT_ASSERT(d_mds->get_hash("/path/name.txt") == d_mds->get_hash("path/name.txt"));
    }

    void get_hash_test_error()
    {
        d_mds = GlobalMetadataStore::get_instance(d_mds_dir, c_mds_prefix, 0);
        DBG(cerr << "retrieved GlobalMetadataStore instance: " << d_mds << endl);
        CPPUNIT_ASSERT(d_mds);

        CPPUNIT_FAIL(d_mds->get_hash(""));  // if this does not throw, it's a test fail
    }

    // This test may fail if the -k option is used.
    void cache_a_dds_response()
    {
        DBG(cerr << __func__ << " - BEGIN" << endl);

        try {
            init_dds_and_mds();

            // Store it - this will work if the the code is cleaning the cache.
            GlobalMetadataStore::StreamDDS write_the_dds_response(d_test_dds);
            bool stored = d_mds->store_dap_response(write_the_dds_response,
                d_test_dds->get_dataset_name() + ".dds_r", d_test_dds->get_dataset_name(), "DDS");

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
    void cache_a_das_response()
    {
        DBG(cerr << __func__ << " - BEGIN" << endl);

        try {
            init_dds_and_mds();

            // Store it - this will work if the the code is cleaning the cache.
            GlobalMetadataStore::StreamDAS write_the_das_response(d_test_dds);
            bool stored = d_mds->store_dap_response(write_the_das_response,
                d_test_dds->get_dataset_name() + ".das_r", d_test_dds->get_dataset_name(), "DAS");

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

    void removeDMRVersion(string& str)
    {
        std::regex dmr_version_regex("dmrVersion=\"[0-9]+\.[0-9]+\"");
        str = std::regex_replace(str, dmr_version_regex, "dmrVersion=\"removed\"");
    }

    void cache_a_dmr_response()
    {
        DBG(cerr << __func__ << " - BEGIN" << endl);

        try {
            init_dds_and_mds();

            // Store it - this will work if the the code is cleaning the cache.
            GlobalMetadataStore::StreamDMR write_the_dmr_response(d_test_dds);
            bool stored = d_mds->store_dap_response(write_the_dmr_response,
                d_test_dds->get_dataset_name() + ".dmr_r", d_test_dds->get_dataset_name(), "DMR");

            CPPUNIT_ASSERT(stored);

            // Now check the file
            string baseline_name = c_mds_baselines + "/" + c_mds_prefix + "SimpleTypes.dmr_r";
            DBG(cerr << "Reading baseline: " << baseline_name << endl);
            CPPUNIT_ASSERT(access(baseline_name.c_str(), R_OK) == 0);

            string test_05_dmr_baseline = read_test_baseline(baseline_name);
            removeDMRVersion(test_05_dmr_baseline);

            string response_name = d_mds_dir + "/" + c_mds_prefix + "SimpleTypes.dmr_r";
            DBG(cerr << "Reading response: " << response_name << endl);
            CPPUNIT_ASSERT(access(response_name.c_str(), R_OK) == 0);

            string stored_response = read_test_baseline(response_name);
            removeDMRVersion(stored_response);

            CPPUNIT_ASSERT(stored_response == test_05_dmr_baseline);
        }
        catch (BESError &e) {
            CPPUNIT_FAIL(e.get_message());
        }

        DBG(cerr << __func__ << " - END" << endl);
    }

    void get_cache_lmt_test(){
    	init_dmr_and_mds();

    	string real_name = string(TEST_SRC_DIR) + "/input-files/test_01.dmr";
		BESFileContainer cont("cont", real_name, "test_handler");
		cont.set_relative_name("/input-files/test_01.dmr");

		BESRequestHandler *besRH = BESRequestHandlerList::TheList()->find_handler("test_handler");
    	CPPUNIT_ASSERT(besRH != 0);

    	string hash_name = d_mds->get_hash(cont.get_relative_name() + "dmr_r");
    	DBG(cerr << "hash_name: " << hash_name << endl);

    	string item_name = d_mds->get_cache_file_name(hash_name, false /*mangle*/);
    	DBG(cerr << "item_name: " << item_name << endl);

    	try {
    		int fd = open(item_name.c_str(), O_RDWR | O_CREAT, 00664 /*mode = rw rw r*/);
    		CPPUNIT_ASSERT(fd != -1);

    		struct stat statbuf;

    		if (stat(item_name.c_str(), &statbuf) == -1){
    			throw BESNotFoundError(strerror(errno), __FILE__, __LINE__);
    		}//end if(error)

    		time_t ctime = statbuf.st_ctime;
    		DBG(cerr << "ctime: " << ctime << endl);
    		time_t mtime = d_mds->get_cache_lmt("/input-files/test_01.dmr", "dmr_r");
    		DBG(cerr << "mtime: " << mtime << endl);

    		bool test = ((ctime - mtime) <= 2);
    		CPPUNIT_ASSERT(test);
    	}
    	catch (BESError &e) {
    		unlink(item_name.c_str());
    		ostringstream oss;
    		oss << "Error: " << e.get_message() << " " << e.get_file() << ":" << e.get_line();
    		CPPUNIT_FAIL(oss.str());
    	}
    	catch (...) {
    		unlink(item_name.c_str());
    		throw;
    	}

    	// TODO The clean_cache_dir() function hangs if this file is left in the cache
    	unlink(item_name.c_str());
    }//get_cache_lmt_test()

	void is_available_helper_dmr_test_1(){
    	init_dmr_and_mds();

    	string real_name = string(TEST_SRC_DIR) + "/input-files/test_01.dmr";
		BESFileContainer cont("cont", real_name, "test_handler");
		cont.set_relative_name("/input-files/test_01.dmr");

		BESRequestHandler *besRH = BESRequestHandlerList::TheList()->find_handler("test_handler");
    	CPPUNIT_ASSERT(besRH != 0);

    	string hash_name = d_mds->get_hash(cont.get_relative_name() + "dmr_r");
    	DBG(cerr << "hash_name: " << hash_name << endl);

    	string item_name = d_mds->get_cache_file_name(hash_name, false /*mangle*/);
    	DBG(cerr << "item_name: " << item_name << endl);

    	try {
    		int fd = open(item_name.c_str(), O_RDWR | O_CREAT, 00664 /*mode = rw rw r*/);
    		CPPUNIT_ASSERT(fd != -1);

    		bool reload = d_mds->is_available_helper(cont.get_real_name(), cont.get_relative_name(), cont.get_container_type(), "dmr_r");
    		// since the cache file is newer than the 'real' file, we should have the lock
    		CPPUNIT_ASSERT(!reload);
    	}
    	catch (BESError &e) {
    		unlink(item_name.c_str());
    		ostringstream oss;
    		oss << "Error: " << e.get_message() << " " << e.get_file() << ":" << e.get_line();
    		CPPUNIT_FAIL(oss.str());
    	}
    	catch (...) {
    		unlink(item_name.c_str());
    		throw;
    	}

    	// TODO The clean_cache_dir() function hangs if this file is left in the cache
    	unlink(item_name.c_str());
    }//end is_available_helper_dmr_test_1()

    void is_available_helper_dmr_test_2(){
    	init_dmr_and_mds();

    	string relative_file = "/tmp/temp_01.dmr";
    	string real_name = string(TEST_BUILD_DIR) + relative_file;
    	BESFileContainer cont("cont", real_name, "test_handler");
    	cont.set_relative_name(relative_file);

		BESRequestHandler *besRH = BESRequestHandlerList::TheList()->find_handler("test_handler");
    	CPPUNIT_ASSERT(besRH != 0);

    	string hash_name = d_mds->get_hash(cont.get_relative_name() + "dmr_r");
    	DBG(cerr << "hash_name: " << hash_name << endl);

    	string item_name = d_mds->get_cache_file_name(hash_name, false /*mangle*/);
    	DBG(cerr << "item_name: " << item_name << endl);

    	try {
    		int fd = open(item_name.c_str(), O_RDWR | O_CREAT, 00664 /*mode = rw rw r*/);
    		CPPUNIT_ASSERT(fd != -1);

    		sleep(3);

    		int gd = open(real_name.c_str(), O_RDWR | O_CREAT, 00664 /*mode = rw rw r*/);
    		CPPUNIT_ASSERT(gd != -1);

    		bool reload = d_mds->is_available_helper(cont.get_real_name(), cont.get_relative_name(), cont.get_container_type(), "dmr_r");
    		// since the cache file is newer than the 'real' file, we should have the lock
    		CPPUNIT_ASSERT(reload);
    	}
    	catch (BESError &e) {
    		unlink(item_name.c_str());
    		unlink(real_name.c_str());
    		ostringstream oss;
    		oss << "Error: " << e.get_message() << " " << e.get_file() << ":" << e.get_line();
    		CPPUNIT_FAIL(oss.str());
    	}
    	catch (...) {
    		unlink(item_name.c_str());
    		unlink(real_name.c_str());
    		throw;
    	}

    	// TODO The clean_cache_dir() function hangs if this file is left in the cache
    	unlink(item_name.c_str());
    	unlink(real_name.c_str());
    }//end is_available_helper_dmr_test_2()

    void is_available_helper_dds_test_1(){
    	init_dds_and_mds();

    	string real_name = string(TEST_SRC_DIR) + "/input-files/sequence_1.dds";
    	BESFileContainer cont("cont", real_name, "test_handler");
    	cont.set_relative_name("/input-files/sequence_1.dds");

		BESRequestHandler *besRH = BESRequestHandlerList::TheList()->find_handler("test_handler");
    	CPPUNIT_ASSERT(besRH != 0);

    	string hash_name = d_mds->get_hash(cont.get_relative_name() + "dds_r");
    	DBG(cerr << "hash_name: " << hash_name << endl);

    	string item_name = d_mds->get_cache_file_name(hash_name, false /*mangle*/);
    	DBG(cerr << "item_name: " << item_name << endl);

    	try {
    		int fd = open(item_name.c_str(), O_RDWR | O_CREAT, 00664 /*mode = rw rw r*/);
    		CPPUNIT_ASSERT(fd != -1);

    		bool reload = d_mds->is_available_helper(cont.get_real_name(), cont.get_relative_name(), cont.get_container_type(), "dds_r");
    		// since the cache file is newer than the 'real' file, we should have the lock
    		CPPUNIT_ASSERT(!reload);
    	}
    	catch (BESError &e) {
    		unlink(item_name.c_str());
    		ostringstream oss;
    		oss << "Error: " << e.get_message() << " " << e.get_file() << ":" << e.get_line();
    		CPPUNIT_FAIL(oss.str());
    	}
    	catch (...) {
    		unlink(item_name.c_str());
    		throw;
    	}

    	// TODO The clean_cache_dir() function hangs if this file is left in the cache
    	unlink(item_name.c_str());
    }//end is_available_helper_dds_test_1()

    void is_available_helper_dds_test_2(){
    	init_dds_and_mds();

    	string relative_file = "/tmp/temp_02.dds";
    	string real_name = string(TEST_BUILD_DIR) + relative_file;
    	BESFileContainer cont("cont", real_name, "test_handler");
    	cont.set_relative_name(relative_file);

		BESRequestHandler *besRH = BESRequestHandlerList::TheList()->find_handler("test_handler");
    	CPPUNIT_ASSERT(besRH != 0);

    	string hash_name = d_mds->get_hash(cont.get_relative_name() + "dds_r");
    	DBG(cerr << "hash_name: " << hash_name << endl);

    	string item_name = d_mds->get_cache_file_name(hash_name, false /*mangle*/);
    	DBG(cerr << "item_name: " << item_name << endl);

    	try {
    		int fd = open(item_name.c_str(), O_RDWR | O_CREAT, 00664 /*mode = rw rw r*/);
    		CPPUNIT_ASSERT(fd != -1);

    		sleep(3);

    		int gd = open(real_name.c_str(), O_RDWR | O_CREAT, 00664 /*mode = rw rw r*/);
    		CPPUNIT_ASSERT(gd != -1);

    		bool reload = d_mds->is_available_helper(cont.get_real_name(), cont.get_relative_name(), cont.get_container_type(), "dds_r");
    		// since the cache file is newer than the 'real' file, we should have the lock
    		CPPUNIT_ASSERT(reload);
    	}
    	catch (BESError &e) {
    		unlink(item_name.c_str());
    		unlink(real_name.c_str());
    		ostringstream oss;
    		oss << "Error: " << e.get_message() << " " << e.get_file() << ":" << e.get_line();
    		CPPUNIT_FAIL(oss.str());
    	}
    	catch (...) {
    		unlink(item_name.c_str());
    		unlink(real_name.c_str());
    		throw;
    	}

    	// TODO The clean_cache_dir() function hangs if this file is left in the cache
    	unlink(item_name.c_str());
    	unlink(real_name.c_str());
    }//end is_available_helper_dds_test_2()

    void is_available_helper_das_test_1(){
    	init_dds_and_mds();

    	string real_name = string(TEST_BUILD_DIR) + "/tmp/test_01.das";
    	BESFileContainer cont("cont", real_name, "test_handler");
    	cont.set_relative_name("/tmp/test_01.das");

		BESRequestHandler *besRH = BESRequestHandlerList::TheList()->find_handler("test_handler");
    	CPPUNIT_ASSERT(besRH != 0);

    	string hash_name = d_mds->get_hash(cont.get_relative_name() + "das_r");
    	DBG(cerr << "hash_name: " << hash_name << endl);

    	string item_name = d_mds->get_cache_file_name(hash_name, false /*mangle*/);
    	DBG(cerr << "item_name: " << item_name << endl);

    	try {
    		int gd = open(real_name.c_str(), O_RDWR | O_CREAT, 00664 /*mode = rw rw r*/);
    		CPPUNIT_ASSERT(gd != -1);

    		sleep(3);

    		int fd = open(item_name.c_str(), O_RDWR | O_CREAT, 00664 /*mode = rw rw r*/);
    		CPPUNIT_ASSERT(fd != -1);

    		bool reload = d_mds->is_available_helper(cont.get_real_name(), cont.get_relative_name(), cont.get_container_type(), "das_r");
    		// since the cache file is newer than the 'real' file, we should have the lock
    		CPPUNIT_ASSERT(!reload);
    	}
    	catch (BESError &e) {
    		unlink(item_name.c_str());
    		unlink(real_name.c_str());
    		ostringstream oss;
    		oss << "Error: " << e.get_message() << " " << e.get_file() << ":" << e.get_line();
    		CPPUNIT_FAIL(oss.str());
    	}
    	catch (...) {
    		unlink(item_name.c_str());
    		unlink(real_name.c_str());
    		throw;
    	}

    	// TODO The clean_cache_dir() function hangs if this file is left in the cache
    	unlink(item_name.c_str());
    	unlink(real_name.c_str());
    }//end is_available_helper_das_test_1()

    void is_available_helper_das_test_2(){
    	init_dds_and_mds();

    	string relative_file = "/tmp/temp_03.das";
    	string real_name = string(TEST_BUILD_DIR) + relative_file;
    	BESFileContainer cont("cont", real_name, "test_handler");
    	cont.set_relative_name(relative_file);

		BESRequestHandler *besRH = BESRequestHandlerList::TheList()->find_handler("test_handler");
    	CPPUNIT_ASSERT(besRH != 0);

    	string hash_name = d_mds->get_hash(cont.get_relative_name() + "das_r");
    	DBG(cerr << "hash_name: " << hash_name << endl);

    	string item_name = d_mds->get_cache_file_name(hash_name, false /*mangle*/);
    	DBG(cerr << "item_name: " << item_name << endl);

    	try {
    		int fd = open(item_name.c_str(), O_RDWR | O_CREAT, 00664 /*mode = rw rw r*/);
    		CPPUNIT_ASSERT(fd != -1);

    		sleep(3);

    		int gd = open(real_name.c_str(), O_RDWR | O_CREAT, 00664 /*mode = rw rw r*/);
    		CPPUNIT_ASSERT(gd != -1);

    		bool reload = d_mds->is_available_helper(cont.get_real_name(), cont.get_relative_name(), cont.get_container_type(), "das_r");
    		// since the cache file is newer than the 'real' file, we should have the lock
    		CPPUNIT_ASSERT(reload);
    	}
    	catch (BESError &e) {
    		unlink(item_name.c_str());
    		unlink(real_name.c_str());
    		ostringstream oss;
    		oss << "Error: " << e.get_message() << " " << e.get_file() << ":" << e.get_line();
    		CPPUNIT_FAIL(oss.str());
    	}
    	catch (...) {
    		unlink(item_name.c_str());
    		unlink(real_name.c_str());
    		throw;
    	}

    	// TODO The clean_cache_dir() function hangs if this file is left in the cache
    	unlink(item_name.c_str());
    	unlink(real_name.c_str());
    }//end is_available_helper_das_test_2()

    void is_available_helper_dmrpp_test_1(){
    	init_dmr_and_mds();

    	string real_name = string(TEST_SRC_DIR) + "/input-files/chunked_fourD.h5.dmrpp";
    	BESFileContainer cont("cont", real_name, "test_handler");
    	cont.set_relative_name("/input-files/chunked_fourD.h5.dmrpp");

		BESRequestHandler *besRH = BESRequestHandlerList::TheList()->find_handler("test_handler");
    	CPPUNIT_ASSERT(besRH != 0);

    	string hash_name = d_mds->get_hash(cont.get_relative_name() + "dmrpp_r");
    	DBG(cerr << "hash_name: " << hash_name << endl);

    	string item_name = d_mds->get_cache_file_name(hash_name, false /*mangle*/);
    	DBG(cerr << "item_name: " << item_name << endl);

    	try {
    		int fd = open(item_name.c_str(), O_RDWR | O_CREAT, 00664 /*mode = rw rw r*/);
    		CPPUNIT_ASSERT(fd != -1);

    		bool reload = d_mds->is_available_helper(cont.get_real_name(), cont.get_relative_name(), cont.get_container_type(), "dmrpp_r");
    		// since the cache file is newer than the 'real' file, we should have the lock
    		CPPUNIT_ASSERT(!reload);
    	}
    	catch (BESError &e) {
    		unlink(item_name.c_str());
    		ostringstream oss;
    		oss << "Error: " << e.get_message() << " " << e.get_file() << ":" << e.get_line();
    		CPPUNIT_FAIL(oss.str());
    	}
    	catch (...) {
    		unlink(item_name.c_str());
    		throw;
    	}

    	// TODO The clean_cache_dir() function hangs if this file is left in the cache
    	unlink(item_name.c_str());
    }//end is_available_helper_dmrpp_test_1()

    void is_available_helper_dmrpp_test_2(){
    	init_dmr_and_mds();

    	string relative_file = "/tmp/temp_04.dmrpp";
    	string real_name = string(TEST_BUILD_DIR) + relative_file;
    	BESFileContainer cont("cont", real_name, "test_handler");
    	cont.set_relative_name(relative_file);

		BESRequestHandler *besRH = BESRequestHandlerList::TheList()->find_handler("test_handler");
    	CPPUNIT_ASSERT(besRH != 0);

    	string hash_name = d_mds->get_hash(cont.get_relative_name() + "dmrpp_r");
    	DBG(cerr << "hash_name: " << hash_name << endl);

    	string item_name = d_mds->get_cache_file_name(hash_name, false /*mangle*/);
    	DBG(cerr << "item_name: " << item_name << endl);

    	try {
    		int fd = open(item_name.c_str(), O_RDWR | O_CREAT, 00664 /*mode = rw rw r*/);
    		CPPUNIT_ASSERT(fd != -1);

    		sleep(3);

    		int gd = open(real_name.c_str(), O_RDWR | O_CREAT, 00664 /*mode = rw rw r*/);
    		CPPUNIT_ASSERT(gd != -1);

    		bool reload = d_mds->is_available_helper(cont.get_real_name(), cont.get_relative_name(), cont.get_container_type(), "dmrpp_r");
    		// since the cache file is newer than the 'real' file, we should have the lock
    		CPPUNIT_ASSERT(reload);
    	}
    	catch (BESError &e) {
    		unlink(item_name.c_str());
    		unlink(real_name.c_str());
    		ostringstream oss;
    		oss << "Error: " << e.get_message() << " " << e.get_file() << ":" << e.get_line();
    		CPPUNIT_FAIL(oss.str());
    	}
    	catch (...) {
    		unlink(item_name.c_str());
    		unlink(real_name.c_str());
    		throw;
    	}

    	// TODO The clean_cache_dir() function hangs if this file is left in the cache
    	unlink(item_name.c_str());
    	unlink(real_name.c_str());
    }//end is_available_helper_dmrpp_test_2()

    // TODO This test depends on the path /relative/name not existing. Fix.
    void is_dmr_available_test_1() {
    	init_dmr_and_mds();

    	BESFileContainer cont("cont", "/real/relative/name", "test_handler");
    	cont.set_relative_name("/relative/name");

    	DBG(cerr << "cont.get_real_name: " << cont.get_real_name() << endl);
    	DBG(cerr << "cont.get_relative_name: " << cont.get_relative_name() << endl);

    	CPPUNIT_ASSERT(cont.get_real_name() == "/real/relative/name");
    	CPPUNIT_ASSERT(cont.get_relative_name() == "/relative/name");

    	GlobalMetadataStore::MDSReadLock lock = d_mds->is_dmr_available(cont);
    	CPPUNIT_ASSERT(!lock()); // The path /relative/name should not exist
    }//end is_dmr_available_test_1()

    void is_dmr_available_test_2() {
    	init_dmr_and_mds();
    	// BESFileContainer(const string &sym_name, const string &real_name, const string &type);
    	string real_name = string(TEST_SRC_DIR) + "/input-files/test_01.dmr";
    	BESFileContainer cont("cont", real_name, "test_handler");
    	cont.set_relative_name("/input-files/test_01.dmr");

    	DBG(cerr << "cont.get_real_name: " << cont.get_real_name() << endl);
    	DBG(cerr << "cont.get_relative_name: " << cont.get_relative_name() << endl);

    	CPPUNIT_ASSERT(cont.get_real_name() == real_name);
    	CPPUNIT_ASSERT(cont.get_relative_name() == "/input-files/test_01.dmr");

    	BESRequestHandler *besRH = BESRequestHandlerList::TheList()->find_handler("test_handler");
    	DBG(cerr << "besRH: " << (void*)besRH << endl);
    	DBG(cerr << "d_handler: " << (void*)&d_handler << endl);
    	CPPUNIT_ASSERT(besRH != 0);
    	DBG(cerr << "d_handler.get_name(): " << d_handler.get_name() << endl);
    	DBG(cerr << "besRH->get_name(): " << besRH->get_name() << endl);

    	//string item_name = get_cache_file_name(get_hash(cont.get_relative_name() + "dmr_r"), false);
    	string hash_name = d_mds->get_hash(cont.get_relative_name() + "dmr_r");
    	DBG(cerr << "hash_name: " << hash_name << endl);

    	string item_name = d_mds->get_cache_file_name(hash_name, false /*mangle*/);
    	DBG(cerr << "item_name: " << item_name << endl);

    	try {
    		int fd = open(item_name.c_str(), O_RDWR | O_CREAT, 00664 /*mode = rw rw r*/);
    		CPPUNIT_ASSERT(fd != -1);

    		GlobalMetadataStore::MDSReadLock lock = d_mds->is_dmr_available(cont);
    		// since the cache file is newer than the 'real' file, we should have the lock
    		CPPUNIT_ASSERT(lock());
    	}
    	catch (BESError &e) {
    		unlink(item_name.c_str());
    		ostringstream oss;
    		oss << "Error: " << e.get_message() << " " << e.get_file() << ":" << e.get_line();
    		CPPUNIT_FAIL(oss.str());
    	}
    	catch (...) {
    		unlink(item_name.c_str());
    		throw;
    	}

    	// TODO The clean_cache_dir() function hangs if this file is left in the cache
    	unlink(item_name.c_str());
    }//end is_dmr_available_test_2()

    void is_dmr_available_test_3() {
    	init_dmr_and_mds();

    	// BESFileContainer(const string &sym_name, const string &real_name, const string &type);
    	string relative_file = "/tmp/temp_01.dmr";
    	string real_name = string(TEST_BUILD_DIR) + relative_file;
    	BESFileContainer cont("cont", real_name, "test_handler");
    	cont.set_relative_name(relative_file);

    	DBG(cerr << "cont.get_real_name: " << cont.get_real_name() << endl);
    	DBG(cerr << "cont.get_relative_name: " << cont.get_relative_name() << endl);

    	CPPUNIT_ASSERT(cont.get_real_name() == real_name);
    	CPPUNIT_ASSERT(cont.get_relative_name() == "/tmp/temp_01.dmr");

    	BESRequestHandler *besRH = BESRequestHandlerList::TheList()->find_handler("test_handler");
    	DBG(cerr << "besRH: " << (void*)besRH << endl);
    	DBG(cerr << "d_handler: " << (void*)&d_handler << endl);
    	CPPUNIT_ASSERT(besRH != 0);
    	DBG(cerr << "d_handler.get_name(): " << d_handler.get_name() << endl);
    	DBG(cerr << "besRH->get_name(): " << besRH->get_name() << endl);

    	//string item_name = get_cache_file_name(get_hash(cont.get_relative_name() + "dmr_r"), false);
    	string hash_name = d_mds->get_hash(cont.get_relative_name() + "dmr_r");
    	DBG(cerr << "hash_name: " << hash_name << endl);

    	string item_name = d_mds->get_cache_file_name(hash_name, false /*mangle*/);
    	DBG(cerr << "item_name: " << item_name << endl);

    	try {
    		int fd = open(item_name.c_str(), O_RDWR | O_CREAT, 00664 /*mode = rw rw r*/);
    		CPPUNIT_ASSERT(fd != -1);

    		sleep(3);

    		int gd = open(real_name.c_str(), O_RDWR | O_CREAT, 00664 /*mode = rw rw r*/);
    		CPPUNIT_ASSERT(gd != -1);

    		GlobalMetadataStore::MDSReadLock lock = d_mds->is_dmr_available(cont);
    		// since the cache file is newer than the 'real' file, we should have the lock
    		CPPUNIT_ASSERT(!lock());
    	}
    	catch (BESError &e) {
    		unlink(item_name.c_str());
    		unlink(real_name.c_str());
    		ostringstream oss;
    		oss << "Error: " << e.get_message() << " " << e.get_file() << ":" << e.get_line();
    		CPPUNIT_FAIL(oss.str());
    	}
    	catch (...) {
    		unlink(item_name.c_str());
    		unlink(real_name.c_str());
    		throw;
    	}

    	// TODO The clean_cache_dir() function hangs if this file is left in the cache
    	unlink(item_name.c_str());
    	unlink(real_name.c_str());
    }//end is_dmr_available_test_3()

    void is_dmr_available_test_4() {
    	init_dmr_and_mds();
    	// BESFileContainer(const string &sym_name, const string &real_name, const string &type);
    	string real_name = string(TEST_SRC_DIR) + "/input-files/test_01.dmr";
    	BESFileContainer cont("cont", real_name, "test_handler");
    	cont.set_relative_name("/input-files/test_01.dmr");

    	DBG(cerr << "cont.get_real_name: " << cont.get_real_name() << endl);
    	DBG(cerr << "cont.get_relative_name: " << cont.get_relative_name() << endl);
    	DBG(cerr << "cont.get_container_type: " << cont.get_container_type() << endl);

    	CPPUNIT_ASSERT(cont.get_real_name() == real_name);
    	CPPUNIT_ASSERT(cont.get_relative_name() == "/input-files/test_01.dmr");

    	BESRequestHandler *besRH = BESRequestHandlerList::TheList()->find_handler("test_handler");
    	DBG(cerr << "besRH: " << (void*)besRH << endl);
    	DBG(cerr << "d_handler: " << (void*)&d_handler << endl);
    	CPPUNIT_ASSERT(besRH != 0);
    	DBG(cerr << "d_handler.get_name(): " << d_handler.get_name() << endl);
    	DBG(cerr << "besRH->get_name(): " << besRH->get_name() << endl);

    	//string item_name = get_cache_file_name(get_hash(cont.get_relative_name() + "dmr_r"), false);
    	string hash_name = d_mds->get_hash(cont.get_relative_name() + "dmr_r");
    	DBG(cerr << "hash_name: " << hash_name << endl);

    	string item_name = d_mds->get_cache_file_name(hash_name, false /*mangle*/);
    	DBG(cerr << "item_name: " << item_name << endl);

    	try {
    		int fd = open(item_name.c_str(), O_RDWR | O_CREAT, 00664 /*mode = rw rw r*/);
    		CPPUNIT_ASSERT(fd != -1);

    		GlobalMetadataStore::MDSReadLock lock = d_mds->is_dmr_available(
    				cont.get_real_name(), cont.get_relative_name(), cont.get_container_type());
    		// since the cache file is newer than the 'real' file, we should have the lock
    		CPPUNIT_ASSERT(lock());
    	}
    	catch (BESError &e) {
    		unlink(item_name.c_str());
    		ostringstream oss;
    		oss << "Error: " << e.get_message() << " " << e.get_file() << ":" << e.get_line();
    		CPPUNIT_FAIL(oss.str());
    	}
    	catch (...) {
    		unlink(item_name.c_str());
    		throw;
    	}

    	// TODO The clean_cache_dir() function hangs if this file is left in the cache
    	unlink(item_name.c_str());
    }//end is_dmr_available_test_4()

    void is_dmr_available_test_5() {
    	init_dmr_and_mds();

    	// BESFileContainer(const string &sym_name, const string &real_name, const string &type);
    	string relative_file = "/tmp/temp_01.dmr";
    	string real_name = string(TEST_BUILD_DIR) + relative_file;
    	BESFileContainer cont("cont", real_name, "test_handler");
    	cont.set_relative_name(relative_file);

    	DBG(cerr << "cont.get_real_name: " << cont.get_real_name() << endl);
    	DBG(cerr << "cont.get_relative_name: " << cont.get_relative_name() << endl);

    	CPPUNIT_ASSERT(cont.get_real_name() == real_name);
    	CPPUNIT_ASSERT(cont.get_relative_name() == "/tmp/temp_01.dmr");

    	BESRequestHandler *besRH = BESRequestHandlerList::TheList()->find_handler("test_handler");
    	DBG(cerr << "besRH: " << (void*)besRH << endl);
    	DBG(cerr << "d_handler: " << (void*)&d_handler << endl);
    	CPPUNIT_ASSERT(besRH != 0);
    	DBG(cerr << "d_handler.get_name(): " << d_handler.get_name() << endl);
    	DBG(cerr << "besRH->get_name(): " << besRH->get_name() << endl);

    	//string item_name = get_cache_file_name(get_hash(cont.get_relative_name() + "dmr_r"), false);
    	string hash_name = d_mds->get_hash(cont.get_relative_name() + "dmr_r");
    	DBG(cerr << "hash_name: " << hash_name << endl);

    	string item_name = d_mds->get_cache_file_name(hash_name, false /*mangle*/);
    	DBG(cerr << "item_name: " << item_name << endl);

    	try {
    		int fd = open(item_name.c_str(), O_RDWR | O_CREAT, 00664 /*mode = rw rw r*/);
    		CPPUNIT_ASSERT(fd != -1);

    		sleep(3);

    		int gd = open(real_name.c_str(), O_RDWR | O_CREAT, 00664 /*mode = rw rw r*/);
    		CPPUNIT_ASSERT(gd != -1);

    		GlobalMetadataStore::MDSReadLock lock = d_mds->is_dmr_available(
    				cont.get_real_name(), cont.get_relative_name(), cont.get_container_type());
    		// since the cache file is newer than the 'real' file, we should have the lock
    		CPPUNIT_ASSERT(!lock());
    	}
    	catch (BESError &e) {
    		unlink(item_name.c_str());
    		unlink(real_name.c_str());
    		ostringstream oss;
    		oss << "Error: " << e.get_message() << " " << e.get_file() << ":" << e.get_line();
    		CPPUNIT_FAIL(oss.str());
    	}
    	catch (...) {
    		unlink(item_name.c_str());
    		unlink(real_name.c_str());
    		throw;
    	}

    	// TODO The clean_cache_dir() function hangs if this file is left in the cache
    	unlink(item_name.c_str());
    	unlink(real_name.c_str());
    }//end is_dmr_available_test_5()

    // TODO This test depends on the path /relative/name not existing. Fix.
    void is_dds_available_test_1() {
    	init_dds_and_mds();

    	BESFileContainer cont("cont", "/real/relative/name", "test_handler");
    	cont.set_relative_name("/relative/name");

    	DBG(cerr << "cont.get_real_name: " << cont.get_real_name() << endl);
    	DBG(cerr << "cont.get_relative_name: " << cont.get_relative_name() << endl);

    	CPPUNIT_ASSERT(cont.get_real_name() == "/real/relative/name");
    	CPPUNIT_ASSERT(cont.get_relative_name() == "/relative/name");

    	GlobalMetadataStore::MDSReadLock lock = d_mds->is_dds_available(cont);
    	CPPUNIT_ASSERT(!lock()); // The path /relative/name should not exist
    }//end is_dds_available_test_1()

    void is_dds_available_test_2() {
    	init_dds_and_mds();
    	// BESFileContainer(const string &sym_name, const string &real_name, const string &type);
    	string real_name = string(TEST_SRC_DIR) + "/input-files/sequence_1.dds";
    	BESFileContainer cont("cont", real_name, "test_handler");
    	cont.set_relative_name("/input-files/sequence_1.dds");

    	DBG(cerr << "cont.get_real_name: " << cont.get_real_name() << endl);
    	DBG(cerr << "cont.get_relative_name: " << cont.get_relative_name() << endl);

    	CPPUNIT_ASSERT(cont.get_real_name() == real_name);
    	CPPUNIT_ASSERT(cont.get_relative_name() == "/input-files/sequence_1.dds");

    	BESRequestHandler *besRH = BESRequestHandlerList::TheList()->find_handler("test_handler");
    	DBG(cerr << "besRH: " << (void*)besRH << endl);
    	DBG(cerr << "d_handler: " << (void*)&d_handler << endl);
    	CPPUNIT_ASSERT(besRH != 0);
    	DBG(cerr << "d_handler.get_name(): " << d_handler.get_name() << endl);
    	DBG(cerr << "besRH->get_name(): " << besRH->get_name() << endl);

    	//string item_name = get_cache_file_name(get_hash(cont.get_relative_name() + "dmr_r"), false);
    	string hash_name = d_mds->get_hash(cont.get_relative_name() + "dds_r");
    	DBG(cerr << "hash_name: " << hash_name << endl);

    	string item_name = d_mds->get_cache_file_name(hash_name, false /*mangle*/);
    	DBG(cerr << "item_name: " << item_name << endl);

    	try {
    		int fd = open(item_name.c_str(), O_RDWR | O_CREAT, 00664 /*mode = rw rw r*/);
    		CPPUNIT_ASSERT(fd != -1);

    		GlobalMetadataStore::MDSReadLock lock = d_mds->is_dds_available(cont);
    		// since the cache file is newer than the 'real' file, we should have the lock
    		CPPUNIT_ASSERT(lock());
    	}
    	catch (BESError &e) {
    		unlink(item_name.c_str());
    		ostringstream oss;
    		oss << "Error: " << e.get_message() << " " << e.get_file() << ":" << e.get_line();
    		CPPUNIT_FAIL(oss.str());
    	}
    	catch (...) {
    		unlink(item_name.c_str());
    		throw;
    	}

    	// TODO The clean_cache_dir() function hangs if this file is left in the cache
    	unlink(item_name.c_str());
    }//end is_dds_available_test_2()

    void is_dds_available_test_3() {
    	init_dds_and_mds();
    	// BESFileContainer(const string &sym_name, const string &real_name, const string &type);
    	string relative_file = "/tmp/temp_02.dds";
    	string real_name = string(TEST_BUILD_DIR) + relative_file;
    	BESFileContainer cont("cont", real_name, "test_handler");
    	cont.set_relative_name(relative_file);

    	DBG(cerr << "cont.get_real_name: " << cont.get_real_name() << endl);
    	DBG(cerr << "cont.get_relative_name: " << cont.get_relative_name() << endl);

    	CPPUNIT_ASSERT(cont.get_real_name() == real_name);
    	CPPUNIT_ASSERT(cont.get_relative_name() == "/tmp/temp_02.dds");

    	BESRequestHandler *besRH = BESRequestHandlerList::TheList()->find_handler("test_handler");
    	DBG(cerr << "besRH: " << (void*)besRH << endl);
    	DBG(cerr << "d_handler: " << (void*)&d_handler << endl);
    	CPPUNIT_ASSERT(besRH != 0);
    	DBG(cerr << "d_handler.get_name(): " << d_handler.get_name() << endl);
    	DBG(cerr << "besRH->get_name(): " << besRH->get_name() << endl);

    	//string item_name = get_cache_file_name(get_hash(cont.get_relative_name() + "dmr_r"), false);
    	string hash_name = d_mds->get_hash(cont.get_relative_name() + "dds_r");
    	DBG(cerr << "hash_name: " << hash_name << endl);

    	string item_name = d_mds->get_cache_file_name(hash_name, false /*mangle*/);
    	DBG(cerr << "item_name: " << item_name << endl);

    	try {
    		int fd = open(item_name.c_str(), O_RDWR | O_CREAT, 00664 /*mode = rw rw r*/);
    		CPPUNIT_ASSERT(fd != -1);

    		sleep(3);

    		int gd = open(real_name.c_str(), O_RDWR | O_CREAT, 00664 /*mode = rw rw r*/);
    		CPPUNIT_ASSERT(gd != -1);

    		GlobalMetadataStore::MDSReadLock lock = d_mds->is_dds_available(cont);
    		// since the cache file is newer than the 'real' file, we should have the lock
    		CPPUNIT_ASSERT(!lock());
    	}
    	catch (BESError &e) {
    		unlink(item_name.c_str());
    		unlink(real_name.c_str());
    		ostringstream oss;
    		oss << "Error: " << e.get_message() << " " << e.get_file() << ":" << e.get_line();
    		CPPUNIT_FAIL(oss.str());
    	}
    	catch (...) {
    		unlink(item_name.c_str());
    		unlink(real_name.c_str());
    		throw;
    	}

    	// TODO The clean_cache_dir() function hangs if this file is left in the cache
    	unlink(item_name.c_str());
    	unlink(real_name.c_str());
    }//end is_dds_available_test_3()

    // TODO This test depends on the path /relative/name not existing. Fix.
    void is_das_available_test_1() {
    	init_dds_and_mds();

    	BESFileContainer cont("cont", "/real/relative/name", "test_handler");
    	cont.set_relative_name("/relative/name");

    	DBG(cerr << "cont.get_real_name: " << cont.get_real_name() << endl);
    	DBG(cerr << "cont.get_relative_name: " << cont.get_relative_name() << endl);

    	CPPUNIT_ASSERT(cont.get_real_name() == "/real/relative/name");
    	CPPUNIT_ASSERT(cont.get_relative_name() == "/relative/name");

    	GlobalMetadataStore::MDSReadLock lock = d_mds->is_das_available(cont);
    	CPPUNIT_ASSERT(!lock()); // The path /relative/name should not exist
    }//end is_das_available_test_1()

    void is_das_available_test_2() {
    	init_dds_and_mds();
    	// BESFileContainer(const string &sym_name, const string &real_name, const string &type);
    	string real_name = string(TEST_BUILD_DIR) + "/tmp/test_01.das";
    	BESFileContainer cont("cont", real_name, "test_handler");
    	cont.set_relative_name("/tmp/test_01.das");

    	DBG(cerr << "cont.get_real_name: " << cont.get_real_name() << endl);
    	DBG(cerr << "cont.get_relative_name: " << cont.get_relative_name() << endl);

    	CPPUNIT_ASSERT(cont.get_real_name() == real_name);
    	CPPUNIT_ASSERT(cont.get_relative_name() == "/tmp/test_01.das");

    	BESRequestHandler *besRH = BESRequestHandlerList::TheList()->find_handler("test_handler");
    	DBG(cerr << "besRH: " << (void*)besRH << endl);
    	DBG(cerr << "d_handler: " << (void*)&d_handler << endl);
    	CPPUNIT_ASSERT(besRH != 0);
    	DBG(cerr << "d_handler.get_name(): " << d_handler.get_name() << endl);
    	DBG(cerr << "besRH->get_name(): " << besRH->get_name() << endl);

    	//string item_name = get_cache_file_name(get_hash(cont.get_relative_name() + "dmr_r"), false);
    	string hash_name = d_mds->get_hash(cont.get_relative_name() + "das_r");
    	DBG(cerr << "hash_name: " << hash_name << endl);

    	string item_name = d_mds->get_cache_file_name(hash_name, false /*mangle*/);
    	DBG(cerr << "item_name: " << item_name << endl);

    	try {
    		int gd = open(real_name.c_str(), O_RDWR | O_CREAT, 00664 /*mode = rw rw r*/);
    		CPPUNIT_ASSERT(gd != -1);

    		sleep(3);

    		int fd = open(item_name.c_str(), O_RDWR | O_CREAT, 00664 /*mode = rw rw r*/);
    		CPPUNIT_ASSERT(fd != -1);

    		GlobalMetadataStore::MDSReadLock lock = d_mds->is_das_available(cont);
    		// since the cache file is newer than the 'real' file, we should have the lock
    		CPPUNIT_ASSERT(lock());
    	}
    	catch (BESError &e) {
    		unlink(item_name.c_str());
    		unlink(real_name.c_str());
    		ostringstream oss;
    		oss << "Error: " << e.get_message() << " " << e.get_file() << ":" << e.get_line();
    		CPPUNIT_FAIL(oss.str());
    	}
    	catch (...) {
    		unlink(item_name.c_str());
    		unlink(real_name.c_str());
    		throw;
    	}

    	// TODO The clean_cache_dir() function hangs if this file is left in the cache
    	unlink(item_name.c_str());
    	unlink(real_name.c_str());
    }//end is_das_available_test_2()

    void is_das_available_test_3() {
    	init_dds_and_mds();
    	// BESFileContainer(const string &sym_name, const string &real_name, const string &type);
    	string relative_file = "/tmp/temp_03.das";
    	string real_name = string(TEST_BUILD_DIR) + relative_file;
    	BESFileContainer cont("cont", real_name, "test_handler");
    	cont.set_relative_name(relative_file);

    	DBG(cerr << "cont.get_real_name: " << cont.get_real_name() << endl);
    	DBG(cerr << "cont.get_relative_name: " << cont.get_relative_name() << endl);

    	CPPUNIT_ASSERT(cont.get_real_name() == real_name);
    	CPPUNIT_ASSERT(cont.get_relative_name() == "/tmp/temp_03.das");

    	BESRequestHandler *besRH = BESRequestHandlerList::TheList()->find_handler("test_handler");
    	DBG(cerr << "besRH: " << (void*)besRH << endl);
    	DBG(cerr << "d_handler: " << (void*)&d_handler << endl);
    	CPPUNIT_ASSERT(besRH != 0);
    	DBG(cerr << "d_handler.get_name(): " << d_handler.get_name() << endl);
    	DBG(cerr << "besRH->get_name(): " << besRH->get_name() << endl);

    	//string item_name = get_cache_file_name(get_hash(cont.get_relative_name() + "dmr_r"), false);
    	string hash_name = d_mds->get_hash(cont.get_relative_name() + "das_r");
    	DBG(cerr << "hash_name: " << hash_name << endl);

    	string item_name = d_mds->get_cache_file_name(hash_name, false /*mangle*/);
    	DBG(cerr << "item_name: " << item_name << endl);

    	try {
    		int fd = open(item_name.c_str(), O_RDWR | O_CREAT, 00664 /*mode = rw rw r*/);
    		CPPUNIT_ASSERT(fd != -1);

    		sleep(3);

    		int gd = open(real_name.c_str(), O_RDWR | O_CREAT, 00664 /*mode = rw rw r*/);
    		CPPUNIT_ASSERT(gd != -1);

    		GlobalMetadataStore::MDSReadLock lock = d_mds->is_das_available(cont);
    		// since the cache file is newer than the 'real' file, we should have the lock
    		CPPUNIT_ASSERT(!lock());
    	}
    	catch (BESError &e) {
    		unlink(item_name.c_str());
    		unlink(real_name.c_str());
    		ostringstream oss;
    		oss << "Error: " << e.get_message() << " " << e.get_file() << ":" << e.get_line();
    		CPPUNIT_FAIL(oss.str());
    	}
    	catch (...) {
    		unlink(item_name.c_str());
    		unlink(real_name.c_str());
    		throw;
    	}

    	// TODO The clean_cache_dir() function hangs if this file is left in the cache
    	unlink(item_name.c_str());
    	unlink(real_name.c_str());
    }//end is_das_available_test_3()

    // TODO This test depends on the path /relative/name not existing. Fix.
    void is_dmrpp_available_test_1() {
    	init_dmr_and_mds();

    	BESFileContainer cont("cont", "/real/relative/name", "test_handler");
    	cont.set_relative_name("/relative/name");

    	DBG(cerr << "cont.get_real_name: " << cont.get_real_name() << endl);
    	DBG(cerr << "cont.get_relative_name: " << cont.get_relative_name() << endl);

    	CPPUNIT_ASSERT(cont.get_real_name() == "/real/relative/name");
    	CPPUNIT_ASSERT(cont.get_relative_name() == "/relative/name");

    	GlobalMetadataStore::MDSReadLock lock = d_mds->is_dmrpp_available(cont);
    	CPPUNIT_ASSERT(!lock()); // The path /relative/name should not exist
    }//end is_dmr_available_test_1()

    void is_dmrpp_available_test_2() {
    	init_dmr_and_mds();
    	// BESFileContainer(const string &sym_name, const string &real_name, const string &type);
    	string real_name = string(TEST_SRC_DIR) + "/input-files/chunked_fourD.h5.dmrpp";
    	BESFileContainer cont("cont", real_name, "test_handler");
    	cont.set_relative_name("/input-files/chunked_fourD.h5.dmrpp");

    	DBG(cerr << "cont.get_real_name: " << cont.get_real_name() << endl);
    	DBG(cerr << "cont.get_relative_name: " << cont.get_relative_name() << endl);

    	CPPUNIT_ASSERT(cont.get_real_name() == real_name);
    	CPPUNIT_ASSERT(cont.get_relative_name() == "/input-files/chunked_fourD.h5.dmrpp");

    	BESRequestHandler *besRH = BESRequestHandlerList::TheList()->find_handler("test_handler");
    	DBG(cerr << "besRH: " << (void*)besRH << endl);
    	DBG(cerr << "d_handler: " << (void*)&d_handler << endl);
    	CPPUNIT_ASSERT(besRH != 0);
    	DBG(cerr << "d_handler.get_name(): " << d_handler.get_name() << endl);
    	DBG(cerr << "besRH->get_name(): " << besRH->get_name() << endl);

    	//string item_name = get_cache_file_name(get_hash(cont.get_relative_name() + "dmr_r"), false);
    	string hash_name = d_mds->get_hash(cont.get_relative_name() + "dmrpp_r");
    	DBG(cerr << "hash_name: " << hash_name << endl);

    	string item_name = d_mds->get_cache_file_name(hash_name, false /*mangle*/);
    	DBG(cerr << "item_name: " << item_name << endl);

    	try {
    		int fd = open(item_name.c_str(), O_RDWR | O_CREAT, 00664 /*mode = rw rw r*/);
    		CPPUNIT_ASSERT(fd != -1);

    		GlobalMetadataStore::MDSReadLock lock = d_mds->is_dmrpp_available(cont);
    		// since the cache file is newer than the 'real' file, we should have the lock
    		CPPUNIT_ASSERT(lock());
    	}
    	catch (BESError &e) {
    		unlink(item_name.c_str());
    		ostringstream oss;
    		oss << "Error: " << e.get_message() << " " << e.get_file() << ":" << e.get_line();
    		CPPUNIT_FAIL(oss.str());
    	}
    	catch (...) {
    		unlink(item_name.c_str());
    		throw;
    	}

    	// TODO The clean_cache_dir() function hangs if this file is left in the cache
    	unlink(item_name.c_str());
    }//end is_dmrpp_available_test_2()

    void is_dmrpp_available_test_3() {
    	init_dmr_and_mds();

    	// BESFileContainer(const string &sym_name, const string &real_name, const string &type);
    	string relative_file = "/tmp/temp_04.dmrpp";
    	string real_name = string(TEST_BUILD_DIR) + relative_file;
    	BESFileContainer cont("cont", real_name, "test_handler");
    	cont.set_relative_name(relative_file);

    	DBG(cerr << "cont.get_real_name: " << cont.get_real_name() << endl);
    	DBG(cerr << "cont.get_relative_name: " << cont.get_relative_name() << endl);

    	CPPUNIT_ASSERT(cont.get_real_name() == real_name);
    	CPPUNIT_ASSERT(cont.get_relative_name() == "/tmp/temp_04.dmrpp");

    	BESRequestHandler *besRH = BESRequestHandlerList::TheList()->find_handler("test_handler");
    	DBG(cerr << "besRH: " << (void*)besRH << endl);
    	DBG(cerr << "d_handler: " << (void*)&d_handler << endl);
    	CPPUNIT_ASSERT(besRH != 0);
    	DBG(cerr << "d_handler.get_name(): " << d_handler.get_name() << endl);
    	DBG(cerr << "besRH->get_name(): " << besRH->get_name() << endl);

    	//string item_name = get_cache_file_name(get_hash(cont.get_relative_name() + "dmr_r"), false);
    	string hash_name = d_mds->get_hash(cont.get_relative_name() + "dmrpp_r");
    	DBG(cerr << "hash_name: " << hash_name << endl);

    	string item_name = d_mds->get_cache_file_name(hash_name, false /*mangle*/);
    	DBG(cerr << "item_name: " << item_name << endl);

    	try {
    		int fd = open(item_name.c_str(), O_RDWR | O_CREAT, 00664 /*mode = rw rw r*/);
    		CPPUNIT_ASSERT(fd != -1);

    		sleep(3);

    		int gd = open(real_name.c_str(), O_RDWR | O_CREAT, 00664 /*mode = rw rw r*/);
    		CPPUNIT_ASSERT(gd != -1);

    		GlobalMetadataStore::MDSReadLock lock = d_mds->is_dmrpp_available(cont);
    		// since the cache file is newer than the 'real' file, we should have the lock
    		CPPUNIT_ASSERT(!lock());
    	}
    	catch (BESError &e) {
    		unlink(item_name.c_str());
    		unlink(real_name.c_str());
    		ostringstream oss;
    		oss << "Error: " << e.get_message() << " " << e.get_file() << ":" << e.get_line();
    		CPPUNIT_FAIL(oss.str());
    	}
    	catch (...) {
    		unlink(item_name.c_str());
    		unlink(real_name.c_str());
    		throw;
    	}

    	// TODO The clean_cache_dir() function hangs if this file is left in the cache
    	unlink(item_name.c_str());
    	unlink(real_name.c_str());
    }//end is_dmrpp_available_test_3()


#if 0
    void cache_a_dmrpp_response()
    {
        DBG(cerr << __func__ << " - BEGIN" << endl);

        try {
            init_dmrpp_and_mds();

            // Store it - this will work if the the code is cleaning the cache.
            GlobalMetadataStore::StreamDMRpp write_the_dmrpp_response(d_test_dmr);
            bool stored = d_mds->store_dap_response(write_the_dmrpp_response, d_test_dmr->name() + ".dmrpp_r", d_test_dmr->name(), "DMRpp");

            CPPUNIT_ASSERT(stored);

            // Now check the file
            string baseline_name = c_mds_baselines + "/" + c_mds_prefix + "chunked_fourD.h5.dmrpp_r";
            DBG(cerr << "Reading baseline: " << baseline_name << endl);
            CPPUNIT_ASSERT(access(baseline_name.c_str(), R_OK) == 0);

            string chunked_4d_dmrpp_baseline = read_test_baseline(baseline_name);

            string response_name = d_mds_dir + "/" + c_mds_prefix + "chunked_fourD.h5.dmrpp_r";
            DBG(cerr << "Reading response: " << response_name << endl);
            CPPUNIT_ASSERT(access(response_name.c_str(), R_OK) == 0);

            string stored_response = read_test_baseline(response_name);

            CPPUNIT_ASSERT(stored_response == chunked_4d_dmrpp_baseline);
        }
        catch (BESError &e) {
            CPPUNIT_FAIL(e.get_message());
        }

        DBG(cerr << __func__ << " - END" << endl);
    }
#endif

    void add_response_test()
     {
         DBG(cerr << __func__ << " - BEGIN" << endl);

         try {
             init_dds_and_mds();

             // Store it - this will work if the the code is cleaning the cache.
             bool stored = d_mds->add_responses(d_test_dds, d_test_dds->get_dataset_name());

             CPPUNIT_ASSERT(stored);

             // look for the files
             string dds_cache_name = d_mds->get_cache_file_name(d_mds->get_hash(d_test_dds->get_dataset_name().append("dds_r")), false /*mangle*/);
             DBG(cerr << __func__ << " - dds_cache_name: " << dds_cache_name << endl);
             CPPUNIT_ASSERT(access(dds_cache_name.c_str(), R_OK) == 0);

             string das_cache_name = d_mds->get_cache_file_name(d_mds->get_hash(d_test_dds->get_dataset_name().append("das_r")), false /*mangle*/);
             DBG(cerr << __func__ << " - das_cache_name: " << das_cache_name << endl);
             CPPUNIT_ASSERT(access(das_cache_name.c_str(), R_OK) == 0);

             /// Previously the MDS built all three metadata responses using
             /// either the DDS or DMR. Now it only does that when SYMETRIC_ADD_RESPONSES
             /// is defined. When that is not defined (the default) the DDS
             /// builds only the DAP2 responses and the DMR builds only the
             /// DAP4 responses. jhrg 3/20/18
#if SYMETRIC_ADD_RESPONSES
             string dmr_cache_name = d_mds->get_cache_file_name(d_mds->get_hash(d_test_dds->get_dataset_name().append("dmr_r")), false /*mangle*/);
             DBG(cerr << __func__ << " - dmr_cache_name: " << das_cache_name << endl);
             CPPUNIT_ASSERT(access(dmr_cache_name.c_str(), R_OK) == 0);
#endif
         }
         catch (BESError &e) {
             CPPUNIT_FAIL(e.get_message());
         }

         DBG(cerr << __func__ << " - END" << endl);
     }

    void get_dds_response_test() {
        DBG(cerr << __func__ << " - BEGIN" << endl);

         try {
             init_dds_and_mds();

             // Store it - this will work if the the code is cleaning the cache.
             bool stored = d_mds->add_responses(d_test_dds, d_test_dds->get_dataset_name());

             CPPUNIT_ASSERT(stored);

             // Now lets read the object from the cache
             ostringstream oss;
             d_mds->write_dds_response(d_test_dds->get_dataset_name(), oss);
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
             init_dds_and_mds();

             // Store it - this will work if the the code is cleaning the cache.
             bool stored = d_mds->add_responses(d_test_dds, d_test_dds->get_dataset_name());

             CPPUNIT_ASSERT(stored);

             // Now lets read the object from the cache
             ostringstream oss;
             d_mds->write_das_response(d_test_dds->get_dataset_name(), oss);
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

    void write_dmr_response_test() {
        DBG(cerr << __func__ << " - BEGIN" << endl);

         try {
             init_dmr_and_mds();

             // Store it - this will work if the the code is cleaning the cache.
             bool stored = d_mds->add_responses(d_test_dmr, d_test_dmr->name());

             CPPUNIT_ASSERT(stored);

             // Now lets read the object from the cache
             ostringstream oss;
             d_mds->write_dmr_response(d_test_dmr->name(), oss);
             DBG(cerr << "DMR response: " << endl << oss.str() << endl);

             string baseline_name = c_mds_baselines + "/" + c_mds_prefix + "test_01.dmr_r";
             DBG(cerr << "Reading baseline: " << baseline_name << endl);
             CPPUNIT_ASSERT(access(baseline_name.c_str(), R_OK) == 0);

             string test_05_dmr_baseline = read_test_baseline(baseline_name);

             CPPUNIT_ASSERT(test_05_dmr_baseline == oss.str());
         }
         catch (BESError &e) {
             CPPUNIT_FAIL(e.get_message());
         }

         DBG(cerr << __func__ << " - END" << endl);
    }

    void write_dmr_response_test_error()
    {
        DBG(cerr << __func__ << " - BEGIN" << endl);

        init_dmr_and_mds();

        // Store it - this will work if the the code is cleaning the cache.
        bool stored = d_mds->add_responses(d_test_dmr, d_test_dmr->name());

        CPPUNIT_ASSERT(stored);

        // Now write_dmr_response() will throw
        BESContextManager::TheManager()->unset_context("xml:base");

        // Now lets read the object from the cache
        ostringstream oss;
        d_mds->write_dmr_response(d_test_dmr->name(), oss);
        DBG(cerr << "DMR response: " << endl << oss.str() << endl);

        CPPUNIT_FAIL("Should have thrown an error.");

        DBG(cerr << __func__ << " - END" << endl);
     }

#if SYMETRIC_ADD_RESPONSES
    void get_dmr_response_test_2() {
        DBG(cerr << __func__ << " - BEGIN" << endl);

         try {
             init_dds_and_mds();

             // Store it - this will work if the the code is cleaning the cache.
             bool stored = d_mds->add_responses(d_test_dds, d_test_dds->get_dataset_name());

             CPPUNIT_ASSERT(stored);

             // Now lets read the object from the cache
             ostringstream oss;
             d_mds->write_dmr_response(d_test_dds->get_dataset_name(), oss);
             DBG(cerr << "DMR response: " << endl << oss.str() << endl);

             string baseline_name = c_mds_baselines + "/" + c_mds_prefix + "SimpleTypes.dmr_r";
             DBG(cerr << "Reading baseline: " << baseline_name << endl);
             CPPUNIT_ASSERT(access(baseline_name.c_str(), R_OK) == 0);

             string test_05_dmr_baseline = read_test_baseline(baseline_name);

             CPPUNIT_ASSERT(test_05_dmr_baseline == oss.str());
         }
         catch (BESError &e) {
             CPPUNIT_FAIL(e.get_message());
         }

         DBG(cerr << __func__ << " - END" << endl);
    }
#endif

    void remove_object_test()
    {
        DBG(cerr << __func__ << " - BEGIN" << endl);

        try {
            init_dds_and_mds();

            // Store it - this will work if the the code is cleaning the cache.
            bool stored = d_mds->add_responses(d_test_dds, d_test_dds->get_dataset_name());

            CPPUNIT_ASSERT(stored);

            // look for the files
            string dds_cache_name = d_mds->get_cache_file_name(d_mds->get_hash(d_test_dds->get_dataset_name().append("dds_r")), false /*mangle*/);
            DBG(cerr << __func__ << " - dds_cache_name: " << dds_cache_name << endl);
            CPPUNIT_ASSERT(access(dds_cache_name.c_str(), R_OK) == 0);

            string das_cache_name = d_mds->get_cache_file_name(d_mds->get_hash(d_test_dds->get_dataset_name().append("das_r")), false /*mangle*/);
            DBG(cerr << __func__ << " - das_cache_name: " << das_cache_name << endl);
            CPPUNIT_ASSERT(access(das_cache_name.c_str(), R_OK) == 0);

#if SYMETRIC_ADD_RESPONSES
            string dmr_cache_name = d_mds->get_cache_file_name(d_mds->get_hash(d_test_dds->get_dataset_name().append("dmr_r")), false /*mangle*/);
            DBG(cerr << __func__ << " - dmr_cache_name: " << dmr_cache_name << endl);
            CPPUNIT_ASSERT(access(dmr_cache_name.c_str(), R_OK) == 0);
#endif

            bool removed = d_mds->remove_responses(d_test_dds->get_dataset_name());
            CPPUNIT_ASSERT(removed);

            CPPUNIT_ASSERT(access(dds_cache_name.c_str(), R_OK) != 0);
            CPPUNIT_ASSERT(access(das_cache_name.c_str(), R_OK) != 0);
#if SYMETRIC_ADD_RESPONSES
            CPPUNIT_ASSERT(access(dmr_cache_name.c_str(), R_OK) != 0);
#endif
        }
        catch (BESError &e) {
            CPPUNIT_FAIL(e.get_message());
        }

        DBG(cerr << __func__ << " - END" << endl);
    }

    // These test duplicate many of the above, but use DMRs in place of DDSs
    void cache_a_dds_response_dmr()
    {
        DBG(cerr << __func__ << " - BEGIN" << endl);

        try {
            init_dmr_and_mds();

            // Store it - this will work if the the code is cleaning the cache.
            GlobalMetadataStore::StreamDDS write_the_dds_response(d_test_dmr);
            bool stored = d_mds->store_dap_response(write_the_dds_response,
                d_test_dmr->name() + ".dds_r", d_test_dmr->name(), "DDS");

            CPPUNIT_ASSERT(stored);

            // Now check the file
            string baseline_name = c_mds_baselines + "/" + c_mds_prefix + "test_array_4.dds_r";
            DBG(cerr << "Reading baseline: " << baseline_name << endl);
            CPPUNIT_ASSERT(access(baseline_name.c_str(), R_OK) == 0);

            string test_01_dmr_baseline = read_test_baseline(baseline_name);

            string response_name = d_mds_dir + "/" + c_mds_prefix + "test_array_4.dds_r";
            // read_test_baseline() just reads stuff from a file - it will work for the response, too.
            DBG(cerr << "Reading response: " << response_name << endl);
            CPPUNIT_ASSERT(access(response_name.c_str(), R_OK) == 0);

            string stored_response = read_test_baseline(response_name);

            CPPUNIT_ASSERT(stored_response == test_01_dmr_baseline);
        }
        catch (BESError &e) {
            CPPUNIT_FAIL(e.get_message());
        }

        DBG(cerr << __func__ << " - END" << endl);
    }

    void cache_a_das_response_dmr()
    {
        DBG(cerr << __func__ << " - BEGIN" << endl);

        try {
            init_dmr_and_mds();

            // Store it - this will work if the the code is cleaning the cache.
            GlobalMetadataStore::StreamDAS write_the_das_response(d_test_dmr);
            bool stored = d_mds->store_dap_response(write_the_das_response,
                d_test_dmr->name() + ".das_r", d_test_dmr->name(), "DAS");

             CPPUNIT_ASSERT(stored);

            // Now check the file
            string baseline_name = c_mds_baselines + "/" + c_mds_prefix + "test_array_4.das_r";
            DBG(cerr << "Reading baseline: " << baseline_name << endl);
            CPPUNIT_ASSERT(access(baseline_name.c_str(), R_OK) == 0);

            string test_01_dmr_baseline = read_test_baseline(baseline_name);

            string response_name = d_mds_dir + "/" + c_mds_prefix + "test_array_4.das_r";
            // read_test_baseline() just reads stuff from a file - it will work for the response, too.
            DBG(cerr << "Reading response: " << response_name << endl);
            CPPUNIT_ASSERT(access(response_name.c_str(), R_OK) == 0);

            string stored_response = read_test_baseline(response_name);

            CPPUNIT_ASSERT(stored_response == test_01_dmr_baseline);
        }
        catch (BESError &e) {
            CPPUNIT_FAIL(e.get_message());
        }

        DBG(cerr << __func__ << " - END" << endl);
    }

    void cache_a_dmr_response_dmr()
    {
        DBG(cerr << __func__ << " - BEGIN" << endl);

        try {
            init_dmr_and_mds();

            // Store it - this will work if the the code is cleaning the cache.
            GlobalMetadataStore::StreamDMR write_the_dmr_response(d_test_dmr);
            bool stored = d_mds->store_dap_response(write_the_dmr_response,
                d_test_dmr->name() + ".dmr_r", d_test_dmr->name(), "DMR");

            CPPUNIT_ASSERT(stored);

            // Now check the file
            string baseline_name = c_mds_baselines + "/" + c_mds_prefix + "test_array_4.dmr_r";
            DBG(cerr << "Reading baseline: " << baseline_name << endl);
            CPPUNIT_ASSERT(access(baseline_name.c_str(), R_OK) == 0);

            string test_01_dmr_baseline = read_test_baseline(baseline_name);

            string response_name = d_mds_dir + "/" + c_mds_prefix + "test_array_4.dmr_r";
            // read_test_baseline() just reads stuff from a file - it will work for the response, too.
            DBG(cerr << "Reading response: " << response_name << endl);
            CPPUNIT_ASSERT(access(response_name.c_str(), R_OK) == 0);

            string stored_response = read_test_baseline(response_name);

            CPPUNIT_ASSERT(stored_response == test_01_dmr_baseline);
        }
        catch (BESError &e) {
            CPPUNIT_FAIL(e.get_message());
        }

        DBG(cerr << __func__ << " - END" << endl);
    }

    void add_response_dmr_test()
     {
         DBG(cerr << __func__ << " - BEGIN" << endl);

         try {
             init_dmr_and_mds();

             // Store it - this will work if the the code is cleaning the cache.
             bool stored = d_mds->add_responses(d_test_dmr, d_test_dmr->name());

             CPPUNIT_ASSERT(stored);

#if SYMETRIC_ADD_RESPONSES
             // look for the files
             string dds_cache_name = d_mds->get_cache_file_name(d_mds->get_hash(d_test_dmr->name().append("dds_r")), false /*mangle*/);
             DBG(cerr << __func__ << " - dds_cache_name: " << dds_cache_name << endl);
             CPPUNIT_ASSERT(access(dds_cache_name.c_str(), R_OK) == 0);

             string das_cache_name = d_mds->get_cache_file_name(d_mds->get_hash(d_test_dmr->name().append("das_r")), false /*mangle*/);
             DBG(cerr << __func__ << " - das_cache_name: " << das_cache_name << endl);
             CPPUNIT_ASSERT(access(das_cache_name.c_str(), R_OK) == 0);
#endif
             string dmr_cache_name = d_mds->get_cache_file_name(d_mds->get_hash(d_test_dmr->name().append("dmr_r")), false /*mangle*/);
             DBG(cerr << __func__ << " - dmr_cache_name: " << dmr_cache_name << endl);
             CPPUNIT_ASSERT(access(dmr_cache_name.c_str(), R_OK) == 0);
         }
         catch (BESError &e) {
             CPPUNIT_FAIL(e.get_message());
         }

         DBG(cerr << __func__ << " - END" << endl);
     }

    void get_dds_object_test() {
        DBG(cerr << __func__ << " - BEGIN" << endl);

        try {
            init_dds_and_mds();

            // Store it - this will work if the the code is cleaning the cache.
            bool stored = d_mds->add_responses(d_test_dds, d_test_dds->get_dataset_name());

            CPPUNIT_ASSERT(stored);

            DDS *dds = d_mds->get_dds_object(d_test_dds->get_dataset_name());

            CPPUNIT_ASSERT(dds);

            DBG(cerr << "DDS: " << dds->get_dataset_name() << endl);

            ostringstream oss;
            dds->print_xml(oss, false);

            string baseline_name = c_mds_baselines + "/" + c_mds_prefix + "SimpleTypes2.ddx";
            DBG(cerr << "Reading baseline: " << baseline_name << endl);
            CPPUNIT_ASSERT(access(baseline_name.c_str(), R_OK) == 0);

            string SimpleTypes_ddx_baseline = read_test_baseline(baseline_name);

            CPPUNIT_ASSERT(SimpleTypes_ddx_baseline == oss.str());

        }
        catch (BESError &e) {
            CPPUNIT_FAIL(e.get_message());
        }
        catch(Error &e) {
            CPPUNIT_FAIL(e.get_error_message());
        }
        catch (std::exception &e) {
            CPPUNIT_FAIL(e.what());
        }

        DBG(cerr << __func__ << " - END" << endl);
    }

    void get_dmr_object_test()
    {
        DBG(cerr << __func__ << " - BEGIN" << endl);

        try {
            init_dmr_and_mds();

            // Store it - this will work if the the code is cleaning the cache.
            bool stored = d_mds->add_responses(d_test_dmr, d_test_dmr->name());

            CPPUNIT_ASSERT(stored);

            DMR *dmr = d_mds->get_dmr_object(d_test_dmr->name());

            CPPUNIT_ASSERT(dmr);

            DBG(cerr << "DMR: " << dmr->name() << endl);

            ostringstream oss;
            XMLWriter writer;
            dmr->print_dap4(writer);
            oss << writer.get_doc();

            string baseline_name = c_mds_baselines + "/" + c_mds_prefix + "test_01.dmr_r";
            DBG(cerr << "Reading baseline: " << baseline_name << endl);
            CPPUNIT_ASSERT(access(baseline_name.c_str(), R_OK) == 0);

            string test_01_dmr_baseline = read_test_baseline(baseline_name);

            CPPUNIT_ASSERT(test_01_dmr_baseline == oss.str());
        }
        catch (BESError &e) {
            CPPUNIT_FAIL(e.get_message());
        }
        catch (Error &e) {
            CPPUNIT_FAIL(e.get_error_message());
        }
        catch (std::exception &e) {
            CPPUNIT_FAIL(e.what());
        }

        DBG(cerr << __func__ << " - END" << endl);
    }

    // (int fd, ostream &os, const string &xml_base)
    void insert_xml_base_test() {
        string source_file = string(TEST_SRC_DIR) + "/input-files/insert_xml_base_src.txt";
        DBG(cerr << __func__ << " Input file: " << source_file << endl);

        int fd = open(source_file.c_str(), O_RDONLY);

        CPPUNIT_ASSERT(fd > 0);

        ostringstream oss;
        GlobalMetadataStore::insert_xml_base(fd, oss, "URI");

        DBG(cerr << __func__ << " Result: " << oss.str() << endl);

        string baseline_name = string(TEST_SRC_DIR) + "/input-files/insert_xml_base_baseline.txt";
        DBG(cerr << "Reading baseline: " << baseline_name << endl);
        CPPUNIT_ASSERT(access(baseline_name.c_str(), R_OK) == 0);

        string insert_xml_base_baseline = read_test_baseline(baseline_name);

        CPPUNIT_ASSERT(insert_xml_base_baseline == oss.str());
    }

    void insert_xml_base_test_2() {
         string source_file = string(TEST_SRC_DIR) + "/input-files/insert_xml_base_src2.txt";
         DBG(cerr << __func__ << " Input file: " << source_file << endl);

         int fd = open(source_file.c_str(), O_RDONLY);

         CPPUNIT_ASSERT(fd > 0);

         ostringstream oss;
         GlobalMetadataStore::insert_xml_base(fd, oss, "URI-2");

         DBG(cerr << __func__ << " Result: " << oss.str() << endl);

         string baseline_name = string(TEST_SRC_DIR) + "/input-files/insert_xml_base_baseline2.txt";
         DBG(cerr << "Reading baseline: " << baseline_name << endl);
         CPPUNIT_ASSERT(access(baseline_name.c_str(), R_OK) == 0);

         string insert_xml_base_baseline = read_test_baseline(baseline_name);

         CPPUNIT_ASSERT(insert_xml_base_baseline == oss.str());
     }

    // This tests a real file and one that is bigger than the character buffer, so
    // it will trigger the bug where a file larger than the buffer causes the xml:base
    // attribute to appear several times. jhrg 6/11/18
    void insert_xml_base_test_3() {
         string source_file = string(TEST_SRC_DIR) + "/input-files/chunked_shuffled_fourD.h5.dmrpp";
         DBG(cerr << __func__ << " Input file: " << source_file << endl);

         int fd = open(source_file.c_str(), O_RDONLY);

         CPPUNIT_ASSERT(fd > 0);

         ostringstream oss;
         GlobalMetadataStore::insert_xml_base(fd, oss, "URI-3");

         DBG(cerr << __func__ << " Result: " << oss.str() << endl);

         string baseline_name = string(TEST_SRC_DIR) + "/input-files/chunked_shuffled_fourD.h5.dmrpp.baseline";
         DBG(cerr << "Reading baseline: " << baseline_name << endl);
         CPPUNIT_ASSERT(access(baseline_name.c_str(), R_OK) == 0);

         // Strip out dmr version
         std::regex dmr_version_regex("dmrVersion=\"[0-9]+\.[0-9]+\"");
         auto stripped_input = std::regex_replace(oss.str(), dmr_version_regex, "dmrVersion=\"removed\"");

         string insert_xml_base_baseline = read_test_baseline(baseline_name);
         CPPUNIT_ASSERT_MESSAGE("The baseline " + insert_xml_base_baseline + " did not match the value " + stripped_input, insert_xml_base_baseline == stripped_input);
     }

    void insert_xml_base_test_error() {
        string source_file = string(TEST_SRC_DIR) + "/no_such_file";
        DBG(cerr << __func__ << " Input file: " << source_file << endl);

        int fd = open(source_file.c_str(), O_RDONLY);

        ostringstream oss;
        // This should through BESInternalError
        GlobalMetadataStore::insert_xml_base(fd, oss, "URI");

        CPPUNIT_FAIL("Expected GlobalMetadataStore::insert_xml_base to throw BESInternalError");
    }

    CPPUNIT_TEST_SUITE( GlobalMetadataStoreTest );

    CPPUNIT_TEST(ctor_test_1);
    CPPUNIT_TEST(ctor_test_2);
    CPPUNIT_TEST(ctor_test_3);

    CPPUNIT_TEST(get_hash_test);
    CPPUNIT_TEST_EXCEPTION(get_hash_test_error, BESError);

    CPPUNIT_TEST(cache_a_dds_response);
    CPPUNIT_TEST(cache_a_das_response);
    CPPUNIT_TEST(cache_a_dmr_response);

    CPPUNIT_TEST(get_cache_lmt_test);

    CPPUNIT_TEST(is_available_helper_dmr_test_1);
    CPPUNIT_TEST(is_available_helper_dmr_test_2);

    CPPUNIT_TEST(is_available_helper_dds_test_1);
    CPPUNIT_TEST(is_available_helper_dds_test_2);

    CPPUNIT_TEST(is_available_helper_das_test_1);
    CPPUNIT_TEST(is_available_helper_das_test_2);

    CPPUNIT_TEST(is_available_helper_dmrpp_test_1);
    CPPUNIT_TEST(is_available_helper_dmrpp_test_2);

    CPPUNIT_TEST(is_dmr_available_test_1);
    CPPUNIT_TEST(is_dmr_available_test_2);
    CPPUNIT_TEST(is_dmr_available_test_3);
    CPPUNIT_TEST(is_dmr_available_test_4);
    CPPUNIT_TEST(is_dmr_available_test_5);

    CPPUNIT_TEST(is_dds_available_test_1);
    CPPUNIT_TEST(is_dds_available_test_2);
    CPPUNIT_TEST(is_dds_available_test_3);

    CPPUNIT_TEST(is_das_available_test_1);
    CPPUNIT_TEST(is_das_available_test_2);
    CPPUNIT_TEST(is_das_available_test_3);

    CPPUNIT_TEST(is_dmrpp_available_test_1);
    CPPUNIT_TEST(is_dmrpp_available_test_2);
    CPPUNIT_TEST(is_dmrpp_available_test_3);

    CPPUNIT_TEST(add_response_test);

    CPPUNIT_TEST(get_dds_response_test);
    CPPUNIT_TEST(get_das_response_test);
    CPPUNIT_TEST(write_dmr_response_test);

#ifndef XML_BASE_MISSING_MEANS_OMIT_ATTRIBUTE
    CPPUNIT_TEST_EXCEPTION(write_dmr_response_test_error, BESInternalError);
#endif

#if SYMETRIC_ADD_RESPONSES
    CPPUNIT_TEST(get_dmr_response_test_2);
#endif
    CPPUNIT_TEST(remove_object_test);

    CPPUNIT_TEST(cache_a_dds_response_dmr);
    CPPUNIT_TEST(cache_a_das_response_dmr);
    CPPUNIT_TEST(cache_a_dmr_response_dmr);
    CPPUNIT_TEST(add_response_dmr_test);

    CPPUNIT_TEST(get_dds_object_test);
    CPPUNIT_TEST(get_dmr_object_test);

    CPPUNIT_TEST(insert_xml_base_test);
    CPPUNIT_TEST(insert_xml_base_test_2);
    CPPUNIT_TEST(insert_xml_base_test_3);
    CPPUNIT_TEST_EXCEPTION(insert_xml_base_test_error, BESInternalError);

    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(GlobalMetadataStoreTest);

}

int main(int argc, char*argv[])
{
    int option_char;
    while ((option_char = getopt(argc, argv, "dbkh")) != -1)
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
            unsigned int prefix_len = bes::GlobalMetadataStoreTest::suite()->getName().append("::").size();
            for (std::vector<Test*>::const_iterator i = tests.begin(), e = tests.end(); i != e; ++i) {
                cerr << (*i)->getName().replace(0, prefix_len, "") << endl;
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
    }
    else {
        int i = 0;
        while (i < argc) {
            if (debug) cerr << "Running " << argv[i] << endl;
            test = bes::GlobalMetadataStoreTest::suite()->getName().append("::").append(argv[i]);
            wasSuccessful = wasSuccessful && runner.run(test);

            ++i;
        }
    }

    return wasSuccessful ? 0 : 1;
}
