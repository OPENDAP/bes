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

class BESKeys;

/** @brief Implementation of a caching mechanism for files that have been
 * decompressed.
 */
class BESCache2: public BESObj {

private:
    static BESCache2 * d_instance;

    static const char BES_CACHE_CHAR = '#';

    string d_cache_dir;  /// pathname of the cache directory
    string d_prefix;     /// tack this on the front of cache file name

    /// How many megabytes can the cache hold before we have to purge
    unsigned long long d_max_cache_size_in_megs;

    // This class implements a singleton, so the constructor is hidden.
    BESCache2(BESKeys *keys, const string &cache_dir_key, const string &prefix_key, const string &size_key);
    // Testing
    BESCache2(const string &cache_dir, const string &prefix, unsigned long size);

    // Suppress the assignment operator and default copy ctor, ...
    BESCache2() { }
    BESCache2(const BESCache2 &rhs) { }
    BESCache2 &operator=(const BESCache2 &rhs) { }

    void m_check_ctor_params();

    /// for filename -> filesize map below
    struct cache_entry {
        string name;
        unsigned long long size;
    };

    // Sugar for the multimap of entries sorted with older files first.
    typedef std::multimap<double, cache_entry, std::less<double> > CacheFilesByAgeMap;

    CacheFilesByAgeMap d_contents;
    unsigned long long m_collect_cache_dir_info();

    /// Name of the file that tracks the size of the cache
    string d_cache_info;
    int d_cache_info_fd;

    // map that relates files to the descriptor used to obtain a lock
    typedef std::map<std::string, int> FilesAndLockDescriptors;
    FilesAndLockDescriptors d_locks;

public:
    virtual ~BESCache2() { }

    virtual void record_descriptor(const string &file, int fd) {
    	d_locks[file] = fd;
    }

    virtual int get_descriptor(const string &file) {
    	return d_locks{file};
    }

    string get_cache_file_name(const string &src);

    virtual bool create_and_lock(const string &target, int &fd);
    virtual bool get_read_lock(const string &target, int &fd);
    virtual void unlock(const string &target);
    virtual void unlock(int fd);

    virtual unsigned long long update_cache_info(const string &target);
    virtual bool lock_cache_info();
    virtual void unlock_cache_info();

    virtual bool cache_too_big();
    virtual void purge();

    static BESCache2 *get_instance(BESKeys *keys, const string &cache_dir_key, const string &prefix_key, const string &size_key);
    static BESCache2 *get_instance(const string &cache_dir, const string &prefix, unsigned long size); // Testing
    static BESCache2 *get_instance();

    virtual void dump(ostream &strm) const ;
};

#endif // BESCache2_h_
