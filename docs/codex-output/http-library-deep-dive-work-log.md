# HTTP Library Deep Dive Work Log

## 2026-06-12 15:20:10 MDT

Prompt:

> Now perform a deep dive on the whole library in the 'http' directory. I want to know about redundancies, refactorings to reduce code and test coverage.

Plan:

1. Read the repository and docs instructions relevant to `docs/` and `http/`.
2. Inventory the `http/` library sources, tests, and configuration.
3. Map important in-tree users in BES modules, especially DMR++, CMR, NGAP, gateway, httpd catalog, and s3_reader.
4. Identify correctness risks, redundancy, and simplification opportunities.
5. Assess current test coverage and propose focused additions.
6. Write the analysis and work log in `docs/`, then update `docs/Makefile.am`.

Work performed:

- Read `AGENTS.md` and `docs/AGENTS.md`.
- Ignored `retired/` as instructed.
- Reviewed `http/Makefile.am`, `http/unit-tests/Makefile.am`, `http/tests/Makefile.am`, `http/tests/testsuite.at`, and `http/tests/testsuite_s3.at`.
- Inventoried the library sources and rough size with `rg --files http` and `wc -l`.
- Reviewed the main library components: `CurlUtils`, `HttpUtils`, `RemoteResource`, `CredentialsManager`, `AccessCredentials`, `AllowedHosts`, `ProxyConfig`, `EffectiveUrl`, `EffectiveUrlCache`, `HttpError`, `HttpNames`, `awsv4`, and `url_impl`.
- Reviewed module users with `rg`, including DMR++ `Chunk`, `DmrppCommon`, `DMZ`, `CurlHandlePool`, NGAP containers, CMR `CmrApi`, `JsonUtils`, `CmrContainer`, `gateway_module`, `httpd_catalog_module`, and `s3_reader`.
- Checked existing URL-specific findings in `docs/http-url-deep-dive.md` and built on them rather than duplicating the full URL analysis.

Findings recorded in the report:

- `RemoteResource` initializes `d_fd` to `0` while the destructor closes any descriptor not equal to `-1`, which can close stdin for default-constructed or `file://` resources.
- `CurlUtils::init()` has a likely compile-path error in the `CURLOPT_ACCEPT_ENCODING` branch because it refers to names that are not in scope.
- `CredentialsManager::get(const string&)` does not preserve the longest matching key because `best_key` is declared inside the loop.
- `CredentialsManagerTest` is disabled, leaving central credential behavior without active focused coverage.
- `CurlUtils.cc` combines too many responsibilities and duplicates common curl setup with DMR++ `CurlHandlePool`.
- Several configuration and regex paths are repeatedly loaded or compiled.
- `ProxyConfig` uses an older raw-pointer singleton pattern while other classes have moved toward local-static singletons.
- `AccessCredentials::add()` ignores duplicate keys and does not invalidate cached S3 classification.
- Header storage, MIME lookup, duplicate macros, dead code, and shallow `shared_ptr<http::url>` call sites provide lower-risk simplification opportunities.

Validation:

- This pass was documentation and static analysis only.
- I did not run `./configure`, `make`, `make check`, or `make distcheck`.
- I did not run network-dependent HTTP tests.
- External prerequisites such as libdap, hyrax-dependencies, AWS credentials, EDL credentials, and live remote services were not used.
