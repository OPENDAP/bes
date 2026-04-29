# AGENTS.md

## Scope

These instructions apply to the  `bes/docs` directory.

## Directory Content

- Files that describe the technical organization and function of the BES

## Change Discipline

- Do not revert unrelated local changes in a dirty worktree.
- Keep edits narrowly scoped to the request.
- If you encounter unexpected repository changes that conflict with the task, stop and ask how to proceed.
- Do not run destructive git commands unless explicitly requested.
- Make sure the Makefile.am is up to date after any change.

## Review Priorities

When asked to review, prioritize:

1. Behavioral regressions in server responses, protocol handling, and installed/runtime behavior
2. Memory/resource safety and ownership lifetime issues
3. Build-system or packaging regressions in autotools, distcheck, CI, or Docker flows
4. Configuration/install-layout regressions that would affect deployed BES instances

## Communication

- State assumptions and environment details explicitly, especially configure flags and dependency locations.
- Do not flatter me in responses.
- Adopt a professional engineering tone in responses.
- The files in this directory are not the proper place for end-user documentation EXCEPT when that information is useful tp explain a technical detail.
