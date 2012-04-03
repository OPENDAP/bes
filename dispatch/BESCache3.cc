// BESCache3.cc

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
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>

#include <string>
#include <sstream>
#include <cstring>
#include <cerrno>

#include "BESCache3.h"

#include "BESSyntaxUserError.h"
#include "BESInternalError.h"

#include "TheBESKeys.h"
#include "BESDebug.h"
#include "BESLog.h"

using namespace std;

// conversion factor
static const unsigned long long BYTES_PER_MEG = 1048576ULL;

// Max cache size in megs, so we can check the user input and warn.
// 2^64 / 2^20 == 2^44
static const unsigned long long MAX_CACHE_SIZE_IN_MEGABYTES = (1ULL << 44);

BESCache3 *BESCache3::d_instance = 0;

// The BESCache3 code is a singleton that assumes it's running in the absence of threads but that
// the cache is shared by several processes, each of which have their own instance of BESCache3.
/** Get an instance of the BESCache3 object. This class is a singleton, so the
 * first call to any of three 'get_instance()' methods makes an instance and subsequent call
 * return a pointer to that instance.
 *
 * @param keys The BESKeys object (hold various configuration parameters)
 * @param cache_dir_key Key to use to get the value of the cache directory
 * @param prefix_key Key for the item/file prefix. Each file added to the cache uses this
 * as a prefix so cached items can be easily identified when /tmp is used for the cache.
 * @param size_key How big should the cache be, in megabytes
 * @return A pointer to a BESCache3 object
 */
BESCache3 *
BESCache3::get_instance(BESKeys *keys, const string &cache_dir_key, const string &prefix_key, const string &size_key)
{
    if (d_instance == 0)
        d_instance = new BESCache3(keys, cache_dir_key, prefix_key, size_key);

    return d_instance;
}

/** Get an instance of the BESCache3 object. This version is used for testing; it does
 * not use the BESKeys object but takes values for the parameters directly.
 */
BESCache3 *
BESCache3::get_instance(const string &cache_dir, const string &prefix, unsigned long size)
{
    if (d_instance == 0)
        d_instance = new BESCache3(cache_dir, prefix, size);

    return d_instance;
}

/** Get an instance of the BESCache3 object. This version is used when there's no
 * question that the cache has been instantiated.
 */
BESCache3 *
BESCache3::get_instance()
{
    if (d_instance == 0)
        throw BESInternalError("Tried to get the BESCache3 instance, but it hasn't been created yet", __FILE__, __LINE__);

    return d_instance;
}

static inline string get_errno() {
	char *s_err = strerror(errno);
	if (s_err)
		return s_err;
	else
		return "Unknown error.";
}

// Apply cmd and type to the entire file.
// cmd is one of F_GETLK, F_SETLK, F_SETLKW, F_UNLCK
// type is one of F_RDLCK, F_WRLCK
static inline int lock(int fd, int cmd, int type) {
    struct flock lock;
    lock.l_type = type;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;
    lock.l_pid = getpid();

    return fcntl(fd, cmd, &lock);
}

void unlock(int fd)
{
    if (lock(fd, F_SETLK, F_UNLCK) == -1) {
        throw BESInternalError("An error occurred trying to unlock the file" + get_errno(), __FILE__, __LINE__);
    }

    if (close(fd) == -1)
        throw BESInternalError("Could not close the (just) unlocked file.", __FILE__, __LINE__);
}

/** Get a shared read lock on an existing file.

 @param file_name The name of the file.
 @param ref_fp if successful, the file descriptor of the file on which we
 have a shared read lock. This is used to release the lock.

 @return If the file does not exist, return immediately indicating
 failure (false), otherwise block until a shared read-lock can be
 obtained and then return true.

 @exception Error is thrown to indicate a number of untoward
 events. */
