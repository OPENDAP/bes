# Task Plan: Make dmrpp_module tests and tools relink when `libngap` or `libmodules_common` changes

**Task:** TASK-004
**Status:** In Progress
**Created:** 2026-11-21

## Summary

Five link lines in `modules/dmrpp_module` bring in two in-tree libtool convenience libraries
(`libngap.la` and `libmodules_common.la`) with `-L<dir> -l<lib>` flags. Automake doesn't
treat `-l` flags as dependencies, so rebuilding either library doesn't relink the tests and
tools that use it. This task replaces those flags with the libraries' `.la` paths, which
automake does track.

## Motivation

This came up during BUG-005 (2026-11-20). After the fix to `NgapApi.cc` was in:
- `libngap.a` was rebuilt, but `make check` reran the old `NgapApiTest` binary.
- The tests therefore failed against code that had already been fixed.
- Deleting the binary was the only way to get a real result.

A stale binary can also do the opposite: pass when the new code is broken. CLAUDE.md treats
`make check` as the source of truth, so its results have to reflect the current code.

The generated `ngap_container/unit-tests/Makefile.in` shows the gap:
- `NgapApiTest_DEPENDENCIES` includes `$(top_builddir)/dap/.libs/libdap_module.a` and
  `$(top_builddir)/aws/libbes_aws.la`, which are linked by path.
- It does not include `libngap` or `libmodules_common`, which are linked with `-L/-l`.

## Related requirements / constraints

None in `docs/requirements/` or `docs/constraints/`. Related project rules:
- `.claude/rules/build-autotools.md`: "Link through Libtool `.la` files and let Libtool pick
  the platform's suffix and rpath."
- `CLAUDE.md`: "A test that `make check` doesn't run doesn't exist." A test that runs a stale
  binary is close to the same thing.

## Scope

**In scope.** Exactly these five links, recorded here as of commit `HEAD` of
`jhrg/dmrpp-lib-relink-deps` when the branch was created:

| # | File | Variable | Current | Replacement |
|---|---|---|---|---|
| 1 | `modules/dmrpp_module/ngap_container/unit-tests/Makefile.am` | `LIBADD` | `-L$(top_builddir)/modules/common -lmodules_common` | `$(top_builddir)/modules/common/libmodules_common.la` |
| 2 | same | `LIBADD` | `-L$(builddir)/../ -lngap` | `$(top_builddir)/modules/dmrpp_module/ngap_container/libngap.la` |
| 3 | `modules/dmrpp_module/unit-tests/Makefile.am` | `LIBADD` | `-L$(top_builddir)/modules/common -lmodules_common` | `$(top_builddir)/modules/common/libmodules_common.la` |
| 4 | `modules/dmrpp_module/Makefile.am` | `build_dmrpp_LDADD` | `-L$(builddir)/ngap_container -lngap` | `$(builddir)/ngap_container/libngap.la` |
| 5 | `modules/dmrpp_module/build_dmrpp_h4/Makefile.am` | `build_dmrpp_h4_LDADD` | `-L$(builddir)/../ngap_container -lngap` | `$(top_builddir)/modules/dmrpp_module/ngap_container/libngap.la` |

Each new path keeps the library at the same position in the link line, so link order
doesn't change.

**Out of scope:**
- `libdmrpp_module_la_LIBADD` in `modules/dmrpp_module/Makefile.am`. It links `-lngap`,
  `-ltest-types`, and `-Ldmrpp_transmitter -ldmrpp_return_as` into the installed module.
  Putting a convenience `.la` into an installed libtool library carries a different risk
  (libtool embeds the whole archive). That deserves its own task if it's wanted.
- `modules/fileout_netcdf/data/build_test_data/Makefile.am` (`-L$(top_builddir)/dap -ldap_module`).
- The commented-out `AM_LDADD` in `modules/functions/stare/Makefile.am`, and anything under
  `retired/`.
- Other link-line cleanup in these files: the `$(top_builddir)/dap/.libs/libdap_module.a`
  references (which reach into `.libs/`), `build_dmrpp_LDFLAGS` carrying a library, and
  `-lz` without a configure check.
