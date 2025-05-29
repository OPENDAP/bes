// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of ngap_module, A C++ module that can be loaded in to
// the OPeNDAP Back-End Server (BES) and is able to handle remote requests.

// Copyright (c) 2023 OPeNDAP, Inc.
// Author: James Gallagher <jgallagher@opendap.org>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.

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

/**
 * @brief A simple memory cache.
 *
 * This is a header-only class. It is used by NgapRequestHandler to cache
 * string values. It could be tested more completely and used in other places.
 *
 * The cache implements a simple FIFO queue to purge the oldest entries when
 * the cache is full. Purging is done when the count of cached items exceeds
 * the maximum number of items allowed in the cache. The private purge method
 * will remove the oldest P entries from the cache where P is the number of
 * 'purge items' as set with the initialize() method.
 *
 * Instead of initializing the cache (setting the max and purge item counts)
 * using a constructor, am initialize() method is used. The constructors never
 * throw exceptions. If initialize() fails, it returns false. The get() and put()
 * methods may throw exceptions.
 *
 * The cache is not thread-safe.
 *
 * @tparam VALUE As of 10/17/23 only tested with std::string
 */
template <typename VALUE>
class MemoryCache {
    unsigned int d_max_items = 100;       //< Max number of items to cache (not bytes, but items)
    unsigned int d_purge_items = 20;     //< When purging, remove this many items

    std::deque<std::string> d_fifo_keys; //< FIFO queue of keys; used by purge()
    std::unordered_map<std::string, VALUE> d_cache;

    /// Purge the cache of the oldest entries.
    void purge() {
        // if number of elements > threshold, purge
        for (int entries = 0; entries < d_purge_items; ++entries) {
            std::string key = d_fifo_keys.front();

            d_cache.erase(d_cache.find(key));
            d_fifo_keys.pop_front();
        }
    }

    /// Check the cache for consistency.
    bool invariant(bool expensive = true) const {
        if (d_cache.size() > d_max_items)
            return false;
        if (d_fifo_keys.size() > d_max_items)
            return false;
        if (d_cache.size() != d_fifo_keys.size())
            return false;

        if (expensive) {
            // check that the keys in the queue are also in the map
            for (const auto &key : d_fifo_keys) {
                if (d_cache.find(key) == d_cache.end())
                    return false;
            }
        }

        return true;
    }

    friend class MemoryCacheTest;

public:
    MemoryCache() = default;    // this makes testing easier
    MemoryCache(const MemoryCache &src) = delete;
    MemoryCache &operator=(const MemoryCache &src) = delete;
    virtual ~MemoryCache() = default;

    /**
     * @brief Initialize the cache.
     * @param max_items Must be greater than zero
     * @param purge_items Must be greater than zero
     * @return Return True if the cache was initialized, false otherwise.
     */
    virtual bool initialize(int max_items, int purge_items) {
        if (max_items <= 0 || purge_items <= 0)
            return false;

        d_max_items = (unsigned int)max_items;
        d_purge_items = (unsigned int)purge_items;
        return true;
    }

    /**
     * @brief Get the item from the cache.
     * If the item is not in the cache, the value-result parameter is not modified.
     * @param key
     * @param value Value-result parameter; operator=() is used to copy the value of the cached item.
     * @return Return True if the item is in the cache, false otherwise.
     */
    virtual bool get(const std::string &key, VALUE &value) {
        if (d_cache.find(key) != d_cache.end()) {
            value = d_cache[key];
            return true;
        }
        else {
            return false;
        }
    }

    /**
     * @brief Put the item in the cache.
     * If the key is already in the cache, the value is updated.
     * @param key
     * @param value
     */
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

    /// @brief How many items are in the cache?
    virtual unsigned long size() const { return d_cache.size(); }

    /// @brief Clear the cache
    virtual void clear() { d_cache.clear(); d_fifo_keys.clear(); }
};

} // ngap

#endif //BES_MEMORYCACHE_H
