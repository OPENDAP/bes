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

using namespace std;

BESH4Cache *BESH4Cache::d_instance = 0;
const string BESH4Cache::PATH_KEY   = "HDF4.Cache.latlon.path";
const string BESH4Cache::PREFIX_KEY = "HDF4.Cache.latlon.prefix";
const string BESH4Cache::SIZE_KEY   = "HDF4.Cache.latlon.size";

unsigned long BESH4Cache::getCacheSizeFromConfig(){

    bool found;
    string size;
    unsigned long size_in_megabytes = 0;
    TheBESKeys::TheKeys()->get_value( SIZE_KEY, size, found ) ;
    if( found ) {
        BESDEBUG("cache", "In BESH4Cache::getDefaultCacheSize(): Located BES key " <<
                        SIZE_KEY<< "=" << size << endl);
        istringstream iss(size);
        iss >> size_in_megabytes;
    }
    else {
        string msg = "[ERROR] BESH4Cache::getCacheSize() - The BES Key " + SIZE_KEY + " is not set! It MUST be set to utilize the HDF4 cache. ";
        BESDEBUG("cache", msg);
        throw BESInternalError(msg , __FILE__, __LINE__);
    }
    return size_in_megabytes;
}                       
        
string BESH4Cache::getCachePrefixFromConfig(){

    bool found;
    string prefix = "";
    TheBESKeys::TheKeys()->get_value( PREFIX_KEY, prefix, found ) ;
    if( found ) {
        BESDEBUG("cache", "In BESH4Cache::getDefaultCachePrefix(): Located BES key " <<
                        PREFIX_KEY<< "=" << prefix << endl);
        prefix = BESUtil::lowercase( prefix ) ;
    }
    else {
        string msg = "[ERROR] BESH4Cache::getCachePrefix() - The BES Key " + PREFIX_KEY + " is not set! It MUST be set to utilize the HDF4 cache. ";            
        BESDEBUG("cache", msg);
        throw BESInternalError(msg , __FILE__, __LINE__);
    }
    
    return prefix; 
}


string BESH4Cache::getCacheDirFromConfig(){

    bool found;

    string cacheDir = "";
    TheBESKeys::TheKeys()->get_value( PATH_KEY, cacheDir, found ) ;
    if( found ) {
        BESDEBUG("cache", "In BESH4Cache::getCachePrefix(): Located BES key " <<
                        PATH_KEY<< "=" << cacheDir << endl);
        cacheDir = BESUtil::lowercase( cacheDir ) ;
    }
    else {
        string msg =   "[ERROR] BESH4Cache::getCacheDir() - The BES Key " + PATH_KEY + " is not set! It MUST be set to utilize the HDF4 cache. ";
        BESDEBUG("cache", msg);
        throw BESInternalError(msg , __FILE__, __LINE__);
    }
    return cacheDir;
}

BESH4Cache::BESH4Cache(){

    BESDEBUG("cache", "In BESH4Cache::BESH4Cache()" << endl);

    string cacheDir = getCacheDirFromConfig();
    string prefix = getCachePrefixFromConfig();
    unsigned long size_in_megabytes = getCacheSizeFromConfig();

    BESDEBUG("cache", "BESH4Cache() - Cache config params: " << cacheDir << ", " << prefix << ", " << size_in_megabytes << endl);

    //cerr << endl << "***** BESH4Cache::BESH4Cache() - Read cache params: " << path << ", " << prefix << ", " << size << endl;

    // The required params must be present. If initialize() is not called,
    // then d_cache will stay null and is_available() will return false.
    // Also, the directory 'path' must exist, or d_cache will be null.
    if (!cacheDir.empty() && size_in_megabytes > 0) {
        BESDEBUG("cache", "Before calling initialize function." << endl);
        initialize(cacheDir, prefix, size_in_megabytes);
    }

    BESDEBUG("cache", "Leaving BESH4Cache::BESH4Cache()" << endl);
}

#if 0

/** @brief Protected constructor that takes as arguments keys to the cache directory,
 * file prefix, and size of the cache to be looked up a configuration file
 *
 * The keys specified are looked up in the specified keys object. If not
 * found or not set correctly then an exception is thrown. I.E., if the
 * cache directory is empty, the size is zero, or the prefix is empty.
 *
 * @param cache_dir_key key to look up in the keys file to find cache dir
 * @param prefix_key key to look up in the keys file to find the cache prefix
 * @param size_key key to look up in the keys file to find the cache size (in MBytes)
 * @throws BESSyntaxUserError if keys not set, cache dir or prefix empty,
 * size is 0, or if cache dir does not exist.
 */
