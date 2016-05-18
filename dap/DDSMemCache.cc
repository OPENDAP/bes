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

void DDSMemCache::add(DDS &dds, const string &key)
{
    time_t t;
    time(&t);

    index_t::iterator i = index.find(key);
    i->second = t;

    cache_t::iterator c = cache.find(t);
    c->second = new Entry(dds, key);
}

void DDSMemCache::remove(const string &key)
{
    index_t::iterator i = index.find(key);
    if (i != index.end()) {
        time_t timestamp = i->second;

        index.erase(key);

        cache_t::iterator c = cache.find(timestamp);
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
bool DDSMemCache::get_dds(const string &key, DDS &cached_dds)
{
    index_t::iterator i = index.find(key);
    if (i != index.end()) {
        time_t orig_timestamp = i->second;

        if (orig_timestamp > 0) {
            time_t new_timestamp;
            time(&new_timestamp);
            i->second = new_timestamp; // update the timestamp

            cache_t::iterator c = cache.find(orig_timestamp);
            if (c != cache.end()) {
                assert(c->second);
                // get the DDS
                cached_dds = c->second->get_dds();

                // now erase & reinsert the pair
                pair<time_t, Entry*> p(new_timestamp, c->second);
                // how expensive is this operation?
                cache.erase(c);
                cache.insert(p);

                return true;
            }
        }
    }

    return false;
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
        const string &name = c->second->get_name();
        delete c->second;
        cache.erase(c);
        index_t::iterator pos = index.find(name);
        assert(pos != index.end());
        index.erase(pos);
    }
}

// } namespace bes
