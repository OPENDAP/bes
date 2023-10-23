// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of the BES component of the Hyrax Data Server.

// Copyright (c) 2023 OPeNDAP, Inc.
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

#include <memory>
#include <iostream>
#include <future>
#include <thread>
#include <algorithm>

#include <dirent.h>

#include "BESFileLockingCache.h"
#include "BESDebug.h"
#include "BESUtil.h"
#include "BESInternalError.h"

#include "test_config.h"

#include "modules/common/run_tests_cppunit.h"

using namespace std;

static const std::string CACHE_PREFIX = string("bes_cache");
static const std::string MATCH_PREFIX = string(CACHE_PREFIX) + string("#");
static const string TEST_CACHE_DIR = BESUtil::assemblePath(TEST_SRC_DIR, "cache"); // TODO SRC to BUILD? jhrg 6/26/23

#define prolog std::string("BESFileLockingCacheTest::").append(__func__).append("() - ")

// namespace http {

class BESFileLockingCacheTest : public CppUnit::TestFixture {
    string d_data_dir = TEST_DATA_DIR;  // TODO Never used jhrg 6/26/23
    string d_build_dir = TEST_BUILD_DIR;    // TODO Used only in the MT test jhrg 6/26/23

    static void purge_cache(const string &cache_dir, const string &cache_prefix) {
        if (!(cache_dir.empty() && cache_prefix.empty())) {
            DBG2(cerr << prolog << "Purging " << cache_dir << " of files with prefix: " << cache_prefix << endl);
            string sys_cmd = "mkdir -p " + cache_dir;
            (void)system(sys_cmd.c_str());

            sys_cmd = "";
            sys_cmd += "exec rm -rf " + BESUtil::assemblePath(cache_dir, cache_prefix) + "*";
            (void)system(sys_cmd.c_str());
            DBG2(cerr << prolog << "The cache has been purged." << endl);
        }
    }

    /**
     * @brief Set up the cache.
     *
     * Add a set of eight test files to the cache, with names that are easy to
     * work with and each with an access time two seconds later than the
     * preceding one.
     * @note The above comment about 2s is no longer true. jhrg 1/11/23
     *
     * @param cache_dir Directory that holds the cached files.
     */
    static void init_cache(const string &cache_dir) {
        DBG(cerr << __func__ << "() - BEGIN " << endl);

        string t_file = cache_dir + "/template.txt";
        for (int i = 1; i < 9; i++) {
            ostringstream s;
            s << BESUtil::assemblePath(cache_dir, CACHE_PREFIX) << "#usr#local#data#template0" << i << ".txt";

            string cmd = "cp " + t_file + " " + s.str();
            (void)system(cmd.c_str());

            // This is a hack. The cache should be in the build dir, not the src dir.
            // TODO FIX jhrg 6/26/23
            cmd = "chmod a+w " + s.str();
            (void)system(cmd.c_str());
        }

        DBG(cerr << __func__ << "() - END " << endl);
    }

    /// Print all the things in cache_dir that match match_prefix.
    static string show_cache(const string &cache_dir, const string &match_prefix) {
        map<string, string> contents;   // TODO Remove jhrg 6/26/23
        ostringstream oss;
        DIR *dip = opendir(cache_dir.c_str());
        CPPUNIT_ASSERT(dip);
        const struct dirent *dit = nullptr;
        while ((dit = readdir(dip)) != nullptr) {
            string dirEntry = dit->d_name;
            if (dirEntry.compare(0, match_prefix.size(), match_prefix) == 0) {
                oss << dirEntry << endl;
                contents[dirEntry] = dirEntry; // TODO Remove jhrg 6/26/23
            }
        }

        closedir(dip);
        return oss.str();
    }

    static void check_cache(const string &cache_dir, const string &should_be, unsigned int num_files) {
        DBG(cerr << __func__ << "() - BEGIN, should_be: " << should_be << ", num_files: " << num_files << endl);

        vector<string> contents;
        DIR *dip = opendir(cache_dir.c_str());
        CPPUNIT_ASSERT_MESSAGE("Could not open the cache directory", dip);
        const struct dirent *dit = nullptr;
        while ((dit = readdir(dip)) != nullptr) {
            string entry = dit->d_name;
            if (entry.find(MATCH_PREFIX) != string::npos) //(entry.compare(0, match_prefix.size(), match_prefix) == 0)
                contents.emplace_back(entry);
        }
        closedir(dip);

        CPPUNIT_ASSERT_MESSAGE("Number of entries did not match expected number", num_files == contents.size());

        auto the_entry = find(contents.begin(), contents.end(), should_be);
        CPPUNIT_ASSERT_MESSAGE("Could not find the expected entry in the cache", the_entry != contents.end());
    }

public:

    // Called once before everything gets tested
    BESFileLockingCacheTest() = default;