bool getSharedLock(const string &file_name, int &ref_fd)
{
	BESDEBUG("cache_internal", "getSharedLock: " << file_name <<endl);

    int fd;
    if ((fd = open(file_name.c_str(), O_RDONLY)) < 0) {
        switch (errno) {
        case ENOENT:
            return false;

        default:
            throw BESInternalError(get_errno(), __FILE__, __LINE__);
        }
    }

    if (lock(fd, F_SETLKW, F_RDLCK) == -1) {
        throw BESInternalError(get_errno(), __FILE__, __LINE__);
    }

    BESDEBUG("cache_internal", "getSharedLock exit: " << file_name <<endl);

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
bool getExclusiveLock(string file_name, int &ref_fd)
{
	BESDEBUG("cache_internal", "getExclusiveLock: " << file_name <<endl);

    int fd;
    if ((fd = open(file_name.c_str(), O_RDWR)) < 0) {
        switch (errno) {
        case ENOENT:
            return false;

        default:
            throw BESInternalError(get_errno(), __FILE__, __LINE__);
        }
    }

    if (lock(fd, F_SETLKW, F_WRLCK) == -1) {
        throw BESInternalError(get_errno(), __FILE__, __LINE__);
    }

    BESDEBUG("cache_internal", "getExclusiveLock exit: " << file_name <<endl);

    // Success
    ref_fd = fd;
    return true;
}

/** Get an exclusive read/write lock on an existing file without blocking.

 @param file_name The name of the file.
 @param ref_fp if successful, the file descriptor of the file on which we
 have an exclusive read/write lock.

 @return If the file does not exist or if the file is locked,
 return immediately indicating failure (false), otherwise return true.

 @exception Error is thrown to indicate a number of untoward
 events. */
bool getExclusiveLock_nonblocking(string file_name, int &ref_fd)
{
	BESDEBUG("cache_internal", "getExclusiveLock_nonblocking: " << file_name <<endl);

    int fd;
    if ((fd = open(file_name.c_str(), O_RDWR)) < 0) {
        switch (errno) {
        case ENOENT:
            return false;

        default:
            throw BESInternalError(get_errno(), __FILE__, __LINE__);
        }
    }

    struct flock lock;
    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;
    lock.l_pid = getpid();

    if (fcntl(fd, F_SETLK, &lock) /*lock(fd, F_SETLK, F_WRLCK)*/ == -1) {
        switch (errno) {
        case EAGAIN:
            BESDEBUG("cache_internal", "getExclusiveLock_nonblocking exit (false): " << file_name << " by: " << lock.l_pid << endl);
            return false;

        default:
         throw BESInternalError(get_errno(), __FILE__, __LINE__);
        }
    }

    BESDEBUG("cache_internal", "getExclusiveLock_nonblocking exit (true): " << file_name <<endl);

    // Success
    ref_fd = fd;
    return true;
}
#if 1
// FIXME Not used
// This cannot be used on a file that this process has open because it will release
// any locks when close is called (and it will never show that file as locked).
bool isLocked(string file_name)
{
	BESDEBUG("cache_internal", "isLocked: " << file_name <<endl);

    int fd;
    if ((fd = open(file_name.c_str(), O_WRONLY)) < 0) {
        throw BESInternalError(get_errno(), __FILE__, __LINE__);
    }

    struct flock lock;
    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;
    lock.l_pid = getpid();

    if (fcntl(fd, F_GETLK, &lock) == -1) {
        close(fd);
        throw BESInternalError(get_errno(), __FILE__, __LINE__);
    }

    close(fd);

    // Success
    if (lock.l_type == F_UNLCK) {
        BESDEBUG("cache_internal", "isLocked exit (false): " << file_name << endl);
        return false;
    }
    else {
    	BESDEBUG("cache_internal", "isLocked exit (true): " << file_name << " by: " << lock.l_pid << endl);
    	return true;
    }

}
#endif
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
bool createLockedFile(string file_name, int &ref_fd)
{
	BESDEBUG("cache_internal", "createLockedFile: " << file_name <<endl);

    int fd;
    if ((fd = open(file_name.c_str(), O_CREAT | O_EXCL | O_RDWR, 0666)) < 0) {
        switch (errno) {
        case EEXIST:
            return false;

        default:
            throw BESInternalError(get_errno(), __FILE__, __LINE__);
        }
    }

    if (lock(fd, F_SETLKW, F_WRLCK) == -1) {
        throw BESInternalError(get_errno(), __FILE__, __LINE__);
    }

    BESDEBUG("cache_internal", "createLockedFile exit: " << file_name <<endl);

    // Success
    ref_fd = fd;
    return true;
}

/** Private method */
void BESCache3::m_check_ctor_params()
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

    if (d_max_cache_size_in_bytes <= 0) {
        string err = "The cache size was not specified, must be greater than zero";
        throw BESSyntaxUserError(err, __FILE__, __LINE__);
    }

    // If the user specifies a cache that is too large,
    // it is a user exception and we should tell them.
    if (d_max_cache_size_in_bytes > MAX_CACHE_SIZE_IN_MEGABYTES) {
        std::ostringstream msg;
        msg << "The specified cache size was larger than the max cache size of: " << MAX_CACHE_SIZE_IN_MEGABYTES;
        throw BESSyntaxUserError(msg.str(), __FILE__, __LINE__);
    }

    BESDEBUG( "cache", "BES Cache: directory " << d_cache_dir
            << ", prefix " << d_prefix
            << ", max size " << d_max_cache_size_in_bytes << endl );
}

