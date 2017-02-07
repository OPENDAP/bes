/*
 * AggMemberDatasetDimensionCache.h
 *
 *  Created on: Sep 25, 2015
 *      Author: ndp
 */

#ifndef MODULES_NCML_MODULE_AGGMEMBERDATASETDIMENSIONCACHE_H_
#define MODULES_NCML_MODULE_AGGMEMBERDATASETDIMENSIONCACHE_H_

#include "BESFileLockingCache.h"

namespace agg_util
{
// Forward declaration
class AggMemberDataset;

/**
 * This child of BESFileLockingCache manifests a cache for the ncml_handler in which
 * AggMemberDataset's dimension values are stored. This allows the ncml_handler
 * to make one (slow) pass over the aggregation member dataset list, collect all of the
 * dimension information anmd cache it. Then, the next time the aggregation is accessed
 * loading the dimension information from the cache goes lickety-split compared with
 * the time needed to retrieve the DDSs from the source datasets. (This is especially
 * true for the hdf4 handler)
 *
 * This implementation utilizes the BES.Catalog.catalog.RootDirectory/BES.Data.RootDirectory
 * to locate the source dataset files in order to verify of the cache is up-to-date
 * and updates cache components as needed.
 *
 */
class AggMemberDatasetDimensionCache: public BESFileLockingCache
{
private:
    static AggMemberDatasetDimensionCache * d_instance;
    static void delete_instance();

    string d_dimCacheDir;
    string d_dataRootDir;
    string d_dimCacheFilePrefix;
    unsigned long d_maxCacheSize;

	AggMemberDatasetDimensionCache();
	AggMemberDatasetDimensionCache(const AggMemberDatasetDimensionCache &src);

	bool is_valid(const std::string &cache_file_name, const std::string &dataset_file_name);


    static string getBesDataRootDirFromConfig();
    static string getCacheDirFromConfig();
    static string getDimCachePrefixFromConfig();
    static unsigned long getCacheSizeFromConfig();


protected:

	AggMemberDatasetDimensionCache(const string &data_root_dir, const string &stored_results_subdir, const string &prefix, unsigned long long size);

public:
	static const string CACHE_DIR_KEY;
	static const string PREFIX_KEY;
	static const string SIZE_KEY;
	 // static const string CACHE_CONTROL_FILE;

    static AggMemberDatasetDimensionCache *get_instance(const string &bes_catalog_root_dir, const string &stored_results_subdir, const string &prefix, unsigned long long size);
    static AggMemberDatasetDimensionCache *get_instance();

    void loadDimensionCache(AggMemberDataset *amd);

	virtual ~AggMemberDatasetDimensionCache();
};

} /* namespace agg_util */

#endif /* MODULES_NCML_MODULE_AGGMEMBERDATASETDIMENSIONCACHE_H_ */
