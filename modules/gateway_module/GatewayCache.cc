/*
 * GatewayCache.cc
 *
 *  Created on: Oct 2, 2015
 *      Author: ndp
 */

#include "GatewayCache.h"
#include <string>
#include <fstream>
#include <sstream>
#include <sys/stat.h>

#include "BESInternalError.h"
#include "BESDebug.h"
#include <BESUtil.h>
#include "TheBESKeys.h"


namespace gateway
{

GatewayCache *GatewayCache::d_instance = 0;
const string GatewayCache::DIR_KEY       = "Gateway.Cache.dir";
const string GatewayCache::PREFIX_KEY    = "Gateway.Cache.prefix";
const string GatewayCache::SIZE_KEY      = "Gateway.Cache.size";


unsigned long GatewayCache::getCacheSizeFromConfig(){

	bool found;
    string size;
    unsigned long size_in_megabytes = 0;
    TheBESKeys::TheKeys()->get_value( SIZE_KEY, size, found ) ;
    if( found ) {
    	std::istringstream iss(size);
    	iss >> size_in_megabytes;
    }
    else {
    	string msg = "[ERROR] GatewayCache::getCacheSize() - The BES Key " + SIZE_KEY + " is not set! It MUST be set to utilize the NcML Dimension Cache. ";
    	BESDEBUG("cache", msg << endl);
        throw BESInternalError(msg , __FILE__, __LINE__);
    }
    return size_in_megabytes;
}

string GatewayCache::getCacheDirFromConfig(){
	bool found;
    string subdir = "";
    TheBESKeys::TheKeys()->get_value( DIR_KEY, subdir, found ) ;

	if( !found ) {
    	string msg = "[ERROR] GatewayCache::getSubDirFromConfig() - The BES Key " + DIR_KEY + " is not set! It MUST be set to utilize the NcML Dimension Cache. ";
    	BESDEBUG("cache", msg << endl);
        throw BESInternalError(msg , __FILE__, __LINE__);
	}

    return subdir;
}


string GatewayCache::getCachePrefixFromConfig(){
	bool found;
    string prefix = "";
    TheBESKeys::TheKeys()->get_value( PREFIX_KEY, prefix, found ) ;
	if( found ) {
		prefix = BESUtil::lowercase( prefix ) ;
	}
	else {
    	string msg = "[ERROR] GatewayCache::getResultPrefix() - The BES Key " + PREFIX_KEY + " is not set! It MUST be set to utilize the NcML Dimension Cache. ";
    	BESDEBUG("cache", msg << endl);
        throw BESInternalError(msg , __FILE__, __LINE__);
	}

    return prefix;
}



GatewayCache::GatewayCache()
{
	BESDEBUG("cache", "GatewayCache::GatewayCache() -  BEGIN" << endl);

	string cacheDir               = getCacheDirFromConfig();
    string cachePrefix            = getCachePrefixFromConfig();
    unsigned long cacheSizeMbytes = getCacheSizeFromConfig();

    BESDEBUG("cache", "GatewayCache() - Cache configuration params: " << cacheDir << ", " << cachePrefix << ", " << cacheSizeMbytes << endl);

  	initialize(cacheDir, cachePrefix, cacheSizeMbytes);

    BESDEBUG("cache", "GatewayCache::GatewayCache() -  END" << endl);

}
GatewayCache::GatewayCache(const string &cache_dir, const string &prefix, unsigned long long size){

	BESDEBUG("cache", "GatewayCache::GatewayCache() -  BEGIN" << endl);

  	initialize(cache_dir, prefix, size);

  	BESDEBUG("cache", "GatewayCache::GatewayCache() -  END" << endl);
}



GatewayCache *
GatewayCache::get_instance(const string &cache_dir, const string &result_file_prefix, unsigned long long max_cache_size)
{
    if (d_instance == 0){
        if (dir_exists(cache_dir)) {
        	try {
                d_instance = new GatewayCache(cache_dir, result_file_prefix, max_cache_size);
#ifdef HAVE_ATEXIT
                atexit(delete_instance);
#endif
        	}
        	catch(BESInternalError &bie){
        	    BESDEBUG("cache", "[ERROR] GatewayCache::get_instance(): Failed to obtain cache! msg: " << bie.get_message() << endl);
        	}
    	}
    }
    return d_instance;
}

/** Get the default instance of the GatewayCache object. This will read "TheBESKeys" looking for the values
 * of SUBDIR_KEY, PREFIX_KEY, an SIZE_KEY to initialize the cache.
 */
GatewayCache *
GatewayCache::get_instance()
{
    if (d_instance == 0) {
		try {
			d_instance = new GatewayCache();
#ifdef HAVE_ATEXIT
            atexit(delete_instance);
#endif
		}
		catch(BESInternalError &bie){
			BESDEBUG("cache", "[ERROR] GatewayCache::get_instance(): Failed to obtain cache! msg: " << bie.get_message() << endl);
		}
    }

    return d_instance;
}



GatewayCache::~GatewayCache()
{
	delete_instance();
}





} /* namespace gateway */
