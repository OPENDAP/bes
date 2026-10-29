# Task Plan: Unit tests for the RelatedUrls selection rules in `get_urls_from_granules_umm_json_v1_4`

**Task:** TASK-003
**Status:** Draft
**Created:** 2026-10-29

## Summary

This task adds CppUnit tests for every FR-007–FR-010 selection clause in
`NgapApi::get_urls_from_granules_umm_json_v1_4`, plus one test for the "`hits` > 1 uses
`items[0]`" behavior. Every new test passes on the current code. No production code changes.

## Motivation

- **Gaps in the existing tests.** `NgapApiTest` covers the error paths (no hits, non-array
  `items` / `RelatedUrls`, missing `RelatedUrls` / `URL` / `Type`), three real-world fixtures,
  and the BUG-001 suffix cases. Most selection clauses have no test of their own: "first
  match", "no `Subtype`", `http://`, the s3credentials `Type` rule, and FR-010's partial
  results. The real-world fixtures pass whether or not those clauses hold.
- **Lines 433–435 split off.** The question about these lines (`mitr` after
  `FindMember("umm")`) is a real defect. When `"umm"` is missing, `mitr == MemberEnd()`
  and line 435 dereferences it. If `items[0]` is `{}`, rapidjson keeps no member array, so
  the iterator wraps a null pointer and the dereference crashes. Otherwise it reads past the
  member array. A test for this can't pass without a code fix, so it goes in a separate bug
  (see Out of scope), not in this task.

## Related requirements / constraints

- **FR-007** (Approved): https data URL. It must be the first `GET DATA` entry with no
  `Subtype`, an `http://` or `https://` scheme, and no `.xml` / `.dmrpp` suffix.
- **FR-008** (Approved): S3 data URL. Same rule, with the `s3://` scheme.
- **FR-009** (Implemented): s3credentials URL. It must be the first
  `VIEW RELATED INFORMATION` entry with an http(s) scheme and the `s3credentials` suffix.
- **FR-010** (Implemented): no https URL is an error, but a missing S3 or s3credentials URL
  is not.
- No NFR, UC, IC, or ADR entries apply.

## Scope

**In scope:**
- `modules/dmrpp_module/ngap_container/unit-tests/NgapApiTest.cc`: new test methods and
  their `CPPUNIT_TEST` lines.
- `modules/dmrpp_module/ngap_container/unit-tests/cmr_json_responses/`: the new fixtures
  listed below. `Makefile.am` ships this whole directory (`EXTRA_DIST = ... cmr_json_responses`),
  so no Makefile change is needed.

**Out of scope:**
- **Malformed CMR responses.** These crash on the current code instead of throwing:
  - parse errors
  - missing or non-int `"hits"`
  - missing `"items"` (`operator[]` asserts in builds without `NDEBUG`, which includes this
    developer build)
  - empty `items`
  - `items[0]` not an object
  - missing or non-object `"umm"` (`NgapApi.cc:433-435`, HYRAX-2316)
  - non-string `Type` / `URL`

  They are to be logged as a separate bug (via `/fix-bug`), where each gets a regression
  test that fails before the fix.
- **Missing `Type`.** Whether it should skip the entry instead of failing is the TASK-001
  open question.
- **`Subtype` on the s3credentials entry.** FR-009 doesn't mention it. The current code
  ignores `Subtype` there, and no test pins that down until FR-009 says what it should do.
- **Production code.** Any change to `NgapApi.cc`, `NgapApi.h`, or other files.
- **TASK-002's tests.** TASK-002 is on its own branch and not merged, and its tests aren't
  touched.

## Approach

Branch from `claude`, which already has TASK-001 and BUG-001.

1. **Fixtures.** Use the shape of the BUG-001 fixtures: `hits`, `took`, and `items[0].umm.RelatedUrls`.
   - Every `RelatedUrls` entry has both `Type` and `URL`. An entry missing either throws
     (see `related_url_wo_URL` / `related_url_wo_Type`), which would hide the rule under test.
   - Use `data.example` hosts and the bucket name `bucket`, so expected values are short and
     clearly synthetic.
   - Check that each new file is valid JSON (`python3 -I -m json.tool <file>`).

   | Fixture | `RelatedUrls`, in order | Expected (https, S3, s3credentials) |
   |---|---|---|
   | `first_match_wins.json` | GET DATA `https://data.example/a.nc`; GET DATA `https://data.example/b.nc`; GET DATA `s3://bucket/a.nc`; GET DATA `s3://bucket/b.nc`; VRI `https://a.example/s3credentials`; VRI `https://b.example/s3credentials` | `…/a.nc`, `s3://bucket/a.nc`, `https://a.example/s3credentials` |
   | `subtype_skipped.json` | GET DATA + `Subtype` `VIRTUAL COLLECTION` `https://data.example/sub.nc`; same with `s3://bucket/sub.nc`; GET DATA `https://data.example/a.nc`; GET DATA `s3://bucket/a.nc` | `…/a.nc`, `s3://bucket/a.nc`, empty |
   | `http_scheme.json` | GET DATA `http://data.example/a.nc`; VRI `http://data.example/s3credentials` | `http://data.example/a.nc`, empty, `http://data.example/s3credentials` |
   | `wrong_type_or_scheme.json` | `GET RELATED VISUALIZATION` `https://data.example/browse.nc`; GET DATA `ftp://data.example/a.nc`; `USE SERVICE API` `https://x.example/s3credentials`; VRI `s3://bucket/s3credentials`; VRI `https://data.example/s3credentials.html`; GET DATA `https://data.example/a.nc` | `…/a.nc`, empty, empty |
   | `https_only.json` | GET DATA `https://data.example/a.nc` | `…/a.nc`, empty, empty |
   | `https_and_s3_only.json` | GET DATA `https://data.example/a.nc`; GET DATA `s3://bucket/a.nc` | `…/a.nc`, `s3://bucket/a.nc`, empty |
   | `multiple_hits.json` | `hits: 2`; `items[0]`: GET DATA `https://data.example/first.nc`; `items[1]`: GET DATA `https://data.example/second.nc` | `…/first.nc`, empty, empty |

   (VRI = `VIEW RELATED INFORMATION`.)

   - **`first_match_wins.json`:** the https entries come before the S3 ones. The loop
     checks each entry against all three rules, so the order across kinds doesn't matter.
     The order within each kind is what's being tested.
   - **`wrong_type_or_scheme.json`:** the valid https entry comes *last*, so a test that
     passes shows the earlier entries were skipped and not merely never reached.
