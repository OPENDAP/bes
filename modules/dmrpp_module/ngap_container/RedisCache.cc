// RedisCache.cc

// This file was originally part of bes, A C++ back-end server
// implementation framework for the OPeNDAP Data Access Protocol.

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

/*
 * Cached items are files in a shared directory (e.g., EFS), named by the hashed key,
 * as in FileCache. The sizes, LRU times and locks that FileCache keeps in flock(2)
 * and the cache_info file are kept in Redis so hosts can share the cache. An item is
 * in the cache only once its size is recorded in Redis.
 */

#include "config.h"

#include <cerrno>
#include <cstring>
#include <functional>
#include <iomanip>
#include <sstream>
#include <thread>
#include <vector>

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <hiredis/hiredis.h>
#include <openssl/sha.h>

#include "RedisCache.h"

using namespace std::chrono;

namespace {

    constexpr milliseconds LOCK_TIMEOUT{60'000};    // Locks expire after LOCK_TIMEOUT to ensure process don't hold up the cache
    constexpr milliseconds READ_LOCK_WAIT{500};     // How long get() waits for a writer to finish
    constexpr milliseconds LOCK_POLL_INTERVAL{10};
    constexpr int REDIS_TIMEOUT_SECONDS = 2;
    constexpr long long PURGE_BATCH_SIZE = 100;

    std::string errno_str() {
        const char *s_err = strerror(errno);
        return s_err ? s_err : "unknown error";
    }

    /// Identifies this process in Redis lock values. Computed on each call so it is correct after fork().
    std::string process_token() {
        char host[256] = {};
        gethostname(host, sizeof(host) - 1);
        return std::string(host) + ":" + std::to_string(getpid());
    }

    std::string now_ms() {
        return std::to_string(duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count());
    }

    /// Run a function when the enclosing scope exits.
    class ScopeExit {
        std::function<void()> d_fn;

    public:
        explicit ScopeExit(std::function<void()> fn) : d_fn(std::move(fn)) {}
        ScopeExit(const ScopeExit &) = delete;
        ScopeExit &operator=(const ScopeExit &) = delete;
        ~ScopeExit() { d_fn(); }
    };

    struct ReplyDeleter {
        void operator()(redisReply *r) const { freeReplyObject(r); }
    };

    using Reply = std::unique_ptr<redisReply, ReplyDeleter>;

    /// Run a Redis command. Returns null on a connection error
    Reply command(redisContext *c, const std::vector<std::string> &parts) {
        if (!c)
            return nullptr;

        std::vector<const char *> argv;
        std::vector<size_t> argvlen;
        for (const auto &p : parts) {
            argv.push_back(p.data());
            argvlen.push_back(p.size());
        }

        Reply reply(static_cast<redisReply *>(
                redisCommandArgv(c, static_cast<int>(argv.size()), argv.data(), argvlen.data())));
        if (!reply)
            REDIS_ERROR("Redis command " + parts[0] + " failed: " + std::string(c->errstr));
        else if (reply->type == REDIS_REPLY_ERROR)
            REDIS_ERROR("Redis command " + parts[0] + " returned an error: " + std::string(reply->str, reply->len));

        return reply;
    }

    /// Run a Lua script; return true if it returned 1.
    bool eval_ok(redisContext *c, const std::string &script, const std::vector<std::string> &keys,
                 const std::vector<std::string> &args) {
        std::vector<std::string> parts{"EVAL", script, std::to_string(keys.size())};
        parts.insert(parts.end(), keys.begin(), keys.end());
        parts.insert(parts.end(), args.begin(), args.end());

        auto reply = command(c, parts);
        return reply && reply->type == REDIS_REPLY_INTEGER && reply->integer == 1;
    }

    /// Convert a reply holding a number to a uint64_t
    uint64_t reply_to_u64(const redisReply *reply) {
        if (!reply)
            return 0;
        if (reply->type == REDIS_REPLY_INTEGER)
            return reply->integer > 0 ? reply->integer : 0;
        if (reply->type == REDIS_REPLY_STRING) {
            try {
                long long v = std::stoll(std::string(reply->str, reply->len));
                return v > 0 ? v : 0;
            } catch (const std::exception &) {
                return 0;
            }
        }
        return 0;
    }

    /// Call 'attempt' until it returns true or 'timeout' passes. A zero timeout means try once.
    bool retry_until(milliseconds timeout, const std::function<bool()> &attempt) {
        const auto deadline = steady_clock::now() + timeout;
        while (!attempt()) {
            if (steady_clock::now() >= deadline)
                return false;
            std::this_thread::sleep_for(LOCK_POLL_INTERVAL);
        }
        return true;
    }

    // The lock scripts must be Lua so that 'check then lock' is atomic

    // Shared lock - fails if the item is being written or evicted
    // KEYS: write lock, eviction lock, reader zset
    // ARGV: token, expiry time (ms)
    const std::string READ_LOCK_SCRIPT = R"lua(
        if redis.call('EXISTS', KEYS[1]) == 1 or redis.call('EXISTS', KEYS[2]) == 1 then return 0 end
        redis.call('ZADD', KEYS[3], ARGV[2], ARGV[1])
        return 1
    )lua";

