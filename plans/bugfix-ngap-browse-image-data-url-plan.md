# Bugfix Plan: NGAP selects a browse image listed as `GET DATA` as the granule's data URL

**Bug:** BUG-005
**Status:** Fixed
**Created:** 2026-11-20

## Summary

Some CMR records (for example LP DAAC MOD13Q1) list browse images (`BROWSE.*.jpg`) as
`GET DATA` entries ahead of the real data file. The NGAP container selects the first image as
the data URL, appends `.dmrpp`, and fails to find a DMR++. This fix makes the data-URL filter
skip browse-image extensions as well as `.xml` and `.dmrpp`. It does this for both the https
and the S3 data URL.

## Root cause

See BUG-005 in `docs/bugs/BUG-LOG.md` for the full trace. In short:
- `get_data_http_url` (`NgapApi.cc:321-338`) and `get_data_s3_url` (`NgapApi.cc:341-357`)
  reject only URLs that end in `.xml` or `.dmrpp`.
- In the MOD13Q1 record, the first `GET DATA` entry for each scheme is `BROWSE...1.jpg`. These
  entries have no `Subtype` or `MimeType`, so the first-match loop at `NgapApi.cc:459-472`
  selects them.
- `NgapOwnedContainer.cc:527-532` appends `.dmrpp` to the selected URLs (FR-011), producing a
  URL with no DMR++ behind it.
- The `.cmr.xml` entries are already rejected.

The code does what FR-007/FR-008 say. The requirements never anticipated browse images
mislabeled as `GET DATA`.

## Related requirements

- **FR-007 / FR-008** (Approved). These define the https and S3 data-URL selection rules. They
  aren't strictly violated, but they're incomplete: following them selects a browse image.
  They'll be **superseded** by new FRs that add the browse-image exclusion (decision,
  2026-11-20). The FR-007/FR-008 rows get `Status: Superseded by FR-0XX`, and their wording
  isn't edited.
- **FR-011**. The DMR++ is found by appending `.dmrpp` to the selected URL. This is what turns
  the wrong selection into a failed request.
- **FR-010**. If every https `GET DATA` URL is excluded, the request must still fail with an
  error. That doesn't change.
- The real requirement is that the server serves well-formed and these common malformed CMR
  records alike. No UC/NFR captures that yet.

## Fix approach

**Step 0 — Requirements (FR-007, FR-008).** Run `/new-requirement` twice, giving FR-012
(https) and FR-013 (S3) unless newer numbers exist by then. Each has the same wording as
FR-007/FR-008, except that the URL "does not end in `.xml`, `.dmrpp`, or a browse-image
extension (`.jpg`, `.jpeg`, `.png`, `.gif`), compared case-insensitively". Then mark
FR-007/FR-008 `Superseded by FR-012` / `Superseded by FR-013`. Two points to confirm during that
interview, both TBD here:
- **The extension list.** The bug report names `.jpg`. Adding `.jpeg`, `.png`, and `.gif` is a
  proposal. `.tif`/`.tiff` are deliberately **not** included, because GeoTIFF is a real data
  format.
- **Case-insensitive matching.** This is new: today `.XML` would be accepted.

**Step 1 — Code (new FRs).** In `NgapApi.cc`:
- Add one helper, `is_excluded_data_url(const std::string &url)`, backed by a single list of
  excluded suffixes (`.xml`, `.dmrpp`, `.jpg`, `.jpeg`, `.png`, `.gif`). Compare
  case-insensitively against the end of the URL.
- Call it from both `get_data_http_url` and `get_data_s3_url`, replacing the paired
  `ends_with` checks. One list means the two filters can't drift apart.
- Update the doc comment on `get_urls_from_granules_umm_json_v1_4` (`NgapApi.cc:374-395`) and
  the FR citations in the inline comments.
- Use C++14 only. Lower-case a copy of the URL with `std::tolower` on `unsigned char`.

**Step 2 — Test fixtures (BUG-005).** Your two new files in `unit-tests/cmr_json_responses/`
are in the CMR *search* JSON format (camelCase `relatedUrls`/`url`/`type`, no `hits`/`items`).
`get_urls_from_granules_umm_json_v1_4` can't parse that format; it would throw "no integer
'hits' value". To fix that:
- Replace each file with its `granules.umm_json_v1_4` response, fetched once by the command in
  `cmr_json_responses/README`. Copies fetched on 2026-11-20 confirmed the same `RelatedUrls`
  order.
