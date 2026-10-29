# Bugfix Plan: `get_urls_from_granules_umm_json_v1_4` crashes or misreports on malformed CMR responses instead of throwing

**Bug:** BUG-002
**Status:** Draft
**Created:** 2026-10-29

## Summary

`NgapApi::get_urls_from_granules_umm_json_v1_4` and its three `RelatedUrls` helpers rely on
rapidjson asserts that production builds compile out. A malformed CMR response therefore
crashes the `beslistener`, or is reported as "granule not found", instead of throwing. This
fix adds an explicit check before every rapidjson access whose precondition isn't already
checked. Each check throws `BESInternalError`.

## Root cause

See the BUG-002 entry in `docs/bugs/BUG-LOG.md`. Line numbers are as of commit a37efa713.
These preconditions aren't checked:
1. The parse succeeded and the document is an object (`NgapApi.cc:414`).
2. `hits` exists and is an int (`NgapApi.cc:416-417`). In release builds a missing `hits`
   reads as 0, which gives a false not-found.
3. `items` exists (`NgapApi.cc:423`), using `FindMember` rather than `operator[]`, and is
   non-empty (`NgapApi.cc:430`).
4. `items[0]` is an object, `umm` exists, and `umm` is an object (`NgapApi.cc:431-433`;
   HYRAX-2316).
5. Each `RelatedUrls` entry is an object, and its `Type` and `URL` are strings
   (`NgapApi.cc:294-385`).

## Related requirements

- **No requirement covers malformed CMR responses.** The correct behavior (throw
  `BESInternalError`, never crash) was decided on 2026-10-29 for this bug but isn't written
  down. Consider `/new-requirement` for an FR along the lines of "a CMR response that isn't
  a well-formed UMM-G granule document is reported as an internal error." This plan doesn't
  depend on it.
- **FR-006** (Implemented): zero hits is reported as not-found. Release builds also report a
  non-JSON response or a missing `hits` as not-found. After the fix, only a present `hits < 1`
  takes that branch.
- **FR-010** (Implemented): no https URL is an error. Unchanged. A response that passes every
  new check but has no usable URL still throws "Failed to locate a data URL".

## Fix approach

All changes are in `modules/dmrpp_module/ngap_container/NgapApi.cc`. Every new failure throws
`BESInternalError(msg, __FILE__, __LINE__)`. Each `msg` names the missing or mistyped element
and includes `rest_path` where it's in scope, the same way the existing
"did not contain the data URL information" message does.

1. **Parse** (after line 414). If `cmr_granule_response.HasParseError()`, throw with the
   parse error text. Get it from `rapidjson::GetParseError_En(GetParseError())`
   (`rapidjson/error/en.h`) plus `GetErrorOffset()`. Then, if
   `!cmr_granule_response.IsObject()`, throw.
2. **`hits`** (lines 416–417). Replace `operator[]("hits")` with `FindMember`. Throw if the
   member is missing or `!IsInt()`. The existing `hits < 1` → `BESNotFoundError` stays as is
   (FR-006).
3. **`items`** (lines 423–430).
   - Replace `operator[]("items")` with `FindMember`.
   - Throw if `items` is missing. Use the existing "did not contain the data URL information"
     message, so `items_not_an_array` keeps its behavior.
   - Keep the existing `!IsArray()` check, and add an `Empty()` check after it.
   - Before calling `FindMember`, check `items[0].IsObject()`.
4. **`umm`** (lines 431–433). Throw if `mitr == items_obj.MemberEnd()` or
   `!mitr->value.IsObject()`. Remove the `// BUG? Could mitr be null here?` note at line 432,
   since the code now answers it, and cite BUG-002 / HYRAX-2316 in the commit message instead.
