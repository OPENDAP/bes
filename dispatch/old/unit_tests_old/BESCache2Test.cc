// Copyright (c) 2013 OPeNDAP, Inc. Author: James Gallagher
// <jgallagher@opendap.org>, Patrick West <pwest@opendap.org>
// Nathan Potter <npotter@opendap.org>
//                                                                            
// modify it under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation; either version 2.1 of
// the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// Lesser General Public License for more details.
//
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
// 02110-1301 U\ SA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI.
// 02874-0112.
/*
 * BESCache2Test.cc
 *
 *  Created on: Mar 23, 2012
 *      Author: jimg
 */

#include <time.h>
#include <dirent.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>

#include <sys/file.h>
#include <sys/stat.h>

#include <openssl/md5.h>

#include <vector>
#include <string>
#include <sstream>
#include <iomanip>

#include "BESCache2.h"
#include "BESUncompressManager2.h"
#include "BESError.h"
#include "BESInternalError.h"
#include "BESDebug.h"

using namespace std;

// build a list of all of the (compressed) data files and store their paths in a vector

// return a random int between 0 and N (0 < N)

// sleep for a random number of milliseconds

// decompression processes
// init cache
// init uncompress manager
// loop:
//  pick a name
//  sleep random time 0 < 4 seconds in milliseconds
//  uncompress the file
//  sleep random time 0 < 4 seconds in milliseconds

// main:
// build list of names
// fork M decompression processes

inline string get_errno() {
    char *s_err = strerror(errno);
    if (s_err)
        return s_err;
    else
        return "Unknown error.";
}

vector<string> get_file_names(const string &dir)
{
    DIR *dip = opendir(dir.c_str());
    if (!dip)
        throw BESInternalError("Unable to open cache directory " + dir, __FILE__, __LINE__);

    vector<string> file_names;
    struct dirent *dit;
    while ((dit = readdir(dip)) != NULL) {
        file_names.push_back(dit->d_name);
    }

    closedir(dip);

    return file_names;
}

void sleep(int milliseconds)
{
    //__darwin_time_t tv_sec;
    //long        tv_nsec;
    int seconds = milliseconds / 1000;
    long nanoseconds = (milliseconds - (seconds * 1000)) * 10e5;

    //cerr << "Seconds to sleep: "<< seconds << ", nanoseconds to sleep: " << nanoseconds << endl;

    struct timespec rqtp = { seconds, nanoseconds };

    nanosleep(&rqtp, 0);
}

// This should use a seed if really 'random' responses are need...
int random(int N)
{
    if (N < 1)
        throw BESInternalError("In random(), N must be > 0.", __FILE__, __LINE__);

    return rand() % N;
}

string get_md5(const string &file)
{
    int fd = open(file.c_str(), O_RDONLY);
    if (fd == -1)
        throw BESInternalError("Can't open for md5. " + get_errno() + " " + file, __FILE__, __LINE__);

    struct stat buf;
    int statret = stat(file.c_str(), &buf);
    if (statret != 0)
        throw BESInternalError("Can't stat for md5. " + get_errno() + " " + file, __FILE__, __LINE__);

    vector<unsigned char> data(buf.st_size);
    if (read(fd, data.data(), buf.st_size) != buf.st_size)
        throw BESInternalError("Can't read for md5. " + get_errno() + " " + file, __FILE__, __LINE__);

    close(fd);

    unsigned char *result = MD5(data.data(), buf.st_size, 0);
    ostringstream oss;
    oss.setf ( ios::hex, ios::basefield );
    for (int i = 0; i < MD5_DIGEST_LENGTH; ++i) {
        oss << setfill('0') << setw(2) << (unsigned int)result[i];
    }

    return oss.str();
}

void print_name(const string &name) {
    cerr << "Name: " << name << endl;;
}
bool dot_file(const string &file) {
    return file.at(0) == '.';
}

void decompression_process(int files_to_get, bool simulate_use = false)
{
    srand(time(0));

    BESDebug::SetUp("cerr,cache_purge,cache_contents,cache");

    // Make a cache object for this process. Hardwire the cache directory name
    BESCache2 *cache = BESCache2::get_instance("./cache2", "tc_", 200);

    // Get a list of all of the test files
    vector<string> files = get_file_names("./cache2_data_files");
    files.erase(remove_if(files.begin(), files.end(), dot_file), files.end());
    if (files.size() == 0)
        throw BESInternalError("No files in the data directory for the cache tests.", __FILE__, __LINE__);
    int num_files = files.size();

    std:map<string,string> md5;

#if 0
    cerr << "Files for the test: (" << getpid() << ")" << endl;
    for_each(files.begin(), files.end(), print_name);
#endif

    for (int i = 0; i < files_to_get; ++i) {
        // randomly choose a compressed file
        string file = files.at(random(num_files));

        // write its name to the log, along with the time
        time_t t;
        time(&t);
        BESDEBUG("uncompress", "Getting file '" << file << "' (time: " << t << ")." << endl);

        // get the file
        file = "./cache2_data_files/" + file;
        string cfile;
        bool in_cache = BESUncompressManager2::TheManager()->uncompress( file, cfile, cache ) ;

        struct stat buf;
        int statret = stat(cfile.c_str(), &buf);
        if (statret != 0)
            throw BESInternalError("Can't stat for size. " + string(get_errno()) + " " + cfile, __FILE__, __LINE__);

        if (buf.st_size == 0)
            throw BESInternalError("Zero-byte file. " + cfile, __FILE__, __LINE__);

        // sleep for up to one second
        if (simulate_use)
            sleep(random(1000));

        string hash = get_md5(cfile);
        if (md5[file] != "" && md5[file] != hash)
            throw BESInternalError("md5 failed " + cfile + ": md5[file]: " + md5[file] + ", hash: " + hash, __FILE__, __LINE__);
        else
            md5[file] = hash;

        time(&t);
        // write cfile to the log along with the time
        if (in_cache)
            BESDEBUG("uncompress", "    " << "done using the file, unlocking " << cfile << " (time: " << t  << ")" << endl);

        cache->unlock(cfile);
    }
}

int main(int argc, char *argv[])
{
    if (argc != 4) {
        cerr << "expected three arguments: # of child processes, # of files to decompress, 0/1 for simulated use." << endl;
        exit(1);
    }
    int num_children = atoi(argv[1]);
    int num_files = atoi(argv[2]);
    bool simulate_use = string(argv[3]) == "1" ? true: false;

    cerr << "Number of child processes: " << num_children << endl;
    cerr << "Number of files to decompress: " << num_files << endl;
    cerr << "Simulate use? " << simulate_use << endl;

    for (int i = 0; i < num_children; ++i) {
        int pid = fork();
        if (pid == -1) {
            cerr << "Could not fork!" << endl;
            exit(1);
        }
        if (pid == 0) {
            // child
            try {
                decompression_process(num_files, simulate_use);
            }
            catch (BESInternalError &e) {
                cerr << "Caught BIE: " << e.get_message() << " at: " << e.get_file() << ":" << e.get_line() << endl;
            }
            catch (BESError &e) {
                cerr << "Caught BE: " << e.get_message() << " at: " << e.get_file() << ":" << e.get_line() << endl;
            }

            exit(0);
        }
        else {
            // parent
            cerr << "Forked: " << pid << endl;
        }
    }

    do {
        // wait
        int status;
        pid_t p = wait(&status);
        cerr << "Child: " << p << ", status: " << status << endl;
        --num_children;
    } while (num_children > 0);
}
