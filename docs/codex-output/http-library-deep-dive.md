# HTTP Library Deep Dive

## Scope

This review covers the shared `http/` library, its unit and autotest coverage, and the in-tree users I found in BES modules. I ignored `retired/` per repository instructions. The main external consumers are DMR++ chunk access, NGAP/CMR URL and DMR++ retrieval, CMR containers, `gateway_module`, `httpd_catalog_module`, and `s3_reader`.

The short version: the library is useful and should not be collapsed into loose functions, but it has grown into a mixed bag of URL parsing, curl setup, retry policy, credential lookup, proxy configuration, effective URL caching, temporary-file management, AWS signing, and HTTP error reporting. The safest simplification is not a broad rewrite. It is a sequence of small extractions and bug fixes that keep the public behavior stable.

## Main Findings

### 1. `RemoteResource` has a real file-descriptor lifetime hazard

`RemoteResource` initializes `d_fd` to `0` (`http/RemoteResource.h:61-62`) and the destructor closes any descriptor that is not `-1` (`http/RemoteResource.cc:117-121`). The constructor marks `file://` resources initialized without opening a descriptor (`http/RemoteResource.cc:83-88`), so a `file://` `RemoteResource`, or a default-constructed test object, can close file descriptor 0 when destroyed.

This is a high-priority fix before deeper cleanup. Initialize `d_fd` to `-1`, and only close descriptors that the object actually opened or was explicitly given. Add regression tests for default construction and `file://` construction that prove stdin remains open.

### 2. `CurlUtils.cc` has too many responsibilities

`CurlUtils.cc` is the library's main complexity center. It contains curl handle initialization, proxy setup, user-agent/netrc/cookie setup, response header capture, retry policy, HTTP status interpretation, redirect discovery, S3 signing hooks, EDL auth headers, URL filtering, range formatting, and debug/error formatting.

That breadth makes it hard to reason about behavior and creates duplication with DMR++'s `CurlHandlePool`. For example, `CurlUtils::init()` configures most easy-handle options for ordinary requests, while `modules/dmrpp_module/CurlHandlePool.cc` repeats many of the same settings for pooled range requests: error buffer, header/write callbacks, cookies, redirect limits, user-agent, auth/netrc, S3 signing, and EDL token handling.

The right extraction is a reusable curl-handle configuration layer, not a replacement for DMR++'s pool. DMR++ still needs pooled handles, shared state, range reads, and chunk-specific callbacks. But common setup should be callable on an existing `CURL *`.

Recommended split:

- `CurlHandleConfig`: pure-ish helper that applies common BES curl options to a provided `CURL *`.
- `CurlRetryPolicy`: retryable curl codes, retryable HTTP status codes, retry delay, and no-retry URL matching.
- `CurlAuthHeaders`: EDL headers, credential lookup, and S3 signing.
- `CurlTransfer`: high-level `http_get`, `http_head`, redirect lookup, and fd/string transfer functions.

This would reduce code without changing user-facing behavior.

### 3. There is a compile-path risk in curl compression setup

`CurlUtils::init()` tries to use `CURLOPT_ACCEPT_ENCODING` when that macro exists, but the active branch refers to `curl` and `check_setopt_result` instead of the local `ceh` and `eval_curl_easy_setopt_result` names (`http/CurlUtils.cc:444-451`). If that branch is compiled with a libcurl that defines `CURLOPT_ACCEPT_ENCODING`, it appears likely to fail compilation.

This should be fixed immediately and covered by the normal build matrix. It is small, isolated, and not behaviorally risky.

### 4. Credential lookup and ownership need cleanup

`CredentialsManager::get(const shared_ptr<http::url>&)` uses longest-prefix matching correctly (`http/CredentialsManager.cc:134-145`). The string overload resets `best_key` inside the loop (`http/CredentialsManager.cc:162-170`), so any matching key can win based on map iteration rather than longest matching prefix.

Credentials are also stored as raw `AccessCredentials *` values. The manager has a `clear()` helper but the ownership model is not obvious from the interface. This code would be simpler and safer with `std::unique_ptr<AccessCredentials>` in the map and raw non-owning pointers returned to callers.

`AccessCredentials::add()` uses `insert()` (`http/AccessCredentials.cc:64-67`), so duplicate keys are ignored. That is surprising for a key/value object. `is_s3_cred()` also caches its result (`http/AccessCredentials.cc:73-79`), but `add()` does not invalidate the cache. Either make `AccessCredentials` immutable after loading, or make `add()` assign/update and reset the S3 cached state.

The active test suite currently excludes `CredentialsManagerTest` because it was tied to broken/replaced S3 signing assertions (`http/unit-tests/Makefile.am:72-77`). That leaves important credential parsing and lookup behavior under-tested. Split those tests: re-enable credentials-file/env/longest-prefix tests, and remove only the obsolete signing assertions.

