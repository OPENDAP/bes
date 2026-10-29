---
paths:
  - "**/unit-tests/**"
  - "**/tests/**"
  - "**/*.at"
  - "**/*.baseline"
  - "modules/common/handler_tests_macros.m4"
  - "modules/common/run_tests_cppunit.h"
---

# Testing rules — CppUnit + Autotest

- **Unit tests** use CppUnit and live in `<component>/unit-tests/`. Each test is one
  program named in `TESTS`, with `check_PROGRAMS = $(TESTS)`, linked against the
  component's Libtool library plus `$(CPPUNIT_LIBS)` (`$(CPPUNIT_CFLAGS)` in
  `AM_CPPFLAGS`). The whole block sits inside `if CPPUNIT ... else ... endif` so a
  build without CppUnit still succeeds and prints a notice from `check-local` instead.
  Tests that only make sense in developer builds are added under `if BES_DEVELOPER`.
- A CppUnit test is a `CppUnit::TestFixture` with `CPPUNIT_TEST_SUITE` / `CPPUNIT_TEST`
  / `CPPUNIT_TEST_SUITE_END`, and its `main` is the shared driver from
  `modules/common/run_tests_cppunit.h`:
  `int main(int argc, char *argv[]) { return bes_run_tests<FooTest>(argc, argv, "bes,foo") ? 0 : 1; }`.
  That gives every test the same `-d` / `-D` / `-b` (debug, verbose debug, BESDEBUG
  contexts) and `-h` options, and lets you run a single test method by name.
- Unit tests find their data through a configure-generated header (`test_config.h`
  from `test_config.h.in`, listed in `BUILT_SOURCES`) that defines `TEST_SRC_DIR`,
  `TEST_BUILD_DIR`, and similar absolute paths. Tests never hardcode a path or assume
  the working directory. Read inputs from the source directory; write outputs only to
  the build directory.
- **Integration and baseline tests** use Autotest and live in `<component>/tests/`.
  The suite is `testsuite.at`, compiled by `autom4te --language=autotest` into a
  `testsuite` script, with `package.m4` generated from `configure.ac` and an `atlocal`
  produced from `atlocal.in` through `AC_CONFIG_FILES`. `check-local` runs
  `$(SHELL) '$(TESTSUITE)' $(TESTSUITEFLAGS)`; `installcheck-local` runs it again with
  `AUTOTEST_PATH='$(bindir)'` against the installed binaries; `clean-local` runs
  `testsuite --clean`. Copy this wiring from an existing `tests/Makefile.am` rather than
  inventing a new one.
- Use the shared macros in `modules/common/handler_tests_macros.m4`
  (`AT_BESCMD_RESPONSE_TEST`, `AT_BESCMD_BINARYDATA_RESPONSE_TEST`,
  `AT_BESCMD_ERROR_RESPONSE_TEST`, `AT_BESCMD_BESCONF_RESPONSE_TEST`, ...) instead of
  writing raw `AT_SETUP` / `AT_CHECK` / `AT_CLEANUP` blocks. Each takes a `.bescmd`
  input, runs `besstandalone`, and compares against `<input>.baseline`. Mark known
  failures with the macro's `xfail` argument (which uses `AT_XFAIL_IF`) and a comment or
  ticket — don't delete or comment out the test.
- **Baselines** — where the output is a text artifact (DAP response, DMR/DDS/DAS,
  generated metadata, CLI output) — are checked-in `*.baseline` files next to their
  inputs. A mismatch fails the test and Autotest keeps the test's directory and diff in
  `testsuite.dir/` and `testsuite.log`. To update, run
  `./testsuite --baselines=yes [test numbers or -k keyword]`, which writes
  `<input>.baseline.tmp` instead of comparing; review it and move it over the old
  baseline. Update mode is off by default and never on in CI. Review `git diff` on
  baseline files like source code; a baseline change in a PR needs a reason.
- Output that legitimately varies between runs (dates, version strings, temp paths) is
  scrubbed before comparison with the existing macros (`REMOVE_VERSIONS`, the
  `*_SCRUB_DATES_*` variants) or a new one added to the shared `.m4`, not by loosening
  the comparison or regenerating the baseline each run.
- Running pieces of the suite: in a `tests/` directory, `./testsuite -l` lists tests,
  `./testsuite 12 15-20` or `./testsuite -k <keyword>` runs a subset, and
  `./testsuite -v -d -x <N>` shows a failing test's commands and keeps its scratch
  directory. `TESTSUITEFLAGS=-j<N>` runs an Autotest suite in parallel. In a
  `unit-tests/` directory, `make check TESTS=fooT` runs one program and
  `./fooT -d <method>` runs one method with debugging on.
- Commands inside `AT_CHECK` and any helper scripts are portable shell: `#!/bin/sh`,
  POSIX only. No bashisms, and no GNU-only flags (`sed -i`, `readlink -f`, `date -d`,
  `echo -e`, `grep -P`); use `printf`, and write to a temp file then `mv` instead of
  `sed -i`. Autotest already gives each test its own scratch directory under
  `testsuite.dir/`; a standalone helper script that needs scratch space uses
  `mktemp -d "${TMPDIR:-/tmp}/name.XXXXXX"`, removed by a `trap`.
- Paths and configuration reach tests through `atlocal`, `package.m4`
  (`AT_TOP_SRCDIR`), the generated `bes.conf` files (built from `bes.conf.in` with
  `@abs_top_srcdir@` / `@abs_top_builddir@` substituted), and `test_config.h` — never
  hardcoded. Inside a test, use `$abs_srcdir` / `$abs_builddir`, not relative paths that
  guess the working directory. Keep long logic in an `.m4` macro or a script file
  rather than a quote-heavy `AT_CHECK` line; nested m4 and shell quoting is fragile and
  hard to review.
- Tests are deterministic and independent: no reliance on test order, wall-clock time,
  network, or a shared scratch location, so `make check -j` and
  `./testsuite -j<N>` are safe. Fixed seeds only. Tests that genuinely need the network
  (e.g. remote-resource or AWS tests) are kept in a separate suite or behind a
  configure option or keyword so the default `make check` doesn't need them.
- The suite also runs clean under ASan + UBSan on Linux and macOS: configure with
  `--enable-asan` (add `--enable-developer` for usable stack traces) and run
  `make check`. LeakSanitizer isn't available on macOS, so leak checking is Linux-only;
  Valgrind with the checked-in suppression files (`vg-*-supp.txt`, see
  `README-VALGRIND.md`) is the alternative.