    // Exclusive (write or eviction) lock: fails if there are live readers or the other exclusive lock is held
    // KEYS: lock to take, other exclusive lock, reader zset
    // ARGV: token, timeout (ms), now (ms)
    const std::string EXCLUSIVE_LOCK_SCRIPT = R"lua(
        redis.call('ZREMRANGEBYSCORE', KEYS[3], '-inf', ARGV[3])
        if redis.call('ZCARD', KEYS[3]) > 0 or redis.call('EXISTS', KEYS[2]) == 1 then return 0 end
        if redis.call('SET', KEYS[1], ARGV[1], 'NX', 'PX', ARGV[2]) then return 1 end
        return 0
    )lua";

    // Delete a lock only if this process holds it
    // KEYS: lock
    // ARGV: token
    const std::string UNLOCK_SCRIPT = R"lua(
        if redis.call('GET', KEYS[1]) == ARGV[1] then return redis.call('DEL', KEYS[1]) end
        return 0
    )lua";

} // namespace

RedisCache::Item::~Item() {
    if (d_fd != -1)
        close(d_fd);
    if (d_locked && d_cache)
        d_cache->release_read_lock(d_key);
}

RedisCache::~RedisCache() {
    disconnect_redis();
}

/// Make sure there is a working connection. Reconnects if it is broken or was inherited across fork().
bool RedisCache::connect_redis() {
    if (d_redis && (d_redis_pid != getpid() || d_redis->err))
        disconnect_redis();

    if (d_redis)
        return true;

    if (d_redis_host.empty()) {
        REDIS_ERROR("connect_redis() called before initialize().");
        return false;
    }

    const timeval timeout = {REDIS_TIMEOUT_SECONDS, 0};
    d_redis = redisConnectWithTimeout(d_redis_host.c_str(), d_redis_port, timeout);
    if (!d_redis || d_redis->err) {
        REDIS_ERROR("Could not connect to Redis at " + d_redis_host + ":" + std::to_string(d_redis_port) + " - " +
        (d_redis ? std::string(d_redis->errstr) : "could not allocate the context"));
        disconnect_redis();
        return false;
    }

    redisSetTimeout(d_redis, timeout);
    d_redis_pid = getpid();

    if (!select_db()) {
        disconnect_redis();
        return false;
    }

    return true;
}

void RedisCache::disconnect_redis() {
    if (d_redis)
        redisFree(d_redis);
    d_redis = nullptr;
    d_redis_pid = -1;
}

bool RedisCache::select_db() {
    auto reply = command(d_redis, {"SELECT", std::to_string(d_redis_db)});
    return reply && reply->type != REDIS_REPLY_ERROR;
}

