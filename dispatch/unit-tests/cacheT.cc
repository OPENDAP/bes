// cacheT.C

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2004-2009 University Corporation for Atmospheric Research
// Author: Patrick West <pwest@ucar.edu> and Jose Garcia <jgarcia@ucar.edu>
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
// You can contact University Corporation for Atmospheric Research at
// 3080 Center Green Drive, Boulder, CO 80301

// (c) COPYRIGHT University Corporation for Atmospheric Research 2004-2005
// Please read the full copyright statement in the file COPYRIGHT_UCAR.
//
// Authors:
//      pwest       Patrick West <pwest@ucar.edu>
//      jgarcia     Jose Garcia <jgarcia@ucar.edu>

#include <dirent.h> // for closedir opendir
#include <unistd.h> // for sleep

#include <cstdlib>
#include <iostream>
#include <sstream>

#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/extensions/TestFactoryRegistry.h>

#include "BESDebug.h"
#include "BESError.h"
#include "BESFileLockingCache.h"
#include "BESInternalError.h"
#include "BESUtil.h"
#include "TheBESKeys.h"

#include "test_config.h"

using namespace std;
using namespace CppUnit;

// Not run by default!
// Set from the command-line invocation of the main only
// since we're not sure the OS has room for the test files.
static bool RUN_64_BIT_CACHE_TEST = false;

static const std::string CACHE_PREFIX = string("bes_cache");
static const std::string MATCH_PREFIX = string(CACHE_PREFIX) + string("#");

static const string TEST_CACHE_DIR = BESUtil::assemblePath(TEST_SRC_DIR, "cache");

// For the 64 bit  (> 4G tests)
static const std::string CACHE_DIR_TEST_64 = string(TEST_SRC_DIR) + string("/test_cache_64");
static const unsigned long long MAX_CACHE_SIZE_IN_MEGS_TEST_64 = 5000ULL; // in Mb...
// shift left by 20 multiplies by 1Mb
static const unsigned long long MAX_CACHE_SIZE_IN_BYTES_TEST_64 = (MAX_CACHE_SIZE_IN_MEGS_TEST_64 << 20);
static const unsigned long long NUM_FILES_TEST_64 = 6ULL;
static const unsigned long long FILE_SIZE_IN_MEGS_TEST_64 = 1024ULL; // in Mb

static bool debug = false;
static bool bes_debug = false;
#undef DBG
#define DBG(x)                                                                                                         \
    do {                                                                                                               \
        if (debug)                                                                                                     \
            (x);                                                                                                       \
    } while (false)

/// Run a command in a shell.
void run_sys(const string &cmd) {
    int status;
    DBG(cerr << __func__ << " command: '" << cmd);
    status = system(cmd.c_str());
    DBG(cerr << "' status: " << status << endl);
}

/**
 * @brief Set up the cache.
 *
 * Add a set of eight test files to the cache, with names that are easy to
 * work with and each with an access time two seconds later than the
 * preceding one
 *
 * @param cache_dir Directory that holds the cached files.
 */
void init_cache(const string &cache_dir) {
    DBG(cerr << __func__ << "() - BEGIN " << endl);

    string t_file = cache_dir + "/template.txt";
    for (int i = 1; i < 9; i++) {
        ostringstream s;
        s << BESUtil::assemblePath(cache_dir, CACHE_PREFIX) << "#usr#local#data#template0" << i << ".txt";

        string cmd = "";
        cmd.append("cp -f ").append(t_file).append(" ").append(s.str());
        run_sys(cmd);

        cmd = "";
        cmd.append("chmod a+w ").append(s.str());
        run_sys(cmd);

        cmd = "";
        cmd.append("cat ").append(s.str()).append(" > /dev/null");
        run_sys(cmd);
    }

    DBG(cerr << __func__ << "() - END " << endl);
}

/**
 * @param cache_dir Name of the cache directory
 * @param should_be This file should be in the cache
 * @param num_files There should be this many file with the MATCH_PREFIX
 */
