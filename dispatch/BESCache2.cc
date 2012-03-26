// BESCache2.cc

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2012 OPeNDAP, Inc
// Author: James Gallagher <jgallagher@opendap.org>
// Based on code by Patrick West <pwest@ucar.edu> and Jose Garcia <jgarcia@ucar.edu>
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

#include "config.h"

#include <sys/file.h>
#include <unistd.h>  // for unlink
#include <dirent.h>
#include <sys/stat.h>

#include <string>
#include <sstream>
#include <cstring>
#include <cerrno>

#include "BESCache2.h"
#include "TheBESKeys.h"
#include "BESSyntaxUserError.h"
#include "BESInternalError.h"
#include "BESDebug.h"

using namespace std;

// conversion factor
static const unsigned long long BYTES_PER_MEG = 1048576ULL;

// Max cache size in megs, so we can check the user input and warn.
// 2^64 / 2^20 == 2^44
static const unsigned long long MAX_CACHE_SIZE_IN_MEGABYTES = (1ULL << 44);

BESCache2 *BESCache2::d_instance = 0;

// The BESCache2 code is a singleton that assumes it's running in the absence of threads but that
// the cache is shared by several processes, each of which have their own instance of BESCache2.
BESCache2 *
BESCache2::get_instance(BESKeys *keys, const string &cache_dir_key, const string &prefix_key, const string &size_key)
{
    if (d_instance == 0)
        d_instance = new BESCache2(keys, cache_dir_key, prefix_key, size_key);

    return d_instance;
}

BESCache2 *
BESCache2::get_instance(const string &cache_dir, const string &prefix, unsigned long size)
{
    if (d_instance == 0)
        d_instance = new BESCache2(cache_dir, prefix, size);

    return d_instance;
}

BESCache2 *
BESCache2::get_instance()
{
    if (d_instance == 0)
        throw BESInternalError("Tried to get the BESCache2 instance, but it hasn't been created yet", __FILE__, __LINE__);

    return d_instance;
}

/** Get a shared read lock on an existing file.

 @param file_name The name of the file.
 @param ref_fp if successful, the file descriptor of the file on which we
 have a shared read lock.

 @note For all of these functions, the _file_ is locked, not the descriptor.
 So, while they all have file descriptors as value-result parameters, those
 can actually be ignored.

 @return If the file does not exist, return immediately indicating
 failure (false), otherwise block until a shared read-lock can be
 obtained and then return true.

 @exception Error is thrown to indicate a number of untoward
 events. */
static bool getSharedLock(const string &file_name, int &ref_fd)
{
    int fd = open(file_name.c_str(), O_SHLOCK | O_RDONLY);
    if (fd == -1) {
        switch (errno) {
        case ENOENT:
            return false; // This indicates the case where
            // the file does not exist, so it's
            // not an error - the 'test' part
            // of 'test and set' has failed.
        case EACCES:
            throw BESInternalError("Access permission failure", __FILE__, __LINE__);
        case EINTR:
            throw BESInternalError("The open call was interrupted", __FILE__, __LINE__);
        case ELOOP:
            throw BESInternalError("Too many symbolic links", __FILE__, __LINE__);
        case EMFILE:
            throw BESInternalError("Too many open files", __FILE__, __LINE__);
        case ENAMETOOLONG:
            throw BESInternalError("The filename is too big", __FILE__, __LINE__);
        case EOPNOTSUPP:
            throw BESInternalError("Locking is not supported", __FILE__, __LINE__);
        default: {
            ostringstream oss;
            oss << fd;
            throw BESInternalError("Error code: " + oss.str(), __FILE__, __LINE__);
        }
        }
    }

    // Success
    ref_fd = fd;
    return true;
}

/** Get an exclusive read/write lock on an existing file.

 @param file_name The name of the file.
 @param ref_fp if successful, the file descriptor of the file on which we
 have an exclusive read/write lock.

 @return If the file does not exist, return immediately indicating
 failure (false), otherwise block until an exclusive read/write
 lock can be obtained and then return true.

 @exception Error is thrown to indicate a number of untoward
 events. */
