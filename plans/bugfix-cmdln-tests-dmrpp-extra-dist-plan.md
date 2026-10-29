# Bugfix Plan: `cmdln/tests/dmrpp/` is missing from the distribution

**Bug:** BUG-004
**Status:** Draft
**Created:** 2026-10-29

## Summary

`cmdln/tests/Makefile.am` doesn't ship the `dmrpp/` test-data directory, so `make distcheck`
fails `cmdln/tests` test 61 and stops before reaching later directories. The fix adds `dmrpp`
to `EXTRA_DIST`.

## Root cause

See the BUG-004 entry in `docs/bugs/BUG-LOG.md`.
- `EXTRA_DIST` (`cmdln/tests/Makefile.am:8-9`) lists `ff hdf4 hdf5 nc show ssfunc
  mds_initial_state`, but not `dmrpp`.
- 763ed1c5f (HYRAX-1732) added the `dmrpp/` directory and its test, but not the
  `EXTRA_DIST` entry.

## Related requirements

- **None.** This violates the CLAUDE.md rule that `make distcheck` must pass, and the
  `.claude/rules/build-autotools.md` rule that every file the tests need is in the
  distribution.

## Fix approach

1. In `cmdln/tests/Makefile.am`, add `dmrpp` to the `EXTRA_DIST` directory list, on line 9
   next to `ssfunc`. Listing a directory ships its whole tree, as the other entries do.
2. Regenerate with `autoreconf -fiv` (or let `make` rerun automake). Don't commit the
   generated `Makefile.in`.

**Why this scope.** This is the only directory `testsuite.at` uses that's missing from the
list; the completeness check below confirms it. Making `cmdln/tests` hermetic is a separate
problem (see Out of scope).

**Completeness check.** Every data directory `testsuite.at` uses must be listed in
`EXTRA_DIST`. Run this from the source tree with POSIX `sh`, not zsh: zsh doesn't
word-split `$used`, so it would silently print nothing.

```sh
cd cmdln/tests
used=$(grep -oE '^AT_BESCMD_[A-Z_]+\(\[[a-z0-9_]+/' testsuite.at | sed -E 's/.*\(\[//; s#/##' | sort -u)
for d in $used; do
  grep -qE "(^|[[:space:]])$d([[:space:]]|$)" Makefile.am || echo "MISSING from EXTRA_DIST: $d"
done
```

Before the fix, this prints `MISSING from EXTRA_DIST: dmrpp`. After the fix, it prints
nothing.

## Regression risk

- **Minimal.** Only the distributed file set grows, by three small text files
  (`dmrpp-return-as-test.bescmd` and its `.baseline` / `.baseline_alt`). Nothing in the
  in-tree build changes.

## Verification

1. `make dist`, then `tar tzf bes-*.tar.gz | grep cmdln/tests/dmrpp/` lists all three files.
2. The completeness check prints nothing.
3. Run `make distcheck`.
   - `cmdln/tests` 61 should pass.
   - 48 and 51 pass only once BUG-003 is fixed in the installed server and
     `/tmp/hyrax_func` is cleared. Until then they still fail, for that unrelated reason.
   - With all three passing, distcheck goes on to the later directories. Any new failure
     there is a separate bug.

## Out of scope

- **BUG-003** (the function-response cache keeps a partial entry), which is what fails tests
  48 and 51.
- **Making `cmdln/tests` independent of the installed, running BES and its `/tmp` cache.**
  This needs its own task.
- **Any other directory's `EXTRA_DIST`.** distcheck hasn't got past `cmdln/tests` yet.