5. **The `RelatedUrls` helpers** (lines 294–385). The three helpers repeat the same
   URL/Type lookup, so add one file-local helper (in the anonymous namespace from BUG-001):
   `string get_string_member(const rapidjson::Value &obj, const char *name)`. It:
   - throws if `!obj.IsObject()`, if the member is missing, or if it's `!IsString()`;
   - keeps the existing message text for the missing case ("The umm/RelatedUrls element
     does not contain the URL object" / "...the Type object"), so `related_url_wo_URL` and
     `related_url_wo_Type` keep their behavior;
   - returns the string.

   Each helper then calls it twice instead of its current eight lines.
   - **Order:** today each helper looks up `URL` first and then `Type`. Keep that order, so
     which message an entry missing both gets doesn't change.
   - **Subtype:** the `FindMember("Subtype")` check is a presence test only and is already
     safe on an object, so it stays.

**Why this scope.** The bug is one pattern: an unchecked rapidjson precondition. It repeats
across one function and its three helpers, so the fix covers all of them in one change.
- **Narrower** (just line 433): that would leave nine other crash or false-not-found paths
  from the same input.
- **Broader** (a schema validator, `RAPIDJSON_ASSERT` mapped to a throw project-wide, or a
  new HTTP status for the client): that would change behavior in every rapidjson user in the
  BES. It's out of scope; see below.

## Regression risk

- **Existing error tests keep their exception types.**
  - `no_hits` → `BESNotFoundError`
  - `items_not_an_array` → `BESInternalError`
  - `no_related_urls` → `BESInternalError`
  - `related_urls_not_an_array` → `BESNotFoundError` (the type question is a TASK-002
    out-of-scope item; unchanged here)
  - `related_url_wo_URL` / `related_url_wo_Type` → `BESInternalError`

  These tests check only the exception type, so the message changes in steps 1–3 are safe.
- **Well-formed responses don't change.** The `minimal_json`, `lpdaac`, `podaac`, and BUG-001
  fixtures must still give byte-identical tuples.
- **Strict `IsInt()` on `hits`.** If CMR ever sent `hits` as a value rapidjson stores only as
  `Int64` / `Uint64` (over 2^31), `IsInt()` would fail where `GetInt()` used to truncate.
  That's not a realistic value for a single-granule query. If it's a concern, use
  `IsInt64()` and `GetInt64()`.
- **Callers.** The function is private. Its only production caller is
  `convert_ngap_resty_path_to_data_access_urls` (`NgapApi.cc:538`), whose caller
  `NgapOwnedContainer::build_data_urls_to_daac_bucket` already lets `BESInternalError`
  propagate. The CMR memory cache stores only successful results, so nothing malformed can
  be cached.
- **Merge overlap.**
  - **TASK-002** (branch `task-002-ngapapi-cleanup`, unmerged) changes
    `build_cmr_query_url*` and the anonymous namespace in `NgapApi.cc`. A conflict is
    possible where both add helpers to that namespace; resolve it by keeping both.
  - **TASK-003** (planned) adds tests and registration lines in `NgapApiTest.cc` next to
    the ones this plan adds.

## Verification

Each item below is a reproduction case from BUG-002, added to `NgapApiTest` as a regression
test.
- **Fixtures:** the JSON is short, and case d. isn't JSON at all, so pass each input as an
  inline string literal, not a fixture file. This leaves the TASK-001 fixture-completeness
  check unaffected.
- **Names:** `test_get_urls_from_granules_umm_json_v1_4_malformed_<case>`.
- **Assertion:** each test uses `CPPUNIT_ASSERT_THROW(..., BESInternalError)`.

1. **Every case throws `BESInternalError`:** a. `items[0]` is `{}`; b. no `umm` among other
   members; c. `umm` is a number; d. `<html>503</html>`; e. no `hits`; f. `hits` is a string;
   g. no `items`; h. empty `items`; i. `items[0]` is a number; j. `Type` is `7`, and
   separately `URL` is `null`; k. a `RelatedUrls` entry is a number.
2. **Cases d. and e. are not `BESNotFoundError`.** `CPPUNIT_ASSERT_THROW` with
   `BESInternalError` already shows this, because both classes derive directly from
   `BESError` (`dispatch/BESNotFoundError.h:40`, `dispatch/BESInternalError.h:43`), so neither
   is caught as the other.
3. **The tests fail before the fix.**
   - Pre-fix, in this developer build, they abort the test program instead of failing an
     assertion. So run each one on its own against the old `NgapApi.cc`
     (`./NgapApiTest test_get_urls_from_granules_umm_json_v1_4_malformed_<case>`), confirm
     it aborts or segfaults, and record the result in the PR description.
   - After the fix, the whole suite runs to completion.
4. **ASan and release mode.** Run the unit tests in a build configured *without*
   `--enable-developer` (so `NDEBUG` is defined, as in production) and with `--enable-asan`.
   ASan/UBSan should report nothing for the new tests.
5. `make check` in `modules/dmrpp_module/ngap_container/unit-tests`, then from the top of the
   build tree, then `make distcheck`. Distcheck currently fails in `cmdln/tests` for an
   unrelated reason. That failure is logged separately and blocks this step until it's
   fixed.

## Out of scope

- **Mapping `RAPIDJSON_ASSERT` to a throwing macro project-wide.** It would affect every
  rapidjson user in the BES: `aws/SignedUrlCache.cc`, plus `FONcTransmitter.cc` and
  `history_utils.cc` in `fileout_netcdf`. That needs its own task or ADR.
- **The client-facing status.** Turning a malformed upstream reply into a 502 (for example
  via `http::HttpError`) instead of a 500 is a requirement question.
- **The `BESNotFoundError` vs. `BESInternalError` choice** for a non-array `RelatedUrls`.
- **Whether a missing `Type` should skip the entry** instead of failing (the TASK-001 open
  question).
- **A requirement for malformed-response handling.** That goes through `/new-requirement`;
  see Related requirements.
- **TASK-003's selection-rule tests.**
- **Unchecked JSON access elsewhere.** The s3credentials response parsing in
  `aws/SignedUrlCache.cc` and the `fileout_netcdf` history code haven't been checked for the
  same pattern. If it's needed, open a separate deep dive.
