/////////////////////////////////////////////////////////////////////////////
// This file includes cache handling routines for the HDF4 handler.
// The skeleton of the code is adapted from BESDapResponseCache.cc under bes/dap 
//  Authors:   MuQun Yang <myang6@hdfgroup.org>  
// Copyright (c) 2014 The HDF Group
/////////////////////////////////////////////////////////////////////////////

#include <sys/stat.h>
#include <iostream>
#include  <sstream>

#include "BESH4MCache.h"
#include "BESUtil.h"

#include "BESInternalError.h"
#include "TheBESKeys.h"
#include "BESDebug.h"
#include "HDF4RequestHandler.h"

using namespace std;

BESH4Cache *BESH4Cache::d_instance = 0;
bool BESH4Cache::d_enabled = true;

const string BESH4Cache::PATH_KEY = "HDF4.Cache.latlon.path";
const string BESH4Cache::PREFIX_KEY = "HDF4.Cache.latlon.prefix";
const string BESH4Cache::SIZE_KEY = "HDF4.Cache.latlon.size";

long BESH4Cache::getCacheSizeFromConfig()
{
    if (HDF4RequestHandler::get_cache_latlon_size_exist() == true) {
        BESDEBUG("cache",
            "In BESH4Cache::getCacheSize(): Located BES key " << SIZE_KEY<< "=" << HDF4RequestHandler::get_cache_latlon_size() << endl);
        return HDF4RequestHandler::get_cache_latlon_size();
    }
    else {
        string msg = "[ERROR] BESH4Cache::getCacheSize() - The BES Key " + SIZE_KEY
            + " is not set! It MUST be set to utilize the HDF4 cache. ";
        BESDEBUG("cache", msg);
        throw BESInternalError(msg, __FILE__, __LINE__);
    }
}

string BESH4Cache::getCachePrefixFromConfig()
{
    if (HDF4RequestHandler::get_cache_latlon_prefix_exist() == true) {
        BESDEBUG("cache",
            "In BESH4Cache::getCachePrefix(): Located BES key " << PREFIX_KEY<< "=" << HDF4RequestHandler::get_cache_latlon_prefix() << endl);
        return HDF4RequestHandler::get_cache_latlon_prefix();
    }
    else {
        string msg = "[ERROR] BESH4Cache::getCachePrefix() - The BES Key " + PREFIX_KEY
            + " is not set! It MUST be set to utilize the HDF4 cache. ";
        BESDEBUG("cache", msg);
        throw BESInternalError(msg, __FILE__, __LINE__);
    }
}

string BESH4Cache::getCacheDirFromConfig()
{
    if (HDF4RequestHandler::get_cache_latlon_path_exist() == true) {
        BESDEBUG("cache",
            "In BESH4Cache::getCacheDirFromConfig(): Located BES key " << PATH_KEY<< "=" << HDF4RequestHandler::get_cache_latlon_path() << endl);
        return HDF4RequestHandler::get_cache_latlon_path();
    }
    else {
        string msg = "[ERROR] BESH4Cache::getCachePrefix() - The BES Key " + PREFIX_KEY
            + " is not set! It MUST be set to utilize the HDF4 cache. ";
        BESDEBUG("cache", msg);
        throw BESInternalError(msg, __FILE__, __LINE__);
    }
}

BESH4Cache::BESH4Cache()
{
    BESDEBUG("cache", "In BESH4Cache::BESH4Cache()" << endl);

    string cacheDir = getCacheDirFromConfig();
    string prefix = getCachePrefixFromConfig();
    long size_in_megabytes = getCacheSizeFromConfig();

    BESDEBUG("cache",
        "BESH4Cache() - Cache config params: " << cacheDir << ", " << prefix << ", " << size_in_megabytes << endl);

    // The required params must be present. If initialize() is not called,
    // then d_cache will stay null and is_available() will return false.
    // Also, the directory 'path' must exist, or d_cache will be null.
    if (!cacheDir.empty() && size_in_megabytes > 0) {
        BESDEBUG("cache", "Before calling initialize function." << endl);
        initialize(cacheDir, prefix, size_in_megabytes);
    }

    BESDEBUG("cache", "Leaving BESH4Cache::BESH4Cache()" << endl);
}

/** Get the default instance of the BESH4Cache object. This will read "TheBESKeys" looking for the values
 * of FUNCTION_CACHE_PATH, FUNCTION_CACHE_PREFIX, an FUNCTION_CACHE_SIZE to initialize the cache.
 */