uint64_t RedisCache::get_cache_size() const {
    // May need to reconnect. Should not change the logical state of the cache.
    if (!const_cast<RedisCache *>(this)->connect_redis())
        return 0;
    auto reply = command(d_redis, {"GET", total_key()});
    return reply_to_u64(reply.get());
}

bool RedisCache::update_lru(const std::string &key) {
    // XX so that an item evicted meanwhile is not put back in the index.
    auto reply = command(d_redis, {"ZADD", lru_key(), "XX", now_ms(), key});
    return reply && reply->type != REDIS_REPLY_ERROR;
}

/**
 * Redis command definitions:
 * HSET/HGET/HDEL/HEXISTS: Set/read/delete/check a field in a hash (field -> value)
 * SADD/SREM/SMEMBERS:     Add to/remove from/list a set (unique strings, no order)
 * ZADD/ZREM:              Add or update/remove a member of a sorted set (sorted by score)
 * MULTI/EXEC:             Queue commands, then run them together as one transaction with nothing else in between
 * INCRBY/DECRBY:	       Add to/subtract from a number stored as a string
 * XX:                     An option to ZADD meaning “only update members that already exist, never add new ones.”
 */

// Record a new item. The caller holds the item's write lock.
bool RedisCache::record_item(const std::string &key, uint64_t size) {
    const std::string sz = std::to_string(size);
    auto reply = command(d_redis, {"MULTI"});
    command(d_redis, {"HSET", sizes_key(), key, sz});
    command(d_redis, {"SADD", keyset_key(), key});
    command(d_redis, {"ZADD", lru_key(), now_ms(), key});
    command(d_redis, {"INCRBY", total_key(), sz});
    reply = command(d_redis, {"EXEC"});
    return reply && reply->type == REDIS_REPLY_ARRAY;
}

// Remove an item's metadata. The caller holds the item's eviction lock.
bool RedisCache::remove_item_metadata(const std::string &key) {
    auto size = command(d_redis, {"HGET", sizes_key(), key});
    if (!size || size->type == REDIS_REPLY_ERROR)
        return false;

    command(d_redis, {"MULTI"});
    command(d_redis, {"DECRBY", total_key(), std::to_string(reply_to_u64(size.get()))});
    command(d_redis, {"HDEL", sizes_key(), key});
    command(d_redis, {"ZREM", lru_key(), key});
    command(d_redis, {"SREM", keyset_key(), key});
    auto reply = command(d_redis, {"EXEC"});
    return reply && reply->type == REDIS_REPLY_ARRAY;
}

bool RedisCache::acquire_write_lock(const std::string &key, milliseconds timeout) {
    const std::string token = process_token();
    return retry_until(timeout, [&]() {
        return eval_ok(d_redis, EXCLUSIVE_LOCK_SCRIPT, {write_lock_key(key), eviction_lock_key(key), read_lock_key(key)},
                       {token, std::to_string(LOCK_TIMEOUT.count()), now_ms()});
    });
}

void RedisCache::release_write_lock(const std::string &key) {
    if (connect_redis())
        eval_ok(d_redis, UNLOCK_SCRIPT, {write_lock_key(key)}, {process_token()});
}

bool RedisCache::acquire_read_lock(const std::string &key, milliseconds timeout) {
    const std::string token = process_token();
    return retry_until(timeout, [&]() {
        const std::string expires = std::to_string(
                duration_cast<milliseconds>((system_clock::now() + LOCK_TIMEOUT).time_since_epoch()).count());
        return eval_ok(d_redis, READ_LOCK_SCRIPT, {write_lock_key(key), eviction_lock_key(key), read_lock_key(key)},
                       {token, expires});
    });
}

void RedisCache::release_read_lock(const std::string &key) {
    if (connect_redis())
        command(d_redis, {"ZREM", read_lock_key(key), process_token()});
}

