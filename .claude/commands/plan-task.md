---
description: Log a maintenance/engineering task (or resume an existing one) and draft a plan for it — for scoped mechanical work that isn't new capability or a bug fix
argument-hint: <description of the task, or an existing TASK-### ID>
---

Handle a task: $ARGUMENTS

## Steps

1. **Resolve new vs. existing.** If `$ARGUMENTS` is a `TASK-###` ID already present
   in `docs/tasks/TASK-LOG.md`, read that entry and resume from wherever it left off.
   If `$ARGUMENTS` is empty, ask for a short description. Otherwise treat it as a new
   task.

2. **For a new task, interview for a concrete scope — don't accept a vague goal as
   a task entry:**
   - Title (short)
   - Category: Dependency Update / Code Cleanup / Test Coverage / Build & Tooling /
     Documentation / Other — ask, don't guess
   - **In scope:** exactly what's included, specific enough that "does this file
     count" has an obvious answer
   - **Out of scope:** what looks related but isn't included this time
   - Motivation: why this, why now

   A task like "remove all the old compile-time directives" or "update the
   dependencies" fails quietly when scope is fuzzy — some instances get missed, or
   the task creeps into a redesign nobody signed up for. Pin the boundary down
   before assigning an ID.

3. **Search `docs/requirements/` and `docs/constraints/` for anything related.** A
   task doesn't need requirement backing the way a feature does — retrofitting tests
   onto legacy code, or bumping a dependency, usually predates any written
   requirement — so finding nothing is a normal outcome, not a blocker. If something
   *is* related, note it rather than asserting a connection that isn't really there.

4. **Assign the next `TASK-###` ID** (read `docs/tasks/TASK-LOG.md`, find the
   highest existing number, increment) and append a new section following the
   existing format, `Status: Open`.

5. **Draft the plan** using the shape in `plans/_template-task.md`: scope (in/out),
   approach, and — for anything mechanical — how completeness gets checked (a
   `grep` count before and after, an enumerated list checked off) rather than "done
   when it looks done." Write it to `plans/task-<slug>-plan.md`. Confirm before
   overwriting an existing file.

6. **Update the task's status to `Plan Ready`** in `docs/tasks/TASK-LOG.md` and link
   the plan file. Leave `Done` for the user to set once the work actually lands and
   is verified — not something to self-report from having written a plan.

7. **Report back:** the `TASK-###` ID, scope (in/out), any related requirements or
   constraints found, and where the plan was written.

Do not start implementing. This command produces a logged task and a plan for
review.
