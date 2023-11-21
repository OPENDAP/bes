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

#include <thread>
#include <future>
#include <iomanip>
#include <chrono>

#include <algorithm>

#include <dirent.h>
#include <sys/wait.h>

#include <openssl/sha.h>

#include "FileCache.h"
#include "TheBESKeys.h"
#include "BESUtil.h"
#include "BESDebug.h"

#include "test_config.h"

#include "modules/common/run_tests_cppunit.h"

using namespace std;

#define prolog string("FileCacheTest::").append(__func__).append("() - ")

string sha256(const string &filename)
{
    fstream file(filename, ios::in);
    if (!file)
        CPPUNIT_FAIL("Tried to open a file to compute the SHA256 hash of its contents");

    SHA256_CTX sha256;
    SHA256_Init(&sha256);

    file.seekg (0, fstream::end);
    streamoff length = file.tellg();
    file.seekg (0, fstream::beg);

    vector<char> buffer(length);
    file.read(buffer.data(), length);

    SHA256_Update(&sha256, buffer.data(), length);
    vector<unsigned char> hash(SHA256_DIGEST_LENGTH);
    SHA256_Final(hash.data(), &sha256);

    stringstream ss;
    for(int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << hex << setw(2) << setfill('0') << (int)hash[i];
    }
    return ss.str();
}

class FileCacheTest : public CppUnit::TestFixture {
    std::string cache_dir = string(TEST_BUILD_DIR) + "/fc";

public:

    // Called once before everything gets tested
    FileCacheTest() = default;

    // Called at the end of the test
    ~FileCacheTest() override = default;

    // Called before each test
    void setUp() override
    {
        TheBESKeys::TheKeys()->set_key("BES.LogName", "./bes.log");
        TheBESKeys::TheKeys()->set_key("BES.LogVerbose", "yes");
        // Create the cache directory
        if (mkdir(cache_dir.c_str(), 0755) != 0 && errno != EEXIST) {
            DBG2(cerr << prolog << "Failed to create cache directory: " << cache_dir << '\n');
            CPPUNIT_FAIL("Failed to create cache directory in setUp()");
        }
    }

    void tearDown() override
    {
        // Remove the cache directory
        DIR *dir = nullptr;
        struct dirent *ent{nullptr};
        if ((dir = opendir(cache_dir.c_str())) != nullptr) {
            /* print all the files and directories within directory */
            while ((ent = readdir(dir)) != nullptr) {
                // Skip the . and .. files and the cache info file
                if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
                    continue;
                string cache_file = BESUtil::pathConcat(cache_dir, ent->d_name);
                DBG2(cerr << prolog << "Removing cache file: " << cache_file << '\n');
                if (remove(cache_file.c_str()) != 0) {
                    CPPUNIT_FAIL(string("Error removing ") + ent->d_name + " from cache directory (" + cache_dir + ")");
                }
            }
            closedir(dir);
            if (remove(cache_dir.c_str()) != 0) {
                CPPUNIT_FAIL("Error removing cache directory:" + cache_dir);
            }
        }
        else {
            CPPUNIT_FAIL("Failed to open cache directory in tearDown()");
        }
    }

    void test_hash_key() {
        const string str = "hello";
        const string hash_value = "2cf24dba5fb0a30e26e83b2ac5b9e29e1b161e5c1fa7425e73043362938b9824";

        truncate("./bes.log", 0);
        const string returned_value = FileCache::hash_key(str);
        DBG(cerr << prolog << "hash of " << str << " is " << returned_value << '\n');
        DBG(cerr << prolog << str << " should be " << hash_value << '\n');
        CPPUNIT_ASSERT_MESSAGE("Hash of " + str + " should be " + hash_value, returned_value == hash_value);
        // the hash value should not be logged
        int fd = open("./bes.log", O_RDONLY);
        CPPUNIT_ASSERT_MESSAGE("Info should not be logged", FileCache::get_file_size(fd) == 0);
        close(fd);
    }

    void test_hash_key_with_info_log() {
        const string str = "hello";
        const string hash_value = "2cf24dba5fb0a30e26e83b2ac5b9e29e1b161e5c1fa7425e73043362938b9824";

        truncate("./bes.log", 0);
        const string returned_value = FileCache::hash_key(str, true);
        DBG(cerr << prolog << "hash of " << str << " is " << returned_value << '\n');
        DBG(cerr << prolog << str << " should be " << hash_value << '\n');
        CPPUNIT_ASSERT_MESSAGE("Hash of " + str + " should be " + hash_value, returned_value == hash_value);
        int fd = open("./bes.log", O_RDONLY);
        CPPUNIT_ASSERT_MESSAGE("Info should be logged", FileCache::get_file_size(fd) > 0);
        close(fd);
    }

    // a cache that has not been initialized should fail the invariant test
    void test_unintialized_cache()
    {
        FileCache fc;
        CPPUNIT_ASSERT_MESSAGE("Cache invariant should be false - no cache file open", !fc.invariant());
    }

    void test_intialized_cache()
    {
        FileCache fc;
        CPPUNIT_ASSERT_MESSAGE("Cache should initialize", fc.initialize(cache_dir, 100, 20));
        CPPUNIT_ASSERT_MESSAGE("Cache invariant should be true - cache initialized", fc.invariant());
        CPPUNIT_ASSERT_MESSAGE("d_cache_info_fd should not be -1 - cache initialized", fc.d_cache_info_fd != -1);
        CPPUNIT_ASSERT_MESSAGE("d_purge_size should be 20 - cache initialized", fc.d_purge_size == 20);
        CPPUNIT_ASSERT_MESSAGE("d_max_cache_size_in_bytes should be 100 - cache initialized",
                               fc.d_max_cache_size_in_bytes == 100);
        CPPUNIT_ASSERT_MESSAGE("d_cache_dir should be \"/tmp/\" - cache initialized", fc.d_cache_dir == cache_dir);
    }

