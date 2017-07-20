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
    }
    ~FileLockingCacheTest()
    {
    }

    void get_and_hold_read_lock(long int nap_time)
    {
        DBG(cerr << endl << __func__ << "() - BEGIN " << endl);

        BESFileLockingCache cache(TEST_CACHE_DIR, CACHE_PREFIX, 1);
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
        if(!locked){
            DBG(cerr << __func__ << "() - END - FAILED to get read lock on " << cache_file_name << endl);
            return;
            //throw BESError("Failed to get read lock on "+cache_file_name, BES_INTERNAL_ERROR, __FILE__,__LINE__);
        }


        DBG(cerr << __func__ << "() - Read lock  ACQUIRED @" << stop << endl);
        DBG(cerr << __func__ << "() - Lock acquisition took " << stop - start << " seconds." << endl);
        DBG(cerr << __func__ << "() - Holding lock for " << nap_time << " seconds" << endl);
        sleep(nap_time);
        cache.unlock_and_close(cache_file_name);
        DBG(cerr << __func__ << "() - Lock Released" << endl);
        DBG(cerr << __func__ << "() - END - SUCCEEDED" << endl);
    }

    void get_and_hold_exclusive_lock(long int nap_time)
    {
        DBG(cerr << endl << __func__ << "() - BEGIN " << endl);
        try {
            BESFileLockingCache cache(TEST_CACHE_DIR, CACHE_PREFIX, 1);
            int fd;
            string cache_file_name = cache.get_cache_file_name(LOCK_TEST_FILE);

            time_t start = time(NULL);
            DBG(cerr << __func__ << "() - Exclusive lock REQUESTED @" << start << endl);
            bool locked = cache.create_and_lock(cache_file_name,fd);
            time_t stop = time(0);
            DBG(cerr << __func__ << "() - cache.create_and_lock() returned " << (locked ? "true" : "false") << endl);
            if(!locked)
                throw BESError("Failed to get exclusive lock on "+cache_file_name,
                    BES_INTERNAL_ERROR, __FILE__,__LINE__);
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
        DBG(cerr << __func__ << "() - END " << endl);
    }

};

// test fixture class
int main(int argc, char*argv[])
{
    FileLockingCacheTest flc_test;

    GetOpt getopt(argc, argv, "db:pr:x:h");
    int option_char;
    long int time;
    while ((option_char = getopt()) != EOF) {
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
            cerr << "get_and_hold_read_lock for " << time << " seconds" << endl;
            flc_test.get_and_hold_read_lock(time);
            cerr << "get_and_hold_read_lock DONE" << endl;
            break;

        case 'p':
            cerr << "purging cache dir: " << TEST_CACHE_DIR << " cache_prefix: "<< CACHE_PREFIX << endl;
            purge_cache(TEST_CACHE_DIR,CACHE_PREFIX);
            cerr << "purge_cache DONE" << endl;
            break;

        case 'x':
            std::istringstream(getopt.optarg) >> time;
            cerr << "get_and_hold_exclusive_lock for " << time << " seconds." << endl;
            flc_test.get_and_hold_exclusive_lock(time);
            cerr << "get_and_hold_exclusive_lock DONE" << endl;
            break;

        case 'h':
        default:
            cerr << "A usage statement would be nice." << endl;
            break;
        }
    }

    return 0;
}