BESH4Cache *
BESH4Cache::get_instance()
{
    if (d_enabled && d_instance == 0) {
        struct stat buf;
        string cache_dir = getCacheDirFromConfig();
        if ((stat(cache_dir.c_str(), &buf) == 0) && (buf.st_mode & S_IFDIR)) {
            try {
                d_instance = new BESH4Cache();

                d_enabled = d_instance->cache_enabled();
                if(!d_enabled){
                    delete d_instance;
                    d_instance = NULL;
                    BESDEBUG("cache", "BESH4Cache::"<<__func__ << "() - " <<
                        "Cache is DISABLED"<< endl);
                }
                else {
    #ifdef HAVE_ATEXIT
                    atexit(delete_instance);
    #endif
                    BESDEBUG("cache", "BESH4Cache::" << __func__ << "() - " <<
                        "Cache is ENABLED"<< endl);
                }
            }
            catch (BESInternalError &bie) {
                BESDEBUG("cache",
                    "BESH4Cache::get_instance(): Failed to obtain cache! msg: " << bie.get_message() << endl);
            }
        }
    }

    return d_instance;
}

// Check whether the real lat/lon file size is the same as the expected lat/lon size. If not, return false. 
bool BESH4Cache::is_valid(const string & cache_file_name, const int expected_file_size)
{

    struct stat st;
    int result = stat(cache_file_name.c_str(), &st);
    if (result != 0) {
        string msg = "Cannot check the cached file " + cache_file_name;
        throw BESInternalError(msg, __FILE__, __LINE__);
    }
    if (expected_file_size == st.st_size)
        return true;
    else
        return false;
}

// This call will try to obtain the read lock.
bool BESH4Cache::get_data_from_cache(const string & cache_file_name, const int expected_file_size, int &fd)
{
#if 0
    cerr<<"coming to get_data_from_cache "<<endl;
    BESDEBUG("cache", "In BESH4Cache::get_data_from_cache()" << endl);
    cerr<<"cache_file_name is "<<cache_file_name <<endl;
    int fd1;
    string cache_file_name1 = cache_file_name;
    get_read_lock(cache_file_name1,fd1);

    cerr<<"After get_read_lock "<<endl;
#endif
    if (false == get_read_lock(cache_file_name, fd))
        return false;
    else if (false == is_valid(cache_file_name, expected_file_size)) {
        unlock_and_close(cache_file_name);
        purge_file(cache_file_name);
        return false;
    }
    else
        return true;
}

bool BESH4Cache::write_cached_data(const string & cache_file_name, const int expected_file_size,
    const vector<double> &val)
{

    BESDEBUG("cache", "In BESH4Cache::write_cached_data()" << endl);
    int fd = 0;
    bool ret_value = false;

    // 1. create_and_lock. 
    if (create_and_lock(cache_file_name, fd)) {

        ssize_t ret_val = 0;

        // 2. write the file.
        ret_val = write(fd, &val[0], expected_file_size);

        // 3. If the written size is not the same as the expected file size, purge the file.
        if (ret_val != expected_file_size) {
            if (unlink(cache_file_name.c_str()) != 0) {
                string msg = "Cannot remove the corrupt cached file " + cache_file_name;
                throw BESInternalError(msg, __FILE__, __LINE__);
            }

        }
        else {
            unsigned long long size = update_cache_info(cache_file_name);
            if (cache_too_big(size)) update_and_purge(cache_file_name);
            ret_value = true;
        }
        // 4. release the lock.
        unlock_and_close(cache_file_name);

    }

    return ret_value;

}

bool BESH4Cache::write_cached_data2(const string & cache_file_name, const int expected_file_size, const void *buf)
{

    BESDEBUG("cache", "In BESH4Cache::write_cached_data()" << endl);
    int fd = 0;
    bool ret_value = false;

    // 1. create_and_lock. 
    if (create_and_lock(cache_file_name, fd)) {

        ssize_t ret_val = 0;

        // 2. write the file.
        ret_val = write(fd, buf, expected_file_size);

        // 3. If the written size is not the same as the expected file size, purge the file.
        if (ret_val != expected_file_size) {
            if (unlink(cache_file_name.c_str()) != 0) {
                string msg = "Cannot remove the corrupt cached file " + cache_file_name;
                throw BESInternalError(msg, __FILE__, __LINE__);
            }

        }
        else {
            unsigned long long size = update_cache_info(cache_file_name);
            if (cache_too_big(size)) update_and_purge(cache_file_name);
            ret_value = true;
        }
        // 4. release the lock.
        unlock_and_close(cache_file_name);

    }

    return ret_value;

}
#if 0
void BESH4Cache::dummy_test_func() {

    cerr<<"BESH4Cache function is fine "<<endl;

}

string BESH4Cache::get_cache_file_name_h4(const string & src, bool mangle) {

    return src;
}
#endif