static bool getExclusiveLock(string file_name, int &ref_fd)
{
    int fd = open(file_name.c_str(), O_EXLOCK | O_RDWR);
    if (fd == -1) {
        switch (errno) {
        case ENOENT:
            return false; // This indicates the case where
            // the file does not exist, so it's
            // not an error - the 'test' part
            // of 'test and set' has failed.
        case EACCES:
            throw BESInternalError("Access permission failure", __FILE__, __LINE__);
        case EINTR:
            throw BESInternalError("The open call was interrupted", __FILE__, __LINE__);
        case ELOOP:
            throw BESInternalError("Too many symbolic links", __FILE__, __LINE__);
        case EMFILE:
            throw BESInternalError("Too many open files", __FILE__, __LINE__);
        case ENAMETOOLONG:
            throw BESInternalError("The filename is too big", __FILE__, __LINE__);
        case EOPNOTSUPP:
            throw BESInternalError("Locking is not supported", __FILE__, __LINE__);
        default: {
            ostringstream oss;
            oss << fd;
            throw BESInternalError("Error code: " + oss.str(), __FILE__, __LINE__);
        }
        }
    }

    // Success
    ref_fd = fd;
    return true;
}

/** Create a new file and get an exclusive read/write lock on it. If
 the file already exists, this call fails.

 @param file_name The name of the file.
 @param ref_fp if successful, the file descriptor of the file on which we
 have an exclusive read/write lock.

 @return If the file exists, return immediately indicating failure
 (false), otherwise block until the file is created and an
 exclusive read/write lock can be obtained, then return true.

 @exception Error is thrown to indicate a number of untoward
 events. */

static bool createLockedFile(string file_name, int &ref_fd)
{
    int fd = open(file_name.c_str(), O_CREAT | O_EXCL | O_EXLOCK | O_RDWR, 0666);
    if (fd == -1) {
        switch (errno) {
        case EEXIST:
            return false; // This indicates the case where
            // the file already exists, so it's
            // not an error - the 'test' part
            // of 'test and set' has failed.
        case EACCES:
            throw BESInternalError("Access permission failure", __FILE__, __LINE__);
        case EROFS:
            throw BESInternalError("Read-only file system", __FILE__, __LINE__);
        case EINTR:
            throw BESInternalError("The open call was interrupted", __FILE__, __LINE__);
        case EMFILE:
            throw BESInternalError("Too many open files", __FILE__, __LINE__);
        case ENAMETOOLONG:
            throw BESInternalError("The filename is too big", __FILE__, __LINE__);
        case EOPNOTSUPP:
            throw BESInternalError("Locking is not supported", __FILE__, __LINE__);
        case EDQUOT:
            throw BESInternalError("User cannot get more space", __FILE__, __LINE__);
        case ENOSPC:
            throw BESInternalError("No space left on device", __FILE__, __LINE__);
        default: {
            ostringstream oss;
            oss << fd;
            throw BESInternalError("Error code: " + oss.str(), __FILE__, __LINE__);
        }
        }
    }

    // Success
    ref_fd = fd;
    return true;
}

/** Private method */
void BESCache2::m_check_ctor_params()
{
    if (d_cache_dir.empty()) {
        string err = "The cache directory was not specified, must be non-empty";
        throw BESSyntaxUserError(err, __FILE__, __LINE__);
    }

    struct stat buf;
    int statret = stat(d_cache_dir.c_str(), &buf);
    if (statret != 0 || !S_ISDIR(buf.st_mode)) {
        string err = "The cache directory " + d_cache_dir + " does not exist";
        throw BESSyntaxUserError(err, __FILE__, __LINE__);
    }

    if (d_prefix.empty()) {
        string err = "The cache file prefix was not specified, must not be empty";
        throw BESSyntaxUserError(err, __FILE__, __LINE__);
    }

    if (d_max_cache_size_in_megs <= 0) {
        string err = "The cache size was not specified, must be greater than zero";
        throw BESSyntaxUserError(err, __FILE__, __LINE__);
    }

    // If the user specifies a cache that is too large,
    // it is a user exception and we should tell them.
    if (d_max_cache_size_in_megs > MAX_CACHE_SIZE_IN_MEGABYTES) {
        std::ostringstream msg;
        msg << "The specified cache size was larger than the max cache size of: " << MAX_CACHE_SIZE_IN_MEGABYTES;
        throw BESSyntaxUserError(msg.str(), __FILE__, __LINE__);
    }

    BESDEBUG( "bes", "BES Cache: directory " << d_cache_dir
            << ", prefix " << d_prefix
            << ", max size " << d_max_cache_size_in_megs << endl );
}

