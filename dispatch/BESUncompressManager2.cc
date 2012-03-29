// BESUncompressManager2.cc

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

#include "BESUncompressManager2.h"
#include "BESUncompressGZ.h"
#include "BESUncompressBZ2.h"
#include "BESUncompressZ.h"
#include "BESCache2.h"

#include "BESInternalError.h"
#include "BESDebug.h"

#include "TheBESKeys.h"

BESUncompressManager2 *BESUncompressManager2::_instance = 0;

/** @brief constructs an uncompression manager adding gz, z, and bz2
 * uncompression methods by default.
 *
 * Adds methods to uncompress gz, bz2, and Z files.
 *
 * Looks for a configuration parameter for the number of times to try to
 * lock the cache (BES.Uncompress.NumTries) and the time in microseconds
 * between tries (BES.Uncompress.Retry).
 */
BESUncompressManager2::BESUncompressManager2()
{
    add_method("gz", BESUncompressGZ::uncompress);
    add_method("bz2", BESUncompressBZ2::uncompress);
    add_method("Z", BESUncompressZ::uncompress);
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
bool BESUncompressManager2::add_method(const string &name, p_bes_uncompress method)
{
    BESUncompressManager2::UCIter i;
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
p_bes_uncompress BESUncompressManager2::find_method(const string &name)
{
    BESUncompressManager2::UCIter i;
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
bool BESUncompressManager2::uncompress(const string &src, string &cfile, BESCache2 *cache)
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

    // If we are here, the file has an extension that identifies it as compressed. Is a
    // decompressed version in the library already?
    BESDEBUG( "uncompress2", "uncompress - is cached? " << src << endl );
    cfile = cache->get_cache_file_name(src);
    int fd;
    if (cache->get_read_lock(cfile, fd)) {
        BESDEBUG( "uncompress", "uncompress - cached hit: " << cfile << endl );
        return true;
    }

    // Now we actually try to decompress the file, given that there's not a decomp'd version
    // in the cache. First make an empty file and get an exclusive lock on it.
    if (cache->create_and_lock(cfile, fd)) {
    	BESDEBUG( "uncompress", "uncompress - caching " << cfile << endl );
#if 0
    	// Here we need to look at the size of the cache and purge if needed
    	if (cache->cache_too_big())
            cache->purge();
#endif
    	// decompress
        p(src, cfile);

        // Now update the size file info.
        unsigned long long size = cache->update_cache_info(cfile);
        BESDEBUG( "uncompress2", "uncompress - cache size now " << size << endl );

        // Before unlocking the file, grab a read lock on the cache info file to prevent
        // another process from deleting this new file in a purge operation before we get
        // it locked for reading.
        if (!cache->lock_cache_info())
            throw BESInternalError("Could not lock the cache info file.", __FILE__, __LINE__);

        // Release the (exclusive) write lock
        cache->unlock(fd);

        // The file descriptor is not used here, but it is recorded so that unlock(name)
        // will work.
        cache->get_read_lock(cfile, fd);

        // Now unlock cache info since we have a read lock on the new file
        cache->unlock_cache_info();

        // Here we need to look at the size of the cache and purge if needed
        if (cache->cache_too_big())
            cache->purge();

        return true;
    }
    else {
        // the code can get here if two processes try to decompress the same file at the same exact
        // time. Both might find the file not in the cache, but only one will be able to create
        // the file for the uncompressed data. This second call will block until the unlock()
        // call above.
        BESDEBUG( "uncompress2", "uncompress - testing is_cached again... " << endl );
        if (cache->get_read_lock(cfile, fd)) {
            BESDEBUG( "uncompress", "uncompress - (second test) cache hit: " << cfile << endl );
            return true;
        }
    }

    throw BESInternalError("Could not decompress the file: " + src, __FILE__, __LINE__);

    // never gets here...
    return false;
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance along with the names of the
 * registered decompression methods.
 *
 * @param strm C++ i/o stream to dump the information to
 */
void BESUncompressManager2::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << "BESUncompressManager2::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();
    if (_uncompress_list.size()) {
        strm << BESIndent::LMarg << "registered uncompression methods:" << endl;
        BESIndent::Indent();
        BESUncompressManager2::UCIter i = _uncompress_list.begin();
        BESUncompressManager2::UCIter ie = _uncompress_list.end();
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

BESUncompressManager2 *
BESUncompressManager2::TheManager()
{
    if (_instance == 0) {
        _instance = new BESUncompressManager2;
    }
    return _instance;
}