void check_cache(const string &cache_dir, const string &should_be, unsigned int num_files) {
    DBG(cerr << __func__ << "() - BEGIN, should_be: " << should_be << ", num_files: " << num_files << endl);

    map<string, string> contents;
    string match_prefix = MATCH_PREFIX;
    DIR *dip = opendir(cache_dir.c_str());
    CPPUNIT_ASSERT(dip);
    struct dirent *dit;
    while ((dit = readdir(dip)) != NULL) {
        string dirEntry = dit->d_name;
        if (dirEntry.compare(0, match_prefix.size(), match_prefix) == 0)
            contents[dirEntry] = dirEntry;
    }

    closedir(dip);

    CPPUNIT_ASSERT(num_files == contents.size());

    bool found = false;
    for (map<string, string>::const_iterator ci = contents.begin(), ce = contents.end(); ci != ce; ci++) {
        DBG(cerr << "contents: " << (*ci).first << endl);
        if ((*ci).first == should_be) {
            found = true;
            break;
        }
    }

    CPPUNIT_ASSERT(found);

    DBG(cerr << __func__ << "() - END " << endl);
}

void purge_cache(const string &cache_dir, const string &cache_prefix) {
    DBG(cerr << __func__ << "() - BEGIN " << endl);
    ostringstream s;
    s << "rm -" << (debug ? "v" : "") << "f " << BESUtil::assemblePath(cache_dir, cache_prefix) << "*";
    run_sys(s.str());
    DBG(cerr << __func__ << "() - END " << endl);
}

string show_cache(const string &cache_dir, const string &match_prefix) {
    map<string, string> contents;
    ostringstream oss;
    DIR *dip = opendir(cache_dir.c_str());
    CPPUNIT_ASSERT(dip);
    struct dirent *dit = nullptr;
    while ((dit = readdir(dip)) != NULL) {
        string dirEntry = dit->d_name;
        if (dirEntry.compare(0, match_prefix.size(), match_prefix) == 0) {
            oss << dirEntry << endl;
            contents[dirEntry] = dirEntry;
        }
    }

    closedir(dip);
    return oss.str();
}

class cacheT : public TestFixture {

public:
    cacheT() = default;
    ~cacheT() = default;

    void setUp() override {
        string bes_conf = (string)TEST_SRC_DIR + "/cacheT_bes.keys";
        TheBESKeys::ConfigFile = bes_conf;

        if (bes_debug) {
            BESDebug::SetUp("cerr,cache");
            DBG(cerr << "setup() - BESDEBUG Enabled " << endl);
        }
    }

    void tearDown() override {
        purge_cache(TEST_CACHE_DIR, CACHE_PREFIX);
        purge_cache(CACHE_DIR_TEST_64, CACHE_PREFIX);
    }

    // Fill the test directory with files summing > 4Gb.
    // First calls clean in case the thing exists.
    // TODO HACK This really needs to be cleaned up
    // to not use system but instead make the actual
    // C calls to make the files so it can checkl errno
    // and report the problems and make sure the test is
    // valid!
    void init_64_bit_cache(const std::string &cacheDir) {
        DBG(cerr << __func__ << "() - BEGIN " << endl);
        DBG(cerr << __func__ << "() - Called with dir = " << cacheDir << endl);
        DBG(cerr << __func__ << "() - Note: this makes large files and might take a while!" << endl);

        // 0) Make sure it's not there
        DBG(cerr << __func__ << "() - Cleaning old test cache..." << endl);
        clean_64_bit_cache(cacheDir);
        DBG(cerr << __func__ << "() -  ... done." << endl);

        BESFileLockingCache cache64(CACHE_DIR_TEST_64, CACHE_PREFIX, MAX_CACHE_SIZE_IN_MEGS_TEST_64);

        // 1) make the dir and files.
        string mkdirCmd = string("mkdir -p ") + cacheDir;
        DBG(cerr << __func__ << "() - Shell call: " << mkdirCmd << endl);
        system(mkdirCmd.c_str());

        DBG(cerr << __func__ << "() - Making " << NUM_FILES_TEST_64 << " files..." << endl);
        for (unsigned int i = 0; i < NUM_FILES_TEST_64; ++i) {
            std::stringstream fss;
            fss << cacheDir << "/" << MATCH_PREFIX << "_file_" << i << ".txt";
            DBG(cerr << __func__ << "() - Creating filename=" << fss.str()
                     << " of size (mb) = " << FILE_SIZE_IN_MEGS_TEST_64 << endl);
            std::stringstream mkfileCmdSS;
            mkfileCmdSS << "mkfile -n " << FILE_SIZE_IN_MEGS_TEST_64 << "m" << " " << fss.str();
            DBG(cerr << __func__ << "() - Shell call: " << mkfileCmdSS.str() << endl);
            system(mkfileCmdSS.str().c_str());
            DBG(cerr << __func__ << "() - " << "Done making file. Updating cache control file..." << endl);
            cache64.update_cache_info(fss.str());
        }
        DBG(cerr << __func__ << "() - " << "END " << endl);
    }