    // Called at the end of the test
    ~BESFileLockingCacheTest() override = default;

    // Called before each test
    void setUp() override {
        DBG(cerr << endl);
        DBG(cerr << "setUp() - BEGIN" << endl);

        init_cache(TEST_CACHE_DIR);

        DBG(cerr << "setUp() - END" << endl);
    }

    void tearDown() override {
        DBG(cerr << endl);
        DBG(cerr << "tearDown() - BEGIN" << endl);

        purge_cache(TEST_CACHE_DIR, CACHE_PREFIX);

        DBG(cerr << "tearDown() - END" << endl);
    }
    
/*##################################################################################################*/
/* TESTS BEGIN */

    void test_empty_cache_dir_name_cache_creation() {
        DBG(cerr << endl << __func__ << "() - BEGIN " << endl);
        try {
            // Should return a disabled cache if the dir is "";
            BESFileLockingCache cache("", CACHE_PREFIX, 0);
            DBG(cerr << __func__ << "() - Cache is " << (cache.cache_enabled() ? "en" : "dis") << "abled." << endl);
            CPPUNIT_ASSERT(!cache.cache_enabled());

        }
        catch (const BESError &e) {
            CPPUNIT_FAIL("Failed to create disabled cache. msg: " + e.get_verbose_message());
        }
        DBG(cerr << __func__ << "() - END " << endl);
    }

    void test_missing_cache_dir_cache_creation()
    {
        DBG(cerr << endl << __func__ << "() - BEGIN " << endl);
        //Only run this test if you don't have access (mainly for in case something is built in root)
        if (access("/", W_OK) != 0) {
            CPPUNIT_ASSERT_THROW_MESSAGE("Expected an exception - should not be able to create cache in root dir",
                                         BESFileLockingCache cache("/dummy", CACHE_PREFIX, 0),
                                         BESError);
        }
        DBG(cerr << __func__ << "() - END " << endl);
    }

    void test_empty_prefix_name_cache_creation()
    {
        DBG(cerr << endl << __func__ << "() - BEGIN " << endl);
        CPPUNIT_ASSERT_THROW_MESSAGE("Expected an exception - should not be able to to create cache with empty prefix",
                                     BESFileLockingCache cache(TEST_CACHE_DIR, "", 1),
                                     BESError);
        DBG(cerr << __func__ << "() - END " << endl);
    }

    void test_size_zero_cache_creation()
    {
        DBG(cerr << endl << __func__ << "() - BEGIN " << endl);
        try {
            BESFileLockingCache cache(TEST_CACHE_DIR, CACHE_PREFIX, 0);
            CPPUNIT_ASSERT("Created cache with 0 size - correct given new behavior for this class");
        }
        catch (const BESError &e) {
            DBG(cerr << __func__ << "() - Unable to create cache with 0 size. " << "That's good. msg: " << e.get_message() << endl);
            CPPUNIT_FAIL("Could not make cache with zero size - new behavior uses this as an unbounded cache - it should work");
        }
        DBG(cerr << __func__ << "() - END " << endl);
    }

    void test_good_cache_creation()
    {
        DBG(cerr << endl << __func__ << "() - BEGIN " << endl);
        try {
            BESFileLockingCache cache(TEST_CACHE_DIR, CACHE_PREFIX, 1);
            DBG(cerr << __func__ << "() - Cache is " << (cache.cache_enabled() ? "en" : "dis") << "abled." << endl);
            CPPUNIT_ASSERT(cache.cache_enabled());

        }
        catch (const BESError &e) {
            DBG(cerr << __func__ << "() - FAILED to create cache! msg: " << e.get_message() << endl);
            CPPUNIT_ASSERT(!"Failed to create cache");
        }
        DBG(cerr << __func__ << "() - END " << endl);
    }

    void test_check_cache_for_non_existent_compressed_file() {
        DBG(cerr << endl << __func__ << "() - BEGIN " << endl);

        int fd;
        BESFileLockingCache cache(TEST_CACHE_DIR, CACHE_PREFIX, 1);
        string no_such_file = BESUtil::assemblePath(TEST_CACHE_DIR, "dummy.nc.gz");
        DBG(cerr << __func__ << "() - Attempting to acquire a read lock on non-existent file." << endl);
        bool success = cache.get_read_lock(no_such_file, fd);
        DBG(cerr << __func__ << "() - cache.get_read_lock() returned " << (success ? "true" : "false") << endl);
        if (success) {
            DBG(cerr << __func__ << "() - OUCH! That shouldn't have worked! "
                                    "Releasing lock and closing file before we ASSERT... " << endl);
            CPPUNIT_FAIL("Able to acquire a read lock on non-existent file");
        }
        CPPUNIT_ASSERT(!success);

        DBG(cerr << __func__ << "() - END " << endl);
    }

