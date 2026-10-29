---
description: Report every place an FR/NFR/UC/IC/ADR/BUG/TASK ID is referenced, and flag it if nothing references it yet
argument-hint: <ID, e.g. FR-014>
---

Trace references to: $ARGUMENTS

If `$ARGUMENTS` is empty or doesn't look like a `PREFIX-###` ID, ask for one.

## Steps

1. **Find the definition.** Locate the ID's own entry in `docs/requirements/`,
   `docs/constraints/`, `docs/decisions/DECISIONS.md`, `docs/bugs/BUG-LOG.md` (for a
   `BUG-###`), or `docs/tasks/TASK-LOG.md` (for a `TASK-###`) and quote it briefly.

2. **Search the whole repo for other references** — `docs/`, `plans/`, and any source
   code or comments — for the literal ID string. Use `grep -rnw --exclude-dir=.git` (`-w` so `FR-001` can't match inside `FR-0010`)
   rather than guessing
   from memory.

3. **Report, grouped by kind:**
   - Which use case(s) reference it (for an FR/NFR).
   - Which FR/NFR it references (for a UC).
   - Which plan(s) in `plans/` cite it (feature plans or bugfix plans), and where.
   - Which bug(s) in `docs/bugs/BUG-LOG.md` cite it, if it's an FR/NFR/UC being
     violated.
   - Which task(s) in `docs/tasks/TASK-LOG.md` cite it, if it's an FR/NFR/UC/IC that
     informed a maintenance task.
   - Any code/comment references found.

4. **Flag orphans plainly.** If an FR/NFR/UC has zero references outside its own
   definition — no use case links it, no plan implements it — say so explicitly:
   that's either a requirement nobody has picked up yet, or one that no longer
   matters and should be reconsidered. For a `BUG-###` with no fix plan, say so too —
   it's logged but not yet worked. Same for a `TASK-###` with no plan.

5. **Change nothing.** This command reports; it doesn't edit docs, plans, or code.
