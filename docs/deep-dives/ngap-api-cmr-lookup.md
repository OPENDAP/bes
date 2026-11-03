# NgapApi: REST path → CMR query → data-access URLs

**Date:** 2026-10-29 · **Commit:** 24e22d1d4
**Question:** What do `modules/dmrpp_module/ngap_container/NgapApi.cc` / `NgapApi.h`
do, how are they used and tested, and where are the risk areas?

## What it is

`ngap::NgapApi` is a class made up only of static methods (the header notes it "would be better
implemented as a set of functions in a namespace", jhrg 10/8/25). Its one public entry point is
`convert_ngap_resty_path_to_data_access_urls(rest_path)`. That method returns
`DataAccessUrls = std::tuple<string,string,string>` holding
`(https data URL, s3:// data URL, s3credentials (TEA) endpoint URL)`.

There is one production caller:
`NgapOwnedContainer::build_data_urls_to_daac_bucket()` (`NgapOwnedContainer.cc:234`). It wraps
the call in a per-process `MemoryCache` keyed on `rest_path + ':' + UID`. That cache is enabled
by `NGAP.UseCMRCache`. A retired caller in `retired/modules/builddmrpp_module/` still uses the
old name `convert_ngap_resty_path_to_data_access_url` (singular) and isn't built.

## Control flow

1. **`build_cmr_query_url(rest_path)`**: converts the REST path to a CMR query URL.
   - If the path contains `/providers/`, it uses the legacy path
     `build_cmr_query_url_old_rpath_format`:
     `/providers/<P>/collections/<entry_title>/granules/<granule_ur>` or
     `/providers/<P>/concepts/<concept_id>/granules/<granule_ur>`. Each value is first
     `curl_easy_unescape`d and then `curl_easy_escape`d, which avoids double encoding
     (HYRAX-2148). The result is a query with `provider=` + `entry_title=` or
     `collection_concept_id=` + `granule_ur=`.
   - Otherwise it uses the current format:
     `/collections/<concept_id>[/<ShortName.Version>]/granules/<granule_ur>`. Anything after
     the first `/` in the collection segment is dropped. Values are escaped only (no unescape
     first). The result is a query with `collection_concept_id=` + `granule_ur=`.
   - The endpoint comes from `get_cmr_search_endpoint_url()`, which reads `NGAP.cmr_host_url`
     and `NGAP.cmr_search_endpoint_path` once. The defaults are `https://cmr.earthdata.nasa.gov`
     and `/search/granules.umm_json_v1_4`. The value is cached in a function-local `static string`.
   - Malformed paths throw `BESSyntaxUserError`.
2. **`curl::http_get(...)`** with EDL auth headers (`curl::add_edl_auth_headers`). On
   `http::HttpError`: a 5xx gets a service-chaining message
   (`add_service_chaining_message`); other errors get a prefix. The error is then rethrown.
3. **`get_urls_from_granules_umm_json_v1_4(rest_path, json)`** parses the CMR response with
   rapidjson:
   - `hits < 1` throws `BESNotFoundError`.
   - It reads `items[0].umm.RelatedUrls[]` and takes the first match for each URL:
     - `get_data_http_url`: `Type == "GET DATA"`, no `Subtype`, starts with `http(s)://`,
       and the URL does not contain `.xml` or `.dmrpp`.
     - `get_data_s3_url`: the same rules, but the URL must start with `s3://`.
     - `get_s3credentials_url`: `Type == "VIEW RELATED INFORMATION"`, starts with
       `http(s)://`, and ends in `s3credentials`.
   - Only the https data URL is required. If it's missing, the method throws
     `BESInternalError`.
4. Back in the entry point:
   - If the s3 URL or the TEA URL is missing, it writes an `INFO_LOG` line containing
     "SERVICE CHAIN WARNING".
   - It then removes a trailing `.dmrpp` from both data URLs (kln 6/6/25).

## Tests

`unit-tests/NgapApiTest.cc` (CppUnit, part of `make check`) covers:
- Both path formats (`resty_path_to_cmr_query_test_01..04`), plus pre-encoded legacy input.
- These JSON error cases: no hits, `items` not an array, no `RelatedUrls`, `RelatedUrls` not
  an array, no valid related URL, a related URL without `URL`.
- The minimal, LP DAAC, and PO.DAAC fixtures in `cmr_json_responses/`.

What the tests don't reach:
- `convert_ngap_resty_path_to_data_access_urls` itself. It needs a live CMR, so there is no
  test of the HTTP-error handling or the `.dmrpp` stripping in place.
- `test_dmrpp_is_removed_from_data_url` copies the stripping logic into the test body. It does
  not call `NgapApi`, so it can't catch a defect in the real code (see below).
- `related_url_wo_Type.json` / `related_urls_wo_Type.json` exist as fixtures, but no
  registered test uses them.
- `signed_url_is_expired_test` lives in this suite but tests `http::url`, not `NgapApi`.

## Risk areas / observations to verify

1. **Wrong string in the s3 `.dmrpp` strip** (`NgapApi.cc:533`):
   `data_s3_url.erase(data_access_url.size() - suffix.size())` uses the *https* URL's length.
   By the time this line runs, the https URL may already have been shortened. If it fired, it
   would truncate the s3 URL at the wrong offset, or throw `std::out_of_range`.
2. **The `.dmrpp` strip looks unreachable.** Both `get_data_http_url` and `get_data_s3_url`
   reject any candidate with `rfind(".dmrpp") != npos`, meaning `.dmrpp` *anywhere* in the URL.
   So no URL that reaches lines 524–534 can end in `.dmrpp`. Either the strip block is dead
   code, or the filter is stricter than intended. That makes item 1 latent rather than live.
   Confirm which behavior is wanted.
3. **"Ends in" vs. "contains".** The comments say a URL "cannot end in .xml or .dmrpp". The
   check rejects those strings anywhere in the URL. For example, a path segment such as
   `/foo.xml.d/x.nc` would be rejected.
4. **Unchecked rapidjson access in release builds.** `configure.ac` defines `NDEBUG`, and
   `RAPIDJSON_ASSERT` defaults to `assert`, so these cases have undefined behavior (likely a
   crash) in production instead of throwing a `BESError`:
   - Parse errors aren't checked.
   - A missing `"hits"` and `GetInt()` on a non-int aren't checked.
   - `items[0]` on an empty array isn't checked.
   - A missing `"umm"` isn't checked: `mitr` is dereferenced at line 428 with no `MemberEnd`
     test.
   - `GetString()` on a non-string `Type` or `URL` isn't checked.

   The input comes from CMR over the network, for example an HTML error page with a 200
   status. Every fixture contains `"hits"`, so the tests never reach this case.
5. **Off-by-one in the length checks.**
   - `collection_index <= provider_index + 1` (line 114) and
     `granule_index <= collection_index + 1` (line 133) reject a value of exactly one
     character, though the comments say one character is the minimum.
   - The same applies to `granules_key_index <= collections_index + 1` at line 241.
   - Real IDs are longer, so the practical impact is probably nil.
6. **Misleading error text.** At line 225 the message prints `provider_index + 1`. On that
   path `provider_index == npos`, so the message reads "expected at least 0".
7. **Inconsistent decoding between formats.** The legacy format unescapes and then escapes.
   The current format only escapes, so a percent-encoded granule UR in a new-style path would
   be double-encoded. This may be intentional, because the OLFS may already decode; verify.
8. **Exception-class choices.** A non-array `RelatedUrls` throws `BESNotFoundError` (404-ish),
   while the analogous `items` case throws `BESInternalError`.
9. **Null handling for libcurl handles.** `curl_easy_init()` and `curl_easy_escape()` can
   return null; the code doesn't check, and `.append(nullptr)` is undefined behavior.
   This is unlikely to happen in practice.
10. **Function-local static cache** in `get_cmr_search_endpoint_url()`. It is read once per
    process, so changing BES keys after the first call (for example in tests) has no effect.
    The empty-check-then-assign isn't thread-safe, but the call path is single-threaded today.
11. **Header hygiene.**
    - `<vector>` and `<map>` are included but unused.
    - `namespace http { class url; }` is forward-declared but unused.
    - Default/deleted constructors are declared on a class that is never instantiated.

## Requirements cross-check

There is nothing to check against yet. `docs/requirements/*`, `docs/constraints/*`,
`docs/decisions/DECISIONS.md`, the BUG log, and the TASK log contain only `_EXAMPLE_`
placeholders. The behavior described above is undocumented as requirements. The candidates
for formalizing are:
- The REST-path grammar (both formats).
- The RelatedUrls selection rules.
- "https data URL required; s3/TEA optional".
- The `INFO_LOG` service-chaining warning.
- CMR cache keying by UID.
