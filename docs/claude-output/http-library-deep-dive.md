# Deep Dive: `http/` Library — Full Analysis
## Redundancy, Simplification, and Test Coverage

**Date:** 2026-06-12

---

## 1. Library Overview

The `http/` directory builds `libbes_http.la`, a support library for the BES server and its modules. It provides URL parsing, remote resource retrieval via libcurl, AWS S3 signing, an effective-URL redirect cache, access control, proxy configuration, and credential management.

Thirteen source files, thirteen headers. Summary:

| File pair | Responsibility |
|---|---|
| `url_impl` | Parse URL string into components; hold trusted flag and ingest time |
| `EffectiveUrl` | Subclass of `url`; adds response headers and Cache-Control expiry |
| `EffectiveUrlCache` | Singleton; caches source URL → EffectiveUrl after redirect chain |
| `RemoteResource` | Downloads HTTP/file resources to temp files |
| `CurlUtils` | libcurl HTTP GET/HEAD, redirect following, S3 signing, retry logic |
| `awsv4` | AWS Signature V4 computation |
| `AllowedHosts` | Singleton; ACL via regex patterns from config |
| `CredentialsManager` | Singleton; maps URL patterns to AWS credentials |
| `AccessCredentials` | KVP store for AWS id/key/region/url |
| `ProxyConfig` | Singleton; proxy host/port/auth from config |
| `HttpError` | Exception class wrapping curl errors and HTTP status |
| `HttpUtils` | Free functions: MIME type detection, URL encoding |
| `HttpNames.h` | ~30 `#define` constants |

---

## 2. Redundancies Catalogue

### 2.1 Dual `shared_ptr` / value API surface (in-progress migration)

Four locations carry paired overloads where the `shared_ptr` version just dereferences and calls the value version. All are marked TODO for removal (jhrg 2/20/25):

**`AllowedHosts.h` / `AllowedHosts.cc`** — 4 methods, 2 are thin wrappers:
```cpp
bool is_allowed(const http::url &candidate_url);
bool is_allowed(const http::url &candidate_url, std::string &whynot);
bool is_allowed(const std::shared_ptr<http::url> &candidate_url);         // dereferences
bool is_allowed(const std::shared_ptr<http::url> &candidate_url, std::string &whynot); // dereferences
```

**`CurlUtils.h` / `CurlUtils.cc`** — 4 functions, 2 wrappers:
```cpp
curl_slist *sign_s3_url(const std::string &target_url, ...);
curl_slist *sign_s3_url(const std::shared_ptr<http::url> &target_url, ...);  // calls string version
bool is_url_signed_for_s3(const std::string &url);
bool is_url_signed_for_s3(const std::shared_ptr<http::url> &target_url);     // calls string version
```

**`awsv4.h` / `awsv4.cc`** — 3 overloads, 2 are wrappers around the string version:
```cpp
// TODO Remove. jhrg 2/20/25
std::string compute_awsv4_signature(const std::shared_ptr<http::url> &uri, ...);
std::string compute_awsv4_signature(const http::url &uri, ...);  // calls string version
std::string compute_awsv4_signature(const std::string &canonical_uri, ...);  // real implementation
```

**`url_impl.h`** — 2 constructors that deep-copy from a `shared_ptr`, marked TODO:
```cpp
// TODO Remove these shared_ptr methods if possible. jhrg 2/20/25
explicit url(const std::shared_ptr<http::url> &source_url);
url(const std::shared_ptr<http::url> &source_url, bool trusted);
```

**Remedy:** This work is already planned. Remove the eight identified wrappers/constructors and update ~30 call sites (all in-tree). See section 7 for the full migration path.

---

### 2.2 Dead function declarations in `CurlUtils.h`

Two functions are declared but have no implementation in `CurlUtils.cc` or anywhere in the active codebase:

```cpp
void http_get(const std::string &target_url, std::vector<char> &buf);
bool http_head(const std::string &target_url, int tries = 3, unsigned long wait_time_us = 1'000'000);
```

The only implementations found are in `retired/http/http_head.cc`. The only call sites are also in `retired/`. These declarations will cause a linker error if ever called. They are orphaned declarations from retired code.

**Remedy:** Delete both declarations from `CurlUtils.h`.

---

### 2.3 Dead `#if 0` blocks

**`HttpUtils.cc` lines 248–307:** `decompose_url()` — a prior URL-parsing function superseded by the `url` class. Approximately 60 lines. Never compiled, never called.

**`RemoteResource.cc` lines 255–260:** A `lseek` call moved into `curl::super_easy_perform()`. Six lines with a comment explaining the move.

**Remedy:** Delete both blocks.

