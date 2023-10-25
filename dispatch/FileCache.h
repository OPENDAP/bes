// FileCache.h

// This file was originally part of bes, A C++ back-end server
// implementation framework for the OPeNDAP Data Access Protocol.
// Copied to libdap. This is used to cache responses built from
// functional CE expressions.

// Copyright (c) 2023 OPeNDAP, Inc
// Author: James Gallagher <jgallagher@opendap.org>
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

#ifndef FileCache_h_
#define FileCache_h_ 1

#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>

#include "BESUtil.h"
#include "BESLog.h"

static inline std::string get_errno() {
    const char *s_err = strerror(errno);
    return s_err ? s_err : "unknown error";
}

// Name of the file that tracks the size of the cache
#define CACHE_INFO_FILE_NAME "cache_info"

// Build a lock of a certain type. Locks the entire file.
//
// Using whence == SEEK_SET with start and len set to zero means lock the whole file.
// jhrg 9/8/18
static inline struct flock *advisory_lock(int type)
{
    static struct flock lock{0};
    lock.l_type = type;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;
    lock.l_pid = getpid();

    return &lock;
}

/**
 * @brief Implementation of a caching mechanism for compressed data.
 *
 * This cache uses simple advisory locking found on most modern unix file systems.
 * It was originally designed to hold the decompressed versions of compressed files.
 * Compressed files are uncompressed and stored in a cache where they can be
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
 * close + unlock operations are performed atomically. Other methods that operate
 * on the cache info file must only be called when the lock has been obtained.
 *
 * @note The locking mechanism uses Unix fcntl(2) and so is _per process_. That
 * means that while getting an exclusive lock in one process will keep other
 * processes from also getting an exclusive lock, it _will not_ prevent other
 * threads in the same process from getting another 'exclusive lock.' We could
 * switch to flock(2) and get thread-safe locking, but we would trade off the
 * ability to work with files on NFS volumes.
 */
class FileCache {

    // pathname of the cache directory
    std::string d_cache_dir;

    /// How many bytes can the cache hold before we have to purge?
    /// A value of zero indicates a cache of unlimited size.
    unsigned long long d_max_cache_size_in_bytes = 0;

    // When we purge, how much should we throw away. Set in the ctor to 80% of the max size.
    unsigned long long d_purge_size = 0;

    // Name of the file that tracks the size of the cache
    int d_cache_info_fd = -1;

    void purge();

    bool invariant(bool expensive = true) const {
        if (d_cache_info_fd < 0)
            return false;
        return true;
    }

    // Open the cache info file and write a zero to it.
    // Assign the file descriptor to d_cache_info_fd.
    // d_cache_dir must be set.
    bool open_cache_info() {
        if (d_cache_dir.empty())
            return false;
        if ((d_cache_info_fd = open(BESUtil::pathConcat(d_cache_dir, CACHE_INFO_FILE_NAME).c_str(), O_RDWR | O_CREAT, 0666)) < 0)
            return false;
        unsigned long long size = 0;
        if (write(d_cache_info_fd, &size, sizeof(size)) != sizeof(size))
            return false;
        return true;
    }

    bool update_cache_info_size(long long size) {
        if (d_cache_info_fd == -1)
            return false;
        if (lseek(d_cache_info_fd, 0, SEEK_SET) == -1)
            return false;
        if (write(d_cache_info_fd, &size, sizeof(size)) != sizeof(size))
            return false;
        return true;
    }

    // Return the size of the cache as recorded in the cache info file.
    // Return zero on error or if there's nothing in the cache.
    unsigned long long get_cache_info_size() {
        if (d_cache_info_fd == -1)
            return 0;
        if (lseek(d_cache_info_fd, 0, SEEK_SET) == -1)
            return 0;
        unsigned long long size = 0;
        if (read(d_cache_info_fd, &size, sizeof(size)) != sizeof(size))
            return 0;
        return size;
    }

    friend class cacheT;
    friend class FileLockingCacheTest;  // This is in dispatch/tests
    friend class FileCacheTest;

public:
    FileCache() = default;
    FileCache(const FileCache &) = delete;
    FileCache &operator=(const FileCache &rhs) = delete;