/** Private method. */
void BESCache3::m_initialize_cache_info()
{
    d_max_cache_size_in_bytes = min(d_max_cache_size_in_bytes, MAX_CACHE_SIZE_IN_MEGABYTES);
    if (d_max_cache_size_in_bytes > MAX_CACHE_SIZE_IN_MEGABYTES)
    	*(BESLog::TheLog()) << "Cache size too big in configuration file, set to max limit." << endl ;

    d_max_cache_size_in_bytes *= BYTES_PER_MEG;
    d_target_size = d_max_cache_size_in_bytes * 0.8;

    m_check_ctor_params(); // Throws BESSyntaxUserError on error.

    d_cache_info = d_cache_dir + "/bes.cache.info";

    // See if we can create it. If so, that means it doesn't exist. So make it and
    // set the cache initial size to zero.
    if (createLockedFile(d_cache_info, d_cache_info_fd)) {
        // initialize the cache size to zero
        unsigned long long size = 0;
        if(write(d_cache_info_fd, &size, sizeof(unsigned long long)) != sizeof(unsigned long long))
            throw BESInternalError("Could not write size info to the cache info file in startup!", __FILE__, __LINE__);

        unlock(d_cache_info_fd);
    }
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
 * @param size_key key to look up in the keys file to find the cache size (in MBytes)
 * @throws BESSyntaxUserError if keys not set, cache dir or prefix empty,
 * size is 0, or if cache dir does not exist.
 */
BESCache3::BESCache3(BESKeys *keys, const string &cache_dir_key, const string &prefix_key, const string &size_key) :
        d_max_cache_size_in_bytes(0)
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
    is >> d_max_cache_size_in_bytes;

    m_initialize_cache_info();
}

/** Private.
 * @note size is in megabytes; but the class maintains the max size in bytes. ...must convert here.
 * @param cache_dir
 * @param prefix
 * @param size
 */
BESCache3::BESCache3(const string &cache_dir, const string &prefix, unsigned long size) :
        d_cache_dir(cache_dir), d_prefix(prefix), d_max_cache_size_in_bytes(size)
{
    m_initialize_cache_info();
}

/** Build the name of file that will holds the uncompressed data from
 * 'src' in the cache.
 *
 * @note How names are mangled: 'src' is the full name of the file to be
 * cached.Tthe file name passed has an extension on the end that will be
 * stripped once the file is cached. For example, if the full path to the
 * file name is /usr/lib/data/fnoc1.nc.gz then the resulting file name
 * will be \#&lt;prefix&gt;\#usr\#lib\#data\#fnoc1.nc.
 */
