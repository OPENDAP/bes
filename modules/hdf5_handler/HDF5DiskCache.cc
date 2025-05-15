/////////////////////////////////////////////////////////////////////////////
// This file includes cache handling routines for the HDF5 handler.
// The skeleton of the code is adapted from BESDapResponseCache.cc under bes/dap 
//  Authors:   Kent Yang <myang6@hdfgroup.org>  
// Copyright (c) 2014 The HDF Group
/////////////////////////////////////////////////////////////////////////////

#include <sys/stat.h>
#include <iostream>
#include  <sstream>

#include "HDF5DiskCache.h"
#include "BESUtil.h"

#include "BESInternalError.h"
#include "TheBESKeys.h"
#include "BESDebug.h"
#include "HDF5RequestHandler.h"

using namespace std;

HDF5DiskCache *HDF5DiskCache::d_instance = nullptr;
const string HDF5DiskCache::PATH_KEY = "H5.DiskCacheDataPath";
const string HDF5DiskCache::PREFIX_KEY = "H5.DiskCacheFilePrefix";
const string HDF5DiskCache::SIZE_KEY = "H5.DiskCacheSize";
const int HDF5DiskCache::CACHE_BUF_SIZE = 1073741824;

long HDF5DiskCache::getCacheSizeFromConfig(const long cache_size)
{
    if (cache_size >0) {
        BESDEBUG("cache",
            "In HDF5DiskCache::getCacheSizeFromConfig(): Located BES key " << SIZE_KEY<< "=" << cache_size << endl);
        return cache_size;
    } 
    else {
        string msg = "[ERROR] HDF5DiskCache::getCacheSize() - The BES Key " + SIZE_KEY
            + " is either not set or the size is not a positive integer! It MUST be set and the size must be greater than 0 to use the HDF5 Disk cache. ";
        BESDEBUG("cache", msg);
        throw BESInternalError(msg, __FILE__, __LINE__);
    }
}

string HDF5DiskCache::getCachePrefixFromConfig(const string& cache_prefix)
{
    if (cache_prefix!="") {
        BESDEBUG("cache",
            "In HDF5DiskCache::getCachePrefixFromConfig(): Located BES key " << PATH_KEY<< "=" << cache_prefix << endl);
        return cache_prefix;
    }
    else {
        string msg = "[ERROR] HDF5DiskCache::getCachePrefixFromConfig() - The BES Key " + PREFIX_KEY
            + " is either not set or the value is an empty string! It MUST be set to be a valid string  to utilize the HDF5 Disk cache. ";
        BESDEBUG("cache", msg);
        throw BESInternalError(msg, __FILE__, __LINE__);
    }
}

string HDF5DiskCache::getCacheDirFromConfig(const string& cache_dir)
{
    if (cache_dir!="") {
        BESDEBUG("cache",
            "In HDF5DiskCache::getCacheDirFromConfig(): Located BES key " << PATH_KEY<< "=" << cache_dir << endl);
        return cache_dir;
    }
    else {
        string msg = "[ERROR] HDF5DiskCache::getCacheDirFromConfig() - The BES Key " + PREFIX_KEY
            + " is either not set or the value is an empty string! It MUST be set to be a valid path to utilize the HDF5 Disk cache. ";
        BESDEBUG("cache", msg);
        throw BESInternalError(msg, __FILE__, __LINE__);
    }
}


HDF5DiskCache::HDF5DiskCache(const unsigned long long _cache_size, const string &_cache_dir, const string &_cache_prefix)
{
    BESDEBUG("cache", "In HDF5DiskCache::HDF5DiskCache()" << endl);

    string cacheDir = getCacheDirFromConfig(_cache_dir);
    string prefix = getCachePrefixFromConfig(_cache_prefix);
    unsigned long long size_in_megabytes = getCacheSizeFromConfig(_cache_size);

    BESDEBUG("cache",
        "HDF5DiskCache() - Cache config params: " << cacheDir << ", " << prefix << ", " << size_in_megabytes << endl);

    // The required params must be present. If initialize() is not called,
    // then d_cache will stay null and is_available() will return false.
    // Also, the directory 'path' must exist, or d_cache will be null.
    if (!cacheDir.empty() && size_in_megabytes > 0) {
        BESDEBUG("cache", "Before calling initialize function." << endl);
        initialize(cacheDir, prefix, size_in_megabytes);
    }

    BESDEBUG("cache", "Leaving HDF5DiskCache::HDF5DiskCache()" << endl);
}

