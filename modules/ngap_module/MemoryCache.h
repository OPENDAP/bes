//
// Created by James Gallagher on 10/16/23.
//

#ifndef BES_MEMORYCACHE_H
#define BES_MEMORYCACHE_H

#include <string>
#include <unordered_map>
#include <deque>

#ifndef BESInternalError_h_
#include "BESInternalError.h"
#endif

namespace ngap {

template <typename VALUE>
class MemoryCache {
    unsigned int d_max_items;       //< Max number of items to cache (not bytes, but items)
    unsigned int d_purge_items;     //< When purging, remove this many items
    
    std::deque<std::string> d_fifo_keys; //< FIFO queue of keys; used by purge()
    std::unordered_map<std::string, VALUE> d_cache;

    void purge() {
        // if number of elements > threshold, purge
        for (int entries = 0; entries < d_purge_items; ++entries) {
            std::string key = d_fifo_keys.front();

            d_cache.erase(d_cache.find(key));
            d_fifo_keys.pop_front();
        }
    }

    bool invariant(bool expensive = false) const {
        if (d_cache.size() > d_max_items)
            return false;
        if (d_fifo_keys.size() > d_max_items)
            return false;
        if (d_cache.size() != d_fifo_keys.size())
            return false;

        if (expensive) {
            // check that the keys in the cache are in the queue
            for (auto &key : d_fifo_keys) {
                if (d_cache.find(key) == d_cache.end())
                    return false;
            }
        }

        return true;
    }

    friend class MemoryCacheTest;

public:
    MemoryCache() = default;    // this makes testing easier
    MemoryCache(const MemoryCache *src) = delete;
    MemoryCache(unsigned int t, unsigned int s): d_max_items(t), d_purge_items(s) {}
    ~MemoryCache() = default;
    MemoryCache &operator=(const MemoryCache *src) = delete;

    // Return True if the item is in the cache, false otherwise.
    // If the time is cache, value holds a reference to the item
    virtual bool get(const std::string &key, VALUE &value) {
        if (d_cache.find(key) != d_cache.end()) {
            value = d_cache[key];
            return true;
        }
        else {
            return false;
        }
    }

    // Caches a _copy_ of the value. If the key is already in the cache, the value is updated.
    virtual void put(const std::string &key, const VALUE &value) {
        // add key, value to the queue of entries if key is not in the cache
        // add or overwrite/update the value associated with key
        if (d_cache.find(key) == d_cache.end()) {
            d_cache.insert(std::pair<std::string, VALUE>(key, value));
            d_fifo_keys.push_back(key);
            if (d_cache.size() > d_max_items)
                purge();
        }
        else
            d_cache[key] = value;
    }

    virtual unsigned int size() const { return d_cache.size(); }
    virtual void clear() { d_cache.clear(); }
};

} // ngap

#endif //BES_MEMORYCACHE_H
