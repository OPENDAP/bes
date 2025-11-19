// cacheT.C

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2017 University Corporation for Atmospheric Research
// Author: Nathan Potter <ndp@opendap.org>
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

#include "config.h"

#include <iostream>
#include <sstream>
#include <string>

#include <cstdio> /* printf */
#include <cstdlib>
#include <ctime>

#include <unistd.h> // for sleep

#include "BESDebug.h"
#include "BESError.h"
#include "BESFileLockingCache.h"
#include "BESUtil.h"
#include "TheBESKeys.h"

#include "test_config.h"

using namespace std;

#define CACHE_PREFIX "flc_"
#define LOCK_TEST_FILE "lock_test"
#define TEST_CACHE_DIR TEST_BUILD_DIR "/cache"

bool debug = false;
bool bes_debug = false;
#undef DBG
#define DBG(x)                                                                                                         \
    do {                                                                                                               \
        if (debug)                                                                                                     \
            (x);                                                                                                       \
    } while (false)
#define prolog string("FileLockingCacheTest::").append(__func__).append("() - ")

void run_sys(const string &cmd) {
    int status;
    DBG(cerr << prolog << " command: '" << cmd);
    status = system(cmd.c_str());
    DBG(cerr << "' status: " << status << endl);
}

/**
 * Purge the cache by removing the contents of the directory.
 */
int purge_cache(const string &cache_dir, const string &cache_prefix) {
    DBG(cerr << prolog << "BEGIN " << endl);

    ostringstream s;
    s << "rm -" << (debug ? "v" : "") << "f " << BESUtil::assemblePath(cache_dir, cache_prefix) << "*";
    DBG(cerr << prolog << "command: '" << s.str() << "' " << endl);
    int status = system(s.str().c_str());
    DBG(cerr << prolog << "status: " << status << endl);
    DBG(cerr << prolog << "END " << endl);
    return status;
}

class FileLockingCacheTest {
private:
public:
    FileLockingCacheTest() = default;

    ~FileLockingCacheTest() = default;

    /**
     * Get a read lock for given time on a file. By default the file is the
     * LOCK_TEST_FILE file defined as a global above.
     */
    int get_and_hold_read_lock(long int nap_time, const string &file_name = LOCK_TEST_FILE,
                               const string &cache_dir = TEST_CACHE_DIR) {
        DBG(cerr << endl << prolog << "BEGIN " << endl);

        BESFileLockingCache cache(cache_dir, CACHE_PREFIX, 1);
        DBG(cerr << prolog << "Made FLC object. d_cache_info_fd: " << cache.d_cache_info_fd << endl);

        string cache_file_name = cache.get_cache_file_name(file_name);
        int fd = 0;

        DBG(cerr << prolog << "cache file name: " << cache_file_name << endl);
        time_t start = time(nullptr); /* get current time; same as: timer = time(NULL)  */
        DBG(cerr << prolog << "Read lock REQUESTED @" << start << endl);
        bool locked = cache.get_read_lock(cache_file_name, fd);
        time_t stop = time(nullptr);
        DBG(cerr << prolog << "cache.get_read_lock() returned " << (locked ? "TRUE" : "FALSE") << " (fd: " << fd << ")"
                 << endl);

        DBG(cerr << prolog << "cache.d_cache_info_fd: " << cache.d_cache_info_fd << endl);

        if (!locked) {
            DBG(cerr << prolog << "END - FAILED to get read lock on " << cache_file_name << endl);
            return 2;
        }

        DBG(cerr << prolog << "Read lock  ACQUIRED @" << stop << endl);
        DBG(cerr << prolog << "Lock acquisition took " << stop - start << " seconds." << endl);
        DBG(cerr << prolog << "Holding lock for " << nap_time << " seconds" << endl);

        sleep(nap_time);
        cache.unlock_and_close(cache_file_name);

        DBG(cerr << prolog << "Lock Released" << endl);
        DBG(cerr << prolog << "END - SUCCEEDED" << endl);
        return 0;
    }

    /**
     * Create and lock a file. By default, the file is the LOCK_TEST_FILE defined above.
     */
    int get_and_hold_exclusive_lock(long int nap_time, const string &file_name = LOCK_TEST_FILE,
                                    const string &cache_dir = TEST_CACHE_DIR) {
        DBG(cerr << endl << prolog << "BEGIN " << endl);
        try {
            BESFileLockingCache cache(cache_dir, CACHE_PREFIX, 1);
            DBG(cerr << prolog << "Made FLC object. d_cache_info_fd: " << cache.d_cache_info_fd << endl);
            int fd;
            string cache_file_name = cache.get_cache_file_name(file_name);

            time_t start = time(nullptr);
            DBG(cerr << prolog << "Exclusive lock REQUESTED @" << start << endl);
            bool locked = cache.create_and_lock(cache_file_name, fd);
            time_t stop = time(nullptr);
            DBG(cerr << prolog << "cache.create_and_lock() returned " << (locked ? "true" : "false") << endl);
            if (!locked) {
                cerr << "Failed to get exclusive lock on " << cache_file_name << endl;
                return 1;
            }
            DBG(cerr << prolog << "Exclusive lock  ACQUIRED @" << stop << endl);
            DBG(cerr << prolog << "Lock acquisition took " << stop - start << " seconds." << endl);
            DBG(cerr << prolog << "Holding lock for " << nap_time << " seconds" << endl);
            for (long int i = 0; i < nap_time; i++) {
                write(fd, ".", 1);
                sleep(1);
            }
            cache.unlock_and_close(cache_file_name);
            DBG(cerr << prolog << "Lock Released" << endl);
        } catch (BESError &e) {
            DBG(cerr << prolog << "FAILED to create cache! msg: " << e.get_message() << endl);
        }
        return 0;
        DBG(cerr << prolog << "END " << endl);
    }
};

