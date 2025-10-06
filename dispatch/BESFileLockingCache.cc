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

#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <string>
#include <sstream>
#include <vector>

#include <cstring>
#include <cerrno>

#include "BESInternalError.h"

#include "BESUtil.h"
#include "BESDebug.h"
#include "BESLog.h"

#include "BESFileLockingCache.h"

// Symbols used with BESDEBUG.
#define CACHE "cache"
#define LOCK "cache-lock"
#define LOCK_STATUS "cache-lock-status"

#define CACHE_CONTROL "cache_control"

#define prolog std::string("BESFileLockingCache::").append(__func__).append("() - ")

using namespace std;

// conversion factor
static const unsigned long long BYTES_PER_MEG = 1048576ULL;

// Max cache size in megs, so we can check the user input and warn.
// 2^64 / 2^20 == 2^44
static const unsigned long long MAX_CACHE_SIZE_IN_MEGABYTES = (1ULL << 44);

static inline string get_errno() {
    const char *s_err = strerror(errno);
    return s_err ? s_err : "unknown error";
}

// Build a lock of a certain type.
//
// Using whence == SEEK_SET with start and len set to zero means lock the whole file.
// jhrg 9/8/18
static inline struct flock *advisory_lock(int type)
{
    static struct flock lock;
    lock.l_type = type;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;
    lock.l_pid = getpid();

    return &lock;
}

/// Move this to the header if it's needed elsewhere or scope needs to be contained. jhrg 6/29/23
class AdvisoryLockGuard {
    int d_fd; /// the descriptor to lock

public:
    AdvisoryLockGuard() = delete;
    AdvisoryLockGuard(const AdvisoryLockGuard &) = delete;
    AdvisoryLockGuard &operator=(const AdvisoryLockGuard &) = delete;

    /// Get an advisory file lock on a file. Use F_RDLCK or F_WRLCK for 'type.'
    AdvisoryLockGuard(int fd, int type) : d_fd(fd) {
        struct flock *l = advisory_lock(type);
        if (fcntl(d_fd, F_SETLKW, l) == -1) {
            ERROR_LOG("Could not lock the advisory file lock (fcntl: " + get_errno() + ").");
        }
    }

    ~AdvisoryLockGuard() {
        struct flock *l = advisory_lock(F_UNLCK);
        if (fcntl(d_fd, F_SETLKW, l) == -1) {
            ERROR_LOG("Could not unlock the advisory file lock (fcntl: " + get_errno() + ").");
        }
    }
};

/** @brief Make an instance of FileLockingCache
 *
 * Instantiate the FileLockingClass, using the given values for the cache
 * directory, item prefix and size. If the cache directory is the empty string,
 * do not initialize the cache, but instead mark the cache as 'disabled.' Otherwise,
 * attempt to build the FileLockingCache object and throw an exception if the
 * directory, ..., values are not valid.
 *
 * @note A side effect of this is that the cache_enabled() property will be set
 * to true or false based on whether the cache should be used or note. This ensures
 * that the evaluation of the various parameters (and the BESKeys that are likely
 * used by specializations) will happen only once when the cache is disabled.
 *
 * @param cache_dir The directory into which the cache files will be written.
 * @param prefix    The prefix that will be added to each cache file.
 * @param size      The size of the cache in MBytes
 *
 * @throws BESInternalError If the cache_dir does not exist or is not writable.
 * size is 0, or if cache dir does not exist.
 * @throws BESError If the parameters (directory, ...) are invalid.
 */
BESFileLockingCache::BESFileLockingCache(string cache_dir, string prefix, unsigned long long size) :
    d_cache_dir(std::move(cache_dir)), d_prefix(std::move(prefix)), d_max_cache_size_in_bytes(size)
{
    m_initialize_cache_info();
}

/**
 * A blocking call to create a file locked for write.
 *
 * @note Used by the methods m_initialize_cache_info() and create_and_lock()
 *
 * @param file_name The name of the file to lock
 * @param ref_fd Return-value parameter that holds the file descriptor that's locked.
 * @return True when the lock is acquired, false if the file already exists.
 */
static bool createLockedFile(const string &file_name, int &ref_fd)
{
    BESDEBUG(LOCK,  prolog << "BEGIN file: " << file_name <<endl);

    int fd;
    if ((fd = open(file_name.c_str(), O_CREAT | O_EXCL | O_RDWR, 0666)) < 0) {
        switch (errno) {
            case EEXIST:
                return false;

            default:
                throw BESInternalError(file_name + ": " + get_errno(), __FILE__, __LINE__);
        }
    }

    struct flock *l = advisory_lock(F_WRLCK);
    // F_SETLKW == set lock, blocking
    if (fcntl(fd, F_SETLKW, l) == -1) {
        close(fd);
        ostringstream oss;
        oss << "cache process: " << l->l_pid << " triggered a locking error for '" << file_name << "': " << get_errno();
        throw BESInternalError(oss.str(), __FILE__, __LINE__);
    }

    BESDEBUG(LOCK,  prolog << "END file: " << file_name <<endl);

    // Success
    ref_fd = fd;
    return true;
}