    void test_two_intialized_caches_same_directory()
    {
        FileCache fc;
        CPPUNIT_ASSERT_MESSAGE("Cache should initialize", fc.initialize(cache_dir, 100, 20));
        CPPUNIT_ASSERT_MESSAGE("Cache invariant should be true - cache initialized", fc.invariant());
        CPPUNIT_ASSERT_MESSAGE("d_cache_info_fd should not be -1 - cache initialized", fc.d_cache_info_fd != -1);
        CPPUNIT_ASSERT_MESSAGE("d_purge_size should be 20 - cache initialized", fc.d_purge_size == 20);
        CPPUNIT_ASSERT_MESSAGE("d_max_cache_size_in_bytes should be 100 - cache initialized",
                               fc.d_max_cache_size_in_bytes == 100);
        CPPUNIT_ASSERT_MESSAGE("d_cache_dir should be \"/tmp/\" - cache initialized", fc.d_cache_dir == cache_dir);

        FileCache fc2;
        CPPUNIT_ASSERT_MESSAGE("Cache should initialize", fc2.initialize(cache_dir, 100, 20));
        CPPUNIT_ASSERT_MESSAGE("Cache invariant should be true - cache initialized", fc2.invariant());
        CPPUNIT_ASSERT_MESSAGE("d_cache_info_fd should not be -1 - cache initialized", fc2.d_cache_info_fd != -1);
        CPPUNIT_ASSERT_MESSAGE("d_purge_size should be 20 - cache initialized", fc2.d_purge_size == 20);
        CPPUNIT_ASSERT_MESSAGE("d_max_cache_size_in_bytes should be 100 - cache initialized",
                               fc2.d_max_cache_size_in_bytes == 100);
        CPPUNIT_ASSERT_MESSAGE("d_cache_dir should be \"/tmp/\" - cache initialized", fc2.d_cache_dir == cache_dir);
    }

    // test the private open_cache_into() method
    void test_open_cache_info()
    {
        FileCache fc;
        CPPUNIT_ASSERT_MESSAGE("Cache invariant should be false - no cache file open", !fc.invariant());

        fc.d_cache_dir = cache_dir;
        CPPUNIT_ASSERT_MESSAGE("The cache file should be opened", fc.open_cache_info());

        string cache_info_filename = BESUtil::pathConcat(cache_dir, fc.CACHE_INFO_FILE_NAME);
        DBG(cerr << prolog << "cache_info_filename: " << cache_info_filename << '\n');
        CPPUNIT_ASSERT_MESSAGE("The cache file should exist", access(cache_info_filename.c_str(), F_OK) == 0);

        struct stat cifsb{0};
        if (stat(cache_info_filename.c_str(), &cifsb) != 0)
            CPPUNIT_FAIL("stat() failed on cached file");
        CPPUNIT_ASSERT_MESSAGE("The cache file should be 8 bytes", cifsb.st_size == 8);
    }

    void test_open_cache_info_cache_dir_not_set()
    {
        FileCache fc;
        CPPUNIT_ASSERT_MESSAGE("Cache invariant should be false - no cache file open", !fc.invariant());

        CPPUNIT_ASSERT_MESSAGE("The cache file should not be opened", !fc.open_cache_info());
    }

    void test_get_cache_info_size()
    {
        FileCache fc;
        CPPUNIT_ASSERT_MESSAGE("Cache invariant should be false - no cache file open", !fc.invariant());

        fc.d_cache_dir = cache_dir;
        CPPUNIT_ASSERT_MESSAGE("The cache file should be opened", fc.open_cache_info());

        CPPUNIT_ASSERT_MESSAGE("Initially, the cache info size should be zero", fc.get_cache_info_size() == 0);
    }

    void test_update_cache_info_size()
    {
        FileCache fc;
        CPPUNIT_ASSERT_MESSAGE("Cache invariant should be false - no cache file open", !fc.invariant());

        fc.d_cache_dir = cache_dir;
        CPPUNIT_ASSERT_MESSAGE("The cache file should be opened", fc.open_cache_info());

        CPPUNIT_ASSERT_MESSAGE("Initially, the cache info size should be zero", fc.get_cache_info_size() == 0);

        CPPUNIT_ASSERT_MESSAGE("The cache info size should be updated", fc.update_cache_info_size(10));
        CPPUNIT_ASSERT_MESSAGE("The cache info size should be 10", fc.get_cache_info_size() == 10);

        CPPUNIT_ASSERT_MESSAGE("The cache info size should be updated",
                               fc.update_cache_info_size(fc.get_cache_info_size() + 17));
        CPPUNIT_ASSERT_MESSAGE("The cache info size should be 10", fc.get_cache_info_size() == 27);

    }

    void test_put_single_file()
    {
        DBG(cerr << prolog << "cache dir: " << cache_dir << '\n');

        FileCache fc;
        CPPUNIT_ASSERT_MESSAGE("Cache should initialize", fc.initialize(cache_dir, 100, 20));
        string source_file = string(TEST_SRC_DIR) + "/cache/template.txt";

        bool status = fc.put("key1", source_file);
        CPPUNIT_ASSERT_MESSAGE("put() should return true", status);

        // cached file is really there
        string raw_cached_file_path = cache_dir + "/key1";
        CPPUNIT_ASSERT_MESSAGE("Should see cached file", access(raw_cached_file_path.c_str(), F_OK) == 0);

        // Now check the size of the cached file - same as the source file?
        struct stat sb{0};
        if (stat(source_file.c_str(), &sb) != 0)
            CPPUNIT_FAIL("stat() failed on cached file");
        struct stat rcfp_b{0};
        if (stat(raw_cached_file_path.c_str(), &rcfp_b) != 0)
            CPPUNIT_FAIL("stat() failed on cached file");
        CPPUNIT_ASSERT_MESSAGE("Cached file should be the same size as the source file", sb.st_size == rcfp_b.st_size);

        // Was cache_info_size updated correctly?
        CPPUNIT_ASSERT_MESSAGE("Cached info size should be the size of this one file",
                               fc.get_cache_info_size() == rcfp_b.st_size);

        CPPUNIT_ASSERT_MESSAGE("Cached and source file should have the same sha256 hash",
                               sha256(source_file) == sha256(raw_cached_file_path));
    }

    void test_put_two_files()
    {
        DBG(cerr << prolog << "cache dir: " << cache_dir << '\n');

        FileCache fc;
        CPPUNIT_ASSERT_MESSAGE("Cache should initialize", fc.initialize(cache_dir, 100, 20));
        string source_file = string(TEST_SRC_DIR) + "/cache/template.txt";

        bool status = fc.put("key1", source_file);
        CPPUNIT_ASSERT_MESSAGE("put() should return true", status);
        status = fc.put("key2", source_file);
        CPPUNIT_ASSERT_MESSAGE("put() should return true", status);

        // We cache the same source file twice, so the cache info file size should be 2 * source file size
        struct stat sb{0};
        if (stat(source_file.c_str(), &sb) != 0)
            CPPUNIT_FAIL("stat() failed on cached file");

        CPPUNIT_ASSERT_MESSAGE("Cached info size should be the size of these two files",
                               fc.get_cache_info_size() == sb.st_size * 2);
    }

