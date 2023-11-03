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

#include <vector>
#include <algorithm>
#include <map>
#include <mutex>

#include <cstring>

#include <unistd.h>
#include <dirent.h>
#include <sys/file.h>
#include <sys/stat.h>

#include "BESUtil.h"
#include "BESLog.h"

static inline std::string get_errno() {
    const char *s_err = strerror(errno);
    return s_err ? s_err : "unknown error";
}

const unsigned long long MEGABYTE = 1048576;

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
 * This is the Nth rewrite of the original BES 'uncompress cache' and it now
 * supplies a much simpler interface: initialize(), put(), get(), and del().
 * The constructor for FileCache no longer initializes the cache, use the named
 * method for that. The put(key, source file) method locks the cache and copies
 * the contents of 'source file' into a new file that is named 'key.' The
 * get(key, item) method opens the file named 'key' and returns an instance of
 * FileCache::Item that holds the locked (shared, using flock(2)) file. Use
 * close(item.get_fd()) to release the lock and close the file. The del(key)
 * method deletes the file if it can get an exclusive lock on the file. The
 * del() method is the only method that uses a non-blocking lock on a file.
 * If can be forced to use a blocking lock using an optional second argument.
 *
 * @note The locking mechanism uses Unix flock(2) and so is _per file_.
 * Using flock(2) instead of fcntl(2) means that the locking is thread-safe. On
 * older linux kernels (< 2.6.12) flock(2) does not work with NFSv4. Based on
 * stackoverflow[0], it should work with an AWS EFS volume and newer NFS implementations.
 * (YMMV).
 *
 * [0]: stackoverflow.com/questions/53177938/is-it-safe-to-use-flock-on-aws-efs-to-emulate-a-critical-section
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

    const std::string CACHE_INFO_FILE_NAME = "cache_info";

    static std::string get_lock_type_string(int lock_type) {
        return (lock_type == LOCK_EX) ? "Exclusive": "Shared";
    }

    /// Manage the Cache-level locking. Each instance has to be initialized with d_cache_info_fd.
    class CacheLock {
    private:
        int d_fd = -1;
        std::mutex cache_lock_mtx;

    public:
        CacheLock() = default;
        CacheLock(const CacheLock &) = delete;
        explicit CacheLock(int fd) : d_fd(fd) {}
        CacheLock &operator=(const CacheLock &) = delete;
        ~CacheLock() {
            if (flock(d_fd, LOCK_UN) < 0)
                ERROR_LOG("Could not unlock the FileCache.\n");
        }

        bool lock_the_cache(int lock_type, const std::string &msg = "") {
            if (d_fd < 0) {
                ERROR_LOG("Call to CacheLock::lock_the_cache with uninitialized lock object\n");
                return false;
            }
            const std::lock_guard<std::mutex> lock(cache_lock_mtx);
            if (flock(d_fd, lock_type) < 0) {
                if (msg.empty())
                    ERROR_LOG(msg << get_lock_type_string(lock_type) << get_errno() << '\n');
                else
                    ERROR_LOG(msg << get_errno() << '\n');
                return false;
            }
            return true;
        }
    };

