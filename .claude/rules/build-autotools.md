---
paths:
  - "configure.ac"
  - "**/Makefile.am"
  - "conf/**"
  - "**/*.in"
  - "**/*.mk"
---

# Build rules — Autoconf / Automake / Libtool

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
- When the question is "does this function/header exist" rather than "which OS", use
  an Autoconf check (`AC_CHECK_FUNCS`, `AC_CHECK_HEADERS`) and the resulting `HAVE_FOO`
  macro from `config.h`. OS-specific build logic belongs in `configure.ac` (e.g.
  `AS_CASE([$host], [*darwin*], ...)` setting an `AM_CONDITIONAL` like `DARWIN`), not in
  ad hoc `uname` calls in a `Makefile.am`.
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
- Never hardcode `.so`, `.dylib`, or `lib` prefixes (BES modules are loaded by name
  from `bes.conf`). Link through Libtool `.la` files and let Libtool pick the platform's
  suffix and rpath. Uninstalled programs in the build tree are Libtool wrapper scripts
  that already find uninstalled libraries — don't set `LD_LIBRARY_PATH`, and never
  depend on `DYLD_LIBRARY_PATH`, which macOS SIP strips when a protected binary such as
  `/bin/sh` is in the process chain.
- Build modes are configure options, not hand-edited flags: `--enable-developer`
  (`-g3 -O0 -Wall`, defines `DEVELOPER`, sets the `BES_DEVELOPER` conditional; without
  it the build is optimized with `-DNDEBUG`), `--enable-asan` (ASan + UBSan), and
  `--enable-coverage` (gcov). Code compiled only in developer mode is guarded by
  `#ifndef NDEBUG` or the `DEVELOPER` macro, and must not change release behavior.
- Don't silence a warning with a pragma without a comment saying why.
- Parallelism: `make -j<N>` and `make check -j<N>`. Get `<N>` with
  `getconf _NPROCESSORS_ONLN`, which works on both OSes — not `nproc` (Linux-only) or
  `sysctl -n hw.ncpu` (macOS-only). A directory whose tests can't run in parallel says
  so with `.NOTPARALLEL:` and a comment explaining why.
- Automake doesn't emit `compile_commands.json`. Generate it with `bear -- make` (or
  `compiledb make`) in the build directory so clang-tidy and editors see the real
  flags; don't commit it.
