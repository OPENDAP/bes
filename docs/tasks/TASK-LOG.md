<!-- CONTRACT: scoped engineering/maintenance work that isn't new capability (FR/NFR/
     UC) and isn't corrective (BUG) — dependency bumps, removing dead code or stale
     compile-time directives, retrofitting tests onto code that predates them,
     build/tooling changes. The common thread: the "right" outcome is usually
     mechanical and completeness matters (did every instance get handled?), not
     open-ended design. Delete the TASK-EXAMPLE section once you've seen the shape. -->

# Task Log

Convention: `TASK-###`, sequential, never renumbered. Status: `Open` → `Plan Ready` →
`In Progress` → `Done` (or `Won't Do` / `Superseded by TASK-0XX`). **Only the user sets
`Done`** — it's a claim about verified work, not something to self-report from having
written a plan. Category is one of: Dependency
Update, Code Cleanup, Test Coverage, Build/Tooling, Documentation (add categories as
needed).

---

## TASK-EXAMPLE — Add unit tests for the legacy `parser/` module

*(delete this section once you've seen the shape)*

**Category:** Test Coverage
**Status:** Plan Ready
**Created:** YYYY-MM-DD
**Related requirements/constraints:** None found — `parser/` predates FR/NFR/UC
tracking in this repo
**Plan:** `plans/task-parser-test-coverage-plan.md`

**Scope:** Every public function in `parser/`, tested against the fixtures already
in `parser/testdata/`. Does not include refactoring the module itself, even where
the tests reveal awkward internal structure — that's a separate task if pursued.

**Motivation:** `parser/` has zero test coverage and has caused two regressions in
the last quarter that tests would have caught before merge.

---

## TASK-001 — Wire the unused NgapApi CMR fixtures into NgapApiTest

**Category:** Test Coverage
**Status:** Plan Ready
**Created:** 2026-10-29
**Related requirements/constraints:** FR-007 (an entry with no `Type` can't be selected; the
test pins down what happens instead)
**Plan:** `plans/task-ngapapi-unused-fixtures-plan.md`

**Scope:** Two fixtures in `modules/dmrpp_module/ngap_container/unit-tests/cmr_json_responses/`
that no registered test uses:
- `related_url_wo_Type.json`: register the existing, unregistered test
  `test_get_urls_from_granules_umm_json_v1_4_related_url_wo_Type` in `NgapApiTest`'s suite.
- `related_urls_wo_Type.json`: a misnamed duplicate of `no_related_urls.json`. Delete it.

The scope includes only tests that pass on the current code.

It excludes:
- The trim test that copies logic instead of calling `NgapApi` (covered by BUG-001's plan).
- Tests that need code fixes first: unchecked rapidjson access, one-character values,
  "expected at least 0" error text. These are to be logged as separate BUGs.
- Current-format double encoding (needs a requirement decision first).
- Testing `convert_ngap_resty_path_to_data_access_urls`, which needs a CMR stand-in.
- The cleanup items (TASK-002).

**Motivation:** Found by the 2026-10-29 deep dive
(`docs/deep-dives/ngap-api-cmr-lookup.md`). A test was written but never registered, and one
fixture duplicates another under a misleading name.

---

## TASK-002 — NgapApi code cleanup from the 2026-10-29 deep dive

**Category:** Code Cleanup
**Status:** Plan Ready
**Created:** 2026-10-29
**Related requirements/constraints:** FR-001–FR-004 (the escape refactor must not change the
CMR queries they define)
**Plan:** `plans/task-ngapapi-cleanup-plan.md`

**Scope:** `modules/dmrpp_module/ngap_container/NgapApi.cc` and `NgapApi.h`, plus an
include-only change to `NgapOwnedContainer.cc` and `unit-tests/NgapApiTest.cc`. Tests go in
`unit-tests/NgapApiTest.cc`.
- Replace the five `curl_easy_escape` / `curl_easy_unescape` call sites in
  `build_cmr_query_url*` with one file-local RAII helper that returns `std::string` and
  throws `BESInternalError` on null. Hold the `CURL*` handle in a `std::unique_ptr` with
  `curl_easy_cleanup`, and check `curl_easy_init` for null.
- Remove the function-local `static` cache in `get_cmr_search_endpoint_url()`, so the BES
  keys are read on every call.
- Remove the unused `<vector>` and `<map>` includes and the unused `http::url` forward
  declaration from `NgapApi.h`. Add `#include <vector>` to `NgapOwnedContainer.cc` and
  `NgapApiTest.cc`, which use `vector` without including it.

Out of scope:
- The `BESNotFoundError` vs. `BESInternalError` choice for a non-array `RelatedUrls`, which is
  a design decision.
- Converting `NgapApi` to free functions in a namespace (the header note, jhrg 10/8/25).
- Everything in BUG-001 and TASK-001.

**Motivation:** Found by the 2026-10-29 deep dive
(`docs/deep-dives/ngap-api-cmr-lookup.md`).

---

<!-- Add new tasks via /plan-task, or by hand — keep the section format: metadata
     lines, then Scope / Motivation. Unlike a bug, a task doesn't need a symptom or
     reproduction — it needs a clear boundary of what's included and what isn't,
     since "remove all the X" quietly becoming "remove most of the X" is the
     characteristic failure mode here. -->