/** Get the default instance of the HDF5DiskCache object. This will read "TheBESKeys" looking for the values
 * of FUNCTION_CACHE_PATH, FUNCTION_CACHE_PREFIX, an FUNCTION_CACHE_SIZE to initialize the cache.
 */
HDF5DiskCache *
HDF5DiskCache::get_instance(const long _cache_size, const string &_cache_dir, const string &_cache_prefix)
{
    if (d_instance == nullptr) {
        struct stat buf;
        string cache_dir = getCacheDirFromConfig(_cache_dir);
        if ((stat(cache_dir.c_str(), &buf) == 0) && (buf.st_mode & S_IFDIR)) {
            try {
                d_instance = new HDF5DiskCache(_cache_size,_cache_dir,_cache_prefix);
#ifdef HAVE_ATEXIT
                atexit(delete_instance);
#endif
            }
            catch (BESInternalError &bie) {
                BESDEBUG("cache",
                    "HDF5DiskCache::get_instance(): Failed to obtain cache! msg: " << bie.get_message() << endl);
            }
        }
    }

    return d_instance;
}

// Check whether the real lat/lon file size is the same as the expected lat/lon size. If not, return false. 
bool HDF5DiskCache::is_valid(const string & cache_file_name, int64_t expected_file_size) const
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
bool HDF5DiskCache::get_data_from_cache(const string & cache_file_name, int64_t expected_file_size, int &fd)
{
#if 0
    cerr<<"coming to get_data_from_cache "<<endl;
    BESDEBUG("cache", "In HDF5DiskCache::get_data_from_cache()" << endl);
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

bool HDF5DiskCache::write_cached_data(const string & cache_file_name, int64_t expected_file_size,
    const vector<double> &val)
{

    BESDEBUG("cache", "In HDF5DiskCache::write_cached_data()" << endl);
    int fd = 0;
    bool ret_value = false;

    // 1. create_and_lock. 
    if (create_and_lock(cache_file_name, fd)) {

        ssize_t ret_val = 0;

        // 2. write the file.
        ret_val = write(fd, val.data(), expected_file_size);

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

bool HDF5DiskCache::write_cached_data2(const string & cache_file_name, int64_t expected_file_size, const void *buf)
{

    BESDEBUG("cache", "In HDF5DiskCache::write_cached_data()" << endl);
    int fd = 0;
    bool ret_value = false;
cerr<<"cache_file_name: "<<cache_file_name <<endl;

    // 1. create_and_lock. 
    if (create_and_lock(cache_file_name, fd)) {

        ssize_t ret_val = 0;

        // 2. write the file.
        
        bool write_disk_cache = true;
        
        if (expected_file_size > HDF5DiskCache::CACHE_BUF_SIZE) {
            ssize_t bytes_written = 0;
            while (bytes_written < expected_file_size) {
cerr<<"write big cache "<<endl;
                int one_write_buf_size = HDF5DiskCache::CACHE_BUF_SIZE;
                if (expected_file_size <(bytes_written+HDF5DiskCache::CACHE_BUF_SIZE)) 
                    one_write_buf_size = expected_file_size - bytes_written;

                ret_val = write(fd,buf,one_write_buf_size);
                if (ret_val <0) {

cerr<<"write big cache failed. "<<endl;
                    write_disk_cache = false;
                    break;
                }
                bytes_written +=ret_val;
                buf = (const char *)buf + ret_val;

            }

            
        }
        else  {
            ret_val = write(fd, buf, expected_file_size);
            if (ret_val != expected_file_size)
                write_disk_cache = false;
               
        }

        // 3. If the written size is not the same as the expected file size, purge the file.
        if (write_disk_cache == false) {
            if (unlink(cache_file_name.c_str()) != 0) {
                string msg = "Cannot remove the corrupt cached file " + cache_file_name;
                throw BESInternalError(msg, __FILE__, __LINE__);
            }

        }
        else {
            unsigned long long size = update_cache_info(cache_file_name);
cerr<<"write the same size "<<endl;
            if (cache_too_big(size)) {
cerr<<"cache_too_big: "<<endl;
                update_and_purge(cache_file_name);
            }
            ret_value = true;
        }
        // 4. release the lock.
        unlock_and_close(cache_file_name);

    }
else 
cerr<<"cannot get a lock"<<endl;

    return ret_value;

}
#if 0
void HDF5DiskCache::dummy_test_func() {

    cerr<<"HDF5DiskCache function is fine "<<endl;

}

string HDF5DiskCache::get_cache_file_name_h4(const string & src, bool mangle) {

    return src;
}
#endif