BESH4Cache::BESH4Cache(const string &cache_dir, const string &prefix, unsigned long long size): BESFileLockingCache(cache_dir,prefix,size) {

}
/** Get an instance of the BESH4Cache object. This class is a singleton, so the
 * first call to any of three 'get_instance()' methods makes an instance and subsequent calls
 * return a pointer to that instance.
 *
 *
 * @param cache_dir_key Key to use to get the value of the cache directory
 * @param prefix_key Key for the item/file prefix. Each file added to the cache uses this
 * as a prefix so cached items can be easily identified when /tmp is used for the cache.
 * @param size_key How big should the cache be, in megabytes
 * @return A pointer to a BESH4Cache object
 */
BESH4Cache *
BESH4Cache::get_instance(const string &cache_dir, const string &prefix, unsigned long long size)
{
    if (d_instance == 0){
        if(dir_exists(cache_dir)){
                try {
                d_instance = new BESH4Cache(cache_dir, prefix, size);
                }
                catch(BESInternalError &bie){
                    BESDEBUG("cache", "BESH4Cache::get_instance(): Failed to obtain cache! msg: " << bie.get_message() << endl);
                }
        }
    }
    return d_instance;
}
#endif

/** Get the default instance of the BESH4Cache object. This will read "TheBESKeys" looking for the values
 * of FUNCTION_CACHE_PATH, FUNCTION_CACHE_PREFIX, an FUNCTION_CACHE_SIZE to initialize the cache.
 */
BESH4Cache *
BESH4Cache::get_instance()
{
    if (d_instance == 0) {
        struct stat buf;
        string cache_dir = getCacheDirFromConfig();
        if((stat(cache_dir.c_str(),&buf)==0) && (buf.st_mode & S_IFDIR)){
            try {
                d_instance = new BESH4Cache();
            }
            catch(BESInternalError &bie){
                BESDEBUG("cache", "BESH4Cache::get_instance(): Failed to obtain cache! msg: " << bie.get_message() << endl);
            }
        }
    }

    return d_instance;
}

void BESH4Cache::delete_instance() {
    BESDEBUG("cache","BESH4Cache::delete_instance() - Deleting singleton BESH4Cache instance." << endl);
    cerr << "BESH4Cache::delete_instance() - Deleting singleton BESH4Cache instance. d_instance="<< d_instance << endl;
    delete d_instance;
    d_instance = 0;
}

// Check whether the real lat/lon file size is the same as the expected lat/lon size. If not, return false. 
bool BESH4Cache::is_valid(const string & cache_file_name, const int expected_file_size){

    struct stat st;
    int result = stat(cache_file_name.c_str(),&st);
    if(result != 0 ) {
        string msg ="Cannot check the cached file " + cache_file_name;
        throw BESInternalError(msg,__FILE__,__LINE__);
    }
    if(expected_file_size == st.st_size)
        return true;
    else
        return false;
}

// This call will try to obtain the read lock.
bool BESH4Cache::get_data_from_cache(const string & cache_file_name, const int expected_file_size,int &fd) {
#if 0
cerr<<"coming to get_data_from_cache "<<endl;
    BESDEBUG("cache", "In BESH4Cache::get_data_from_cache()" << endl);
cerr<<"cache_file_name is "<<cache_file_name <<endl;
int fd1;
string cache_file_name1 = cache_file_name;
    get_read_lock(cache_file_name1,fd1);

cerr<<"After get_read_lock "<<endl;
#endif
    if(false == get_read_lock(cache_file_name,fd))
        return false;
    else if(false == is_valid(cache_file_name,expected_file_size)) {
        unlock_and_close(cache_file_name);
        purge_file(cache_file_name);
        return false;
    }
    else
        return true;
}


bool BESH4Cache::write_cached_data( const string & cache_file_name,const int expected_file_size,const vector<double> &val) {

    BESDEBUG("cache", "In BESH4Cache::write_cached_data()" << endl);
    int fd = 0;
    bool ret_value = false;

    // 1. create_and_lock. 
    if(create_and_lock(cache_file_name,fd)) {

        ssize_t ret_val = 0;

        // 2. write the file.
        ret_val = write(fd,&val[0],expected_file_size);


        // 3. If the written size is not the same as the expected file size, purge the file.
        if(ret_val != expected_file_size) {
            if(unlink(cache_file_name.c_str())!=0){
                string msg = "Cannot remove the corrupt cached file " + cache_file_name;
                throw BESInternalError(msg , __FILE__, __LINE__);
            }
                
        }
        else {
            unsigned long long size = update_cache_info(cache_file_name);
            if(cache_too_big(size))
                update_and_purge(cache_file_name);
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



