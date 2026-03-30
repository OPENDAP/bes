# AGENTS.md

## Scope

These instructions apply to the entire `bes` repository.

## Project Context

- `bes` is the Back-End Server for Hyrax and is a long-lived C++ codebase with downstream users who depend on stable behavior.
- Prefer compatibility, behavioral stability, and small reviewable diffs over broad cleanup or refactoring.
- Treat changes to protocol behavior, server startup/configuration, packaging, and installed layout as high risk.

## Primary Build System

- Use autotools for normal development work in this repository.
- A top-level `CMakeLists.txt` exists, but ignore that since it is a relic.

## Autotools Workflow

For a fresh git checkout:

```sh
autoreconf --force --install --verbose
./configure --prefix="$prefix" --with-dependencies="$prefix/deps" --enable-developer
make -j
make check
```

For a source-distribution style build:

```sh
./configure
make -j
make check
```

Notes:

- Check that `prefix` is defined before using commands that rely on it.
- Use `--with-dependencies=<path>` when BES dependencies are installed outside standard system paths.
- `--enable-developer` is the normal developer-mode configure option in this repo.
- Useful configure toggles defined at the top level include `--enable-asan`, `--enable-coverage`, `--without-cmr`, `--without-ngap`, and `--without-s3`.
- `make distcheck` is a supported validation path here; top-level build files already carry `AM_DISTCHECK_CONFIGURE_FLAGS` logic for dependency-aware distcheck runs.

## Testing Expectations

- For code changes, run focused validation first, then broaden test scope when the risk warrants it.
- Default autotools validation is `make check`.
- Parallel test runs are used in CI, but `INSTALL` notes that some tests may object to parallel execution; if `-j` causes issues, retry serially and mention that in your summary.
- If you change build, packaging, or install behavior, consider whether `make distcheck` or the Docker build path should also be exercised.

## CI And Container Workflows

- CI is defined in [`.travis.yml`](.travis.yml) and uses autotools, `make check`, `make distcheck`, coverage builds, and Docker image builds.
- Additional CI in [`.github/workflows`](.github/workflows) for OSX Intel and ARM64.
- Docker builds are driven by [`Dockerfile`](Dockerfile) and [`travis/build-rhel-docker.sh`](travis/build-rhel-docker.sh).
- CI commonly configures with `--disable-dependency-tracking`, `--with-dependencies=$prefix/deps`, and `--enable-developer`.
- Docker and CI assume external dependency artifacts such as hyrax-dependencies and libdap builds; do not hardcode local-only assumptions into those paths.

## Generated Files And Build Artifacts

- Prefer editing source inputs such as [`configure.ac`](configure.ac) and [`Makefile.am`](Makefile.am), not generated outputs like `configure`, `Makefile.in`, or `config.h.in`, unless the task explicitly requires regenerating them.
- This repository may contain checked-in generated files and local build artifacts. Do not delete or revert unrelated generated files just to make the tree look clean.
- If you regenerate autotools outputs, keep the resulting changes tightly scoped and mention that regeneration was performed.

## Documentation

- Top-level API docs are built with `make docs`.
- Doc inputs are generated from top-level templates such as `doxy.conf.in` and `main_page.doxygen.in`; keep template/generated relationships consistent with the chosen build workflow.

## Configuration Awareness

- BES is not just a library build; installation layout and runtime configuration matter.
- `INSTALL` and `README.md` both assume post-install configuration under `etc/bes`, including `bes.conf` and module configuration files.
- When changing defaults, installed paths, service scripts, or startup behavior, call out runtime configuration impact explicitly.

## Legacy C++ Constraints

- Match local style in touched files; avoid unrelated formatting sweeps.
- Avoid API or ABI-impacting changes unless explicitly requested.
- Be conservative with ownership, lifetime, and assertion behavior in older pointer-heavy code.
- Developer mode and release mode differ here (`--enable-developer` vs. `NDEBUG` builds), so consider both when changing assertions, debug-only behavior, or diagnostics.

## Change Discipline

- Do not revert unrelated local changes in a dirty worktree.
- Keep edits narrowly scoped to the request.
- If you encounter unexpected repository changes that conflict with the task, stop and ask how to proceed.
- Do not run destructive git commands unless explicitly requested.

## Review Priorities

When asked to review, prioritize:

1. Behavioral regressions in server responses, protocol handling, and installed/runtime behavior
2. Memory/resource safety and ownership lifetime issues
3. Build-system or packaging regressions in autotools, distcheck, CI, or Docker flows
4. Configuration/install-layout regressions that would affect deployed BES instances
5. Missing or weak regression coverage

## Communication

- State assumptions and environment details explicitly, especially configure flags and dependency locations.
- If full validation was not run, say exactly what was run and what was not.
- If a change depends on external prerequisites such as `libdap`, hyrax-dependencies, AWS-fetched artifacts, or system packages, note that clearly.
