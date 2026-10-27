// RedisCache.h

// This file was originally part of bes, A C++ back-end server
// implementation framework for the OPeNDAP Data Access Protocol.
// Copied to libdap. This is used to cache responses built from
// functional CE expressions.

// Copyright (c) 2026 OPeNDAP, Inc
// Author: Kodi Neumiller <kneumiller@opendap.org>
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

#ifndef RedisCache_h_
#define RedisCache_h_ 1

#include <chrono>
#include <cstdint>
#include <memory>
#include <string>

#include <hiredis/hiredis.h>

#include "BESLog.h"
#include "BESUtil.h"

// Make all the error log messages uniform in one small way. This is taken from
// FileCache.h and done so that we can switch to exceptions if that seems necessary. kln 09/24/26
#define ERROR(msg) ERROR_LOG("RedisCache: " + std::string(msg))
#define INFO(msg) INFO_LOG("RedisCache: " + std::string(msg))

/* RedisCache is designed to be called in place of FileCache
 * which means some variables and calls may share the same name so that they can be interchangeable */
class RedisCache {
private:
    // pathname of the cache directory
    std::string d_cache_dir;

    /// How many bytes can the cache hold before we have to purge?
    /// A value of zero indicates a cache of unlimited size.
    unsigned long long d_max_cache_size_in_bytes = 0;

    // When we purge, how much should we throw away. Set in the ctor to 80% of the max size.
    unsigned long long d_purge_size = 0;

    // Variables specific to RedisCache
    std::string d_redis_host;
    int d_redis_port = 6379;
    int d_redis_db = 0;
    std::string d_namespace;
    redisContext *d_redis = nullptr;

    // Redis key helpers
    std::string keyset_key() const {
        return d_namespace + ":keys:set";
    }

    std::string lru_key() const {
        return d_namespace + ":idx:lru";
    }

    std::string sizes_key() const {
        return d_namespace + ":idx:size";
    }

    std::string total_key() const {
        return d_namespace + ":idx:total";
    }

    std::string write_lock_key(const std::string &key) const {
        return d_namespace + ":lock:write:" + key;
    }

    std::string read_lock_key(const std::string &key) const {
        return d_namespace + ":lock:read:" + key;
    }

    std::string eviction_lock_key(const std::string &key) const {
        return d_namespace + ":lock:evict:" + key;
    }

    bool connect_redis();

    void disconnect_redis();

    bool select_db();

    uint64_t get_cache_size() const;

    bool update_lru(const std::string &key);

    bool record_item(const std::string &key, uint64_t size);

    bool remove_item_metadata(const std::string &key);

    bool acquire_write_lock(
            const std::string &key,
            std::chrono::milliseconds timeout);

    void release_write_lock(const std::string &key);

    bool acquire_read_lock(
            const std::string &key,
            std::chrono::milliseconds timeout);

    void release_read_lock(const std::string &key);

    bool acquire_eviction_lock(
            const std::string &key,
            std::chrono::milliseconds timeout);

    void release_eviction_lock(const std::string &key);

    static uint64_t get_file_size(int fd);

    friend class RedisCacheTest;

public:
    RedisCache() = default;

    RedisCache(const RedisCache &) = delete;
    RedisCache &operator=(const RedisCache &) = delete;

    virtual ~RedisCache();

    /* Represents an open cached item.
     * The item keeps the file descriptor open and releases the
     * distributed read lock when it goes out of scope. */
    class Item {
    private:
        int d_fd = -1;

        RedisCache *d_cache = nullptr;
        std::string d_key;
        bool d_locked = false;

    public:
        Item() = default;

        Item(const Item &) = delete;
        Item &operator=(const Item &) = delete;

        Item(
                RedisCache *cache,
                std::string key,
                int fd)
                : d_fd(fd),
                d_cache(cache),
                d_key(std::move(key)),
                d_locked(true) {}

                virtual ~Item();

        int get_fd() const {
            return d_fd;
        }

        void set_fd(int fd) {
            d_fd = fd;
        }

        void set_lock(RedisCache *cache, const std::string &key)
        {
            d_cache = cache;
            d_key = key;
            d_locked = true;
        }
    };

    /* Keeps the Redis write lock until the object goes out of scope.
     * Once writing completes, metadata such as size and LRU information
     * can be updated. */
    class PutItem {
    private:
        int d_fd = -1;

        RedisCache *d_cache = nullptr;
        std::string d_key;
        bool d_locked = false;

    public:
        PutItem() = default;

        PutItem(const PutItem &) = delete;
        PutItem &operator=(const PutItem &) = delete;

        virtual ~PutItem();

        int get_fd() const {
            return d_fd;
        }

        void set_fd(int fd) {
            d_fd = fd;
        }

        void set_lock(RedisCache *cache, const std::string &key)
        {
            d_cache = cache;
            d_key = key;
            d_locked = true;
        }
    };

    /**
     * @brief Initialize the Redis-backed file cache.
     *
     * @param cache_dir: Shared filesystem directory, typically EFS.
     * @param redis_host: Redis hostname/private IP.
     * @param redis_port: Redis TCP port, usually 6379.
     * @param redis_db: Redis logical database.
     * @param name_space: Prefix used for all Redis keys.
     * @param size: Maximum cache size in bytes. Zero means unlimited.
     * @param purge_size: Target cache size after purge.
     */
    virtual bool initialize(
            const std::string &cache_dir,
            const std::string &redis_host,
            int redis_port,
            int redis_db,
            const std::string &name_space,
            long long size,
            long long purge_size);

    // Copy file_name into the cache under key.
    bool put(const std::string &key, const std::string &file_name);

    // Store the supplied data under key.
    bool put_data(const std::string &key, const std::string &data);

    // Create a new cache file and return it locked for writing.
    bool put(const std::string &key, PutItem &item);

    // Get a cache item and hold a Redis-backed shared/read lock
    // until Item is destroyed.
    bool get(const std::string &key, Item &item);

    // Delete an item if it is not currently being read/written.
    bool del(const std::string &key, bool block = false);

    //Remove all cached files and Redis metadata for this namespace.
    bool clear();

    //Purge least-recently-used entries until the target size is reached.
    bool purge();

    // Return current Redis-tracked cache size.
    uint64_t size() const {
        return get_cache_size();
    }

    const std::string &cache_dir() const {
        return d_cache_dir;
    }

    const std::string &redis_host() const {
        return d_redis_host;
    }

    const std::string &name_space() const {
        return d_namespace;
    }

    static std::string hash_key(
            const std::string &key,
            bool log_it = false);
};

#endif //RedisCache_h_