    void test_find_existing_cached_file()
    {
        DBG(cerr << endl << __func__ << "() - BEGIN " << endl);

        BESFileLockingCache cache(TEST_CACHE_DIR, CACHE_PREFIX, 1);
        string file_name = "/usr/local/data/template01.txt";
        string cache_file_name = cache.get_cache_file_name(file_name);
        int fd;

        bool is_it = cache.get_read_lock(cache_file_name, fd);
        DBG(cerr << __func__ << "() - cache.get_read_lock() returned " << (is_it ? "true" : "false") << endl);
        CPPUNIT_ASSERT(is_it);
        if (is_it) {
            cache.unlock_and_close(cache_file_name);
        }

        DBG(cerr << __func__ << "() - END " << endl);
    }

    void test_cache_purge()
    {
        DBG(cerr << endl << __func__ << "() - BEGIN " << endl);

        string latest_file = "/usr/local/data/template01.txt";

        DBG(cerr << __func__ << "() - Cache Before update_and_purge():" << endl
                 << show_cache(TEST_CACHE_DIR, CACHE_PREFIX));

        try {
            BESFileLockingCache cache(TEST_CACHE_DIR, CACHE_PREFIX, 1);
            string latest_cache_file = cache.get_cache_file_name(latest_file);

            // Purge files but not latest_file/latest_cache_file.
            cache.update_and_purge(latest_cache_file);

            // I used a hard-coded string because 'latest_cache_file' contains
            // extra path information that won't be found by check_cache. jhrg 4/26/17
            check_cache(TEST_CACHE_DIR, "bes_cache#usr#local#data#template01.txt"/*latest_cache_file*/, 4);
        }
        catch (const BESError &e) {
            CPPUNIT_FAIL("purge failed: " + e.get_message());
        }

        DBG(cerr << __func__ << "() - Test purge (should not remove any files)" << endl);

        try {
            BESFileLockingCache cache(TEST_CACHE_DIR, CACHE_PREFIX, 1);
            string latest_cache_file = cache.get_cache_file_name(latest_file);
            cache.update_and_purge(latest_cache_file);
            check_cache(TEST_CACHE_DIR, "bes_cache#usr#local#data#template01.txt"/*latest_cache_file*/, 4);
        }
        catch (const BESError &e) {
            CPPUNIT_FAIL("purge failed: " + e.get_message());
        }

        DBG(cerr << __func__ << "() - END " << endl);
    }

    // Multi-threaded tests.
#if 0
    void test_lock_cache_write_mt() {
        DBG(cerr << endl << __func__ << "() - BEGIN " << endl);
        const int num_threads = 3;

        // All the threads should be able to get the cache_file_name
        BESFileLockingCache cache(TEST_CACHE_DIR, CACHE_PREFIX, 1);
        vector<future<bool>> lock_cache_write_futures;
        for (int i = 0; i < num_threads; ++i) {
            lock_cache_write_futures.push_back(async(launch::async, [&cache]() {
                cache.lock_cache_write();
                return true;
            }));
        }

        DBG(cerr << "I'm doing my own work!" << endl);
        std::this_thread::sleep_for(std::chrono::seconds(1));
        DBG(cerr << "I'm done with my own work!" << endl);

        DBG(cerr << "Start querying. Only one thread should be ready" << endl);
        vector<int> ready_indices;
        do {
            for (int i = 0; i < num_threads; ++i) {
                std::chrono::milliseconds span (100);
                if (lock_cache_write_futures[i].wait_for(span) == std::future_status::ready) {
                    ready_indices.push_back(i);
                }
            }
            // Only one thread should be ready at a time.
            DBG(cerr << "Threads ready: " << ready_indices.size() << endl);
            CPPUNIT_ASSERT_MESSAGE("At most one thread calling lock_write_cache() should return at any time",
                                   ready_indices.size() <= 1);

            for (auto index: ready_indices) {
                DBG(cerr << "Future " << index << " is ready\n");
                CPPUNIT_ASSERT_MESSAGE("The future should return true", lock_cache_write_futures[index].get());
            }
        } while (!ready_indices.empty());

        DBG(cerr << __func__ << "() - END " << endl);
    }

