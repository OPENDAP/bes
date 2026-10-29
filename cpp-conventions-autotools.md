<!-- Paste the body below into CLAUDE.md in place of the {{CONVENTIONS}} placeholder,
     and drop or demote this H2 so CLAUDE.md keeps a single `## Conventions` heading. -->

## Conventions — C++ (Autoconf / Automake / Libtool, Autotest + CppUnit)

**Stack:** C++14 on Linux and macOS, built with Autoconf, Automake, and Libtool, tested
with CppUnit (unit tests) and Autotest (integration and baseline tests), all driven by
`make check`. C++14 is the baseline — verify each target compiler actually builds it,
and bump to C++17 only if every target compiler supports it; check before assuming.
Compilers/platforms: `GCC 9+ and Clang 12+ on Linux; Apple Clang on macOS (arm64 +
x86_64)`. Unit-test framework: `CppUnit` (1.12 or later).

### Build & layout (Autoconf / Automake)

- The language standard is chosen once, in `configure.ac`: `CXX_FLAGS_CHECK` probes for
  `--std=c++14` and substitutes it as `CXX_STD_FLAG`, which configure adds to
  `CXXFLAGS`. Don't hardcode `-std=` in any `Makefile.am`, and keep it `c++14`, not
  `gnu++14`, so code that only compiles with GNU extensions fails for everyone, not
  just on someone else's machine.
- `AC_PREREQ` and `AM_INIT_AUTOMAKE([<min version> ...])` state the oldest Autoconf and
  Automake that must work. Don't use macros or Automake features newer than the oldest
  versions CI bootstraps with. Project-local macros live in `conf/` (`AC_CONFIG_MACRO_DIR`);
  add a new `.m4` there instead of inlining long shell logic in `configure.ac`.
- Generated files are never committed: `configure`, `Makefile.in`, `Makefile`,
  `config.h`, `config.status`, `aclocal.m4`, `libtool`, Autotest `testsuite` scripts,
  `package.m4`, `atconfig`, `atlocal`, and anything built from a `.in` template. Commit
  the sources (`configure.ac`, `Makefile.am`, `*.in`, `*.at`, `conf/*.m4`) and
  regenerate with `autoreconf -fiv`.
- Flags in `Makefile.am` go in the Automake variables — `AM_CPPFLAGS`, `AM_CXXFLAGS`,
  `AM_LDFLAGS`, or per-target `<target>_CPPFLAGS` / `_CXXFLAGS` / `_LIBADD` / `_LDADD`.
  Never assign `CPPFLAGS`, `CXXFLAGS`, or `LDFLAGS` in a `Makefile.am`; those belong to
  the user and to configure.
- Dependencies are found in `configure.ac` (`PKG_CHECK_MODULES`, `AC_ARG_WITH`, or a
  macro in `conf/`) and exported with `AC_SUBST` as `FOO_CFLAGS` / `FOO_LIBS`. Optional
  features use `AM_CONDITIONAL` and an `if FOO ... endif` block in `Makefile.am`, never
  a hardcoded path or a check run at `make` time.
- VPATH (out-of-source) builds must work: refer to sources as `$(srcdir)/...` or
  `$(top_srcdir)/...` and to build products as `$(builddir)/...` or `$(top_builddir)/...`.
  `make distcheck` builds from the tarball in a separate directory with a read-only
  source tree and runs `make check`; it must pass before a release, and it is the test
  that catches a missing `$(srcdir)` or a file left out of the distribution.
- Every file the build or tests need that isn't a compiled source — test data,
  `*.baseline` files, `*.at` files, `*.in` templates, `.bescmd` inputs — is listed in
  `EXTRA_DIST` (or a `dist_*_DATA` variable). Anything a build or test creates is in
  `CLEANFILES` or `DISTCLEANFILES`.