string BESCache3::get_cache_file_name(const string &src)
{
    string target = src;
    if (target.at(0) == '/') {
        target = src.substr(1, target.length() - 1);
    }
    string::size_type slash = 0;
    while ((slash = target.find('/')) != string::npos) {
        target.replace(slash, 1, 1, BESCache3::BES_CACHE_CHAR);
    }
    string::size_type last_dot = target.rfind('.');
    if (last_dot != string::npos) {
        target = target.substr(0, last_dot);
    }

    return d_cache_dir + "/" + d_prefix + BESCache3::BES_CACHE_CHAR + target;
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
bool BESCache3::get_read_lock(const string &target, int &fd)
{
    bool status = getSharedLock(target, fd);

    BESDEBUG("cache", "BES Cache: read_lock: " << target << "(" << status << ")" << endl);

    if (status)
    	record_descriptor(target, fd);

    return status;
}

/** @brief Create a file in the cache and lock it for write access. */
bool BESCache3::create_and_lock(const string &target, int &fd)
{
    bool status = createLockedFile(target, fd);

    BESDEBUG("cache", "BES Cache: create_and_lock: " << target << "(" << status << ")" << endl);

    if (status)
    	record_descriptor(target, fd);

    return status;

}

void BESCache3::exclusive_to_shared_lock(int fd)
{
    struct flock lock;
    lock.l_type = F_RDLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;
    lock.l_pid = getpid();

    if (fcntl(fd, F_SETLKW, &lock) /*lock(fd, F_SETLK, F_WRLCK)*/ == -1) {
        throw BESInternalError(get_errno(), __FILE__, __LINE__);
    }
}

/** Get an exclusive lock on the 'cache info' file. This is used to prevent
 * another process from getting an exclusive lock on the cache info file and
 * then deleting files. Use this to switch from an exclusive lock used to
 * write a new file and the shared lock used to read from it without
 * having the file deleted in the brief interval between those two events.
 */
bool BESCache3::lock_cache()
{
    return getExclusiveLock(d_cache_info, d_cache_info_fd);
}

/** Unlock the named file. This does not do any name mangling; it
 * just unlocks whatever is named (or throws BESInternalError if the file
 * cannot be closed).
 * @param file_name The name of the file to unlock.
 * @throws BESInternalError */
void BESCache3::unlock(const string &file_name)
{
    BESDEBUG("cache", "BES Cache: unlock file: " << file_name << endl);

    unlock(get_descriptor(file_name));
}

/** Unlock the file. This does not do any name mangling; it
 * just unlocks whatever is named (or throws BESInternalError if the file
 * cannot be closed).
 * @param fd The descriptor of the file to unlock.
 * @throws BESInternalError */
void BESCache3::unlock(int fd)
{
    BESDEBUG("cache", "BES Cache: unlock fd: " << fd << endl);

    ::unlock(fd);

    BESDEBUG("cache", "BES Cache: unlock " << fd << " Success" << endl);
}

/** Unlock the cache info file. This also closes the file.
 *
 */
void BESCache3::unlock_cache()
{
    BESDEBUG("cache", "BES Cache: unlock: cache_info (fd: " << d_cache_info_fd << ")" << endl);
    unlock(d_cache_info_fd);
}

/** @brief Update the cache info file to include 'target'
 * Add the size of the named file to the total cache size recorded in the
 * cache info file. The cache info file is exclusively locked by this
 * method for its duration.
 *
 * @param target The name of the file
 */
unsigned long long BESCache3::update_cache_info(const string &target)
{
    // If we can lock the cache info file, that's bad because it should already
    // be exclusively locked.
#if 0
    // This test does not work when using fcntl(2) locking because fcntl(2) will
    // 'upgrade' the lock within a process; this call will always succeed unless
    // another process has the file locked (which cannot be true by design).
	if (getExclusiveLock_nonblocking(d_cache_info, d_cache_info_fd))
        throw BESInternalError("Expected the cache info file to be locked!", __FILE__, __LINE__);
#endif

	if (lseek(d_cache_info_fd, 0, SEEK_SET) == -1)
        throw BESInternalError("Could not rewind to front of cache info file.", __FILE__, __LINE__);

    // read the size from the cache info file
    unsigned long long current_size;
    if(read(d_cache_info_fd, &current_size, sizeof(unsigned long long)) != sizeof(unsigned long long))
        throw BESInternalError("Could not get read size info from the cache info file!", __FILE__, __LINE__);

    struct stat buf;
    int statret = stat(target.c_str(), &buf);
    if (statret == 0)
         current_size += buf.st_size;
    else
        throw BESInternalError("Could not read the size of the new file: " + target + " : " + get_errno(), __FILE__, __LINE__);

    BESDEBUG("cache", "BES Cache: cache size updated to: " << current_size << endl);

    if (lseek(d_cache_info_fd, 0, SEEK_SET) == -1)
        throw BESInternalError("Could not rewind to front of cache info file.", __FILE__, __LINE__);

    if(write(d_cache_info_fd, &current_size, sizeof(unsigned long long)) != sizeof(unsigned long long))
        throw BESInternalError("Could not write size info from the cache info file!", __FILE__, __LINE__);

    return current_size;
}

/** @brief look at the cache size; is it too large?
 * Look at the cache size and see if it is too big.
 *
 * @return True if the size is too big, false otherwise. */
bool BESCache3::cache_too_big(unsigned long long current_size) const
{
    return current_size > d_max_cache_size_in_bytes;
}

unsigned long long BESCache3::get_cache_size() const
{
    if (lseek(d_cache_info_fd, 0, SEEK_SET) == -1)
        throw BESInternalError("Could not rewind to front of cache info file.", __FILE__, __LINE__);
	// read the size from the cache info file
	unsigned long long current_size;
	if(read(d_cache_info_fd, &current_size, sizeof(unsigned long long)) != sizeof(unsigned long long))
	    throw BESInternalError("Could not get read size info from the cache info file!", __FILE__, __LINE__);
    return current_size;
}


static bool entry_op(cache_entry &e1, cache_entry &e2)
{
    return e1.time < e2.time;
}

/** Private. Get info about all of the files (size and last use time). */
unsigned long long BESCache3::m_collect_cache_dir_info(CacheFiles &contents)
{
    DIR *dip = opendir(d_cache_dir.c_str());
    if (!dip)
        throw BESInternalError("Unable to open cache directory " + d_cache_dir, __FILE__, __LINE__);

    struct dirent *dit;
    vector<string> files;
    // go through the cache directory and collect all of the files that
    // start with the matching prefix
    while ((dit = readdir(dip)) != NULL) {
        string dirEntry = dit->d_name;
        if (dirEntry.compare(0, d_prefix.length(), d_prefix) == 0) {
            files.push_back(d_cache_dir + "/" + dirEntry);
        }
    }

    closedir(dip);

    unsigned long long current_size = 0;
    struct stat buf;
    for (vector<string>::iterator file = files.begin(); file != files.end(); ++file) {
        if (stat(file->c_str(), &buf) == 0) {
            current_size += buf.st_size;
            cache_entry entry;
            entry.name = *file;
            entry.size = buf.st_size;
            entry.time = buf.st_atime;
            // Sanity check; Removed after initial testing since some files might be zero bytes
#if 0
            if (entry.size == 0)
                throw BESInternalError("Zero-byte file found in cache. " + *file, __FILE__, __LINE__);
#endif
            contents.push_back(entry);
        }
    }

    // Sort so smaller (older) times are first.
    contents.sort(entry_op);

    return current_size;
}

/** @brief Purge files from the cache
 *
 * Purge files, oldest to newest, if the current size of the cache exceeds the
 * size of the cache specified in the constructor. This method uses an exclusive
 * lock on the cache for the duration of the purge process.
 *
 */
void BESCache3::purge(unsigned long long current_size)
{
    BESDEBUG("cache_purge", "purge - starting the purge" << endl);
#if 0
    // See comment about fcntl(2) locking in update_cache)info() above.
    if (getExclusiveLock_nonblocking(d_cache_info, d_cache_info_fd))
        throw BESInternalError("Expected the cache info file to be locked!", __FILE__, __LINE__);
#endif
    CacheFiles contents;
    unsigned long long computed_size = m_collect_cache_dir_info(contents);

#if 0
    // FIXME if we keep this, the code in uncompress can be simplified
    // Sanity check...
    if (current_size != computed_size) {
        ostringstream oss;
        oss << "Cache info recorded size and computed size differ: " << current_size << ", " << computed_size;
        throw BESInternalError(oss.str(), __FILE__, __LINE__);
    }
#endif

    if (BESISDEBUG( "cache_contents" )) {
        BESDEBUG( "cache_contents", endl << "BEFORE Purge " << computed_size/BYTES_PER_MEG << endl );
        CacheFiles::iterator ti = contents.begin();
        CacheFiles::iterator te = contents.end();
        for (; ti != te; ti++) {
            BESDEBUG( "cache_contents", (*ti).time << ": " << (*ti).name << ": size " << (*ti).size/BYTES_PER_MEG << endl );
        }
    }

    BESDEBUG( "cache_purge", "purge - current and target size (in MB) " << computed_size/BYTES_PER_MEG  << ", " << d_target_size/BYTES_PER_MEG << endl );

    // d_target_size is 80% of the maximum cache size.
    // Grab the first which is the oldest in terms of access time.
    CacheFiles::iterator i = contents.begin();
    while (computed_size > d_target_size) {
        // If we've processed all entries, exit the loop.
        if (i == contents.end())
            break;

        // Grab an exclusive lock but do not block - if another process has the file locked
        // just move on to the next file.
        int cfile_fd;
        if (/*!isLocked(i->name) && */getExclusiveLock_nonblocking(i->name, cfile_fd)) {
            BESDEBUG( "cache_purge", "purge: " << i->name << " removed." << endl );

            if (unlink(i->name.c_str()) != 0)
                throw BESInternalError("Unable to purge the file " + i->name + " from the cache: " + get_errno(), __FILE__, __LINE__);

            unlock(cfile_fd);
            computed_size -= i->size;
#if 0
            // This code used to try to reuse the list
            // If the file was removed from the cache, remove from the list of files too
            contents.erase(i);
            i = contents.begin();
#endif
        }
        else {
            // This information is useful when debugging... Might comment out for production
            BESDEBUG( "cache_purge", "purge: " << i->name << " is in use." << endl );
#if 0
            ++i;
#endif
        }

        ++i;

        BESDEBUG( "cache_purge", "purge - current and target size (in MB) " << computed_size/BYTES_PER_MEG << ", " << d_target_size/BYTES_PER_MEG << endl );
    }

    if (lseek(d_cache_info_fd, 0, SEEK_SET) == -1)
        throw BESInternalError("Could not rewind to front of cache info file.", __FILE__, __LINE__);

    if(write(d_cache_info_fd, &computed_size, sizeof(unsigned long long)) != sizeof(unsigned long long))
        throw BESInternalError("Could not write size info to the cache info file!", __FILE__, __LINE__);

    if (BESISDEBUG( "cache_contents" )) {
    	contents.clear();
        computed_size = m_collect_cache_dir_info(contents);
        BESDEBUG( "cache_contents", endl << "AFTER Purge " << computed_size/BYTES_PER_MEG << endl );
        CacheFiles::iterator ti = contents.begin();
        CacheFiles::iterator te = contents.end();
        for (; ti != te; ti++) {
            BESDEBUG( "cache_contents", (*ti).time << ": " << (*ti).name << ": size " << (*ti).size/BYTES_PER_MEG << endl );
        }
    }
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance along with information about
 * this cache.
 *
 * @param strm C++ i/o stream to dump the information to
 */
void BESCache3::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << "BESCache3::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();
    strm << BESIndent::LMarg << "cache dir: " << d_cache_dir << endl;
    strm << BESIndent::LMarg << "prefix: " << d_prefix << endl;
    strm << BESIndent::LMarg << "size (bytes): " << d_max_cache_size_in_bytes << endl;
    BESIndent::UnIndent();
}

