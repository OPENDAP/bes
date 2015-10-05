/*
 * BESUncompressCache.h
 *
 *  Created on: Oct 2, 2015
 *      Author: ndp
 */

#ifndef DISPATCH_BESUNCOMPRESSCACHE_H_
#define DISPATCH_BESUNCOMPRESSCACHE_H_

#include "BESFileLockingCache.h"


class BESUncompressCache: public BESFileLockingCache
{
private:
    static BESUncompressCache * d_instance;
    static void delete_instance() { delete d_instance; d_instance = 0; }

    string d_dimCacheDir;
    string d_dataRootDir;
    string d_dimCacheFilePrefix;
    unsigned long d_maxCacheSize;

    BESUncompressCache();
    BESUncompressCache(const BESUncompressCache &src);

	bool is_valid(const std::string &cache_file_name, const std::string &dataset_file_name);


    static string getCacheDirFromConfig();
    static string getDimCachePrefixFromConfig();
    static unsigned long getCacheSizeFromConfig();


protected:

    BESUncompressCache(const string &data_root_dir, const string &cache_dir, const string &prefix, unsigned long long size);

public:
	static const string DIR_KEY;
	static const string PREFIX_KEY;
	static const string SIZE_KEY;


    static BESUncompressCache *get_instance(const string &bes_catalog_root_dir, const string &cache_dir, const string &prefix, unsigned long long size);
    static BESUncompressCache *get_instance();


    static string assemblePath(const string &firstPart, const string &secondPart, bool addLeadingSlash =  false);

    // Overrides parent method
    virtual string get_cache_file_name(const string &src, bool mangle = true);

	virtual ~BESUncompressCache();
};



#endif /* DISPATCH_BESUNCOMPRESSCACHE_H_ */
