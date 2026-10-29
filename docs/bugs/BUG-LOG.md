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

## BUG-002 — `get_urls_from_granules_umm_json_v1_4` crashes or misreports on malformed CMR responses instead of throwing

**Severity:** High
**Status:** Fix Planned
**Reported:** 2026-10-29
**Related requirements:** None directly. No requirement says how a malformed CMR response is
handled, so this violates the undocumented assumption that malformed upstream input throws a
`BESError` and never crashes. The nearest is FR-006: release builds report a non-JSON response,
or one with no `"hits"`, as "granule not found", which FR-006 reserves for zero hits.
Tracked in Jira as HYRAX-2316.
**Fix plan:** `plans/bugfix-ngap-malformed-cmr-response-plan.md`

**Reproduction steps:**
1. In `NgapApiTest`, call `NgapApi::get_urls_from_granules_umm_json_v1_4("placeholder", json)`
   once for each `json` below:
   - a. `{"hits":1,"items":[{}]}`: `items[0]` has no `umm`
   - b. `{"hits":1,"items":[{"meta":{"x":1}}]}`: `items[0]` has other members but no `umm`
   - c. `{"hits":1,"items":[{"umm":3}]}`: `umm` is not an object
   - d. `<html>503</html>`: not JSON
   - e. `{"items":[...]}`: no `hits`
   - f. `{"hits":"1","items":[...]}`: `hits` is not an int
   - g. `{"hits":1}`: no `items`
   - h. `{"hits":1,"items":[]}`: `items` is empty
   - i. `{"hits":1,"items":[3]}`: `items[0]` is not an object
   - j. A `RelatedUrls` entry whose `Type` is `7`, or whose `URL` is `null`: not a string
   - k. A `RelatedUrls` entry that is not an object, for example `"RelatedUrls":[3]`
2. Build once as a developer build (`--enable-developer`, asserts on) and once as a release
   build (`NDEBUG`), ideally with `--enable-asan`.

**Expected behavior:** Each case throws `BESInternalError` with a message that names what was
wrong with the CMR response. Nothing crashes, and nothing is reported as not-found unless
`hits` is present and `< 1` (FR-006).

**Actual behavior:** Reproduced 2026-10-29 with a scratch copy of the function's rapidjson
access sequence, built against the repo's `rapidjson/`.
- **Developer build:** a. segfaults. Every other case, including g., aborts on a rapidjson
  `assert`; for g., `operator[]` asserts on a missing member.
- **Release build** (`NDEBUG`, which is what production uses; see `configure.ac:167-172`):
  - a., b., c., h., i., j., k.: undefined behavior. ASan/UBSan report a null-pointer member
    access, a wild pointer offset, a null reference binding, or SEGV.
  - d., e.: throw `BESNotFoundError` "does not identify a granule in CMR", a false not-found.
  - f.: continues with a garbage `hits` value.
  - g.: throws `BESInternalError` correctly.
- The server runs one forked `beslistener` per OLFS connection (`BESServerHandler.cc:108-118`),
  so a crash ends that connection and every request on it.

**Root cause:** Line numbers are as of commit a37efa713. `get_urls_from_granules_umm_json_v1_4`
(`NgapApi.cc:410-470`) and its helpers `get_data_http_url`, `get_data_s3_url`, and
`get_s3credentials_url` (`NgapApi.cc:294-385`) use rapidjson accessors whose preconditions
are enforced only by `RAPIDJSON_ASSERT`. `RAPIDJSON_ASSERT` defaults to `assert`
(`rapidjson/rapidjson.h:404-407`), so production builds don't check them at all. The code
checks some preconditions itself (`items.IsArray()`, the `RelatedUrls` lookup and type, the
`URL` / `Type` lookups) but not these:
1. **No parse check** (line 414). `Parse()` errors aren't tested with `HasParseError()`, and
   the document isn't checked with `IsObject()`.
2. **`hits`** (lines 416–417). `operator[]("hits")` on a missing member asserts. In release
   builds it returns a static null value, and `GetInt()` on that returns 0, so this case and
   any parse failure (case d.) take the `hits < 1` not-found branch. `GetInt()` on a non-int
   asserts, or returns an unrelated union member.
3. **`items`** (line 423). The same `operator[]` problem, though the following `IsArray()`
   check happens to save release builds.
4. **`items[0]`** (line 430). No `Empty()` check. `operator[](0)` on an empty array asserts,
   or in release returns a reference into a null element buffer.
5. **`umm`** (lines 431–433; the HYRAX-2316 note at 432). `FindMember("umm")` is never
   compared with `MemberEnd()`, and `items[0].IsObject()` isn't checked first. For an empty
   object, rapidjson stores no member array, so `MemberEnd()` wraps a null pointer and
   `mitr->value` is a null dereference (case a.). Otherwise it is one past the member array
   (case b.). `umm.IsObject()` isn't checked either (case c.), so the next `FindMember` runs
   on a non-object.
6. **`RelatedUrls` entries** (lines 294–385). Each helper calls `obj.FindMember` without
   `obj.IsObject()` (case k.) and calls `GetString()` on `Type` / `URL` without `IsString()`
   (case j.).

The checks that do exist sit on the paths the original fixtures exercised. The unchecked ones
were never reached by a test, because every fixture is a well-formed CMR response.

Checked and not affected: the only production caller is
`convert_ngap_resty_path_to_data_access_urls` (`NgapApi.cc:538`), via
`NgapOwnedContainer::build_data_urls_to_daac_bucket` (`NgapOwnedContainer.cc:257`). Neither
inspects the JSON, so the fix belongs entirely in the function and its helpers.

---

## BUG-003 — Function-response cache keeps a partial entry when the server function throws, so repeats of the request get "DDX response document parse failed"