---

### 2.4 Duplicate `#define` in `HttpNames.h`

`HTTP_URL_BASE_KEY` is defined twice on consecutive lines:

```
// HttpNames.h:57  #define HTTP_TARGET_URL_KEY "target_url"
// HttpNames.h:58  #define HTTP_URL_BASE_KEY "url_base"      ← first definition
// HttpNames.h:59  #define HTTP_QUERY_STRING_KEY "query_string"
// HttpNames.h:60  #define HTTP_URL_BASE_KEY "url_base"      ← duplicate
// HttpNames.h:61  #define HTTP_INGEST_TIME_KEY "ingest_time"
```

Not a compile error (identical values), but a clear copy-paste slip.

---

### 2.5 Dead `#define` constants in `HttpNames.h`

Five defines on lines 57–61 (`HTTP_TARGET_URL_KEY`, `HTTP_URL_BASE_KEY`, `HTTP_QUERY_STRING_KEY`, `HTTP_INGEST_TIME_KEY`, and the duplicate `HTTP_URL_BASE_KEY`) are used only inside the `#if 0` dead block in `HttpUtils.cc`. They are never referenced from compiled code.

**Remedy:** Remove these five defines along with the dead `decompose_url()` block.

---

### 2.6 Duplicate `#include` in `HttpUtils.cc`

`BESUtil.h` is included twice:

```cpp
// line 42
#include "BESUtil.h"
// ...
// line 52
#include "BESUtil.h"
```

Header guards prevent harm, but it is noise.

**Remedy:** Remove one of the two includes.

---

### 2.7 `get_type_from_content_type()` rebuilds the MIME list on every call

`HttpUtils.cc` line 151–152:
```cpp
void get_type_from_content_type(const string &ctype, string &type) {
    map<string,string> mime_list;
    load_mime_list_from_keys(mime_list);    // reads TheBESKeys every call
```

`load_mime_list_from_keys()` reads and parses BES config keys on every invocation. The result is never cached. This is called per-resource when determining MIME types.

**Remedy:** Cache the mime list in a `static` local variable populated once on first call.

---

### 2.8 `CurlUtils.cc`: libcurl encoding option branch uses wrong variable

`CurlUtils.cc` lines 445–451:
```cpp
#ifndef CURLOPT_ACCEPT_ENCODING
    res = curl_easy_setopt(ceh, CURLOPT_ENCODING, "");        // uses ceh ✓
    ...
#else
    res = curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, ""); // uses 'curl' ← undefined
    check_setopt_result(res, ...);                             // different helper function
#endif
```

