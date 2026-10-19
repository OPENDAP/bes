# Deep Dive: `http/` Library — Simplification and `url` Class Analysis

## Date: 2026-06-12

---

## 1. Inventory of `http/` Classes and Responsibilities

| Class / File | Type | Responsibility |
|---|---|---|
| `url` (url_impl.h/cc) | Base class | Parse URLs into components; hold parsed state and metadata |
| `EffectiveUrl` (EffectiveUrl.h/cc) | Subclass of `url` | URL after HTTP redirects; adds response-header parsing for cache-control expiry |
| `EffectiveUrlCache` (EffectiveUrlCache.h/cc) | Singleton | Maps source URL strings → cached `EffectiveUrl` objects; expiry-aware |
| `RemoteResource` (RemoteResource.h/cc) | Value class | Downloads HTTP/file resources to temp files; owns a `shared_ptr<url>` |
| `AllowedHosts` (AllowedHosts.h/cc) | Singleton | ACL: checks URLs against allowed-host pattern list |
| `CredentialsManager` (CredentialsManager.h/cc) | Singleton | Maps URL patterns → AWS credentials |
| `AccessCredentials` (AccessCredentials.h/cc) | Data bag | Key-value store for AWS id/secret/region/url |
| `ProxyConfig` (ProxyConfig.h/cc) | Singleton | Proxy host/port/auth from BES config |
| `CurlUtils` (CurlUtils.h/cc) | Free functions | HTTP GET/HEAD, redirect following, S3 signing, curl handle init |
| `awsv4` (awsv4.h/cc) | Free functions | AWS Signature V4 computation |
| `HttpError` (HttpError.h/cc) | Exception class | Wraps curl errors and HTTP status, stores origin URL and response body |
| `HttpUtils` (HttpUtils.h/cc) | Free functions | MIME type detection, URL encoding, config key parsing |
| `HttpNames.h` | Constants | ~30 `#define` constants for protocol names, header keys, defaults |

---

## 2. The `url` Class in Detail

### Fields (all private)

```cpp
std::string d_source_url_str;                        // raw input string
std::string d_protocol;                              // "http://", "https://", "file://"
std::string d_host;                                  // hostname, lowercased
std::string d_path;                                  // path component
std::string d_query;                                 // raw query string
kvp_map_t d_query_kvp;                              // map<string, vector<string>>
std::chrono::system_clock::time_point d_ingest_time; // creation timestamp
bool d_trusted;                                      // security/auth context flag
```

### Constructors

1. Default: `url()` — unused in production; exists for container storage.
2. String: `url(std::string url_s, bool trusted = false)` — the only production path; calls `parse()` immediately.
3. Copy: `url(const http::url &src_url) = default`.
4. From `shared_ptr` (two overloads) — **marked TODO for removal (jhrg 2/20/25)**; do a deep-copy of all fields from the pointed-to object.

### Methods (all virtual)

```
str()                          full URL string
protocol() / host() / path() / query()    components
get_url_no_query()             protocol + host + path
ingest_time()                  time_t from d_ingest_time
set_ingest_time()              the only setter
query_parameter_value(key)     first KVP value or ""
query_parameter_values(key)    vector of values (throws if absent)
query_parameter_values_size(key) count of values
is_expired()                   checks AWS X-Amz-Expires KVP
is_trusted()                   returns d_trusted
dump()                         debug string
```

Assignment operator is `= delete`. All methods are virtual.

### `EffectiveUrl` — the only subclass

Overrides only three members: `is_expired()` (uses Cache-Control headers), `dump()`, and the destructor. Everything else is inherited unchanged.

---

## 3. Method Usage Frequency Across All Call Sites

Based on grep across `http/`, `modules/dmrpp_module/`, and `modules/cmr_module/`:

| Method | Approx. call sites | Key callers |
|---|---|---|
| `str()` / `->str()` | ~50 | CurlUtils, EffectiveUrlCache, RemoteResource, AllowedHosts |
| `protocol()` | ~10 | RemoteResource, AllowedHosts |
| `host()` | ~8 | awsv4, AllowedHosts, CredentialsManager |
| `path()` | ~15 | CurlUtils, awsv4, RemoteResource, AllowedHosts |
| `query()` | ~5 | awsv4, AllowedHosts |
| `is_trusted()` | ~5 | CurlUtils, EffectiveUrlCache |
| `query_parameter_value()` | ~10 | CurlUtils (in `is_expired`), unit tests |
| `query_parameter_values()` | ~8 | unit tests primarily |
| `is_expired()` | ~3 | EffectiveUrlCache, EffectiveUrl itself |
| `ingest_time()` | ~2 | EffectiveUrl override, EffectiveUrlCache |
| `get_url_no_query()` | ~1 | CurlUtils (single profiling call) |
| `query_parameter_values_size()` | ~0 | Unit tests only |
| `set_ingest_time()` | ~0 | Unit tests only |

Five methods account for >90% of production use: `str()`, `path()`, `host()`, `protocol()`, `query()`.

---

## 4. Redundancy Identified