#if 1
    bool purge() {
        // Lock the cache. Ensure the cache is unlocked no matter how we exit
        CacheLock lock(d_cache_info_fd);
        if (!lock.lock_the_cache(LOCK_EX, "Error locking the cache in clear()."))
            return false;

        struct item_info {
            std::string d_name;
            off_t d_size;
            item_info(const std::string &name, off_t size) :d_name(name), d_size(size) {}
        };

        // sorted by access time, with the oldest time first
        std::multimap<unsigned long, struct item_info, std::less<>> items;

        // When we move the C++-17, we can use std::filesystem to do this. jhrg 10/24/23
        DIR *dir;
        const struct dirent *ent;
        if ((dir = opendir (d_cache_dir.c_str())) != nullptr) {
            /* print all the files and directories within directory */
            while ((ent = readdir(dir)) != nullptr) {
                // Skip the '.' and '..' files and the cache info file
                if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0
                    || strcmp(ent->d_name, CACHE_INFO_FILE_NAME.c_str()) == 0)
                    continue;
                // for each file, record its name, size and access time
                struct stat sb{0};
                std::string path_name = BESUtil::pathConcat(d_cache_dir, ent->d_name);
                if (stat(path_name.c_str(), &sb) < 0) {
                    ERROR_LOG("Error purging the cache directory (" << ent->d_name << ") - " << get_errno() << '\n');
                    return false;
                }

                items.insert(std::pair<unsigned long, item_info>(sb.st_atime, item_info(path_name, sb.st_size)));
            }
            closedir (dir);

            // choose which files to remove

            // update the cache info file
#if 0
            if (!update_cache_info_size(0))
                return false;
#endif

            return true;
        }
        else {
            ERROR_LOG("Error opening the cache directory (" << d_cache_dir << ") for purging - " << get_errno() << '\n');
            return false;
        }
    }
#endif

    // These private methods assume they are called on a locked instance of the cache.

    bool invariant() const {
        if (d_cache_info_fd < 0)
            return false;
        return true;
    }

    // Return the size of the file in bytes. Return zero on error.
    static unsigned long long get_file_size(int fd) {
        struct stat sb{0};
        if (fstat(fd, &sb) != 0)
            return 0;
        return sb.st_size;
    }

    // Open the cache info file and write a zero to it.
    // Assign the file descriptor to d_cache_info_fd.
    // d_cache_dir must be set.
    bool open_cache_info() {
        if (d_cache_dir.empty())
            return false;
        if ((d_cache_info_fd = open(BESUtil::pathConcat(d_cache_dir, CACHE_INFO_FILE_NAME).c_str(), O_RDWR | O_CREAT | O_EXCL, 0666)) >= 0) {
            unsigned long long size = 0;
            if (write(d_cache_info_fd, &size, sizeof(size)) != sizeof(size))
                return false;
        }
        else if ((d_cache_info_fd = open(BESUtil::pathConcat(d_cache_dir, CACHE_INFO_FILE_NAME).c_str(), O_RDWR, 0666)) < 0) {
            return false;
        }
        return true;
    }

    // Return the size of the cache as recorded in the cache info file.
    // Return zero on error or if there's nothing in the cache.
    unsigned long long get_cache_info_size() const {
        if (d_cache_info_fd == -1)
            return 0;
        if (lseek(d_cache_info_fd, 0, SEEK_SET) == -1)
            return 0;
        unsigned long long size = 0;
        if (read(d_cache_info_fd, &size, sizeof(size)) != sizeof(size))
            return 0;
        return size;
    }

    bool update_cache_info_size(unsigned long long size) const {
        if (d_cache_info_fd == -1)
            return false;
        if (lseek(d_cache_info_fd, 0, SEEK_SET) == -1)
            return false;
        if (write(d_cache_info_fd, &size, sizeof(size)) != sizeof(size))
            return false;
        return true;
    }

    friend class FileCacheTest;

