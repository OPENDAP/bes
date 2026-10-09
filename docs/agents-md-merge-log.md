# AGENTS.md merge log

10/8/26 14:57:24 Compare the two AGENTS files and combine them asking me for confirmation of anything that might lead to unmonitored code changes.

- Compared `AGENTS.md` (repo-wide) with `docs/AGENTS.md` (scoped to `docs/`). Both are gitignored (`**/AGENTS.md`), so I backed them up to the session scratchpad before editing.
- Most of `docs/AGENTS.md` repeated the root file (Change Discipline, Review Priorities 1-4, the "state assumptions" line).
- Rules unique to `docs/AGENTS.md`: the directory content, "no end-user docs", "professional engineering tone", and "Make sure the Makefile.am is up to date after any change".
- The Makefile.am rule could cause agents to edit build files without being asked, so I asked about it. Decision: it applies only to technical docs in `docs/` and only to `EXTRA_DIST` in `docs/Makefile.am`. Logs and plans are excluded.
- Asked where the merged result should go. Decision: the root file keeps all repo-wide rules, and `docs/AGENTS.md` is cut down to the docs-only rules plus a pointer to the root file. Nothing was deleted.
- Root `AGENTS.md`: added the tone line and a pointer to `docs/AGENTS.md`.
- `docs/AGENTS.md`: rewritten to cover scope, directory content (including work logs and plans), and the scoped Makefile.am rule.
- Not changed: `CLAUDE.md`.

10/8/26 15:02:36 Yes. Also, I have updated the CLAUDE.md file.

- (Context: the user asked whether Claude reads AGENTS.md. It does not load it automatically. I recommended keeping CLAUDE.md standalone and adding a tracked docs/CLAUDE.md.)
- Created `docs/CLAUDE.md` with the docs-only rules (directory content, the scoped Makefile.am rule). I wrote them out directly rather than using an `@AGENTS.md` import, because AGENTS.md is gitignored.
- Staged `docs/CLAUDE.md` with `git add`. Not committed.
- Reviewed the user's updated top-level `CLAUDE.md`. The "Logging rules" section is gone and a "Writing Documents" section was added. Nothing in it conflicts with `docs/CLAUDE.md`.
