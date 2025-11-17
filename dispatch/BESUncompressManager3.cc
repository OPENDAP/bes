// BESUncompressManager3.cc

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2012 OPeNDAP, Inc
// Author: James Gallagher <jgallagher@opendap.org>
// 		   Patrick West <pwest@ucar.edu> and
//		   Jose Garcia <jgarcia@ucar.edu>
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

#include "config.h"

#include <mutex>

#include <sstream>

using std::endl;
using std::istringstream;
using std::ostream;
using std::string;

#include "BESUncompress3BZ2.h"
#include "BESUncompress3GZ.h"
#include "BESUncompress3Z.h"
#include "BESUncompressManager3.h"

#include "BESFileLockingCache.h"

#include "BESDebug.h"
#include "BESInternalError.h"

#include "TheBESKeys.h"

BESUncompressManager3 *BESUncompressManager3::d_instance = nullptr;
static std::once_flag d_euc_init_once;

/** @brief constructs an uncompression manager adding gz, z, and bz2
 * uncompression methods by default.
 *
 * Adds methods to uncompress gz, bz2, and Z files.
 *
 * Looks for a configuration parameter for the number of times to try to
 * lock the cache (BES.Uncompress.NumTries) and the time in microseconds
 * between tries (BES.Uncompress.Retry).
 */
BESUncompressManager3::BESUncompressManager3() {
    add_method("gz", BESUncompress3GZ::uncompress);
    add_method("bz2", BESUncompress3BZ2::uncompress);
    add_method("Z", BESUncompress3Z::uncompress);
}

BESUncompressManager3::~BESUncompressManager3() {}

/** @brief create_and_lock a uncompress method to the list
 *
 * This method actually adds to the list a static method that knows how to
 * uncompress a particular type of file. For example, a .gz or .bz2 file.
 *
 * @param name name of the method to add to the list
 * @param method the static function that uncompress the particular type of file
 * @return true if successfully added, false if it already exists
 */
bool BESUncompressManager3::add_method(const string &name, p_bes_uncompress method) {
    std::lock_guard<std::recursive_mutex> lock_me(d_cache_lock_mutex);

    BESUncompressManager3::UCIter i;
    i = _uncompress_list.find(name);
    if (i == _uncompress_list.end()) {
        _uncompress_list[name] = method;
        return true;
    }
    return false;
}

/** @brief returns the uncompression method specified
 *
 * This method looks up the uncompression method with the given name and
 * returns that method.
 *
 * @param name name of the uncompression method to find
 * @return the function of type p_bes_uncompress
 */
p_bes_uncompress BESUncompressManager3::find_method(const string &name) {
    std::lock_guard<std::recursive_mutex> lock_me(d_cache_lock_mutex);

    BESUncompressManager3::UCIter i;
    i = _uncompress_list.find(name);
    if (i != _uncompress_list.end()) {
        return (*i).second;
    }
    return 0;
}

/** @brief If the file 'src' should be uncompressed, do so and return a
 *  new file name on the value-result param 'target'.
 *
 *  This code tests that the file named by 'src' really should be uncompressed,
 *  returning false if it clearly should not (i.e., it does not end in an
 *  extension like '.gz') or cannot. If the file is uncompressed, this
 *  code takes care of doing all the stuff that needs to happen as far as
 *  caching the uncompressed data, ensuring that the file holding those
 *  data is locked for read-only access.
 *
 *  When new data are added to the cache, this code checks the size and, if
 *  the cache is too large, uses a LRU algorithm to remove files to make
 *  room for the new file.
 *
 *  @note As implemented, the purge() code removes files so that the
 *  resulting size of the cache will be about 80% of the maximum size of
 *  the cache as set in the bes.conf file.
 *
 * @note If there is a problem uncompressing the file, the uncompress code is
 * responsible for closing the source file, the target file, AND
 * REMOVING THE TARGET FILE. If the target file is left in place after
 * an error, then the cache might use that file in the future for a
 * request.
 *
 * @param src file to be uncompressed
 * @param cache_file Name of file to uncompress into
 * @param cache BESCache object to uncompress the src file in
 * @return true if the file's contents are in the cache and at the pathname
 * cfile
 * @throws BESInternalError if there is a problem uncompressing
 * the file
 */
