# HTTP URL Deep Dive

## Scope And Assumptions

This review covers `http/`, the BES and AWS code paths that use `http::url`, and the module call sites I found outside `http/`, with emphasis on `modules/dmrpp_module/` and `modules/cmr_module/`. I ignored `retired/` and treated checked-in data fixtures as evidence only when they were tied to tests.

No build or test validation was run because this was an analysis-only pass. I used source inspection with `rg`, `find`, `sed`, and `nl`; no `./configure`, `make`, `make check`, or network-backed tests were run. Any future code change in this area should be validated with focused `http/unit-tests`, DMR++ chunk/NGAP tests, CMR tests, and then broader `make check` using the normal autotools configuration:

```sh
./configure --prefix="$prefix" --with-dependencies="$prefix/deps" --enable-developer
make -j20
make check
```

This analysis depends on BES runtime configuration semantics: default catalog root, allowed hosts, credentials configuration, effective URL caching, TEA/EDL redirects, and S3 signing. Those external prerequisites are part of the behavior being analyzed, not incidental details.

## Short Answer

The `http::url` class is useful, but it is not clean. It currently combines at least four responsibilities:

- URL component parsing.
- BES catalog-root conversion for relative/file paths.
- trust state used by allowed-host checks.
- expiration logic for CloudFront/AWS presigned URLs.

Replacing it wholesale with a set of free functions would probably make the code worse unless those functions returned a small value object anyway. The object gives call sites a stable bundle of `str`, `protocol`, `host`, `path`, `query`, `trusted`, and `ingest_time`. That bundle matters in `AllowedHosts`, `RemoteResource`, `EffectiveUrlCache`, AWS signing, DMR++ chunk access, and tests.

The better simplification is incremental:

1. Keep a small immutable URL value type.
2. Remove `shared_ptr<http::url>` overloads where ownership is not needed.
3. Move policy-heavy behavior out of the URL value type: catalog path coercion and presigned URL expiration.
4. Centralize URL query building/encoding helpers now duplicated in modules.
5. Fix the concrete bugs and edge cases before doing larger refactors.

## What `http::url` Does Today

The class stores original URL string, protocol, host, path, query string, parsed query values, ingest time, and trusted state in one object (`http/url_impl.h:49-56`). Its public API exposes parsed components, query lookups, no-query formatting, expiration, trust, and debug dump (`http/url_impl.h:101-128`).

The constructor parses eagerly (`http/url_impl.h:66-70`). If the input has no `://`, `url::parse()` rewrites it into a `file://` URL under the default catalog root (`http/url_impl.cc:63-88`). This is important security behavior, not just parsing.

For `file://`, parsing stores everything after `file://` as the path (`http/url_impl.cc:105-107`). For `http://` and `https://`, it lowercases host and protocol, splits path/query, and parses query pairs (`http/url_impl.cc:109-124`). Unsupported protocols throw during construction (`http/url_impl.cc:127-131`).

The query parser supports repeated keys by storing a vector per key (`http/url_impl.cc:140-158`). This is useful and tested with repeated `q` parameters in `HttpUrlTest`. It does not decode percent escapes, does not treat semicolon as a delimiter, and ignores query records without `=`.

Expiration logic reads CloudFront `Expires` or AWS `X-Amz-Expires` plus optional `X-Amz-Date` from the query string (`http/url_impl.cc:212-288`). `EffectiveUrl` subclasses `url` and extends `is_expired()` with `Cache-Control: max-age` from response headers before falling back to `url::is_expired()` (`http/EffectiveUrl.cc:65-99`).

## Where The Class Is Valuable

`AllowedHosts` depends on parsed protocol/path and trust state. For `file://`, it uses `candidate_url.path()` and verifies catalog containment with `BESUtil::check_path()` (`http/AllowedHosts.cc:101-153`). For `http(s)`, it allows either configured host regexes or `candidate_url.is_trusted()` (`http/AllowedHosts.cc:160-163`).

`RemoteResource` depends on protocol and path to split file access from remote retrieval (`http/RemoteResource.cc:80-107`). It receives a URL object, validates protocol behavior, derives a basename from the parsed path, and then hands the object to curl retrieval.

AWS signing uses parsed path, query, and host. There are existing overloads that accept `http::url` or `shared_ptr<http::url>` and delegate to the core string-based signer (`http/awsv4.cc:394-424`). `curl::sign_s3_url(const string&)` still constructs a temporary `http::url` only to get those fields (`http/CurlUtils.cc:1515-1523`).