### 5. Configuration and regex work is repeated

Several hot or repeated paths load config or compile regexes each time:

- `curl::is_retryable()` reads `Http.No.Retry.Regex` and compiles `BESRegex` objects per call (`http/CurlUtils.cc:720-731`).
- Proxy no-proxy regexes are stored as strings in `ProxyConfig` and evaluated from curl setup paths; the class loads `Http.ProxyProtocol` but the implementation comment says it is unused downstream (`http/ProxyConfig.cc:67-73`).
- `get_type_from_content_type()` reloads the MIME map for every lookup (`http/HttpUtils.cc:148-153`).
- curl handle setup reloads/calls config helpers such as max redirects, cookies, netrc, and user agent, while DMR++ caches some of these separately in `CurlHandlePool`.

These do not need a global rewrite. Cache immutable or effectively immutable configuration after BES configuration is loaded, and expose test reset hooks where tests legitimately mutate `TheBESKeys`.

### 6. Singleton style is inconsistent

`CredentialsManager` already uses a local static singleton. `ProxyConfig` uses a raw static pointer with lazy `new` and no locking (`http/ProxyConfig.cc:27-34`). This is avoidable complexity and a thread-safety risk. Convert it to a local static instance unless there is a test dependency that requires reset/replacement.

`EffectiveUrlCache` is more stateful and has explicit cache behavior, so treat it separately. Do not casually collapse cache state into free functions.

### 7. Header handling can be made safer

`EffectiveUrl` preserves response header order by using parallel name and value vectors (`http/EffectiveUrl.h:52-54`). That works, but parallel vectors are an avoidable invariant. A `vector<pair<string,string>>` would preserve order and duplicate headers while reducing the chance of name/value skew.

`HttpError` carries response headers and response body into error details. That is useful for diagnostics, but response bodies can be large or sensitive. Consider capping body length and documenting whether authentication-related response content can reach BESInfo output.

### 8. Small redundancy cleanup is available

Low-risk cleanup candidates:

- `HTTP_URL_BASE_KEY` is defined twice (`http/HttpNames.h:57-60`).
- `HttpUtils.cc` has dead `#if 0` URL decomposition code and duplicate include noise.
- `HttpUtils::get_type_from_url()` hard-codes the catalog name `"catalog"` (`http/HttpUtils.cc:168-171`). That may be intentional, but it should either use the configured/default catalog API or be documented as a framework assumption.
- `RemoteResource` callers in CMR, gateway, httpd catalog, and s3_reader often construct `shared_ptr<http::url>` only to pass it directly into `RemoteResource`. A string-taking constructor would remove boilerplate without changing the core URL object.
- `CurlUtils.h` still exposes `shared_ptr<http::url>` overloads that are marked as temporary (`http/CurlUtils.h:95-102`). Keep DMR++ internal ownership where needed, but move stateless helpers to `const http::url&` or `const string&`.

## How Effective Is The Current Library?

The library is effective as shared infrastructure. It centralizes behavior that must remain consistent across BES modules: allowed-host checks, remote retrieval, retry behavior, redirect/effective URL caching, EDL/S3 credential handling, user-agent/cookie/netrc configuration, and conversion of remote resources into local files.

It is less effective as an abstraction boundary. Callers often need to know which layer owns which policy:

- CMR uses `http::url_encode()` and `RemoteResource`, mostly as convenience wrappers.
- DMR++ keeps `shared_ptr<http::url>` in chunks and uses `EffectiveUrlCache`, curl helpers, and credentials over long-lived chunk-read workflows.
- NGAP uses both HTTP helpers and AWS/SignedUrl cache behavior.
- `gateway_module`, `httpd_catalog_module`, and `s3_reader` use `RemoteResource` to materialize remote content.

That split means a functions-only library would probably scatter state and policy. The better design is a small set of value types plus focused services:

- `http::url` or `http::ParsedUrl`: parsed URL value plus trust/ingest metadata.
- `RemoteResource`: materializes a URL into a local file/descriptor.
- `CurlTransfer`/`CurlHandleConfig`: curl setup and execution.
- `CredentialsManager`: credential lookup only.
- `EffectiveUrlCache`: redirect/effective URL cache only.
- `HttpError`: structured transport error.

## Refactoring Plan

### Phase 0: Fix isolated correctness issues

Do these before broad refactoring:

1. Initialize `RemoteResource::d_fd` to `-1` and add fd-lifetime tests.
2. Fix the `CURLOPT_ACCEPT_ENCODING` branch in `CurlUtils::init()`.
3. Fix `CredentialsManager::get(const string&)` longest-prefix matching.
4. Re-enable credential tests after removing stale S3-signing expectations.
5. Remove the duplicate `HTTP_URL_BASE_KEY` definition.
6. Decide whether `AccessCredentials::add()` updates existing keys or the object becomes immutable after load.

### Phase 1: Make tests deterministic

Separate library unit tests from integration tests:

- Keep pure tests in `http/unit-tests`: URL parsing, allowed hosts, credential lookup, access credential behavior, retry policy, range formatting, HTTP error formatting, and header parsing.
- Move live service tests against `test.opendap.org`, Earthdata Login, DAACs, CloudFront, or S3 into an opt-in integration group.
- Expand the existing local mock-S3 retry tests (`http/tests/testsuite_s3.at:34-36`) because those are deterministic and exercise important retry behavior.
- Replace the ten identical live `test.opendap.org` autotest invocations (`http/tests/testsuite.at:50-59`) with a local fixture or make them explicitly stress/integration tests.

### Phase 2: Extract reusable curl setup

Introduce a helper that configures an already-created easy handle. Use it first from `CurlUtils::init()`, then migrate DMR++ `CurlHandlePool` to call the same helper for common settings while preserving its pool/range/multi-specific behavior.

This is the largest code-reduction opportunity because it removes duplicated curl option policy without destabilizing DMR++ chunk IO.

### Phase 3: Reduce pointer overloads

Keep URL objects where they carry real state, especially DMR++ chunk URLs and effective URL cache entries. Remove `shared_ptr<http::url>` overloads where callees only read fields. Prefer:

- `const http::url&` when parsed URL state is already available.
- `const string&` for pure string helpers.
- A `RemoteResource(string, trusted=false, uid="")` constructor for shallow callers.

### Phase 4: Separate URL parsing from BES policy

The URL deep dive already concluded that replacing `http::url` with loose functions would be a step backward. The object should stay, but it should become narrower. Move catalog-relative path coercion, expiration policy, and service-specific URL rewriting into named helpers or policy functions.

### Phase 5: Modernize ownership carefully

Use RAII where it reduces actual risk:

- `unique_ptr` in credential maps.
- a small RAII wrapper for `curl_slist *`.
- a small fd wrapper or stricter fd state in `RemoteResource`.
- local-static singletons where singleton behavior is truly required.

Avoid broad formatting or API churn. BES is long-lived and downstream users depend on stable behavior.

## Test Coverage Assessment

Current coverage has useful pieces:

- `HttpUrlTest`, `AllowedHostsTest`, `HttpUtilsTest`, `HttpErrorTest`, `awsv4_test`, and `CurlSListTest` cover a meaningful amount of pure behavior.
- `CurlUtilsTest` covers URL signing shape, URL filtering, retry policy, EDL header construction, and some live retrieval paths.
- `RemoteResourceTest` exercises materialization behavior, but some paths depend on live network resources.
- `EffectiveUrlCacheTest` covers cache behavior but also appears to include live service assumptions.
- `http/tests/testsuite_s3.at` gives a good local retry-test model.

Important gaps:

- No active `CredentialsManagerTest` despite credentials being central to DMR++, NGAP, and S3 paths.
- No regression test for `RemoteResource` closing fd 0.
- No deterministic test for `CredentialsManager::get(string)` longest-prefix matching.
- No test for duplicate/update behavior in `AccessCredentials::add()`.
- Weak or absent tests for `ProxyConfig`, `Http.NoProxy`, and proxy auth settings.
- Weak coverage for malformed or edge-case `Cache-Control` and signed URL expiration.
- Network-dependent unit tests make `make check` more fragile than it should be.

Suggested new focused tests:

- `RemoteResourceDefaultDoesNotCloseStdin`
- `RemoteResourceFileUrlDoesNotCloseStdin`
- `CredentialsManagerStringLookupUsesLongestPrefix`
- `AccessCredentialsAddDuplicateUpdatesOrRejectsExplicitly`
- `RetryPolicyCachesNoRetryRegexAndMatchesFullUrl`
- `ProxyConfigLoadsHostPortNoProxyAndAuth`
- `HttpUtilsTypeLookupUsesExpectedCatalog`
- `EffectiveUrlHeaderParsingPreservesDuplicateHeaders`

## Bottom Line

The `http` library can be simplified, but the valuable boundary is not "classes versus functions." The valuable boundary is "stateful services and value objects versus mixed policy blobs." Keep `http::url`, `RemoteResource`, `CredentialsManager`, and `EffectiveUrlCache` as concepts. Shrink them, fix the correctness issues, centralize curl handle configuration, and split deterministic unit tests from live integration tests. That gives real code reduction with much less risk than replacing the library with free functions.