/** @brief Private constructor that takes as arguments keys to the cache directory,
 * file prefix, and size of the cache to be looked up a configuration file
 *
 * The keys specified are looked up in the specified keys object. If not
 * found or not set correctly then an exception is thrown. I.E., if the
 * cache directory is empty, the size is zero, or the prefix is empty.
 *
 * @param keys BESKeys object used to look up the keys
 * @param cache_dir_key key to look up in the keys file to find cache dir
 * @param prefix_key key to look up in the keys file to find the cache prefix
 * @param size_key key to look up in the keys file to find the cache size
 * @throws BESSyntaxUserError if keys not set, cache dir or prefix empty,
 * size is 0, or if cache dir does not exist.
 */
BESCache2::BESCache2(BESKeys *keys, const string &cache_dir_key, const string &prefix_key, const string &size_key) :
        d_max_cache_size_in_megs(0)
{
    bool found = false;
    keys->get_value(cache_dir_key, d_cache_dir, found);
    if (!found)
        throw BESSyntaxUserError("The cache directory key " + cache_dir_key + " was not found in the BES configuration file", __FILE__, __LINE__);

    found = false;
    keys->get_value(prefix_key, d_prefix, found);
    if (!found)
        throw BESSyntaxUserError("The prefix key " + prefix_key + " was not found in the BES configuration file", __FILE__, __LINE__);

    found = false;
    string cache_size_str;
    keys->get_value(size_key, cache_size_str, found);
    if (!found)
        throw BESSyntaxUserError("The size key " + size_key + " was not found in the BES configuration file", __FILE__, __LINE__);

    std::istringstream is(cache_size_str);
    is >> d_max_cache_size_in_megs;

    m_check_ctor_params(); // Throws BESSyntaxUserError on error.

    d_cache_info = d_cache_dir + "/bes.cache.info";

    // See if we can create it. If so, that means it doesn't exist. So make it and
    // set the cache initial size to zero.
    int fd;
    if (createLockedFile(d_cache_info, fd)) {
        // initialize the cache size to zero
        unsigned long long size = 0;
        if(write(fd, &size, sizeof(unsigned long long)) != sizeof(unsigned long long))
            throw BESInternalError("Could not write size info to the cache info file in startup!", __FILE__, __LINE__);

        unlock(d_cache_info);
    }
}
BESCache2::BESCache2(const string &cache_dir, const string &prefix, unsigned long size) :
        d_cache_dir(cache_dir), d_prefix(prefix), d_max_cache_size_in_megs(size)
{

    m_check_ctor_params(); // Throws BESSyntaxUserError on error.

    d_cache_info = d_cache_dir + "/bes.cache.info";

    // See if we can create it. If so, that means it doesn't exist. So make it and
    // set the cache initial size to zero.
    int fd;
    if (createLockedFile(d_cache_info, fd)) {
        // initialize the cache size to zero
        unsigned long long size = 0;
        if(write(fd, &size, sizeof(unsigned long long)) != sizeof(unsigned long long))
            throw BESInternalError("Could not write size info to the cache info file in startup!", __FILE__, __LINE__);

        unlock(d_cache_info);
    }
}

/** Private method to build the name of file that will holds the uncompressed data from
 * 'src' in the cache.
 *
 * @note How names are mangled: 'src' is the full name of the file to be
 * cached.Tthe file name passed has an extension on the end that will be
 * stripped once the file is cached. For example, if the full path to the
 * file name is /usr/lib/data/fnoc1.nc.gz then the resulting file name
 * will be \#&lt;prefix&gt;\#usr\#lib\#data\#fnoc1.nc.
 */
string BESCache2::m_get_cache_file_name(const string &src)
{
    string target = src;
    if (target.at(0) == '/') {
        target = src.substr(1, target.length() - 1);
    }
    string::size_type slash = 0;
    while ((slash = target.find('/')) != string::npos) {
        target.replace(slash, 1, 1, BESCache2::BES_CACHE_CHAR);
    }
    string::size_type last_dot = target.rfind('.');
    if (last_dot != string::npos) {
        target = target.substr(0, last_dot);
    }

    return d_cache_dir + "/" + d_prefix + BESCache2::BES_CACHE_CHAR + target;
}

/** @brief Get a read-only lock on the file if it exists.
 *
 * Try to get a read-only lock on the file, blocking until we can get it.
 * If the file does not exist, return false.
 *
 * @note If this code returns false, that means the file did not exist
 * in the cache at the time of the test. by the time the caller gets
 * the result, the file may have been added to the cache by another
 * process.
 *
 * @param src src file that will be cached eventually
 * @param target a value-result parameter set to the resulting cached file
 * @return true if the file is in the cache and has been locked, false if
 * the file is/was not in the cache.
 * @throws Error if the attempt to get the (shared) lock failed for any
 * reason other than that the file does/did not exist.
 */
