/*
 * BESUncompressCache.cc
 *
 *  Created on: Oct 2, 2015
 *      Author: ndp
 */

#include "BESUncompressCache.h"
#include <string>
#include <fstream>
#include <sstream>
#include <sys/stat.h>

#include "BESInternalError.h"
#include "BESDebug.h"
#include "TheBESKeys.h"


static const string BES_DATA_ROOT("BES.Data.RootDirectory");
static const string BES_CATALOG_ROOT("BES.Catalog.catalog.RootDirectory");

BESUncompressCache *BESUncompressCache::d_instance = 0;
const string BESUncompressCache::DIR_KEY       = "BES.UncompressCache.dir";
const string BESUncompressCache::PREFIX_KEY    = "BES.UncompressCache.prefix";
const string BESUncompressCache::SIZE_KEY      = "BES.UncompressCache.size";


unsigned long BESUncompressCache::getCacheSizeFromConfig(){

	bool found;
    string size;
    unsigned long size_in_megabytes = 0;
    TheBESKeys::TheKeys()->get_value( SIZE_KEY, size, found ) ;
    if( found ) {
    	std::istringstream iss(size);
    	iss >> size_in_megabytes;
    }
    else {
    	string msg = "[ERROR] BESUncompressCache::getCacheSize() - The BES Key " + SIZE_KEY + " is not set! It MUST be set to utilize the NcML Dimension Cache. ";
    	BESDEBUG("cache", msg << endl);
        throw BESInternalError(msg , __FILE__, __LINE__);
    }
    return size_in_megabytes;
}

string BESUncompressCache::getCacheDirFromConfig(){
	bool found;
    string subdir = "";
    TheBESKeys::TheKeys()->get_value( DIR_KEY, subdir, found ) ;

	if( !found ) {
    	string msg = "[ERROR] BESUncompressCache::getSubDirFromConfig() - The BES Key " + DIR_KEY + " is not set! It MUST be set to utilize the NcML Dimension Cache. ";
    	BESDEBUG("cache", msg << endl);
        throw BESInternalError(msg , __FILE__, __LINE__);
	}
	else {
		while(*subdir.begin() == '/' && subdir.length()>0){
			subdir = subdir.substr(1);
		}
		// So if it's value is "/" or the empty string then the subdir will default to the root
		// directory of the BES data system.
	}

    return subdir;
}


string BESUncompressCache::getCachePrefixFromConfig(){
	bool found;
    string prefix = "";
    TheBESKeys::TheKeys()->get_value( PREFIX_KEY, prefix, found ) ;
	if( found ) {
		prefix = BESUtil::lowercase( prefix ) ;
	}
	else {
    	string msg = "[ERROR] BESUncompressCache::getResultPrefix() - The BES Key " + PREFIX_KEY + " is not set! It MUST be set to utilize the NcML Dimension Cache. ";
    	BESDEBUG("cache", msg << endl);
        throw BESInternalError(msg , __FILE__, __LINE__);
	}

    return prefix;
}

/**
 * @brief Build the name of file that will holds the uncompressed data from
 * 'src' in the cache.
 *
 * Overrides the generic file name generator in BESFileLocking cache.
 * Because this is the uncompress cache, we know that our job is
 * to simply decompress the file and hand it off to the appropriate
 * response handler for the associated file type. Since the state of
 * file's "compressedness" is determined in ALL cases by suffix on the
 * file name (or resource ID if you wish) we know that in addition to
 * building the generic name we want to remove the compression suffix
 * so that the resulting name/id (unmangled if previously mangled)
 * will correctly match the BES TypeMatch regex system.
 *
 *
 * @note How names are mangled: 'src' is the full name of the file to be
 * cached.The file name passed has an extension on the end that will be
 * stripped once the file is cached. For example, if the full path to the
 * file name is /usr/lib/data/fnoc1.nc.gz then the resulting file name
 * will be \#&lt;prefix&gt;\#usr\#lib\#data\#fnoc1.nc.
 *
 * @param src The source name to cache
 * @param mangle if True, assume the name is a file pathname and mangle it.
 * If false, do not mangle the name (assume the caller has sent a suitable
 * string) but do turn the string into a pathname located in the cache directory
 * with the cache prefix. the 'mangle' param is true by default.
 */
string BESUncompressCache::get_cache_file_name(const string &src, bool mangle)
{
    string target = src;

    if (mangle) {
        string::size_type last_dot = target.rfind('.');
        if (last_dot != string::npos) {
            target = target.substr(0, last_dot);
        }
    }
    target = BESFileLockingCache::get_cache_file_name(target);

    BESDEBUG("cache", "BESFileLockingCache::get_cache_file_name - target:      '" << target  << "'" << endl);

    return target;
}