- Name them to match the existing fixtures:
  - `MOD13Q1.A2016257.h27v06.061.2021358013828.json`
  - `3B-HHR-L.MS.MRG.3IMERG.20260603-S190000-E192959.1140.V07C.HDF5.json`

  The second name drops the `GPM_3IMERGHHL.07:` prefix because a `:` in a file name is
  troublesome for make and tar.
- `EXTRA_DIST` already ships the whole directory, so `Makefile.am` doesn't change.
- Tests read only these files, never the network (testing rules).

**Step 3 — Tests (BUG-005, new FRs, FR-010).** In `NgapApiTest.cc`, add and register:
- `test_get_urls_from_granules_umm_json_v1_4_lpdaac_browse_images`: reads the MOD13Q1 fixture.
  It expects the `.hdf` https URL, the `.hdf` S3 URL, and
  `https://data.lpdaac.earthdatacloud.nasa.gov/s3credentials`. This is the regression test: it
  must fail before Step 1 and pass after.
- `test_get_urls_from_granules_umm_json_v1_4_gesdisc`: reads the GPM fixture (a well-formed
  record). It expects the `.HDF5` https and S3 URLs and
  `https://data.gesdisc.earthdata.nasa.gov/s3credentials`.
- `test_get_urls_from_granules_umm_json_v1_4_browse_image_suffixes`: uses a small synthetic
  fixture, `data_url_browse_image_suffixes.json`. It has `.jpg`, `.JPG`, `.jpeg`, `.png`,
  `.gif`, and `.XML` entries ahead of a `.nc`, for both schemes, and expects the `.nc` URLs.
  Add a `.tif` case that **is** accepted, to pin the GeoTIFF exclusion.
- `test_get_urls_from_granules_umm_json_v1_4_only_browse_image`: the only `GET DATA` https URL
  is a `.jpg`. Expect `BESInternalError` (FR-010).

**Why this scope.** I considered a broader heuristic: pick the `GET DATA` URL whose file name
matches the `GranuleUR` or `ProducerGranuleId`, which would also catch other kinds of junk
entries. I didn't choose it because:
- `GranuleUR` often isn't the file name. GES DISC prefixes it with `GPM_3IMERGHHL.07:`.
- `DataGranule.Identifiers` is optional.
- The browse file name in this very record (`...013829`) differs from the granule's
  (`...013828`) by only one character, which shows how loose a name-based match would have to
  be.

The suffix denylist matches the existing `.xml` rule, fixes the reported common case, and is
cheap to extend.

## Regression risk

- **Callers.** `get_urls_from_granules_umm_json_v1_4` is called from
  `convert_ngap_resty_path_to_data_access_urls` (`NgapApi.cc:548`), whose result goes to
  `NgapOwnedContainer`. Every NGAP granule request goes through this filter.
- **Data files with an image extension.** A granule whose real data file ends in `.jpg`,
  `.png`, or `.gif` would no longer be selectable. Low risk: the DMR++ handler can't serve those
  formats anyway.
- **Case-insensitive `.xml`/`.dmrpp`.** URLs ending in `.XML` or `.DMRPP` would now be
  rejected, where today they're accepted.
- **Existing coverage.** These tests guard the unchanged behavior and must all still pass:
  - `minimal`, `lpdaac` (ECOSTRESS), and `podaac` (CYGNSS)
  - `dmrpp_not_suffix`, `xml_not_suffix`, and `rejected_suffixes` (BUG-001)
  - the malformed-input tests (BUG-002)

## Verification

1. Before Step 1, add the Step 3 tests. Run
   `cd modules/dmrpp_module/ngap_container/unit-tests && make check TESTS=NgapApiTest`.
   `lpdaac_browse_images`, `browse_image_suffixes`, and `only_browse_image` should fail (the
   last one returns a `.jpg` instead of throwing). `gesdisc` should pass.
2. Apply Step 1 and rerun. Every test in `NgapApiTest` passes.
3. Rerun BUG-005 reproduction steps 1–3. The returned URLs are the `.hdf` URLs.
4. Run `make check` in `modules/dmrpp_module` (covers `NgapOwnedContainerTest` too).
5. Optionally, on an NGAP deployment, run reproduction step 4. The `.dmr` request succeeds.
6. Run with `--enable-asan` if convenient (testing rules).

## Out of scope

