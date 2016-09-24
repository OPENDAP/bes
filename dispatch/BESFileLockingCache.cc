// This file was originally part of bes, A C++ back-end server
// implementation framework for the OPeNDAP Data Access Protocol.
// Copied to libdap. This is used to cache responses built from
// functional CE expressions.

// Moved back to the BES. 6/11/13 jhrg

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

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include <string>
#include <sstream>
#include <vector>
#include <cstring>
#include <cerrno>

//#define DODS_DEBUG

#include "BESInternalError.h"

#include "BESUtil.h"
#include "BESDebug.h"
#include "BESLog.h"

#include "BESFileLockingCache.h"

using namespace std;

// conversion factor
static const unsigned long long BYTES_PER_MEG = 1048576ULL;

// Max cache size in megs, so we can check the user input and warn.
// 2^64 / 2^20 == 2^44
static const unsigned long long MAX_CACHE_SIZE_IN_MEGABYTES = (1ULL << 44);

/** @brief Protected constructor that takes as arguments keys to the cache directory,
 * file prefix, and size of the cache to be looked up a configuration file
 *
 * The keys specified are looked up in the specified keys object. If not
 * found or not set correctly then an exception is thrown. I.E., if the
 * cache directory is empty, the size is zero, or the prefix is empty.
 *
 * @param cache_dir The directory into which the cache files will be written.
 * @param prefix    The prefix that will be added to each cache file.
 * @param size      The size of the cache in MBytes
 * @throws BESInternalError If the cache_dir does not exist or is not writable.
 * size is 0, or if cache dir does not exist.
 */
BESFileLockingCache::BESFileLockingCache(const string &cache_dir, const string &prefix, unsigned long long size) :
    d_cache_dir(cache_dir), d_prefix(prefix), d_max_cache_size_in_bytes(size), d_target_size(0), d_cache_info(""),
    d_cache_info_fd(-1)
{
    m_initialize_cache_info();
}

void BESFileLockingCache::initialize(const string &cache_dir, const string &prefix, unsigned long long size)
{
    d_cache_dir = cache_dir;
    d_prefix = prefix;
    d_max_cache_size_in_bytes = size;

    m_initialize_cache_info();
}

static inline string get_errno()
{
    char *s_err = strerror(errno);
    if (s_err)
        return s_err;
    else
        return "Unknown error.";
}

// Build a lock of a certain type.
static inline struct flock *lock(int type)
{
    static struct flock lock;
    lock.l_type = type;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;
    lock.l_pid = getpid();

    return &lock;
}

inline void BESFileLockingCache::m_record_descriptor(const string &file, int fd)
{
    BESDEBUG("cache",
        "BESFileLockingCache::m_record_descriptor() - Recording descriptor: " << file << ", " << fd << endl);
    d_locks.insert(std::pair<string, int>(file, fd));
}

inline int BESFileLockingCache::m_get_descriptor(const string &file)
{
    BESDEBUG("cache", "BESFileLockingCache::m_get_descriptor(): d_locks size: " << d_locks.size() << endl);
    FilesAndLockDescriptors::iterator i = d_locks.find(file);
    if (i == d_locks.end()) return -1;

    int fd = i->second;
    BESDEBUG("cache",
        "BESFileLockingCache::m_get_descriptor(): Found file descriptor [" << fd << "] for file: " << file << endl);
    d_locks.erase(i);
    return fd;
}

string lockStatus(const int fd)
{
    struct flock isLocked, lock_query;

    isLocked.l_type = F_WRLCK; /* Test for any lock on any part of file. */
    isLocked.l_start = 0;
    isLocked.l_whence = SEEK_SET;
    isLocked.l_len = 0;
    lock_query = isLocked;

    int ret = fcntl(fd, F_GETLK, &lock_query);

    stringstream ss;
    ss << endl;
    if (ret == -1) {
        ss << "ERROR! fnctl(" << fd << ",F_GETLK, &lock) returned: " << ret << "   errno[" << errno << "]: "
            << strerror(errno) << endl;
    }
    else {
        ss << "SUCCESS. fnctl(" << fd << ",F_GETLK, &lock) returned: " << ret << endl;
    }

    ss << "lock_info.l_len:    " << lock_query.l_len << endl;
    ss << "lock_info.l_pid:    " << lock_query.l_pid << endl;
    ss << "lock_info.l_start:  " << lock_query.l_start << endl;

    string type;
    switch (lock_query.l_type) {
    case F_RDLCK:
        type = "F_RDLCK";
        break;
    case F_WRLCK:
        type = "F_WRLCK";
        break;
    case F_UNLCK:
        type = "F_UNLCK";
        break;

    }

    ss << "lock_info.l_type:   " << type << endl;
    ss << "lock_info.l_whence: " << lock_query.l_whence << endl;

    return ss.str();
}

