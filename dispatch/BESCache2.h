// BESCache2.h

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2012 OPeNDAP, Inc
// Author: James Gallagher <jgallagher@opendap.org>
// Based in code by Patrick West <pwest@ucar.edu> and Jose Garcia <jgarcia@ucar.edu>
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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// You can contact University Corporation for Atmospheric Research at
// 3080 Center Green Drive, Boulder, CO 80301

#ifndef BESCache2_h_
#define BESCache2_h_ 1

#include <algorithm>
#include <map>
#include <string>
#include <sstream>

#include "BESObj.h"
#include "BESDebug.h"

class BESKeys;

typedef struct {
    string name;
    unsigned long long size;
    time_t time;
} cache_entry;

typedef std::list<cache_entry> CacheFiles;

#if 0
//class tally_file_info;
/// for filename -> filesize map below
typedef struct {
    string name;
    unsigned long long size;
    time_t time;
} cache_entry;

// Sugar for the multimap of entries sorted with older files first.
typedef std::list<cache_entry> CacheFiles;
#endif

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
class BESCache2: public BESObj {

private:
    static BESCache2 * d_instance;

    static const char BES_CACHE_CHAR = '#';

    string d_cache_dir;  /// pathname of the cache directory
    string d_prefix;     /// tack this on the front of cache file name

    /// How many megabytes can the cache hold before we have to purge
    unsigned long long d_max_cache_size_in_bytes;
    // When we purge, how much should we throw away. Set in the ctor to 80% of the max size.
    unsigned long long d_target_size;

    // This class implements a singleton, so the constructor is hidden.
    BESCache2(BESKeys *keys, const string &cache_dir_key, const string &prefix_key, const string &size_key);
    // Testing
    BESCache2(const string &cache_dir, const string &prefix, unsigned long size);

    // Suppress the assignment operator and default copy ctor, ...
    BESCache2() { }
    BESCache2(const BESCache2 &rhs) { }
    BESCache2 &operator=(const BESCache2 &rhs) { }

    void m_check_ctor_params();
    void m_initialize_cache_info();
#if 0
    /// for filename -> filesize map below
    struct cache_entry {
        string name;
        unsigned long long size;
        time_t time;
    };

    // Sugar for the multimap of entries sorted with older files first.
#if 0
    typedef std::multimap<time_t, cache_entry, std::less<time_t> > CacheFilesByAgeMap;
    CacheFilesByAgeMap d_contents;
#endif
    typedef std::list<cache_entry> CacheFiles;
    CacheFiles d_contents;
    unsigned long long m_collect_cache_dir_info();
#endif

    unsigned long long m_collect_cache_dir_info(CacheFiles &contents);

    /// Name of the file that tracks the size of the cache
    string d_cache_info;
    int d_cache_info_fd;

    // map that relates files to the descriptor used to obtain a lock
    typedef std::map<string, int> FilesAndLockDescriptors;
    FilesAndLockDescriptors d_locks;

    virtual void record_descriptor(const string &file, int fd) {
        BESDEBUG("cache", "BES Cache: recording descriptor: " << file << ", " << fd << endl);
        d_locks.insert(std::pair<string, int>(file, fd));
    }

    virtual int get_descriptor(const string &file) {
        FilesAndLockDescriptors::iterator i = d_locks.find(file);
        int fd = i->second;
        BESDEBUG("cache", "BES Cache: getting descriptor: " << file << ", " << fd << endl);
        d_locks.erase(i);
        return fd;
    }

public:
    virtual ~BESCache2() { }

    string get_cache_file_name(const string &src);

    virtual bool create_and_lock(const string &target, int &fd);
    virtual bool get_read_lock(const string &target, int &fd);
    virtual void unlock(const string &target);
    virtual void unlock(int fd);

    virtual bool lock_cache_info();
    virtual void unlock_cache_info();

    virtual unsigned long long update_cache_info(const string &target);
    virtual bool cache_too_big(unsigned long long current_size);
    virtual void purge(unsigned long long current_size);

    static BESCache2 *get_instance(BESKeys *keys, const string &cache_dir_key, const string &prefix_key, const string &size_key);
    static BESCache2 *get_instance(const string &cache_dir, const string &prefix, unsigned long size); // Testing
    static BESCache2 *get_instance();

    virtual void dump(ostream &strm) const ;
    //void tally_file_info(const string &file);
    friend bool entry_op(cache_entry &e1, cache_entry &e2);
    //friend unsigned long long collect_cache_dir_info(CacheFiles &d_contents);
};

#endif // BESCache2_h_