bool RedisCache::acquire_eviction_lock(const std::string &key, milliseconds timeout) {
    const std::string token = process_token();
    return retry_until(timeout, [&]() {
        return eval_ok(d_redis, EXCLUSIVE_LOCK_SCRIPT, {eviction_lock_key(key), write_lock_key(key), read_lock_key(key)},
                       {token, std::to_string(LOCK_TIMEOUT.count()), now_ms()});
    });
}

void RedisCache::release_eviction_lock(const std::string &key) {
    if (connect_redis())
        eval_ok(d_redis, UNLOCK_SCRIPT, {eviction_lock_key(key)}, {process_token()});
}

bool RedisCache::initialize(const std::string &cache_dir, const std::string &redis_host, int redis_port,
                            int redis_db, const std::string &name_space, long long size, long long purge_size) {
    if (size < 0 || purge_size < 0) {
        ERROR_LOG("RedisCache::initialize() - size and purge_size must be >= 0\n");
        return false;
    }

    struct stat sb = {};
    if (stat(cache_dir.c_str(), &sb) != 0) {
        BESUtil::mkdir_p(cache_dir, 0775);
        if (stat(cache_dir.c_str(), &sb) != 0) {
            ERROR_LOG("RedisCache::initialize() - could not stat the cache directory: " + cache_dir);
            return false;
        }
    }

    // NgapOwnedContainer calls initialize() for every container, so only reconnect if the server changed.
    if (d_redis && (redis_host != d_redis_host || redis_port != d_redis_port || redis_db != d_redis_db))
        disconnect_redis();

    d_cache_dir = cache_dir;
    d_redis_host = redis_host;
    d_redis_port = redis_port;
    d_redis_db = redis_db;
    d_namespace = name_space;
    d_max_cache_size_in_bytes = (unsigned long long)size;
    d_purge_size = (unsigned long long)purge_size;

    return connect_redis();
}

/**
 * @brief Put data in the cache under the given key.
 * @return True if the data are cached, false if the key is already cached,
 * another process is writing it, or there was an error.
 */
bool RedisCache::put_data(const std::string &key, const std::string &data) {
    if (!connect_redis())
        return false;

    if (!acquire_write_lock(key, milliseconds(0)))
        return false;
    ScopeExit unlock([this, &key]() { release_write_lock(key); });

    auto exists = command(d_redis, {"HEXISTS", sizes_key(), key});
    if (!exists || exists->type != REDIS_REPLY_INTEGER || exists->integer == 1)
        return false;

    // Write a temporary file and rename it so that a partial file is never seen.
    const std::string item_name = BESUtil::pathConcat(d_cache_dir, key);
    const std::string tmp_name = item_name + ".tmp." + process_token();
    int fd = open(tmp_name.c_str(), O_CREAT | O_TRUNC | O_WRONLY, 0666);
    if (fd < 0) {
        REDIS_ERROR("Error creating key/file: " + key + " " + errno_str());
        return false;
    }

    const ssize_t bytes_written = write(fd, data.data(), data.size());
    const bool ok = bytes_written >= 0 && static_cast<std::string::size_type>(bytes_written) == data.size() &&
            fsync(fd) == 0;
    close(fd);

    if (!ok || rename(tmp_name.c_str(), item_name.c_str()) != 0) {
        REDIS_ERROR("Error writing data to cache file: " + key + " " + errno_str());
        unlink(tmp_name.c_str());
        return false;
    }

    return record_item(key, data.size());
}

/**
 * @brief Get a read-locked item from the cache.
 * The read lock is released when the Item goes out of scope.
 * @return True if the item was found and locked, false otherwise
 */
bool RedisCache::get(const std::string &key, Item &item) {
    if (!connect_redis())
        return false;

    if (!acquire_read_lock(key, READ_LOCK_WAIT))
        return false;
    bool handed_off = false;
    ScopeExit unlock([this, &key, &handed_off]() {
        if (!handed_off)
            release_read_lock(key);
    });

    auto exists = command(d_redis, {"HEXISTS", sizes_key(), key});
    if (!exists || exists->type != REDIS_REPLY_INTEGER || exists->integer == 0)
        return false;

    int fd = open(BESUtil::pathConcat(d_cache_dir, key).c_str(), O_RDONLY);
    if (fd < 0) {
        REDIS_ERROR("Error opening the cache item in get for: " + key + " " + errno_str());
        return false;
    }

    update_lru(key);

    item.set_fd(fd);
    item.set_lock(this, key);
    handed_off = true;
    return true;
}

