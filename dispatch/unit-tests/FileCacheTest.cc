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

#include "FileCache.h"
#include "BESUtil.h"
#include "BESDebug.h"

#include "test_config.h"

#include "modules/common/run_tests_cppunit.h"

using namespace std;

#define prolog std::string("FileCacheTest::").append(__func__).append("() - ")

class FileCacheTest : public CppUnit::TestFixture {
    std::string cache_dir = string(TEST_BUILD_DIR) + "/test_cache";

public:

    // Called once before everything gets tested
    FileCacheTest() = default;

    // Called at the end of the test
    ~FileCacheTest() override = default;

    // Called before each test
    void setUp() override {
        // Create the cache directory
        if (mkdir(cache_dir.c_str(), 0755) != 0) {
            if (errno != EEXIST) {
                DBG(cerr << prolog << "Failed to create cache directory: " << cache_dir << '\n');
                CPPUNIT_FAIL("Failed to create cache directory in setUp()");
            }
        }
    }

    void tearDown() override {
        // Remove the cache directory
        DIR *dir = nullptr;
        struct dirent *ent{0};
        if ((dir = opendir (cache_dir.c_str())) != nullptr) {
            /* print all the files and directories within directory */
            while ((ent = readdir (dir)) != nullptr) {
                // Skip the . and .. files and the cache info file
                if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
                    continue;
                string cache_file = BESUtil::pathConcat(cache_dir, ent->d_name);
                DBG(cerr << prolog << "Removing cache file: " << cache_file << '\n');
                if (remove(cache_file.c_str()) != 0) {
                    CPPUNIT_FAIL(string("Error removing ") + ent->d_name + " from cache directory (" + cache_dir + ")");
                }
            }
            closedir (dir);
            if (remove(cache_dir.c_str()) != 0) {
                CPPUNIT_FAIL("Error removing cache directory:" + cache_dir);
            }
        } else {
            CPPUNIT_FAIL("Failed to open cache directory in tearDown()");
        }
    }

    void test_unintialized_cache() {
        FileCache fc;
        CPPUNIT_ASSERT_MESSAGE("Cache invariant should be false - no cache file open", !fc.invariant());
    }

    void test_open_cache_info() {
        FileCache fc;
        CPPUNIT_ASSERT_MESSAGE("Cache invariant should be false - no cache file open", !fc.invariant());

        fc.d_cache_dir = cache_dir;
        CPPUNIT_ASSERT_MESSAGE("The cache file should be opened", fc.open_cache_info());

        string cache_info_filename = BESUtil::pathConcat(cache_dir, CACHE_INFO_FILE_NAME);
        DBG(cerr << prolog << "cache_info_filename: " << cache_info_filename << '\n');
        CPPUNIT_ASSERT_MESSAGE("The cache file should exist", access(cache_info_filename.c_str(), F_OK) == 0);

        struct stat cifsb{0};
        if (stat(cache_info_filename.c_str(), &cifsb) != 0)
            CPPUNIT_FAIL("stat() failed on cached file");
        CPPUNIT_ASSERT_MESSAGE("The cache file should be 8 bytes", cifsb.st_size == 8);
    }

    void test_open_cache_info_cache_dir_not_set() {
        FileCache fc;
        CPPUNIT_ASSERT_MESSAGE("Cache invariant should be false - no cache file open", !fc.invariant());

        CPPUNIT_ASSERT_MESSAGE("The cache file should not be opened", !fc.open_cache_info());
    }

    void test_get_cache_info_size_0() {
        FileCache fc;
        CPPUNIT_ASSERT_MESSAGE("Cache invariant should be false - no cache file open", !fc.invariant());

        fc.d_cache_dir = cache_dir;
        CPPUNIT_ASSERT_MESSAGE("The cache file should be opened", fc.open_cache_info());

        CPPUNIT_ASSERT_MESSAGE("Initially, the cache info size should be zero", fc.get_cache_info_size() == 0);
    }

    void test_intialized_cache() {
        FileCache fc;
        CPPUNIT_ASSERT_MESSAGE("Cache should initialize", fc.initialize("/tmp/", 100, 20));
        CPPUNIT_ASSERT_MESSAGE("Cache invariant should be true - cache initialized", fc.invariant());
        CPPUNIT_ASSERT_MESSAGE("d_cache_info_fd should not be -1 - cache initialized", fc.d_cache_info_fd != -1);
        CPPUNIT_ASSERT_MESSAGE("d_purge_size should be 20 - cache initialized", fc.d_purge_size == 20);
        CPPUNIT_ASSERT_MESSAGE("d_max_cache_size_in_bytes should be 100 - cache initialized", fc.d_max_cache_size_in_bytes == 100);
        CPPUNIT_ASSERT_MESSAGE("d_cache_dir should be \"/tmp/\" - cache initialized", fc.d_cache_dir == "/tmp/");
    }

    void test_put_single_file() {
        DBG(cerr << prolog << "cache dir: " << cache_dir << '\n');

        FileCache fc;
        CPPUNIT_ASSERT_MESSAGE("Cache should initialize", fc.initialize(cache_dir, 100, 20));
        string source_file = string(TEST_SRC_DIR) + "/cache/template.txt";

        bool status = fc.put("key1", source_file);
        struct stat sb{0};
        if (stat(source_file.c_str(), &sb) != 0)
            CPPUNIT_FAIL("stat() failed on cached file");

        CPPUNIT_ASSERT_MESSAGE("put() should return true", status);
        string raw_cached_file_path = cache_dir + "/key1";
        CPPUNIT_ASSERT_MESSAGE("Should see cached file", access(raw_cached_file_path.c_str(), F_OK) == 0);

        struct stat rcfp_b{0};
        if (stat(raw_cached_file_path.c_str(), &rcfp_b) != 0)
            CPPUNIT_FAIL("stat() failed on cached file");
        CPPUNIT_ASSERT_MESSAGE("Cached file should be the same size as the source file", sb.st_size == rcfp_b.st_size);
    }

    CPPUNIT_TEST_SUITE(FileCacheTest);

    CPPUNIT_TEST(test_unintialized_cache);
    CPPUNIT_TEST(test_open_cache_info);
    CPPUNIT_TEST(test_open_cache_info_cache_dir_not_set);
    CPPUNIT_TEST(test_get_cache_info_size_0);
    CPPUNIT_TEST(test_intialized_cache);
    CPPUNIT_TEST(test_put_single_file);

    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(FileCacheTest);

// } // namespace http

int main(int argc, char *argv[]) {
    return bes_run_tests<FileCacheTest>(argc, argv, "cerr,file-cache") ? 0 : 1;
}