BESUncompressCache::BESUncompressCache()
{
	BESDEBUG("cache", "BESUncompressCache::BESUncompressCache() -  BEGIN" << endl);

	d_dimCacheDir = getCacheDirFromConfig();
    d_dimCacheFilePrefix = getCachePrefixFromConfig();
    d_maxCacheSize = getCacheSizeFromConfig();

    BESDEBUG("cache", "BESUncompressCache() - Cache configuration params: " << d_dimCacheDir << ", " << d_dimCacheFilePrefix << ", " << d_maxCacheSize << endl);

  	// initialize(d_dimCacheDir, CACHE_CONTROL_FILE, d_dimCacheFilePrefix, d_maxCacheSize);
  	initialize(d_dimCacheDir, d_dimCacheFilePrefix, d_maxCacheSize);

    BESDEBUG("cache", "BESUncompressCache::BESUncompressCache() -  END" << endl);

}
BESUncompressCache::BESUncompressCache(const string &data_root_dir, const string &cache_dir, const string &prefix, unsigned long long size){

	BESDEBUG("cache", "BESUncompressCache::BESUncompressCache() -  BEGIN" << endl);

	d_dataRootDir = data_root_dir;
	d_dimCacheDir = cache_dir;
	d_dimCacheFilePrefix = prefix;
	d_maxCacheSize = size;

//  	initialize(d_dimCacheDir, CACHE_CONTROL_FILE, d_dimCacheFilePrefix, d_maxCacheSize);
  	initialize(d_dimCacheDir, d_dimCacheFilePrefix, d_maxCacheSize);

  	BESDEBUG("cache", "BESUncompressCache::BESUncompressCache() -  END" << endl);
}



BESUncompressCache *
BESUncompressCache::get_instance(const string &data_root_dir, const string &cache_dir, const string &result_file_prefix, unsigned long long max_cache_size)
{
    if (d_instance == 0){
        if (dir_exists(cache_dir)) {
        	try {
                d_instance = new BESUncompressCache(data_root_dir, cache_dir, result_file_prefix, max_cache_size);
#ifdef HAVE_ATEXIT
                atexit(delete_instance);
#endif
        	}
        	catch(BESInternalError &bie){
        	    BESDEBUG("cache", "[ERROR] BESUncompressCache::get_instance(): Failed to obtain cache! msg: " << bie.get_message() << endl);
        	}
    	}
    }
    return d_instance;
}

/** Get the default instance of the GatewayCache object. This will read "TheBESKeys" looking for the values
 * of SUBDIR_KEY, PREFIX_KEY, an SIZE_KEY to initialize the cache.
 */
BESUncompressCache *
BESUncompressCache::get_instance()
{
    if (d_instance == 0) {
		try {
			d_instance = new BESUncompressCache();
#ifdef HAVE_ATEXIT
            atexit(delete_instance);
#endif
		}
		catch(BESInternalError &bie){
			BESDEBUG("cache", "[ERROR] BESUncompressCache::get_instance(): Failed to obtain cache! msg: " << bie.get_message() << endl);
		}
    }

    return d_instance;
}




BESUncompressCache::~BESUncompressCache()
{
	delete_instance();
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
bool BESUncompressCache::is_valid(const string &cache_file_name, const string &local_id)
{
    // If the cached response is zero bytes in size, it's not valid.
    // (hmmm...)
	string datasetFileName = assemblePath(d_dataRootDir,local_id, true);

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

#if 0
string BESUncompressCache::assemblePath(const string &firstPart, const string &secondPart, bool addLeadingSlash){

	//BESDEBUG("cache", "BESUncompressCache::assemblePath() -  BEGIN" << endl);
	//BESDEBUG("cache", "BESUncompressCache::assemblePath() -  firstPart: "<< firstPart << endl);
	//BESDEBUG("cache", "BESUncompressCache::assemblePath() -  secondPart: "<< secondPart << endl);

	string firstPathFragment = firstPart;
	string secondPathFragment = secondPart;


	if(addLeadingSlash){
	    if(*firstPathFragment.begin() != '/')
	    	firstPathFragment = "/" + firstPathFragment;
	}

	// make sure there are not multiple slashes at the end of the first part...
	while(*firstPathFragment.rbegin() == '/' && firstPathFragment.length()>0){
		firstPathFragment = firstPathFragment.substr(0,firstPathFragment.length()-1);
		//BESDEBUG("cache", "BESUncompressCache::assemblePath() -  firstPathFragment: "<< firstPathFragment << endl);
	}

	// make sure first part ends with a "/"
    if(*firstPathFragment.rbegin() != '/'){
    	firstPathFragment += "/";
    }
	//BESDEBUG("cache", "BESUncompressCache::assemblePath() -  firstPathFragment: "<< firstPathFragment << endl);

	// make sure second part does not BEGIN with a slash
	while(*secondPathFragment.begin() == '/' && secondPathFragment.length()>0){
		secondPathFragment = secondPathFragment.substr(1);
	}

	//BESDEBUG("cache", "BESUncompressCache::assemblePath() -  secondPathFragment: "<< secondPathFragment << endl);

	string newPath = firstPathFragment + secondPathFragment;

	//BESDEBUG("cache", "BESUncompressCache::assemblePath() -  newPath: "<< newPath << endl);
	//BESDEBUG("cache", "BESUncompressCache::assemblePath() -  END" << endl);

	return newPath;
}
#endif

