# Rpath Makefile.am Deep Dive

## Work Log

### 2026-05-12 12:32:49 MDT

Prompt:

> Perform a deep dive examining the Makefile.am files of this repo and locate all places where 'rpath' is used or should be used. The libraries this code links to that are primary concern are found in ../build/deps/lib. Assume that libtool is used by this repo, but the code that builds the dependencies uses both autotools and cmake. In particular, I want to address problems that currently show up in the modules/dmrpp_module portion of the repo. Lastly, we have found that splitting rpath in two options (e.g., -Wl,rpath -Wl,<some path>) is fragile when using libtool and using the one-arg form is more robust.

Assumptions and environment details:

- Repository: `/Users/jhrg/src/opendap/hyrax/bes`.
- Current dependency library directory of concern: `../build/deps/lib`, configured in the existing build as `/Users/jhrg/src/opendap/hyrax/build/deps/lib`.
- Current generated Makefiles show a Darwin build, using `@loader_path`.
- Existing dirty worktree entries before this work included `docs/Makefile.am` and `docs/ncml-module-deep-dive.md`; I did not edit those.
- This repo uses libtool, so module and program links that define target-specific `*_LDFLAGS` need explicit `$(AM_LDFLAGS)` when they must inherit configure-provided rpath flags.

Commands used for the audit included:

- `rg --files -g 'Makefile.am'`
- `rg -n "rpath|RPATH|runpath|RUNPATH|-Wl,|LDFLAGS|LIBADD|LDADD|_la_LDFLAGS|_la_LIBADD|AM_LDFLAGS" -g 'Makefile.am' -g 'configure.ac' -g '*.m4'`
- `rg -n -g 'Makefile.am' -g 'configure.ac' -g '*.m4' -g '!templates/conf/libtool.m4' -- '-Wl,-rpath|-Wl, *rpath|-rpath'`
- Focused reads of `modules/dmrpp_module/**/Makefile.am`, `configure.ac`, and selected generated Makefiles.

## Findings

Explicit project rpath construction is centralized in `configure.ac`:

- `configure.ac` builds dependency-prefix rpath flags into substituted `AM_LDFLAGS` when `--with-dependencies` is used.
- It uses the one-argument linker form: `-Wl,-rpath,<path>`.
- For Darwin it adds `-Wl,-rpath,@loader_path`.
- For non-Darwin it adds `-Wl,-rpath,'$ORIGIN'`.
- GDAL-specific rpath construction also uses one-argument form for `deps/proj/lib`.

The only `Makefile.am` that directly adds rpath flags is:

- `aws/unit-tests/Makefile.am`, which adds `-Wl,-rpath,$(aws_libdir)`, OS-specific loader-relative rpath, and optional `$(aws_libdir64)`.

The fragile split form `-Wl,rpath -Wl,<path>` was not found in the audited `Makefile.am` files.

## DMR++ Problem

The DMR++ makefiles were dropping or bypassing configure-provided `AM_LDFLAGS` in places that link against libraries from the dependency prefix.

Affected source inputs:

- `modules/dmrpp_module/Makefile.am`
- `modules/dmrpp_module/unit-tests/Makefile.am`
- `modules/dmrpp_module/ngap_container/Makefile.am`
- `modules/dmrpp_module/ngap_container/unit-tests/Makefile.am`
- `modules/dmrpp_module/build_dmrpp_h4/Makefile.am`
- `modules/dmrpp_module/dmrpp_transmitter/Makefile.am`

Why this matters:

- Generated DMR++ Makefiles previously showed `AM_LDFLAGS` containing only coverage append variables in several directories, not the configured dependency rpath flags.
- Target-specific variables such as `libdmrpp_module_la_LDFLAGS`, `build_dmrpp_LDFLAGS`, and `build_dmrpp_h4_LDFLAGS` bypass Automake's normal `AM_LDFLAGS` use unless `$(AM_LDFLAGS)` is included explicitly.
- DMR++ links against HDF5, OpenSSL, libdap, XML2, and HDF4/HDF-EOS paths that may resolve through `../build/deps/lib`.

## Change Made

I made a narrow DMR++ patch:

- Removed empty `AM_LDFLAGS =` initializers in DMR++ directories so `@AM_LDFLAGS@` from `configure.ac` can be preserved.
- Changed `AM_LDFLAGS = $(DMRPP_LDFLAGS)` to `AM_LDFLAGS += $(DMRPP_LDFLAGS)` in `dmrpp_transmitter`.
- Added explicit `$(AM_LDFLAGS)` to target-specific link flags for:
  - `libdmrpp_module_la_LDFLAGS`
  - `build_dmrpp_LDFLAGS`
  - `build_dmrpp_h4_LDFLAGS`
- Regenerated the affected `Makefile.in` files with Automake so they now contain `@AM_LDFLAGS@` in the DMR++ areas.

## Broader Repo Risk

There are many other `Makefile.am` files with empty `AM_LDFLAGS =` assignments or target-specific `*_LDFLAGS`. These are not all bugs, but they are audit candidates when the target links against shared libraries from `--with-dependencies`.

Examples of higher-risk patterns:

- Empty `AM_LDFLAGS =` in modules that link to `$(H5_LDFLAGS)`, `$(NC_LDFLAGS)`, `$(HDF4_LDFLAGS)`, `$(HDFEOS2_LDFLAGS)`, `$(OPENSSL_LDFLAGS)`, `$(DAP_SERVER_LIBS)`, or `$(DAP_CLIENT_LIBS)`.
- Target-specific `*_LDFLAGS = ...` without `$(AM_LDFLAGS)` for installed modules or binaries.
- `AM_LDFLAGS = $(SOME_MODULE_LDFLAGS)` instead of `AM_LDFLAGS += $(SOME_MODULE_LDFLAGS)` when the configure-provided rpath must be retained.

Known good local example:

- `modules/functions/Makefile.am` explicitly avoids initializing `AM_LDFLAGS` and includes `$(AM_LDFLAGS)` in `libfunctions_module_la_LDFLAGS`.

I intentionally did not sweep every Makefile because this is a long-lived codebase and broad build-system edits would need more validation than a DMR++-focused fix.

## Validation

Validation performed:

- Audited `Makefile.am`, `configure.ac`, and generated DMR++ Makefiles with `rg`.
- Ran Automake on the six affected DMR++ Makefiles.
- Verified affected generated Makefile templates now preserve `@AM_LDFLAGS@` and include `$(AM_LDFLAGS)` in the target-specific DMR++ link variables.
- Ran `./config.status` for the six affected DMR++ Makefiles.
- Verified the configured DMR++ Makefiles now include `/Users/jhrg/src/opendap/hyrax/build/deps/lib` as `-Wl,-rpath,/Users/jhrg/src/opendap/hyrax/build/deps/lib`, plus `-Wl,-rpath,@loader_path`.

Attempted but did not complete:

- `make -C modules/dmrpp_module`
- This failed during autotools refresh/configure recheck before compiling DMR++, because `dap-config` was not found and configure reported `Could not find libdap`.

Validation not performed:

- I did not complete `make`.
- I did not run `make check` or `make distcheck`.

External prerequisites:

- Full validation depends on the configured dependency prefix, libdap, HDF5/HDF4/HDF-EOS, OpenSSL, XML2, and other hyrax-dependencies artifacts being available in the expected locations.