- Code is built as Libtool libraries (`lib_LTLIBRARIES` / `noinst_LTLIBRARIES`) that
  the executables, modules, **and** the tests link against — tests never recompile the
  library's sources. A new directory is added to its parent's `SUBDIRS` and its
  `Makefile` (plus any `atlocal` / `*.in` outputs) to `AC_CONFIG_FILES` in
  `configure.ac`.
- Layout follows the existing tree: each component (`dispatch/`, `http/`, `dap/`, and
  each handler under `modules/<name>/`) holds its sources and headers together, with
  CppUnit tests in `<component>/unit-tests/` and Autotest suites in `<component>/tests/`.
- Build modes are configure options, not hand-edited flags: `--enable-developer`
  (`-g3 -O0 -Wall`, defines `DEVELOPER`, sets the `BES_DEVELOPER` conditional; without
  it the build is optimized with `-DNDEBUG`), `--enable-asan` (ASan + UBSan), and
  `--enable-coverage` (gcov). Code compiled only in developer mode is guarded by
  `#ifndef NDEBUG` or the `DEVELOPER` macro, and must not change release behavior.
- Warnings: `-Wall` is on in developer builds; add `-Wextra` there too. Move to
  `-Werror` once the codebase is warning-clean, as an opt-in configure option
  (`--enable-werror`) that CI turns on. Don't silence a warning with a pragma without a
  comment saying why.
- Automake doesn't emit `compile_commands.json`. Generate it with `bear -- make` (or
  `compiledb make`) in the build directory so clang-tidy and editors see the real
  flags; don't commit it.

### Language & style

- RAII everywhere. No naked `new`/`delete`, and no `malloc`/`free` except to wrap a C
  API. Owning pointers are `std::unique_ptr` (`std::make_unique` exists in C++14);
  `std::shared_ptr` only when ownership is genuinely shared. Raw pointers and
  references are non-owning, always.
- `std::string` and standard containers over `char*` and C arrays. No `strcpy`,
  `sprintf`, or `gets`; use `snprintf` with its return value checked, or `std::string`.
- Exceptions are allowed for errors at API boundaries: derive from `std::exception`,
  throw by value, catch by `const&`. Destructors are `noexcept`. Never let an exception
  escape across a C ABI, a thread boundary, or a callback invoked from C code. Don't
  use exceptions for routine control flow (not-found, end-of-input). If this project
  builds with `-fno-exceptions`, replace this bullet with status codes / enums.
- `<cstdint>` fixed-width types for file formats, wire formats, and anything whose size
  is part of the contract; `std::size_t` for sizes and indices. No mixed
  signed/unsigned comparisons without an explicit, commented cast.
- Never read or write binary data by casting a struct or pointer. Serialize field by
  field with explicit endianness.
- `const` by default; `override` on every virtual override; a virtual destructor on
  every polymorphic base; `nullptr`, never `NULL` or `0`; `enum class` over plain
  `enum`; `constexpr` over `#define` for constants.
- Headers are self-contained (each compiles on its own), use include guards
  (`#pragma once` is fine), and never contain `using namespace`. Include what you use;
  forward-declare where that avoids an include.
- C++17 library/language features are off-limits at the C++14 baseline: no
  `std::optional`, `std::variant`, `std::string_view`, `std::filesystem`, structured
  bindings, or `if constexpr`. If one is truly needed, that's a decision to record as
  an ADR, not a quiet workaround.

### Portability (Linux & macOS)

- Standard C++ and POSIX only. Anything available on just one OS (`<endian.h>`,
  `<malloc.h>`, `strlcpy`/`strlcat`, `qsort_r`, `pthread_setname_np`, `memrchr`, the
  GNU-vs-XSI `strerror_r`) goes behind a small wrapper in one file, with
  `#if defined(__linux__)` / `#if defined(__APPLE__)` branches, each exercised by a
  test. When the question is "does this function/header exist" rather than "which OS",
  prefer an Autoconf check (`AC_CHECK_FUNCS`, `AC_CHECK_HEADERS`) and the resulting
  `HAVE_FOO` macro from `config.h`.
