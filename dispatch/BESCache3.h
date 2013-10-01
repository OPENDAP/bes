// BESCache3.h

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2012 OPeNDAP, Inc
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

#include <map>
#include <string>

#include "BESObj.h"
#include "BESDebug.h"

class BESKeys;

// These typedefs are used to record information about the files in the cache.
// See BESCache3.cc and look at the purge() method.
typedef struct {
    string name;
    unsigned long long size;
    time_t time;
} cache_entry;

typedef std::list<cache_entry> CacheFiles;

/** @brief Implementation of a caching mechanism for compressed data.
 * This cache uses simple advisory locking found on most modern unix file systems.
 * Compressed files are decompressed and stored in a cache where they can be
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
class BESCache3: public BESObj {

private:
    static BESCache3 * d_instance;

    static const char BES_CACHE_CHAR = '#';

    string d_cache_dir;  /// pathname of the cache directory
    string d_prefix;     /// tack this on the front of cache file name

    /// How many megabytes can the cache hold before we have to purge
    unsigned long long d_max_cache_size_in_bytes;
    // When we purge, how much should we throw away. Set in the ctor to 80% of the max size.
    unsigned long long d_target_size;

    // This class implements a singleton, so the constructor is hidden.
    BESCache3(BESKeys *keys, const string &cache_dir_key, const string &prefix_key, const string &size_key);
    // Testing
    BESCache3(const string &cache_dir, const string &prefix, unsigned long size);

    // Suppress the assignment operator and default copy ctor, ...
    BESCache3() { }
    BESCache3(const BESCache3 &) :BESObj() { }
    BESCache3 &operator=(const BESCache3 &) { return *this; }

    void m_check_ctor_params();
    void m_initialize_cache_info();

    unsigned long long m_collect_cache_dir_info(CacheFiles &contents);

    /// Name of the file that tracks the size of the cache
    string d_cache_info;
    int d_cache_info_fd;

    void m_record_descriptor(const string &file, int fd);
    int m_get_descriptor(const string &file);

    // map that relates files to the descriptor used to obtain a lock
    typedef std::map<string, int> FilesAndLockDescriptors;
    FilesAndLockDescriptors d_locks;

public:
    virtual ~BESCache3() { }

    string get_cache_file_name(const string &src);

    virtual bool create_and_lock(const string &target, int &fd);
    virtual bool get_read_lock(const string &target, int &fd);
    virtual void exclusive_to_shared_lock(int fd);
    virtual void unlock_and_close(const string &target);
    virtual void unlock_and_close(int fd);

    virtual void lock_cache_write();
    virtual void lock_cache_read();
    virtual void unlock_cache();

    virtual unsigned long long update_cache_info(const string &target);
    virtual bool cache_too_big(unsigned long long current_size) const;
    virtual unsigned long long get_cache_size();
    virtual void update_and_purge(const string &new_file);

    static BESCache3 *get_instance(BESKeys *keys, const string &cache_dir_key, const string &prefix_key, const string &size_key);
    static BESCache3 *get_instance(const string &cache_dir, const string &prefix, unsigned long size); // Testing
    static BESCache3 *get_instance();

    virtual void dump(ostream &strm) const ;
};

#endif // BESCache3_h_