    void test_put_duplicate_key()
    {
        DBG(cerr << prolog << "cache dir: " << cache_dir << '\n');

        FileCache fc;
        CPPUNIT_ASSERT_MESSAGE("Cache should initialize", fc.initialize(cache_dir, 100, 20));
        string source_file = string(TEST_SRC_DIR) + "/cache/template.txt";

        bool status = fc.put("key1", source_file);
        CPPUNIT_ASSERT_MESSAGE("first put() should return true", status);

        status = fc.put("key2", source_file);
        CPPUNIT_ASSERT_MESSAGE("second put() should return true", status);

        status = fc.put("key1", source_file);
        CPPUNIT_ASSERT_MESSAGE("third put() with duplicate key name should return false", !status);
    }

    void test_put_duplicate_key_two_processes()
    {
        DBG(cerr << prolog << "cache dir: " << cache_dir << '\n');

        FileCache fc;
        CPPUNIT_ASSERT_MESSAGE("Cache should initialize", fc.initialize(cache_dir, 100, 20));
        string source_file = string(TEST_SRC_DIR) + "/cache/template.txt";

        if (fork() == 0) {
            // child process
            // sleep here to give the parent a head start, not much use.
            bool status = fc.put("key1", source_file);
            DBG(cerr << prolog << "child process put() status: " << status << '\n');
            exit(status ? 0 : 1);   // exit with 0 if status is true, 1 if false (it's a process)
        }
        else {
            // parent process - generally faster. I used std::this_thread::sleep_for() but this is
            // not multi-threaded code. It's just a way to sleep for a short time.
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            bool status = fc.put("key1", source_file);
            DBG(cerr << prolog << "parent process put() status: " << status << '\n');
            int child_status;
            wait(&child_status);
            DBG(cerr << prolog << "child process exit status: " << WEXITSTATUS(child_status) << '\n');
            // exit status for a process: 0 is true, 1 is false
            bool xor_status = (status && WEXITSTATUS(child_status) != 0) || (!status && WEXITSTATUS(child_status) == 0);
            CPPUNIT_ASSERT_MESSAGE("one status should be true and one false", xor_status);
        }

        CPPUNIT_ASSERT_MESSAGE("Cached and source file should have the same sha256 hash",
                               sha256(source_file) == sha256(BESUtil::pathConcat(cache_dir, "key1")));
    }

    /// Used in three places. jhrg 11/7/23
    bool multiple_put_operations(FileCache &fc, const string &source_file) const {
        auto f1 = async(launch::async,
                        [source_file, &fc](const string &key) -> bool {
                            // give the other threads a head start
                            this_thread::sleep_for(chrono::milliseconds(1));
                            bool status = fc.put(key, source_file);
                            return status;
                        },
                        "key1");
        auto f2 = async(launch::async,
                        [source_file, &fc](const string &key) -> bool {
                            bool status = fc.put(key, source_file);
                            return status;
                        },
                        "key1");
        auto f3 = async(launch::async,
                        [source_file, &fc](const string &key) -> bool {
                            bool status = fc.put(key, source_file);
                            return status;
                        },
                        "key1");

        DBG(cerr << "Start querying\n");

        bool f1_status = f1.get();
        bool f2_status = f2.get();
        bool f3_status = f3.get();
        DBG(cerr << "f1_status: " << f1_status << '\n');
        DBG(cerr << "f2_status: " << f2_status << '\n');
        DBG(cerr << "f3_status: " << f3_status << '\n');
        bool xor_status = (f1_status && !f2_status && !f3_status) ||
                          (!f1_status && f2_status && !f3_status) ||
                          (!f1_status && !f2_status && f3_status);
        return xor_status;
    }

    void test_put_duplicate_key_three_threads()
    {
        FileCache fc;
        CPPUNIT_ASSERT_MESSAGE("Cache should initialize", fc.initialize(cache_dir, 100, 20));
        string source_file = string(TEST_SRC_DIR) + "/cache/template.txt";

        bool xor_status = multiple_put_operations(fc, source_file);

        CPPUNIT_ASSERT_MESSAGE("Only one status should be true", xor_status);

        CPPUNIT_ASSERT_MESSAGE("Cached and source file should have the same sha256 hash",
                               sha256(source_file) == sha256(BESUtil::pathConcat(cache_dir, "key1")));
    }

