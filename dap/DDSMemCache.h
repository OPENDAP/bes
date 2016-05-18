
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
 * DDSMemCache.h
 *
 *  Created on: May 18, 2016
 *      Author: jimg
 */

#ifndef DAP_DDSMEMCACHE_H_
#define DAP_DDSMEMCACHE_H_

#include <time.h>
#include <cassert>

#include <string>
#include <map>

#include <DDS.h>

//namespace bes {

class DDSMemCache {
private:
    struct Entry {
        libdap::DDS d_dds;
        const string &d_name;

        Entry(libdap::DDS d, const string &n): d_dds(d), d_name(n) { }
        ~Entry() { }

        libdap::DDS get_dds() const { return d_dds; }
        const string &get_name() const { return d_name; }
    };

    // Lookup the time stamp using the dataset name, then use the time stamp
    // to lookup the DDS. This lets the code use the ordering of the 'cache'
    // map to simplify purging the oldest elements.
    typedef map<time_t, Entry*> cache_t;
    cache_t cache;
    typedef map<const string, time_t> index_t;
    index_t index;

public:
    DDSMemCache() { }

    virtual ~DDSMemCache();

    virtual void add(libdap::DDS &dds, const std::string &key);

    virtual void remove(const std::string &key);

    virtual bool get_dds(const std::string &key, libdap::DDS &cached_dds);

    virtual void purge(float fraction = 0.2);

    virtual void dump(ostream &os) {
        os << "DDSMemCache" << endl;
    }
};

// } namespace bes

#endif /* DAP_DDSMEMCACHE_H_ */
