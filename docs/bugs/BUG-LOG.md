<!-- CONTRACT: deviations from intended behavior, tracked from report through root
     cause to fix. Not a duplicate of docs/requirements/ — a bug means something
     already documented (or implicitly assumed) isn't holding true in practice.
     Delete the BUG-EXAMPLE section once you've seen the shape. -->

# Bug Log

Convention: `BUG-###`, sequential, never renumbered. Status: `Open` → `Investigating`
→ `Fix Planned` → `Fixed` → (or `Won't Fix` / `Duplicate of BUG-0XX`). `Open` means
logged with reproduction steps but root cause not yet looked at (`/fix-bug` sets it
at step 4 and moves to `Investigating` at step 5; an entry logged by hand starts here
too). **Only the user sets a bug to `Fixed`** — that's a claim about verified
deployed behavior, not something to self-report from having written a fix plan.

---

## BUG-EXAMPLE — Password reset link accepted after expiry

*(delete this section once you've seen the shape)*

**Severity:** High
**Status:** Fix Planned
**Reported:** YYYY-MM-DD
**Related requirements:** FR-EXAMPLE, UC-EXAMPLE (violates the "time-limited" part of both)
**Fix plan:** `plans/bugfix-reset-link-expiry-plan.md`

**Reproduction steps:**
1. Request a password reset
2. Wait past the stated expiry window (currently 1 hour)
3. Follow the link anyway

**Expected behavior:** Link is rejected with an "expired, request a new one" message
(per FR-EXAMPLE).

**Actual behavior:** Link still succeeds and resets the password.

**Root cause:** Expiry is checked against token creation time using the token's own
embedded claim, but that claim is client-suppliable and never verified against issue
time recorded server-side. *(Fill this in during investigation — leave `TBD` until
actually found in the code, not guessed from symptoms.)*

---

## BUG-001 — NGAP data-URL filter rejects `.xml`/`.dmrpp` anywhere, and the `.dmrpp` trim on the S3 URL uses the wrong length

**Severity:** Medium
**Status:** Fix Planned
**Reported:** 2026-10-29
**Related requirements:** FR-007, FR-008 (the filter rejects URLs that only *contain* `.xml` / `.dmrpp`; the requirements reject only URLs that *end in* them; line 533 would corrupt the FR-008 S3 URL)
**Fix plan:** `plans/bugfix-ngap-dmrpp-url-filter-plan.md`

**Reproduction steps:**
1. In `NgapApiTest`, call `NgapApi::get_urls_from_granules_umm_json_v1_4("placeholder", json)`.
   Use a UMM-G response with `hits: 1` whose only `GET DATA` https entry (no `Subtype`) is
   `https://data.example/granule.dmrpp.nc` (or `https://data.example/v1.xml.d/granule.nc`).
2. Read the result.
3. Separately, for the trim at `NgapApi.cc:524-534`, run that block on its own with
   `data_access_url = "https://data.example/a.nc.dmrpp"` and
   `data_s3_url = "s3://some-bucket/long/prefix/a.nc.dmrpp"`. Through the public API this
   block is unreachable, because steps 1–2's filter rejects any URL containing `.dmrpp`
   first.

**Expected behavior:**
- Steps 1–2 return `https://data.example/granule.dmrpp.nc`. It doesn't end in `.xml` or
  `.dmrpp`, so FR-007 selects it. The same applies to `s3://` URLs under FR-008.
- Step 3 gives `s3://some-bucket/long/prefix/a.nc`.

**Actual behavior:**
- Steps 1–2 throw `BESInternalError` ("Failed to locate a data URL").
- Step 3 gives `s3://some-bucket/lo`. It truncates at the trimmed https URL's length (25) minus 6.
  If the trimmed https URL were longer than the S3 URL, it would throw `std::out_of_range`
  instead.

**Root cause:** There are three related defects. Line numbers are as of commit 24e22d1d4.
1. **The filter tests "contains", not "ends in".** `get_data_http_url` (`NgapApi.cc:311-312`)
   and `get_data_s3_url` (`NgapApi.cc:343-344`) test `candidate_url.rfind(".xml") == npos`
   and `rfind(".dmrpp") == npos`. That is true only when the substring appears nowhere in the
   URL. The comments and FR-007/FR-008 call for an ends-with test.
2. **Wrong operand in the S3 trim** (`NgapApi.cc:533`). `data_s3_url.erase(...)` computes its
   offset from `data_access_url.size()`, which line 528 may already have shortened. It should
   use `data_s3_url.size()`.
3. **The trim block is dead code under either filter rule.** Both filters reject any URL
   ending in `.dmrpp` before the trim at lines 524–534 runs. This is true of the current
   "contains" test and of the FR-007/FR-008 "ends in" test. So defect 2 can't be reached
   through the public API, and the block can never fire.
   `NgapApiTest::test_dmrpp_is_removed_from_data_url` copies the trim logic into the test
   instead of calling `NgapApi`, so it couldn't catch defect 2.

Consequence of fixing (1): `NgapOwnedContainer::dmrpp_read_from_daac_bucket`
(`NgapOwnedContainer.cc:527`) appends `.dmrpp` to the https URL only when
`find(".dmrpp") == npos`. Once the filter accepts URLs like `granule.dmrpp.nc`, that check
would skip the append, and the data file would be fetched as if it were the DMR++. The S3 URL
at line 532 always gets `.dmrpp` appended, so the two URLs are already handled inconsistently.

Checked and not affected: `extract_s3_data_urls_from_dmrpp` (`NgapOwnedContainer.cc:645`,
used at line 723) reads URLs injected into the DMR++. It does not feed the DMR++ URL
construction.

---

<!-- Add new bugs via /fix-bug, or by hand — keep the section format: metadata lines,
     then Reproduction / Expected / Actual / Root Cause. A bug with no reproduction
     steps yet is a symptom report, not a bug entry — get concrete steps before
     assigning an ID if at all possible. -->
