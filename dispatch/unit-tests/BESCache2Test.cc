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

#include <sys/file.h>
#include <unistd.h>  // for unlink
#include <dirent.h>
#include <sys/stat.h>

#include <vector>
#include <string>

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

    cerr << "Seconds to sleep: "<< seconds << ", nanoseconds to sleep: " << nanoseconds << endl;

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

void print_name(const string &name) {
    cerr << "Name: " << name << endl;;
}
bool dot_file(const string &file) {
    return file.at(0) == '.';
}

void decompression_process(int files_to_get)
{
    BESDebug::SetUp("cerr,uncompress,cache");

    // Make a cache object for this process. Hardwire the cache directory name
    BESCache2 *cache = BESCache2::get_instance("./cache2", "tc_", 200);

    // Get a list of all of the test files
    vector<string> files = get_file_names("./cache2_data_files");
    files.erase(remove_if(files.begin(), files.end(), dot_file), files.end());
    if (files.size() == 0)
        throw BESInternalError("No files in the data directory for the cache tests.", __FILE__, __LINE__);

    cerr << "Files for the test: " << endl;
    for_each(files.begin(), files.end(), print_name);

    int num_files = files.size();

    for (int i = 0; i < files_to_get; ++i) {
        // randomly choose a compressed file
        string file = files.at(random(num_files));

        // write its name to the log, along with the time
        time_t t;
        time(&t);
        cerr << "Getting file '" << file << "' (time: " << t << ")." << endl;

        // get the file
        file = "./cache2_data_files/" + file;
        string cfile;
        bool in_cache = BESUncompressManager2::TheManager()->uncompress( file, cfile, cache ) ;

        time(&t);
        // write cfile to the log along with the time
        if (in_cache)
            cerr << "    " << "in the cache as " << cfile << " (time: " << t  << ")" << endl;

        cache->unlock(cfile);
    }
}

int main(int argc, char *argv[])
{
    try {
        decompression_process(222);
    }
    catch (BESInternalError &e) {
        cerr << "Caught BIE: " << e.get_message() << " at: " << e.get_file() << ":" << e.get_line() << endl;
    }
    catch (BESError &e) {
        cerr << "Caught BE: " << e.get_message() << " at: " << e.get_file() << ":" << e.get_line() << endl;
    }
}