`CURLOPT_ACCEPT_ENCODING` is defined in all current versions of libcurl, so the `#else` branch is taken but references `curl` (not `ceh`) and calls `check_setopt_result` (which doesn't exist in this file). This is dead code in practice but would be a compile error if `CURLOPT_ACCEPT_ENCODING` were ever undefined. The branch also calls a non-existent helper `check_setopt_result` instead of `eval_curl_easy_setopt_result`.

**Remedy:** Simplify to unconditionally use `CURLOPT_ACCEPT_ENCODING` with `ceh` and the correct helper, and delete the `#ifndef` branch.

---

### 2.9 `CurlUtils.cc`: EDL token injection duplicated between `http_get()` and `gru_mk_attempt()`

`http_get()` lines 1261–1271 (inside `#ifdef DEVELOPER`) looks up EDL credentials and adds an Authorization header. `gru_mk_attempt()` lines 1748–1757 does the same thing without the `DEVELOPER` guard. The logic is nearly identical. The `FIXME` comment in `gru_mk_attempt` notes it is "Hackery for DMR++ Ownership POC code."

This means the feature is only active in `http_get()` when compiled with `DEVELOPER` but always active in the redirect-URL path. The inconsistency is a latent behavior difference.

---

### 2.10 `CurlUtils.cc`: resource cleanup boilerplate repeated three times

`http_get_and_write_resource()`, `http_get()`, and `gru_mk_attempt()` each contain the same pattern:
```cpp
try {
    // ... work ...
    curl_slist_free_all(req_headers);
    if (ceh) curl_easy_cleanup(ceh);
}
catch (...) {
    curl_slist_free_all(req_headers);
    if (ceh) curl_easy_cleanup(ceh);
    throw;
}
```

A small RAII wrapper for `CURL*` and `curl_slist*` would eliminate this entirely. Example:
```cpp
struct CurlEasyGuard {
    CURL *ceh;
    curl_slist *headers;
    ~CurlEasyGuard() {
        curl_slist_free_all(headers);
        if (ceh) curl_easy_cleanup(ceh);
    }
};
```

---

### 2.11 `url` class: 15 virtual methods, only 2 overridden

`EffectiveUrl` is the only subclass and only overrides `is_expired()` and `dump()`. The other 13 virtual declarations (`str`, `protocol`, `host`, `path`, `query`, `ingest_time`, `get_url_no_query`, `set_ingest_time`, `query_parameter_value`, `query_parameter_values_size`, `query_parameter_values`, `is_trusted`) carry vtable overhead for no runtime benefit.

**Remedy:** Remove `virtual` from all methods except `is_expired()`, `dump()`, and the destructor.

---

### 2.12 `AccessCredentials`: virtual methods with no subclasses

`get()`, `add()`, and `is_s3_cred()` are all virtual in `AccessCredentials` but there are no subclasses anywhere in the codebase. The `virtual` declarations are unused overhead.

**Remedy:** Remove `virtual` from all three methods.

---

### 2.13 `AllowedHosts`: virtual destructor with no polymorphic use

`AllowedHosts` is a singleton and has no subclasses. Its `virtual ~AllowedHosts() = default` declaration is unnecessary.

---

### 2.14 `ProxyConfig`: different (older, non-thread-safe) singleton pattern

`ProxyConfig` uses an old-school static-pointer singleton:
```cpp
class ProxyConfig {
    static ProxyConfig *d_instance;
    ProxyConfig();                    // private constructor
public:
    static ProxyConfig *theOne();     // lazy initialization in .cc file
};
```

All other singletons in the library use C++11 function-local static, which is guaranteed thread-safe:
```cpp
static AllowedHosts *theHosts() {
    static AllowedHosts instance;
    return &instance;
}
```

The `ProxyConfig` implementation likely initializes `d_instance` with an unsynchronized `if (!d_instance) d_instance = new ProxyConfig()` pattern, which has a data race on the first call from multiple threads.

**Remedy:** Rewrite `ProxyConfig::theOne()` to use the function-local static pattern.

---

### 2.15 Orphaned test files: `FileWriter.h`, `FileWriterTest1.cc`, `FileWriterTest2.cc`

These three files exist in `http/unit-tests/` but:
- `FileWriter.h` is not in the library's sources (`Makefile.am` for `http/` does not list it).
- `FileWriterTest1` and `FileWriterTest2` are not in the `UNIT_TESTS` list in `http/unit-tests/Makefile.am`.

They are never compiled as part of `make check`. They appear to be a self-contained concurrency experiment that was written to demonstrate mutex-scoping behavior and then left behind.

**Remedy:** Delete all three files or move `FileWriter.h` into the library if it has a genuine purpose.

---

### 2.16 `gru_mk_attempt()` — cryptic name

The `gru_` prefix almost certainly abbreviates "get redirect url" but this is not documented. The function implements one attempt of the `get_redirect_url()` retry loop.

**Remedy:** Rename to `attempt_get_redirect_url()` or similar when touching the function for other reasons.

---

## 3. The `url` / `EffectiveUrl` Class Design

This is covered in detail in `docs/http-url-deep-dive.md`. Summary:

- Thirteen of fifteen virtual methods are never overridden.
- Assignment operator is deleted, preventing true value semantics.
- Two `shared_ptr` constructors are marked TODO for removal.
- `EffectiveUrl` adds only two parallel vectors of response header names/values and overrides `is_expired()` to consult them.
- Five methods (`str`, `path`, `host`, `protocol`, `query`) account for >90% of production call sites.

The design is in transition from heap-allocated `shared_ptr` ownership to value semantics. The right endpoint is a plain `ParsedUrl` struct plus free functions; the immediate action is finishing the TODO cleanup.

---

## 4. Test Coverage Assessment

### 4.1 What is tested

| Test binary | Covers |
|---|---|
| `HttpUrlTest` | URL parsing, query KVPs, AWS expiry detection, file protocol |
| `HttpUtilsTest` | MIME type detection, `url_encode`, tokenization |
| `HttpErrorTest` | Exception construction, XML serialization via BESXMLInfo |
| `RemoteResourceTest` | HTTP and file resource retrieval, multithreading, rename, temp file delete flag |
| `EffectiveUrlCacheTest` | Cache enable/disable, skip regex, expiry, trusted-URL propagation |
| `AllowedHostsTest` | ACL for HTTP/HTTPS/S3/file URLs |
| `awsv4_test` | AWS V4 signing using AWS test vectors (9 cases) |
| `CurlUtilsTest` | S3 URL detection, filter_aws_url, EDL auth headers, redirect handling |
| `CurlSListTest` | curl slist header append/iterate |

### 4.2 Coverage gaps

**`CredentialsManagerTest` — removed entirely**

`unit-tests/Makefile.am` line 76–77:
> CredentialsManagerTest was removed because it was testing our S3 signing code and that code is broken and being replaced by the AWS C++ SDK.

`CredentialsManager` is still in production code and is called from the hot path in `CurlUtils.cc`. The credential loading, URL matching, file permission checking (mode 600 enforcement), and NGAP credential integration are all without automated tests. This is the most significant coverage gap.

**`ProxyConfig` — no test at all**

There is no test that exercises proxy configuration loading, proxy setup in `configure_curl_handle_for_proxy()`, or the NoProxy regex. Proxy bugs would only surface in integration or manual testing.

**`http_head` and `http_get(vector<char>&)` — declared, never tested, may not be implemented**

These are declared in `CurlUtils.h` but have no implementation in `CurlUtils.cc`. Their only implementations exist in `retired/http/http_head.cc`. No active test exercises them and no active production code calls them. They are dead declarations.

**`FileWriterTest1`, `FileWriterTest2` — exist but not built**

These test files are in the directory but not in the `UNIT_TESTS` list. They are never run by `make check`.

**`EffectiveUrlCache::append_edl_username_to_key()` missing-UID path**

The function logs a `SERVICE CHAIN WARNING` and returns the raw key when the EDL UID context is missing. No test appears to verify this fallback path or the log message.

**`AllowedHosts::check()` and `is_retryable()` compile regex on every call**

Both functions create `BESRegex` objects from pattern strings inside a loop on each invocation. There is a `TODO` comment on `is_retryable()` acknowledging this. No test covers the performance cost. The fix (compile once, store in a container) is unambiguous.

**`EffectiveUrl` header parsing edge cases**

`EffectiveUrl::ingest_response_headers()` silently drops headers that have no `:` delimiter (logs an error). `get_header()` does a reverse linear scan. No test verifies the error path or multi-valued headers.

**Retry logic in `super_easy_perform()`**

The retry-with-backoff logic (`retry_limit = 3`, `url_retry_time = 250'000`) has no unit test. The truncate-and-retry path for S3 failures is exercised by `CurlUtilsTest` only at the integration level.

---

## 5. Simplification Opportunities (Beyond Redundancy Removal)

### 5.1 Collapse `EffectiveUrl` into `url` with an optional header store

`EffectiveUrl` adds exactly two vectors (`d_response_header_names`, `d_response_header_values`) to `url` and overrides `is_expired()` to consult them. The inheritance relationship creates complexity in `EffectiveUrlCache` (which stores `shared_ptr<EffectiveUrl>`) and forces callers to understand the type hierarchy.

Option: add `std::vector<std::string> response_headers` to `url` and fold `EffectiveUrl::is_expired()` into `url::is_expired()` with an if-populated check. `EffectiveUrl` disappears; callers use `url` everywhere. This requires updating `EffectiveUrlCache` to store `shared_ptr<url>` (it already partly works this way internally).

### 5.2 `get_type_from_content_type()` / `load_mime_list_from_keys()` caching

Cache the result of `load_mime_list_from_keys()` in a `static std::map` populated on first call. The MIME list does not change at runtime.

### 5.3 RAII for curl handles

A 10-line RAII guard for `CURL*` and `curl_slist*` would eliminate the try/catch cleanup boilerplate in `http_get_and_write_resource()`, `http_get()`, and `gru_mk_attempt()`.

### 5.4 `AllowedHosts::check()` — precompile regexes

The allowed-hosts regex list is loaded once in the constructor but compiled from strings on every call to `check()`. Pre-compiling at construction time (store `BESRegex` objects instead of `string` patterns) would eliminate repeated compilation overhead.

### 5.5 `is_retryable()` — same regex compilation issue

The no-retry regex list is re-read from `TheBESKeys` and compiled from strings on every call. Cache compiled `BESRegex` objects in a static or member variable.

---

## 6. Summary Table

| Finding | Type | Severity | File(s) |
|---|---|---|---|
| Dual shared_ptr/value API surface (8 wrappers) | Redundancy | Medium | AllowedHosts, CurlUtils, awsv4, url_impl.h |
| Dead declarations: `http_head`, `http_get(vector<char>)` | Dead code | Low | CurlUtils.h |
| `decompose_url()` in `#if 0` | Dead code | Low | HttpUtils.cc |
| `lseek` block in `#if 0` | Dead code | Low | RemoteResource.cc |
| `HTTP_URL_BASE_KEY` defined twice | Bug (benign) | Low | HttpNames.h |
| 4 dead `#define` constants | Dead code | Low | HttpNames.h |
| `BESUtil.h` included twice | Noise | Low | HttpUtils.cc |
| `get_type_from_content_type()` reloads MIME list | Performance | Low | HttpUtils.cc |
| CURLOPT_ENCODING branch uses wrong variable | Latent bug | Medium | CurlUtils.cc |
| EDL token injection inconsistency (#ifdef DEVELOPER) | Behavioral | Medium | CurlUtils.cc |
| Cleanup boilerplate repeated 3× | Simplification | Low | CurlUtils.cc |
| 13 unnecessary virtual methods in `url` | Performance/clarity | Low | url_impl.h |
| Virtual methods in `AccessCredentials` (no subclasses) | Clarity | Low | AccessCredentials.h |
| `ProxyConfig` non-thread-safe singleton | Correctness risk | Medium | ProxyConfig.h/cc |
| Orphaned FileWriter files | Dead code | Low | unit-tests/ |
| **`CredentialsManagerTest` removed** | **Test gap** | **High** | unit-tests/Makefile.am |
| **`ProxyConfig` has no test** | **Test gap** | **High** | unit-tests/ |
| **`http_head`/`http_get(vector)` dead declarations** | **Test gap** | **Medium** | CurlUtils.h |
| FileWriterTest1/2 not in UNIT_TESTS | Test gap | Low | unit-tests/Makefile.am |
| AllowedHosts::check() recompiles regex | Performance | Low | AllowedHosts.cc |
| is_retryable() recompiles regex | Performance | Low | CurlUtils.cc |

---

## 7. Recommended Action Sequence

**PR 1 — Dead code and obvious fixes (no behavioral change)**

1. Delete the two dead function declarations from `CurlUtils.h` (`http_head`, `http_get(vector<char>)`).
2. Delete the `#if 0` block in `HttpUtils.cc` (lines 248–307).
3. Delete the `#if 0` block in `RemoteResource.cc` (lines 255–260).
4. Remove the duplicate `HTTP_URL_BASE_KEY` definition from `HttpNames.h` line 60.
5. Remove the four dead `#define` constants (`HTTP_TARGET_URL_KEY`, `HTTP_QUERY_STRING_KEY`, `HTTP_URL_BASE_KEY`, `HTTP_INGEST_TIME_KEY`) from `HttpNames.h`.
6. Remove one of the two `#include "BESUtil.h"` lines from `HttpUtils.cc`.
7. Delete `FileWriterTest1.cc`, `FileWriterTest2.cc`, `FileWriter.h` from `unit-tests/`.
8. Fix the CURLOPT_ACCEPT_ENCODING branch in `CurlUtils.cc` to use `ceh` and `eval_curl_easy_setopt_result`, and delete the `#ifndef CURLOPT_ACCEPT_ENCODING` guard.

**PR 2 — Finish the TODO migration (remove shared_ptr API surface)**

9. Delete the two `shared_ptr` constructors from `url_impl.h` (lines 75–95).
10. Delete `sign_s3_url(shared_ptr<url>)` and `is_url_signed_for_s3(shared_ptr<url>)` from `CurlUtils`.
11. Delete `compute_awsv4_signature(shared_ptr<url>)` from `awsv4`.
12. Delete the two `shared_ptr` overloads from `AllowedHosts`.
13. Update all call sites (~30) to use value or string interfaces.
14. Run `make check` in `http/unit-tests/`, `modules/dmrpp_module/`, and `modules/cmr_module/`.

**PR 3 — Structural cleanup (low risk)**

15. Remove `virtual` from all `url` methods except `is_expired()`, `dump()`, and the destructor.
16. Remove `virtual` from `AccessCredentials::get()`, `add()`, `is_s3_cred()`.
17. Remove `virtual ~AllowedHosts()`.
18. Rewrite `ProxyConfig::theOne()` to use function-local static.
19. Add RAII guards for `CURL*` / `curl_slist*` in `CurlUtils.cc`.
20. Fix `AllowedHosts::check()` to precompile regex patterns at construction.
21. Fix `is_retryable()` to cache compiled regex objects.
22. Fix `get_type_from_content_type()` to cache the MIME map.

**PR 4 — Test coverage restoration**

23. Restore or rewrite `CredentialsManagerTest` to cover the active code paths (URL matching, config parsing, file permissions) without depending on the broken S3 signing code that was removed.
24. Add `ProxyConfigTest` covering: no-proxy configured, proxy configured, NoProxy regex matching.
25. Add test for `EffectiveUrlCache::append_edl_username_to_key()` missing-UID path.
26. Add test for `EffectiveUrl::ingest_response_headers()` with a malformed header (no colon).