bool BESCache2::get_read_lock(const string &src, string &target)
{
    target = m_get_cache_file_name(src);
    int fd; // not used
    return getSharedLock(target, fd);
}

/** @brief Create a file in the cache and lock it for write access. */
bool BESCache2::create_and_lock(const string &src, string &target, int &fd)
{
    target = m_get_cache_file_name(src);

    return createLockedFile(target, fd);
}

/** Get a shared lock on the 'cache info' file. This is used to prevent
 * another process from getting an exclusive lock on the cache info file and
 * then deleting files. Use this to switch from an exclusive lock used to
 * write a new file and the shared lock used to read from it without
 * having the file deleted in the brief interval between those two events.
 */
bool BESCache2::lock_cache_info()
{
    int fd;
    return getSharedLock(d_cache_info, fd);
}

/** Unlock the name file. This does not do any name mangling; it
 * just unlocks whatever is named (or throws BESInternalError if the file
 * cannot be opened).
 * @param file_name The name of the file to unlock.
 * @throws BESInternalError */
void BESCache2::unlock(const string &file_name)
{
    int fd = open(file_name.c_str(), O_RDONLY);
    if (fd == -1)
        throw BESInternalError("A file was locked and could not be unlocked!", __FILE__, __LINE__);

    if (flock(fd, LOCK_UN) == -1)
        throw BESInternalError("An error occurred trying to unlock the file", __FILE__, __LINE__);
    //unlock(target);
}
void BESCache2::unlock(int fd)
{
    if (flock(fd, LOCK_UN) == -1)
        throw BESInternalError("An error occurred trying to unlock the file", __FILE__, __LINE__);
}

void BESCache2::unlock_cache_info()
{
    unlock(d_cache_info);
}

/** @brief Update teh cache info file to include 'target'
 * Add the size of the named file to the total cache size recorded in the
 * cache info file. The cache info file is exclusively locked by this
 * method for its duration.
 *
 * @param target The name of the file
 */
unsigned long long BESCache2::update_cache_info(const string &target)
{
    // get an exclusive lock on the cache info file and make sure
    // it holds the correct value for the cache size.
    int fd;
    if (!getExclusiveLock(d_cache_info, fd))
        throw BESInternalError("Could not get a lock on the cache info file!", __FILE__, __LINE__);

    // read the size from the cache info file
    unsigned long long current_size;
    if(read(fd, &current_size, sizeof(unsigned long long)) != sizeof(unsigned long long))
        throw BESInternalError("Could not get read size info from the cache info file!", __FILE__, __LINE__);

    struct stat buf;
    int statret = stat(target.c_str(), &buf);
    if (statret == 0)
         current_size += buf.st_size;
    else
        throw BESInternalError("Could not read the size of the new file", __FILE__, __LINE__);

    if(write(fd, &current_size, sizeof(unsigned long long)) != sizeof(unsigned long long))
        throw BESInternalError("Could not write size info from the cache info file!", __FILE__, __LINE__);

    unlock(d_cache_info);

    return current_size;
}

/** @brief look at the cache size; is it too large?
 * Look at the cache size and see if it is too big. This method gets
 * a shared lock on the cache info file.
 *
 * @return True if the size is too big, false otherwise. */
bool BESCache2::cache_too_big()
{
    int fd;
    if (!getSharedLock(d_cache_info, fd))
        throw BESInternalError("Could not get a lock on the cache info file!", __FILE__, __LINE__);

    // read the size from the cache info file
    unsigned long long current_size;
    if(read(fd, &current_size, sizeof(unsigned long long)) != sizeof(unsigned long long))
        throw BESInternalError("Could not get read size info from the cache info file!", __FILE__, __LINE__);

    unlock(d_cache_info);

    return current_size > d_max_cache_size_in_megs;
}

/** @brief Purge files from the cache
 *
 * Purge files, oldest to newest, if the current size of the cache exceeds the
 * size of the cache specified in the constructor. This method uses an exclusive
 * lock on the cache for the duration of the purge process.
 *
 */
