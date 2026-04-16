# Plan: Add `make gh-docs` Target

## Goal
Add a top-level `gh-docs` target that automates publishing the Doxygen reference guide to the `gh-pages` branch using `make docs` instead of `doxygen doxy.conf`.

## Scope Notes
- Keep the change narrowly scoped to the top-level autotools inputs and the gh-pages publishing workflow.
- Require the current branch to be exactly `master` before any work starts.
- Require a clean worktree and index so rollback is reliable.
- Ensure any failure returns the repository to its initial state, including branch and `gh-pages` ref state.
- Prefer maintained source inputs such as `Makefile.am` and a helper script over hand-editing generated autotools outputs.

## Steps
1. Add a new top-level phony target `gh-docs` in `Makefile.am` that invokes a dedicated helper script instead of embedding the full workflow directly in the make recipe.
2. Add a top-level script for the workflow and include it in `EXTRA_DIST` so it is available in normal source distributions.
3. In the script, add preflight checks that fail before any mutation:
   - current branch must be `master`
   - worktree and index must be clean
   - `gh-pages` must exist locally or as `origin/gh-pages`
4. Capture the initial repo state before switching branches:
   - starting branch name
   - whether local `gh-pages` already exists
   - starting SHA for the `gh-pages` branch or remote-tracking ref used
   - whether `html/` existed on `master` before the run
5. Use `set -e` and a trap-based cleanup handler so any failing command restores the original state:
   - return to `master`
   - remove generated `html/` from `master` only if this run created it
   - reset the `gh-pages` work branch to its original SHA
   - delete any temporary local `gh-pages` branch created from `origin/gh-pages`
6. Implement the publish sequence to match the documented recipe, with `make docs` replacing direct doxygen invocation:
   - check out the `gh-pages` work branch
   - remove tracked `html/` and commit `"Removed old docs"` when that removal changes the index
   - check out `master`
   - run `$(MAKE) docs`
   - check out the `gh-pages` work branch
   - `git add --force html`
   - commit `"New docs"`
   - push `gh-pages`
   - check out `master`
7. Update `README.gh-pages.md` so the by-hand recipe uses `make docs` and mentions `make gh-docs` as the automated path.
8. Regenerate only the necessary autotools outputs after editing the source inputs so the new target is available in configured builds.

## Validation
- On a clean `master` checkout with `gh-pages` available locally, `make gh-docs` builds, commits, pushes, and finishes back on `master`.
- On any non-`master` branch, `make gh-docs` fails immediately without changing the repo.
- With any tracked or untracked local changes present, `make gh-docs` fails immediately without changing the repo.
- When only `origin/gh-pages` exists, `make gh-docs` succeeds using a temporary local branch and leaves no extra branch state afterward.
- In a disposable clone, force a failure after branch switching or after doc generation and verify cleanup restores the starting branch, restores `gh-pages` to its original SHA, and removes only generated `html/` content created by the run.

## Assumptions
- The branch gate is intentionally strict: only `master` is accepted, not `main` or the current remote default.
- The target includes the final push by default to match the documented end-to-end workflow.
- A clean-tree requirement is preferred over auto-stashing to keep rollback simple and predictable.
- The commit messages remain `"Removed old docs"` and `"New docs"` unless changed during implementation.