public:
    /// Used to mange the state of an open file descriptor for a cached item.
    class Item {
        int d_fd = -1;

        // This is static because two threads might want each want to lock the same file. jhrg 11/01/23
        std::mutex item_mtx; // Overkill to make a static mutex? jhrg 11/01/23

    public:
        Item() = default;
        Item(const Item &) = delete;
        explicit Item(int fd) : d_fd(fd) { }
        Item &operator=(const Item &) = delete;
        virtual ~Item() {
             if (d_fd != -1) {
                close(d_fd);    // Also releases any locks
                d_fd = -1;
            }
        }

        int get_fd() const {
            return d_fd;
        }
        void set_fd(int fd) {
            d_fd = fd;
        }

        bool lock_the_item(int lock_type, const std::string &msg = "") {
            if (d_fd < 0) {
                ERROR_LOG("Call to Item::lock_the_item() with uninitialized item file descriptor.\n");
                return false;
            }
            const std::lock_guard<std::mutex> lock(item_mtx);
            if (flock(d_fd, lock_type) < 0) {
                if (msg.empty())
                    ERROR_LOG("Could not get " << get_lock_type_string(lock_type) << " lock: " << get_errno() << '\n');
                else
                    ERROR_LOG(msg << ": " << get_errno() << '\n');
                return false;
            }

            return true;
        }
    };

    class PutItem : public Item {
        FileCache &d_fc;
    public:
        PutItem() = delete;
        explicit PutItem(FileCache &fc) : d_fc(fc) {}
        PutItem(const PutItem &) = delete;
        const PutItem &operator=(const PutItem &) = delete;
        ~PutItem() override {
            if (!d_fc.update_cache_info_size(d_fc.get_cache_info_size() + get_file_size(get_fd()))) {
                ERROR_LOG("Could not update the cache info file while unlocking a put item: " << get_errno() << '\n');
            }
        }
    };

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

    // Add a copy of the file to the Cache, locks the cache while adding
    bool put(const std::string &key, const std::string &file_name) {
        // Lock the cache. Ensure the cache is unlocked no matter how we exit
        CacheLock lock(d_cache_info_fd);
        if (!lock.lock_the_cache(LOCK_EX, "locking the cache in put() for: " + key))
            return false;

        // Create the new cache entry
        std::string key_file_name = BESUtil::pathConcat(d_cache_dir, key);
        int fd;
        if ((fd = open(key_file_name.c_str(), O_CREAT | O_EXCL | O_RDWR, 0666)) < 0) {
            if  (errno == EEXIST)
                return false;
            else {
                ERROR_LOG("Error creating key/file: " << key << " " << get_errno() << '\n');
                return false;
            }
        }

        // The Item instance will take care of closing the file.
        Item fdl(fd);

        // Lock the file for writing; released when the file descriptor is closed.
        if (!fdl.lock_the_item(LOCK_EX, "Error locking the just created key/file: " + key))
            return false;

        // Copy the contents of the file_name to the new file
        int fd2;
        if ((fd2 = open(file_name.c_str(), O_RDONLY)) < 0) {
            ERROR_LOG("Error reading from source file: " << file_name << " " << get_errno() << '\n');
            return false;
        }

        Item fdl2(fd2);     // The 'source' file is not locked; the Item ensures it is closed.

        // TODO Here we might use st_blocks and st_blksize if that will speed up the transfer.
        //  This is likely to matter only for large files (where large means...?). jhrg 11/02/23
        std::vector<char> buf(std::min(MEGABYTE, get_file_size(fd2)));
        ssize_t n;
        while ((n = read(fd2, buf.data(), buf.size())) > 0) {
            if (write(fd, buf.data(), n) != n) {
                ERROR_LOG("Error writing to destination file: " << key << " " << get_errno() << '\n');
                return false;
            }
        }

        // NB: The cache_info file ws locked on entry to this method.
        if (!update_cache_info_size(get_cache_info_size() + get_file_size(fd)))
            return false;

        // The fd_wrapper instances will take care of closing (and thus unlocking) the files.
        return true;
    }

    // This version of put() returns a locked open file descriptor in 'item.'
    // When the item is destroyed the cache size it updated and the descriptor
    // is closed. This exists so that code can write directly to a locked file.
    bool put(const std::string &key, PutItem &item) {
        // Lock the cache. Ensure the cache is unlocked no matter how we exit
        CacheLock lock(d_cache_info_fd);
        if (!lock.lock_the_cache(LOCK_EX, "locking the cache in put() for: " + key))
            return false;

        // Create the new cache entry
        std::string key_file_name = BESUtil::pathConcat(d_cache_dir, key);
        int fd;
        if ((fd = open(key_file_name.c_str(), O_CREAT | O_EXCL | O_RDWR, 0666)) < 0) {
            if  (errno == EEXIST)
                return false;
            else {
                ERROR_LOG("Error creating key/file: " << key << " " << get_errno() << '\n');
                return false;
            }
        }

        // The Item instance will take care of closing the file.
        item.set_fd(fd);

        // Lock the file for writing; released when the file descriptor is closed.
        if (!item.lock_the_item(LOCK_EX, "Error locking the just created key/file: " + key))
            return false;

        // The Item instances will take care of closing (and thus unlocking) the files.
        return true;
    }

    bool get(const std::string &key, Item &item, int lock_type = LOCK_SH | LOCK_NB) {
        // Lock the cache. Ensure the cache is unlocked no matter how we exit
        CacheLock lock(d_cache_info_fd);
        if (!lock.lock_the_cache(LOCK_EX, "Error locking the cache in get() for: " + key))
            return false;

        // open the file
        std::string key_file_name = BESUtil::pathConcat(d_cache_dir, key);
        int fd = open(key_file_name.c_str(), O_RDONLY, 0666);
        if (fd < 0) {
            ERROR_LOG("Error opening the cache item in get for: " << key << " " << get_errno() << '\n');
            return false;
        }

        item.set_fd(fd);
        if (!item.lock_the_item(lock_type, "locking the cache item in get() for: " + key))
            return false;

        // Here's where we should update the info about the item in the cache_info file

        return true;
    }

    // Remove a file from the Cache. By default, try to lock the item exclusively,
    // using a non-blocking lock. If that fails, retry and maybe use a blocking lock.
    bool del(const std::string &key, int lock_type = LOCK_EX | LOCK_NB) {
        // Lock the cache. Ensure the cache is unlocked no matter how we exit
        CacheLock lock(d_cache_info_fd);
        if (!lock.lock_the_cache(LOCK_EX, "Error locking the cache in del()."))
            return false;

        std::string key_file_name = BESUtil::pathConcat(d_cache_dir, key);
        int fd = open(key_file_name.c_str(), O_WRONLY, 0666);
        if (fd < 0) {
            ERROR_LOG("Error opening the cache item in del() for: " << key << " " << get_errno() << '\n');
            return false;
        }

        Item item(fd);
        if (!item.lock_the_item(lock_type, "locking the cache item in del() for: " + key))
            return false;

        auto file_size = get_file_size(fd);

        if (remove(key_file_name.c_str()) != 0) {
            ERROR_LOG("Error removing " << key << " from cache directory (" << d_cache_dir << ") - " << get_errno() << '\n');
            return false;
        }

        if (!update_cache_info_size(get_cache_info_size() - file_size))
            return false;

        return true;
    }

    /**
     * @brief Remove all files from the cache. Zero the cache info file.
     * @return false if the cache directory could not be opened or a file
     * in the cache could not be removed, true otherwise.
     */
    bool clear() {
        // Lock the cache. Ensure the cache is unlocked no matter how we exit
        CacheLock lock(d_cache_info_fd);
        if (!lock.lock_the_cache(LOCK_EX, "Error locking the cache in clear()."))
            return false;

        // When we move the C++-17, we can use std::filesystem to do this. jhrg 10/24/23
        DIR *dir;
        struct dirent *ent;
        if ((dir = opendir (d_cache_dir.c_str())) != nullptr) {
            /* print all the files and directories within directory */
            while ((ent = readdir (dir)) != nullptr) {
                // Skip the '.' and '..' files and the cache info file
                if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0
                    || strcmp(ent->d_name, CACHE_INFO_FILE_NAME.c_str()) == 0)
                    continue;
                if (remove(BESUtil::pathConcat(d_cache_dir, ent->d_name).c_str()) != 0) {
                    ERROR_LOG("Error removing " << ent->d_name << " from cache directory (" << d_cache_dir << ") - " << get_errno() << '\n');
                    return false;
                }
            }
            closedir (dir);

            // Zero the cache info file
            if (!update_cache_info_size(0))
                return false;

            return true;
        }
        else {
            ERROR_LOG("Error clearing the cache directory (" << d_cache_dir << ") - " << get_errno() << '\n');
            return false;
        }
    }
};

#endif // FileCache_h_
