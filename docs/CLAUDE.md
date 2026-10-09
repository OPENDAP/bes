# CLAUDE.md for `docs/`

These rules apply to the `docs/` directory. They add to the top-level `CLAUDE.md`, which also applies here.

## Directory content

- Technical documents that describe the organization and function of the BES (for example, `BES_*.md`).
- Work logs and plans written under the top-level "Writing Documents" rules also live here. They are not technical documentation.
- This directory is not the place for end-user documentation, EXCEPT when that information helps explain a technical detail.

## Makefile.am

- When you add, rename, or remove a technical document (a `BES_*.md`-style file), update `EXTRA_DIST` in `docs/Makefile.am` to match.
- Do not add work logs, plans, or other scratch documents to `EXTRA_DIST`.
- Do not edit any other `Makefile.am` because of a docs change.
