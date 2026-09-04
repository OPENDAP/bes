# Plan: Normalize Automake AM_* Variable Use

## Work Log

### 2026-05-12 13:19:56 MDT

Prompt:

> Make a plan to update every Makefile.am so that it make consistent use of the AM_LDFLAGS, AM_CXXFLAGS, ... AM_*, variables. Us the current `configure.ac` file as a guide, but if the best path forward involves changes there, include those in the plan but do so as an explicit step separate from any work to alter the Makefile.am files.

Assumptions and environment details:

- Repository: `/Users/jhrg/src/opendap/hyrax/bes`.
- Primary build system is autotools/libtool.
- The current `configure.ac` is the source of truth for repo-wide Automake substitutions and conditionals.
- The most behavior-sensitive variable is `AM_LDFLAGS`, because `configure.ac` uses it for dependency library search paths and rpath entries when `--with-dependencies` is used.
- Existing untracked documents before this plan included `docs/ncml-module-deep-dive.md` and `docs/rpath-makefile-am-deep-dive.md`.

Audit commands used:

- `rg -n "AC_SUBST\\(\\[?AM_|AM_[A-Z0-9_]+|AX_|CXXFLAGS|CPPFLAGS|LDFLAGS|LDADD|LIBADD" configure.ac conf modules/*/configure.ac modules/*/conf/*.m4`
- `rg -n "^AM_[A-Z0-9_]+\\s*(\\+?=|=)|^[A-Za-z0-9_]+_(LDFLAGS|CXXFLAGS|CPPFLAGS|CFLAGS|LDADD|LIBADD)\\s*(\\+?=|=)|^LDADD\\s*(\\+?=|=)|^LIBADD\\s*(\\+?=|=)" -g 'Makefile.am'`

## Goal

Make every `Makefile.am` follow one consistent rule set for Automake-wide variables:

- Preserve repo-wide values provided by `configure.ac`.
- Use `AM_*` variables for defaults shared by targets in a directory.
- Use target-specific variables only for real target-specific differences.
- Make intentional exceptions explicit with comments.
- Keep the diff reviewable by separating policy, configure changes, active build changes, retired build changes, and validation.

## Proposed Rules

### General

- Do not set `AM_* =` to an empty value unless there is a documented reason to block inherited/configured values.
- Prefer `AM_* += ...` when adding local flags to a variable that may be set by `configure.ac`, included make fragments, or a parent pattern.
- When a target-specific variable is defined and the target should inherit directory defaults, include the corresponding `$(AM_*)` explicitly.
- Keep user variables such as `CPPFLAGS`, `CXXFLAGS`, `LDFLAGS`, and `LIBS` as user/configure inputs, not as local Makefile scratch variables.
- Keep local aggregation variables named by purpose, such as `LIBADD`, `TEST_LIBADD`, or `MODULE_LIBADD`, but use them consistently.

### `AM_LDFLAGS`

- Treat `AM_LDFLAGS` as configured global linker flags.
- In `Makefile.am`, never use `AM_LDFLAGS = ...` except for a fully documented opt-out.
- Replace empty `AM_LDFLAGS =` with no assignment.
- Replace `AM_LDFLAGS = $(FOO_LDFLAGS)` with `AM_LDFLAGS += $(FOO_LDFLAGS)` when configured rpath/search paths must be preserved.
- For libtool libraries and modules with `*_la_LDFLAGS`, start with `$(AM_LDFLAGS)` unless the target is deliberately isolated.
- For programs with `*_LDFLAGS`, start with `$(AM_LDFLAGS)` when they link dependency-prefix shared libraries.
- Keep one-argument rpath form: `-Wl,-rpath,<path>`.

### `AM_CPPFLAGS`

- Use `AM_CPPFLAGS` for directory-wide include paths and preprocessor definitions.
- Use target-specific `*_CPPFLAGS = $(AM_CPPFLAGS) ...` only when the target needs extra includes or definitions.
- Avoid duplicating the same include paths in both `AM_CPPFLAGS` and each target-specific `*_CPPFLAGS`.
- Keep module identity defines such as `-DMODULE_NAME` and `-DMODULE_VERSION` in `AM_CPPFLAGS` for module directories.

### `AM_CXXFLAGS` and `AM_CFLAGS`

- Use `AM_CXXFLAGS` and `AM_CFLAGS` for directory-wide compiler warning/options only.
- Use `+=` for conditional additions such as developer flags, compiler-specific warnings, coverage, or ASAN-related additions.
- Do not use target-specific `*_CXXFLAGS` when `*_CPPFLAGS` is intended; flags like include paths and `-D...` belong in CPPFLAGS.
- Keep `CXXFLAGS_DEBUG` as an optional local helper only if it is already a local convention; do not expand that pattern.

### `AM_LDADD`

