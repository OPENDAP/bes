# Task Plan: NgapApi code cleanup from the 2026-10-29 deep dive

**Task:** TASK-002
**Status:** Draft
**Created:** 2026-10-29

## Summary

This task cleans up three things in `NgapApi`:
- **curl calls.** Unchecked libcurl escape/unescape calls are replaced with one RAII helper
  that checks for null.
- **Endpoint cache.** The function-local static cache of the CMR endpoint URL is removed.
- **Header.** `NgapApi.h` loses its unused includes and forward declaration. The two files
  that relied on its `<vector>` include get their own.

## Motivation

The 2026-10-29 deep dive (`docs/deep-dives/ngap-api-cmr-lookup.md`, items 9–11) found these:
- **Unchecked curl calls.** `build_cmr_query_url*` makes five `curl_easy_escape` /
  `curl_easy_unescape` calls and two `curl_easy_init` calls without checking for null.
  `std::string::append(nullptr)` is undefined behavior. Each call site also manages its
  memory with a matching `curl_free` by hand.
- **Static endpoint cache.** `get_cmr_search_endpoint_url()` caches its result in a
  function-local `static string`. Tests can't change the endpoint after the first call, and
  the empty-check-then-assign pattern isn't thread-safe.
- **Header.** `NgapApi.h` includes headers it doesn't use and forward-declares a class it
  doesn't use.

## Related requirements / constraints

- **FR-002, FR-003, FR-004** (legacy path formats, no double encoding) and **FR-001**
  (current format). The escape refactor must not change any CMR query these produce. The
  existing tests `resty_path_to_cmr_query_test_01..04` and
  `resty_path_to_cmr_query_test_legacy_url_pre_encoded` check this.
- No IC entries exist. The C++14 and RAII conventions in `CLAUDE.md` apply. The helper
  uses `std::unique_ptr` with custom deleters, which is fine in C++14.

## Scope

**In scope:**
- `modules/dmrpp_module/ngap_container/NgapApi.cc`:
  - the curl helper
  - the `unique_ptr` `CURL*` handle
  - removing the static cache
- `modules/dmrpp_module/ngap_container/NgapApi.h`: remove `#include <vector>`,
  `#include <map>`, and the `namespace http { class url; }` forward declaration.
- `modules/dmrpp_module/ngap_container/NgapOwnedContainer.cc`: add `#include <vector>`
  (include only).
- `modules/dmrpp_module/ngap_container/unit-tests/NgapApiTest.cc`: add `#include <vector>`,
  plus one new test (Approach step 4).

**Out of scope:**
- Which exception class a non-array `RelatedUrls` throws (`BESNotFoundError` vs.
  `BESInternalError`), which is a design decision.
- Converting `NgapApi` to free functions in a namespace (the header note, jhrg 10/8/25).
- Any change to the legacy format's unescape-then-escape or the current format's escape-only
  behavior. The current-format double-encoding question needs a requirement first.
- Unchecked rapidjson access, the one-character length checks, and the "expected at least 0"
  error text. These are bugs to log separately.
- Everything in BUG-001 (filter, trim, container `.dmrpp` append) and TASK-001 (fixtures).
- Any other change to `NgapOwnedContainer.cc` or `NgapApiTest.cc`.

## Approach

1. **The curl helpers** (in `NgapApi.cc`, inside an anonymous namespace in `ngap`):
   - `std::string url_escape(CURL *ceh, const std::string &s)` calls `curl_easy_escape`,
     throws `BESInternalError` (with `__FILE__`, `__LINE__`) on null, and copies the result
     into a `std::string`. The curl buffer is held in
     `std::unique_ptr<char, decltype(&curl_free)>`, so it is freed even if the copy throws.
   - `std::string url_unescape(CURL *ceh, const std::string &s)` does the same with
     `curl_easy_unescape`. It builds the result with `std::string(ptr, out_len)` so that
     the decoded length is kept.
   - The legacy function currently does
     `escape(unescape(x), unescaped_output_length)`. It becomes
     `url_escape(ceh, url_unescape(ceh, x))`. The decoded length still carries through,
     because `url_escape` passes `s.size()`, which equals `out_len`.
   - Check: `curl_easy_escape`'s length parameter is `int`. Keep the existing
     `static_cast<int>` and don't add a new range check; that's out of scope.
2. **The `CURL*` handle.** In both `build_cmr_query_url` and
   `build_cmr_query_url_old_rpath_format`:
   - Replace `CURL *ceh = curl_easy_init(); … curl_easy_cleanup(ceh);` with
     `std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> ceh(curl_easy_init(), &curl_easy_cleanup);`.
   - Throw `BESInternalError` if it's null.
   - The existing comment "This easy-handle is only created so we can use the
     curl_easy_escape()…" stays.