- `configure.ac`, and any directory outside `modules/dmrpp_module/`.

## Approach

1. **Record the baseline.** In each of the four build directories:
   - Delete the affected binaries:
     - `ngap_container/unit-tests/{MemoryCacheTest,NgapApiTest,NgapOwnedContainerTest,NgapServiceChainingTest}`
     - the `dmrpp_module/unit-tests` check programs
     - `build_dmrpp`
     - `build_dmrpp_h4`
   - Rebuild them with `make V=1`, and save each libtool link command to the scratchpad.
   - Also save each target's `_DEPENDENCIES` line from the generated `Makefile.in`.
2. **Make the five edits** in the table, one line each, keeping the surrounding variable's
   layout.
3. **Regenerate.** Run `autoreconf -fiv` from the top of the source tree, or let make's
   rebuild rules rerun `automake` and `config.status`. Never commit `Makefile.in`.
4. **Compare the link commands.** Rebuild with `make V=1` and diff each link command against
   the baseline. Libtool already resolved `-lngap` to `libngap.la` through `-L`, so the expanded
   link should differ only in how the archive is named, not in which objects get linked.
   Investigate any added or removed library before going on.

**Completeness check:**
- Before the change, this prints 5 lines (checked 2026-11-21). Four of them hold the five
  in-scope links, with rows 1 and 2 on the same line, `ngap_container/unit-tests/Makefile.am:9`.
  The fifth is the out-of-scope `libdmrpp_module_la_LIBADD` (`modules/dmrpp_module/Makefile.am:68`):

  ```sh
  grep -nE -- '-L[^ ]*(modules/common|ngap_container|builddir\)/\.\./?) +-l(ngap|modules_common)\b' \
      modules/dmrpp_module/Makefile.am modules/dmrpp_module/*/Makefile.am \
      modules/dmrpp_module/*/unit-tests/Makefile.am
  ```

- After the change, the only remaining hit is `libdmrpp_module_la_LIBADD` (out of scope).
- In each regenerated `Makefile.in`, these `_DEPENDENCIES` lines now name `libngap.la` or
  `libmodules_common.la`, as appropriate:
  - `MemoryCacheTest_`, `NgapApiTest_`, `NgapOwnedContainerTest_`, and
    `NgapServiceChainingTest_DEPENDENCIES`
  - the `dmrpp_module/unit-tests` programs' `_DEPENDENCIES`
  - `build_dmrpp_DEPENDENCIES` and `build_dmrpp_h4_DEPENDENCIES`

## Verification

1. **Relink test.** This is the behavior the task exists for.
   - `touch modules/dmrpp_module/ngap_container/NgapApi.cc`, then run `make` in
     `ngap_container`.
   - Then run `make check` in `ngap_container/unit-tests`. All four test programs relink, which
     shows in the make output or as a newer binary timestamp than `.libs/libngap.a`.
   - Repeat for `build_dmrpp` and `build_dmrpp_h4`.
   - Repeat with a touched `modules/common` source for the `dmrpp_module/unit-tests` programs
     and the ngap tests.
2. **Tests still pass.** `make check` in `modules/dmrpp_module` (all subdirectories).
3. **Distcheck.** `make distcheck` from the top of the build tree. It builds out of tree, where
   `$(builddir)` and `$(top_builddir)` differ from `$(srcdir)`, so path mistakes show up there.
   If distcheck already fails for an unrelated reason (e.g. BUG-004), record that and confirm
   the failure is unchanged.
4. **Both platforms.** CI covers Linux. Locally, check macOS (Apple Clang). The concern is
   libtool's handling of a convenience archive on each platform, so a green build on one isn't
   enough.

## Risks

- **Duplicate symbols.** `build_dmrpp` compiles `$(BES_SRCS)` directly and also links
  `libngap`. Both are already linked today, so the set of linked objects shouldn't change;
  step 4 of the approach checks this.
