
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

#include <time.h>
#include <cassert>

#include <string>
#include <map>

#include <DapObj.h>

//namespace bes {

/**
 * @brief An in-memory cache for DDS objects
 *
 * This cache stores pointers to DDS objects in memory (not on a
 * disk) and thus it is not a persistent cache. It provides no
 * assurances regarding multi-process thread safety. Since the
 * cache stores pointers, it assumes that some other part of the
 * software has allocated the cached pointer and will be responsible
 * for deleting it. The software using the cache is responsible for
 * ensuring that the pointers in the cache reference valid objects
 * (or that it doesn't matter if they don't...).
 *
 */
class ObjMemCache {
private:
    struct Entry {
        libdap::DapObj *d_obj; // A weak pointer - we do not manage this storage
        const std::string d_name;

        // We need the string so that we can delete the index entry easily
        Entry(libdap::DapObj *o, const std::string &n): d_obj(o), d_name(n) { }
        ~Entry() { }
    };

    unsigned int d_count;

    typedef std::pair<unsigned int, Entry*> cache_pair_t;  // used by map::insert()
    typedef std::map<unsigned int, Entry*> cache_t;
    cache_t cache;

    typedef std::pair<const std::string, unsigned int> index_pair_t;
    typedef std::map<const std::string, unsigned int> index_t;
    index_t index;

    friend class DDSMemCacheTest;

public:
    ObjMemCache(): d_count(0) { }

    virtual ~ObjMemCache();

    virtual void add(libdap::DapObj *obj, const std::string &key);

    virtual void remove(const std::string &key);

    virtual libdap::DapObj *get_obj(const std::string &key);
    virtual libdap::DapObj *extract_obj(const string &key);

    /**
     * @brief How many items are in the cache
     * @return
     */
    virtual unsigned int get_cache_size() const {
        assert(cache.size() == index.size());
        return cache.size();
    }

    virtual void purge(float fraction = 0.2);

    /**
     * @brief What is in the cache
     * @param os
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
