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
#include "BESDebug.h"

#include "test_config.h"

#include "modules/common/run_tests_cppunit.h"

using namespace std;

#define prolog string("FileCacheTest::").append(__func__).append("() - ")
#define CLEAN_CACHE 1

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

string
read_file(const string &fn)
{
    ifstream is;
    is.open (fn.c_str(), ios::binary );

    if (!is)
        return {"Could not read file: " + fn};

    // get length of file:
    is.seekg (0, ios::end);
    long length = is.tellg();

    // back to start
    is.seekg (0, ios::beg);

    // allocate memory:
    vector<char> buffer(length+1);

    // read data as a block:
    is.read (buffer.data(), length);
    is.close();
    buffer[length] = '\0';

    return {buffer.data()};
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
#if CLEAN_CACHE
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
            DBG(cerr << prolog << "Could not clean the cache directory: " << cache_dir << "(process: "
                << getpid() << ")\n");
        }
#endif
    }

    static void demonstrate_flock(const string &filename) {
        int fd = open(filename.c_str(), O_RDWR | O_CREAT, 0666);
        if (fd == -1) {
            CPPUNIT_FAIL("Failed to open the file for flock demonstration: " + string(filename) + " " + get_errno());
        }

        pid_t pid = fork();
        if (pid == -1) {
            close(fd);
            CPPUNIT_FAIL("Failed to fork in flock demonstrator: " + get_errno());
        }

        if (pid == 0) {  // Child process
            int fd2 = open(filename.c_str(), O_RDWR, 0666);
            if (fd2 == -1) {
                CPPUNIT_FAIL("Failed to open the file for flock demonstration: " + string(filename) + " " + get_errno());
            }
            DBG(cerr << prolog << "Child: Attempting to acquire lock...\n");
            if (flock(fd2, LOCK_EX | LOCK_NB) == -1) {
                if (errno == EWOULDBLOCK) {
                    DBG(cerr << prolog << "Child: Lock is already held by another process.\n");
                } else {
                    CPPUNIT_FAIL("Child process: The only acceptable error is EWOULDBLOCK: " + get_errno());
                    exit(EXIT_FAILURE);
                }
            } else {
                DBG(cerr << prolog << "Child: Lock acquired! Writing to file.\n");
                dprintf(fd2, "Child: Writing to the file.\n");
                // Simulate work
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                DBG(cerr << prolog << "Child: Releasing lock.\n");
                flock(fd2, LOCK_UN);
            }
            close(fd2);
            exit(EXIT_SUCCESS);
        } else {  // Parent process
            DBG(cerr << prolog << "Parent: Attempting to acquire lock...\n");
            if (flock(fd, LOCK_EX | LOCK_NB) == -1) {
                if (errno == EWOULDBLOCK) {
                    DBG(cerr << prolog << "Parent: Lock is already held by another process.\n");
                } else {
                    CPPUNIT_FAIL("Parent process: The only acceptable error is EWOULDBLOCK: " + get_errno());
                }
            } else {
                DBG(cerr << prolog << "Parent: Lock acquired! Writing to file.\n");
                dprintf(fd, "Parent: Writing to the file.\n");
                // Simulate work
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                DBG(cerr << prolog << "Parent: Releasing lock.\n");
                flock(fd, LOCK_UN);
            }
            close(fd);

            // Wait for the child process to finish
            pid_t child = wait(nullptr);
            CPPUNIT_ASSERT_MESSAGE("The child process should have exited: " + get_errno(), child != -1);
        }
    }

    void test_flock() {
        string test_file_path = cache_dir + "/flock_test.txt";
        demonstrate_flock(test_file_path);
        string contents = read_file(test_file_path);
        DBG(cerr << prolog << "File contents: " << contents << '\n');
        auto result = (contents.find("Parent: Writing to the file.") != string::npos && contents.find("Child: Writing to the file.") == string::npos)
                || (contents.find("Child: Writing to the file.") != string::npos && contents.find("Parent: Writing to the file.") == string::npos);
        CPPUNIT_ASSERT_MESSAGE("The file should contain the child's xor the parent's message", result);
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
    void test_uninitialized_cache()
    {
        FileCache fc;
        CPPUNIT_ASSERT_MESSAGE("Cache invariant should be false - no cache file open", !fc.invariant());
    }

    void test_initialized_cache()
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

    void test_two_initialized_caches_same_directory()
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

        struct stat cifsb = {};
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
        struct stat sb = {};
        if (stat(source_file.c_str(), &sb) != 0)
            CPPUNIT_FAIL("stat() failed on cached file");
        struct stat rcfp_b = {};
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
        struct stat sb = {};
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
            FileCache fc2;
            CPPUNIT_ASSERT_MESSAGE("Cache should initialize", fc2.initialize(cache_dir, 100, 20));

            bool status = fc2.put("key1", source_file);
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
    static bool multiple_put_operations(FileCache &fc, const string &source_file) {
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
            FileCache fc2;
            CPPUNIT_ASSERT_MESSAGE("Cache should initialize", fc2.initialize(cache_dir, 100, 20));

            bool xor_status = multiple_put_operations(fc2, source_file);

            DBG(cerr << prolog << "child process put() status: " << xor_status << '\n');
            exit(xor_status ? 0 : 1);   // exit with 0 if status is true, 1 if false (it's a process)
        }
        else {
            // parent process - generally faster. I used std::this_thread::sleep_for() but this is
            // not multi-threaded code. It's just a way to sleep for a short time.
            std::this_thread::sleep_for(std::chrono::milliseconds(1));

            bool xor_status = multiple_put_operations(fc, source_file);

            int child_status;
            wait(&child_status);
            DBG(cerr << prolog << "child process exit status: " << WEXITSTATUS(child_status) << '\n');
            // exit status for a process: 0 is true, 1 is false
            bool xor_parent_child_status = (xor_status && WEXITSTATUS(child_status) != 0) || (!xor_status && WEXITSTATUS(child_status) == 0);
            CPPUNIT_ASSERT_MESSAGE("one status should be true and one false", xor_parent_child_status);
        }
    }

    /**
     * @brief Helper function for test_put_two_files_two_processes()
     * @note when put() exits, the cache_info file will be updated so that the cache size is
     * incremented to include the new file. The test_put_two_files_two_processes() test is checking
     * to see that the cache_info file is correctly protected against simultaneous writes. This
     * function is called from two different processes.
     */
    void put_helper(FileCache &fc, const string &source_file, const string &key) {
        FileCache::PutItem item(fc);

        bool status = fc.put(key, source_file);    // creates and locks an empty file.
        CPPUNIT_ASSERT_MESSAGE("put() should return true", status);

        // cached file is really there
        string raw_cached_file_path = cache_dir + "/" + key;
        CPPUNIT_ASSERT_MESSAGE("Should see cached file", access(raw_cached_file_path.c_str(), F_OK) == 0);

        // Now check the size of the cached file - same as the source file?
        struct stat rcfp_b = {};
        if (stat(raw_cached_file_path.c_str(), &rcfp_b) != 0)
            CPPUNIT_FAIL("stat() failed on cached file");

        struct stat source_file_b = {};
        if (stat(raw_cached_file_path.c_str(), &source_file_b) != 0)
            CPPUNIT_FAIL("stat() failed on source file");

        CPPUNIT_ASSERT_MESSAGE("Cached file should be the same size as the source file",
                               source_file_b.st_size == rcfp_b.st_size);

        CPPUNIT_ASSERT_MESSAGE("Cached and source file should have the same sha256 hash",
                               sha256(source_file) == sha256(raw_cached_file_path));
    }

    // See test_put_fd_version_two_files_two_processes() for a similar test and for an explanation
    // of some of the finer points of flock(2) when used to lock files in parent and child processes.
    // jhrg 1/2/25
    void test_put_two_files_two_processes() {
        DBG(cerr << prolog << "cache dir: " << cache_dir << '\n');

        FileCache fc;
        CPPUNIT_ASSERT_MESSAGE("Cache should initialize", fc.initialize(cache_dir, 100*MEGABYTE, 20*MEGABYTE));
        string source_file_1 = string(TEST_SRC_DIR) + "/cache/testfile.txt.bz2";    // 74 bytes
        string source_file_2 = string(TEST_SRC_DIR) + "/cache/testfile.txt.gz";     // 70 bytes

        if (fork() == 0) {
            // child process.
            FileCache fc2;
            CPPUNIT_ASSERT_MESSAGE("Cache should initialize", fc2.initialize(cache_dir, 100*MEGABYTE, 20*MEGABYTE));
            // Each process will put 1000 files into the cache. This will test the cache_info file
            // for protection against simultaneous writes.
            for (int i = 0; i < 1000; i++) {
                put_helper(fc2, source_file_1, "key1-" + to_string(i));
            }

            FileCache::Item item;
            bool status = fc2.get("key1-999", item);
            DBG(cerr << prolog << "child process get() status: " << status << '\n');
            exit(status ? 0 : 1);   // exit with 0 if status is true, 1 if false (it's a process)
        }
        else {
            // parent process - generally faster. I used std::this_thread::sleep_for() but, this is
            // not multithreaded code. It's just a way to sleep for a short time.
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            for (int i = 0; i < 1000; i++) {
                put_helper(fc, source_file_2, "key2-" + to_string(i));
            }

            FileCache::Item item;
            bool status = fc.get("key2-999", item);
            DBG(cerr << prolog << "parent process get() status: " << status << '\n');
            CPPUNIT_ASSERT_MESSAGE("get() in the parent should return true", status);

            int child_status;
            wait(&child_status);
            DBG(cerr << prolog << "child process exit status: " << WEXITSTATUS(child_status) << '\n');
            // exit status for a process: 0 is true, 1 is false
            CPPUNIT_ASSERT_MESSAGE("get() in the child should return true", WEXITSTATUS(child_status) == 0);
        }


        auto cache_info_file_size = FileCache::get_file_size(fc.d_cache_info_fd);
        auto cache_size = fc.get_cache_info_size();
        CPPUNIT_ASSERT_MESSAGE("Cache info file size should be 8 bytes but is " + to_string(cache_info_file_size)
                               + " and the recorded cache size should be 144,000 but is " + to_string(cache_size),
                               cache_info_file_size == 8);
        CPPUNIT_ASSERT_MESSAGE("Cache size should be 144,000, but is " + to_string(cache_size), cache_size == 144'000);
    }

    void test_put_fd_version_single_file()
    {
        DBG(cerr << prolog << "cache dir: " << cache_dir << '\n');

        FileCache fc;
        CPPUNIT_ASSERT_MESSAGE("Cache should initialize", fc.initialize(cache_dir, 100, 20));
        string source_file = string(TEST_SRC_DIR) + "/cache/template.txt";

        struct stat sb = {};  // used later...
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
            struct stat rcfp_b = {};
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

    void test_put_fd_version_single_file_with_purge()
    {
        DBG(cerr << prolog << "cache dir: " << cache_dir << '\n');

        FileCache fc;
        CPPUNIT_ASSERT_MESSAGE("Cache should initialize", fc.initialize(cache_dir, 100, 20));
        string source_file = string(TEST_SRC_DIR) + "/cache/template.txt";

        struct stat sb = {};  // used later...
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
            struct stat rcfp_b = {};
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

            // will this deadlock?
            CPPUNIT_ASSERT_MESSAGE("purge() should work since the item (key1) is locked, but not the cache itself.", fc.purge());
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

    /**
     * @brief Helper function for test_put_a_file_two_processes()
     * @note when this exits, the cache_info file will be updated so that the cache size is
     * incremented to include the new file. The test_put_a_file_two_processes test is checking
     * to see that the cache_info file is correctly protected against simultaneous writes .
     */
    void put_an_item_helper(FileCache &fc, const string &source_file, const string &key) {
        FileCache::PutItem item(fc);

        bool status = fc.put(key, item);    // creates and locks an empty file.
        CPPUNIT_ASSERT_MESSAGE("put() should return true", status);

        // cached file is really there
        string raw_cached_file_path = cache_dir + "/" + key;
        CPPUNIT_ASSERT_MESSAGE("Should see cached file", access(raw_cached_file_path.c_str(), F_OK) == 0);

        // Write stuff to the open, locked 'item.'
        int fd2;
        if ((fd2 = open(source_file.c_str(), O_RDONLY)) < 0) {
            CPPUNIT_FAIL("Failed to open the source data");
        }

        auto source_file_size = FileCache::get_file_size(fd2);
        std::vector<char> buf(source_file_size + 1);
        ssize_t n;
        while ((n = read(fd2, buf.data(), buf.size())) > 0) {
            if (write(item.get_fd(), buf.data(), n) != n) {
                CPPUNIT_FAIL("Failed to write the source data");
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        // Now check the size of the cached file - same as the source file?
        struct stat rcfp_b = {};
        if (stat(raw_cached_file_path.c_str(), &rcfp_b) != 0)
            CPPUNIT_FAIL("stat() failed on cached file");

        CPPUNIT_ASSERT_MESSAGE("Cached file should be the same size as the source file", source_file_size == rcfp_b.st_size);

        CPPUNIT_ASSERT_MESSAGE("Cached and source file should have the same sha256 hash",
                               sha256(source_file) == sha256(raw_cached_file_path));
    }

    // This test uses 1000 put operations in each of two processes to make it virtually certain there
    // will be a collision if locking fail. On the master branch w/o the fixed FileCache code, it always
    // fails. jhrg 1/1/25
    void test_put_fd_version_two_files_two_processes() {
        DBG(cerr << prolog << "cache dir: " << cache_dir << '\n');

        FileCache fc;
        CPPUNIT_ASSERT_MESSAGE("Cache should initialize", fc.initialize(cache_dir, 100*MEGABYTE, 20*MEGABYTE));
        string source_file_1 = string(TEST_SRC_DIR) + "/cache/testfile.txt.bz2";    // 74 bytes
        string source_file_2 = string(TEST_SRC_DIR) + "/cache/testfile.txt.gz";     // 70 bytes

        if (fork() == 0) {
            // child process
            // Use a new cache object to ensure each process opens its own cache_info file. There is an
            // issue where if two processes share the same fd, flock(2) will not lock the file as expected.
            // This can be fixed by having each process open the cache_info file. jhrg 1/1/25
            FileCache fc2;  // Use a new cache object to ensure each process opens its own cache_info file.
            CPPUNIT_ASSERT_MESSAGE("Cache should initialize", fc2.initialize(cache_dir, 100*MEGABYTE, 20*MEGABYTE));
            // Both processes will put the same file in the cache, 1000 times over. In testing, this
            // has always caused a collision if the cache_info file is not locked correctly. jhrg 1/2/25
            for (int i = 0; i < 1000; i++) {
                put_an_item_helper(fc2, source_file_1, "key1-" + to_string(i));
            }

            FileCache::Item item;
            bool status = fc2.get("key1-999", item);
            DBG(cerr << prolog << "child process get() status: " << status << '\n');
            exit(status ? 0 : 1);   // exit with 0 if status is true, 1 if false (it's a process)
        }
        else {
            // parent process - generally faster. I used std::this_thread::sleep_for() but this is
            // not multi-threaded code. It's just a way to sleep for a short time.
            // Put the second file in the cache.
             for (int i = 0; i < 1000; i++) {
                put_an_item_helper(fc, source_file_2, "key2-" + to_string(i));
            }

            FileCache::Item item;
            bool status = fc.get("key2-999", item);
            DBG(cerr << prolog << "parent process get() status: " << status << '\n');
            CPPUNIT_ASSERT_MESSAGE("get() in the parent should return true", status);

            int child_status;
            wait(&child_status);
            DBG(cerr << prolog << "child process exit status: " << WEXITSTATUS(child_status) << '\n');
            // exit status for a process: 0 is true, 1 is false
            CPPUNIT_ASSERT_MESSAGE("get() in the child should return true", WEXITSTATUS(child_status) == 0);
        }

        auto cache_info_file_size = FileCache::get_file_size(fc.d_cache_info_fd);
        auto cache_size = fc.get_cache_info_size();
        CPPUNIT_ASSERT_MESSAGE("Cache info file size should be 8 bytes but is " + to_string(cache_info_file_size)
                                + " and the recorded cache size should be 144,000 but is " + to_string(cache_size),
                                cache_info_file_size == 8);
        CPPUNIT_ASSERT_MESSAGE("Cache size should be 144,000, but is " + to_string(cache_size), cache_size == 144'000);
    }

    void test_put_data_version_single_file()
    {
        DBG(cerr << prolog << "cache dir: " << cache_dir << '\n');

        FileCache fc;
        CPPUNIT_ASSERT_MESSAGE("Cache should initialize", fc.initialize(cache_dir, 100, 20));
        string source_file = string(TEST_SRC_DIR) + "/cache/template.txt";

        struct stat sb = {};  // used later...
        if (stat(source_file.c_str(), &sb) != 0)
            CPPUNIT_FAIL("stat() failed on cached file");

        string key1_data = read_file(source_file);
        bool status = fc.put_data("key1", key1_data);
        CPPUNIT_ASSERT_MESSAGE("put() should return true", status);

        // cached file is really there
        string raw_cached_file_path = cache_dir + "/key1";
        CPPUNIT_ASSERT_MESSAGE("Should see cached file", access(raw_cached_file_path.c_str(), F_OK) == 0);

        // Now check the size of the cached file - same as the source file?
        struct stat rcfp_b = {};
        if (stat(raw_cached_file_path.c_str(), &rcfp_b) != 0)
            CPPUNIT_FAIL("stat() failed on cached file");

        CPPUNIT_ASSERT_MESSAGE("Cached file should be the same size as the source file",
                               key1_data.size() == rcfp_b.st_size);

        // Was cache_info_size updated correctly?
        CPPUNIT_ASSERT_MESSAGE("Cached info size should be the size of this one file",
                               fc.get_cache_info_size() == key1_data.size());

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

    void test_put_data_version_single_file_with_purge() {
        DBG(cerr << prolog << "cache dir: " << cache_dir << '\n');

        FileCache fc;
        CPPUNIT_ASSERT_MESSAGE("Cache should initialize", fc.initialize(cache_dir, 100*MEGABYTE, 20*MEGABYTE));
        string source_file = string(TEST_SRC_DIR) + "/cache/template.txt";

        struct stat sb = {};  // used later...
        if (stat(source_file.c_str(), &sb) != 0)
            CPPUNIT_FAIL("stat() failed on cached file");

        string key1_data = read_file(source_file);

        bool status = fc.put_data("key1", key1_data);
        CPPUNIT_ASSERT_MESSAGE("put() should return true", status);

        // cached file is really there
        string raw_cached_file_path = cache_dir + "/key1";
        CPPUNIT_ASSERT_MESSAGE("Should see cached file", access(raw_cached_file_path.c_str(), F_OK) == 0);

        // Now check the size of the cached file - same as the source file?
        struct stat rcfp_b = {};
        if (stat(raw_cached_file_path.c_str(), &rcfp_b) != 0)
            CPPUNIT_FAIL("stat() failed on cached file");

        CPPUNIT_ASSERT_MESSAGE("Cached file should be the same size as the source file", sb.st_size == rcfp_b.st_size);

        CPPUNIT_ASSERT_MESSAGE("Cached and source file should have the same sha256 hash",
                               sha256(source_file) == sha256(raw_cached_file_path));

        // will this deadlock? It should not (adapted from the PutItem test above). jhrg 1/10/25
        CPPUNIT_ASSERT_MESSAGE("purge() should work.", fc.purge());

        // Was cache_info_size updated correctly?
        CPPUNIT_ASSERT_MESSAGE("Cached info size (" + to_string(fc.get_cache_info_size()) + ") should be the size of this one file",
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

    /**
     * @brief Helper function for test_put_data_version_two_files_two_processes()
     * @note when this exits, the cache_info file will be updated so that the cache size is
     * incremented to include the new file. The test_put_data_version_two_files_two_processes test is checking
     * to see that the cache_info file is correctly protected against simultaneous writes .
     */
    void put_a_string_helper(FileCache &fc, const string &data, const string &key) {
        bool status = fc.put_data(key, data);    // creates and locks an empty file.
        CPPUNIT_ASSERT_MESSAGE("put() should return true", status);

        // cached file is really there
        string raw_cached_file_path = cache_dir + "/" + key;
        CPPUNIT_ASSERT_MESSAGE("Should see cached file", access(raw_cached_file_path.c_str(), F_OK) == 0);

        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        // Now check the size of the cached file - same as the source file?
        struct stat rcfp_b = {};
        if (stat(raw_cached_file_path.c_str(), &rcfp_b) != 0)
            CPPUNIT_FAIL("stat() failed on cached file");

        CPPUNIT_ASSERT_MESSAGE("Cached file should be the same size as the source file", data.size() == rcfp_b.st_size);
    }

    // This test uses 1000 put operations in each of two processes to make it virtually certain there
    // will be a collision if locking fail. On the master branch w/o the fixed FileCache code, it always
    // fails. jhrg 1/1/25
    void test_put_data_version_two_files_two_processes() {
        DBG(cerr << prolog << "cache dir: " << cache_dir << '\n');

        FileCache fc;
        CPPUNIT_ASSERT_MESSAGE("Cache should initialize", fc.initialize(cache_dir, 100*MEGABYTE, 20*MEGABYTE));
        string source_data_1 = read_file(string(TEST_SRC_DIR) + "/cache/testfile_plain.txt");    // 40 bytes
        string source_data_2 = read_file(string(TEST_SRC_DIR) + "/cache/testfile_plain_2.txt");

        if (fork() == 0) {
            // child process
            // Use a new cache object to ensure each process opens its own cache_info file. There is an
            // issue where if two processes share the same fd, flock(2) will not lock the file as expected.
            // This can be fixed by having each process open the cache_info file. jhrg 1/1/25
            FileCache fc2;  // Use a new cache object to ensure each process opens its own cache_info file.
            CPPUNIT_ASSERT_MESSAGE("Cache should initialize", fc2.initialize(cache_dir, 100*MEGABYTE, 20*MEGABYTE));
            // Both processes will put the same file in the cache, 1000 times over. In testing, this
            // has always caused a collision if the cache_info file is not locked correctly. jhrg 1/2/25
            for (int i = 0; i < 1000; i++) {
                put_a_string_helper(fc2, source_data_1, "key1-" + to_string(i));
            }

            FileCache::Item item;
            bool status = fc2.get("key1-999", item);
            DBG(cerr << prolog << "child process get() status: " << status << '\n');
            exit(status ? 0 : 1);   // exit with 0 if status is true, 1 if false (it's a process)
        }
        else {
            // parent process - generally faster. I used std::this_thread::sleep_for() but this is
            // not multi-threaded code. It's just a way to sleep for a short time.
            // Put the second file in the cache.
            for (int i = 0; i < 1000; i++) {
                put_a_string_helper(fc, source_data_2, "key2-" + to_string(i));
            }

            FileCache::Item item;
            bool status = fc.get("key2-999", item);
            DBG(cerr << prolog << "parent process get() status: " << status << '\n');
            CPPUNIT_ASSERT_MESSAGE("get() in the parent should return true", status);

            int child_status;
            wait(&child_status);
            DBG(cerr << prolog << "child process exit status: " << WEXITSTATUS(child_status) << '\n');
            // exit status for a process: 0 is true, 1 is false
            CPPUNIT_ASSERT_MESSAGE("get() in the child should return true", WEXITSTATUS(child_status) == 0);
        }

        auto cache_info_file_size = FileCache::get_file_size(fc.d_cache_info_fd);
        auto cache_size = fc.get_cache_info_size();
        CPPUNIT_ASSERT_MESSAGE("Cache info file size should be 8 bytes but is " + to_string(cache_info_file_size)
                               + " and the recorded cache size should be 143,000 but is " + to_string(cache_size),
                               cache_info_file_size == 8);
        CPPUNIT_ASSERT_MESSAGE("Cache size should be 143,000, but is " + to_string(cache_size), cache_size == 143'000);
    }

    // This version of put_a_string helper handles when put() fails. Return true/false if the
    // put succeeded. jhrg 1/10/25
    bool relaxed_put_a_string_helper(FileCache &fc, const string &data, const string &key) {
        bool status = fc.put_data(key, data);    // creates and locks an empty file.

        if (status) {
            // cached file is really there - for collisions, the file might be there in name only
            // because a different process might not be done writing it. So only try to get the size
            // of the file if put() succeeded.
            string raw_cached_file_path = cache_dir + "/" + key;
            CPPUNIT_ASSERT_MESSAGE("Should see cached file", access(raw_cached_file_path.c_str(), F_OK) == 0);

            int delay = rand() % 10; // 0-9 ms delay
            std::this_thread::sleep_for(std::chrono::milliseconds(delay));

            // Now check the size of the cached file - same as the source file?
            struct stat rcfp_b = {};
            if (stat(raw_cached_file_path.c_str(), &rcfp_b) != 0)
                CPPUNIT_FAIL("stat() failed on cached file");

            CPPUNIT_ASSERT_MESSAGE("Cached file should be the same size as the source file", data.size() == rcfp_b.st_size);
        }

        return status;
    }

    int fork_child_helper(const string &key_basename, const string &data) {
        int child_pid = fork();
        if (child_pid == 0) {
            // child process
            // Use a new cache object to ensure each process opens its own cache_info file. There is an
            // issue where if two processes share the same fd, flock(2) will not lock the file as expected.
            // This can be fixed by having each process open the cache_info file. jhrg 1/1/25
            FileCache fc;  // Use a new cache object to ensure each process opens its own cache_info file.
            CPPUNIT_ASSERT_MESSAGE("Cache should initialize", fc.initialize(cache_dir, 100 * MEGABYTE, 20 * MEGABYTE));
            // Both processes will put the same file in the cache, 1000 times over. In testing, this
            // has always caused a collision if the cache_info file is not locked correctly. jhrg 1/2/25
            int success = 0;
            for (int i = 0; i < 1000; i++) {
                if (relaxed_put_a_string_helper(fc, data, key_basename + to_string(i)))
                    success += 1;
            }
            DBG(cerr << "child process put() success: " << success << '\n');
            exit(0);   // exit with 0 if status is true, 1 if false (it's a process)
        }
        else {
            DBG(cerr << prolog << "child process pid: " << child_pid << '\n');
            return child_pid;
        }
    }

    void test_put_data_version_2000_files_18_processes() {
        DBG(cerr << prolog << "cache dir: " << cache_dir << '\n');

        FileCache fc;
        CPPUNIT_ASSERT_MESSAGE("Cache should initialize", fc.initialize(cache_dir, 100*MEGABYTE, 20*MEGABYTE));
        string source_data_1 = read_file(string(TEST_SRC_DIR) + "/cache/testfile_plain.txt");    // 40 bytes
        string source_data_2 = read_file(string(TEST_SRC_DIR) + "/cache/testfile_plain_2.txt");

        // Lots of collisions here. This test is a stress test for the cache_info file. jhrg 1/2/25
        fork_child_helper("key1-", source_data_1);
        fork_child_helper("key2-", source_data_2);
        fork_child_helper("key1-", source_data_1);
        fork_child_helper("key2-", source_data_2);
        fork_child_helper("key1-", source_data_1);
        fork_child_helper("key2-", source_data_2);
        fork_child_helper("key1-", source_data_1);
        fork_child_helper("key2-", source_data_2);
        fork_child_helper("key1-", source_data_1);
        fork_child_helper("key2-", source_data_2);
        fork_child_helper("key1-", source_data_1);
        fork_child_helper("key2-", source_data_2);
        fork_child_helper("key1-", source_data_1);
        fork_child_helper("key2-", source_data_2);
        fork_child_helper("key1-", source_data_1);
        fork_child_helper("key2-", source_data_2);
        fork_child_helper("key1-", source_data_1);
        fork_child_helper("key2-", source_data_2);

        int child_status;
        int child_pid;
        while ((child_pid = wait(&child_status)) > 0) {
            DBG(cerr << prolog << "child process " << to_string(child_pid) << ") exit status: "
                << WEXITSTATUS(child_status) << '\n');
            // exit status for a process: 0 is true, 1 is false
            CPPUNIT_ASSERT_MESSAGE("get() in the child should return true, but returned: "
                                    + to_string(WEXITSTATUS(child_status)), WEXITSTATUS(child_status) == 0);
        }

        auto cache_info_file_size = FileCache::get_file_size(fc.d_cache_info_fd);
        auto cache_size = fc.get_cache_info_size();
        CPPUNIT_ASSERT_MESSAGE("Cache info file size should be 8 bytes but is " + to_string(cache_info_file_size)
                               + " and the recorded cache size should be 143,000 but is " + to_string(cache_size),
                               cache_info_file_size == 8);
        CPPUNIT_ASSERT_MESSAGE("Cache size should be 143,000, but is " + to_string(cache_size), cache_size == 143'000);
    }

    void test_get_a_file_not_cached()
    {
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
            FileCache fc2;
            CPPUNIT_ASSERT_MESSAGE("Cache should initialize", fc2.initialize(cache_dir, 100, 20));
            FileCache::Item item;
            bool status = fc2.get("key1", item);
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
        struct stat sb = {};
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
                struct stat cb = {};
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
                struct stat cb = {};
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
            FileCache fc2;
            CPPUNIT_ASSERT_MESSAGE("Cache should initialize", fc2.initialize(cache_dir, 100, 20));

            CPPUNIT_ASSERT_MESSAGE("put() key1 should return true", fc2.put("key1", source_file));
            CPPUNIT_ASSERT_MESSAGE("put() key2 should return true", fc2.put("key2", source_file));

            FileCache::Item item1;
            bool status1 = fc2.get("key1", item1);
            DBG(cerr << prolog << "child process get() key1 status: " << status1 << '\n');


            FileCache::Item item2;
            bool status2 = fc2.get("key2", item2);
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
            std::this_thread::sleep_for(std::chrono::milliseconds(250));
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
        // for the removal of key0.
        string key1 = BESUtil::pathConcat(cache_dir, "key1");
        string key2 = BESUtil::pathConcat(cache_dir, "key2");
        string key3 = BESUtil::pathConcat(cache_dir, "key3");
        CPPUNIT_ASSERT_MESSAGE("Cache should not have key9 after purge",
                               std::find(files.begin(), files.end(), key0) == files.end()
                               || std::find(files.begin(), files.end(), key1) == files.end()
                               || std::find(files.begin(), files.end(), key2) == files.end()
                               || std::find(files.begin(), files.end(), key3) == files.end());
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
            std::this_thread::sleep_for(std::chrono::milliseconds(201));
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
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
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

    CPPUNIT_TEST(test_flock);

    CPPUNIT_TEST(test_hash_key);
    CPPUNIT_TEST(test_hash_key_with_info_log);

    CPPUNIT_TEST(test_uninitialized_cache);
    CPPUNIT_TEST(test_initialized_cache);
    CPPUNIT_TEST(test_two_initialized_caches_same_directory);

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
    CPPUNIT_TEST(test_put_two_files_two_processes);

    CPPUNIT_TEST(test_put_fd_version_single_file);
    CPPUNIT_TEST(test_put_fd_version_two_files_two_processes);
    CPPUNIT_TEST(test_put_fd_version_single_file_with_purge);

    CPPUNIT_TEST(test_put_data_version_single_file);
    CPPUNIT_TEST(test_put_data_version_single_file_with_purge);
    CPPUNIT_TEST(test_put_data_version_two_files_two_processes);
    CPPUNIT_TEST(test_put_data_version_2000_files_18_processes);

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
