/*
 * GatewayCache.h
 *
 *  Created on: Oct 2, 2015
 *      Author: ndp
 */

#ifndef MODULES_GATEWAY_MODULE_GATEWAYCACHE_H_
#define MODULES_GATEWAY_MODULE_GATEWAYCACHE_H_

#include "BESFileLockingCache.h"

namespace gateway
{


class GatewayCache: public BESFileLockingCache
{
private:
    static bool d_enabled;
    static GatewayCache * d_instance;
    static void delete_instance() { delete d_instance; d_instance = 0; }

    GatewayCache();
    GatewayCache(const GatewayCache &src);

    static string getCacheDirFromConfig();
    static string getCachePrefixFromConfig();
    static unsigned long getCacheSizeFromConfig();


protected:

    GatewayCache(const string &cache_dir, const string &prefix, unsigned long long size);

public:
	static const string DIR_KEY;
	static const string PREFIX_KEY;
	static const string SIZE_KEY;

    static GatewayCache *get_instance(const string &cache_dir, const string &prefix, unsigned long long size);
    static GatewayCache *get_instance();


	virtual ~GatewayCache();
};


} /* namespace gateway */

#endif /* MODULES_GATEWAY_MODULE_GATEWAYCACHE_H_ */