### A. Duplicate method overloads: `shared_ptr` vs. value

This pattern appears in four different classes, all carrying TODO comments for removal:

**AllowedHosts** — 4 overloads (2 redundant):
```cpp
bool is_allowed(const http::url &candidate_url);
bool is_allowed(const http::url &candidate_url, std::string &whynot);
bool is_allowed(const std::shared_ptr<http::url> &candidate_url);         // just dereferences
bool is_allowed(const std::shared_ptr<http::url> &candidate_url, std::string &whynot); // just dereferences
```

**awsv4** — 3 overloads (1 already marked for removal):
```cpp
// TODO Remove when we retire the shared_ptr<http::url> interface. jhrg 2/20/25
std::string compute_awsv4_signature(const std::shared_ptr<http::url> &uri, ...);
std::string compute_awsv4_signature(const http::url &uri, ...);  // calls string version
std::string compute_awsv4_signature(const std::string &canonical_uri, ...);  // actual impl
```

**CurlUtils** — 4 overloads (2 marked for removal):
```cpp
// TODO jhrg 2/20/25
curl_slist *sign_s3_url(const std::string &target_url, ...);
curl_slist *sign_s3_url(const std::shared_ptr<http::url> &target_url, ...);

bool is_url_signed_for_s3(const std::string &url);
bool is_url_signed_for_s3(const std::shared_ptr<http::url> &target_url);
```

### B. `url` constructors that copy from `shared_ptr`

Two constructors exist only to convert `shared_ptr<url>` to a value `url`. This is transitional scaffolding left from the pointer-to-value migration.

### C. Virtual methods with no polymorphic override

Fifteen methods are declared `virtual` in `url`. Only `is_expired()` and `dump()` are overridden in `EffectiveUrl`. The other 13 virtual declarations carry vtable overhead with no runtime benefit.

### D. Dead code in HttpUtils.cc

A `decompose_url()` function (approximately lines 248–305) is wrapped in `#if 0` — a prior URL parsing implementation that was superseded by the `url` class.

---

## 5. Assessment: Value Type, Polymorphic Base, or Data Bag?

**Increasingly a value type.** The class is immutable after construction (assignment deleted, only one setter), copy-constructible, and the majority of call sites use it as a const reference or copy. The `shared_ptr` overloads and the `shared_ptr` constructors are migration scaffolding from an older pointer-passing style.

**Minimally polymorphic.** There is exactly one subclass (`EffectiveUrl`) overriding three members. There is no downcasting, no polymorphic deletion, no use through base-class pointers at runtime in any hot path. The virtual declarations are largely vestigial.

**Not a simple data bag.** The URL parsing in `parse()` and `parse_query_string()` is non-trivial and properly encapsulated. The `is_expired()` logic — checking AWS `X-Amz-Date`/`X-Amz-Expires` query parameters against `ingest_time` — is a real computation, not just field access.

**Conclusion:** The class is in transition. The original design assumed heap-allocated `shared_ptr<url>` everywhere; the current design is pushing toward value semantics but the old API surface is not yet fully cleaned up.

---

## 6. Thread-Safety

The `url` class is effectively immutable: all state is set at construction, the assignment operator is deleted, and `set_ingest_time()` is not called in production. Instances can be safely shared by const reference across threads without synchronization.

The caching and credential layers around it use explicit mutexes:
- `EffectiveUrlCache`: `std::mutex d_cache_lock_mutex`
- `CredentialsManager`: `std::recursive_mutex d_lock_mutex`

No thread-safety concern exists in `url` itself.

---

## 7. Effect of Replacing `url` with Free Functions

### What the class currently provides

1. A parsed representation of a URL string (protocol, host, path, query, KVP map).
2. A `trusted` flag for auth context propagation.
3. An `ingest_time` for expiry computation.
4. The `is_expired()` predicate for AWS-signed URLs.

### What a free-function design would look like

```cpp
namespace http {

struct ParsedUrl {
    std::string raw;
    std::string protocol;
    std::string host;
    std::string path;
    std::string query;
    std::map<std::string, std::vector<std::string>> query_kvp;
    std::chrono::system_clock::time_point ingest_time;
    bool trusted = false;
    // optional: response headers (currently in EffectiveUrl subclass)
    std::vector<std::pair<std::string, std::string>> response_headers;
};

ParsedUrl parse_url(const std::string &url_str, bool trusted = false);
std::string get_url_no_query(const ParsedUrl &u);
bool is_expired(const ParsedUrl &u);
std::string query_parameter_value(const ParsedUrl &u, const std::string &key);
const std::vector<std::string> &query_parameter_values(const ParsedUrl &u, const std::string &key);

} // namespace http
```

`EffectiveUrl` becomes a `ParsedUrl` with the `response_headers` field populated — no subclass needed.

### What changes at call sites