    void test_put_duplicate_key_two_processes_three_threads_each()
    {
        DBG(cerr << prolog << "cache dir: " << cache_dir << '\n');

        FileCache fc;
        CPPUNIT_ASSERT_MESSAGE("Cache should initialize", fc.initialize(cache_dir, 100, 20));
        string source_file = string(TEST_SRC_DIR) + "/cache/template.txt";

        if (fork() == 0) {
            // child process
            // sleep here to give the parent a head start, not much use.
            bool xor_status = multiple_put_operations(fc, source_file);
#if 0
            auto f1 = async(launch::async,
                            [source_file, &fc](const string &key) -> bool {
                                // give the other threads a head start
                                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                                bool status = fc.put(key, source_file);
                                return status;
                            },
                            "key1");
            auto f2 = async(launch::async,
                            [source_file, &fc](const string &key) -> bool {
                                bool status = fc.put(key, source_file);
                                return status;
                            },
                            "key1");
            auto f3 = async(launch::async,
                            [source_file, &fc](const string &key) -> bool {
                                bool status = fc.put(key, source_file);
                                return status;
                            },
                            "key1");

            DBG(cerr << "Start querying\n" );

            bool f1_status = f1.get();
            bool f2_status = f2.get();
            bool f3_status = f3.get();
            DBG(cerr << "f1_status: " << f1_status << '\n');
            DBG(cerr << "f2_status: " << f2_status << '\n');
            DBG(cerr << "f3_status: " << f3_status << '\n');
            bool xor_status = (f1_status && !f2_status && !f3_status) ||
                              (!f1_status && f2_status && !f3_status) ||
                              (!f1_status && !f2_status && f3_status);
#endif

            DBG(cerr << prolog << "child process put() status: " << xor_status << '\n');
            exit(xor_status ? 0 : 1);   // exit with 0 if status is true, 1 if false (it's a process)
        }
        else {
            // parent process - generally faster. I used std::this_thread::sleep_for() but this is
            // not multi-threaded code. It's just a way to sleep for a short time.
            std::this_thread::sleep_for(std::chrono::milliseconds(1));

            bool xor_status = multiple_put_operations(fc, source_file);
#if 0
            auto f1 = async(launch::async,
                            [source_file, &fc](const string &key) -> bool {
                                // give the other threads a head start
                                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                                bool status = fc.put(key, source_file);
                                return status;
                            },
                            "key1");
            auto f2 = async(launch::async,
                            [source_file, &fc](const string &key) -> bool {
                                bool status = fc.put(key, source_file);
                                return status;
                            },
                            "key1");
            auto f3 = async(launch::async,
                            [source_file, &fc](const string &key) -> bool {
                                bool status = fc.put(key, source_file);
                                return status;
                            },
                            "key1");

            DBG(cerr << "Start querying\n" );

            bool f1_status = f1.get();
            bool f2_status = f2.get();
            bool f3_status = f3.get();
            DBG(cerr << "f1_status: " << f1_status << '\n');
            DBG(cerr << "f2_status: " << f2_status << '\n');
            DBG(cerr << "f3_status: " << f3_status << '\n');
            bool xor_status = (f1_status && !f2_status && !f3_status) ||
                              (!f1_status && f2_status && !f3_status) ||
                              (!f1_status && !f2_status && f3_status);
#endif
            int child_status;
            wait(&child_status);
            DBG(cerr << prolog << "child process exit status: " << WEXITSTATUS(child_status) << '\n');
            // exit status for a process: 0 is true, 1 is false
            bool xor_parent_child_status = (xor_status && WEXITSTATUS(child_status) != 0) || (!xor_status && WEXITSTATUS(child_status) == 0);
            CPPUNIT_ASSERT_MESSAGE("one status should be true and one false", xor_parent_child_status);
        }
    }

    void test_put_fd_version_single_file()
    {
        DBG(cerr << prolog << "cache dir: " << cache_dir << '\n');

        FileCache fc;
        CPPUNIT_ASSERT_MESSAGE("Cache should initialize", fc.initialize(cache_dir, 100, 20));
        string source_file = string(TEST_SRC_DIR) + "/cache/template.txt";

        struct stat sb{0};  // used later...
        if (stat(source_file.c_str(), &sb) != 0)
            CPPUNIT_FAIL("stat() failed on cached file");

        {
            FileCache::PutItem item(fc);

            bool status = fc.put("key1", item);
            CPPUNIT_ASSERT_MESSAGE("put() should return true", status);

            // cached file is really there
            string raw_cached_file_path = cache_dir + "/key1";
            CPPUNIT_ASSERT_MESSAGE("Should see cached file", access(raw_cached_file_path.c_str(), F_OK) == 0);

            // Write stuff to the open, locked 'item.'
            int fd2;
            if ((fd2 = open(source_file.c_str(), O_RDONLY)) < 0) {
                CPPUNIT_FAIL("Failed to open the source data");
            }

            std::vector<char> buf(std::min(MEGABYTE, FileCache::get_file_size(fd2)));
            ssize_t n;
            while ((n = read(fd2, buf.data(), buf.size())) > 0) {
                if (write(item.get_fd(), buf.data(), n) != n) {
                    CPPUNIT_FAIL("Failed to write the source data");
                }
            }

            // Now check the size of the cached file - same as the source file?
            struct stat rcfp_b{0};
            if (stat(raw_cached_file_path.c_str(), &rcfp_b) != 0)
                CPPUNIT_FAIL("stat() failed on cached file");
            CPPUNIT_ASSERT_MESSAGE("Cached file should be the same size as the source file", sb.st_size == rcfp_b.st_size);

            CPPUNIT_ASSERT_MESSAGE("Cached and source file should have the same sha256 hash",
                                   sha256(source_file) == sha256(raw_cached_file_path));

            // Before the PutItem goes out of scope, get(key1) should fail because the
            // item has an exclusive lock; del() should fail, too.
            FileCache::Item get_item;
            CPPUNIT_ASSERT_MESSAGE("get() should fail since the file is still locked", !fc.get("key1", get_item));
            CPPUNIT_ASSERT_MESSAGE("del() should fail since the file is still locked", !fc.del("key1"));

            // The PutItem goes out of scope and the descriptor should unlock and the
            // cache_info should be updated.
        }

        // Was cache_info_size updated correctly?
        CPPUNIT_ASSERT_MESSAGE("Cached info size should be the size of this one file",
                               fc.get_cache_info_size() == sb.st_size);

        {
            FileCache::Item get_item;   // get_item unlocks the file on exit from the block
            CPPUNIT_ASSERT_MESSAGE("get() should work now since the file is not locked", fc.get("key1", get_item));
            CPPUNIT_ASSERT_MESSAGE("del() should not work since the file is locked, now by get()", !fc.del("key1"));
        }

        CPPUNIT_ASSERT_MESSAGE("del() should work since the file is not locked", fc.del("key1"));

        // Was cache_info_size updated correctly?
        CPPUNIT_ASSERT_MESSAGE("Cached info size should be the size of this one file",
                               fc.get_cache_info_size() == 0);
    }

    void test_get_a_file_not_cached() {
        DBG(cerr << prolog << "cache dir: " << cache_dir << '\n');

        FileCache fc;
        CPPUNIT_ASSERT_MESSAGE("Cache should initialize", fc.initialize(cache_dir, 100, 20));
        string source_file = string(TEST_SRC_DIR) + "/cache/template.txt";

        FileCache::Item item;
        bool status = fc.get("key1", item);
        CPPUNIT_ASSERT_MESSAGE("get() should return false", !status);
        CPPUNIT_ASSERT_MESSAGE("Item::get_fd() should return -1", item.get_fd() == -1);
      }