DMR++ stores URL objects directly in chunks. `Chunk` owns `shared_ptr<http::url> d_data_url` (`modules/dmrpp_module/Chunk.h:77-80`); `DmrppCommon` has many `add_chunk()` overloads that accept `shared_ptr<http::url>` (`modules/dmrpp_module/DmrppCommon.h:344-390`); `DMZ` constructs those URLs from dataset and chunk `href` attributes (`modules/dmrpp_module/DMZ.cc:355`, `modules/dmrpp_module/DMZ.cc:1960`). `Chunk::get_data_url()` may turn the original URL into a locally presigned URL or a redirect/effective URL before returning it (`modules/dmrpp_module/Chunk.cc:1359-1393`).

CMR uses the class more shallowly. `JsonUtils::get_as_json()` and `CmrContainer::access()` mostly construct `http::url` only as the input to `RemoteResource` (`modules/cmr_module/JsonUtils.cc:58-64`, `modules/cmr_module/CmrContainer.cc:142-148`). Those call sites would be easy to migrate if `RemoteResource` grew a string-taking constructor.

## Weaknesses And Redundancy

The biggest design problem is responsibility mixing. A class named `url` silently consults the BES catalog and rewrites relative strings to `file://` (`http/url_impl.cc:63-88`). That makes construction context-sensitive. A pure parser would not do this, but many current call sites probably rely on it.

`url::is_expired()` is policy logic, not URL parsing. It is tied to AWS and CloudFront query names and mutable ingest time (`http/url_impl.cc:212-288`). This makes the base URL object carry service-chain cache behavior even when a call site only needs `host()` or `path()`.

There is API duplication around `shared_ptr<http::url>`. The header itself marks shared pointer constructors as removable (`http/url_impl.h:74-95`). `CurlUtils`, `awsv4`, `AllowedHosts`, and `CredentialsManager` all carry pointer overloads beside value/string overloads. The comments in `CurlUtils` and `awsv4` show this refactor has already started.

Some uses of `shared_ptr<http::url>` are not ownership-driven. Examples: `AllowedHosts::is_allowed(const shared_ptr<url>&)` immediately dereferences to the value overload (`http/AllowedHosts.cc:73-84`), and `curl::sign_s3_url(const shared_ptr<url>&)` delegates to the string overload (`http/CurlUtils.cc:1542-1544`). These are low-risk cleanup targets once call sites move.

There is duplicated URL encoding/building logic. `http::url_encode()` exists (`http/HttpUtils.cc:215-240`), CMR uses it for facet keys, while NGAP manually creates a libcurl easy handle only to call `curl_easy_escape()` and sometimes `curl_easy_unescape()` while building CMR URLs (`modules/dmrpp_module/ngap_container/NgapApi.cc:145-181`). These should converge on one helper for query parameter construction, ideally one that has tests for already-encoded path elements.

The DMR++ curl pool repeatedly calls `chunk->get_data_url()` inside one handle acquisition (`modules/dmrpp_module/CurlHandlePool.cc:309-324`). That accessor can do real work: presigned URL lookup, fallback effective URL lookup, and tracking query handling (`modules/dmrpp_module/Chunk.cc:1359-1393`). Fetching once into a local `shared_ptr` would be clearer and safer.

## Concrete Bugs Or Bug Risks Found

`CredentialsManager::get(const string&)` likely has a longest-prefix bug. The `shared_ptr<url>` overload keeps `best_key` outside the loop (`http/CredentialsManager.cc:134-145`), but the string overload declares `best_key` inside the loop (`http/CredentialsManager.cc:162-170`). That means every matching key beats an empty key, so result selection can depend on map iteration rather than longest matching credential prefix.

`NgapApi::convert_ngap_resty_path_to_data_access_urls()` erases a `.dmrpp` suffix from `data_s3_url` using `data_access_url.size()` instead of `data_s3_url.size()` (`modules/dmrpp_module/ngap_container/NgapApi.cc:521-530`). If the URLs differ in length, it can erase at the wrong offset or throw.

`url::is_expired()` assumes AWS date fields are long enough and numeric before calling `substr()`/`stoi()` (`http/url_impl.cc:239-271`). Malformed signed URLs could throw `std::invalid_argument` or `std::out_of_range` instead of a BES error, and short strings can produce surprising behavior.