/** Unlock and close the file descriptor.
 *
 * @param fd The file descriptor to close.
 * @throws BESBESInternalErroror if either fnctl(2) or open(2) return an error.
 */
static void unlock(int fd)
{
    if (fcntl(fd, F_SETLK, lock(F_UNLCK)) == -1) {
        throw BESInternalError("An error occurred trying to unlock the file: " + get_errno(), __FILE__, __LINE__);
    }

    BESDEBUG("cache", "BESFileLockingCache::unlock() - lock status: " << lockStatus(fd) << endl);

    if (close(fd) == -1) throw BESInternalError("Could not close the (just) unlocked file.", __FILE__, __LINE__);

    BESDEBUG("cache", "BESFileLockingCache::unlock() - File Closed. fd: " << fd << endl);
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
    BESDEBUG("cache", "getSharedLock(): Acquiring cache read lock for " << file_name <<endl);

    int fd;
    if ((fd = open(file_name.c_str(), O_RDONLY)) < 0) {
        switch (errno) {
        case ENOENT:
            return false;

        default:
            throw BESInternalError(get_errno(), __FILE__, __LINE__);
        }
    }

    struct flock *l = lock(F_RDLCK);
    if (fcntl(fd, F_SETLKW, l) == -1) {
        close(fd);
        ostringstream oss;
        oss << "cache process: " << l->l_pid << " triggered a locking error: " << get_errno();
        throw BESInternalError(oss.str(), __FILE__, __LINE__);
    }

    BESDEBUG("cache", "getSharedLock(): SUCCESS Read Lock Acquired For " << file_name <<endl);

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
bool BESFileLockingCache::getExclusiveLock(string file_name, int &ref_fd)
{
    BESDEBUG("cache", "BESFileLockingCache::getExclusiveLock() - " << file_name <<endl);

    int fd;
    if ((fd = open(file_name.c_str(), O_RDWR)) < 0) {
        switch (errno) {
        case ENOENT:
            return false;

        default:
            BESDEBUG("cache",
                "BESFileLockingCache::getExclusiveLock() -  FAILED to open file. name: " << file_name << " name_length: " << file_name.length( )<<endl);
            throw BESInternalError(get_errno(), __FILE__, __LINE__);
        }
    }

    struct flock *l = lock(F_WRLCK);
    if (fcntl(fd, F_SETLKW, l) == -1) {
        close(fd);
        ostringstream oss;
        oss << "cache process: " << l->l_pid << " triggered a locking error: " << get_errno();
        throw BESInternalError(oss.str(), __FILE__, __LINE__);
    }

    BESDEBUG("cache", "BESFileLockingCache::getExclusiveLock() -  exit: " << file_name <<endl);

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
static bool getExclusiveLockNB(string file_name, int &ref_fd)
{
    BESDEBUG("cache", "getExclusiveLock_nonblocking: " << file_name <<endl);

    int fd;
    if ((fd = open(file_name.c_str(), O_RDWR)) < 0) {
        switch (errno) {
        case ENOENT:
            return false;

        default:
            throw BESInternalError(get_errno(), __FILE__, __LINE__);
        }
    }

    struct flock *l = lock(F_WRLCK);
    if (fcntl(fd, F_SETLK, l) == -1) {
        switch (errno) {
        case EAGAIN:
            BESDEBUG("cache",
                "getExclusiveLock_nonblocking exit (false): " << file_name << " by: " << l->l_pid << endl);
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

    BESDEBUG("cache", "getExclusiveLock_nonblocking exit (true): " << file_name <<endl);

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
    BESDEBUG("cache", "createLockedFile() - filename: " << file_name <<endl);

    int fd;
    if ((fd = open(file_name.c_str(), O_CREAT | O_EXCL | O_RDWR, 0666)) < 0) {
        switch (errno) {
        case EEXIST:
            return false;

        default:
            throw BESInternalError(get_errno(), __FILE__, __LINE__);
        }
    }

    struct flock *l = lock(F_WRLCK);
    if (fcntl(fd, F_SETLKW, l) == -1) {
        close(fd);
        ostringstream oss;
        oss << "cache process: " << l->l_pid << " triggered a locking error: " << get_errno();
        throw BESInternalError(oss.str(), __FILE__, __LINE__);
    }

    BESDEBUG("cache", "createLockedFile exit: " << file_name <<endl);

    // Success
    ref_fd = fd;
    return true;
}

/** Private method */
void BESFileLockingCache::m_check_ctor_params()
{
    // TODO Should this really be a fatal error? What about just not 
    // using the cache in this case or writing out a warning message
    // to the log. jhrg 10/23/15
    if (d_cache_dir.empty()) {
        string err = "BESFileLockingCache::m_check_ctor_params() - The cache directory was not specified";
        throw BESInternalError(err, __FILE__, __LINE__);
    }

#if 0
    // This code has a Time of check, time of Use (TOCTOU) error.
    // It could be that stat returns that the directory does not
    // exist, then another process makes the directory and then this
    // code tries and fails. I think it would be better to just try
    // and if it fails, return an error only when the code indicates
    // there really is an error. 
    //
    // jhrg 10/23/15
    struct stat buf;
    int statret = stat(d_cache_dir.c_str(), &buf);
    if (statret != 0 || !S_ISDIR(buf.st_mode)) {
        // Try to make the directory
        int status = mkdir(d_cache_dir.c_str(), 0775);
        if (status != 0) {
            string err = "BESFileLockingCache::m_check_ctor_params() - The cache directory " + d_cache_dir + " does not exist or could not be created.";
            throw BESInternalError(err, __FILE__, __LINE__);
        }
    }
#endif

    // I changed these to BES_SYNTAX_USER_ERROR. jhrg 10/23/15

    int status = mkdir(d_cache_dir.c_str(), 0775);
    // If there is an error and it's not that the dir already exists,
    // throw an exception.
    if (status == -1 && errno != EEXIST) {
        string err = "The cache directory " + d_cache_dir + " could not be created: " + strerror(errno);
        throw BESError(err, BES_SYNTAX_USER_ERROR, __FILE__, __LINE__);
    }

    if (d_prefix.empty()) {
        string err = "The cache file prefix was not specified, must not be empty";
        throw BESError(err, BES_SYNTAX_USER_ERROR, __FILE__, __LINE__);
    }

    if (d_max_cache_size_in_bytes <= 0) {
        string err = "The cache size was not specified, must be greater than zero";
        throw BESError(err, BES_SYNTAX_USER_ERROR, __FILE__, __LINE__);
    }

    BESDEBUG("cache",
        "BESFileLockingCache::m_check_ctor_params() - directory " << d_cache_dir << ", prefix " << d_prefix << ", max size " << d_max_cache_size_in_bytes << endl);
}

/** Private method. */
void BESFileLockingCache::m_initialize_cache_info()
{
    BESDEBUG("cache", "BESFileLockingCache::m_initialize_cache_info() - BEGIN" << endl);

    // The value set in configuration files, etc., is the size in megabytes. The private
    // variable holds the size in bytes (converted below).
    d_max_cache_size_in_bytes = min(d_max_cache_size_in_bytes, MAX_CACHE_SIZE_IN_MEGABYTES);
    d_max_cache_size_in_bytes *= BYTES_PER_MEG;
    d_target_size = d_max_cache_size_in_bytes * 0.8;
    BESDEBUG("cache",
        "BESFileLockingCache::m_initialize_cache_info() - d_max_cache_size_in_bytes: " << d_max_cache_size_in_bytes << " d_target_size: "<<d_target_size<< endl);

    m_check_ctor_params(); // Throws BESInternalError on error.

    d_cache_info = BESUtil::assemblePath(d_cache_dir, d_prefix + ".cache_control", true);

    BESDEBUG("cache", "BESFileLockingCache::m_initialize_cache_info() - d_cache_info: " << d_cache_info << endl);

    // See if we can create it. If so, that means it doesn't exist. So make it and
    // set the cache initial size to zero.
    if (createLockedFile(d_cache_info, d_cache_info_fd)) {
        // initialize the cache size to zero
        unsigned long long size = 0;
        if (write(d_cache_info_fd, &size, sizeof(unsigned long long)) != sizeof(unsigned long long))
            throw BESInternalError("Could not write size info to the cache info file `" + d_cache_info + "`", __FILE__,
                __LINE__);

        // This leaves the d_cache_info_fd file descriptor open
        unlock_cache();
    }
    else {
        if ((d_cache_info_fd = open(d_cache_info.c_str(), O_RDWR)) == -1) {
            throw BESInternalError(get_errno(), __FILE__, __LINE__);
        }
    }

    BESDEBUG("cache", "BESFileLockingCache::m_initialize_cache_info() - d_cache_info_fd: " << d_cache_info_fd << endl);
    BESDEBUG("cache", "BESFileLockingCache::m_initialize_cache_info() - END" << endl);
}

const string chars_excluded_from_filenames = "<>=,/()\"\':? []()$";

/**
 * Returns the fully qualified file system path name for the dimension cache file
 * associated with this particular cache resource.
 *
 * @note How names are mangled: ALl occurrences of the characters
 * '<', '>', '=', ',', '/', '(', ')', '"', ''', ':', '?', and ' ' with the
 * '#' character.
 *
 * @param src The source name to cache
 * @param mangle if True, assume the name is a file pathname and mangle it.
 * If false, do not mangle the name (assume the caller has sent a suitable
 * string) but do turn the string into a pathname located in the cache directory
 * with the cache prefix. the 'mangle' param is true by default.
 */
string BESFileLockingCache::get_cache_file_name(const string &src, bool mangle)
{
    // Old way of building String, retired 10/02/2015 - ndp
    // Return d_cache_dir + "/" + d_prefix + BESFileLockingCache::DAP_CACHE_CHAR + target;
    BESDEBUG("cache", __PRETTY_FUNCTION__ << " - src: '" << src << "' mangle: "<< mangle << endl);

    string target = src;

    target = BESUtil::assemblePath(getCacheFilePrefix(), src);
    BESDEBUG("cache",  __PRETTY_FUNCTION__ << " - target: '" << target << "'" << endl);

    if (mangle) {
        if (target.at(0) == '/') {
            target = src.substr(1, target.length() - 1);
        }

        string::size_type pos = target.find_first_of(chars_excluded_from_filenames);
        while (pos != string::npos) {
            target.replace(pos, 1, "#", 1);
            pos = target.find_first_of(chars_excluded_from_filenames);
        }
    }

    BESDEBUG("cache",  __PRETTY_FUNCTION__ << " - target: '" << target << "'" << endl);

    if (target.length() > 254) {
        ostringstream msg;
        msg << "Cache filename is longer than 254 characters (name length: ";
        msg << target.length() << ", name: " << target;
        throw BESInternalError(msg.str(), __FILE__, __LINE__);
    }

    target = BESUtil::assemblePath(getCacheDirectory(), target, true);

    BESDEBUG("cache",  __PRETTY_FUNCTION__ << " - d_cache_dir: '" << d_cache_dir << "'" << endl);
    BESDEBUG("cache",  __PRETTY_FUNCTION__ << " - d_prefix:    '" << d_prefix << "'" << endl);
    BESDEBUG("cache",  __PRETTY_FUNCTION__ << " - target:      '" << target << "'" << endl);

    return target;
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
bool BESFileLockingCache::get_read_lock(const string &target, int &fd)
{
    lock_cache_read();

    bool status = getSharedLock(target, fd);

    BESDEBUG("cache2",
        "BESFileLockingCache::get_read_lock() - " << target << " (status: " << status << ", fd: " << fd << ")" << endl);

    if (status) m_record_descriptor(target, fd);

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
 * @throws BESBESInternalErroror if any error except EEXIST is returned by open(2) or
 * if fcntl(2) returns an error. */
bool BESFileLockingCache::create_and_lock(const string &target, int &fd)
{
    lock_cache_write();

    bool status = createLockedFile(target, fd);

    BESDEBUG("cache2",
        "BESFileLockingCache::create_and_lock() - " << target << " (status: " << status << ", fd: " << fd << ")" << endl);

    if (status) m_record_descriptor(target, fd);

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
void BESFileLockingCache::exclusive_to_shared_lock(int fd)
{
    struct flock lock;
    lock.l_type = F_RDLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;
    lock.l_pid = getpid();

    if (fcntl(fd, F_SETLKW, &lock) == -1) {
        throw BESInternalError(get_errno(), __FILE__, __LINE__);
    }

    BESDEBUG("cache", "BESFileLockingCache::exclusive_to_shared_lock() - lock status: " << lockStatus(fd) << endl);
}

/** Get an exclusive lock on the 'cache info' file. The 'cache info' file
 * is used to control certain cache actions, ensuring that they are atomic.
 * These include making sure that the create_and_lock() and read_and_lock()
 * operations are atomic as well as the purge and related operations.
 *
 * @note This is intended to be used internally only but might be useful in
 * some settings.
 */
void BESFileLockingCache::lock_cache_write()
{
    BESDEBUG("cache", "BESFileLockingCache::lock_cache_write() - d_cache_info_fd: " << d_cache_info_fd << endl);

    if (fcntl(d_cache_info_fd, F_SETLKW, lock(F_WRLCK)) == -1) {
        throw BESInternalError("An error occurred trying to lock the cache-control file" + get_errno(), __FILE__,
            __LINE__);
    }

    BESDEBUG("cache", "BESFileLockingCache::lock_cache_write() - lock status: " << lockStatus(d_cache_info_fd) << endl);
}

/** Get a shared lock on the 'cache info' file.
 *
 */
void BESFileLockingCache::lock_cache_read()
{
    BESDEBUG("cache", "BESFileLockingCache::lock_cache_read() - d_cache_info_fd: " << d_cache_info_fd << endl);

    if (fcntl(d_cache_info_fd, F_SETLKW, lock(F_RDLCK)) == -1) {
        throw BESInternalError("An error occurred trying to lock the cache-control file" + get_errno(), __FILE__,
            __LINE__);
    }

    BESDEBUG("cache", "BESFileLockingCache::lock_cache_read() - lock status: " << lockStatus(d_cache_info_fd) << endl);
}

/** Unlock the cache info file.
 *
 * @note This is intended to be used internally only bt might be useful in
 * some settings.
 */
void BESFileLockingCache::unlock_cache()
{
    BESDEBUG("cache", "BESFileLockingCache::unlock_cache() - d_cache_info_fd: " << d_cache_info_fd << endl);

    if (fcntl(d_cache_info_fd, F_SETLK, lock(F_UNLCK)) == -1) {
        throw BESInternalError("An error occurred trying to unlock the cache-control file" + get_errno(), __FILE__,
            __LINE__);
    }

    BESDEBUG("cache", "BESFileLockingCache::unlock_cache() - lock status: " << lockStatus(d_cache_info_fd) << endl);
}

/** Unlock the named file.
 *
 * This does not do any name mangling; it just closes and unlocks whatever
 * is named (or throws BESBESInternalErroror if the file cannot be closed).
 * If the file was opened more than once, all descriptors are closed. If you
 * need to close a specific descriptor, use the other version of
 * unlock_and_close().
 *
 * @note This method assumes that the file was opened/locked using one of
 * read_and_lock() or create_and_lock(). Those methods record the name/file-
 * descriptor pairs so that the files can be properly closed and locks
 * released.
 *
 * @param file_name The name of the file to unlock.
 * @throws BESBESInternalErroror */
void BESFileLockingCache::unlock_and_close(const string &file_name)
{
    BESDEBUG("cache2", "BESFileLockingCache::unlock_and_close() - BEGIN file: " << file_name << endl);

    int fd = m_get_descriptor(file_name);	// returns -1 when no more files desp. remain
    while (fd != -1) {
        unlock(fd);
        fd = m_get_descriptor(file_name);
    }

    BESDEBUG("cache", "BESFileLockingCache::unlock_and_close() - lock status: " << lockStatus(d_cache_info_fd) << endl);
    BESDEBUG("cache2", "BESFileLockingCache::unlock_and_close() -  END"<< endl);
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
unsigned long long BESFileLockingCache::update_cache_info(const string &target)
{
    unsigned long long current_size;
    try {
        lock_cache_write();

        if (lseek(d_cache_info_fd, 0, SEEK_SET) == -1)
            throw BESInternalError("Could not rewind to front of cache info file.", __FILE__, __LINE__);

        // read the size from the cache info file
        if (read(d_cache_info_fd, &current_size, sizeof(unsigned long long)) != sizeof(unsigned long long))
            throw BESInternalError("Could not get read size info from the cache info file!", __FILE__, __LINE__);

        struct stat buf;
        int statret = stat(target.c_str(), &buf);
        if (statret == 0)
            current_size += buf.st_size;
        else
            throw BESInternalError("Could not read the size of the new file: " + target + " : " + get_errno(), __FILE__,
                __LINE__);

        BESDEBUG("cache", "BESFileLockingCache::update_cache_info() - cache size updated to: " << current_size << endl);

        if (lseek(d_cache_info_fd, 0, SEEK_SET) == -1)
            throw BESInternalError("Could not rewind to front of cache info file.", __FILE__, __LINE__);

        if (write(d_cache_info_fd, &current_size, sizeof(unsigned long long)) != sizeof(unsigned long long))
            throw BESInternalError("Could not write size info from the cache info file!", __FILE__, __LINE__);

        unlock_cache();
    }
    catch (...) {
        unlock_cache();
        throw;
    }

    return current_size;
}

/** @brief look at the cache size; is it too large?
 * Look at the cache size and see if it is too big.
 *
 * @return True if the size is too big, false otherwise. */
bool BESFileLockingCache::cache_too_big(unsigned long long current_size) const
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
unsigned long long BESFileLockingCache::get_cache_size()
{
    unsigned long long current_size;
    try {
        lock_cache_read();

        if (lseek(d_cache_info_fd, 0, SEEK_SET) == -1)
            throw BESInternalError("Could not rewind to front of cache info file.", __FILE__, __LINE__);
        // read the size from the cache info file
        if (read(d_cache_info_fd, &current_size, sizeof(unsigned long long)) != sizeof(unsigned long long))
            throw BESInternalError("Could not get read size info from the cache info file!", __FILE__, __LINE__);

        unlock_cache();
    }
    catch (...) {
        unlock_cache();
        throw;
    }

    return current_size;
}

static bool entry_op(cache_entry &e1, cache_entry &e2)
{
    return e1.time < e2.time;
}

/** Private. Get info about all of the files (size and last use time). */
unsigned long long BESFileLockingCache::m_collect_cache_dir_info(CacheFiles &contents)
{
    DIR *dip = opendir(d_cache_dir.c_str());
    if (!dip) throw BESInternalError("Unable to open cache directory " + d_cache_dir, __FILE__, __LINE__);

    struct dirent *dit;
    vector<string> files;
    // go through the cache directory and collect all of the files that
    // start with the matching prefix
    while ((dit = readdir(dip)) != NULL) {
        string dirEntry = dit->d_name;
        if (dirEntry.compare(0, d_prefix.length(), d_prefix) == 0 && dirEntry != d_cache_info) {
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
void BESFileLockingCache::update_and_purge(const string &new_file)
{
    BESDEBUG("cache", "purge - starting the purge" << endl);

    try {
        lock_cache_write();

        CacheFiles contents;
        unsigned long long computed_size = m_collect_cache_dir_info(contents);
#if 0
        if (BESISDEBUG( "cache_contents" )) {
            BESDEBUG("cache", "BEFORE Purge " << computed_size/BYTES_PER_MEG << endl );
            CacheFiles::iterator ti = contents.begin();
            CacheFiles::iterator te = contents.end();
            for (; ti != te; ti++) {
                BESDEBUG("cache", (*ti).time << ": " << (*ti).name << ": size " << (*ti).size/BYTES_PER_MEG << endl );
            }
        }
#endif
        BESDEBUG("cache",
            "BESFileLockingCache::update_and_purge() - current and target size (in MB) " << computed_size/BYTES_PER_MEG << ", " << d_target_size/BYTES_PER_MEG << endl);

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
                    BESDEBUG("cache", "purge: " << i->name << " removed." << endl);

                    if (unlink(i->name.c_str()) != 0)
                        throw BESInternalError(
                            "Unable to purge the file " + i->name + " from the cache: " + get_errno(), __FILE__,
                            __LINE__);

                    unlock(cfile_fd);
                    computed_size -= i->size;
                }
                ++i;

                BESDEBUG("cache",
                    "BESFileLockingCache::update_and_purge() - current and target size (in MB) " << computed_size/BYTES_PER_MEG << ", " << d_target_size/BYTES_PER_MEG << endl);
            }
        }

        if (lseek(d_cache_info_fd, 0, SEEK_SET) == -1)
            throw BESInternalError("Could not rewind to front of cache info file.", __FILE__, __LINE__);

        if (write(d_cache_info_fd, &computed_size, sizeof(unsigned long long)) != sizeof(unsigned long long))
            throw BESInternalError("Could not write size info to the cache info file!", __FILE__, __LINE__);
#if 0
        if (BESISDEBUG( "cache_contents" )) {
            contents.clear();
            computed_size = m_collect_cache_dir_info(contents);
            BESDEBUG("cache", "AFTER Purge " << computed_size/BYTES_PER_MEG << endl );
            CacheFiles::iterator ti = contents.begin();
            CacheFiles::iterator te = contents.end();
            for (; ti != te; ti++) {
                BESDEBUG("cache", (*ti).time << ": " << (*ti).name << ": size " << (*ti).size/BYTES_PER_MEG << endl );
            }
        }
#endif
        unlock_cache();
    }
    catch (...) {
        unlock_cache();
        throw;
    }
}

/** @brief Purge a single file from the cache
 *
 * Purge a single file from the cache. The file might be old, etc., and need to
 * be removed. Don't use this to shrink the cache when it gets too big, use
 * update_and_purge() instead since that file optimizes accesses to the cache
 * control file for several changes in a row.
 *
 * @todo This is a new feature; add to BESCache3
 *
 * @param file The name of the file to purge.
 */
void BESFileLockingCache::purge_file(const string &file)
{
    BESDEBUG("cache", "BESFileLockingCache::purge_file() - starting the purge" << endl);

    try {
        lock_cache_write();

        // Grab an exclusive lock on the file
        int cfile_fd;
        if (getExclusiveLock(file, cfile_fd)) {
            // Get the file's size
            unsigned long long size = 0;
            struct stat buf;
            if (stat(file.c_str(), &buf) == 0) {
                size = buf.st_size;
            }

            BESDEBUG("cache", "BESFileLockingCache::purge_file() - " << file << " removed." << endl);

            if (unlink(file.c_str()) != 0)
                throw BESInternalError("Unable to purge the file " + file + " from the cache: " + get_errno(), __FILE__,
                    __LINE__);

            unlock(cfile_fd);

            unsigned long long cache_size = get_cache_size() - size;

            if (lseek(d_cache_info_fd, 0, SEEK_SET) == -1)
                throw BESInternalError("Could not rewind to front of cache info file.", __FILE__, __LINE__);

            if (write(d_cache_info_fd, &cache_size, sizeof(unsigned long long)) != sizeof(unsigned long long))
                throw BESInternalError("Could not write size info to the cache info file!", __FILE__, __LINE__);
        }

        unlock_cache();
    }
    catch (...) {
        unlock_cache();
        throw;
    }
}

const string BESFileLockingCache::getCacheFilePrefix()
{
    return d_prefix;
}

const string BESFileLockingCache::getCacheDirectory()
{
    return d_cache_dir;
}

/**
 * Does the directory exist?
 *
 * @param dir The pathname to test.
 * @return True if the directory exists, false otherwise
 */
bool BESFileLockingCache::dir_exists(const string &dir)
{
    struct stat buf;

    return (stat(dir.c_str(), &buf) == 0) && (buf.st_mode & S_IFDIR);
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance along with information about
 * this cache.
 *
 * @param strm C++ i/o stream to dump the information to
 */
void BESFileLockingCache::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << "BESFileLockingCache::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();
    strm << BESIndent::LMarg << "cache dir: " << d_cache_dir << endl;
    strm << BESIndent::LMarg << "prefix: " << d_prefix << endl;
    strm << BESIndent::LMarg << "size (bytes): " << d_max_cache_size_in_bytes << endl;
    BESIndent::UnIndent();
}
