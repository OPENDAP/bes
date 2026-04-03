# AGENTS.md

## Scope

These instructions apply to the entire `bes` repository.

## Project Context

- `bes` is the Back-End Server for Hyrax.
- It is both:
  - a framework for processing BES XML requests, and
  - a plugin host for data-reader, catalog, function, and file-output modules.
- The most important architectural split is:
  - framework code in top-level directories such as `dispatch`, `server`, `ppt`, `xmlcommand`, `http`, and `standalone`,
  - format- and feature-specific plugins under `modules/`.
- Prioritize compatibility, behavioral stability, and small diffs over cleanup refactors.

## Framework Map

When working in framework code, treat these directories as the main execution path:

- `dispatch/`: core execution state, registries, default framework services, plugin loading, error handling, catalogs, container storage, and transmitters.
- `server/`: daemon/listener process, signal handling, and connection lifecycle.
- `ppt/`: transport protocol and chunked communication with OLFS.
- `xmlcommand/`: parsing BES XML requests into command objects and response-handler plans.
- `http/`: shared remote-access infrastructure used by framework and modules.
- `standalone/`: in-process execution path for tests and command-line workflows.
- `dap/`: core DAP response handlers, transmitters, and DAP service registration used by many modules.

The high-level request flow is:

```text
ServerApp
  -> PPTServer
  -> BESServerHandler
  -> BESXMLInterface
  -> response handler execution
  -> request handler dispatch
  -> transmitter
```

## Module Map

Use the module categories from `docs/module-deep-dive.md` when reasoning about `modules/`.

### Core reader modules

- `modules/netcdf_handler`
- `modules/hdf4_handler`
- `modules/hdf5_handler`
- `modules/dmrpp_module`
- `modules/csv_handler`
- `modules/fits_handler`
- `modules/freeform_handler`
- `modules/xml_data_handler`
- `modules/ncml_module`
- `modules/gdal_module`

### Writer and transformation modules

- `modules/fileout_netcdf`
- `modules/fileout_json`
- `modules/fileout_covjson`
- `modules/usage`
- `modules/w10n_handler`

### Function modules

- `modules/functions`
- `modules/ugrid_functions`
- `modules/debug_functions`

### Catalog and container/storage modules

- `modules/gateway_module`
- `modules/httpd_catalog_module`
- `modules/cmr_module`
- `modules/s3_reader`

### Support-only or non-runtime directories

- `modules/common`
- `modules/data`
- `modules/docs`

### NGAP note

- In this checkout, top-level `modules/ngap_module` appears incomplete or vestigial.
- The live NGAP-related code used by BES is under `modules/dmrpp_module/ngap_container/`.
- Do not assume `modules/ngap_module` is the active implementation without checking current build files first.

## Build Systems

- Only autotools at this time.

## Autotools Workflow

For a fresh checkout:

```sh
autoreconf --force --install --verbose
./configure --prefix=$prefix --with-dependencies=$prefix/deps --enable-developer
make -j
make -j check
```

For an already-configured tree:

```sh
make -j
make -j check
```

Notes:

- Use module- or directory-scoped tests first when changes are localized.
- `TESTSUITEFLAGS=-j<N>` can be used to parallelize autotools test runs.
- Many module tests depend on generated config files and locally built shared libraries; check the nearest `tests/Makefile.am` and `*.conf.in` before assuming a test is standalone.

## Testing Expectations

- For framework changes, prefer targeted coverage in the affected area first:
  - `dispatch/unit-tests`
  - `xmlcommand/unit-tests`
  - `http/unit-tests`
  - `dap/unit-tests`
  - module-specific `tests/` and `unit-tests/`
- For request-lifecycle or transport changes, include at least one path that exercises real request execution, typically via `standalone`, `server` tests, or autotools integration suites.
- For module changes, run the narrowest module test suite that actually covers the behavior you touched.
- If you do not run full validation, say exactly what was not run.

## BES-Specific Review Priorities

When reviewing or modifying code, prioritize:

1. Request-lifecycle regressions in XML parsing, planning, dispatch, transmit, timeout, or cleanup.
2. Behavioral regressions in DAP2/DAP4 responses.
3. Container type resolution, catalog behavior, and storage backend correctness.
4. Memory/resource lifetime issues in pointer-heavy and cache-heavy code.
5. Plugin registration symmetry: anything added in `initialize()` should usually be removed in `terminate()`.
6. Build and test regressions across autotools and CMake when shared logic changes.

## Change Discipline

- Do not broaden changes from one module into unrelated modules without a concrete reason.
- Keep framework changes especially small and explicit; many modules depend on registry behavior in `dispatch/`.
- Avoid opportunistic renames in older code unless required by the task.
- Be conservative with ownership changes in request handlers, response handlers, caches, and container storage code.
- Do not edit generated parser/scanner outputs if the real source is available:
  - edit `*.yy` or `*.lex` sources rather than generated `*.tab.cc`, `*.tab.hh`, or `lex.*.cc`,
  - unless the task explicitly requires updating generated files too.

## Documentation Pointers

- Framework overview: `docs/framework-deep-dive.md`
- Module inventory: `docs/module-deep-dive.md`
- Existing legacy docs:
  - `docs/BES_Server_Architecture.doc`
  - `docs/BES_PPT.doc`
  - `docs/Overview.md`

When updating docs, prefer keeping those deep-dive documents aligned with the code rather than copying generic Hyrax descriptions.

## Communication

- Be explicit about whether a change is in framework code or module code.
- Name the execution layer affected: transport, XML planning, dispatch, request handler, response handler, transmitter, catalog, or container storage.
- If repository structure looks stale or partially vestigial, say so explicitly instead of normalizing it away.
