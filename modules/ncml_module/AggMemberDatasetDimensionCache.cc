/*
 * AggMemberDatasetDimensionCache.cc
 *
 *  Created on: Sep 25, 2015
 *      Author: ndp
 */

#include "AggMemberDatasetDimensionCache.h"
#include "AggMemberDataset.h"
#include <string>
#include <fstream>
#include <sstream>
#include <sys/stat.h>

#include "util.h"
#include "BESInternalError.h"
#include "BESUtil.h"
#include "BESDebug.h"
#include "TheBESKeys.h"


static const string BES_DATA_ROOT("BES.Data.RootDirectory");
static const string BES_CATALOG_ROOT("BES.Catalog.catalog.RootDirectory");


namespace agg_util
{

AggMemberDatasetDimensionCache *AggMemberDatasetDimensionCache::d_instance = 0;
bool AggMemberDatasetDimensionCache::d_enabled = true;

const string AggMemberDatasetDimensionCache::CACHE_DIR_KEY = "NCML.DimensionCache.directory";
const string AggMemberDatasetDimensionCache::PREFIX_KEY    = "NCML.DimensionCache.prefix";
const string AggMemberDatasetDimensionCache::SIZE_KEY      = "NCML.DimensionCache.size";
// const string AggMemberDatasetDimensionCache::CACHE_CONTROL_FILE  = "ncmlAggDimensions.cache.info";

/**
 * Checks TheBESKeys for the AggMemberDatasetDimensionCache::SIZE_KEY
 * Returns the value if found, throws an exception otherwise.
 */
unsigned long AggMemberDatasetDimensionCache::getCacheSizeFromConfig(){

	bool found;
    string size;
    unsigned long size_in_megabytes = 0;
    TheBESKeys::TheKeys()->get_value( SIZE_KEY, size, found ) ;
    if( found ) {
    	std::istringstream iss(size);
    	iss >> size_in_megabytes;
    }
    else {
    	string msg = "[ERROR] AggMemberDatasetDimensionCache::getCacheSize() - The BES Key " + SIZE_KEY + " is not set! It MUST be set to utilize the NcML Dimension Cache. ";
    	BESDEBUG("cache", msg << endl);
        throw BESInternalError(msg , __FILE__, __LINE__);
    }
    return size_in_megabytes;
}

/**
 * Checks TheBESKeys for the AggMemberDatasetDimensionCache::CACHE_DIR_KEY
 * Returns the value if found, throws an exception otherwise.
 */
string AggMemberDatasetDimensionCache::getCacheDirFromConfig(){
	bool found;
    string subdir = "";
    TheBESKeys::TheKeys()->get_value( CACHE_DIR_KEY, subdir, found ) ;

	if( !found ) {
    	string msg = "[ERROR] AggMemberDatasetDimensionCache::getSubDirFromConfig() - The BES Key " + CACHE_DIR_KEY + " is not set! It MUST be set to utilize the NcML Dimension Cache. ";
    	BESDEBUG("cache", msg << endl);
        throw BESInternalError(msg , __FILE__, __LINE__);
	}

    return subdir;
}


/**
 * Checks TheBESKeys for the AggMemberDatasetDimensionCache::PREFIX_KEY
 * Returns the value if found, throws an exception otherwise.
 */
string AggMemberDatasetDimensionCache::getDimCachePrefixFromConfig(){
	bool found;
    string prefix = "";
    TheBESKeys::TheKeys()->get_value( PREFIX_KEY, prefix, found ) ;
	if( found ) {
		prefix = BESUtil::lowercase( prefix ) ;
	}
	else {
    	string msg = "[ERROR] AggMemberDatasetDimensionCache::getResultPrefix() - The BES Key " + PREFIX_KEY + " is not set! It MUST be set to utilize the NcML Dimension Cache. ";
    	BESDEBUG("cache", msg << endl);
        throw BESInternalError(msg , __FILE__, __LINE__);
	}

    return prefix;
}


/**
 * Checks TheBESKeys for BES_CATALOG_ROOT, and failing that for
 * BES_DATA_ROOT.
 * Returns the value if found, throws an exception otherwise.
 */
string AggMemberDatasetDimensionCache::getBesDataRootDirFromConfig(){
	bool found;
    string cacheDir = "";
    TheBESKeys::TheKeys()->get_value( BES_CATALOG_ROOT, cacheDir, found ) ;
    if( !found ) {
        TheBESKeys::TheKeys()->get_value( BES_DATA_ROOT, cacheDir, found ) ;
        if( !found ) {
        	string msg = ((string)"[ERROR] AggMemberDatasetDimensionCache::getStoredResultsDir() - Neither the BES Key ") + BES_CATALOG_ROOT +
        			"or the BES key " + BES_DATA_ROOT + " have been set! One MUST be set to utilize the NcML Dimension Cache. ";
        	BESDEBUG("cache", msg << endl);
            throw BESInternalError(msg , __FILE__, __LINE__);
        }
    }
    return cacheDir;

}

/**
 * Builds a instance of the cache object using the values found in TheBESKeys
 */
AggMemberDatasetDimensionCache::AggMemberDatasetDimensionCache()
{
	BESDEBUG("cache", "AggMemberDatasetDimensionCache::AggMemberDatasetDimensionCache() -  BEGIN" << endl);

	d_dimCacheDir = getCacheDirFromConfig();
	d_dataRootDir = getBesDataRootDirFromConfig();

    d_dimCacheFilePrefix = getDimCachePrefixFromConfig();
    d_maxCacheSize = getCacheSizeFromConfig();

    BESDEBUG("cache", "AggMemberDatasetDimensionCache() - Stored results cache configuration params: " << d_dimCacheDir << ", " << d_dimCacheFilePrefix << ", " << d_maxCacheSize << endl);

  	// initialize(d_dimCacheDir, CACHE_CONTROL_FILE, d_dimCacheFilePrefix, d_maxCacheSize);
  	initialize(d_dimCacheDir, d_dimCacheFilePrefix, d_maxCacheSize);

    BESDEBUG("cache", "AggMemberDatasetDimensionCache::AggMemberDatasetDimensionCache() -  END" << endl);

}

/**
 * Builds a instance of the cache object using the values passed in.
 */
AggMemberDatasetDimensionCache::AggMemberDatasetDimensionCache(const string &data_root_dir, const string &cache_dir, const string &prefix, unsigned long long size){

	BESDEBUG("cache", "AggMemberDatasetDimensionCache::AggMemberDatasetDimensionCache() -  BEGIN" << endl);

	d_dataRootDir = data_root_dir;
	d_dimCacheDir = cache_dir;
	d_dimCacheFilePrefix = prefix;
	d_maxCacheSize = size;

//  	initialize(d_dimCacheDir, CACHE_CONTROL_FILE, d_dimCacheFilePrefix, d_maxCacheSize);
  	initialize(d_dimCacheDir, d_dimCacheFilePrefix, d_maxCacheSize);

  	BESDEBUG("cache", "AggMemberDatasetDimensionCache::AggMemberDatasetDimensionCache() -  END" << endl);
}


/**
 * Get an instance of this singleton class, if one has not already been built a new one will be built using the passed parameters.
 * NB: This method is meant for unit tests and is not expected to be utilized during the normal use pattern in the server.
 */
AggMemberDatasetDimensionCache *
AggMemberDatasetDimensionCache::get_instance(const string &data_root_dir, const string &cache_dir, const string &result_file_prefix, unsigned long long max_cache_size)
{
    if (d_enabled && d_instance == 0){
        if (libdap::dir_exists(cache_dir)) {
            d_instance = new AggMemberDatasetDimensionCache(data_root_dir, cache_dir, result_file_prefix, max_cache_size);
            d_enabled = d_instance->cache_enabled();
            if(!d_enabled){
                delete d_instance;
                d_instance = NULL;
                BESDEBUG("cache", "AggMemberDatasetDimensionCache::"<<__func__ << "() - " <<
                    "Cache is DISABLED"<< endl);
           }
            else {
    #ifdef HAVE_ATEXIT
                atexit(delete_instance);
    #endif
                BESDEBUG("cache", "AggMemberDatasetDimensionCache::"<<__func__ << "() - " <<
                    "Cache is ENABLED"<< endl);
           }
    	}
    }
    return d_instance;
}

/**
 * Get the instance of the singleton AggMemberDatasetDimensionCache object. If one has not yet been built a new
 * one will be by interrogating "TheBESKeys" looking for the values
 * of CACHE_DIR_KEY, PREFIX_KEY, and SIZE_KEY to initialize the cache.
 */
AggMemberDatasetDimensionCache *
AggMemberDatasetDimensionCache::get_instance()
{
    if (d_enabled && d_instance == 0) {
        d_instance = new AggMemberDatasetDimensionCache();
        d_enabled = d_instance->cache_enabled();
        if(!d_enabled){
            delete d_instance;
            d_instance = NULL;
            BESDEBUG("cache", "AggMemberDatasetDimensionCache::"<<__func__ << "() - " <<
                "Cache is DISABLED"<< endl);
       }
        else {
#ifdef HAVE_ATEXIT
            atexit(delete_instance);
#endif
            BESDEBUG("cache", "AggMemberDatasetDimensionCache::"<<__func__ << "() - " <<
                "Cache is ENABLED"<< endl);
       }
}

    return d_instance;
}


/**
 * Deletes the instance of this singleton, Called on exit by atexit()
 */
void AggMemberDatasetDimensionCache::delete_instance() {
    BESDEBUG("cache","AggMemberDatasetDimensionCache::delete_instance() - Deleting singleton BESStoredDapResultCache instance." << endl);
    delete d_instance;
    d_instance = 0;
}



AggMemberDatasetDimensionCache::~AggMemberDatasetDimensionCache()
{
	// Nothing to do here....
}

/**
 * Is the item named by cache_entry_name valid? This code tests that the
 * cache entry is non-zero in size (returns false if that is the case, although
 * that might not be correct) and that the dataset associated with this
 * ResponseBulder instance is at least as old as the cached entry.
 *
 * @param cache_file_name File name of the cached entry
 * @param local_id The id, relative to the BES Catalog/Data root of the source dataset.
 * @return True if the thing is valid, false otherwise.
 */
bool AggMemberDatasetDimensionCache::is_valid(const string &cache_file_name, const string &local_id)
{
    // If the cached response is zero bytes in size, it's not valid.
    // (hmmm...)
	string datasetFileName = BESUtil::assemblePath(d_dataRootDir,local_id, true);

    off_t entry_size = 0;
    time_t entry_time = 0;
    struct stat buf;
    if (stat(cache_file_name.c_str(), &buf) == 0) {
        entry_size = buf.st_size;
        entry_time = buf.st_mtime;
    }
    else {
        return false;
    }

    if (entry_size == 0)
        return false;

    time_t dataset_time = entry_time;
    if (stat(datasetFileName.c_str(), &buf) == 0) {
        dataset_time = buf.st_mtime;
    }

    // Trick: if the d_dataset is not a file, stat() returns error and
    // the times stay equal and the code uses the cache entry.

    // TODO Fix this so that the code can get a LMT from the correct handler.
    // TODO Consider adding a getLastModified() method to the libdap::DDS object to support this
    // TODO The DDS may be expensive to instantiate - I think the handler may be a better location for an LMT method, if we can access the handler when/where needed.
    if (dataset_time > entry_time)
        return false;

    return true;
}



/**
 * Loads the dimensions of the passed  AggMemberDataset. If the dimensions are in the cache, and the cache file
 * is valid (length>0 and LMT < the LMT of the source dataset file) then the dimensions will be read from the
 * cache file. Otherwise the source data file will be used to build a DDS from which the dimension can be extracted
 * and subsequently cached.
 */
void AggMemberDatasetDimensionCache::loadDimensionCache(AggMemberDataset *amd){
    BESDEBUG("cache", "AggMemberDatasetDimensionCache::loadDimensionCache() - BEGIN" << endl );

    // Get the cache filename for this thing, mangle name.
    string local_id = amd->getLocation();
    BESDEBUG("cache", "AggMemberDatasetDimensionCache::loadDimensionCache() - local resource id: "<< local_id << endl );
    string cache_file_name = get_cache_file_name(local_id, true);
    BESDEBUG("cache", "AggMemberDatasetDimensionCache::loadDimensionCache() - cache_file_name: "<< cache_file_name << endl );

    int fd;
    try {
        // If the object in the cache is not valid, remove it. The read_lock will
        // then fail and the code will drop down to the create_and_lock() call.
        // is_valid() tests for a non-zero length cache file (cache_file_name) and
    	// for the source data file (local_id) with a newer LMT than the cache file.
        if (!is_valid(cache_file_name, local_id)){
            BESDEBUG("cache", "AggMemberDatasetDimensionCache::loadDimensionCache() - File is not valid. Purging file from cache. filename: " << cache_file_name << endl);
        	purge_file(cache_file_name);
        }

        if (get_read_lock(cache_file_name, fd)) {
            BESDEBUG("cache", "AggMemberDatasetDimensionCache::loadDimensionCache() - Dimension cache file exists. Loading dimension cache from file: " << cache_file_name << endl);

            ifstream istrm(cache_file_name.c_str());
            if (!istrm)
            	throw libdap::InternalErr(__FILE__, __LINE__, "Could not open '" + cache_file_name + "' to read cached dimensions.");

            amd->loadDimensionCache(istrm);

            istrm.close();


        }
        else {
			// If here, the cache_file_name could not be locked for read access, or it was out of date.
        	// So we are going to (re)build the cache file.

        	// We need to build the DDS object and extract the dimensions.
        	// We do not lock before this operation because it may take a _long_ time and
        	// we don't want to monopolize the cache while we do it.
        	amd->fillDimensionCacheByUsingDDS();

        	// Now, we try to make an empty cache file and get an exclusive lock on it.
        	if (create_and_lock(cache_file_name, fd)) {
        		// Woohoo! We got the exclusive lock on the new cache file.
				BESDEBUG("cache", "AggMemberDatasetDimensionCache::loadDimensionCache() - Created and locked cache file: " << cache_file_name << endl);

				// Now we open it (again) using the more friendly ostream API.
				ofstream ostrm(cache_file_name.c_str());
				if (!ostrm)
					throw libdap::InternalErr(__FILE__, __LINE__, "Could not open '" + cache_file_name + "' to write cached response.");

				// Save the dimensions to the cache file.
				amd->saveDimensionCache(ostrm);

		        // And close the cache file;s ostream.
				ostrm.close();

				// Change the exclusive lock on the new file to a shared lock. This keeps
				// other processes from purging the new file and ensures that the reading
				// process can use it.
				exclusive_to_shared_lock(fd);

				// Now update the total cache size info and purge if needed. The new file's
				// name is passed into the purge method because this process cannot detect its
				// own lock on the file.
				unsigned long long size = update_cache_info(cache_file_name);
				if (cache_too_big(size))
					update_and_purge(cache_file_name);
			}
			// get_read_lock() returns immediately if the file does not exist,
			// but blocks waiting to get a shared lock if the file does exist.
			else if (get_read_lock(cache_file_name, fd)) {
				// If we got here then someone else rebuilt the cache file before we could do it.
				// That's OK, and since we already built the DDS we have all of the cache info in memory
				// from directly accessing the source dataset(s), so we need to do nothing more,
				// Except send a debug statement so we can see that this happened.
				BESDEBUG("cache", "AggMemberDatasetDimensionCache::loadDimensionCache() - Couldn't create and lock cache file, But I got a read lock. "
						"Cache file may have been rebuilt by another process. "
						"Cache file: " << cache_file_name << endl);
			}
			else {
				throw libdap::InternalErr(__FILE__, __LINE__, "AggMemberDatasetDimensionCache::loadDimensionCache() - Cache error during function invocation.");
			}
        }

        BESDEBUG("cache", "AggMemberDatasetDimensionCache::loadDimensionCache() - unlocking and closing cache file "<< cache_file_name  << endl );
		unlock_and_close(cache_file_name);
    }
    catch (...) {
        BESDEBUG("cache", "AggMemberDatasetDimensionCache::loadDimensionCache() - caught exception, unlocking cache and re-throw." << endl );
        unlock_cache();
        throw;
    }

    BESDEBUG("cache", "AggMemberDatasetDimensionCache::loadDimensionCache() - END (local_id=`"<< local_id << "')" << endl );

}






} /* namespace agg_util */
