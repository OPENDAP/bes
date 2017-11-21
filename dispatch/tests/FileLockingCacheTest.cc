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

#include <unistd.h>  // for sleep
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>  // for closedir opendir

#include <iostream>
#include <sstream>
#include <cstdlib>
#include <cstring>
#include <string>
#include <iostream>

#include <stdio.h>      /* printf */
#include <time.h>
#include <ctime>

#include <GetOpt.h> // I think this is an error

#include "TheBESKeys.h"
#include "BESError.h"
#include "BESDebug.h"
#include "BESUtil.h"
#include "BESFileLockingCache.h"

#include "FileLockingCacheTest.h"

#include "test_config.h"

using namespace std;

static const std::string CACHE_PREFIX = string("flc_");
static const std::string MATCH_PREFIX = string(CACHE_PREFIX) + string("#");
static const std::string TEST_CACHE_DIR = BESUtil::assemblePath(TEST_SRC_DIR, "cache");

static const std::string LOCK_TEST_FILE = std::string("lock_test");

static bool debug = false;
static bool bes_debug = false;
#undef DBG
#define DBG(x) do { if (debug) (x); } while(false);

void run_sys(const string cmd)
{
    int status;
    DBG(cerr << __func__ << " command: '" << cmd);
    status = system(cmd.c_str());
    DBG(cerr << "' status: " << status << endl);
}

void purge_cache(const string &cache_dir, const string &cache_prefix)
{
    DBG(cerr << __func__ << "() - BEGIN " << endl);
    ostringstream s;
    s << "rm -" << (debug ? "v" : "") << "f " << BESUtil::assemblePath(cache_dir, cache_prefix) << "*";
    DBG(cerr << __func__ << "() - cmd: '" << s.str() << "' ");
    int status = system(s.str().c_str());
    DBG(cerr << "status: " << status << endl);
    DBG(cerr << __func__ << "() - END " << endl);
}

class FileLockingCacheTest {
private:

public:
    FileLockingCacheTest()
    {
        // string bes_conf = (string) TEST_ABS_SRC_DIR + "/flct_bes.keys";
        // TheBESKeys::ConfigFile = bes_conf;

    }
    ~FileLockingCacheTest()
    {
    }

    int get_and_hold_read_lock(long int nap_time)
    {
        DBG(cerr << endl << __func__ << "() - BEGIN " << endl);

        BESFileLockingCache cache(TEST_CACHE_DIR, CACHE_PREFIX, 1);
        DBG(cerr << __func__ << "() - Made FLC object. d_cache_info_fd: " << cache.d_cache_info_fd << endl);

        string cache_file_name = cache.get_cache_file_name(LOCK_TEST_FILE);
        int fd=0;

        DBG(cerr << __func__ << "() - cache file name:" << cache_file_name << endl);
        time_t start = time(NULL);  /* get current time; same as: timer = time(NULL)  */
        DBG(cerr << __func__ << "() - Read lock REQUESTED @" << start << endl);
        bool locked = cache.get_read_lock(cache_file_name, fd);
        time_t stop = time(0);
        DBG(cerr << __func__ << "() - cache.get_read_lock() returned " << (locked ? "TRUE" : "FALSE")
                << " (fd: " << fd  << ")"<< endl);
#if 0
        DBG(cerr << __func__ << "() - cache file name: " << cache_file_name << endl);
        DBG(cerr << __func__ << "() - BES_INTERNAL_ERROR: " << BES_INTERNAL_ERROR << endl);
        DBG(cerr << __func__ << "() - __FILE__: " << __FILE__ << endl);
        DBG(cerr << __func__ << "() - __LINE__: " << __LINE__ << endl);
#endif

        DBG(cerr << __func__ << "() - cache.d_cache_info_fd: " << cache.d_cache_info_fd << endl);

        if(!locked){
            DBG(cerr << __func__ << "() - END - FAILED to get read lock on " << cache_file_name << endl);
            return 2;
            //throw BESError("Failed to get read lock on "+cache_file_name, BES_INTERNAL_ERROR, __FILE__,__LINE__);
        }


        DBG(cerr << __func__ << "() - Read lock  ACQUIRED @" << stop << endl);
        DBG(cerr << __func__ << "() - Lock acquisition took " << stop - start << " seconds." << endl);
        DBG(cerr << __func__ << "() - Holding lock for " << nap_time << " seconds" << endl);
        sleep(nap_time);
        cache.unlock_and_close(cache_file_name);
        DBG(cerr << __func__ << "() - Lock Released" << endl);
        DBG(cerr << __func__ << "() - END - SUCCEEDED" << endl);
        return 0;
    }