- **Out-of-tree paths.** `$(builddir)/ngap_container/libngap.la` in
  `modules/dmrpp_module/Makefile.am` relies on `ngap_container` being built before `.`.
  `SUBDIRS` already orders it that way (`ngap_container ... .`). Row 5 uses
  `$(top_builddir)` rather than `$(builddir)/..` so it doesn't depend on the directory's depth.
- **Conditional builds.** `dmrpp_module/unit-tests` builds its CppUnit programs only under
  `if CPPUNIT`. Check that the `LIBADD` change doesn't break the non-CppUnit build, where
  `LIBADD` may still be used by `pugi_xml_test` (`noinst_PROGRAMS`).

## Open questions

None blocking. If step 4 of the approach shows a real difference in what gets linked, stop
and record it here before going further.

## Work log

### 2026-11-21 10:36 MST — Plan created

**Prompt:**

> /plan-task **Found along the way.** `unit-tests/Makefile.am` links `-lngap` through `LIBADD` without a
>   `_DEPENDENCIES` entry, so a changed `libngap.a` doesn't relink the tests. I had to delete the
>   binary to pick up the change. Not fixed here; it's a candidate `TASK-###`.

- Confirmed the gap in the generated `Makefile.in`, and surveyed every in-tree `-L/-l` link in
  the repo.
- You chose the scope (dmrpp_module tests and tools, 5 links), the category (Build &
  Tooling), the approach (link the `.la` path), and the branch `jhrg/dmrpp-lib-relink-deps`.
- Logged TASK-004 in `docs/tasks/TASK-LOG.md` (Plan Ready) and drafted this plan.

### 2026-11-21 11:22 MST — Implemented; distcheck running

**Prompt:**

> implement TASK-004 fix and mark them In Progress

- **Status.** Set TASK-004 (`docs/tasks/TASK-LOG.md`) and this plan to `In Progress`. Work is on
  branch `jhrg/fix-dmrpp-test-linkage`, which you created in place of
  `jhrg/dmrpp-lib-relink-deps`.
- **Approach step 1 (baseline).** Deleted and relinked all 17 affected programs with `make V=1`,
  and saved the link commands:
  - the 4 ngap tests
  - the 10 dmrpp unit-test programs, plus `pugi_xml_test`
  - `build_dmrpp` and `build_dmrpp_h4`
- **Approach step 2.** Made the five edits in the Scope table. The completeness `grep` now
  finds only the out-of-scope `libdmrpp_module_la_LIBADD` (`modules/dmrpp_module/Makefile.am:68`).
- **Approach step 3.** make's rebuild rules reran automake for the four `Makefile.in` files.
  Expanding each target's `_DEPENDENCIES` now shows:
  - `libngap.la` for the 4 ngap tests, `build_dmrpp`, and `build_dmrpp_h4`
  - `libmodules_common.la` for the 4 ngap tests and the 10 dmrpp unit tests
  - `pugi_xml_test` never used `LIBADD`; it's unchanged
- **Approach step 4.** In every expanded `libtool: link:` command, the only difference is that
  the `-L<dir>` flag is gone and the archive path is relative instead of absolute. The same
  `.a` stays in the same position, and no library was added or removed.
- **Verification 1 (relink).**
  - After touching `ngap_container/NgapApi.cc` and rebuilding `libngap.a`, all 6 `libngap`
    dependents relinked.
  - After touching `common/read_test_baseline.cc` and rebuilding `libmodules_common.a`, all 14
    `libmodules_common` dependents relinked.
- **Verification 2.** `make check -j8` in `modules/dmrpp_module` exits 0.
- **Left.**
  - Verification 3: `make distcheck` is running. The `dmrpp` directory from BUG-004 is now in
    `cmdln/tests` `EXTRA_DIST`, so it shouldn't stop there.
  - Verification 4: the Linux build (CI); this run was macOS only.
  - Setting TASK-004 to `Done` (your call).

### 2026-11-21 11:34 MST — Verification 3: distcheck passed

`make -j8 distcheck` on macOS exited 0 and produced `bes-3.22.1.tar.gz`, with no `FAIL:` or
`ERROR:` lines in the log. Left: verification 4 (the Linux build in CI), and setting TASK-004
to `Done` (your call).