3. **Static cache.** `get_cmr_search_endpoint_url()` keeps its signature. It drops
   `static string cmr_search_endpoint_url` and the `empty()` guard, and returns
   `BESUtil::assemblePath(...)` of the two `TheBESKeys::read_string_key` results directly.
   - Update the Doxygen comment, which currently says the URL is read once and cached.
   - Cost: two key lookups per CMR query. That's negligible next to the HTTP request that
     always follows. When the CMR cache (`NGAP.UseCMRCache`) hits, this function isn't
     called at all.
4. **New test** in `NgapApiTest.cc`: `cmr_endpoint_reads_keys_each_call`.
   - Set `NGAP.cmr_host_url` to `https://cmr.example` with
     `TheBESKeys::TheKeys()->set_key`.
   - Call `build_cmr_query_url` on a current-format path and assert the result starts with
     `https://cmr.example/search/granules.umm_json_v1_4?`.
   - Restore the original value: read it with `TheBESKeys::read_string_key` first and put it
     back in a `try`/`catch(...)` + rethrow, so later tests see the default host either way.
   - Register the test in the suite. It should fail on the current code if run after another
     test has already filled the cache, so register it *after* `resty_path_to_cmr_query_test_01`.
5. **Header.**
   - Remove `#include <vector>`, `#include <map>`, and the `namespace http { class url; }`
     block from `NgapApi.h`.
   - Keep `<string>` and `<tuple>`, which `DataAccessUrls` uses.
   - Add `#include <vector>` to `NgapOwnedContainer.cc`, next to its `<string>` include,
     and to `NgapApiTest.cc`, next to its standard includes.

**Completeness checks** (counts for `modules/dmrpp_module/ngap_container/NgapApi.cc`; the
"before" numbers are as of commit 24e22d1d4):

| Pattern | Before | After |
|---|---|---|
| `curl_easy_escape(` / `curl_easy_unescape(` calls (code, not comments) | 5 / 3 | 1 / 1 (inside the helpers only) |
| `curl_free` | 8 | 0 (replaced by `unique_ptr` deleters; `&curl_free` appears only as a deleter) |
| `curl_easy_cleanup(ceh)` | 2 | 0 (`&curl_easy_cleanup` appears only as a deleter) |
| `static string` | 1 | 0 |

- Recount with `grep -n` and list every remaining match in the PR description.
- For `NgapApi.h`, `grep -nE '<vector>|<map>|class url' NgapApi.h` returns no matches.
- Check every file that includes `NgapApi.h` (`grep -rln '"NgapApi.h"' modules/`; today
  that's `NgapOwnedContainer.h`, `NgapOwnedContainer.cc` via that header, `NgapApiTest.cc`,
  and `NgapOwnedContainerTest.cc` via `NgapOwnedContainer.h`). Each one must compile; the
  build confirms this.
- Each of these files must also include every standard header it uses directly.
  `NgapOwnedContainer.h` already includes `<map>`; `NgapOwnedContainerTest.cc` uses
  `map` and gets it from `NgapOwnedContainer.h`, which isn't affected.

## Verification

- `make check` in `modules/dmrpp_module/ngap_container/unit-tests` passes. All five
  `resty_path_to_cmr_query_*` tests still pass, which shows FR-001–FR-004 query strings are
  unchanged byte for byte.
- The new `cmr_endpoint_reads_keys_each_call` test should fail on the pre-change code and
  pass after. Confirm this by running it against the old `NgapApi.cc` once.
- `make check` from the top of the build tree, and `make distcheck`.
- Build on Linux (GCC) and macOS (Apple Clang). This matters for the include removal,
  because transitive includes differ between standard libraries. Something that compiles
  on libc++ may not on libstdc++, and the reverse.
- `--enable-asan` build of the unit tests, to confirm there are no leaks or double frees
  from the `unique_ptr` deleters.
- The null-return branches can't be forced in a unit test. libcurl returns null only on
  allocation failure. Note this as untested in the PR rather than adding a mocking layer.

## Risks

- **Include removal.** A file could depend on `<vector>` or `<map>` arriving through
  `NgapApi.h` on one platform only. The two-platform build is the guard.
- **Deleter type.** `decltype(&curl_free)` and `decltype(&curl_easy_cleanup)` are
  function-pointer types. If either function is a macro on some libcurl version, taking its
  address fails to compile. Neither is today, but if one is, use a small functor struct
  instead.
- **Merge overlap with BUG-001.** Both change `NgapApi.cc`, though not the same functions:
  BUG-001 touches the URL filters, the trim, and the header's private statics. Land one
  first and rebase the other. If both edit `NgapApi.h`, the private-static block is the
  likely place for a conflict.
- **Key reads per call.** If `TheBESKeys` lookups were ever slow (for example, re-reading
  the config file), removing the cache would show up in profiling. They're in-memory map
  lookups today.

## Open questions

- None that block the task.
