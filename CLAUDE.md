# CLAUDE.md

Guidance for Claude Code when working in the `bes` repository.

## What this repo is

The BES (Back-End Server) is the lower two tiers of Hyrax, OPeNDAP's three-tier data server:

1. **OLFS** (front end, Java, separate repo: github.com/opendap/olfs). Implements the DAP2/DAP4 web API and talks to the BES over PPT.
2. **BES framework** (this repo). A Unix daemon (`besdaemon` → `beslistener`) that parses BES XML requests, dispatches them to plugin modules, and streams responses back.
3. **BES modules** (this repo, `modules/`). Data handlers (netCDF, HDF4/5, DMR++, GDAL, FITS, CSV, FreeForm, NcML, ...) and response builders (fileout_netcdf, fileout_json, fileout_covjson, asciival, ...).

Most DAP2/DAP4 protocol code comes from **libdap4** (github.com/opendap/libdap4), an external dependency. Third-party libraries come from **hyrax-dependencies**.

- Language: C++14. Do not use features from C++17 or later.
- The codebase is long-lived and has downstream users. Prefer stability, compatibility, and small reviewable diffs.
- Ignore `retired/`.

## Layout

| Path | Contents |
| --- | --- |
| `dispatch/` | Core framework: registries, `BESDataHandlerInterface`, request/response handlers, containers, catalogs, caching, errors, logging (`BESLog`), debug (`BESDEBUG`), module loading (`BESModuleApp`) |
| `xmlcommand/` | Parses `<request>` documents into command objects |
| `ppt/` | PPT socket protocol between OLFS and BES |
| `server/` | `besdaemon` (supervisor), `beslistener` (`ServerApp`), `besctl`, `hyraxctl` |
| `standalone/` | `besstandalone`, an in-process BES with no network layer; used by most tests |
| `cmdln/` | `bescmdln` client |
| `http/` | Shared HTTP/curl client code, credentials, effective-URL cache |
| `aws/` | AWS SDK wrapper (built only when the SDK is enabled) |
| `dap/` | DAP2/4 response handlers built on libdap |
| `dapreader/` | Module that reads DAP response files directly; used in tests |
| `modules/` | All standard Hyrax modules, plus `modules/common/` shared test macros |
| `hello_world/`, `templates/` | Example module and templates for new modules |
| `conf/` | autoconf macros (`bes.m4`, etc.) |
| `docs/` | Design docs, plans, and work logs |
| `pugixml/`, `rapidjson/`, `nlohmann/` | Vendored third-party code. Do not edit. |

`modules/hdf4_handler` and `modules/hdf5_handler` are git subtrees (see `subtrees`). Coordinate changes there with the upstream repos.

To learn the architecture, start with `docs/BES_Overview.md`, then read `docs/BES_Framework.md`, `docs/BES_Modules.md`, `docs/BES_Configuration.md`, and `docs/BES_Creating_New_Modules.md`.

### Request flow (short version)

`beslistener` / `besstandalone` → `BESModuleApp` loads the modules listed in `BES.modules` → `BESXMLInterface` parses the request into commands → each command fills a `BESDataHandlerInterface` → a `BESResponseHandler` (looked up by action name) builds the response object, calling the `BESRequestHandler` registered for the container's type → a `BESTransmitter` streams the result.

Modules register their handlers in `initialize()` and remove them in `terminate()`.

## Build (autotools only)

Ignore the top-level `CMakeLists.txt`. It is a relic.

```sh
# prefix must be set; for this checkout it is normally ../build (as an absolute path)
autoreconf --force --install --verbose        # fresh checkout only
./configure --prefix="$prefix" --with-dependencies="$prefix/deps" --enable-developer
make -j20
make check
```

- `--enable-developer` turns on debug and assertion behavior. Release builds define `NDEBUG`, which compiles `BESDEBUG` out. When you change assertions or diagnostics, consider both kinds of build.
- Other configure options: `--enable-asan`, `--enable-coverage`, `--without-cmr`, `--without-ngap`, `--without-s3`, `--with-gdal=`, `--with-aws`.
- Modules are built only when configure finds their dependencies. Check `modules/configured_features.txt` to see what was enabled.
- Edit `configure.ac` and `Makefile.am`, not generated `configure`, `Makefile.in`, or `config.h.in`.

## Testing

There are two kinds of tests.

- **Unit tests**: CppUnit, in `*/unit-tests/`. Run them with `make check` in that directory. Most test binaries accept `-d` (debug) and can run a single test by name. See the `main()` in each test file.
- **Integration tests**: GNU autotest, in `*/tests/` and `bes-autotest/`. Each test is a `.bescmd` XML request run through `besstandalone` and diffed against a baseline. The shared macros are in `modules/common/handler_tests_macros.m4`.
  - Run all of them: `make check` in the tests directory.
  - Run one: `./testsuite -k <keyword>` or `./testsuite <N>`. Add `-v` (verbose) or `-d` (keep the test directory).
  - Regenerate a baseline: `./testsuite --baselines=yes <N>`. This writes `<baseline>.tmp`. Review it before you replace the baseline. Never bulk-regenerate baselines to make failures go away.
- `make distcheck` is a supported validation path. Run it when you change build, packaging, or install behavior.
- If parallel `make check -jN` causes failures, rerun serially and say so.

## Conventions

- Match the style already in the file you touch. Do not reformat unrelated code.
- Errors: throw `BESInternalError`, `BESSyntaxUserError`, `BESNotFoundError`, `BESForbiddenError`, and so on, with `__FILE__, __LINE__`. Libdap `Error`s are translated in `dap/`.
- Debug output: `BESDEBUG("<context>", ...)`. The context name is usually the module name.
- Configuration is read through `TheBESKeys`. If you add or change a key, update the `.conf.in` template and say what this changes at runtime.
- Be conservative with ownership and lifetimes in older pointer-heavy code. Avoid changes that affect the API or ABI (`abi_checker.xml`) unless they are requested.
- High-risk areas: protocol and response behavior, server startup and configuration, installed layout, packaging, `besd`, and systemd files.

## CI

- `.travis.yml`: autotools build, `make check`, `make distcheck`, coverage, and Docker images (`Dockerfile`, `travis/build-rhel-docker.sh`).
- `.github/workflows/`: macOS builds (Intel and ARM64).
- CI uses `--disable-dependency-tracking --with-dependencies=$prefix/deps --enable-developer`. Do not hardcode local paths into CI or Docker files.

## Working rules

- Write docs, plans, and work logs as Markdown in `docs/`.
- Say exactly what validation you ran and what you did not run. Name any external prerequisites the work depends on, such as libdap, hyrax-dependencies, AWS, or system packages.
- Do not make up data. Be concise and critical.
- Do not revert unrelated changes in a dirty worktree. Do not run destructive git commands unless you are asked to.

## Writing Documents

- For all documents you write, use markdown and store them in the `docs` directory
- For all work, write a log explaining your reasoning unless explicitly told otherwise
- In the work log, start each new set of entries with a date/time stamp
- In the work log, include the query/prompt leading to the work
- For a plan, write the plan to a markdown document unless explicitly told otherwise
