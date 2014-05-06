// BESCache3.cc

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2012 OPeNDAP, Inc
// Author: James Gallagher <jgallagher@opendap.org>
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
static inline struct flock *lock(int type) {
    static struct flock lock;
    lock.l_type = type;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;
    lock.l_pid = getpid();

    return &lock;
}

inline void BESCache3::m_record_descriptor(const string &file, int fd) {
    BESDEBUG("cache", "BES Cache: recording descriptor: " << file << ", " << fd << endl);
    d_locks.insert(std::pair<string, int>(file, fd));
}

// Modified on 5/6/14 to support several open descriptors for a single file
// name. jhrg
inline int BESCache3::m_get_descriptor(const string &file) {
    FilesAndLockDescriptors::iterator i = d_locks.find(file);
    if (i == d_locks.end())
    	return -1;

    int fd = i->second;
    BESDEBUG("cache", "BES Cache: getting descriptor: " << file << ", " << fd << endl);
    d_locks.erase(i);
    return fd;
}

/** Unlock and close the file descriptor.
 *
 * @param fd The file descriptor to close.
 * @throws BESInternalError if either fnctl(2) or open(2) return an error.
 */
static void unlock(int fd)
{
    if (fcntl(fd, F_SETLK, lock(F_UNLCK)) == -1) {
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
static bool getSharedLock(const string &file_name, int &ref_fd)
{
	BESDEBUG("cache_internal", "getSharedLock: " << file_name <<endl);

    int fd;
    if ((fd = open(file_name.c_str(), O_RDONLY)) < 0) {
        switch (errno) {
        case ENOENT:
            return false;

        default:
            throw BESInternalError("Could not get shared lock for " + file_name + ": " + get_errno(), __FILE__, __LINE__);
        }
    }

    struct flock *l = lock(F_RDLCK);
    if (fcntl(fd, F_SETLKW, l) == -1) {
        close(fd);
    	ostringstream oss;
    	oss << "cache process: " << l->l_pid << " triggered a locking error: " << get_errno();
        throw BESInternalError(oss.str(), __FILE__, __LINE__);
    }

    BESDEBUG("cache_internal", "getSharedLock exit: " << file_name <<endl);

    // Success
    ref_fd = fd;
    return true;
}

#if 0
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
	BESDEBUG("cache_internal", "getExclusiveLock: " << file_name <<endl);

    int fd;
    if ((fd = open(file_name.c_str(), O_RDWR)) < 0) {
        switch (errno) {
        case ENOENT:
            return false;

        default:
            throw BESInternalError("Could not get exclusive lock for " + file_name + ": " + get_errno(), __FILE__, __LINE__);
        }
    }

    struct flock *l = lock(F_WRLCK);
    if (fcntl(fd, F_SETLKW, l) == -1) {
        close(fd);
    	ostringstream oss;
    	oss << "cache process: " << l->l_pid << " triggered a locking error: " << get_errno();
        throw BESInternalError(oss.str(), __FILE__, __LINE__);
    }

    BESDEBUG("cache_internal", "getExclusiveLock exit: " << file_name <<endl);

    // Success
    ref_fd = fd;
    return true;
}
#endif

/** Get an exclusive read/write lock on an existing file without blocking.

 @param file_name The name of the file.
 @param ref_fp if successful, the file descriptor of the file on which we
 have an exclusive read/write lock.

 @return If the file does not exist or if the file is locked,
 return immediately indicating failure (false), otherwise return true.

 @exception Error is thrown to indicate a number of untoward
 events. */
static bool getExclusiveLockNB(string file_name, int &ref_fd)
{
	BESDEBUG("cache_internal", "getExclusiveLock_nonblocking: " << file_name <<endl);

    int fd;
    if ((fd = open(file_name.c_str(), O_RDWR)) < 0) {
        switch (errno) {
        case ENOENT:
            return false;

        default:
            throw BESInternalError("Could not get a non-blocking exclusive lock for " + file_name + ": " + get_errno(), __FILE__, __LINE__);
        }
    }

    struct flock *l = lock(F_WRLCK);
    if (fcntl(fd, F_SETLK, l) == -1) {
        switch (errno) {
        case EAGAIN:
            BESDEBUG("cache_internal", "getExclusiveLock_nonblocking exit (false): " << file_name << " by: " << l->l_pid << endl);
            close(fd);
            return false;

        default: {
            close(fd);
        	ostringstream oss;
        	oss << "cache process: " << l->l_pid << " triggered a locking error: " << get_errno();
        	throw BESInternalError(oss.str(), __FILE__, __LINE__);
        }
        }
    }

    BESDEBUG("cache_internal", "getExclusiveLock_nonblocking exit (true): " << file_name <<endl);

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
	BESDEBUG("cache_internal", "createLockedFile: " << file_name <<endl);

    int fd;
    if ((fd = open(file_name.c_str(), O_CREAT | O_EXCL | O_RDWR, 0666)) < 0) {
        switch (errno) {
        case EEXIST:
            return false;

        default:
            throw BESInternalError("Could not create locked file (" + file_name + "): " + get_errno(), __FILE__, __LINE__);
        }
    }

    struct flock *l = lock(F_WRLCK);
    if (fcntl(fd, F_SETLKW, l) == -1) {
        close(fd);
    	ostringstream oss;
    	oss << "cache process: " << l->l_pid << " triggered a locking error: " << get_errno();
        throw BESInternalError(oss.str(), __FILE__, __LINE__);
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

    BESDEBUG( "cache_internal", "BES Cache: directory " << d_cache_dir
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
		if (write(d_cache_info_fd, &size, sizeof(unsigned long long)) != sizeof(unsigned long long))
			throw BESInternalError("Could not write size info to the cache info file in startup!", __FILE__, __LINE__);

		// This leaves the d_cache_info_fd file descriptor open
		unlock_cache();
	}
	else {
		if ((d_cache_info_fd = open(d_cache_info.c_str(), O_RDWR)) == -1) {
			throw BESInternalError("Could not open the cache info file (" + d_cache_info + "): " + get_errno(), __FILE__, __LINE__);
		}
	}

    BESDEBUG("cache_internal", "d_cache_info_fd: " << d_cache_info_fd << endl);
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
 * @note size is in megabytes; but the class maintains the max size in bytes.
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
	lock_cache_read();

    bool status = getSharedLock(target, fd);

    BESDEBUG("cache_internal", "BES Cache: get_read_lock: " << target << "(" << status << ")" << endl);

    if (status)
    	m_record_descriptor(target, fd);

    unlock_cache();

    return status;
}

/** @brief Create a file in the cache and lock it for write access.
 * If the file does not exist, make it, open it for read-write access and
 * get an exclusive lock on it. The locking operation blocks, although that
 * should never happen.
 * @param target The name of the file to make/open/lock
 * @param fd Value-result param that holds the file descriptor of the opened
 * file
 * @return True if the operation was successful, false otherwise. This method will
 * return false if the file already existed (the file won't be locked and the
 * descriptor reference is undefined - but likely -1).
 * @throws BESInternalError if any error except EEXIST is returned by open(2) or
 * if fcntl(2) returns an error. */
bool BESCache3::create_and_lock(const string &target, int &fd)
{
	lock_cache_write();

    bool status = createLockedFile(target, fd);

    BESDEBUG("cache_internal", "BES Cache: create_and_lock: " << target << "(" << status << ")" << endl);

    if (status)
    	m_record_descriptor(target, fd);

    unlock_cache();

    return status;

}

/** @brief Transfer from an exclusive lock to a shared lock.
 * If the file has an exclusive write lock on it, change that to a shared
 * read lock. This is an atomic operation. If the call to fcntl(2) is
 * protected by locking the cache, a dead lock will result given typical use
 * of this class. This method exists to help with the situation where one
 * process has the cache locked and is blocking on a shared read lock for
 * a file that a second process has locked exclusively (for writing). By
 * changing the exclusive lock to a shared lock, the first process can get
 * its shared lock and then release the cache.
 *
 * @param fd The file descriptor that is exclusively locked and which, on
 * exit, will have a shared lock.
 */
void BESCache3::exclusive_to_shared_lock(int fd)
{
    struct flock lock;
    lock.l_type = F_RDLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;
    lock.l_pid = getpid();

    if (fcntl(fd, F_SETLKW, &lock) == -1) {
        throw BESInternalError("Could not convert an exclusive to a shared lock: " + get_errno(), __FILE__, __LINE__);
    }
}

/** Get an exclusive lock on the 'cache info' file. The 'cache info' file
 * is used to control certain cache actions, ensuring that they are atomic.
 * These include making sure that the create_and_lock() and read_and_lock()
 * operations are atomic as well as the purge and related operations.
 *
 * @note This is intended to be used internally only bt might be useful in
 * some settings.
 */
void BESCache3::lock_cache_write()
{
    BESDEBUG("cache_internal", "lock_cache - d_cache_info_fd: " << d_cache_info_fd << endl);

    if (fcntl(d_cache_info_fd, F_SETLKW, lock(F_WRLCK)) == -1) {
        throw BESInternalError("An error occurred trying to lock the cache-control file" + get_errno(), __FILE__, __LINE__);
    }
}

/** Get a shared lock on the 'cache info' file.
 *
 */
void BESCache3::lock_cache_read()
{
    BESDEBUG("cache_internal", "lock_cache - d_cache_info_fd: " << d_cache_info_fd << endl);

    if (fcntl(d_cache_info_fd, F_SETLKW, lock(F_RDLCK)) == -1) {
        throw BESInternalError("An error occurred trying to lock the cache-control file" + get_errno(), __FILE__, __LINE__);
    }
}

/** Unlock the cache info file.
 *
 * @note This is intended to be used internally only bt might be useful in
 * some settings.
 */
void BESCache3::unlock_cache()
{
    BESDEBUG("cache_internal", "BES Cache: unlock: cache_info (fd: " << d_cache_info_fd << ")" << endl);

    if (fcntl(d_cache_info_fd, F_SETLK, lock(F_UNLCK)) == -1) {
        throw BESInternalError("An error occurred trying to unlock the cache-control file" + get_errno(), __FILE__, __LINE__);
    }
}

/** Unlock the named file. This does not do any name mangling; it
 * just unlocks whatever is named (or throws BESInternalError if the file
 * cannot be closed).
 *
 * @note This method assumes that the file was opend/locked using one of
 * read_and_lock() or create_and_lock(). Those methods record the name/file-
 * descriptor pairs so that the files can be properly closed and locks
 * released.
 *
 * @param file_name The name of the file to unlock.
 * @throws BESInternalError */
void BESCache3::unlock_and_close(const string &file_name)
{
    BESDEBUG("cache_internal", "BES Cache: unlock file: " << file_name << endl);

    int fd = m_get_descriptor(file_name);	// returns -1 when no more files desp. remain
    while (fd != -1) {
    	unlock(fd);
    	fd = m_get_descriptor(file_name);
    }
}

/** Unlock the file. This does not do any name mangling; it
 * just unlocks whatever is named (or throws BESInternalError if the file
 * cannot be closed).
 * @param fd The descriptor of the file to unlock.
 * @throws BESInternalError */
void BESCache3::unlock_and_close(int fd)
{
    BESDEBUG("cache_internal", "BES Cache: unlock fd: " << fd << endl);

    unlock(fd);

    BESDEBUG("cache_internal", "BES Cache: unlock " << fd << " Success" << endl);
}

/** @brief Update the cache info file to include 'target'
 *
 * Add the size of the named file to the total cache size recorded in the
 * cache info file. The cache info file is exclusively locked by this
 * method for its duration. This updates the cache info file and returns
 * the new size.
 *
 * @param target The name of the file
 * @return The new size of the cache
 */
unsigned long long BESCache3::update_cache_info(const string &target)
{
    try
    {
        lock_cache_write();

        if (lseek(d_cache_info_fd, 0, SEEK_SET) == -1)
            throw BESInternalError("Could not rewind to front of cache info file.", __FILE__, __LINE__);

        // read the size from the cache info file
        unsigned long long current_size;
        if (read(d_cache_info_fd, &current_size, sizeof(unsigned long long)) != sizeof(unsigned long long))
            throw BESInternalError("Could not get read size info from the cache info file!", __FILE__, __LINE__);

        struct stat buf;
        int statret = stat(target.c_str(), &buf);
        if (statret == 0)
            current_size += buf.st_size;
        else
            throw BESInternalError("Could not read the size of the new file: " + target + " : " + get_errno(), __FILE__, __LINE__);

        BESDEBUG("cache_internal", "BES Cache: cache size updated to: " << current_size << endl);

        if (lseek(d_cache_info_fd, 0, SEEK_SET) == -1)
            throw BESInternalError("Could not rewind to front of cache info file.", __FILE__, __LINE__);

        if (write(d_cache_info_fd, &current_size, sizeof(unsigned long long)) != sizeof(unsigned long long))
            throw BESInternalError("Could not write size info from the cache info file!", __FILE__, __LINE__);

        unlock_cache();
        return current_size;
    }
    catch (...)
    {
        unlock_cache();
        throw;
    }

    return 0; // quite warnings
}

/** @brief look at the cache size; is it too large?
 * Look at the cache size and see if it is too big.
 *
 * @return True if the size is too big, false otherwise. */
bool BESCache3::cache_too_big(unsigned long long current_size) const
{
    return current_size > d_max_cache_size_in_bytes;
}

/** @brief Get the cache size.
 * Read the size information from the cache info file and return it.
 * This methods locks the cache.
 *
 *
 * @return The size of the cache.
 */
unsigned long long BESCache3::get_cache_size()
{
    try
    {
        lock_cache_read();

        if (lseek(d_cache_info_fd, 0, SEEK_SET) == -1)
            throw BESInternalError("Could not rewind to front of cache info file.", __FILE__, __LINE__);
        // read the size from the cache info file
        unsigned long long current_size;
        if (read(d_cache_info_fd, &current_size, sizeof(unsigned long long)) != sizeof(unsigned long long))
            throw BESInternalError("Could not get read size info from the cache info file!", __FILE__, __LINE__);

        unlock_cache();
        return current_size;
    }
    catch (...)
    {
        unlock_cache();
        throw;
    }

    return 0; //quite warnings
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
 * @param new_file The name of a file this process just added to the cache. Using
 * fcntl(2) locking there is no way this process can detect its own lock, so the
 * shared read lock on the new file won't keep this process from deleting it (but
 * will keep other processes from deleting it).
 */
void BESCache3::update_and_purge(const string &new_file)
{
    BESDEBUG("cache_purge", "purge - starting the purge" << endl);

    try {
    	lock_cache_write();

        CacheFiles contents;
        unsigned long long computed_size = m_collect_cache_dir_info(contents);

        if (BESISDEBUG( "cache_contents" )) {
            BESDEBUG( "cache_contents", endl << "BEFORE Purge " << computed_size/BYTES_PER_MEG << endl );
            CacheFiles::iterator ti = contents.begin();
            CacheFiles::iterator te = contents.end();
            for (; ti != te; ti++) {
                BESDEBUG( "cache_contents", (*ti).time << ": " << (*ti).name << ": size " << (*ti).size/BYTES_PER_MEG << endl );
            }
        }

        BESDEBUG( "cache_purge", "purge - current and target size (in MB) " << computed_size/BYTES_PER_MEG  << ", " << d_target_size/BYTES_PER_MEG << endl );

        // This deletes files and updates computed_size
        if (cache_too_big(computed_size)) {

            // d_target_size is 80% of the maximum cache size.
            // Grab the first which is the oldest in terms of access time.
            CacheFiles::iterator i = contents.begin();
            while (i != contents.end() && computed_size > d_target_size) {
                // Grab an exclusive lock but do not block - if another process has the file locked
                // just move on to the next file. Also test to see if the current file is the file
            	// this process just added to the cache - don't purge that!
                int cfile_fd;
                if (i->name != new_file && getExclusiveLockNB(i->name, cfile_fd)) {
                    BESDEBUG( "cache_purge", "purge: " << i->name << " removed." << endl );

                    if (unlink(i->name.c_str()) != 0)
                        throw BESInternalError("Unable to purge the file " + i->name + " from the cache: " + get_errno(), __FILE__, __LINE__);

                    unlock(cfile_fd);
                    computed_size -= i->size;
                }
#if 1
                else {
                    // This information is useful when debugging... Might comment out for production
                    BESDEBUG( "cache_purge", "purge: " << i->name << " is in use." << endl );
                }
#endif
                ++i;

                BESDEBUG( "cache_purge", "purge - current and target size (in MB) " << computed_size/BYTES_PER_MEG << ", " << d_target_size/BYTES_PER_MEG << endl );
            }

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

        unlock_cache();
    }
    catch(...) {
    	unlock_cache();
    	throw;
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

