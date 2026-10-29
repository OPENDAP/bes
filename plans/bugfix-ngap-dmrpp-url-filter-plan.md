# Bugfix Plan: NGAP data-URL filter rejects `.xml`/`.dmrpp` anywhere, and the `.dmrpp` trim on the S3 URL uses the wrong length

**Bug:** BUG-001
**Status:** Draft
**Created:** 2026-10-29

## Summary

The CMR `RelatedUrls` filter in `NgapApi.cc` rejects data URLs that contain `.xml` or `.dmrpp`
anywhere in the URL. FR-007/FR-008 reject a URL only when it *ends in* one of them. The
post-filter `.dmrpp` trim is dead code, and it uses the wrong string's length for the S3
URL. This fix does three things:
- Changes the filter to an ends-with test.
- Keeps the `.dmrpp` trim block as a defensive measure, fixes line 533 to use the S3 URL's
  own length, and makes the block testable.
- Makes `NgapOwnedContainer` add `.dmrpp` to the https and S3 data URLs the same way, so the
  relaxed filter can't produce a wrong DMR++ URL.

## Root cause

See the BUG-001 entry in `docs/bugs/BUG-LOG.md`. In brief:
1. `get_data_http_url` and `get_data_s3_url` use `rfind(x) == npos`, which means "does not
   contain x". It should mean "does not end in x".
2. `NgapApi.cc:533` erases from `data_s3_url` at an offset computed from
   `data_access_url.size()`.
3. The trim block (`NgapApi.cc:524-534`) can't fire, because the filter has already rejected
   every URL ending in `.dmrpp`. The rejection holds under both the current rule and the
   FR-007/FR-008 rule.

`NgapOwnedContainer.cc:527` decides whether to append `.dmrpp` with a contains test. Once (1)
is fixed, that test becomes wrong for URLs such as `granule.dmrpp.nc`.

## Related requirements

- **FR-007** (Approved): https data URL selection. Violated because URLs that contain, but
  don't end in, `.xml` or `.dmrpp` are rejected.
- **FR-008** (Approved): S3 data URL selection. Violated in the same way. Defect 2 would also
  corrupt the selected value if the trim block were reachable.
- **FR-011** (Implemented): the DMR++ URL is the data URL plus `.dmrpp`. It isn't violated
  today. Fixing FR-007 alone would break it for URLs such as `granule.dmrpp.nc`, and step 3
  below keeps it holding.

## Fix approach

1. **Ends-with filter (FR-007, FR-008).**
   - In `NgapApi.cc`, add a file-local `ends_with(const std::string &s, const std::string &suffix)`
     helper in an anonymous namespace. It's needed because `std::string::ends_with` is C++20
     and the project targets C++14.
   - In `get_data_http_url` and `get_data_s3_url`, replace the two `rfind(...) == npos` tests
     with `!ends_with(candidate_url, ".xml") && !ends_with(candidate_url, ".dmrpp")`.
   - Update the comments at lines 309 and 341 to match.
   - Optionally, use the same helper in place of the hand-rolled suffix test in
     `get_s3credentials_url` (lines 371/375). It behaves the same; this just shares one helper.
2. **Keep the trim as a defensive measure and fix line 533 (FR-008).** This was decided
   2026-10-29.
   - Move `NgapApi.cc:524-534` into a private static, for example
     `static void strip_dmrpp_suffix(std::string &url)`. Declare it in `NgapApi.h` next to the
     other private statics, so `NgapApiTest` (already a `friend`) can call it.
   - Call it on both `data_access_url` and `data_s3_url`. Each call uses the size of the URL
     it is trimming, so the wrong-operand mistake from line 533 can't recur.
   - Implement it with the same `ends_with` helper from step 1.
   - Add a comment saying the block is defensive. Under FR-007/FR-008 the filter already
     rejects URLs ending in `.dmrpp`, so the block shouldn't fire. It guards against a future
     change to the filter.
3. **Consistent `.dmrpp` append in the container (FR-011).**
   - In `NgapOwnedContainer::dmrpp_read_from_daac_bucket` (`NgapOwnedContainer.cc:526-530`),
     append `.dmrpp` to the https URL unconditionally, the same way the S3 URL is handled at
     line 532.
   - The FR-007 filter now guarantees the https URL doesn't end in `.dmrpp`.
   - Keeping the conditional would need an ends-with test anyway, and the "already present"
     case it guarded can no longer occur.