- **`docs/deep-dives/ngap-api-cmr-lookup.md`** is already stale: it still describes the
  pre-BUG-001 "contains" rule. Refresh it separately, or note it in that file.
- **The `find(".dmrpp")` tests at `NgapOwnedContainer.cc:293` and `:322`** (the path-based
  DMR++ lookups). BUG-001's notes cover these; they're unrelated to data-URL selection.
- **Using `GET RELATED VISUALIZATION` duplicates or `MimeType` to classify entries.** The
  offending record has no `MimeType` on its browse entries.
- **Query strings or fragments after the file extension** (e.g. `.jpg?x=1`). Not seen in
  practice. Add a case if one turns up.

## Work log

### 2026-11-20 16:15 MST — Plan created

**Prompt:**

> /fix-bug The new file @modules/dmrpp_module/ngap_container/unit-tests/cmr_json_responses/MOD13Q1.A2016257.h27v06.061.2021358013828 causes the existing NgaApi.cc code to treat a `GET_DATA` URL that ends in `.jpg` as a real data URL. The code then appends `.dmrpp` and fails to find a DMR++ at the resulting URL, because none exists. Instead, it should have skipped the URLs ending in `.jpg` (and `.cmr.xml`) and used the `.hdf` URL. This CMR metadata record is malformed, but the error is common and our code should be robust.
>
> Once fixed, the unit-tests for this issue can use @"modules/dmrpp_module/ngap_container/unit-tests/cmr_json_responses/GPM_3IMERGHHL.07:3B-HHR-L.MS.MRG.3IMERG.20260603-S190000-E192959.1140.V07C.HDF5"  to ensure that a well-formed CMR record still works. The existing tests here are also good for eliminating regression.

- Logged BUG-005 (High) in `docs/bugs/BUG-LOG.md`. You chose `docs/` + `plans/` over the
  template `ai-docs/` + `ai-plans/`, chose to supersede FR-007/FR-008, and chose to stay on
  branch `jhrg/HYRAX-2333-fix-cmr-get-data-fail`.
- Traced the root cause to the suffix filter in `get_data_http_url` / `get_data_s3_url` and
  confirmed it against live UMM-G v1.4 responses.
- Found that the two new fixtures are in the wrong CMR format for the unit tests.
- Left: the Step 0 requirement interview, then implementation.

### 2026-11-20 16:34 MST — Implemented Steps 0–3

**Prompt:**

> Implement @plans/bugfix-ngap-browse-image-data-url-plan.md

- **Step 0.** You confirmed the extension list (`.xml .dmrpp .jpg .jpeg .png .gif`) and
  case-insensitive matching. Added FR-012/FR-013 (Approved) to
  `docs/requirements/functional-requirements.md`, and marked FR-007/FR-008
  `Superseded by FR-012` / `Superseded by FR-013`.
- **Step 1.** Added `is_excluded_data_url()` to `NgapApi.cc` and used it in `get_data_http_url`
  and `get_data_s3_url`. Updated the doc comment and the FR citations.
- **Step 2.**
  - Replaced the two search-format files with UMM-G v1.4 responses fetched 2026-11-20:
    `MOD13Q1...2021358013828.json` and `3B-HHR-L...V07C.HDF5.json`. The originals are still in
    HEAD under `data/cmr_json_records/`.
  - Added `data_url_browse_image_suffixes.json` and `data_url_only_browse_image.json`.
  - Deviation: put the `.tif` case in its own fixture, `data_url_tif_accepted.json`, with its
    own test `tif_accepted`, so it doesn't share a fixture with the suffix test.
- **Step 3.** Added 5 tests to `NgapApiTest.cc`. Results:
  - Before the fix: 4 of 38 failed (`lpdaac_browse_images`, `browse_image_suffixes`,
    `tif_accepted`, `only_browse_image`); `gesdisc` passed.
  - After the fix: `OK (38 tests)`.
  - All 4 ngap unit-test programs pass, and `make check` in `modules/dmrpp_module` exits 0.
- **Found along the way.** `unit-tests/Makefile.am` links `-lngap` through `LIBADD` without a
  `_DEPENDENCIES` entry, so a changed `libngap.a` doesn't relink the tests. I had to delete the
  binary to pick up the change. Not fixed here; it's a candidate `TASK-###`.
- **Left.**
  - Your review and commit.
  - Optional: reproduction step 4 on an NGAP deployment.
  - Optional: an ASan run.
  - Setting BUG-005 to `Fixed` (your call).