    void test_get_a_file() {
        DBG(cerr << prolog << "cache dir: " << cache_dir << '\n');

        FileCache fc;
        CPPUNIT_ASSERT_MESSAGE("Cache should initialize", fc.initialize(cache_dir, 100, 20));
        string source_file = string(TEST_SRC_DIR) + "/cache/template.txt";

        CPPUNIT_ASSERT_MESSAGE("put() should return true", fc.put("key1", source_file));

        FileCache::Item item;
        bool status = fc.get("key1", item);
        CPPUNIT_ASSERT_MESSAGE("get() should return true", status);
        CPPUNIT_ASSERT_MESSAGE("Item::get_fd() should not return -1", item.get_fd() != -1);
    }

    void test_get_a_file_two_processes() {
        DBG(cerr << prolog << "cache dir: " << cache_dir << '\n');

        FileCache fc;
        CPPUNIT_ASSERT_MESSAGE("Cache should initialize", fc.initialize(cache_dir, 100, 20));
        string source_file = string(TEST_SRC_DIR) + "/cache/template.txt";

        CPPUNIT_ASSERT_MESSAGE("put() should return true", fc.put("key1", source_file));

        if (fork() == 0) {
            // child process
            FileCache::Item item;
            bool status = fc.get("key1", item);
            DBG(cerr << prolog << "child process put() status: " << status << '\n');
            exit(status ? 0 : 1);   // exit with 0 if status is true, 1 if false (it's a process)
        }
        else {
            // parent process - generally faster. I used std::this_thread::sleep_for() but this is
            // not multi-threaded code. It's just a way to sleep for a short time.
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            FileCache::Item item;
            bool status = fc.get("key1", item);
            DBG(cerr << prolog << "parent process put() status: " << status << '\n');
            CPPUNIT_ASSERT_MESSAGE("get() in the parent should return true", status);

            int child_status;
            wait(&child_status);
            DBG(cerr << prolog << "child process exit status: " << WEXITSTATUS(child_status) << '\n');
            // exit status for a process: 0 is true, 1 is false
            CPPUNIT_ASSERT_MESSAGE("get() in the child should return true", WEXITSTATUS(child_status) == 0);
        }

        CPPUNIT_ASSERT_MESSAGE("Cached and source file should have the same sha256 hash",
                               sha256(source_file) == sha256(BESUtil::pathConcat(cache_dir, "key1")));

        FileCache::Item item;
        bool status = fc.get("key1", item);
        CPPUNIT_ASSERT_MESSAGE("get() should return false", status);
        CPPUNIT_ASSERT_MESSAGE("Item::get_fd() should not return -1", item.get_fd() != -1);
    }

    void test_get_duplicate_key_three_threads()
    {
        FileCache fc;
        CPPUNIT_ASSERT_MESSAGE("Cache should initialize", fc.initialize(cache_dir, 100, 20));
        string source_file = string(TEST_SRC_DIR) + "/cache/template.txt";

        CPPUNIT_ASSERT_MESSAGE("Cache put(key1) should work", fc.put("key1", source_file));

        auto f1 = async(launch::async,
                        [source_file, &fc](const string &key) -> bool {
                            FileCache::Item item;
                            bool status = fc.get(key, item);
                            return status;
                        },
                        "key1");
        auto f2 = async(launch::async,
                        [source_file, &fc](const string &key) -> bool {
                            FileCache::Item item;
                            bool status = fc.get(key, item);
                            return status;
                        },
                        "key1");
        auto f3 = async(launch::async,
                        [source_file, &fc](const string &key) -> bool {
                            FileCache::Item item;
                            bool status = fc.get(key, item);
                            return status;
                        },
                        "key1");

        DBG(cerr << "Start querying\n" );

        bool f1_status = f1.get();
        bool f2_status = f2.get();
        bool f3_status = f3.get();
        DBG(cerr << "f1_status: " << f1_status << '\n');
        DBG(cerr << "f2_status: " << f2_status << '\n');
        DBG(cerr << "f3_status: " << f3_status << '\n');

        CPPUNIT_ASSERT_MESSAGE("All three statuses should be true", f1_status && f2_status && f2_status);
    }

    // This tests the cases where the file is open/locked for 'get()' and when it has been closed
    // (i.e., unlocked).
    void test_get_key_in_thread_then_del()
    {
        FileCache fc;
        CPPUNIT_ASSERT_MESSAGE("Cache should initialize", fc.initialize(cache_dir, 100, 20));
        string source_file = string(TEST_SRC_DIR) + "/cache/template.txt";

        CPPUNIT_ASSERT_MESSAGE("Cache put(key1) should work", fc.put("key1", source_file));

        FileCache::Item item;

        auto f1 = async(launch::async,
                        [source_file, &fc, &item]() {
                            bool status = fc.get("key1", item);
                            return status;
                        });

        bool status = f1.get();
        CPPUNIT_ASSERT_MESSAGE("Cache get(key1, item) should work", status);
        CPPUNIT_ASSERT_MESSAGE("Cache del(key1) should not work here", !fc.del("key1"));

        close(item.get_fd());

        CPPUNIT_ASSERT_MESSAGE("Cache del(key1) should work here", fc.del("key1"));
    }

