# Fixing macOS libxml2/libzstd runtime linkage for `functions` and `gdal_module`

## 2026-09-03 11:00 — Prompt

> In this code there seems to be a problem with the way the functions module is linked. That code is in
> `modules/functions` and the runtime error I see is a dlopen failure for `libfunctions_module.so`:
> `Library not loaded: @rpath/libxml2.2.dylib`. The linkage for this library is in the local Makefile.am.
> How can I fix the linkage of this module so that on OSX it will find libxml2?

## Environment

- macOS (Darwin 25.5.0, arm64), in-source build under `bes/`, prefix `/Users/jhrg/src/opendap/hyrax/build`.
- Configured with `--prefix=.../build --enable-developer --with-dependencies=.../build/deps LDFLAGS=-L/opt/homebrew/lib CPPFLAGS=-I/opt/homebrew/include`.
- The shell had an Anaconda "(base)" environment active (`CONDA_DEFAULT_ENV=base`, `/opt/homebrew/anaconda3/bin` and `condabin` on `PATH`). This turned out to matter — see root cause below.
- GDAL is the vendored copy in `build/deps` (`deps/bin/gdal-config`).

## Diagnosis

`otool -L modules/functions/.libs/libfunctions_module.so` showed:

```
@rpath/libxml2.2.dylib (compatibility version 16.0.0, current version 16.9.0)
```

Compatibility/current version 16.0.0/16.9.0 matches `/opt/homebrew/anaconda3/lib/libxml2.2.dylib` exactly
(`otool -D` on that file reports its own install name as `@rpath/libxml2.2.dylib` — it's built relocatable).
The module's `LC_RPATH` entries (`deps/lib`, `@loader_path`, `deps/proj/lib`) don't include
`/opt/homebrew/anaconda3/lib`, so dyld can't resolve it at runtime — hence the dlopen failure.

Root cause, traced through the actual link command (`libtool --mode=link` / `ld64 -v`):

- `libdap`'s `dap-config --server-libs`/`--client-libs` (used to set `DAP_SERVER_LIBS`/`DAP_CLIENT_LIBS` in
  `configure.ac`) emit a **bare** `-lxml2` with no accompanying `-L`, relying on the linker's default search
  path to find the system copy at `/usr/lib/libxml2.2.dylib`.
- `modules/functions/Makefile.am` (and `modules/gdal_module/Makefile.am`) also link `$(GDAL_LDFLAGS)`, which
  is built in `configure.ac` from `gdal-config --libs` plus `gdal-config --dep-libs`. The latter is **baked
  in at GDAL's own build time** and simply records whatever `-L` search paths were active on the machine
  that built the vendored GDAL. In this deps build that includes `-L/opt/homebrew/anaconda3/lib` (needed at
  GDAL-build time to find `-lexpat.1`).
- Because `/opt/homebrew/anaconda3/lib` also happens to contain its own copies of `libxml2.2.dylib` *and*
  `libzstd.1.dylib` (both built relocatable, `install_name @rpath/...`), and because it's the only `-L`
  directory on the link line that actually contains those library names, the linker silently binds the bare
  `-lxml2` (and, in `gdal_module`'s case, `-lzstd`) to the Anaconda copies instead of the system/Homebrew
  ones. No rpath was ever added for the Anaconda directory (nor should one be), so the resulting `.so`
  fails to dlopen outside of a shell with that Anaconda environment on `PATH`.
- This is not `modules/functions`-specific: `modules/gdal_module/.libs/libgdal_module.so` had the identical
  `@rpath/libxml2.2.dylib` problem, and independently a matching `@rpath/libzstd.1.dylib` problem (which
  library gets hijacked depends on the exact relative order of `-L` flags in each module's link line).

Verified experimentally (rebuilding just `libfunctions_module.la` and inspecting `otool -L` /
`otool -l ... | grep RPATH` after each change) before committing to a fix.

## Fix

Two changes, both in `configure.ac`:

1. **Targeted pin** (`XML2_LDFLAGS`): compute an explicit, high-priority `-L` to the real system libxml2
   directory (`$(xcrun --show-sdk-path)/usr/lib` on Darwin) and add it to the link line *before*
   `$(GDAL_LDFLAGS)` in `modules/functions/Makefile.am` (`LIBADD`) and `modules/gdal_module/Makefile.am`
   (`libgdal_module_la_LDFLAGS`). This is enough to fix the reported `libxml2` failure on its own.
2. **Root-cause fix** (`bes_prune_conda_L_paths`): strip any `-L` path containing `conda` out of
   `GDAL_LDFLAGS` right after it's assembled from `gdal-config --libs`/`--dep-libs`, mirroring the existing
   `bes_prune_missing_L_paths` helper. This addresses the whole class of problem (libxml2, libzstd, and any
   other library Conda happens to ship a relocatable copy of), not just the one library reported. With this
   in place, `gdal_module` also stopped hijacking `libzstd`.

Kept `XML2_LDFLAGS` in place alongside the `bes_prune_conda_L_paths` fix as defense in depth (it's harmless
and guards against a similar leak from a source other than Conda).

Files changed: `configure.ac`, `modules/functions/Makefile.am`, `modules/gdal_module/Makefile.am`.
`configure`, `Makefile.in`, `aclocal.m4`, etc. are not checked into this repo, so no generated files needed
to be committed; they were regenerated locally via `autoreconf --force --install` + `./config.status
--recheck` purely to test.

## Validation

- `otool -L` on both `libfunctions_module.so` and `libgdal_module.so` now show `/usr/lib/libxml2.2.dylib`
  and `/opt/homebrew/opt/zstd/lib/libzstd.1.dylib` / `/usr/lib/libexpat.1.dylib` (absolute paths, not
  `@rpath`).
- `modules/functions/tests`: `./testsuite` → **all 96 tests pass** (previously failed at test 1 with the
  reported dlopen error).
- `modules/gdal_module/tests`: `./testsuite` → module now loads; **14/23 tests pass** (previously all 23
  failed with a dlopen error). The remaining 9 failures are baseline/content mismatches (e.g. DMR attribute
  differences), not dlopen/linkage errors — they line up with the in-progress GDAL version-update work
  already underway on this branch (`modules/gdal_module/reader/gdal_utils.cc`,
  `modules/gdal_module/writer/FONgTransform.cc`, both locally modified and untouched by this fix). Not
  investigated further here as out of scope for the linkage question.
- Did not run the full top-level `make check` — only the two directly affected module test suites were
  exercised.

## Caveats / follow-up

- `bes_prune_conda_L_paths` matches the substring `conda`, which covers Anaconda/Miniconda but not, e.g.,
  Miniforge (`/opt/homebrew/Caskroom/miniforge/...` — no "conda" in the path). Widen the match if that shows
  up in practice.
- The durable fix is upstream of this repo: rebuild the vendored GDAL in hyrax-dependencies with any
  Conda environment deactivated, so `gdal-config --dep-libs` never bakes in a Conda path to begin with. The
  `configure.ac` filter here is a workaround for consuming an already-contaminated `gdal-config`, not a
  substitute for fixing the deps build.
- This only filters `GDAL_LDFLAGS`. If another dependency's `*-config`/pkg-config output ever bakes in a
  similar Conda `-L` on a machine with Conda active at deps-build time, it would need the same treatment.