    int get_and_hold_exclusive_lock(long int nap_time)
    {
        DBG(cerr << endl << __func__ << "() - BEGIN " << endl);
        try {
            BESFileLockingCache cache(TEST_CACHE_DIR, CACHE_PREFIX, 1);
            DBG(cerr << __func__ << "() - Made FLC object. d_cache_info_fd: " << cache.d_cache_info_fd << endl);
            int fd;
            string cache_file_name = cache.get_cache_file_name(LOCK_TEST_FILE);

            time_t start = time(NULL);
            DBG(cerr << __func__ << "() - Exclusive lock REQUESTED @" << start << endl);
            bool locked = cache.create_and_lock(cache_file_name,fd);
            time_t stop = time(0);
            DBG(cerr << __func__ << "() - cache.create_and_lock() returned " << (locked ? "true" : "false") << endl);
            if(!locked){
                cerr << "Failed to get exclusive lock on " << cache_file_name << endl;
                return 1;
            }
            DBG(cerr << __func__ << "() - Exclusive lock  ACQUIRED @" << stop << endl);
            DBG(cerr << __func__ << "() - Lock acquisition took " << stop - start << " seconds." << endl);
            DBG(cerr << __func__ << "() - Holding lock for " << nap_time << " seconds" << endl);
            for(long int i=0; i<nap_time ;i++){
                write(fd,".", 1);
                sleep(1);
            }
            cache.unlock_and_close(cache_file_name);
            DBG(cerr << __func__ << "() - Lock Released" << endl);
        }
        catch (BESError &e) {
            DBG(cerr << __func__ << "() - FAILED to create cache! msg: " << e.get_message() << endl);
        }
        return 0;
        DBG(cerr << __func__ << "() - END " << endl);
    }

};


/**
 * Performs each of the tasks indicated by the command line parameters.
 * Tasks are performed in the order they are processed from the command line.
 * If a task returns a non-zero value then the program exits.
 *
 */
int main(int argc, char*argv[])
{
    FileLockingCacheTest flc_test;

    GetOpt getopt(argc, argv, "db:pr:x:h");
    int option_char;
    long int time;
    int retVal=0;
    while (!retVal && (option_char = getopt()) != EOF) {
        switch (option_char) {
        case 'd':
            debug = true;  // debug is a static global
            cerr << "Debug enabled." << endl;
            break;

        case 'b':
            bes_debug = true;  // bes_debug is a static global
            BESDebug::SetUp(string(getopt.optarg));
            cerr << "BES debug enabled." << endl;
            break;

        case 'r':
            std::istringstream(getopt.optarg) >> time;
            DBG(cerr << __func__ << "() - get_and_hold_read_lock for " << time << " seconds" << endl);
            retVal = flc_test.get_and_hold_read_lock(time);
            break;

        case 'p':
            DBG(cerr << __func__ << "() - purging cache dir: " << TEST_CACHE_DIR << " cache_prefix: "<< CACHE_PREFIX << endl);
            purge_cache(TEST_CACHE_DIR,CACHE_PREFIX);
            cerr << "purge_cache DONE" << endl;
            break;

        case 'x':
            std::istringstream(getopt.optarg) >> time;
            DBG(cerr << __func__ << "() -  get_and_hold_exclusive_lock for " << time << " seconds." << endl);
            retVal = flc_test.get_and_hold_exclusive_lock(time);
            break;

        case 'h':
        default:
            cerr << "" << endl;
            cerr << "FileLockingCacheTest            (BES Dispatch)             FileLockingCacheTest" << endl;
            cerr << "" << endl;
            cerr << "Name" << endl;
            cerr << "    FileLockingCacheTest -- Test a systems advisory file locking capability." << endl;
            cerr << "" << endl;
            cerr << "Description" << endl;
            cerr << "    FileLockingCacheTest [-d][-b bes_debug_string][-p][-r time][-x time][-h]" << endl;
            cerr << "    -x time -- Get and hold an exclusive write lock for 'time' seconds." << endl;
            cerr << "    -r time -- Get and hold a shared read lock for 'time' seconds." << endl;
            cerr << "    -p      -- Purge cache files." << endl;
            cerr << "    -d      -- Enable test debugging output." << endl;
            cerr << "    -b str  -- Configure BES debugging with the string 'str'." << endl;
            cerr << "    -h      -- Show this message." << endl;
            cerr << "" << endl;
            cerr << "NOTE: The entire command line is evaluated and each item is executed" << endl;
            cerr << "in the order that it appears on the command line." << endl;
            cerr << "" << endl;
            cerr << "Simple Use:" << endl;
            cerr << "    # Get and hold an exclusive write lock for 10 seconds." << endl;
            cerr << "       FileLockingCacheTest -x 10" << endl;
            cerr << "    # Get and hold a shared read lock for 5 seconds." << endl;
            cerr << "       FileLockingCacheTest -r 5" << endl;
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
            cerr << "       FileLockingCacheTest -d b \"cerr,all\" -p -x 10" << endl;
            cerr << "" << endl;
            cerr << "FileLockingCacheTest                 (END)                 FileLockingCacheTest" << endl;
            cerr << "" << endl;
            break;
        }
    }

    return retVal;
}