    // Clean out all those giant temp files
    void clean_64_bit_cache(const std::string &cacheDir) {
        DBG(cerr << __func__ << "() - BEGIN " << endl);
        std::string rmCmd = string("rm -r") + string(debug ? "v" : "") + string("f ") + cacheDir;
        DBG(cerr << __func__ << "() - Shell call: " << rmCmd << endl);
        system(rmCmd.c_str());
        DBG(cerr << __func__ << "() - END " << endl);
    }

    // Test function to test the cache on larger than 4G
    // (ie > 32 bit)
    // sum total of files sizes, which was a bug.
    // Since this takes a lot of time and space to do,
    // we'll make it optional somehow...
    void do_test_64_bit_cache() {
        DBG(cerr << __func__ << "() - BEGIN " << endl);
        // create a directory with a bunch of large files
        // to init the cache
        init_64_bit_cache(CACHE_DIR_TEST_64);

        // 1) make the cache
        // 2) get the info and make sure size is too big
        // 3) call purge
        // 4) get info again and make sure size is not too big

        // 1) Make the cache
        DBG(cerr << __func__
                 << "() - "
                    "Making a BESFileLockingCache with dir="
                 << CACHE_DIR_TEST_64 << " and prefix=" << CACHE_PREFIX
                 << " and max size (mb)= " << MAX_CACHE_SIZE_IN_MEGS_TEST_64 << endl);

        BESFileLockingCache cache64(CACHE_DIR_TEST_64, CACHE_PREFIX, MAX_CACHE_SIZE_IN_MEGS_TEST_64);

        // Make sure we have a valid test dir
        DBG(cerr << __func__
                 << "() - "
                    "cache64.get_cache_size: "
                 << cache64.get_cache_size() << endl);

        DBG(cerr << __func__
                 << "() - "
                    "cache64.d_max_cache_size_in_bytes: "
                 << cache64.d_max_cache_size_in_bytes << endl);

        DBG(cerr << __func__
                 << "() - "
                    "MAX_CACHE_SIZE_IN_BYTES_TEST_64: "
                 << MAX_CACHE_SIZE_IN_BYTES_TEST_64 << endl);

        CPPUNIT_ASSERT(cache64.d_max_cache_size_in_bytes == MAX_CACHE_SIZE_IN_BYTES_TEST_64);

        // Call purge which should delete enough to make it smaller
        DBG(cerr << __func__ << "() - Calling purge() on cache..." << endl);
        cache64.update_and_purge("");

        DBG(cerr << __func__ << "() - Checking that size is smaller than max..." << endl);
        CPPUNIT_ASSERT(cache64.get_cache_size() <= MAX_CACHE_SIZE_IN_BYTES_TEST_64);

        DBG(cerr << __func__ << "() - Test complete, cleaning test cache dir..." << endl);

        // Delete the massive empty files
        clean_64_bit_cache(CACHE_DIR_TEST_64);
        DBG(cerr << __func__ << "() - END " << endl);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////
    // CPPUNIT

    void test_empty_cache_dir_name_cache_creation() {
        DBG(cerr << endl << __func__ << "() - BEGIN " << endl);
        try {
            // Should return a disabled cache if the dir is "";
            BESFileLockingCache cache("", CACHE_PREFIX, 0);
            DBG(cerr << __func__ << "() - Cache is " << (cache.cache_enabled() ? "en" : "dis") << "abled." << endl);
            CPPUNIT_ASSERT(!cache.cache_enabled());

        } catch (BESError &e) {
            CPPUNIT_FAIL("Failed to create disabled cache. msg: " + e.get_verbose_message());
        }
        DBG(cerr << __func__ << "() - END " << endl);
    }

    void test_missing_cache_dir_cache_creation() {
        DBG(cerr << endl << __func__ << "() - BEGIN " << endl);
        // Only run this test if you don't have access (mainly for in case something is built in root)
        if (access("/", W_OK) != 0) {
            CPPUNIT_ASSERT_THROW_MESSAGE("Expected an exception - should not be able to create cache in root dir",
                                         BESFileLockingCache cache("/dummy", CACHE_PREFIX, 0), BESInternalError);
        }
        DBG(cerr << __func__ << "() - END " << endl);
    }

    void test_empty_prefix_name_cache_creation() {
        DBG(cerr << endl << __func__ << "() - BEGIN " << endl);
        try {
            BESFileLockingCache cache(TEST_CACHE_DIR, "", 1);
            CPPUNIT_ASSERT(!"Created cache with empty prefix");
        } catch (BESError &e) {
            DBG(cerr << __func__ << "() - Unable to create cache with empty prefix. "
                     << "That's good. msg: " << e.get_message() << endl);
        }
        DBG(cerr << __func__ << "() - END " << endl);
    }

    void test_size_zero_cache_creation() {
        DBG(cerr << endl << __func__ << "() - BEGIN " << endl);
        try {
            BESFileLockingCache cache(TEST_CACHE_DIR, CACHE_PREFIX, 0);
            CPPUNIT_ASSERT("Created cache with 0 size - correct given new behavior for this class");
        } catch (BESError &e) {
            DBG(cerr << __func__ << "() - Unable to create cache with 0 size. "
                     << "That's good. msg: " << e.get_message() << endl);
            CPPUNIT_FAIL(
                "Could not make cache with zero size - new behavior uses this as an unbounded cache - it should work");
        }
        DBG(cerr << __func__ << "() - END " << endl);
    }

    void test_good_cache_creation() {
        DBG(cerr << endl << __func__ << "() - BEGIN " << endl);
        try {
            BESFileLockingCache cache(TEST_CACHE_DIR, CACHE_PREFIX, 1);
            DBG(cerr << __func__ << "() - Cache is " << (cache.cache_enabled() ? "en" : "dis") << "abled." << endl);
            CPPUNIT_ASSERT(cache.cache_enabled());

        } catch (BESError &e) {
            DBG(cerr << __func__ << "() - FAILED to create cache! msg: " << e.get_message() << endl);
            CPPUNIT_ASSERT(!"Failed to create cache");
        }
        DBG(cerr << __func__ << "() - END " << endl);
    }

    void test_check_cache_for_non_existent_compressed_file() {
        DBG(cerr << endl << __func__ << "() - BEGIN " << endl);
        try {
            int fd;
            BESFileLockingCache cache(TEST_CACHE_DIR, CACHE_PREFIX, 1);
            string no_such_file = BESUtil::assemblePath(TEST_CACHE_DIR, "dummy.nc.gz");
            DBG(cerr << __func__
                     << "() - Attempting to acquire a read lock on "
                        "non-existent file."
                     << endl);
            bool success = cache.get_read_lock(no_such_file, fd);
            DBG(cerr << __func__ << "() - cache.get_read_lock() returned " << (success ? "true" : "false") << endl);
            if (success) {
                DBG(cerr << __func__
                         << "() - OUCH! That shouldn't have worked! "
                            "Releasing lock and closing file before we ASSERT... "
                         << endl);
                cache.unlock_and_close(no_such_file);
            }
            CPPUNIT_ASSERT(!success);
        } catch (BESError &e) {
            DBG(cerr << __func__ << "() - FAILED checking if non-exist file cached! msg: " << e.get_message() << endl);
            CPPUNIT_ASSERT(!"Error checking if non-exist file cached");
        }
        DBG(cerr << __func__ << "() - END " << endl);
    }

    void test_find_exisiting_cached_file() {
        DBG(cerr << endl << __func__ << "() - BEGIN " << endl);
        try {
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
        } catch (BESError &e) {
            DBG(cerr << __func__ << "() - FAILED checking for cached file! msg: " << e.get_message() << endl);
            CPPUNIT_ASSERT(!"Error checking if expected file is cached");
        }
        DBG(cerr << __func__ << "() - END " << endl);
    }

    void test_cache_purge() {
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
            check_cache(TEST_CACHE_DIR, "bes_cache#usr#local#data#template01.txt" /*latest_cache_file*/, 4);
        } catch (BESError &e) {
            CPPUNIT_FAIL("purge failed: " + e.get_message());
        }

        DBG(cerr << __func__ << "() - Test purge (should not remove any files)" << endl);

        try {
            BESFileLockingCache cache(TEST_CACHE_DIR, CACHE_PREFIX, 1);
            string latest_cache_file = cache.get_cache_file_name(latest_file);
            cache.update_and_purge(latest_cache_file);
            check_cache(TEST_CACHE_DIR, "bes_cache#usr#local#data#template01.txt" /*latest_cache_file*/, 4);
        } catch (BESError &e) {
            CPPUNIT_FAIL("purge failed: " + e.get_message());
        }

        DBG(cerr << __func__ << "() - END " << endl);
    }