**Severity:** High
**Status:** Fix Planned
**Reported:** 2026-10-29
**Related requirements:** None. The DAP function-response cache has no FR/NFR, so this
violates the undocumented assumption that a failed request leaves nothing in the cache and
that repeating a request gives the same result. Found through the `make distcheck` failure
of `cmdln/tests` 48 and 51.
**Fix plan:** `plans/bugfix-function-response-cache-partial-entry-plan.md`

**Reproduction steps:**
1. Run a BES with `DAP.FunctionResponseCache.path` set. The installed dev server
   (`/Users/jhrg/src/opendap/hyrax/build`) uses `/tmp/hyrax_func`. Start with no cache entry
   for the request.
2. `bescmdln -i cmdln/tests/ssfunc/geogrid_func_badargs.bescmd`. Its CE is `geogrid(SST)`
   on `/data/nc/coads_climatology.nc`.
3. Run step 2 again.
4. Look in the cache directory for a `func_*` file whose only content is the resource-ID
   line.

The same applies to `geogrid_func_swapcoords.bescmd` (`geogrid(SST,-82,61,-19,38)`), and to
any server-function CE whose evaluation throws.

**Expected behavior:** Steps 2 and 3 both return the function's own error, "Wrong number of
arguments to geogrid() (expected at least 5 args)", which is what the
`geogrid_func_badargs.bescmd.baseline` file holds. No cache entry is left behind.

**Actual behavior:**
- **Step 2 (first request):** returns the expected error. Evidence: an in-tree `cmdln/tests`
  run at 14:21 on 2026-10-29 recorded zero failures, and both cache files are timestamped
  14:21. That log has since been overwritten by a rerun.
- **Step 3 and every later run:** returns `Type 1` "The DDX response document parse failed:
  Error parsing DDX response: Could not read from input stream." This was seen in the
  distcheck run and in an in-tree rerun of tests 48 and 51.
- **Step 4:** `/tmp/hyrax_func/func_10596934181941624983_0` (90 bytes) holds only
  `…/coads_climatology.nc#geogrid(SST)`.
  `func_11067335069096671403_0` (104 bytes) holds only `…#geogrid(SST,-82,61,-19,38)`.
- The wrong error persists until the cache entry is purged (size-based purge or manual
  removal).

**Root cause:** Line numbers are as of commit a37efa713.
`BESDapFunctionResponseCache::write_dataset_to_cache` (`dap/BESDapFunctionResponseCache.cc:549-617`):
1. `create_and_lock`s the cache file.
2. Writes the resource-ID line (line 570).
3. Evaluates the function (lines 573–575, ending in `eval_function_clauses` at 575).

When step 3 throws, the `catch (...)` block (lines 608–613) closes the stream and calls
`unlock_and_close`. It never removes the file, so the partial entry stays on disk.
- **The partial entry becomes a cache hit.** On the next identical request,
  `load_from_cache` (lines 417–477) reads the first line, sees that it matches
  `resource_id`, and calls `read_cached_data` on the rest of the file. The rest is empty, so
  libdap's `DDXParserSAX2::intern_stream` throws `DDXParseFailed("…Could not read from input
  stream.")` (`libdap4/DDXParserSAX2.cc:971`).
- **The size bookkeeping isn't touched.** `update_cache_info` runs only on the success path
  (line 603), so the cache-size accounting never includes the partial file.

**How it surfaced:** `cmdln/tests` doesn't run a BES of its own. `bescmdln` talks to whatever
server is listening on localhost (here, the installed dev server), so the tests share that
server's cache state across runs.

---

## BUG-004 — `cmdln/tests/dmrpp/` is missing from the distribution, so `make distcheck` fails `cmdln/tests` 61

**Severity:** Medium
**Status:** Fix Planned
**Reported:** 2026-10-29
**Related requirements:** None. It violates the CLAUDE.md rule that `make distcheck` must
pass, and the `.claude/rules/build-autotools.md` rule that every file the tests need is in
the distribution.
**Fix plan:** `plans/bugfix-cmdln-tests-dmrpp-extra-dist-plan.md`

**Reproduction steps:**
1. From the top of the build tree, run `make distcheck`. Or run `make dist` and list the
   tarball: `tar tzf bes-*.tar.gz | grep cmdln/tests/dmrpp`.
2. Check the result of `cmdln/tests` test 61, `BESCMD dmrpp/dmrpp-return-as-test.bescmd`.

**Expected behavior:** The tarball contains `cmdln/tests/dmrpp/dmrpp-return-as-test.bescmd`
and its two baselines, and test 61 passes in the distcheck build, as it does in-tree.

**Actual behavior:**
- `bescmdln` reports that it can't open the input file
  `…/bes-3.22.0/_build/sub/../../cmdln/tests/dmrpp/dmrpp-return-as-test.bescmd`, and the
  test fails.
- `make distcheck` stops in `cmdln/tests`, so later directories (including `modules/`) are
  never checked.
- In-tree, test 61 passes (rerun 2026-10-29).

**Root cause:** `cmdln/tests/Makefile.am:8-9` lists the test-data directories in
`EXTRA_DIST`: `ff hdf4 hdf5 nc show ssfunc mds_initial_state`. `dmrpp` isn't listed.
- The directory and its three files arrived in 763ed1c5f (HYRAX-1732), together with the
  `AT_BESCMD_DMRPP_RESPONSE_TEST` line in `testsuite.at`.
- `Makefile.am` wasn't updated in that commit or later.

---

<!-- Add new bugs via /fix-bug, or by hand — keep the section format: metadata lines,
     then Reproduction / Expected / Actual / Root Cause. A bug with no reproduction
     steps yet is a symptom report, not a bug entry — get concrete steps before
     assigning an ID if at all possible. -->