/**
 * Initialize FileLockingCache
 *
 * @return True if the cache is enabled, False if not. Throws exceptions on error.
 * @exception BESError thrown if the cache directory, item prefix or max size are
 * invalid.
 * @exception BESInternalError thrown of the cache info control file cannot be created
 * or opened.
 */
bool BESFileLockingCache::m_initialize_cache_info()
{
    BESDEBUG(CACHE,  prolog << "BEGIN" << endl);

    // The value set in configuration files, etc., is the size in megabytes. The private
    // variable holds the size in bytes (converted below).
    d_max_cache_size_in_bytes = min(d_max_cache_size_in_bytes, MAX_CACHE_SIZE_IN_MEGABYTES);
    d_max_cache_size_in_bytes *= BYTES_PER_MEG;
    d_target_size = d_max_cache_size_in_bytes * 0.8;

    BESDEBUG(CACHE, prolog << "d_max_cache_size_in_bytes: "
                           << d_max_cache_size_in_bytes << " d_target_size: "<<d_target_size<< endl);

    bool status = m_check_ctor_params(); // Throws BESError on error; otherwise sets the cache_enabled() property
    if (status) {
        d_cache_info = BESUtil::assemblePath(d_cache_dir, d_prefix + CACHE_CONTROL, true);

        BESDEBUG(CACHE, prolog << "d_cache_info: " << d_cache_info << endl);

        // See if we can create it. If so, that means it doesn't exist. So make it and
        // set the cache initial size to zero.
        if (createLockedFile(d_cache_info, d_cache_info_fd)) {
            // initialize the cache size to zero
            unsigned long long size = 0;
            if (write(d_cache_info_fd, &size, sizeof(unsigned long long)) != sizeof(unsigned long long))
                throw BESInternalError(prolog + "Could not write size info to the cache info file `" + d_cache_info + "`",
                                       __FILE__,
                                       __LINE__);

            // This leaves the d_cache_info_fd file descriptor open
#if 0
            unlock_cache();
#endif
            if (fcntl(d_cache_info_fd, F_SETLK, advisory_lock(F_UNLCK)) == -1) {
                throw BESInternalError(prolog +"An error occurred trying to unlock the cache-control file" + get_errno(), __FILE__,
                                       __LINE__);
            }
        }
        else {
            if ((d_cache_info_fd = open(d_cache_info.c_str(), O_RDWR)) == -1) {
                throw BESInternalError(prolog + "Failed to open cache info file: " + d_cache_info + " errno: " + get_errno(), __FILE__, __LINE__);
            }
        }

        BESDEBUG(CACHE, prolog << "d_cache_info_fd: " << d_cache_info_fd << endl);
    }

    BESDEBUG(CACHE, prolog << "END [" << "CACHE IS " << (cache_enabled()?"ENABLED]":"DISABLED]") << endl);

    return status;
}

/** @brief Initialize an instance of FileLockingCache
 *
 * Initialize and instance of FileLockingCache using the passed values for the
 * cache directory, item prefix and max cache size. This will ignore the value
 * of enable_cache() (but will correctly (re)set it based on the directory, ...,
 * values). This provides a way for clients to re-initialize caches on the fly.
 *
 * @param cache_dir The directory into which the cache files will be written.
 * @param prefix The prefix that will be added to each cache file.
 * @param size The size of the cache in MBytes
 *
 * @throws BESInternalError If the cache_dir does not exist or is not writable.
 * size is 0, or if cache dir does not exist.
 * @throws BESError If the parameters (directory, ...) are invalid.
 */
void BESFileLockingCache::initialize(const string &cache_dir, const string &prefix, unsigned long long size)
{
    d_cache_dir = cache_dir;
    d_prefix = prefix;
    d_max_cache_size_in_bytes = size; // converted later on to bytes

    m_initialize_cache_info();
}



inline void BESFileLockingCache::m_record_descriptor(const string &file, int fd)
{
    BESDEBUG(LOCK, prolog << "Recording descriptor: " << file << ", " << fd << endl);

    d_locks.insert(std::pair<string, int>(file, fd));
}

inline int BESFileLockingCache::m_remove_descriptor(const string &file)
{
    BESDEBUG(LOCK, prolog << "d_locks size: " << d_locks.size() << endl);

    FilesAndLockDescriptors::iterator i = d_locks.find(file);
    if (i == d_locks.end()) return -1;

    int fd = i->second;
    d_locks.erase(i);

    BESDEBUG(LOCK, prolog << "Found file descriptor [" << fd << "] for file: " << file << endl);

    return fd;
}

