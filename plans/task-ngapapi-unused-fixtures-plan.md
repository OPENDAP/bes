# Task Plan: Wire the unused NgapApi CMR fixtures into NgapApiTest

**Task:** TASK-001
**Status:** Draft
**Created:** 2026-10-29

## Summary

This task registers the existing but never-run test
`test_get_urls_from_granules_umm_json_v1_4_related_url_wo_Type`. It also deletes
`related_urls_wo_Type.json`, a misnamed duplicate of `no_related_urls.json`. Afterwards, every
fixture in `unit-tests/cmr_json_responses/` is used by a registered test.

## Motivation

The 2026-10-29 deep dive (`docs/deep-dives/ngap-api-cmr-lookup.md`) found two fixtures that
no registered test uses. A closer look found that one already has a test. The test was
written but left out of `CPPUNIT_TEST_SUITE` (`NgapApiTest.cc:356`). The other fixture
duplicates an existing one. Both fixtures arrived in commit 5d95f8940 (2025-06-09).

## Related requirements / constraints

- **FR-007**: an https data URL is selected only from an entry whose `Type` is `GET DATA`.
  The registered test pins down what happens to an entry with no `Type` at all: today it
  throws `BESInternalError`. It does not establish that throwing is the required behavior
  (see Open questions).

## Scope

**In scope:**
- `modules/dmrpp_module/ngap_container/unit-tests/NgapApiTest.cc`: add one `CPPUNIT_TEST(...)`
  line.
- `modules/dmrpp_module/ngap_container/unit-tests/cmr_json_responses/related_urls_wo_Type.json`:
  delete.

**Out of scope:**
- `test_dmrpp_is_removed_from_data_url`, which is covered by the BUG-001 plan.
- Tests that need a code fix first: unchecked rapidjson access, one-character values,
  the "expected at least 0" error text. Each becomes a BUG with its own regression test.
- Current-format double encoding. FR-004 covers only the legacy format, so the intended
  behavior has to be decided first.
- Tests for `convert_ngap_resty_path_to_data_access_urls`, which need a CMR stand-in.
- The cleanup items in TASK-002.

## Approach

1. In `NgapApiTest.cc`'s `CPPUNIT_TEST_SUITE` block, add
   `CPPUNIT_TEST (test_get_urls_from_granules_umm_json_v1_4_related_url_wo_Type);`
   directly after the `..._related_url_wo_URL` line.
2. Delete `cmr_json_responses/related_urls_wo_Type.json` with `git rm`.
   - `unit-tests/Makefile.am` ships the whole directory (`EXTRA_DIST = ... cmr_json_responses
     data`), so no Makefile change is needed.
   - First confirm nothing else references the file:
     `grep -rn related_urls_wo_Type modules/` should return no matches.

**Completeness check.** Every fixture should be referenced by a registered test.
- Before the change, this prints `related_url_wo_Type.json` (only reachable through an
  unregistered test) and `related_urls_wo_Type.json` (no test at all).
- After the change, it should print nothing:
  ```sh
  cd modules/dmrpp_module/ngap_container/unit-tests
  for f in cmr_json_responses/*.json; do
    b=$(basename "$f")
    fn=$(grep -n "$b" NgapApiTest.cc | head -1 | cut -d: -f1)
    [ -z "$fn" ] && { echo "UNUSED: $b"; continue; }
    name=$(awk -v n="$fn" 'NR<=n && /void test_/ {l=$0} END{print l}' NgapApiTest.cc \
           | sed -E 's/.*void (test_[A-Za-z0-9_]+).*/\1/')
    grep -q "CPPUNIT_TEST *($name)" NgapApiTest.cc || echo "UNREGISTERED: $b ($name)"
  done
  ```

## Verification

- `make check` in `modules/dmrpp_module/ngap_container/unit-tests` passes. `NgapApiTest`
  should report one more test than before (17, up from 16).
- Run `./NgapApiTest -d test_get_urls_from_granules_umm_json_v1_4_related_url_wo_Type` (or
  the test runner's equivalent single-test option) to confirm the new test runs and passes.
- `make distcheck` from the top of the build tree passes, which confirms the deleted fixture
  isn't needed by the dist.
- The completeness check above prints nothing.

## Risks

- Low. One line is added and one unreferenced data file is deleted.
- If the registered test fails, the code no longer throws on a missing `Type`, which would
  be news. Report it; don't adjust the test to pass.

## Open questions

- Should an entry with no `Type` make the whole lookup fail (current behavior), or should
  that entry just be skipped? FR-007 doesn't say. This doesn't block the task, because the
  test records current behavior. If you decide on "skip", that's a requirement change
  (`/new-requirement`) plus a bug.
