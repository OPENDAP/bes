//
// Created by James Gallagher on 10/16/23.
//

#include "config.h"

#include "MemoryCache.h"
#include "NgapNames.h"

using namespace std;
using namespace ngap;

#define prolog std::string("MemoryCache<V>::").append(__func__).append("() - ")

/**
 * @brief Purge the cache of the oldest entries.
 * @note this will always purge the cache so that that d_space entries are removed.
 * Caller should check that the cache is larger than d_space before calling this method.
 */
template<typename VALUE>
void MemoryCache<VALUE>::purge() {
    // if number of elements > threshold, purge
    for (int entries = 0; entries < d_space; ++entries) {
        string key = d_cache_map.front();
        d_cache_map.pop();
        if (d_cache_map.erase(key) == 0)
            throw BESInternalError(prolog + "Failed to purge entry (" + key + ") from the MemoryCache", __FILE__, __LINE__);
    }
}

// returns a _copy_ of the caching information
template<typename VALUE>
bool MemoryCache<VALUE>::get(const std::string &key, VALUE &value) {
    if (d_cache_map.find(key) != d_cache_map.end()) {
        value = d_cache_map[key];
        BESDEBUG(NGAP_CACHE, prolog << "Cache hit: " << key << "\n");
        return true;
    }
    else {
        BESDEBUG(NGAP_CACHE, prolog << "Cache miss: " << key << "\n");
        return false;
    }
}

// caches a _copy_ of the value
template<typename VALUE>
void MemoryCache<VALUE>::put(const std::string &key, const VALUE &value) {
    BESDEBUG(NGAP_CACHE, prolog << "Cache addition: " << key << "\n");

    if (d_cache_map.size() >= d_threshold)
        purge();

    // only add key to the queue of entries if key is not in the cache
    if (d_cache_map.find(key) == d_cache_map.end())
        d_cache_map.push(key);

    // add or overwrite/update the value associated with key
    d_cache_map[key] = value;
}