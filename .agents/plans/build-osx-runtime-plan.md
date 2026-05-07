# Reduce macOS CI Runtime in `build-osx.yml`

## Summary

Cut macOS workflow time primarily by reducing duplicate PR coverage and caching the expensive parts that are currently rebuilt every run.

Defaults chosen:

- Keep building `libdap4` from source, but cache that build output and compile results rather than introducing a new prebuilt artifact flow.
- Keep both `macos-15` and `macos-15-intel` for pull requests, `main`/`master` pushes, and `workflow_dispatch`.

## Key Changes

- Split the current matrix behavior by event:
  - PRs: run both `macos-15` and `macos-15-intel`.
  - Pushes to `main`/`master` and manual runs: run both `macos-15` and `macos-15-intel`.
  - Keep existing draft-PR skip behavior and current path filters.
- Add compiler caching with `ccache`:
  - Install `ccache` via Homebrew.
  - Export `CC="ccache clang"` and `CXX="ccache clang++"` before both `libdap4` and BES configure/build steps.
  - Set `CCACHE_DIR` under `$HOME/.ccache`, cap size explicitly, and print `ccache --show-stats` before and after build for observability.
- Add GitHub Actions caches keyed by runner architecture and dependency inputs:
  - Cache Homebrew downloads and installed formula state only if done via a well-supported action path; otherwise do not try to hand-roll Homebrew cellar caching.
  - Cache `~/.ccache`.
  - Cache the built `libdap4` install prefix under `$HOME/install` only for the `libdap4` portion, keyed by:
    - runner (`matrix.runs-on`)
    - `libdap4-snapshot` contents from this repo
    - workflow file hash for relevant build flags
  - Restore the cached prefix before the `libdap4` step; skip clone/build/install when the cache hits and the expected `libdap` files are present.
- Replace raw `brew install ...` with a maintained setup action if possible:
  - Prefer `Homebrew/actions/setup-homebrew` plus a package install step, or a commonly used cache-aware action for Brew formulas.
  - If that is not acceptable, keep `brew install` as-is and rely on `ccache` and `libdap4` caching as the primary savings.
- Tighten the workflow structure without changing BES behavior:
  - Keep `autoreconf`, `./configure`, `make`, `make install`, `besctl start`, `make check`, `besctl stop`.
  - Keep `CONFIGURE_OPTIONS`, `--with-dependencies=$prefix/deps`, and current Homebrew include/lib flags unchanged unless required for `ccache`.
  - Preserve AWS-fetched `hyrax-dependencies` tarball behavior for now; do not redesign that artifact flow in this plan.

## Public and Interface Changes

- Workflow behavior changes:
  - Pull requests retain dual-architecture macOS coverage.
  - `main`/`master` pushes and manual runs retain dual-architecture macOS coverage.
- No intended changes to BES build flags, install layout, runtime configuration, or test semantics.

## Test Plan

- Validate workflow syntax locally by inspecting the rendered YAML and checking matrix/job conditions.
- For one cache-cold run on each architecture:
  - Confirm `brew`, `hyrax-dependencies`, `libdap4`, and BES still build successfully.
  - Confirm `make check` still runs after `besctl start`.
- For one cache-warm rerun on the same branch and same runner:
  - Confirm `ccache` hits increase materially.
  - Confirm `libdap4` step is skipped or reduced to a validation step on cache hit.
  - Compare end-to-end runtime against the current workflow.
- On a PR:
  - Confirm both macOS runners execute.
- On a `main`/`master` push or manual dispatch:
  - Confirm both architectures execute.

## Assumptions

- `libdap4-snapshot` is the canonical repo-controlled version input for the macOS workflow’s `libdap4` dependency.
- Caching compiled `libdap4` under `$HOME/install` is acceptable as long as the cache key includes runner architecture and dependency-version inputs.
- Introducing a new external prebuilt `libdap4` artifact pipeline is out of scope for this change.
- Full validation remains `make check`; no `distcheck` or Docker validation is added to this macOS workflow optimization.