    void test_64_bit_cache_sizes() {
        if (RUN_64_BIT_CACHE_TEST) {
            DBG(cerr << endl << __func__ << "() - BEGIN " << endl);
            try {
                do_test_64_bit_cache();
            } catch (BESError &e) {
                DBG(cerr << __func__ << "Caught BESError. msg: " << e.get_message() << endl);
                CPPUNIT_ASSERT(!"64 bit tests failed");
            }
            DBG(cerr << __func__ << "() - END " << endl);
        }
    }

    CPPUNIT_TEST_SUITE(cacheT);

    CPPUNIT_TEST(test_empty_cache_dir_name_cache_creation);
    CPPUNIT_TEST(test_missing_cache_dir_cache_creation);
    CPPUNIT_TEST(test_size_zero_cache_creation);
    CPPUNIT_TEST(test_good_cache_creation);
    CPPUNIT_TEST(test_check_cache_for_non_existent_compressed_file);
    CPPUNIT_TEST(test_find_exisiting_cached_file);
    CPPUNIT_TEST(test_cache_purge);
    CPPUNIT_TEST(test_64_bit_cache_sizes);

    CPPUNIT_TEST_SUITE_END();
};

// test fixture class

CPPUNIT_TEST_SUITE_REGISTRATION(cacheT);