- `url_obj->str()` → `url_obj.raw` (or `url_obj.protocol + url_obj.host + url_obj.path` if only the computed form is wanted — in practice always `raw`)
- `url_obj->path()` → `url_obj.path`
- `AllowedHosts::is_allowed(const url &)` → `AllowedHosts::is_allowed(const ParsedUrl &)`
- `EffectiveUrlCache` stores `map<string, ParsedUrl>` instead of `map<string, shared_ptr<EffectiveUrl>>`
- `RemoteResource` holds `ParsedUrl` by value instead of `shared_ptr<url>`
- `Chunk` (dmrpp_module) holds `ParsedUrl` by value — currently holds `shared_ptr<url>`

### Concrete benefits

| Issue | Current | With free functions |
|---|---|---|
| Virtual method overhead | 15 virtual methods, 1 vtable | 0 |
| Inheritance hierarchy | `url` → `EffectiveUrl` | flat struct |
| Duplicate API surface | 8+ redundant shared_ptr overloads | 0 (struct passed by value/ref) |
| Assignment | deleted | works normally |
| Type naming confusion | `url` vs `EffectiveUrl` vs `shared_ptr<url>` | `ParsedUrl` everywhere |
| Extensibility | requires subclassing | add fields to struct |

### Concrete costs

| Cost | Severity |
|---|---|
| Breaks the internal `http/` API | Moderate — not public ABI |
| ~150 call sites need update | Mechanical, not architectural |
| `dmrpp_module/Chunk` stores URL in many constructors | Requires touching 8+ constructors |
| Test suite must be updated | Low risk — tests are already exercising the semantics, just the type changes |
| `EffectiveUrlCache` return type changes | Contained within cache layer |

The migration is entirely internal to the BES tree. There is no public C ABI or installed header consumers for the `http::url` type.

---

## 8. Recommended Path

The existing TODO comments (dated 2/20/25, jhrg) document the intent to retire the `shared_ptr<url>` interface. That work is incomplete. Two reasonable endpoints exist:

### Option A: Finish the existing refactoring (lower risk)

Remove the items already marked TODO and nothing more:

1. Delete the two `shared_ptr` constructors in `url` (url_impl.h lines 74–95).
2. Delete the `shared_ptr` overloads in `awsv4`, `CurlUtils`, `AllowedHosts`.
3. Update all call sites to use value or const-reference semantics.
4. Remove the `#if 0` dead code block in `HttpUtils.cc`.
5. Drop `virtual` from all getters except `is_expired()` and `dump()`.

This is ~50–80 lines removed, ~30 call sites updated. Low blast radius, reviewable in a single PR.

### Option B: Full refactoring to `ParsedUrl` struct + free functions (higher reward, higher risk)

Adds to Option A:

6. Introduce `ParsedUrl` struct as described above.
7. Migrate `EffectiveUrl` subclass to a populated `ParsedUrl` (no subclass).
8. Update all call sites: `url_obj->str()` → `url_obj.raw`, etc.
9. Delete `url_impl.h`, `url_impl.cc`, `EffectiveUrl.h`, `EffectiveUrl.cc`.
10. Update `Chunk` (dmrpp), `RemoteResource`, `EffectiveUrlCache`, `CurlUtils`, `AllowedHosts`, `CredentialsManager`, `awsv4`.

This is ~200–300 lines changed across 15 files. Should be done in a dedicated PR with focused testing. `make check` in both `http/` and `modules/dmrpp_module/` is the validation gate.

**Recommendation:** Do Option A first to confirm the build and test suite are clean, then proceed to Option B as a separate PR. The design clearly intends to arrive at Option B; the TODO comments are the roadmap.

---

## 9. Module Survey: External Consumers

### `modules/dmrpp_module/`

- `CurlHandlePool.h`: `dmrpp_easy_handle::d_url` is `shared_ptr<http::url>` — would become `ParsedUrl`.
- `Chunk.h`: ~8 constructors accept `shared_ptr<http::url>` — the most mechanical change required.
- Uses `get_data_url()` / `set_data_url()` getters extensively — would become field access.

### `modules/cmr_module/`

- `CmrContainer.cc`: One creation site: `shared_ptr<http::url> target_url(new http::url(granule_url, true));` — trivial to convert.
- Passes URL to `RemoteResource` constructor.

### Other modules (gateway, httpd_catalog, s3_reader)

Each has 1–3 `url` creation sites; all use the string constructor and pass to `RemoteResource`. Conversion is mechanical.

---

## 10. Summary

The `http/url` class is a well-encapsulated but over-engineered URL parser for its actual use. It was originally designed for heap-allocated sharing via `shared_ptr`; the codebase is mid-migration toward value semantics. The virtual method table is unnecessary. The dual `shared_ptr`/value API surface is redundant and partially marked for removal by the author.

Replacing the class with a plain `ParsedUrl` struct and free functions would eliminate the inheritance hierarchy, the vtable, and all duplicate overloads. The impact is confined to the BES tree (no installed public headers for this type) and the conversion is mechanical. The five most-used methods (`str/raw`, `path`, `host`, `protocol`, `query`) become direct field access.

The safest approach is two PRs: first clean up the TODOs (Option A), then do the struct conversion (Option B).
