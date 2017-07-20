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

#include <GetOpt.h> // I think this is an error

#include "TheBESKeys.h"
#include "BESError.h"
#include "BESDebug.h"
#include "BESUtil.h"
#include "BESFileLockingCache.h"

#include "FileLockingCacheTest.h"

#include "test_config.h"

using namespace std;

// Not run by default!
// Set from the command-line invocation of the main only
// since we're not sure the OS has room for the test files.
static bool RUN_64_BIT_CACHE_TEST = false;

static const std::string CACHE_PREFIX = string("bes_cache");
static const std::string MATCH_PREFIX = string(CACHE_PREFIX) + string("#");

static const string TEST_CACHE_DIR = BESUtil::assemblePath(TEST_SRC_DIR, "cache");

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
void init_cache(const string &cache_dir)
{
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

        // DBG(cerr << __func__ << "() - sleeping for 1 second..." << endl);
        //sleep(1);
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
bool check_cache(const string &cache_dir, const string &should_be, unsigned int num_files)
{
    DBG(cerr << __func__ << "() - BEGIN, should_be: " << should_be << ", num_files: " << num_files << endl);

    map<string, string> contents;
    string match_prefix = MATCH_PREFIX;
    DIR *dip = opendir(cache_dir.c_str());
    if(dip==0){
        throw libdap::Error("Cache dir '"+cache_dir+"' does not exist.");
    }
    struct dirent *dit;
    while ((dit = readdir(dip)) != NULL) {
        string dirEntry = dit->d_name;
        if (dirEntry.compare(0, match_prefix.length(), match_prefix) == 0) contents[dirEntry] = dirEntry;
    }

    closedir(dip);

    bool found = false;
    if(num_files == contents.size()){
        for (map<string, string>::const_iterator ci = contents.begin(), ce = contents.end(); ci != ce; ci++) {
            DBG(cerr << "contents: " << (*ci).first << endl);
            if ((*ci).first == should_be) {
                found = true;
                break;
            }
        }
    }


    DBG(cerr << __func__ << "() - END " << endl);
    return found;
}

void purge_cache(const string &cache_dir, const string &cache_prefix)
{
    DBG(cerr << __func__ << "() - BEGIN " << endl);
    ostringstream s;
    s << "rm -" << (debug ? "v" : "") << "f " << BESUtil::assemblePath(cache_dir, cache_prefix) << "*";
    DBG(cerr << __func__ << "() - cmd: " << s.str() << endl);
    system(s.str().c_str());
    DBG(cerr << __func__ << "() - END " << endl);
}

string show_cache(const string cache_dir, const string match_prefix)
{
    map<string, string> contents;
    ostringstream oss;
    DIR *dip = opendir(cache_dir.c_str());

    if(dip==0){
        throw libdap::Error("Cache dir '"+cache_dir+"' does not exist.");
    }

    struct dirent *dit;
    while ((dit = readdir(dip)) != NULL) {
        string dirEntry = dit->d_name;
        if (dirEntry.compare(0, match_prefix.length(), match_prefix) == 0) {
            oss << dirEntry << endl;
            contents[dirEntry] = dirEntry;
        }
    }

    closedir(dip);
    return oss.str();
}

class FlieLockingCacheTest {
private:

public:
    FlieLockingCacheTest()
    {
    }
    ~FlieLockingCacheTest()
    {
    }

    void get_and_hold_read_lock(long int time)
    {
        DBG(cerr << endl << __func__ << "() - BEGIN " << endl);
        try {
            BESFileLockingCache cache(TEST_CACHE_DIR, CACHE_PREFIX, 1);
            string file_name = "/usr/local/data/template01.txt";
            string cache_file_name = cache.get_cache_file_name(file_name);
            int fd;

            bool locked = cache.get_read_lock(cache_file_name, fd);
            DBG(cerr << __func__ << "() - cache.get_read_lock() returned " << (locked ? "true" : "false") << endl);

            if(!locked)
                throw BESError("Failed to get read lock on "+cache_file_name,
                    BES_INTERNAL_ERROR, __FILE__,__LINE__);
            sleep(time);

            if (locked) {
                cache.unlock_and_close(cache_file_name);
            }


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
    GetOpt getopt(argc, argv, "dbrh");
    int option_char;
    bool purge = true;    // the default
    long int read_lock_and_hold = -1;
    while ((option_char = getopt()) != -1) {
        switch (option_char) {
        case 'd':
            debug = true;  // debug is a static global
            cerr << "Test debug enabled." << endl;
            break;

        case 'b':
            bes_debug = true;  // bes_debug is a static global
            cerr << "BES debug enabled." << endl;
            break;

        case 'r':
            read_lock_and_hold = std::stol(optarg);
            cerr << "read_lock_and_hold: " << read_lock_and_hold << endl;
            break;

        default:
            break;
        }
    }

    // Do this AFTER we process the command line so debugging in the test constructor
    // (which does a one time construction of the test cache) will work.

    init_cache(TEST_CACHE_DIR);

    bool wasSuccessful = true;
    FlieLockingCacheTest flc_test();

    if(read_lock_and_hold>=0)
        flc_test.get_and_hold_read_lock(read_lock_and_hold);


    string test = "";
    int i = getopt.optind;
    if (i == argc) {
        // run them all
        wasSuccessful = runner.run("");
    }
    else {
        while (i < argc) {
            if (debug) cerr << "Running " << argv[i] << endl;
            test = cacheT::suite()->getName().append("::").append(argv[i]);
            wasSuccessful = wasSuccessful && runner.run(test);
        }
    }

    if (purge) purge_cache(TEST_CACHE_DIR, CACHE_PREFIX);

    return wasSuccessful ? 0 : 1;
}