void BESCache2::purge()
{
    BESDEBUG("bes", "purge_if_needed - starting the purge" << endl);

    // get an exclusive lock on the cache info file and make sure
    // it holds the correct value for the cache size.
    int fd;
    if (!getExclusiveLock(d_cache_info, fd))
        throw BESInternalError("Could not get a lock on the cache info file!", __FILE__, __LINE__);

    // read the size from the cache info file; I'm interested to see if this always
    // matches the computed value from m_collect_cache_dir_info
    unsigned long long current_size_from_file;
    if(read(fd, &current_size_from_file, sizeof(unsigned long long)) != sizeof(unsigned long long))
        throw BESInternalError("Could not get read size info from the cache info file!", __FILE__, __LINE__);

    unsigned long long current_size = m_collect_cache_dir_info();

    BESDEBUG("bes", "Cache info recorded size: " << current_size_from_file << ", computed size: "
             << current_size <<endl);

    if (BESISDEBUG( "bes" )) {
        BESDEBUG( "bes", endl << "BEFORE" << endl );
        CacheFilesByAgeMap::iterator ti = d_contents.begin();
        CacheFilesByAgeMap::iterator te = d_contents.end();
        for (; ti != te; ti++) {
            BESDEBUG( "bes", (*ti).first << ": " << (*ti).second.name << ": size " << (*ti).second.size << endl );
        }
    }

    // Take the biggest size we can have in MB, convert to Bytes and find 90%
    // When we delete files, delete down to 90% full. This will give the cache
    // some 'breathing room'
    unsigned long long target_size = d_max_cache_size_in_megs * BYTES_PER_MEG *.9;
    // unsigned long long current_size = d_current_cache_file_size;

    while (current_size > target_size) {
        // Grab the first which is the oldest
        // in terms of access time.
        CacheFilesByAgeMap::iterator i = d_contents.begin();

        // if we've deleted all entries, exit the loop
        if (i == d_contents.end())
            break;

        // Otherwise, remove the file with unlink
        BESDEBUG( "bes", "BESCache2::purge - removing " << (*i).second.name << endl );
        // unlink rather than remove in case the file is in use
        // by a forked BES process
        if (unlink((*i).second.name.c_str()) != 0) {
            char *s_err = strerror(errno);
            string err = "Unable to remove the file " + (*i).second.name + " from the cache: ";
            if (s_err) {
                err.append(s_err);
            }
            else {
                err.append("Unknown error");
            }
            throw BESInternalError(err, __FILE__, __LINE__);
        }

        current_size -= (*i).second.size;
        d_contents.erase(i);
    }

    // read the size from the cache info file
    if(write(fd, &current_size, sizeof(unsigned long long)) != sizeof(unsigned long long))
        throw BESInternalError("Could not write size info to the cache info file!", __FILE__, __LINE__);

    unlock(d_cache_info);

    if (BESISDEBUG( "bes" )) {
        BESDEBUG( "bes", endl << "AFTER" << endl );
        CacheFilesByAgeMap::iterator ti = d_contents.begin();
        CacheFilesByAgeMap::iterator te = d_contents.end();
        for (; ti != te; ti++) {
            BESDEBUG( "bes", (*ti).first << ": " << (*ti).second.name << ": size " << (*ti).second.size << endl );
        }
    }
}

/** Private. Get info about all of the files (size and last use time). */
unsigned long long BESCache2::m_collect_cache_dir_info()
{
    DIR *dip = opendir(d_cache_dir.c_str());
    if (!dip)
        throw BESInternalError("Unable to open cache directory " + d_cache_dir, __FILE__, __LINE__);

    struct stat buf;
    struct dirent *dit;
    unsigned long long current_size = 0;
    // go through the cache directory and collect all of the files that
    // start with the matching prefix
    while ((dit = readdir(dip)) != NULL) {
        string dirEntry = dit->d_name;
        if (dirEntry.compare(0, d_prefix.length(), d_prefix) == 0) {
            // Now that we have found a match we want to get the size of
            // the file and the last access time from the file.
            string fullPath = d_cache_dir + "/" + dirEntry;
            int statret = stat(fullPath.c_str(), &buf);
            if (statret == 0) {
                current_size += buf.st_size;
                cache_entry entry;
                entry.name = fullPath;
                entry.size = buf.st_size;
                // Insert information about the current file and its size (entry) sorted
                // by the access time, with smaller (older) times first.
                d_contents.insert(pair<double, cache_entry> (buf.st_atime, entry));
            }
        }
    }

    closedir(dip);

    return current_size;
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance along with information about
 * this cache.
 *
 * @param strm C++ i/o stream to dump the information to
 */
void BESCache2::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << "BESCache2::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();
    strm << BESIndent::LMarg << "cache dir: " << d_cache_dir << endl;
    strm << BESIndent::LMarg << "prefix: " << d_prefix << endl;
    strm << BESIndent::LMarg << "size (mb): " << d_max_cache_size_in_megs << endl;
    BESIndent::UnIndent();
}

