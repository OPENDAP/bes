// BESCache3.h

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2014 OPeNDAP, Inc
// Author: James Gallagher <jgallagher@opendap.org>,
// Patrick West <pwest@ucar.edu> and Jose Garcia <jgarcia@ucar.edu>
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
// You can contact University Corporation for Atmospheric Research at
// 3080 Center Green Drive, Boulder, CO 80301

#ifndef BESCache3_h_
#define BESCache3_h_ 1
#if 0

#include <string>

#include "BESFileLockingCache.h"
#include "BESDebug.h"

class BESKeys;
/** @brief This class is a shallow wrapper for BESFileLockingCache. It is only
 * used by gateway_module and BESFileContainer. DO NOT USE THIS CLASS! Use the
 * BESFileLockingCache instead!!
 *
 * Implementation of a caching mechanism for compressed data.
 * This cache uses simple advisory locking found on most modern unix file systems.
 * Compressed files are uncompressed and stored in a cache where they can be
 * used over and over until removed from the cache. Several processes can
 * share the cache with each reading from files. At the same time, new files
 * can be added and the cache can be purged, without disrupting the existing
 * read operations.
 *
 * How it works. When a file is added to the cache, the cache is locked - no
 * other processes can add, read or remove files. Once a file has been added,
 * the cache size is examined and, if needed, the cache is purged so that its
 * size is 80% of the maximum size. Then the cache is unlocked. When a process
 * looks to see if a file is already in the cache, the entire cache is locked.
 * If the file is present, a shared read lock is obtained and the cache is unlocked.
 *
 * Methods: create_and_lock() and get_read_lock() open and lock files; the former
 * creates the file and locks it exclusively iff it does not exist, while the
 * latter obtains a shared lock iff the file already exists. The unlock()
 * methods unlock a file. The lock_cache_info() and unlock_cache_info() are
 * used to control access to the whole cache - with the open + lock and
 * close + unlock operations performed atomically. Other methods that operate
 * on the cache info file must only be called when the lock has been obtained.
 */
class BESCache3: public BESFileLockingCache {

private:
	static BESCache3 * d_instance;

protected:
    BESCache3(): BESFileLockingCache() {}
    BESCache3(const string &cache_dir, const string &prefix, unsigned long size) :
    	BESFileLockingCache(cache_dir, prefix, size) {}
    BESCache3(BESKeys *keys, const string &cache_dir_key, const string &prefix_key, const string &size_key);

public:
    virtual ~BESCache3() { }

    static void delete_instance() { delete d_instance; d_instance = 0; }

    static BESCache3 *get_instance(BESKeys *keys, const string &cache_dir_key, const string &prefix_key, const string &size_key);
    static BESCache3 *get_instance(const string &cache_dir, const string &prefix, unsigned long size); // Testing
    static BESCache3 *get_instance();

    virtual void dump(ostream &strm) const ;
};
#endif

#endif // BESCache3_h_