    void test_get_and_put_duplicate_key_two_processes()
    {
        DBG(cerr << prolog << "cache dir: " << cache_dir << '\n');

        string source_file = string(TEST_SRC_DIR) + "/cache/template.txt";
        struct stat sb{0};
        if (stat(source_file.c_str(), &sb) != 0)
            CPPUNIT_FAIL("stat() failed on test data file");
        size_t source_file_size = sb.st_size;

        if (fork() == 0) {
            // child process
            // sleep here to give the parent a head start, not much use.
            FileCache fc;
            CPPUNIT_ASSERT_MESSAGE("Cache should initialize", fc.initialize(cache_dir, 100, 20));

            bool status = fc.put("key1", source_file);
            DBG(cerr << prolog << "child process put() status: " << status << '\n');
            exit(status ? 0 : 1);   // exit with 0 if status is true, 1 if false (it's a process)
        }
        else {
            // parent process - generally faster. I used std::this_thread::sleep_for() but this is
            // not multi-threaded code. It's just a way to sleep for a short time.
            FileCache fc;
            CPPUNIT_ASSERT_MESSAGE("Cache should initialize", fc.initialize(cache_dir, 100, 20));

            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            FileCache::Item item;
            bool status = fc.get("key1", item);
            DBG(cerr << prolog << "parent process get() status: " << status << '\n');

            int child_status;
            wait(&child_status);
            DBG(cerr << prolog << "child process exit status: " << WEXITSTATUS(child_status) << '\n');
            // exit status for a process: 0 is true, 1 is false

            // Because we use blocking locks for access to the cache, if the get and put functions are
            // both called at the same time by two processes, the put() always works if the file does
            // not already exist (because the get always fails if it is run first).
            CPPUNIT_ASSERT_MESSAGE("Child process (which ran put()) should exit with success.",
                                   WEXITSTATUS(child_status) == 0);

            if (status) {
                struct stat cb{0};
                if (fstat(item.get_fd(), &cb) != 0)
                    CPPUNIT_FAIL("fstat() failed on cached file");

                DBG(cerr << prolog << "parent process get() status: " << status << '\n');
                CPPUNIT_ASSERT_MESSAGE("Cached info size should be the size of the source file (first get() call)",
                                       cb.st_size == source_file_size);
            }
            else {
                // if the get operation failed, it might have run into the put operation. Since we know the
                // put has finished, get should work now. And the put operation should have worked
                status = fc.get("key1", item);
                DBG(cerr << prolog << "parent process second call to get() status: " << status << '\n');
                CPPUNIT_ASSERT_MESSAGE("The second call to get() should work.", status);
                struct stat cb{0};
                if (fstat(item.get_fd(), &cb) != 0)
                    CPPUNIT_FAIL("fstat() failed on cached file");

                CPPUNIT_ASSERT_MESSAGE("Cached info size should be the size of the source file (second get() call)",
                                       cb.st_size == source_file_size);
            }
        }

        CPPUNIT_ASSERT_MESSAGE("Cached and source file should have the same sha256 hash",
                               sha256(source_file) == sha256(BESUtil::pathConcat(cache_dir, "key1")));
    }

    void test_put_get_del() {
        DBG(cerr << prolog << "cache dir: " << cache_dir << '\n');

        FileCache fc;
        CPPUNIT_ASSERT_MESSAGE("Cache should initialize", fc.initialize(cache_dir, 100, 20));
        string source_file = string(TEST_SRC_DIR) + "/cache/template.txt";

        CPPUNIT_ASSERT_MESSAGE("put() should return true", fc.put("key1", source_file));
        CPPUNIT_ASSERT_MESSAGE("put() should return true", fc.put("key2", source_file));

        FileCache::Item item1;
        bool status = fc.get("key1", item1);
        CPPUNIT_ASSERT_MESSAGE("get() should return true", status);
        CPPUNIT_ASSERT_MESSAGE("Item::get_fd() should not return -1", item1.get_fd() != -1);

        FileCache::Item item2;
        status = fc.get("key2", item2);
        CPPUNIT_ASSERT_MESSAGE("get() should return true", status);
        CPPUNIT_ASSERT_MESSAGE("Item::get_fd() should not return -1", item2.get_fd() != -1);

        auto size_with_both_files = fc.get_cache_info_size();

        // Both the files held by item1 and item2 have shared locks in place, so del()
        // will block when it tries to get an exclusive lock. Close the FDs to release
        // the locks. jhrg 11/1/23
        close(item1.get_fd());
        close(item2.get_fd());

        status = fc.del("key1");
        CPPUNIT_ASSERT_MESSAGE("del() should return true", status);

        status = fc.get("key1", item1);
        CPPUNIT_ASSERT_MESSAGE("get() should return false since key1 has been deleted", !status);

        status = fc.get("key2", item2);
        CPPUNIT_ASSERT_MESSAGE("get() should return true since key2 has _not_ been deleted", status);

        auto size_with_one_file = fc.get_cache_info_size();
        // key1 and key2 cache the same contents
        CPPUNIT_ASSERT_MESSAGE("Size in cache_info should be smaller by half", size_with_both_files == 2 * size_with_one_file);
    }

    // Test del() on a file when it might be locked by get(). This code uses timing to
    // test a common hard case to debug, but the code does not _depend_ on the timing to
    // work - the test should pass no matter how the parent and child run, but in the
    // common case, del(key1) is called before the shared lock made by the call to get(key1)
    // is released. In that case, del(key1) should block until the lock on key1 is released
    // (which happens when the child process exits). jhrg 11/01/23
    void test_put_get_del_in_two_processes() {
        DBG(cerr << prolog << "cache dir: " << cache_dir << '\n');

        FileCache fc;
        CPPUNIT_ASSERT_MESSAGE("Cache should initialize", fc.initialize(cache_dir, 100, 20));
        string source_file = string(TEST_SRC_DIR) + "/cache/template.txt";

        if (fork() == 0) {
            // child process
            CPPUNIT_ASSERT_MESSAGE("put() key1 should return true", fc.put("key1", source_file));
            CPPUNIT_ASSERT_MESSAGE("put() key2 should return true", fc.put("key2", source_file));

            FileCache::Item item1;
            bool status1 = fc.get("key1", item1);
            DBG(cerr << prolog << "child process get() key1 status: " << status1 << '\n');


            FileCache::Item item2;
            bool status2 = fc.get("key2", item2);
            DBG(cerr << prolog << "child process get() key2 status: " << status2 << '\n');
            if (!status2)
                exit(EXIT_FAILURE);

            DBG(cerr << prolog << "child process has shared locks on both key1 and key2\n");
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            DBG(cerr << prolog << "child process exit\n");
            exit(EXIT_SUCCESS);   // exit with 0 if status is true, 1 if false (it's a process)
        }
        else {
            // parent process - generally faster. I used std::this_thread::sleep_for() but this is
            // not multi-threaded code. It's just a way to sleep for a short time.
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            DBG(cerr << prolog << "Before del() key1\n");
            bool del_status = fc.del("key1");
            DBG(cerr << prolog << "After del() key1, status is: " << del_status << '\n');

            // del(key1) might work, but it might fail, depending on the timing of the parent and
            // child processes.

            int child_status;
            wait(&child_status);
            DBG(cerr << prolog << "child process exit status: " << WEXITSTATUS(child_status) << '\n');
            // exit status for a process: 0 is true, 1 is false

            // Because we use blocking locks for access to the cache, if the get and put functions are
            // both called at the same time by two processes, the put() always works if the file does
            // not already exist (because the get always fails if it is run first).
            CPPUNIT_ASSERT_MESSAGE("Child process (which ran put()) should exit with success.",
                                   WEXITSTATUS(child_status) == 0);
            bool status;
            FileCache::Item item1;
            if (del_status) {
                 status = fc.get("key1", item1);
                CPPUNIT_ASSERT_MESSAGE("get() should return false since key1 has been deleted", !status);
            }
            else {
                status = fc.get("key1", item1);
                CPPUNIT_ASSERT_MESSAGE("get() should return true since key1 was not deleted", status);
            }

            FileCache::Item item2;
            status = fc.get("key2", item2);
            CPPUNIT_ASSERT_MESSAGE("get() should return true since key2 has _not_ been deleted", status);
        }
    }