    virtual ~FileCache() {
        if (d_cache_info_fd != -1) {
            close(d_cache_info_fd);
        }
    }

    /**
     * @brief Initialize the cache.
     * @param cache_dir Directory on some filesystem where the cache will be stored.
     * @param size Allow this many bytes in the cache
     * @param target_size When purging, remove items until this many bytes remain.
     * @return False if the cache object could not be initialized, true otherwise.
     */
    virtual bool initialize(const std::string &cache_dir, long long size, long long purge_size) {
        if (size < 0 || purge_size < 0)
            return false;

        struct stat sb{0};
        if (stat(cache_dir.c_str(), &sb) != 0)
            return false;

        d_cache_dir = cache_dir;

        if (!open_cache_info())
            return false;

        d_max_cache_size_in_bytes = (unsigned long long)size;
        d_purge_size = (unsigned long long)purge_size;
        return true;
    }

    // Add a Value to the Cache, locks the cache while adding
    bool put(const std::string &key, const std::string &file_name) {
        struct fd_wrapper {
            int fd;
            fd_wrapper(int fd) : fd(fd) {}
            ~fd_wrapper() { close(fd); }
        };

        // Create the new cache entry
        std::string key_file_name = BESUtil::pathConcat(d_cache_dir, key);
        int fd;
        if ((fd = open(key_file_name.c_str(), O_CREAT | O_EXCL | O_RDWR, 0666)) < 0) {
            if  (errno == EEXIST)
                return false;
            else {
                ERROR_LOG("Error creating key/file: " << key << " " << get_errno());
                return false;
            }
        }
        fd_wrapper fdw(fd);

        // Lock the file for writing
        struct flock *l = advisory_lock(F_WRLCK);
        if (fcntl(fd, F_SETLKW, l) == -1) {     // F_SETLKW == set blocking write lock
            close(fd);
            ERROR_LOG("Error write locking the just created key/file: " << key << " " << get_errno());
            return false;
        }

        // Copy yhe contents of the file_name to the new file
        int fd2;
        if ((fd2 = open(file_name.c_str(), O_RDONLY)) < 0) {
            ERROR_LOG("Error reading from source file: " << file_name << " " << get_errno());
            return false;
        }
        fd_wrapper fdw2(fd2);

        int n;
        char buf[4096]; // TODO Could make this a function of the size of file_name (use stat). jhrg 10/23/23
        while ((n = read(fd2, buf, sizeof(buf))) > 0) {
            if (write(fd, buf, n) != n) {
                ERROR_LOG("Error writing to destination file: " << key << " " << get_errno());
                return false;
            }
        }

        // The fd_wrapper instances will take care of closing (and thus unlocking) the files.
        return true;
    }

    void put(const std::string &key, int fd);

    // Get a Value from the Cache, locks the cache while getting then return s locked object.
    /*CachedFileAsName*/ void get(const std::string &key);
    /*CachedFileAsDescriptor*/ void get_fd(const std::string &key);

    // Remove a Value from the Cache
    void del(const std::string &key);

    /**
     * @brief Remove all files from the cache. Zero the cache info file.
     * @return false if the cache directory could not be opened or a file
     * in the cache could not be removed, true otherwise.
     */
    bool clear() {
        // When we move the C++-17, we can use std::filesystem to do this. jhrg 10/24/23
        DIR *dir = nullptr;
        struct dirent *ent{0};
        if ((dir = opendir (d_cache_dir.c_str())) != nullptr) {
            /* print all the files and directories within directory */
            while ((ent = readdir (dir)) != nullptr) {
                // Skip the . and .. files and the cache info file
                if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0
                    || strcmp(ent->d_name, CACHE_INFO_FILE_NAME) == 0)
                    continue;
                if (remove(BESUtil::pathConcat(d_cache_dir, ent->d_name).c_str()) != 0) {
                    ERROR_LOG("Error removing " << ent->d_name << " from cache directory (" << d_cache_dir << ") - " << get_errno());
                    return false;
                }
            }
            closedir (dir);

            // Zero the cache info file
            if (!update_cache_info_size(0))
                return false;

            return true;
        } else {
            ERROR_LOG("Error clearing the cache directory (" << d_cache_dir << ") - " << get_errno());
            return false;
        }
    }
};

#endif // FileCache_h_
