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
 * ObjMemCache.cc
 *
 *  Created on: May 18, 2016
 *      Author: jimg
 */


#include "config.h"

#include <string>
#include <map>

#include <DapObj.h>
#include <InternalErr.h>

#include "ObjMemCache.h"

// using namespace bes {

using namespace std;
using namespace libdap;

ObjMemCache::~ObjMemCache()
{
    for (cache_t::iterator i = cache.begin(), e = cache.end(); i != e; ++i) {
        assert(i->second);
        delete i->second;
    }
}

/**
 * @brief Added a DDS to the cache and associate it with a key
 * Add the pointer to the cache, purging the cache of the least
 * recently used items if the cache was initialized with a specific
 * threshold value. If not, the caller must take care of calling
 * the purge() method.
 * @param obj
 * @param key
 */
void ObjMemCache::add(DapObj *obj, const string &key)
{
    ++d_count;

    // if d_entries_threshold is zero, the caller handles calling
    // purge.
    if (d_entries_threshold && (d_count > d_entries_threshold))
        purge(d_purge_threshold);

    index.insert(index_pair_t(key, d_count));

    cache.insert(cache_pair_t(d_count, new Entry(obj, key)));
}

/**
 * @brief Remove the DDS associated with a key
 * @param key
 */
void ObjMemCache::remove(const string &key)
{
    index_t::iterator i = index.find(key);

    if (i != index.end()) {
        unsigned int count = i->second;
        index.erase(i);
        cache_t::iterator c = cache.find(count);
        assert(c != cache.end());
        assert(c->second);  // should never cache a null ptr
        delete c->second;   // delete the Entry*, but not the contained obj*
        cache.erase(c);
    }
}

/**
 * @brief Get the cached item and return true or return false
 * @param key
 * @param cached_dds
 * @return
 */
DapObj *ObjMemCache::get(const string &key)
{
    DapObj *cached_obj = 0;

    index_t::iterator i = index.find(key);
    if (i != index.end()) {
        cache_t::iterator c = cache.find(i->second);
        assert(c != cache.end());
        // leave this second test in, but unless the cache is
        // broken, it should never be false.
        if (c != cache.end()) {
            assert(c->second);
            // get the Entry and the DDS
            Entry *e = c->second;
            cached_obj = e->d_obj;  // cached_obj == the return value

            // now erase & reinsert the pair
            cache.erase(c);
            cache.insert(cache_pair_t(++d_count, e));
        }
        else {
            // I'm leaving the test and this exception in because getting
            // a bad DDS will lead to a bug that is hard to figure out. Other
            // parts of the code I'm assuming assert() calls are good enough.
            // jhrg 5/20/16
            throw InternalErr(__FILE__, __LINE__, "Memory cache consistency error.");
        }

        // update the index
        index.erase(i);
        index.insert(index_pair_t(key, d_count));
    }

    return cached_obj;
}

#if 0
/**
 * @brief Extract (Get/Remove) the object associated with key
 * @param key
 * @return
 */
DapObj *ObjMemCache::extract(const string &key)
{
    DapObj *cached_obj = 0;

    index_t::iterator i = index.find(key);

    if (i != index.end()) {
        unsigned int count = i->second;
        index.erase(i);
        cache_t::iterator c = cache.find(count);
        assert(c != cache.end());
        assert(c->second);  // should never cache a null ptr

        cached_obj = c->second->d_obj;

        delete c->second;   // delete the Entry*, but not the contained obj*
        cache.erase(c);
    }

    return cached_obj;
}
#endif

/**
 * @brief Purge the oldest elements
 *
 * @param fraction (default is 0.2)
 */
void ObjMemCache::purge(float fraction)
{
    // Map are ordered using less by default, so the oldest entries are first
    size_t num_remove = cache.size() * fraction;

    cache_t::iterator c = cache.begin(), e = cache.end();
    for (unsigned int i = 0; i < num_remove && c != e; ++i) {
        const string name = c->second->d_name;
        delete c->second;   // deletes the Entry, not the obj that its internals point to
        cache.erase(c);
        c = cache.begin();  // erase() invalidates the iterator

        index_t::iterator pos = index.find(name);
        assert(pos != index.end());
        index.erase(pos);
    }
}

// } namespace bes