#if USE_GET_SHARED_LOCK
inline int BESFileLockingCache::m_find_descriptor(const string &file)
{
    BESDEBUG(LOCK, prolog << "d_locks size: " << d_locks.size() << endl);

    FilesAndLockDescriptors::iterator i = d_locks.find(file);
    if (i == d_locks.end()) return -1;

    BESDEBUG(LOCK, prolog << "Found file descriptor [" << i->second << "] for file: " << file << endl);

    return i->second;   // return the file descriptor bound to 'file'
}
#endif

/**
 * @brief Used in BESDEBUG() statements
 * @param fd An open file descriptor
 * @return A string with information about its status WRT advisory locking.
 */
static string lockStatus(const int fd)
{
    struct flock lock_query;

    lock_query.l_type = F_WRLCK; /* Test for any lock on any part of a file. */
    lock_query.l_start = 0;
    lock_query.l_whence = SEEK_SET;
    lock_query.l_len = 0;
    lock_query.l_pid = 0;

    int ret = fcntl(fd, F_GETLK, &lock_query);

    stringstream ss;
    ss << endl;
    if (ret == -1) {
        ss << "fnctl(" << fd << ",F_GETLK, &lock) returned: " << ret << "   errno[" << errno << "]: "
            << strerror(errno) << endl;
    }
    else {
        ss << "fnctl(" << fd << ",F_GETLK, &lock) returned: " << ret << endl;
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
 * @throws BESBESInternalError if either fnctl(2) or open(2) return an error.
 */
static void unlock(int fd)
{
    if (fcntl(fd, F_SETLK, advisory_lock(F_UNLCK)) == -1) {
        throw BESInternalError(prolog + "An error occurred trying to unlock the file: " + get_errno(), __FILE__, __LINE__);
    }

    BESDEBUG(LOCK_STATUS,  prolog << "lock status: " << lockStatus(fd) << endl);

    if (close(fd) == -1) throw BESInternalError(prolog + "Could not close the (just) unlocked file.", __FILE__, __LINE__);

    BESDEBUG(LOCK,  prolog << "File Closed. fd: " << fd << endl);
}

/**
 * Check the values of the three main parameters to the cache: The
 * cache's directory, item prefix and maximum size. If the directory
 * value is the empty string, this indicates the cache should not be
 * used - return false. Otherwise, evaluate the parameters and return
 * true if they are valid, or thrown a BESError exception.
 *
 * @note If the directory is named, but does not exist, this method
 * will create it. If it cannot (for any reason, e.g., lack of privilege)
 * then an error is indicated by an exception.
 *
 * @note Sets the cache_enabled property as a side effect.
 *
 * @return True if the parameters are valid and the cache directory exists,
 * false if the cache directory does not exist (the prefix and size are
 * then ignored) and thus the cache object should not be created.
 *
 * @exception BESError
 */
bool BESFileLockingCache::m_check_ctor_params()
{
    // Should this really be a fatal error? What about just not
    // using the cache in this case or writing out a warning message
    // to the log. jhrg 10/23/15
    //
    // Yes, leave this as a fatal error and handle the case when cache_dir is
    // empty in code that specializes this class. Those child classes are
    // all singletons and their get_instance() methods need to return null
    // when caching is turned off. You cannot do that here without throwing
    // and we don't want to throw an exception for every call to a child's
    // get_instance() method  just because someone doesn't want to use a cache.
    // jhrg 9/27/16
    BESDEBUG(CACHE,  prolog << "BEGIN" << endl);

    if (d_cache_dir.empty()) {
        BESDEBUG(CACHE,  prolog << "The cache directory was not specified. CACHE IS DISABLED." << endl);

        disable();
        return false;
    }


    int status = mkdir(d_cache_dir.c_str(), 0775);
    // If there is an error and it's not that the dir already exists,
    // throw an exception.
    if (status == -1 && errno != EEXIST) {
        string err = prolog + "The cache directory " + d_cache_dir + " could not be created: " + strerror(errno);
        throw BESError(err, BES_SYNTAX_USER_ERROR, __FILE__, __LINE__);
    }

    if (d_prefix.empty()) {
        string err = prolog + "The cache file prefix was not specified, must not be empty";
        throw BESError(err, BES_SYNTAX_USER_ERROR, __FILE__, __LINE__);
    }

    // I changed this from '<=' to '<' since the code now uses a cache size
    // of zero to indicate that the cache will never be purged. The other
    // size-related methods all still work. Since the field is unsigned,
    // testing for '< 0' is pointless. Later on in this code the value is capped
    // at MAX_CACHE_SIZE_IN_MEGABYTES (set in this file), which is 2^44.
    // jhrg 2.28.18
#if 0
    if (d_max_cache_size_in_bytes < 0) {
        string err = "The cache size was not specified, must be greater than zero";
        throw BESError(err, BES_SYNTAX_USER_ERROR, __FILE__, __LINE__);
    }
#endif

    BESDEBUG(CACHE,
        "BESFileLockingCache::" << __func__ << "() -" <<
        " d_cache_dir: " << d_cache_dir <<
        " d_prefix: " << d_prefix <<
        " d_max_cache_size_in_bytes: " << d_max_cache_size_in_bytes << endl);

    enable();
    return true;
}


static const string chars_excluded_from_filenames = R"(<>=,/()\"':? []()$)";

/**
 * Returns the fully qualified file system path name for the cache file
 * associated with this particular cache resource. This does not look
 * in the cache to see if the fle is present, it just returns the name
 * that will (or is) be used once/if the file is added to the cache.
 *
 * @note Names are mangled: ALl occurrences of the characters
 * '<', '>', '=', ',', '/', '(', ')', '"', ''', ':', '?', and ' '
 * are replaced with the '#' character.
 *
 * @param src The source name to (or in the) cache
 * @param mangle If True, assume the name is a file pathname and mangle it.
 * If false, do not mangle the name (assume the caller has sent a suitable
 * string) but do turn the string into a pathname located in the cache directory
 * with the cache prefix. The 'mangle' param is true by default.
 */
string BESFileLockingCache::get_cache_file_name(const string &src, bool mangle)
{
    // Old way of building String, retired 10/02/2015 - ndp
    // Return d_cache_dir + "/" + d_prefix + BESFileLockingCache::DAP_CACHE_CHAR + target;
    BESDEBUG(CACHE, prolog << "src: '" << src << "' mangle: "<< mangle << endl);

    string target = get_cache_file_prefix() + src;

    if (mangle) {
        string::size_type pos = target.find_first_of(chars_excluded_from_filenames);
        while (pos != string::npos) {
            target.replace(pos, 1, "#", 1);
            pos = target.find_first_of(chars_excluded_from_filenames);
        }
    }

    if (target.size() > 254) {
        ostringstream msg;
        msg << prolog << "Cache filename is longer than 254 characters (name length: ";
        msg << target.size() << ", name: " << target;
        throw BESInternalError(msg.str(), __FILE__, __LINE__);
    }

    target = BESUtil::assemblePath(get_cache_directory(), target, true);

    BESDEBUG(CACHE,  prolog << "target:      '" << target << "'" << endl);

    return target;
}

#if USE_GET_SHARED_LOCK
/** Get a shared read lock on an existing file.

 @param file_name The name of the file.
 @param ref_fp if successful, the file descriptor of the file on which we
 have a shared read lock. This is used to release the lock.

 @return If the file does not exist, return immediately indicating
 failure (false), otherwise block until a shared read-lock can be
 obtained and then return true.

 @exception BESInternalError Thrown to indicate a number of untoward
 events.
 */
static bool getSharedLock(const string &file_name, int &ref_fd)
{
    BESDEBUG(LOCK, prolog << "Acquiring cache read lock for " << file_name <<endl);

    int fd;
    if ((fd = open(file_name.c_str(), O_RDONLY)) < 0) {
        switch (errno) {
        case ENOENT:
            return false;

        default:
            throw BESInternalError(get_errno(), __FILE__, __LINE__);
        }
    }

    struct flock *l = advisory_lock(F_RDLCK);
    if (fcntl(fd, F_SETLKW, l) == -1) {
        close(fd);
        ostringstream oss;
        oss << prolog << "cache process: " << l->l_pid << " triggered a locking error for '" << file_name << "': " << get_errno();
        throw BESInternalError(oss.str(), __FILE__, __LINE__);
    }

    BESDEBUG(LOCK, prolog << "SUCCESS Read Lock Acquired For " << file_name <<endl);

    // Success
    ref_fd = fd;
    return true;
}
#endif

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
 * @param target The path of the cached file
 * @param fd A value-result parameter set to the locked cached file. Undefined
 * if the file could not be locked for read access.
 * @return true if the file is in the cache and has been locked, false if
 * the file is/was not in the cache.
 * @throws Error if the attempt to get the (shared) lock failed for any
 * reason other than that the file does/did not exist.
 */
bool BESFileLockingCache::get_read_lock(const string &target, int &fd)
{
    AdvisoryLockGuard read_alg(d_cache_info_fd, F_RDLCK);
#if 0
    lock_cache_read();
#endif

    bool status = true;

#if USE_GET_SHARED_LOCK
    status = getSharedLock(target, fd);

    if (status) m_record_descriptor(target, fd);
#else
    fd = m_find_descriptor(target);
    // fd == -1 --> The file is not currently open
    if ((fd == -1) && (fd = open(target.c_str(), O_RDONLY)) < 0) {
        switch (errno) {
        case ENOENT:
            return false;   // The file does not exist

        default:
            throw BESInternalError(get_errno(), __FILE__, __LINE__);
        }
    }

    // The file might be open for writing, so setting a read lock is
    // not possible.
    struct flock *l = advisory_lock(F_RDLCK);
    if (fcntl(fd, F_SETLKW, l) == -1) {
        return false;   // cannot get the lock
    }

    m_record_descriptor(target, fd);
#endif

#if 0
    unlock_cache();
#endif

    return status;
}

/**
 * @brief Create a file in the cache and lock it for write access.
 *
 * If the file does not exist, make it, open it for read-write access and
 * get an exclusive lock on it. The locking operation blocks, although that
 * should never happen.
 *
 * @param target The name of the file to make/open/lock
 * @param fd Value-result param that holds the file descriptor of the opened
 * file
 *
 * @return True if the operation was successful, false otherwise. This method will
 * return false if the file already existed (the file won't be locked and the
 * descriptor reference is undefined - but likely -1).
 *
 * @throws BESBESInternalError if any error except EEXIST is returned by open(2) or
 * if fcntl(2) returns an error. */
bool BESFileLockingCache::create_and_lock(const string &target, int &fd)
{
    AdvisoryLockGuard write_alg(d_cache_info_fd, F_WRLCK);
#if 0
    lock_cache_write();
#endif

    bool status = createLockedFile(target, fd);

    BESDEBUG(LOCK, prolog << "target: " << target << " (status: " << status << ", fd: " << fd << ")" << endl);

    if (status) m_record_descriptor(target, fd);

#if 0
    unlock_cache();
#endif

    return status;
}

/**
 * @brief Transfer from an exclusive lock to a shared lock.
 *
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

    BESDEBUG(LOCK_STATUS, prolog << "lock status: " << lockStatus(fd) << endl);
}

/** Get an exclusive lock on the 'cache info' file. The 'cache info' file
 * is used to control certain cache actions, ensuring that they are atomic.
 * These include making sure that the create_and_lock() and read_and_lock()
 * operations are atomic as well as the purge and related operations.
 *
 * @note This is intended to be used internally only but might be useful in
 * some settings.
 */
#if 0
void BESFileLockingCache::lock_cache_write()
{
    BESDEBUG(LOCK, prolog << "d_cache_info_fd: " << d_cache_info_fd << endl);

    if (fcntl(d_cache_info_fd, F_SETLKW, advisory_lock(F_WRLCK)) == -1) {
        throw BESInternalError("An error occurred trying to lock the cache-control file" + get_errno(), __FILE__,
            __LINE__);
    }

    BESDEBUG(LOCK_STATUS,  prolog << "lock status: " << lockStatus(d_cache_info_fd) << endl);
}

/** Get a shared lock on the 'cache info' file.
 *
 */
void BESFileLockingCache::lock_cache_read()
{
    BESDEBUG(LOCK,  prolog << "d_cache_info_fd: " << d_cache_info_fd << endl);

    if (fcntl(d_cache_info_fd, F_SETLKW, advisory_lock(F_RDLCK)) == -1) {
        throw BESInternalError(prolog + "An error occurred trying to lock the cache-control file" + get_errno(), __FILE__,
            __LINE__);
    }

    BESDEBUG(LOCK_STATUS,  prolog << "lock status: " << lockStatus(d_cache_info_fd) << endl);
}

/** Unlock the cache info file.
 *
 * @note This is intended to be used internally only but might be useful in
 * some settings. Made private 6/29/23 jhrg
 */
void BESFileLockingCache::unlock_cache()
{
    BESDEBUG(LOCK,  prolog << "d_cache_info_fd: " << d_cache_info_fd << endl);

    if (fcntl(d_cache_info_fd, F_SETLK, advisory_lock(F_UNLCK)) == -1) {
        throw BESInternalError(prolog +"An error occurred trying to unlock the cache-control file" + get_errno(), __FILE__,
            __LINE__);
    }

    BESDEBUG(LOCK_STATUS,  prolog << "lock status: " << lockStatus(d_cache_info_fd) << endl);
}
#endif

/** Unlock the named file.
 *
 * This does not do any name mangling; it just closes and unlocks whatever
 * is named (or throws BESBESInternalError if the file cannot be closed).
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
 * @throws BESBESInternalError */
void BESFileLockingCache::unlock_and_close(const string &file_name)
{
    BESDEBUG(LOCK,  prolog << "BEGIN file: " << file_name << endl);

    int fd = m_remove_descriptor(file_name);	// returns -1 when no more files desp. remain
    while (fd != -1) {
        unlock(fd);
        fd = m_remove_descriptor(file_name);
    }

    BESDEBUG(LOCK_STATUS,  prolog << "lock status: " << lockStatus(d_cache_info_fd) << endl);
    BESDEBUG(LOCK,  prolog << "END file: "<< file_name<< endl);
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
    AdvisoryLockGuard write_alg(d_cache_info_fd, F_WRLCK);
    unsigned long long current_size;
#if 0
    try {
        lock_cache_write();
#endif

        if (lseek(d_cache_info_fd, 0, SEEK_SET) == -1)
            throw BESInternalError(prolog + "Could not rewind to front of cache info file.", __FILE__, __LINE__);

        // read the size from the cache info file
        if (read(d_cache_info_fd, &current_size, sizeof(unsigned long long)) != sizeof(unsigned long long))
            throw BESInternalError(prolog + "Could not get read size info from the cache info file!", __FILE__, __LINE__);

        struct stat buf;
        int statret = stat(target.c_str(), &buf);
        if (statret == 0)
            current_size += buf.st_size;
        else
            throw BESInternalError(prolog + "Could not read the size of the new file: " + target + " : " + get_errno(), __FILE__,
                __LINE__);

        BESDEBUG(CACHE,  prolog << "cache size updated to: " << current_size << endl);

        if (lseek(d_cache_info_fd, 0, SEEK_SET) == -1)
            throw BESInternalError(prolog + "Could not rewind to front of cache info file.", __FILE__, __LINE__);

        if (write(d_cache_info_fd, &current_size, sizeof(unsigned long long)) != sizeof(unsigned long long))
            throw BESInternalError(prolog + "Could not write size info from the cache info file!", __FILE__, __LINE__);

#if 0
    unlock_cache();
    }
    catch (...) {
        unlock_cache();
        throw;
    }
#endif

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
 *
 * Read the size information from the cache info file and return it.
 * This methods locks the cache.
 *
 * @return The size of the cache.
 */
unsigned long long BESFileLockingCache::get_cache_size()
{
    AdvisoryLockGuard read_alg(d_cache_info_fd, F_RDLCK);
    unsigned long long current_size;
#if 0
    try {
        lock_cache_read();
#endif

        if (lseek(d_cache_info_fd, 0, SEEK_SET) == -1)
            throw BESInternalError(prolog + "Could not rewind to front of cache info file.", __FILE__, __LINE__);
        // read the size from the cache info file
        if (read(d_cache_info_fd, &current_size, sizeof(unsigned long long)) != sizeof(unsigned long long))
            throw BESInternalError(prolog + "Could not get read size info from the cache info file!", __FILE__, __LINE__);

#if 0
    unlock_cache();
    }
    catch (...) {
        unlock_cache();
        throw;
    }
#endif

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
    if (!dip) throw BESInternalError(prolog + "Unable to open cache directory " + d_cache_dir, __FILE__, __LINE__);

    struct dirent *dit = nullptr;
    vector<string> files;
    // go through the cache directory and collect all the files that
    // start with the matching prefix
    while ((dit = readdir(dip)) != NULL) {
        string dirEntry = dit->d_name;
        if (dirEntry.compare(0, d_prefix.size(), d_prefix) == 0 && dirEntry != d_cache_info) {
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

/**
 * A non-blocking call to get an exclusive (write) lock on a file in the cache.
 * Because this cache uses per-process advisory locking, it's possible to
 * call this several times *from within a single process* and get a lock
 * each time.
 *
 * @note This is used only by the update_and_purge() method.
 *
 * @param file_name Name of the file to lock
 * @param ref_fd Return value parameter that holds the file descriptor that
 * is locked.
 * @return Return false if the file does not exist or the non-blocking lock
 * acquisition fails, otherwise the lock is acquired and then return true.
 * @exception BESInternalError is thrown on any condition other than the file
 * not existing or the lock being unavailable.
 */
bool BESFileLockingCache::get_exclusive_lock_nb(const string &file_name, int &ref_fd)
{
    BESDEBUG(LOCK, prolog << "BEGIN filename: " << file_name <<endl);

    int fd;
    if ((fd = open(file_name.c_str(), O_RDWR)) < 0) {
        switch (errno) {
            case ENOENT:
                return false;

            default:
                throw BESInternalError(prolog + "errno: " + get_errno(), __FILE__, __LINE__);
        }
    }

    struct flock *l = advisory_lock(F_WRLCK);
    if (fcntl(fd, F_SETLK, l) == -1) {
        switch (errno) {
            case EAGAIN:
            case EACCES:
                BESDEBUG(LOCK,prolog << "exit (false): " << file_name << " by: " << l->l_pid << endl);
                close(fd);
                return false;

            default: {
                close(fd);
                ostringstream oss;
                oss << prolog << "cache process: " << l->l_pid << " triggered a locking error for '" << file_name << "': " << get_errno();
                throw BESInternalError(oss.str(), __FILE__, __LINE__);
            }
        }
    }


    // Success
    m_record_descriptor(file_name,fd);
    ref_fd = fd;
    BESDEBUG(LOCK, prolog << "END filename: " << file_name <<endl);
    return true;
}


/** @brief Purge files from the cache
 *
 * Purge files, oldest to newest, if the current size of the cache exceeds the
 * size of the cache specified in the constructor. This method uses an exclusive
 * lock on the cache for the duration of the purge process.
 *
 * @note If the cache size in bytes is zero, calling this method has no affect
 * (the cache is unlimited in size). Other public methods like update_cache_info()
 * and get_cache_size() still work, however.
 *
 * @param new_file Do not delete this file. The name of a file this process just
 * added to the cache. Using fcntl(2) locking there is no way this process can
 * detect its own lock, so the shared read lock on the new file won't keep this
 * process from deleting it (but will keep other processes from deleting it).
 */
void BESFileLockingCache::update_and_purge(const string &new_file)
{
    BESDEBUG(CACHE, prolog << "Starting the purge" << endl);

    if (is_unlimited()) {
        BESDEBUG(CACHE, prolog << "Size is set to unlimited, so no need to purge." << endl);
        return;
    }

    AdvisoryLockGuard write_alg(d_cache_info_fd, F_WRLCK);
#if 0
    try {
        lock_cache_write();
#endif

        CacheFiles contents;
        unsigned long long computed_size = m_collect_cache_dir_info(contents);
#if 0
        if (BESISDEBUG( "cache_contents" )) {
            BESDEBUG(CACHE, "BEFORE Purge " << computed_size/BYTES_PER_MEG << endl );
            CacheFiles::iterator ti = contents.begin();
            CacheFiles::iterator te = contents.end();
            for (; ti != te; ti++) {
                BESDEBUG(CACHE, (*ti).time << ": " << (*ti).name << ": size " << (*ti).size/BYTES_PER_MEG << endl );
            }
        }
#endif
        BESDEBUG(CACHE, prolog << "Current and target size (in MB) "
            << computed_size/BYTES_PER_MEG << ", " << d_target_size/BYTES_PER_MEG << endl);

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
                if (i->name != new_file && get_exclusive_lock_nb(i->name, cfile_fd)) {
                    BESDEBUG(CACHE, prolog << "purge: " << i->name << " removed." << endl);

                    if (unlink(i->name.c_str()) != 0)
                        throw BESInternalError(
                            prolog + "Unable to purge the file " + i->name + " from the cache: " + get_errno(), __FILE__,
                            __LINE__);

                    unlock(cfile_fd);
                    computed_size -= i->size;
                }
                ++i;

                BESDEBUG(CACHE,prolog << "Current and target size (in MB) "
                    << computed_size/BYTES_PER_MEG << ", " << d_target_size/BYTES_PER_MEG << endl);
            }
        }

        if (lseek(d_cache_info_fd, 0, SEEK_SET) == -1)
            throw BESInternalError(prolog + "Could not rewind to front of cache info file.", __FILE__, __LINE__);

        if (write(d_cache_info_fd, &computed_size, sizeof(unsigned long long)) != sizeof(unsigned long long))
            throw BESInternalError(prolog + "Could not write size info to the cache info file!", __FILE__, __LINE__);
#if 0
        if (BESISDEBUG( "cache_contents" )) {
            contents.clear();
            computed_size = m_collect_cache_dir_info(contents);
            BESDEBUG(CACHE, "AFTER Purge " << computed_size/BYTES_PER_MEG << endl );
            CacheFiles::iterator ti = contents.begin();
            CacheFiles::iterator te = contents.end();
            for (; ti != te; ti++) {
                BESDEBUG(CACHE, (*ti).time << ": " << (*ti).name << ": size " << (*ti).size/BYTES_PER_MEG << endl );
            }
        }
#endif
#if 0
    unlock_cache();
    }
    catch (...) {
        unlock_cache();
        throw;
    }
#endif
}

/**
 * A blocking call to get an exclusive (write) lock on a file in the cache.
 * Because this cache uses per-process advisory locking, it's possible to
 * call this several times *from within a single process* and get a lock
 * each time.
 *
 * @note This is used only by the purge_file() method.
 *
 * @param file_name Name of the file to lock
 * @param ref_fd Return value parameter that holds the file descriptor that
 * is locked.
 * @return Return false if the file does not exist, otherwise block until
 * the lock is acquired and then return true.
 * @exception BESInternalError is thrown on any condition other than the file
 * not existing
 */
bool BESFileLockingCache::get_exclusive_lock(const string &file_name, int &ref_fd)
{
    BESDEBUG(LOCK, prolog << "BEGIN filename: " << file_name <<endl);

    int fd;
    if ((fd = open(file_name.c_str(), O_RDWR)) < 0) {
        switch (errno) {
            case ENOENT:
                return false;

            default:
            {
                stringstream msg;
                msg << prolog << "FAILED to open file: " << file_name << " errno: " << get_errno();
                BESDEBUG(LOCK, msg.str() << endl);
                throw BESInternalError(msg.str() , __FILE__, __LINE__);
            }
        }
    }

    struct flock *l = advisory_lock(F_WRLCK);
    if (fcntl(fd, F_SETLKW, l) == -1) {     // F_SETLKW == blocking lock
        close(fd);
        ostringstream oss;
        oss << "cache process: " << l->l_pid << " triggered a locking error for '" << file_name << "': " << get_errno();
        throw BESInternalError(oss.str(), __FILE__, __LINE__);
    }

    // Success
    m_record_descriptor(file_name,fd);
    ref_fd = fd;

    BESDEBUG(LOCK, prolog << "END filename: " << file_name <<endl);
    return true;
}

/**
 * @brief Purge a single file from the cache
 *
 * Purge a single file from the cache. The file might be old, etc., and need to
 * be removed. Don't use this to shrink the cache when it gets too big, use
 * update_and_purge() instead since that file optimizes accesses to the cache
 * control file for several changes in a row.
 *
 * @param file The name of the file to purge.
 */
void BESFileLockingCache::purge_file(const string &file)
{
    AdvisoryLockGuard write_alg(d_cache_info_fd, F_WRLCK);
    BESDEBUG(CACHE, prolog << "Starting the purge" << endl);

#if 0
    try {
        lock_cache_write();
#endif

        // Grab an exclusive lock on the file. SonarScan will complain about nested trys... jhrg 11/16/22
        int cfile_fd;
        try {
            if (get_exclusive_lock(file, cfile_fd)) {
                // Get the file's size
                unsigned long long size = 0;
                struct stat buf;
                if (stat(file.c_str(), &buf) == 0) {
                    size = buf.st_size;
                }

                BESDEBUG(CACHE, prolog << "file: " << file << " removed." << endl);

                if (unlink(file.c_str()) != 0)
                    throw BESInternalError(
                            prolog + "Unable to purge the file " + file + " from the cache: " + get_errno(), __FILE__,
                            __LINE__);
#if 0
                //  The exception above could result in a leak. jhrg 11/16/22 fixed 8/29/25
                  fd = m_remove_descriptor(file);
                  if (fd != cfile_fd){
                    throw BESInternalError(
                            prolog + "File descriptor does not match purged file descriptor for " + file, __FILE__,
                            __LINE__);
                }

                unlock(cfile_fd);
#endif
                unlock_and_close(file);
                unsigned long long cache_size = get_cache_size() - size;

                if (lseek(d_cache_info_fd, 0, SEEK_SET) == -1)
                    throw BESInternalError(prolog + "Could not rewind to front of cache info file.", __FILE__,
                                           __LINE__);

                if (write(d_cache_info_fd, &cache_size, sizeof(unsigned long long)) != sizeof(unsigned long long))
                    throw BESInternalError(prolog + "Could not write size info to the cache info file!", __FILE__,
                                           __LINE__);
            }
        }
        catch (...) {
            unlock_and_close(file);
#if 0
              if (cfile_fd != -1)
                unlock(cfile_fd);
#endif
        }
#if 0
        unlock_cache();
    }
catch (...) {
        unlock_cache();
        throw;
    }
#endif
}

/**
 * Does the directory exist?
 *
 * @note This is a static method, so it can be called from other
 * static methods like those that build instances of singletons.
 *
 * @param dir The pathname to test.
 * @return True if the directory exists, false otherwise
 */
bool BESFileLockingCache::dir_exists(const string &dir)
{
    struct stat buf{0};

    return (stat(dir.c_str(), &buf) == 0) && (buf.st_mode & S_IFDIR);
}

/**
 * @brief dumps information about this object
 *
 * Displays the pointer value of this instance along with information about
 * this cache.
 *
 * @param strm C++ i/o stream to dump the information to
 */
void BESFileLockingCache::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << prolog << "(" << (void *) this << ")" << endl;
    BESIndent::Indent();
    strm << BESIndent::LMarg << "cache dir: " << d_cache_dir << endl;
    strm << BESIndent::LMarg << "prefix: " << d_prefix << endl;
    strm << BESIndent::LMarg << "size (bytes): " << d_max_cache_size_in_bytes << endl;
    BESIndent::UnIndent();
}