4. **Tests.**
   - Replace `test_dmrpp_is_removed_from_data_url`, which tests a copy of the logic, with
     tests that call `get_urls_from_granules_umm_json_v1_4`.
   - Add the new JSON fixtures in `unit-tests/cmr_json_responses/`, and list them in that
     directory's `Makefile.am` `EXTRA_DIST` if it lists fixtures. Check this, so that
     `make distcheck` still passes.

Why this scope: the three defects all come from one design gap. "What does a selected data URL
look like" was enforced with a loose test in one place, patched over with a trim in another,
and assumed with a third test in the container. Fixing only line 533 would leave the
FR-007/FR-008 violation in place and the code still dead. Fixing only the filter would bring
in the container regression from step 3.

## Regression risk

- **Wider filter.** URLs with `.xml` or `.dmrpp` in the middle (for example
  `granule.dmrpp.nc`, `v1.xml.d/…`) will now be selected. That is the intended FR-007/FR-008
  behavior. A DAAC record that relied on the old rejection to skip such an entry would now
  pick it up. The LP DAAC `*.cmr.xml` case that motivated the `.xml` rule ends in `.xml`, so
  it's still rejected; the existing `test_get_urls_from_granules_umm_json_v1_4_lpdaac` covers
  it.
- **Container change.** `dmrpp_read_from_daac_bucket` is the only consumer of the https URL
  for building the DMR++ URL. `extract_s3_data_urls_from_dmrpp` and the signing path at
  `NgapOwnedContainer.cc:723` don't depend on it.
- **CMR cache.** `d_cmr_mem_cache_urls` holds `DataAccessUrls` per process. It's in-memory
  only, so a restart clears it and no stale-cache migration is needed.
- **Existing coverage.** These `NgapApiTest` tests exercise the filter and must still pass:
  `minimal_json`, `lpdaac`, `podaac`, and `no_valid_related_url`. `NgapOwnedContainerTest`
  should still pass. It constructs `DataAccessUrls` directly at lines 545 and 573; check
  whether any assertion there expects the conditional-append behavior.

## Verification

Each step below is a reproduction step from BUG-001, run again as a regression test in
`NgapApiTest`. Each should fail before the fix and pass after.
1. A UMM-G fixture whose only `GET DATA` https entry is
   `https://data.example/granule.dmrpp.nc`.
   - Expect `get<0>` to equal that URL. Before the fix it throws `BESInternalError`.
2. The same with `https://data.example/v1.xml.d/granule.nc`.
3. An S3 variant: an https `GET DATA` entry plus `s3://bucket/granule.dmrpp.nc`.
   - Expect `get<1>` to equal the S3 URL. Before the fix it's empty.
4. Negative cases that must stay rejected: `…/granule.nc.dmrpp` and `…/granule.cmr.xml` for
   both https and S3. Expect them not to be selected.
5. Rewrite `test_dmrpp_is_removed_from_data_url` to call `NgapApi::strip_dmrpp_suffix` instead
   of a copy of the logic.
   - Include the BUG-001 step-3 inputs, trimming the https URL first and then the S3 URL.
     Expect `https://data.example/a.nc` and `s3://some-bucket/long/prefix/a.nc`. The
     pre-fix logic gives `s3://some-bucket/lo`, so run the test against the old code once to
     confirm it fails.
   - Keep the existing no-suffix and `.dmrpp`-in-the-middle cases.
6. In `NgapOwnedContainerTest`, if `dmrpp_read_from_daac_bucket`'s URL construction can be
   reached without network access, assert that a data URL of
   `https://data.example/granule.dmrpp.nc` yields the DMR++ URL
   `https://data.example/granule.dmrpp.nc.dmrpp`. If it can't be reached, note the gap in
   the PR. Don't add a network-dependent test.
7. `make check` from the top of the build tree, then `make distcheck`.

## Out of scope

- The other risks recorded in `docs/deep-dives/ngap-api-cmr-lookup.md`, which are separate
  bugs or tasks:
  - unchecked rapidjson access in release builds
  - off-by-one length checks
  - double-encoding in the current path format
  - the misleading error text
  - header hygiene
- URLs carrying a query string (for example `…/a.nc.dmrpp?x=1`). Neither the old nor the new
  rule handles these, and FR-007/FR-008 don't mention them.
- The legacy `retired/modules/builddmrpp_module/` caller.