- Plain `char` signedness differs across platforms (e.g. x86_64 vs aarch64 Linux). Use
  `signed char`, `unsigned char`, or `uint8_t` wherever it matters.
- No GNU-only compiler extensions (statement expressions, VLAs, bare `__attribute__`).
- Never hardcode `.so`, `.dylib`, or `lib` prefixes (macOS uses `.dylib`; BES modules
  are loaded by name from `bes.conf`). Link through Libtool `.la` files and let Libtool
  pick the platform's suffix and rpath. Uninstalled programs in the build tree are
  Libtool wrapper scripts that already find uninstalled libraries — don't set
  `LD_LIBRARY_PATH`, and never depend on `DYLD_LIBRARY_PATH`, which macOS SIP strips
  when a protected binary such as `/bin/sh` is in the process chain.
- OS-specific build logic belongs in `configure.ac` (e.g. `AS_CASE([$host], [*darwin*], ...)`
  setting an `AM_CONDITIONAL` like `DARWIN`), not in ad hoc `uname` calls in a
  `Makefile.am`.
- macOS filesystems are usually case-insensitive and Linux's are case-sensitive:
  `#include` paths and file names must match case exactly.
- Parallelism: `make -j<N>` and `make check -j<N>` (or `TESTSUITEFLAGS=-j<N>` for an
  Autotest suite). Get `<N>` with `getconf _NPROCESSORS_ONLN`, which works on both OSes —
  not `nproc` (Linux-only) or `sysctl -n hw.ncpu` (macOS-only). A directory whose tests
  can't run in parallel says so with `.NOTPARALLEL:` and a comment explaining why.

### Testing

- `make check` from the top of the build tree is the single command that runs
  everything; `make distcheck` runs the same suite from a VPATH build of the tarball. A
  test that `make check` doesn't run doesn't exist.
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
  `testsuite.dir/` and `testsuite.log`. Baselines change only through the one update
  mode: `./testsuite --baselines=yes [test numbers or -k keyword]`, which writes
  `<input>.baseline.tmp` instead of comparing; review it and move it over the old
  baseline. Never hand-edit a baseline. Update mode is off by default and never on in
  CI. Review `git diff` on baseline files like source code; a baseline change in a PR
  needs a reason.
- Output that legitimately varies between runs (dates, version strings, temp paths) is
  scrubbed before comparison with the existing macros (`REMOVE_VERSIONS`, the
  `*_SCRUB_DATES_*` variants) or a new one added to the shared `.m4`, not by loosening
  the comparison or regenerating the baseline each run.
- Running pieces of the suite: in a `tests/` directory, `./testsuite -l` lists tests,
  `./testsuite 12 15-20` or `./testsuite -k <keyword>` runs a subset, and
  `./testsuite -v -d -x <N>` shows a failing test's commands and keeps its scratch
  directory. In a `unit-tests/` directory, `make check TESTS=fooT` runs one program and
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
- New logic isn't done without a test. A bug fix isn't done without a regression test
  that fails before the fix and passes after.
- The suite also runs clean under ASan + UBSan on Linux and macOS: configure with
  `--enable-asan` (add `--enable-developer` for usable stack traces) and run
  `make check`. LeakSanitizer isn't available on macOS, so leak checking is Linux-only;
  Valgrind with the checked-in suppression files (`vg-*-supp.txt`, see
  `README-VALGRIND.md`) is the alternative.

### Static analysis & formatting

- `clang-format` (config committed to the repo) is the formatting source of truth —
  don't hand-format around it.
- `clang-tidy` with a committed `.clang-tidy` runs clean before a change is considered
  finished, or a new finding is suppressed with a `NOLINT` and a comment saying why.

### Documentation

- Every public function/class in an installed header (`include_HEADERS` /
  `pkginclude_HEADERS`) gets a one-line Doxygen-style comment: what it does,
  ownership/lifetime of pointer or reference arguments and returns, units where
  relevant, whether it can throw, and thread-safety when it isn't obvious.