/**
 * @brief Remove all cached files and the Redis metadata for this namespace.
 * @note Like FileCache::clear(), this does not wait for readers or writers.
 */
bool RedisCache::clear() {
    if (!connect_redis())
        return false;

    auto members = command(d_redis, {"SMEMBERS", keyset_key()});
    if (!members || members->type != REDIS_REPLY_ARRAY)
        return false;

    bool ok = true;
    for (size_t i = 0; i < members->elements; ++i) {
        const std::string key(members->element[i]->str, members->element[i]->len);
        if (unlink(BESUtil::pathConcat(d_cache_dir, key).c_str()) != 0 && errno != ENOENT) {
            REDIS_ERROR("Error removing " + key + " from cache directory (" + d_cache_dir + ") - " + errno_str());
            ok = false;
        }
    }

    auto reply = command(d_redis, {"DEL", keyset_key(), lru_key(), sizes_key(), total_key()});
    return ok && reply && reply->type != REDIS_REPLY_ERROR;
}

/**
 * @brief Purge the least recently used items from the cache.
 * As with FileCache, the caller decides when to call purge(). Nothing happens until the
 * cache reaches its maximum size; then items are removed, oldest first, until more than
 * d_purge_size bytes are gone. Items in use are skipped. Only one process purges at a time.
 * @return True if the purge operation encountered no errors, false otherwise.
 */
bool RedisCache::purge() {
    if (d_max_cache_size_in_bytes == 0 || get_cache_size() < d_max_cache_size_in_bytes)
        return true;

    const std::string purge_lock = d_namespace + ":lock:purge";
    const std::string token = process_token();
    auto lock = command(d_redis, {"SET", purge_lock, token, "NX", "PX", std::to_string(LOCK_TIMEOUT.count())});
    if (!lock || lock->type == REDIS_REPLY_ERROR)
        return false;
    if (lock->type == REDIS_REPLY_NIL) // another process is purging
        return true;
    ScopeExit unlock([this, &purge_lock, &token]() { eval_ok(d_redis, UNLOCK_SCRIPT, {purge_lock}, {token}); });

    uint64_t removed_bytes = 0;
    long long skipped = 0; // items left in the LRU index ahead of the ones not yet examined
    while (removed_bytes <= d_purge_size) {
        auto batch = command(d_redis, {"ZRANGE", lru_key(), std::to_string(skipped),
                                       std::to_string(skipped + PURGE_BATCH_SIZE - 1)});
        if (!batch || batch->type != REDIS_REPLY_ARRAY)
            return false;
        if (batch->elements == 0)
            break;

        for (size_t i = 0; i < batch->elements && removed_bytes <= d_purge_size; ++i) {
            const std::string key(batch->element[i]->str, batch->element[i]->len);

            if (!acquire_eviction_lock(key, milliseconds(0))) { // in use
                ++skipped;
                continue;
            }
            ScopeExit unlock_item([this, &key]() { release_eviction_lock(key); });

            auto size = command(d_redis, {"HGET", sizes_key(), key});
            if (unlink(BESUtil::pathConcat(d_cache_dir, key).c_str()) != 0 && errno != ENOENT) {
                REDIS_ERROR("Error removing " + key + " from cache directory in purge() - " + errno_str());
                ++skipped; // soft error; keep going
                continue;
            }

            if (!remove_item_metadata(key))
                return false;
            removed_bytes += reply_to_u64(size.get());
        }
    }

    return true;
}

/// Return a SHA256 hash of the key. This is the same hash FileCache::hash_key() uses.
std::string RedisCache::hash_key(const std::string &key, bool log_it) {
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
