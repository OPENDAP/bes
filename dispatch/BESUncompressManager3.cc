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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// You can contact University Corporation for Atmospheric Research at
// 3080 Center Green Drive, Boulder, CO 80301

#include <sstream>

using std::istringstream;

#include "BESUncompressManager3.h"
#include "BESUncompress3GZ.h"
#include "BESUncompressBZ2.h"
#include "BESUncompressZ.h"
#include "BESCache3.h"

#include "BESInternalError.h"
#include "BESDebug.h"

#include "TheBESKeys.h"

BESUncompressManager3 *BESUncompressManager3::_instance = 0;

/** @brief constructs an uncompression manager adding gz, z, and bz2
 * uncompression methods by default.
 *
 * Adds methods to uncompress gz, bz2, and Z files.
 *
 * Looks for a configuration parameter for the number of times to try to
 * lock the cache (BES.Uncompress.NumTries) and the time in microseconds
 * between tries (BES.Uncompress.Retry).
 */
BESUncompressManager3::BESUncompressManager3()
{
    add_method("gz", BESUncompress3GZ::uncompress);
#if 0
    add_method("bz2", BESUncompressBZ2::uncompress);
    add_method("Z", BESUncompressZ::uncompress);
#endif
}

/** @brief create_and_lock a uncompress method to the list
 *
 * This method actually adds to the list a static method that knows how to
 * uncompress a particular type of file. For example, a .gz or .bz2 file.
 *
 * @param name name of the method to add to the list
 * @param method the static function that uncompress the particular type of file
 * @return true if successfully added, false if it already exists
 */
bool BESUncompressManager3::add_method(const string &name, p_bes_uncompress method)
{
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
p_bes_uncompress BESUncompressManager3::find_method(const string &name)
{
    BESUncompressManager3::UCIter i;
    i = _uncompress_list.find(name);
    if (i != _uncompress_list.end()) {
        return (*i).second;
    }
    return 0;
}

/** @brief If the file 'src' should be decompressed, do so and return a
 *  new file name on the value-result param 'target'.
 *
 *  This code tests that the file named by 'src' really should be decompressed,
 *  returning false if it clearly should not (i.e., it does not end in an
 *  extension like '.gz') or cannot. If the file is decompressed, this
 *  code takes care of doing all the stuff that needs to happen as far as
 *  caching the decompressed data, ensuring that the file holding those
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
 * @param target target file to uncompress into
 * @param cache BESCache object to uncompress the src file in
 * @return true if the file's contents are in the cache and at the pathname
 * cfile
 * @throws BESInternalError if there is a problem uncompressing
 * the file
 */
bool BESUncompressManager3::uncompress(const string &src, string &cfile, BESCache3 *cache)
{
    BESDEBUG( "uncompress2", "uncompress - src: " << src << endl );

    // All compressed files have a 'dot extension'.
    string::size_type dot = src.rfind(".");
    if (dot == string::npos) {
        BESDEBUG( "uncompress2", "uncompress - no file extension" << endl );
        return false;
    }

    string ext = src.substr(dot + 1, src.length() - dot);

    // If there's no match for the extension, the file is not compressed and we return false.
    // Otherwise, 'p' points to a function that decompresses the data.
    p_bes_uncompress p = find_method(ext);
    if (!p) {
        BESDEBUG( "uncompress2", "uncompress - not compressed " << endl );
        return false;
    }

    // Get the name of the file in the cache (either the code finds this file or
    // or it makes it).
    cfile = cache->get_cache_file_name(src);

    try {
        // Get an exclusive lock on the cache info file - this prevents purge operations
        // by other processes. The cache must be locked to prevent purging while we get
        // either the shared read lock on a file already in the cache or an exclusive
        // write lock needed to add a new file to the cache.

    	// FIXME Does this has to be here or can the cache lock be moved below?
    	if (!cache->lock_cache())
            throw BESInternalError("Could not lock the cache info file.", __FILE__, __LINE__);

    	// If we are here, the file has an extension that identifies it as compressed. Is a
        // decompressed version in the cache already?
        BESDEBUG( "uncompress2", "uncompress - is cached? " << src << endl );

        // The cache is purged here because it's possible to delete the file just created
        // when using fcntl(2) locking because a process cannot detect when it holds the lock
        // on a file - only when another process holds the lock. So read locking the newly
        // created file will prevent another process from getting the exclusive lock needed to
        // delete a file, but not the process that holds the read lock.
        int fd;
        if (cache->get_read_lock(cfile, fd)) {
            BESDEBUG( "uncompress", "uncompress - cached hit: " << cfile << endl );
            // Once we have a shared read lock, allow other process to access the cache
            cache->unlock_cache();
            return true;
        }

    	// Here we need to look at the size of the cache and purge if needed
        // Note that fcntl(2) locks do not work within a process. Thus it is not possible
    	// to determine if the current process has placed a lock on a particular file (only
    	// if another process has). Thus the purge operation has to precede the 'add a new
    	// file operation (because the purge might remove that file).

        unsigned long long size = cache->get_cache_size();
        if (cache->cache_too_big(size))
            cache->purge(size);

        // Now we actually try to decompress the file, given that there's not a decomp'd version
        // in the cache. First make an empty file and get an exclusive lock on it.
        if (cache->create_and_lock(cfile, fd)) {
            BESDEBUG( "uncompress", "uncompress - caching " << cfile << endl );

#if 1
            cache->unlock_cache();
#endif
            // FIXME Catch exceptions here and close fd
            // decompress
            p(src, fd);

#if 1
            if (!cache->lock_cache())
                throw BESInternalError("Could not lock the cache info file.", __FILE__, __LINE__);
#endif

            // Now update the total cache size info.
            unsigned long long size = cache->update_cache_info(cfile);
            BESDEBUG( "uncompress", "uncompress - cache size now " << size << endl );

            // FIXME The process can transfer the lock without closing the file
            cache->exclusive_to_shared_lock(fd);
#if 0
            // Release the (exclusive) write lock on the new file
            cache->unlock(fd);

            // Get a read lock on the new file
            cache->get_read_lock(cfile, fd);
#endif
            // Now unlock cache info since we have a read lock on the new file,
            // the size info has been updated and any purging has completed.
            cache->unlock_cache();

            return true;
        }

        cache->unlock_cache();
        return false;
    }
    catch (...) {
    	BESDEBUG( "uncompress", "caught exception, unlocking cache and re-throw." << endl );
        cache->unlock_cache();
        throw;
    }
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance along with the names of the
 * registered decompression methods.
 *
 * @param strm C++ i/o stream to dump the information to
 */
void BESUncompressManager3::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << "BESUncompressManager3::dump - (" << (void *) this << ")" << endl;
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
    }
    else {
        strm << BESIndent::LMarg << "registered uncompress methods: none" << endl;
    }
    BESIndent::UnIndent();
}

BESUncompressManager3 *
BESUncompressManager3::TheManager()
{
    if (_instance == 0) {
        _instance = new BESUncompressManager3;
    }
    return _instance;
}