    void test_put_get_del_in_two_processes_two_cache_instances() {
        DBG(cerr << prolog << "cache dir: " << cache_dir << '\n');

        string source_file = string(TEST_SRC_DIR) + "/cache/template.txt";

        if (fork() == 0) {
            // child process
            FileCache fc;
            CPPUNIT_ASSERT_MESSAGE("Cache should initialize", fc.initialize(cache_dir, 100, 20));

            CPPUNIT_ASSERT_MESSAGE("put() key1 should return true", fc.put("key1", source_file));
            CPPUNIT_ASSERT_MESSAGE("put() key2 should return true", fc.put("key2", source_file));

            FileCache::Item item1;
            bool status1 = fc.get("key1", item1);
            DBG(cerr << prolog << "child process get() key1 status: " << status1 << '\n');


            FileCache::Item item2;
            bool status2 = fc.get("key2", item2);
            DBG(cerr << prolog << "child process get() key2 status: " << status2 << '\n');
            if (!status2)
                exit(EXIT_FAILURE);

            DBG(cerr << prolog << "child process has shared locks on both key1 and key2\n");
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            DBG(cerr << prolog << "child process exit\n");
            exit(EXIT_SUCCESS);   // exit with 0 if status is true, 1 if false (it's a process)
        }
        else {
            // parent process - generally faster. I used std::this_thread::sleep_for() but this is
            // not multi-threaded code. It's just a way to sleep for a short time.
            FileCache fc;
            CPPUNIT_ASSERT_MESSAGE("Cache should initialize", fc.initialize(cache_dir, 100, 20));

            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            DBG(cerr << prolog << "Before del() key1\n");
            bool del_status = fc.del("key1");
            DBG(cerr << prolog << "After del() key1, status is: " << del_status << '\n');

            // del(key1) might work, but it might fail, depending on the timing of the parent and
            // child processes.

            int child_status;
            wait(&child_status);
            DBG(cerr << prolog << "child process exit status: " << WEXITSTATUS(child_status) << '\n');
            // exit status for a process: 0 is true, 1 is false

            // Because we use blocking locks for access to the cache, if the get and put functions are
            // both called at the same time by two processes, the put() always works if the file does
            // not already exist (because the get always fails if it is run first).
            CPPUNIT_ASSERT_MESSAGE("Child process (which ran put()) should exit with success.",
                                   WEXITSTATUS(child_status) == 0);
            bool status;
            FileCache::Item item1;
            if (del_status) {
                status = fc.get("key1", item1);
                CPPUNIT_ASSERT_MESSAGE("get() should return false since key1 has been deleted", !status);
            }
            else {
                status = fc.get("key1", item1);
                CPPUNIT_ASSERT_MESSAGE("get() should return true since key1 was not deleted", status);
            }

            FileCache::Item item2;
            status = fc.get("key2", item2);
            CPPUNIT_ASSERT_MESSAGE("get() should return true since key2 has _not_ been deleted", status);
        }
    }