    // This test might be bogus. I'll leave it in for now. jhrg 6/26/23
    void test_find_existing_cached_file_mt()
    {
        DBG(cerr << endl << __func__ << "() - BEGIN " << endl);

        const int num_threads = 3;

        // All the threads should be able to get the cache_file_name
        BESFileLockingCache cache(TEST_CACHE_DIR, CACHE_PREFIX, 1);
        string file_name = "/usr/local/data/template01.txt";
        vector<future<string>> cache_file_names_futures;
        for (int i = 0; i < num_threads; ++i) {
            cache_file_names_futures.push_back(async(launch::async, [&cache, &file_name]() {
                return cache.get_cache_file_name(file_name);
            }));
        }

        DBG(cerr << "I'm doing my own work!" << endl);
        std::this_thread::sleep_for(std::chrono::seconds(1));
        DBG(cerr << "I'm done with my own work!" << endl);

        DBG(cerr << "Start querying" << endl);
        string cache_file_name; // used later; just take the last value
        for (int i = 0; i < num_threads; ++i) {
            cache_file_names_futures[i].wait();
            cache_file_name = cache_file_names_futures[i].get();
            DBG(cerr << __func__ << "() - cache_file_name[" << i << "] = " << cache_file_name << endl);
        }

        // ... but only one threads should be able to lock that cached file
        vector<future<bool>> lock_futures;
        for (int i = 0; i < num_threads; ++i) {
            lock_futures.push_back(async(launch::async, [&cache, &cache_file_name]() {
                int fd; // not used
                return cache.get_read_lock(cache_file_name, fd);
            }));
        }

        DBG(cerr << "I'm doing my own work!" << endl);
        std::this_thread::sleep_for(std::chrono::seconds(1));
        DBG(cerr << "I'm done with my own work!" << endl);

        DBG(cerr << "Start querying" << endl);
        int locked = 0;
        for (int i = 0; i < num_threads; ++i) {
            lock_futures[i].wait();
            if (lock_futures[i].get()) {
                locked++;
                DBG(cerr << __func__ << "() - cache_file_name[" << i << "] locked\n");
            }
            else {
                DBG(cerr << __func__ << "() - cache_file_name[" << i << "] NOT locked\n");
            }
        }

        DBG(cerr << __func__ << "Locks on entry: " << locked << endl);
        CPPUNIT_ASSERT(locked >= 1);
        if (locked >= 1) {
            cache.unlock_and_close(cache_file_name);
        }

        DBG(cerr << __func__ << "() - END " << endl);
    }
#endif
#if NEVER
    void get_cache_file_name_test_4_2() {
        DBG(cerr << endl);
        DBG(cerr << prolog << "BEGIN" << endl);

        auto cache = HttpCache::get_instance();
        string url = "http://www.opendap.org/opendap/data/file.nc.dap?dap4.ce=/stuff;/stuff/here;/stuff/here.there";

        vector<future<string>> futures;

        for (size_t i = 0; i < 3; ++i) {
            futures.emplace_back(std::async(std::launch::async,
                                            [cache](const string &url) {
                                                return cache->get_cache_file_name("bob", url);
                                            }, url));
        }

        DBG(cerr << "I'm doing my own work!" << endl);
        std::this_thread::sleep_for(std::chrono::seconds(1));
        DBG(cerr << "I'm done with my own work!" << endl);

        DBG(cerr << "Start querying" << endl);
        auto fn1 = futures[0].get();
        auto fn2 = futures[1].get();
        auto fn3 = futures[2].get();
        DBG(cerr << "Done querying" << endl);
        CPPUNIT_ASSERT_MESSAGE("Expected get_instance() to return the same string", fn1 == fn2);
        CPPUNIT_ASSERT_MESSAGE("Expected get_instance() to return the same string", fn2 == fn3);

        const string prefix = d_build_dir + "/cache/hut_";
        const string hash = "49fbd7c6653d2291399c8018cfebe34a98e368ad34485f641d52f282915d9d55#file.nc.dap"; // 'sha256 hash' of the url
        DBG(cerr << prolog << "file_name: " << fn1 << endl);
        CPPUNIT_ASSERT_MESSAGE("Expected file name to use when caching data", fn1 == prefix + "bob_" + hash);

        DBG(cerr << prolog << "END" << endl);
    }
#endif

/* TESTS END */
/*##################################################################################################*/

    CPPUNIT_TEST_SUITE(BESFileLockingCacheTest);

        CPPUNIT_TEST(test_empty_cache_dir_name_cache_creation);
        CPPUNIT_TEST(test_missing_cache_dir_cache_creation);
        CPPUNIT_TEST(test_empty_prefix_name_cache_creation);
        CPPUNIT_TEST(test_size_zero_cache_creation);
        CPPUNIT_TEST(test_good_cache_creation);
        CPPUNIT_TEST(test_check_cache_for_non_existent_compressed_file);
        CPPUNIT_TEST(test_find_existing_cached_file);
        CPPUNIT_TEST(test_cache_purge);

#if 0
        CPPUNIT_TEST(test_find_existing_cached_file_mt);
        CPPUNIT_TEST(test_lock_cache_write_mt);
#endif

    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(BESFileLockingCacheTest);

// } // namespace http

int main(int argc, char *argv[]) {
    return bes_run_tests</*http::*/BESFileLockingCacheTest>(argc, argv, "cerr,cache,cache-lock,cache-lock-status") ? 0 : 1;
}
