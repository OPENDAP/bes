# Work Log: `http/` Library Full Deep Dive

## 2026-06-12

### Query/Prompt

> Now perform a deep dive on the whole library in the 'http' directory. I want to know about redundancies, refactorings to reduce code and test coverage.

---

### Approach

The prior analysis ([http-url-deep-dive.md](http-url-deep-dive.md)) covered the `url`/`EffectiveUrl` class design. This task extends that to every file in `http/`.

**Step 1 — File inventory.** Used `find` to get the complete file list. Identified 13 source pairs plus test directories (`unit-tests/`, `tests/`).

**Step 2 — Read all production source files directly.** Agent-delegated reads came back as summaries rather than full content on the first attempt, so I read the files directly with the Read tool. Priority order: `url_impl.h/cc`, `EffectiveUrl.h/cc`, `EffectiveUrlCache.h/cc`, `AllowedHosts.h/cc`, `CurlUtils.h/cc`, `awsv4.h/cc`, `HttpUtils.h/cc`, `HttpNames.h`, `RemoteResource.h/cc`, `AccessCredentials.h`, `HttpError.h`, `ProxyConfig.h`.

`CurlUtils.cc` at 1892 lines is the largest file. I read it in its entirety to catch anything the agent would summarize.

**Step 3 — Read test infrastructure.** Read `unit-tests/Makefile.am` to see exactly which tests are built and which are not.

**Step 4 — Targeted greps** to verify specific hypotheses before writing them up:
- Whether `http_head` and `http_get(vector<char>)` have implementations anywhere active.
- Whether the four dead `#define` constants are used anywhere outside `HttpNames.h`.
- Whether `BESUtil.h` is actually included twice in `HttpUtils.cc`.

---

### What Each File Revealed

**`HttpNames.h`:** Two independent issues: `HTTP_URL_BASE_KEY` defined on consecutive lines (lines 58 and 60), and four defines that are only referenced from inside a `#if 0` block in `HttpUtils.cc`. Both were confirmed by grep.

**`HttpUtils.cc`:** Three issues: double-include of `BESUtil.h`, dead `decompose_url()` function in `#if 0`, and `get_type_from_content_type()` calling `load_mime_list_from_keys()` on every invocation with no caching.

**`CurlUtils.h`:** `http_head` and `http_get(vector<char>&)` declared but unimplemented in active code. Grep confirmed the only implementations are in `retired/http/http_head.cc` and call sites exist only there too. These are dead declarations that would fail to link if ever called.

**`CurlUtils.cc` (1892 lines):** Five distinct issues found by reading the full file:
1. The `#ifndef CURLOPT_ACCEPT_ENCODING` else-branch uses `curl` instead of `ceh` and calls a non-existent helper. Dead in practice but a latent compile error.
2. EDL token injection in `http_get()` is gated by `#ifdef DEVELOPER` but the same logic in `gru_mk_attempt()` is unconditional. This creates an asymmetric behavior difference that is probably unintentional.
3. RAII cleanup pattern repeated identically in three functions.
4. `gru_mk_attempt()` is a cryptic name.
5. `is_retryable()` and `AllowedHosts::check()` both recreate regex objects on every call rather than caching compiled objects.

**`ProxyConfig.h`:** Uses an old-style static-pointer singleton (stores `static ProxyConfig *d_instance`). All other singletons in the library use function-local static, which is guaranteed thread-safe by C++11. Without reading `ProxyConfig.cc` to confirm the implementation, the pattern in the header is a red flag. This is the only singleton that diverges.

**`unit-tests/Makefile.am`:** Two important findings:
1. `CredentialsManagerTest` is explicitly commented out with the explanation that S3 signing code was broken and is being replaced by the AWS C++ SDK. This is a real coverage gap because `CredentialsManager` is still in production and on the hot path.
2. `FileWriterTest1.cc`, `FileWriterTest2.cc`, and `FileWriter.h` exist in the directory but `FileWriterTest1` and `FileWriterTest2` are not in `UNIT_TESTS`. They are never compiled or run.

---

### Decisions Made in Analysis

**Including `ProxyConfig` singleton pattern as a medium-severity finding:** Even without confirming the `.cc` implementation, the header's use of a static data member (vs. function-local static) is sufficient to flag this as a thread-safety risk. The pattern is inconsistent with the rest of the library.

**Classifying `CredentialsManagerTest` removal as the highest-severity test gap:** The Makefile comment explicitly says the tests were removed because of broken code, not because the coverage was replaced elsewhere. That combination — active production code, no tests, known-broken implementation in progress — is the highest-risk gap in the test suite.

**Not flagging `echo-token` deprecation (CurlUtils.cc line 1487) as a redundancy:** The TODO comment says to remove it per HYRAX-1036, but since this is an active ticket and not dead code, it belongs in the issue tracker rather than this analysis.

---

### What Was Not Verified

- Whether `ProxyConfig::theOne()` in `ProxyConfig.cc` actually uses the unsafe pattern (assumed from header structure).
- Whether `CredentialsManager::get(shared_ptr<url>)` has a paired shared_ptr overload (the header wasn't read fully).
- `http/tests/` (the autotest suite) was not analyzed. That directory contains integration-level tests for the HTTP stack against a mock S3 server. Coverage there is separate from the unit tests assessed here.
- The dmrpp_module's CurlHandlePool duplicate S3 signing logic was noted but not analyzed in depth.

---

### Output

- Analysis document: `docs/http-library-deep-dive.md`
- This work log: `docs/http-library-deep-dive-work-log.md`