- Be careful: Automake does support `AM_LDADD` for programs, but many older comments in this tree contradict or question that.
- Prefer a local aggregate such as `TEST_LIBADD` or `COMMON_LDADD` for clarity when many test programs share libraries.
- If retaining `AM_LDADD`, use it consistently and avoid also using a generic `LDADD` in the same directory unless there is a reason.

## Step 1: Baseline Inventory

Produce a generated audit table before editing:

- All `Makefile.am` files.
- Assignments to `AM_CPPFLAGS`, `AM_CFLAGS`, `AM_CXXFLAGS`, `AM_LDFLAGS`, `AM_LDADD`, `AM_YFLAGS`, and `AM_LFLAGS`.
- All target-specific `*_CPPFLAGS`, `*_CFLAGS`, `*_CXXFLAGS`, `*_LDFLAGS`, `*_LDADD`, and `*_LIBADD`.
- Whether each file is active, conditional active, test-only, data-generation-only, or under `retired/`.
- Whether the target links against dependency-prefix libraries from variables such as `DAP_*`, `H5_*`, `HDF4_*`, `HDFEOS2_*`, `NC_*`, `GDAL_*`, `OPENSSL_*`, `AWS_*`, `ICU_*`, `STARE_*`, `GF_*`, or `CFITS_*`.

Output:

- Add a short audit appendix to this document or a separate `docs/am-vars-makefile-audit.md`.

## Step 2: Configure.ac Review And Separate Changes

This step is intentionally separate from `Makefile.am` edits.

Review `configure.ac` for whether it should explicitly initialize and substitute repo-wide `AM_*` variables.

Candidate configure changes:

- Initialize `AM_LDFLAGS` before appending dependency flags, then always `AC_SUBST([AM_LDFLAGS])`, even when `--with-dependencies` is not used. This avoids relying on Automake defaults in directories that do not assign it.
- Consider introducing `BES_DEPS_LDFLAGS` for dependency-prefix `-L` and rpath flags, then set `AM_LDFLAGS="$AM_LDFLAGS $BES_DEPS_LDFLAGS"`. This makes the dependency/rpath purpose testable and reduces the semantic load on `AM_LDFLAGS`.
- Keep GDAL-specific rpath in `GDAL_LDFLAGS`, but document why it is separate from the dependency-prefix rpath in `AM_LDFLAGS`.
- Do not move module-specific include or warning policy into `configure.ac` unless there is a repeated, repo-wide need.
- Confirm whether `AM_CXXFLAGS` or `AM_CPPFLAGS` should ever be configured globally. My current read is no, except coverage/ASAN-like logic through included make fragments or explicit conditionals.

Validation for this step:

- `autoreconf --force --install --verbose`
- `./configure --prefix="$prefix" --with-dependencies="$prefix/deps" --enable-developer`
- Inspect generated Makefiles for `AM_LDFLAGS = @AM_LDFLAGS@ ...`.

## Step 3: Mechanical Makefile.am Policy Patch

Apply the lowest-risk mechanical changes across all active `Makefile.am` files:

- Remove empty `AM_LDFLAGS =` assignments.
- Change `AM_LDFLAGS = $(FOO_LDFLAGS)` to `AM_LDFLAGS += $(FOO_LDFLAGS)` where `FOO_LDFLAGS` is additive.
- Add `$(AM_LDFLAGS)` to target-specific `*_LDFLAGS` for active libtool modules/libraries and programs that link dependency-prefix shared libraries.
- Leave comments where a target-specific `*_LDFLAGS` intentionally does not inherit `$(AM_LDFLAGS)`.

Do not combine this with naming cleanup or warning flag cleanup.

Focused validation:

- Regenerate affected `Makefile.in` files.
- Run `./config.status` for affected Makefiles.
- Inspect generated link lines for configured `AM_LDFLAGS`.
- Run focused builds for high-risk modules first: DMR++, HDF5, HDF4, netCDF, GDAL, HTTP/AWS if enabled.

## Step 4: Active Module Consistency Patch

Normalize active module Makefiles by family:

- Core libraries and programs: `dispatch`, `xmlcommand`, `ppt`, `http`, `cmdln`, `server`, `standalone`, `dap`, `dapreader`.
- Built-in modules without heavy external dependencies: `asciival`, `csv_handler`, `fileout_json`, `fileout_covjson`, `gateway_module`, `httpd_catalog_module`, `usage`, `xml_data_handler`, `debug_functions`, `ugrid_functions`.
- Heavy dependency modules: `dmrpp_module`, `hdf4_handler`, `hdf5_handler`, `netcdf_handler`, `fileout_netcdf`, `gdal_module`, `fits_handler`, `ncml_module`, `functions`, `s3_reader`, `cmr_module`.

Within each family:

- Put directory-wide includes/defines in `AM_CPPFLAGS`.
- Put directory-wide C++ warnings in `AM_CXXFLAGS`.
- Preserve coverage additions from `coverage.mk`.
- Keep target-specific `*_CPPFLAGS` and `*_LDFLAGS` only where they add target-specific content.
- Replace generic `LIBADD` with a more descriptive local name only when it improves clarity without broad churn. This is optional, not required for the first consistency pass.

Validation after each family:

- `make -C <family-dir>` or focused subdir build.
- Relevant `make -C <family-dir> check` when tests exist and prerequisites are present.

## Step 5: Test Directory Cleanup

Normalize test directories separately because they commonly use static archives, local object files, and `CPPUNIT`.

Rules:

- Use one shared aggregate variable per test directory, preferably `TEST_LDADD`.
- If keeping `AM_LDADD`, make sure it is used consistently and not mixed with unrelated `LDADD`.
- Keep `CPPUNIT_CFLAGS` in `AM_CPPFLAGS +=`.
- Keep `CPPUNIT_LIBS` in the shared test link aggregate.
- Add `$(AM_LDFLAGS)` only through Automake defaults unless the test defines target-specific `*_LDFLAGS`.

Validation:

- Start with DMR++ and NGAP unit tests once libdap is available.
- Then run representative unit-test directories with external dependencies enabled.
- If parallel `make check` fails due to known serial-test issues, retry serially and record that.

## Step 6: Data-Generation And Helper Program Review

Some `Makefile.am` files build helper programs for data generation or tests.

Plan:

- Identify helper programs that use `*_LDFLAGS = -static`, `BES_DAP_LIB_LDFLAGS`, or direct `.libs/*.a` paths.
- Do not blindly add `$(AM_LDFLAGS)` to static-only links without confirming it is harmless on Darwin and Linux.
- For dynamic helper programs that link dependency-prefix libraries, include `$(AM_LDFLAGS)`.
- Document any intentionally static or mixed static/dynamic link as an exception.

High-risk examples to inspect:

- `modules/fileout_netcdf/data/build_test_data/Makefile.am`
- `modules/fileout_netcdf/unit-tests/Makefile.am`
- DMR++ build helpers
- HDF4/HDF5 helper/data subdirectories

## Step 7: Retired Tree Pass

Decide explicitly whether `retired/` is in scope for behavior changes.

Recommended approach:

- First pass: apply only harmless consistency changes that preserve configured `AM_LDFLAGS`.
- Avoid deeper cleanup unless these trees are still built by `make distcheck` or source-distribution checks.
- Mark any retired exceptions in the audit document.

## Step 8: Regeneration Discipline

After each patch batch:

- Regenerate only the affected `Makefile.in` files when feasible.
- If configure macro changes were made, run full `autoreconf --force --install --verbose`.
- Do not edit generated files by hand.
- Keep generated-file diffs paired with their source `Makefile.am` or `configure.ac` changes.

## Step 9: Validation Matrix

Minimum validation:

- `autoreconf --force --install --verbose`
- `./configure --prefix="$prefix" --with-dependencies="$prefix/deps" --enable-developer`
- `make -j`
- `make check`

Broader validation for build-system changes:

- Source-distribution style build: `./configure && make -j && make check`
- Dependency-aware `make distcheck`
- `--without-cmr --without-ngap --without-s3`
- Optional dependency matrix when available: with/without GDAL, HDF4, HDF5, netCDF, NCML, AWS.
- Darwin and Linux validation, because rpath syntax and libtool behavior differ.

If `make -j` or `make check -j` fails in known parallel-sensitive tests, retry serially and record both outcomes.

## Step 10: Review Checklist

Before submitting:

- No active `Makefile.am` has an unexplained empty `AM_LDFLAGS =`.
- No target-specific `*_LDFLAGS` accidentally bypasses required `$(AM_LDFLAGS)`.
- No rpath uses split form such as `-Wl,rpath -Wl,<path>`.
- `configure.ac` changes, if any, are separate from Makefile normalization commits.
- Runtime/install impact is called out for installed modules and server binaries.
- Full validation gaps are explicit, especially missing libdap, hyrax-dependencies, AWS SDK, or optional module prerequisites.

## Recommended Patch Sequence

1. Audit-only commit: add generated audit document.
2. Optional `configure.ac` commit: clarify/initialize `AM_LDFLAGS` and dependency rpath variables.
3. Active core Makefile commit.
4. Active module Makefile commit.
5. Heavy dependency module commit.
6. Test directory commit.
7. Retired tree commit, if kept in scope.
8. Final validation/fixup commit only if validation reveals link-line regressions.

This sequencing keeps the dangerous part, linker behavior, visible and reviewable instead of burying it in a repo-wide style sweep.