int main(int argc, char *argv[]) {
    int option_char;
    bool purge = true; // the default
    while ((option_char = getopt(argc, argv, "db6ph")) != -1)
        switch (option_char) {
        case 'd':
            debug = true; // debug is a static global
            break;

        case 'b':
            bes_debug = true; // bes_debug is a static global
            break;

        case '6':
            // FIXME Not used
            RUN_64_BIT_CACHE_TEST = true; // RUN_64_BIT_CACHE_TEST is a static global
            DBG(cerr << __func__ << "() - 64 Bit Cache Tests Enabled." << endl);
            break;

        case 'p':
            purge = false;
            break;

        case 'h': { // help - show test names
            cerr << "Usage: cacheT has the following tests:" << endl;
            const std::vector<Test *> &tests = cacheT::suite()->getTests();
            unsigned int prefix_len = cacheT::suite()->getName().append("::").size();
            for (std::vector<Test *>::const_iterator i = tests.begin(), e = tests.end(); i != e; ++i) {
                cerr << (*i)->getName().replace(0, prefix_len, "") << endl;
            }
            break;
        }

        default:
            break;
        }

    argc -= optind;
    argv += optind;

    // Do this AFTER we process the command line so debugging in the test constructor
    // (which does a one time construction of the test cache) will work.

    init_cache(TEST_CACHE_DIR);

    CppUnit::TextTestRunner runner;
    runner.addTest(CppUnit::TestFactoryRegistry::getRegistry().makeTest());

    bool wasSuccessful = true;
    string test = "";
    if (0 == argc) {
        // run them all
        wasSuccessful = runner.run("");
    } else {
        int i = 0;
        while (i < argc) {
            if (debug)
                cerr << "Running " << argv[i] << endl;
            test = cacheT::suite()->getName().append("::").append(argv[i]);
            wasSuccessful = wasSuccessful && runner.run(test);
            ++i;
        }
    }

    if (purge)
        purge_cache(TEST_CACHE_DIR, CACHE_PREFIX);

    return wasSuccessful ? 0 : 1;
}