2. **Tests.** Add one `static void test_get_urls_from_granules_umm_json_v1_4_<fixture-stem>()`
   per fixture.
   - Each one calls `NgapApi::get_urls_from_granules_umm_json_v1_4("placeholder", json)`
     and uses `CPPUNIT_ASSERT_EQUAL` on all three tuple members, including the ones expected
     to be empty. The style is the same as the BUG-001 tests.
   - Register each one in `CPPUNIT_TEST_SUITE` after
     `test_get_urls_from_granules_umm_json_v1_4_rejected_suffixes`.
3. **Confirm each test exercises its rule.** For each selection-rule test, temporarily
   break the matching check in `NgapApi.cc`, check that the test fails, then revert.
   - Examples: drop `noSubtype`; drop the `http://` alternative; return the last match
     instead of the first.
   - Record which mutation each test caught in the PR description. Don't commit the
     mutations.

**Completeness check: every clause has a test.** Fill in the "Test" column when the task is
done; every row must name at least one registered test.

| Clause | Test |
|---|---|
| FR-007: `Type` is `GET DATA` | `wrong_type_or_scheme` |
| FR-007: no `Subtype` | `subtype_skipped` |
| FR-007: `https://` accepted | existing (`minimal_json`, `lpdaac`, `podaac`) |
| FR-007: `http://` accepted | `http_scheme` |
| FR-007: other schemes rejected | `wrong_type_or_scheme` |
| FR-007: not ending in `.xml` / `.dmrpp` | existing (`rejected_suffixes`, `dmrpp_not_suffix`, `xml_not_suffix`) |
| FR-007: first match | `first_match_wins` |
| FR-008: `Type` `GET DATA`, `s3://` | existing (`lpdaac`, `podaac`) |
| FR-008: no `Subtype` | `subtype_skipped` |
| FR-008: not ending in `.xml` / `.dmrpp` | existing (`rejected_suffixes`, `dmrpp_not_suffix`) |
| FR-008: first match | `first_match_wins` |
| FR-009: `Type` `VIEW RELATED INFORMATION` | `wrong_type_or_scheme` (`USE SERVICE API` rejected) |
| FR-009: http(s) scheme | `http_scheme`, `wrong_type_or_scheme` (`s3://` rejected) |
| FR-009: ends in `s3credentials` | `wrong_type_or_scheme` (`.html` suffix rejected) |
| FR-009: first match | `first_match_wins` |
| FR-010: no https URL is an error | existing (`no_valid_related_url`, which has a valid S3 URL) |
| FR-010: missing S3 / s3credentials doesn't fail | `https_only`, `https_and_s3_only` |
| (none): `hits` > 1 uses `items[0]` | `multiple_hits` |

**Completeness check: every fixture is used.** The fixture loop from the TASK-001 plan
(`plans/task-ngapapi-unused-fixtures-plan.md`) prints nothing.

## Verification

- `make check` in `modules/dmrpp_module/ngap_container/unit-tests` passes. `NgapApiTest`
  reports 7 more tests than before: 20 on `claude` today, so 27.
- Step 3's mutation runs each make the matching test fail.
- Both completeness checks above are clean.
- `make check` from the top of the build tree, and `make distcheck`. Distcheck currently
  fails in `cmdln/tests` before it reaches `modules/`; see Open questions.

## Risks

- **A test fails on the current code.** That would mean a reading of FR-007–FR-010 doesn't
  match the code, which is a bug. Report it and log it with `/fix-bug`; don't change the
  expected value to match the code.
- **The `multiple_hits` test pins down an assumption no requirement states.** If CMR can
  return several items for one `granule_ur` and the intended behavior is something else,
  this test will need to change along with a new requirement.
- **Merge overlap with TASK-002.** TASK-002 also adds a test and a registration line to
  `NgapApiTest.cc`, near the `resty_path` tests, a different spot from this task's. A
  textual conflict is unlikely, but whichever lands second gets rebased.

## Open questions

- **Should `multiple_hits` stay?** Keeping the `items[0]` behavior as a test without a
  requirement backing it is a judgment call. It doesn't block the task. If you'd rather back
  it with a requirement first, run `/new-requirement` and cite the new FR here.
- **Distcheck fails today** in `cmdln/tests` (tests 48, 51, 61), which is unrelated to this
  task. It blocks the `make distcheck` step in Verification until it's fixed or logged.