const string version = "FileLockingCacheTest: 1.0";

/**
 * Performs each of the tasks indicated by the command line parameters.
 * Tasks are performed in the order they are processed from the command line.
 * If a task returns a non-zero value then the program exits.
 *
 */
int main(int argc, char *argv[]) {
    FileLockingCacheTest flc_test;

    int option_char;
    long int time;
    string file_name = LOCK_TEST_FILE;
    string cache_dir = TEST_CACHE_DIR;
    while ((option_char = getopt(argc, argv, "vdb:pr:x:hf:c:")) != EOF) {
        switch (option_char) {
        case 'v':
            cerr << version << endl;
            exit(0);

        case 'd':
            debug = true; // debug is a static global
            cerr << "Debug enabled." << endl;
            break;

        case 'b':
            bes_debug = true; // bes_debug is a static global
            BESDebug::SetUp(string(optarg));
            cerr << "BESDEBUG is enabled." << endl;
            break;

        case 'f':
            file_name = optarg;
            DBG(cerr << prolog << "Using filename: " << file_name << endl);
            break;

        case 'c':
            cache_dir = BESUtil::assemblePath(TEST_BUILD_DIR, optarg);
            DBG(cerr << prolog << "Using cache dir: " << cache_dir << endl);
            break;

        case 'p': {
            DBG(cerr << prolog << "Purging cache directory: " << cache_dir << " cache_prefix: " << CACHE_PREFIX
                     << endl);
            int status;
            status = purge_cache(cache_dir, CACHE_PREFIX);
            if (status) {
                cerr << prolog << "purge_cache() FAILED. status: " << status << endl;
                return status;
            }
            cerr << prolog << "purge_cache() completed." << endl;
            break;
        }

        case 'r': {
            std::istringstream(optarg) >> time;
            DBG(cerr << prolog << "Get and hold lock for " << time << " seconds" << endl);
            int status;
            status = flc_test.get_and_hold_read_lock(time, file_name, cache_dir);
            if (status) {
                return status;
            }
            break;
        }

        case 'x': {
            std::istringstream(optarg) >> time;
            DBG(cerr << prolog << "Get and hold exclusive lock for " << time << " seconds." << endl);
            int status;
            status = flc_test.get_and_hold_exclusive_lock(time, file_name, cache_dir);
            if (status) {
                return status;
            }
            break;
        }

        case 'h':
        default: {
            cerr << "" << endl;
            cerr << "FileLockingCacheTest            (BES Dispatch)             FileLockingCacheTest" << endl;
            cerr << "" << endl;
            cerr << "Name" << endl;
            cerr << "    FileLockingCacheTest -- Test a systems advisory file locking capability." << endl;
            cerr << "" << endl;
            cerr << "Description" << endl;
            cerr << "    FileLockingCacheTest [-d][-b bes_debug_string][-p][-c cache_dir][-r time][-x time][-h]"
                 << endl;
            cerr << "    -x time      -- Get and hold an exclusive write lock for 'time' seconds." << endl;
            cerr << "    -r time      -- Get and hold a shared read lock for 'time' seconds." << endl;
            cerr << "    -f file name -- Lock this file. If not given, uses a default name." << endl;
            cerr << "    -p           -- Purge cache files." << endl;
            cerr << "    -c dir_name  -- Use the directory dir_name for the cache test." << endl;
            cerr << "    -d           -- Enable test debugging output." << endl;
            cerr << "    -b str       -- Configure BES debugging with the string 'str'." << endl;
            cerr << "    -h           -- Show this message." << endl;
            cerr << "" << endl;
            cerr << "NOTE: The entire command line is evaluated and each item is executed" << endl;
            cerr << "in the order that it appears on the command line." << endl;
            cerr << "" << endl;
            cerr << "Simple Use:" << endl;
            cerr << "    # Get and hold an exclusive write lock for 10 seconds." << endl;
            cerr << "       FileLockingCacheTest -x 10" << endl;
            cerr << "    # Get and hold a shared read lock for 5 seconds." << endl;
            cerr << "       FileLockingCacheTest -r 5" << endl;
            cerr << "    # Get and hold a shared read lock for 5 seconds on the file 'foobar'." << endl;
            cerr << "       FileLockingCacheTest -f foobar -r 5" << endl;
            cerr << "" << endl;
            cerr << "Example:" << endl;
            cerr << "    # Purge cache, Get and hold an exclusive write lock for 10 seconds," << endl;
            cerr << "    # get and hold shared read lock for 5 seconds." << endl;
            cerr << "       FileLockingCacheTest -p -x 10 -r 5" << endl;
            cerr << "" << endl;
            cerr << "Debugging Example:" << endl;
            cerr << "    # Enable all debugging." << endl;
            cerr << "    # Purge test files from cache." << endl;
            cerr << "    # Get and hold an exclusive write lock for 10 seconds." << endl;
            cerr << "       FileLockingCacheTest -d -b \"cerr,all\" -p -x 10" << endl;
            cerr << "" << endl;
            cerr << "FileLockingCacheTest                 (END)                 FileLockingCacheTest" << endl;
            cerr << "" << endl;
            break;
        }
        }
    }

    return 0;
}
