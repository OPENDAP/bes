
// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2016 OPeNDAP
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

/*
 * ObjMemCache.h
 *
 *  Created on: May 18, 2016
 *      Author: jimg
 */

#ifndef DAP_OBJMEMCACHE_H_
#define DAP_OBJMEMCACHE_H_

#include <cassert>

#include <string>
#include <map>

#include "BESIndent.h"

using namespace std;

namespace libdap {
    class DapObj;
}

//namespace bes {

/**
 * @brief An in-memory cache for DapObj (DAS, DDS, ...) objects
 *
 * This cache stores pointers to DapObj objects in memory (not on
 * disk) and thus, it is not a persistent cache. It provides no
 * assurances regarding multi-process or thread safety. Thus, the cache
 * should only be used by a single process - if there are several
 * BES processes, each should have their own copy of the cache.
 *
 * The cache stores pointers to objects, not objects themselves. The
 * user of the cache must take care of copying objects that are added
 * or accessed to/from the cache unless the lifetime of an pointer
 * in the cache will suffice for the use at hand. For example, a cached
 * DAS pointer can be passed to DDS::transfer_attributes(DAS *);
 * there is no need to copy the underlying DAS. However, returning
 * a DAS to the BES for serialization requires that a copy be made
 * since the BES will delete the returned object.
 *
 * The cache implements a LRU purge policy, where when the purge()
 * method is called, the oldest 20% of times are removed. When an
 * item is accessed (add() or get()), it's access time is updated,
 * so the LRU policy is also a low-budget frequency of use policy
 * without actually keeping count of the total number of accesses.
 * The size (number of items, not bytes) of the cache is examined
 * for every add() call and purge() is called if a preset threshold
 * is exceeded. The purge level (20% by default) can be configured.
 *
 * When an object is removed from the cache using remove() or purge(),
 * it is deleted.
 *
 * @note The cache uses an unsigned long long to track the age of
 * items in the cache. It's possible that numbers could wrap around
 * (although that would be a very long-running process) in which case
 * the code will think that the newest objects in the cache are the
 * oldest and remove them. After d_entries_threshold, this will
 * even itself out and the cache will correctly purge the oldest
 * entries.
 *
 */
class ObjMemCache {
private:
    struct Entry {
        libdap::DapObj *d_obj; // A weak pointer - we do not manage this storage
        const std::string d_name;

        // We need the string so that we can erase the index entry easily
        Entry(libdap::DapObj *o, const std::string &n): d_obj(o), d_name(n) { }
        // deleting an Entry deletes the thing it references
        ~Entry() { delete d_obj; d_obj = 0;}
    };

    unsigned long long d_age;           // When obj was add or last accessed
    unsigned int d_entries_threshold;   // no more than this num of entries
    float d_purge_threshold;            // free up this fraction of the cache

    typedef std::pair<unsigned int, Entry*> cache_pair_t;  // used by map::insert()
    typedef std::map<unsigned int, Entry*> cache_t;
    cache_t cache;

    typedef std::pair<const std::string, unsigned int> index_pair_t;
    // efficiency improvement - use an unordered_map when C++-11 is adopted
    typedef std::map<const std::string, unsigned int> index_t;
    index_t index;

    friend class DDSMemCacheTest;

public:
    /**
     * @brief Initialize the DapObj cache
     * This constructor builds a cache that will require the
     * caller manage the purge() operations. Setting the
     * entries_threshold property to zero disables checking the
     * cache size in add().
     * @see purge().
     */
    ObjMemCache(): d_age(0), d_entries_threshold(0), d_purge_threshold(0.2) { }

    /**
     * @brief Initialize the DapObj cache to use an item count threshold
     *
     * The purge() method will be automatically run whenever the threshold
     * value is exceeded and add() is called.
     * @param entries_threashold Purge the cache when this number of
     * items are exceeded.
     * @param purge_threshold When purging items, remove this fraction of
     * the LRU items (e.g., 0.2 --> the oldest 20% items are removed)
     */
    ObjMemCache(unsigned int entries_threshold, float purge_threshold): d_age(0),
        d_entries_threshold(entries_threshold), d_purge_threshold(purge_threshold) {
        // d_entries_threshold = entries_threshold >> 1; // * 2
    }

    virtual ~ObjMemCache();

    virtual void add(libdap::DapObj *obj, const std::string &key);

    virtual void remove(const std::string &key);

    virtual libdap::DapObj *get(const std::string &key);

    /**
     * @brief How many items are in the cache
     * @return The number of items in the cache
     */
    virtual unsigned int size() const {
        assert(cache.size() == index.size());
        return cache.size();
    }

    virtual void purge(float fraction);

    /**
     * @brief What is in the cache
     * @param os Dump info to this stream
     */
    virtual void dump(ostream &os) {
        os << "ObjMemCache" << endl;
        os << "Length of index: " << index.size() << endl;
        for(index_t::const_iterator it = index.begin(); it != index.end(); ++it)  {
            os << it->first << " --> " << it->second << endl;
        }

        os << "Length of cache: " << cache.size() << endl;
        for(cache_t::const_iterator it = cache.begin(); it != cache.end(); ++it)  {
            os << it->first << " --> " << it->second->d_name << endl;
        }
    }
};

// } namespace bes

#endif /* DAP_OBJMEMCACHE_H_ */
