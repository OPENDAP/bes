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

#include <algorithm>
#include <iomanip>
#include <map>
#include <mutex>
#include <sstream>
#include <utility>
#include <vector>

#include <cstring>

#include <dirent.h>
#include <fcntl.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>

#include <openssl/sha.h>

#include "BESLog.h"
#include "BESUtil.h"

// Make all the error log messages uniform in one small way. This is a macro
// so that we can switch to exceptions if that seems necessary. jhrg 11/06/23
#define ERROR(msg) ERROR_LOG("FileCache: " + std::string(msg))
#define INFO(msg) INFO_LOG("FileCache: " + std::string(msg))

// If this is defined, then the access time of a file is updated when it is
// closed by the Item dtor. This is a hack to get around the fact that the
// access time is not always updated by simple read operations.
// If this is set to zero, the cache may become, in effect, a FIFO and not
// an LRU cache. jhrg 11/03/23
#define FORCE_ACCESS_TIME_UPDATE 1

static inline std::string get_errno() {
    const char *s_err = strerror(errno);
    return s_err ? s_err : "unknown error";
}

const unsigned long long MEGABYTE = 1048576;

/**
 * @brief Implementation of a caching mechanism for files.
 *
 * This cache uses simple advisory locking found on most modern unix file systems.
 * It was originally designed to hold the decompressed versions of compressed files.
 *
 * Compressed files are uncompressed and stored in a cache where they can be
 * used over and over until removed from the cache. Several processes can
 * share the cache with each reading from files. At the same time, new files
 * can be added and the cache can be purged, without disrupting the existing
 * read operations.
 *
 * How it works: Before a file is added to the cache, the cache is locked - no
 * other processes can add, read or remove files. Once a file has been added,
 * the cache size is updated cache is unlocked. Unlike other caches, this
 * implementation does not automatically purge entries when the size becomes too
 * large. It is up to the client code to call the purge() method when the
 * size is at the maximum size.
 *
 * When a process tries to get a file that is already in the cache, the entire cache is
 * locked. If the file is present, a shared read lock on the cached file is
 * obtained and the cache is unlocked.
 *
 * For the put(), get(), and del() methods, the client code must manage the mapping
 * between the things in the cache and the keys.
 *
 * This is the Nth rewrite of the original BES 'uncompress cache' and it now
 * supplies a much simpler interface: initialize(), put(), get(), del(), and purge().
 * The constructor for FileCache no longer initializes the cache, use the named
 * method for that. The put(key, source file) method locks the cache and copies
 * the contents of 'source file' into a new file that is named 'key.' The
 * get(key, item) method opens the file named 'key' and returns an instance of
 * FileCache::Item that holds the locked (shared, using flock(2)) file. Use
 * close(item.get_fd()) to release the lock and close the file. The del(key)
 * method deletes the file if it can get an exclusive lock on the file. The
 * del() method is the only method that uses a non-blocking lock on a file.
 * If can be forced to use a blocking lock using an optional second argument.
 * There is a second put(key, PutItem) method that returns a PutItem instance
 * that holds an open, locked, file descriptor. The PutItem dtor updates the
 * cache_info file. This method exists to allow the caller to write directly
 * to the file and then close the file descriptor to release the lock.
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

    static std::string get_lock_type_string(int lock_type) { return (lock_type == LOCK_EX) ? "Exclusive" : "Shared"; }

    /// Manage the Cache-level locking. Each instance has to be initialized with d_cache_info_fd.
    class CacheLock {
    private:
        int d_fd = -1;

    public:
        CacheLock() = default;
        CacheLock(const CacheLock &) = delete;
        explicit CacheLock(int fd) : d_fd(fd) {}
        CacheLock &operator=(const CacheLock &) = delete;
        ~CacheLock() {
            if (flock(d_fd, LOCK_UN) < 0)
                ERROR("Could not unlock the FileCache.");
        }

        /**
         * This methods locks the cache using the given lock type. It is thread-safe.
         * @param lock_type LOCK_EX or LOCK_SH and can be combined with LOCK_NB
         * @param msg
         * @return true if the cache is locked, false otherwise. Errors are logged.
         */
        bool lock_the_cache(int lock_type, const std::string &msg = "") const {
            if (d_fd < 0) {
                ERROR("Call to CacheLock::lock_the_cache with uninitialized lock object.");
                return false;
            }
            if (flock(d_fd, lock_type) < 0) {
                if (msg.empty())
                    ERROR(msg + get_lock_type_string(lock_type) + get_errno());
                else
                    ERROR(msg + get_errno());
                return false;
            }
            return true;
        }
    };

    // These private methods assume they are called on a locked instance of the cache.

    /**
     * Return the open file descriptor to the file name 'key' in the cache.
     * The cache must be locked exclusively.
     * @param key
     * @return The open file descriptor
     */
    int create_key(const std::string &key) {
        std::string key_file_name = BESUtil::pathConcat(d_cache_dir, key);
        int fd;
        if ((fd = open(key_file_name.c_str(), O_CREAT | O_EXCL | O_RDWR, 0666)) < 0) {
            if (errno == EEXIST) {
                INFO_LOG("Could not create the key/file; it already exists: " + key + " " + get_errno());
                return -1;
            } else {
                ERROR("Error creating key/file: " + key + " " + get_errno());
                return -1;
            }
        }

        return fd;
    }

    /// Scan the cache and return a value-result vector of all the files it
    /// holds except the cache_info file.
    /// Return true if successful, false otherwise.
    bool files_in_cache(std::vector<std::string> &files) const {
        // When we move the C++-17, we can use std::filesystem to do this. jhrg 10/24/23
        DIR *dir;
        const struct dirent *ent;
        if ((dir = opendir(d_cache_dir.c_str())) != nullptr) {
            /* print all the files and directories within directory */
            while ((ent = readdir(dir)) != nullptr) {
                // Skip the '.' and '..' files and the cache info file
                // TODO For large caches, this could be slow. Instead, build the list
                //  and then use three operations to remove these three files. jhrg 12/27/24
                if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0 ||
                    strcmp(ent->d_name, CACHE_INFO_FILE_NAME.c_str()) == 0)
                    continue;
                files.emplace_back(BESUtil::pathConcat(d_cache_dir, ent->d_name));
            }
            closedir(dir);
        } else {
            ERROR("Could not open the cache directory (" + d_cache_dir + ").");
            return false;
        }

        return true;
    }

    /// Return true if the FileCache object is 'consistent.'
    bool invariant() const {
        if (d_cache_info_fd < 0)
            return false;
        return true;
    }

    /// Return the size of an open file in bytes. Return zero on error.
    static unsigned long long get_file_size(int fd) {
        struct stat sb = {};
        if (fstat(fd, &sb) != 0)
            return 0;
        return sb.st_size;
    }

    /// Open the cache info file. If the file does not exist, make and write a zero to it.
    /// Assign the file descriptor to d_cache_info_fd.
    /// d_cache_dir must be set.
    bool open_cache_info() {
        if (d_cache_dir.empty())
            return false;
        // If  O_CREAT and O_EXCL are used together and the file already exists, then open()
        // fails with the error EEXIST. In that case, try to open the file using simple RDWR.
        if ((d_cache_info_fd = open(BESUtil::pathConcat(d_cache_dir, CACHE_INFO_FILE_NAME).c_str(),
                                    O_RDWR | O_CREAT | O_EXCL, 0666)) >= 0) {
            unsigned long long size = 0;
            if (write(d_cache_info_fd, &size, sizeof(size)) != sizeof(size))
                return false;
        } else if ((d_cache_info_fd =
                        open(BESUtil::pathConcat(d_cache_dir, CACHE_INFO_FILE_NAME).c_str(), O_RDWR, 0666)) < 0) {
            return false;
        }
        return true;
    }

    /// Return the size of the cache as recorded in the cache info file.
    /// Return zero on error or if there's nothing in the cache.
    unsigned long long get_cache_info_size() const {
        if (d_cache_info_fd == -1) {
            ERROR("Cache info file not open.");
            return 0;
        }
        if (lseek(d_cache_info_fd, 0, SEEK_SET) == -1) {
            ERROR("Could not seek to the beginning of the cache info file.");
            return 0;
        }
        unsigned long long size = 0;
        if (read(d_cache_info_fd, &size, sizeof(size)) != sizeof(size)) {
            ERROR("Could not read the cache info file.");
            return 0;
        }
        return size;
    }

    /// Add 'size' to the size recorded in the cache_info file
    /// Return true if the size is updated, false otherwise.
    bool update_cache_info_size(unsigned long long size) const {
        if (d_cache_info_fd == -1) {
            ERROR("Cache info file not open.");
            return false;
        }
        if (lseek(d_cache_info_fd, 0, SEEK_SET) == -1) {
            ERROR("Could not seek to the beginning of the cache info file.");
            return false;
        }
        if (write(d_cache_info_fd, &size, sizeof(size)) != sizeof(size)) {
            ERROR("Could not write to the cache info file.");
            return false;
        }
        return true;
    }

    friend class FileCacheTest;

