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
 * DDSMemCache.cc
 *
 *  Created on: May 18, 2016
 *      Author: jimg
 */

#include "config.h"
#include <DDSMemCache.h>

// using namespace bes {

using namespace std;
using namespace libdap;

DDSMemCache::~DDSMemCache()
{
    for (cache_t::iterator i = cache.begin(), e = cache.end(); i != e; ++i) {
        assert(i->second);
        delete i->second;
    }
}

void DDSMemCache::add(DDS *dds, const string &key)
{
    ++d_count;

    index.insert(index_pair_t(key, d_count));

    cache.insert(cache_pair_t(d_count, new Entry(dds, key)));
}

void DDSMemCache::remove(const string &key)
{
    cerr << __PRETTY_FUNCTION__ << " " << key << endl;
    //cerr << "index[" << key << "]: " << index[key] << endl;

    index_t::iterator i = index.find(key);
    cerr << "i != index.end(): " << (i != index.end()) << endl;
    if (i != index.end()) {
        cerr << "found" << endl;
        unsigned int count = i->second;

        index.erase(i);

        cache_t::iterator c = cache.find(count);
        if (c != cache.end()) {
            assert(c->second);  // should never cache a null ptr
            delete c->second;   // delete the Entry*
            cache.erase(c);
        }
    }
}

/**
 * @brief Get the cached item and return true or return false
 * @param key
 * @param cached_dds
 * @return
 */
DDS *DDSMemCache::get_dds(const string &key)
{
    DDS *cached_dds = 0;

    index_t::iterator i = index.find(key);
    if (i != index.end()) {
        cache_t::iterator c = cache.find(i->second);
        if (c != cache.end()) {
            assert(c->second);
            // get the Entry and the DDS
            Entry *e = c->second;
            cached_dds = e->d_dds;

            // now erase & reinsert the pair
            cache.erase(c);
            cache.insert(cache_pair_t(++d_count, e));
        }

        // update the index
        index.erase(i);
        index.insert(index_pair_t(key, d_count));
    }

    return cached_dds;
}

/**
 * @brief Purge the oldest elements
 *
 * @param fraction (default is 0.2)
 */
void DDSMemCache::purge(float fraction)
{
    // Map are ordered using less by default, so the oldest entries are first
    size_t num_remove = cache.size() * fraction;

    cache_t::iterator c = cache.begin(), e = cache.end();
    for (unsigned int i = 0; i < num_remove && c != e; ++i) {
        const string name = c->second->d_name;
        delete c->second;   // deletes the Entry, not the DDS that its internals point to
        cache.erase(c);
        c = cache.begin();  // erase() invalidates the iterator

        index_t::iterator pos = index.find(name);
        assert(pos != index.end());
        index.erase(pos);
    }
}

// } namespace bes