bool BESUncompressManager3::uncompress(const string &src, string &cache_file, BESFileLockingCache *cache) {
    std::lock_guard<std::recursive_mutex> lock_me(d_cache_lock_mutex);

    BESDEBUG("uncompress2", "BESUncompressManager3::uncompress() - src: " << src << endl);

    /**
     * If the cache object is a null pointer then we can't go further, and
     * we know that the item isn't in the cache.
     * FIXME IS THIS AN ERROR??
     * I think maybe it's fine returning false and not throwing because
     * this means the down stream software will try to read the file
     * and, since this test is after checks that determine if the file
     * appears to be compressed, will fail. This may however be difficult
     * to diagnose for the users.
     */
    if (cache == NULL) {
        std::ostringstream oss;
        oss << "BESUncompressManager3::" << __func__ << "() - ";
        oss << "The supplied Cache object is NULL. Decompression Requires An Operational Cache.";
        throw BESInternalError(oss.str(), __FILE__, __LINE__);
    }

    // All compressed files have a 'dot extension'.
    string::size_type dot = src.rfind(".");
    if (dot == string::npos) {
        BESDEBUG("uncompress2", "BESUncompressManager3::uncompress() - no file extension" << endl);
        return false;
    }

    string ext = src.substr(dot + 1, src.size() - dot);

    // If there's no match for the extension, the file is not compressed and we return false.
    // Otherwise, 'p' points to a function that uncompresses the data.
    p_bes_uncompress p = find_method(ext);
    if (!p) {
        BESDEBUG("uncompress2", "BESUncompressManager3::uncompress() - not compressed " << endl);
        return false;
    }

    // Get the name of the file in the cache (either the code finds this file or
    // it makes it).
    cache_file = cache->get_cache_file_name(src);

#if 0
    try {
        BESDEBUG( "uncompress2", "BESUncompressManager3::uncompress() - is cached? " << src << endl );
#endif

    int fd;
    if (cache->get_read_lock(cache_file, fd)) {
        BESDEBUG("uncompress", "BESUncompressManager3::uncompress() - cached hit: " << cache_file << endl);
        return true;
    }

    // Now we actually try to uncompress the file, given that there's not a decomp'd version
    // in the cache. First make an empty file and get an exclusive lock on it.
    if (cache->create_and_lock(cache_file, fd)) {
        BESDEBUG("uncompress", "BESUncompressManager3::uncompress() - caching " << cache_file << endl);

        // uncompress. Make sure that the decompression function does not close
        // the file descriptor.
        p(src, fd);

        // Change the exclusive lock on the new file to a shared lock. This keeps
        // other processes from purging the new file and ensures that the reading
        // process can use it.
        cache->exclusive_to_shared_lock(fd);

        // Now update the total cache size info and purge if needed. The new file's
        // name is passed into the purge method because this process cannot detect its
        // own lock on the file.
        unsigned long long size = cache->update_cache_info(cache_file);
        if (cache->cache_too_big(size))
            cache->update_and_purge(cache_file);

        return true;
    } else {
        if (cache->get_read_lock(cache_file, fd)) {
            BESDEBUG("uncompress", "BESUncompressManager3::uncompress() - cached hit: " << cache_file << endl);
            return true;
        }
    }

    return false;
#if 0
    }
    catch (...) {
    	BESDEBUG( "uncompress", "BESUncompressManager3::uncompress() - caught exception, unlocking cache and re-throw." << endl );
        cache->unlock_cache();
        throw;
    }

    return false;   // gcc warns without this
#endif
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance along with the names of the
 * registered decompression methods.
 *
 * @param strm C++ i/o stream to dump the information to
 */
void BESUncompressManager3::dump(ostream &strm) const {
    std::lock_guard<std::recursive_mutex> lock_me(d_cache_lock_mutex);

    strm << BESIndent::LMarg << "BESUncompressManager3::dump - (" << (void *)this << ")" << endl;
    BESIndent::Indent();
    if (_uncompress_list.size()) {
        strm << BESIndent::LMarg << "registered uncompression methods:" << endl;
        BESIndent::Indent();
        BESUncompressManager3::UCIter i = _uncompress_list.begin();
        BESUncompressManager3::UCIter ie = _uncompress_list.end();
        for (; i != ie; i++) {
            strm << BESIndent::LMarg << (*i).first << endl;
        }
        BESIndent::UnIndent();
    } else {
        strm << BESIndent::LMarg << "registered uncompress methods: none" << endl;
    }
    BESIndent::UnIndent();
}

BESUncompressManager3 *BESUncompressManager3::TheManager() {
    std::call_once(d_euc_init_once, BESUncompressManager3::initialize_instance);
    return d_instance;
}

void BESUncompressManager3::initialize_instance() {
    d_instance = new BESUncompressManager3;
#ifdef HAVE_ATEXIT
    atexit(delete_instance);
#endif
}

void BESUncompressManager3::delete_instance() {
    delete d_instance;
    d_instance = 0;
}