    void test_purge() {
        FileCache fc;
        // Each copy of source_file is 188,973 bytes; 10 --> 1,889,730
        CPPUNIT_ASSERT_MESSAGE("Cache should initialize", fc.initialize(cache_dir, 1'800'000, 370'000));
        string source_file = string(TEST_SRC_DIR) + "/cache/template.txt";

        for (int i = 0; i < 10; ++i) {
            ostringstream oss;
            oss << "key" << i;
            CPPUNIT_ASSERT_MESSAGE("Cache put(keyn) should work", fc.put(oss.str(), source_file));
            oss.clear();
            // this delay spreads the 10 files out over 5 seconds
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }

        CPPUNIT_ASSERT_MESSAGE("Cache info size should be 1,889,730 before purge",
                               fc.get_cache_info_size() == 1'889'730);
        vector<string> files;
        fc.files_in_cache(files);
        DBG(cerr << "files in cache: ");
        DBG(copy(files.begin(), files.end(), ostream_iterator<string>(cerr, "\n")));

        CPPUNIT_ASSERT_MESSAGE("Cache should have 10 files before purge", files.size() == 10);

        string key0 = BESUtil::pathConcat(cache_dir, "key0");
        CPPUNIT_ASSERT_MESSAGE("Cache should have key0 before purge",
                                std::find(files.begin(), files.end(), key0) != files.end());

        fc.purge();

        CPPUNIT_ASSERT_MESSAGE("Cache info size should be 1,511,784 after purge (two files removed)",
                               fc.get_cache_info_size() == 1'511'784);

        files.clear();
        fc.files_in_cache(files);
        DBG(cerr << "\nfiles in cache: ");
        DBG(copy(files.begin(), files.end(), ostream_iterator<string>(cerr, "\n")));

        CPPUNIT_ASSERT_MESSAGE("Cache should have 8 files after purge", files.size() == 8);

        // because the files are added every 0.5s but the time granularity of the cache is 1s,
        // and the test removes two files, we can only be sure that key0 will be removed. It will
        // be the case that key1 xor key2 will be removed, but we can't tell which. Just test
        // for the removal of key0

        CPPUNIT_ASSERT_MESSAGE("Cache should not have key0 after purge",
                               std::find(files.begin(), files.end(), key0) == files.end());
    }

    void test_purge_key0_used_most_recently() {
        FileCache fc;
        // Each copy of source_file is 188,973 bytes; 10 --> 1,889,730
        CPPUNIT_ASSERT_MESSAGE("Cache should initialize", fc.initialize(cache_dir, 1'800'000, 370'000));
        string source_file = string(TEST_SRC_DIR) + "/cache/template.txt";

        for (int i = 0; i < 10; ++i) {
            ostringstream oss;
            oss << "key" << i;
            CPPUNIT_ASSERT_MESSAGE("Cache put(keyn) should work", fc.put(oss.str(), source_file));
            oss.clear();
            // this delay spreads the 10 files out over 5 seconds
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }

        CPPUNIT_ASSERT_MESSAGE("Cache info size should be 1,889,730 before purge",
                               fc.get_cache_info_size() == 1'889'730);

        // key0 is now the most recently used item and should not be removed.
        FileCache::Item item;
        CPPUNIT_ASSERT_MESSAGE("Should be able to get key0", fc.get("key0", item));
        close(item.get_fd());

        fc.purge();

        CPPUNIT_ASSERT_MESSAGE("Cache info size should be 1,511,784 after purge (two files removed)",
                               fc.get_cache_info_size() == 1'511'784);

        vector<string> files;
        fc.files_in_cache(files);
        DBG(cerr << "files in cache: ");
        DBG(copy(files.begin(), files.end(), ostream_iterator<string>(cerr, "\n")));

        CPPUNIT_ASSERT_MESSAGE("Cache should have 8 files after purge", files.size() == 8);

        string key0 = BESUtil::pathConcat(cache_dir, "key0");
        CPPUNIT_ASSERT_MESSAGE("Cache should still have key0 after purge because we just accessed it.",
                               std::find(files.begin(), files.end(), key0) != files.end());
    }

    // This tests if a key opened by get() is not removed by purge(). jhrg 11/03/23
    void test_purge_key0_in_use() {
        FileCache fc;
        // Each copy of source_file is 188,973 bytes; 10 --> 1,889,730
        CPPUNIT_ASSERT_MESSAGE("Cache should initialize", fc.initialize(cache_dir, 1'800'000, 370'000));
        string source_file = string(TEST_SRC_DIR) + "/cache/template.txt";

        for (int i = 0; i < 10; ++i) {
            ostringstream oss;
            oss << "key" << i;
            CPPUNIT_ASSERT_MESSAGE("Cache put(keyn) should work", fc.put(oss.str(), source_file));
            oss.clear();
            // this delay spreads the 10 files out over 5 seconds
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }

        CPPUNIT_ASSERT_MESSAGE("Cache info size should be 1,889,730 before purge",
                               fc.get_cache_info_size() == 1'889'730);

        // key0 is now the most recently used item and should not be removed.
        FileCache::Item item;
        CPPUNIT_ASSERT_MESSAGE("Should be able to get key0", fc.get("key0", item));

        fc.purge();

        CPPUNIT_ASSERT_MESSAGE("Cache info size should be 1,511,784 after purge (two files removed)",
                               fc.get_cache_info_size() == 1'511'784);

        vector<string> files;
        fc.files_in_cache(files);
        DBG(cerr << "files in cache: ");
        DBG(copy(files.begin(), files.end(), ostream_iterator<string>(cerr, "\n")));

        CPPUNIT_ASSERT_MESSAGE("Cache should have 8 files after purge", files.size() == 8);

        string key0 = BESUtil::pathConcat(cache_dir, "key0");
        CPPUNIT_ASSERT_MESSAGE("Cache should still have key0 after purge because it is locked.",
                               std::find(files.begin(), files.end(), key0) != files.end());
    }

    // This tests if purge() skips expensive operations when possible
    void test_purge_efficiency() {
        FileCache fc;
        // Each copy of source_file is 188,973 bytes; 10 --> 1,889,730
        CPPUNIT_ASSERT_MESSAGE("Cache should initialize", fc.initialize(cache_dir, 2'000'000, 370'000));
        string source_file = string(TEST_SRC_DIR) + "/cache/template.txt";

        for (int i = 0; i < 10; ++i) {
            ostringstream oss;
            oss << "key" << i;
            CPPUNIT_ASSERT_MESSAGE("Cache put(keyn) should work", fc.put(oss.str(), source_file));
            oss.clear();
        }

        CPPUNIT_ASSERT_MESSAGE("Cache info size should be 1,889,730 before purge",
                               fc.get_cache_info_size() == 1'889'730);

        CPPUNIT_ASSERT_MESSAGE("Cache purge should do nothing, but return true", fc.purge());

        CPPUNIT_ASSERT_MESSAGE("Cache info size should be unchanged",
                               fc.get_cache_info_size() == 1'889'730);
    }

    CPPUNIT_TEST_SUITE(FileCacheTest);

    CPPUNIT_TEST(test_hash_key);
    CPPUNIT_TEST(test_hash_key_with_info_log);

    CPPUNIT_TEST(test_unintialized_cache);
    CPPUNIT_TEST(test_intialized_cache);
    CPPUNIT_TEST(test_two_intialized_caches_same_directory);

    CPPUNIT_TEST(test_open_cache_info);
    CPPUNIT_TEST(test_open_cache_info_cache_dir_not_set);
    CPPUNIT_TEST(test_get_cache_info_size);
    CPPUNIT_TEST(test_update_cache_info_size);

    CPPUNIT_TEST(test_put_single_file);
    CPPUNIT_TEST(test_put_two_files);
    CPPUNIT_TEST(test_put_duplicate_key);
    CPPUNIT_TEST(test_put_duplicate_key_two_processes);
    CPPUNIT_TEST(test_put_duplicate_key_three_threads);
    CPPUNIT_TEST(test_get_key_in_thread_then_del);
    CPPUNIT_TEST(test_put_duplicate_key_two_processes_three_threads_each);

    CPPUNIT_TEST(test_put_fd_version_single_file);

    CPPUNIT_TEST(test_get_a_file_not_cached);
    CPPUNIT_TEST(test_get_a_file);
    CPPUNIT_TEST(test_get_a_file_two_processes);
    CPPUNIT_TEST(test_get_duplicate_key_three_threads);
    CPPUNIT_TEST(test_get_and_put_duplicate_key_two_processes);

    CPPUNIT_TEST(test_put_get_del);
    CPPUNIT_TEST(test_put_get_del_in_two_processes);
    CPPUNIT_TEST(test_put_get_del_in_two_processes_two_cache_instances);

    CPPUNIT_TEST(test_purge);
    CPPUNIT_TEST(test_purge_key0_used_most_recently);
    CPPUNIT_TEST(test_purge_key0_in_use);
    CPPUNIT_TEST(test_purge_efficiency);

    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(FileCacheTest);

int main(int argc, char *argv[])
{
    return bes_run_tests<FileCacheTest>(argc, argv, "cerr,file-cache") ? 0 : 1;
}
