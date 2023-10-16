//
// Created by James Gallagher on 10/16/23.
//

#ifndef BES_MEMORYCACHE_H
#define BES_MEMORYCACHE_H

#include <string>
#include <unordered_map>

namespace ngap {

template <typename VALUE>
class MemoryCache {
    unsigned int d_threshold;       //< Number of items to cache (not bytes, but items)
    unsigned int d_space;           //< When purging, remove this many items
    std::unordered_map<std::string, VALUE> d_cache_map;

    void purge();

public:
    MemoryCache() = default;    // this makes testing easier
    MemoryCache(const MemoryCache *src) = delete;
    MemoryCache(unsigned int t, unsigned int s): d_threshold(t), d_space(s) {}
    ~MemoryCache() = default;
    MemoryCache &operator=(const MemoryCache *src) = delete;

    virtual bool get(const std::string &key, VALUE &value);    // returns a _copy_ of the caching information
    virtual void put(const std::string &key, const VALUE &value);    // caches a _copy_ of the value
};

} // ngap

#endif //BES_MEMORYCACHE_H
