
# Convenience Libraries and Autotools

What are 'best practices' for convenience libraries in a large autotools project with a number of developers?

In Autotools/libtool, “convenience libraries” are perfect for big projects—**as long as you treat them as purely internal**.

Here’s what “good” looks like and the gotchas to avoid.

# What a convenience library is (and isn’t)

* It’s a **libtool archive** you do **not install** (no `-rpath`): it just groups objects so you can link many modules into one installed lib or program cleanly.
* Automake pattern:

  ```make
  noinst_LTLIBRARIES = libawsutil.la libdaphelpers.la

  libawsutil_la_SOURCES   = a.cc b.cc
  libawsutil_la_CPPFLAGS  = $(AM_CPPFLAGS) $(AWS_CFLAGS)
  libawsutil_la_LIBADD    = $(AWS_DEPS_LIBS)   # external libs this code uses (OK; more below)

  libdaphelpers_la_SOURCES = x.cc y.cc
  ```
* Then you link your *installed* target to these:

  ```make
  lib_LTLIBRARIES = libhyrax.la
  libhyrax_la_SOURCES =
  libhyrax_la_LIBADD  = libawsutil.la libdaphelpers.la $(PUBLIC_LIBS)
  ```

# Best practices

## 1) Keep them **internal-only**

* Use `noinst_LTLIBRARIES` (or `check_LTLIBRARIES` for test-only).
* **Never install** convenience libs. Ship a *single* installed library that already contains their objects.

## 2) Let modules declare what they use, but don’t *depend* on propagation

* It’s fine for a convenience lib to list its external deps in `*_LIBADD` so it links when tested alone:

  ```make
  libawsutil_la_LIBADD = -laws-cpp-sdk-s3 -laws-crt-cpp
  ```
* **However**, for anything you **install** (e.g., `libhyrax.la` or a program), *explicitly list* all external libs you truly need **even if they’re already pulled by convenience libs**:

  ```make
  libhyrax_la_LIBADD = libawsutil.la libdaphelpers.la \
                       -laws-cpp-sdk-s3 -laws-crt-cpp $(OTHER_PUBLIC_LIBS)
  ```

  Why: relying on transitive `.la` `dependency_libs` is brittle (different linkers, `--as-needed`, relinking on installation, and some distros strip `.la` files entirely). Being explicit makes link behavior deterministic across macOS/Linux.

## 3) Keep boundaries coherent, not tiny

* Group code that changes together and shares headers. Avoid a forest of micro-libraries; each convenience lib should be a meaningful module (e.g., “aws utility layer”, “dap helpers”, “http client shim”).

## 4) Avoid cycles

* Don’t let `libA.la` and `libB.la` depend on each other. If you must, refactor common pieces into a third convenience lib `libCore.la` both can use. Cycles force ugly link-line repetitions and can misbehave with different linkers.

## 5) Use per-target flags, not global ones

* Prefer `_CPPFLAGS`, `_CXXFLAGS`, `_LDFLAGS` on each convenience lib so modules compile the same way whether they’re linked into tests or into the final lib:

  ```make
  libawsutil_la_CPPFLAGS = $(AM_CPPFLAGS) $(AWS_INCLUDE_FLAGS)
  libawsutil_la_LDFLAGS  = $(AM_LDFLAGS)
  ```
* Keep RPATH-only on installed libs/programs, not on convenience libs (they don’t have `-rpath` anyway).

## 6) Tests: link to convenience libs directly

* For unit tests that exercise internals, link them against the same convenience libs:

  ```make
  check_PROGRAMS = aws_util_tests
  aws_util_tests_SOURCES = tests.cc
  aws_util_tests_LDADD   = libawsutil.la $(CPPUNIT_LIBS) \
                           -laws-cpp-sdk-s3 -laws-crt-cpp
  ```

  This ensures tests see the same object code you ship.

## 7) Packaging metadata for *installed* outputs

* Provide **pkg-config** for your installed library; encode public vs. private deps:

  ```
  Name: libhyrax
  Version: 3.21
  Requires: libdap   # public (API-visible)
  Requires.private: aws-crt-cpp aws-sdk-cpp-s3  # static-only or impl deps
  Libs: -lhyrax
  Libs.private: -laws-crt-cpp -laws-cpp-sdk-s3
  ```

  Consumers linking dynamically usually only need `-lhyrax`; static users do `pkg-config --static --libs hyrax` to get the private deps.

## 8) macOS/Linux rpaths belong to installed things

* Set rpaths on the **installed lib or program** (not on convenience libs) so the runtime lookup is stable:

  ```make
  if DARWIN
  libhyrax_la_LDFLAGS += -Wl,-rpath,@loader_path/../lib -Wl,-rpath,$(aws_libdir)
  else
  libhyrax_la_LDFLAGS += -Wl,-rpath,'$$ORIGIN/../lib' -Wl,-rpath,'$(aws_libdir)'
  endif
  ```

# Antipatterns to avoid

* Relying on convenience libs’ `.la` to “pull” external deps into the final link—works until it doesn’t.
* Installing convenience libs (defeats the purpose and complicates ABI/semver).
* Over-fragmenting (too many small libs) or circular deps.

# Bottom line

Use convenience libs freely to organize a large codebase and to make tests link cleanly. For the things you **install**, be explicit about external libraries in `*_LIBADD` and ship clear pkg-config metadata. That gets you reproducible, portable links on macOS, Ubuntu, and RHEL—without surprising your users with missing “transitive” dependencies.