public:
    /**
     * @brief Return a SHA256 hash of the given key.
     * @param key The key to has
     * @param log_it If true, write an info message to the bes log so it's easier to track the
     * key --> hash mapping. False by default.
     * @return The SHA256 hash of the key.
     */
    static std::string hash_key(const std::string &key, bool log_it = false) {
        unsigned char md[SHA256_DIGEST_LENGTH];
        SHA256(reinterpret_cast<const unsigned char *>(key.c_str()), key.size(), md);
        std::stringstream hex_stream;
        for (auto b : md) {
            hex_stream << std::hex << std::setw(2) << std::setfill('0') << (int)b;
        }
        if (log_it)
            INFO_LOG(":hash_key() " + key + " -> " + hex_stream.str());
        return {hex_stream.str()};
    }

    /// Manage the state of an open file descriptor for a cached item.
    class Item {
        int d_fd = -1;

    public:
        Item() = default;
        Item(const Item &) = delete;
        explicit Item(int fd) : d_fd(fd) {}
        Item &operator=(const Item &) = delete;
        virtual ~Item() {
            if (d_fd != -1) {
                close(d_fd); // Also releases any locks
                d_fd = -1;
            }
        }

        int get_fd() const { return d_fd; }
        void set_fd(int fd) { d_fd = fd; }

        bool lock_the_item(int lock_type, const std::string &msg = "") const {
            if (d_fd < 0) {
                ERROR("Call to Item::lock_the_item() with uninitialized item file descriptor.");
                return false;
            }
            if (flock(d_fd, lock_type) < 0) {
                if (msg.empty())
                    ERROR("Could not get " + get_lock_type_string(lock_type) + " lock: " + get_errno());
                else
                    ERROR(msg + ": " + get_errno());
                return false;
            }

#if FORCE_ACCESS_TIME_UPDATE
            futimes(d_fd, nullptr);
#endif
            return true;
        }
    };

    /**
     * A PutItem wraps a file descriptor just like an Item, but also ensures
     * that the cache_info file is updated to include the new item's size when
     * the PutIem goes out of scope.
     * @note The file referenced by the PutItem can be closed using close(2),
     * but that will not update the cache_info file.
     */
    class PutItem : public Item {
        FileCache &d_fc;

    public:
        PutItem() = delete;
        explicit PutItem(FileCache &fc) : d_fc(fc) {}
        PutItem(const PutItem &) = delete;
        const PutItem &operator=(const PutItem &) = delete;
        ~PutItem() override {
            // Locking the cache before calling update_cache_info_size() is necessary. jhrg 1/1/25
            CacheLock lock(d_fc.d_cache_info_fd);
            if (!lock.lock_the_cache(LOCK_EX,
                                     "locking the cache in ~PutItem() for descriptor: " + std::to_string(get_fd())))
                return;
            if (!d_fc.update_cache_info_size(d_fc.get_cache_info_size() + get_file_size(get_fd()))) {
                ERROR("Could not update the cache info file while unlocking a put item: " + get_errno());
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
     * @param cache_dir Directory on some filesystem where the cache will be stored. This
     * directory is made if it does not exist.
     * @param size Allow this many bytes in the cache
     * @param purge_size When purging, remove items until this many bytes remain.
     * @return False if the cache object could not be initialized, true otherwise.
     */
    virtual bool initialize(const std::string &cache_dir, long long size, long long purge_size) {
        if (size < 0 || purge_size < 0) {
            ERROR_LOG("FileCache::initialize() - size and purge_size must be >= 0\n");
            return false;
        }

        struct stat sb = {};
        if (stat(cache_dir.c_str(), &sb) != 0) {
            BESUtil::mkdir_p(cache_dir, 0775);
            if (stat(cache_dir.c_str(), &sb) != 0) {
                ERROR_LOG("FileCache::initialize() - could not stat the cache directory: " + cache_dir);
                return false;
            }
        }

        d_cache_dir = cache_dir;

        if (!open_cache_info()) {
            ERROR_LOG("FileCache::initialize() - could not open the cache info file: " + cache_dir);
            return false;
        }

        d_max_cache_size_in_bytes = (unsigned long long)size;
        d_purge_size = (unsigned long long)purge_size;
        return true;
    }

    /**
     * @brief Put an item in the cache
     * Put the contents of a file in the cache, referenced by the given key.
     * The item is unlocked when this method returns and cache_info file
     * is updated.
     * @note This method _copies_ the file contents to cache it.
     * @param key The key used to access the file
     * @param file_name The name of the file that will be cached.
     * @return True if the data are cached, false otherwise.
     */
    bool put(const std::string &key, const std::string &file_name) {
        // Lock the cache. Ensure the cache is unlocked no matter how we exit
        CacheLock lock(d_cache_info_fd);
        if (!lock.lock_the_cache(LOCK_EX, "locking the cache in put(1) for: " + key))
            return false;

        // Create the new cache entry
        int fd = create_key(key);
        if (fd == -1)
            return false;

        // The Item instance will take care of closing the file.
        Item fdl(fd);

        // Lock the file for writing; released when the file descriptor is closed.
        if (!fdl.lock_the_item(LOCK_EX, "locking the just created key/file in put(1): " + key))
            return false;

        // Copy the contents of file_name to the new file
        int fd2;
        if ((fd2 = open(file_name.c_str(), O_RDONLY)) < 0) {
            ERROR("Error reading from source file: " + file_name + " " + get_errno());
            return false;
        }

        Item fdl2(fd2); // The 'source' file is not locked; the Item ensures it is closed.

        // Here we might use st_blocks and st_blksize if that will speed up the transfer.
        // This is likely to matter only for large files (where large means...?). jhrg 11/02/23
        std::vector<char> buf(std::min(MEGABYTE, get_file_size(fd2)));
        ssize_t n;
        while ((n = read(fd2, buf.data(), buf.size())) > 0) {
            if (write(fd, buf.data(), n) != n) {
                ERROR("Error writing to destination file: " + key + " " + get_errno());
                return false;
            }
        }

        // NB: The cache_info file ws locked on entry to this method.
        if (!update_cache_info_size(get_cache_info_size() + get_file_size(fd)))
            return false;

        // The fd_wrapper instances will take care of closing (and thus unlocking) the files.
        return true;
    }

    bool put_data(const std::string &key, const std::string &data) {
        // Lock the cache. Ensure the cache is unlocked no matter how we exit
        CacheLock lock(d_cache_info_fd);
        if (!lock.lock_the_cache(LOCK_EX, "locking the cache in put_data() for: " + key))
            return false;

        // Create the new cache entry
        int fd = create_key(key);
        if (fd == -1)
            return false;

        // The Item instance will take care of closing the file.
        Item fdl(fd);

        // Lock the file for writing; released when the file descriptor is closed.
        if (!fdl.lock_the_item(LOCK_EX, "locking the just created key/file in put_data(): " + key))
            return false;

        // Here we might use st_blocks and st_blksize if that will speed up the transfer.
        // This is likely to matter only for large files (where large means...?). jhrg 11/02/23

        if (write(fd, data.c_str(), data.size()) != data.size()) {
            ERROR("Error writing to data to cache file: " + key + " " + get_errno());
            return false;
        }

        // NB: The cache_info file ws locked on entry to this method.
        if (!update_cache_info_size(get_cache_info_size() + get_file_size(fd)))
            return false;

        // The fd_wrapper instances will take care of closing (and thus unlocking) the files.
        return true;
    }

    /**
     * @brief Put an item in the cache
     * Put an item in the cache by returning an open and lock file descriptor to
     * the item. The called can write directly to the item, rewind the descriptor
     * and read from it and then close the file descriptor to release the Exclusive
     * lock.
     * @note When the PutItem goes out of scope, the cache_info file is updated. Make
     * _sure_ no operation that locks the cache gets called before the PutItem lock
     * is removed, otherwise there will be a deadlock.
     * @param key The key that can be used to access the file
     * @param item A value-result parameter than is a reference to a PutItem instance.
     * @return True if the PutItem holds an open, locked, file descriptor, otherwise
     * false.
     */
    bool put(const std::string &key, PutItem &item) {
        // Lock the cache. Ensure the cache is unlocked no matter how we exit
        CacheLock lock(d_cache_info_fd);
        if (!lock.lock_the_cache(LOCK_EX, "locking the cache in put(2) for: " + key))
            return false;

        // Create the new cache entry
        int fd = create_key(key);
        if (fd == -1)
            return false;

        // The Item instance will take care of closing the file.
        item.set_fd(fd);

        // Lock the file for writing; released when the file descriptor is closed.
        if (!item.lock_the_item(LOCK_EX, "locking the just created key/file in put(2): " + key))
            return false;

        // The Item instances will take care of closing (and thus unlocking) the files.
        return true;
    }

    /**
     * @brief Get a locked (shared) item for a file in the cache.
     * @param key The key to the cached item
     * @param item A reference to an Item - a value-result parameter. Release the lock
     * by closing the file.
     * @param lock_type By default, get a shared non-blocking lock.
     * @return True if the item was found and locked, false otherwise
     */
    bool get(const std::string &key, Item &item, int lock_type = LOCK_SH | LOCK_NB) {
        // Lock the cache. Ensure the cache is unlocked no matter how we exit
        CacheLock lock(d_cache_info_fd);
        if (!lock.lock_the_cache(LOCK_EX, "locking the cache in get() for: " + key))
            return false;

        // open the file
        std::string key_file_name = BESUtil::pathConcat(d_cache_dir, key);
        int fd = open(key_file_name.c_str(), O_RDONLY, 0666);
        if (fd < 0) {
            if (errno == ENOENT)
                return false;
            else {
                ERROR("Error opening the cache item in get for: " + key + " " + get_errno());
                return false;
            }
        }

        item.set_fd(fd);
        if (!item.lock_the_item(lock_type, "locking the item in get() for: " + key))
            return false;

        // Here's where we should update the info about the item in the cache_info file

        return true;
    }

    /**
     * @brief Remove the item at the given key
     * Remove the key/item. Updates the size recorded in cache_info. Returns false
     * if a number of bad things happen OR if the item could not be locked (i.e.,
     * there was an error or the item is in use).
     * @note This is not used by purge()
     * @param key The item key
     * @param lock_type The kind of lock to use; Exclusive Non-blocking by default.
     * @return True if the key/item is deleted, false if not.
     */
    bool del(const std::string &key, int lock_type = LOCK_EX | LOCK_NB) {
        // Lock the cache. Ensure the cache is unlocked no matter how we exit
        CacheLock lock(d_cache_info_fd);
        if (!lock.lock_the_cache(LOCK_EX, "locking the cache in del()."))
            return false;

        std::string key_file_name = BESUtil::pathConcat(d_cache_dir, key);
        int fd = open(key_file_name.c_str(), O_WRONLY, 0666);
        if (fd < 0) {
            ERROR("Error opening the cache item in del() for: " + key + " " + get_errno());
            return false;
        }

        Item item(fd);
        if (!item.lock_the_item(lock_type, "locking the cache item in del() for: " + key))
            return false;

        auto file_size = get_file_size(fd);

        if (remove(key_file_name.c_str()) != 0) {
            ERROR("Error removing " + key + " from cache directory (" + d_cache_dir + ") - " + get_errno());
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
    bool clear() const {
        // Lock the cache. Ensure the cache is unlocked no matter how we exit
        CacheLock lock(d_cache_info_fd);
        if (!lock.lock_the_cache(LOCK_EX, "locking the cache in clear()."))
            return false;

        std::vector<std::string> files;
        if (!files_in_cache(files)) {
            return false;
        }

        for (const auto &file : files) {
            if (remove(file.c_str()) != 0) {
                ERROR("Error removing " + file + " from cache directory (" + d_cache_dir + ") - " + get_errno());
                return false;
            }
        }

        return true;
    }

    /**
     * @brief Purge the lest recently used items from the cache.
     * The purge() method for FileCache is public. It is the user's job to call purge.
     * The idea behind this is that user code can decide if purge needs to be called
     * on every put, every Nth put or not at all. Note that purge() (often) does nothing more
     * than compare the size recorded in the cache_info file (updated on every put())
     * with the configured max cache size.
     * @return True if the purge operation encountered no errors, false if failures
     * were found. Note that if an entry cannot be removed, that is not an error because
     * the item might be locked since it's in use.
     */
    bool purge() {
        // Lock the cache. Ensure the cache is unlocked no matter how we exit
        CacheLock lock(d_cache_info_fd);
        if (!lock.lock_the_cache(LOCK_EX, "locking the cache in purge()."))
            return false;

        uint64_t ci_size = get_cache_info_size();
        if (ci_size < d_max_cache_size_in_bytes)
            return true;

        struct item_info {
            std::string d_name;
            off_t d_size;
            item_info(std::string name, off_t size) : d_name(std::move(name)), d_size(size) {}
        };

        // sorted by access time, with the oldest time first
        std::multimap<unsigned long, struct item_info, std::less<>> items;
        uint64_t total_size = 0; // for a sanity check. jhrg 11/03/23

        std::vector<std::string> files;
        if (!files_in_cache(files))
            return false;

        for (const auto &file : files) {
            struct stat sb = {};

            if (stat(file.c_str(), &sb) < 0) {
                ERROR("Error getting info on " + file + " in purge() - " + get_errno());
                return false;
            }

            items.insert(std::pair<unsigned long, item_info>(sb.st_atime, item_info(file, sb.st_size)));
            total_size += sb.st_size; // sanity check; remove some day? jhrg 11/03/23
        }

        if (ci_size != total_size) {
            ERROR("Error cache_info and the measured size of items differ by " + std::to_string(total_size) +
                  " bytes.");
        }

        // choose which files to remove - since the 'items' map orders the things by time, use that ordering
        uint64_t removed_bytes = 0;
        for (const auto &item : items) {
            if (removed_bytes > d_purge_size)
                break;

            // Get a non-blocking but exclusive lock on the item before deleting. If the code
            // cannot get that lock, move on to the next item. jhrg 11/06/23
            int fd = open(item.second.d_name.c_str(), O_WRONLY, 0666);
            if (fd < 0) {
                ERROR("Error opening the cache item in purge() for: " + item.second.d_name + " " + get_errno());
                return false;
            }
            Item item_lock(fd); // The Item dtor is called on every loop iteration according to Google. jhrg 11/03/23
            if (!item_lock.lock_the_item(LOCK_EX | LOCK_NB,
                                         "locking the cache item in purge() for: " + item.second.d_name))
                continue;

            if (remove(item.second.d_name.c_str()) != 0) {
                ERROR("Error removing " + item.second.d_name + " from cache directory in purge() - " + get_errno());
                // but keep going; this is a soft error
            } else {
                // but only count the bytes if they are actually removed
                removed_bytes += item.second.d_size;
            }
        }

        // update the cache info file
        if (!update_cache_info_size(ci_size - removed_bytes)) {
            ERROR("Error updating the cache_info size in purge() - " + get_errno());
            return false;
        }

        return true;
    }
};

#endif // FileCache_h_