`url::parse()` handles `http://host` without a slash by leaving path empty and query empty because it searches for `/` before `?` (`http/url_impl.cc:111-120`). That may be acceptable, but it should be intentional and tested because signing and basename logic use `path()`.

The query parser does not decode percent-encoded keys/values and only splits on `&` (`http/url_impl.cc:140-148`). That matches current tests for AWS signatures, where preserving encoded bytes may be desired, but it is a parser limitation that should be named before reusing it more broadly.

## What Happens If `url` Is Replaced With Functions?

If replacement means simple functions like `protocol(url)`, `host(url)`, `path(url)`, `query(url)`, `is_trusted(url)`, and `is_expired(url)`, the result is worse. Trust and ingest time are state, not derivable from the string. Every caller that currently passes a URL object would need to carry extra parameters or a sidecar structure.

If replacement means a set of parsing functions returning a `ParsedUrl` struct, that is effectively a cleaner version of the current class. That could be a good destination, but it should be a value type with explicit policy functions, not unrelated free functions scattered across call sites.

The high-churn areas would be DMR++ and caches. DMR++ stores shared URL objects in `Chunk`, `SuperChunk`, `DmrppCommon`, and the curl handle pool. `EffectiveUrlCache` and `SignedUrlCache` preserve trust and ingest/expiration state while cloning cached URLs (`http/EffectiveUrlCache.cc:77-160`, `aws/SignedUrlCache.cc`). A functions-only API would need a replacement object at exactly those boundaries.

The low-churn areas would be CMR, gateway, httpd catalog, and some `RemoteResource` callers. They often construct `http::url` just before passing it to `RemoteResource`. A `RemoteResource(std::string url, bool trusted = false)` constructor could hide object construction without changing behavior.

## Recommended Simplification Plan

First, fix the known bugs: `CredentialsManager::get(const string&)` longest-prefix selection and the NGAP `.dmrpp` suffix erase offset. Add focused regression tests before broader refactoring.

Second, add small helper functions that expose policy explicitly:

- `parse_url_components(string)` for pure parsing.
- `catalog_relative_path_to_file_url(string)` for BES catalog coercion.
- `query_param_values(query, key)` for repeated query parameters.
- `url_without_query(parsed_url)` for safe logging.
- `is_presigned_url_expired(parsed_url, ingest_time)` or a cache-oriented helper outside the base URL type.
- `build_query_string(vector<pair<string,string>>)` for CMR/NGAP query construction.

Third, simplify pointer APIs without changing behavior. Prefer `const http::url&` where an object is already present and `const string&` where only the original URL string is needed. Remove pointer overloads only after all in-tree call sites have migrated.

Fourth, keep `http::url` or rename it to something like `http::Url`/`http::ParsedUrl`, but make the direction explicit: immutable parsed components plus trust/ingest metadata. Avoid letting it keep accumulating service-specific behavior.

Fifth, add a string-taking `RemoteResource` constructor as a compatibility shim. That would simplify CMR, gateway, httpd catalog, and s3_reader call sites without disturbing DMR++ internals.

Sixth, cache `Chunk::get_data_url()` locally inside `CurlHandlePool::get_easy_handle()` so one handle acquisition uses one resolved URL object. This is a small, reviewable DMR++ cleanup with a clear behavioral invariant.

## Suggested Tests Before Refactor

Add tests for:

- `CredentialsManager::get(const string&)` selecting the longest matching prefix.
- NGAP `.dmrpp` suffix removal when HTTP and S3 URLs have different lengths.
- malformed and short `X-Amz-Date` values.
- `http://host`, `http://host?x=y`, and empty-path signing expectations.
- repeated query values and encoded values, documenting whether decoding is intentionally not performed.
- `RemoteResource` string constructor behavior if added.
- DMR++ `CurlHandlePool::get_easy_handle()` behavior with one resolved URL per acquisition.

## Bottom Line

The `url` class is effective as a compatibility and state carrier. It is less effective as a clean abstraction because it mixes parsing, BES file policy, trust, and cache expiration. Do not replace it with loose free functions. Do replace the muddled parts with focused helpers and reduce `shared_ptr<http::url>` APIs gradually. That gives real simplification without destabilizing server access control, DMR++ chunk reads, CMR retrieval, or S3/TEA signing behavior.
