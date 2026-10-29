---
description: Check a plan's requirement citations against the requirement docs — reports gaps, changes nothing
argument-hint: <path to plan file, e.g. plans/password-reset-plan.md>
---

Audit the plan at: $ARGUMENTS

If `$ARGUMENTS` is empty, list the files in `plans/` (excluding `_template-plan.md`,
`_template-bugfix.md`, and `_template-task.md`) and ask which one. If there's exactly
one candidate, confirm before proceeding rather than assuming.

## Steps

1. **Read the plan file in full.** Note whether it's a feature plan, a bugfix plan
   (`plans/bugfix-*`), or a task plan (`plans/task-*`) — a bugfix plan is expected to
   cite a `BUG-###` and, unlike a feature plan, may legitimately cite zero FR/NFR/UC
   if the bug reveals behavior that was never documented in the first place; flag
   that as a gap to close via `/new-requirement`, not as a citation error. A task
   plan is expected to cite a `TASK-###` and, like a bugfix plan, may legitimately
   cite zero FR/NFR/UC — most maintenance work (dependency bumps, retrofitting
   tests) doesn't need requirement backing to be legitimate.

2. **Extract every `FR-###` / `NFR-###` / `UC-###` / `IC-###` / `ADR-###` / `BUG-###` /
   `TASK-###` cited anywhere in it.**

3. **Verify each cited ID actually exists** in the current `docs/requirements/`,
   `docs/constraints/`, `docs/decisions/DECISIONS.md`, `docs/bugs/BUG-LOG.md`, and
   `docs/tasks/TASK-LOG.md` files. Flag any that don't — a typo, a renumbered ID, or an
   ID that was invented rather than looked up. Also flag a cited entry whose Status is
   `Deprecated` or `Superseded` (the plan rests on something no longer current), and
   any ID that exists only as an `_EXAMPLE_` placeholder.

4. **Flag uncited work.** Look at each phase's steps: if a phase does something with
   no ID citation anywhere near it, flag it — either it's scope creep that crept in
   without a requirement behind it, or it's legitimate work that surfaced a gap in
   the requirement docs and should get one via `/new-requirement`.

5. **Report coverage**, not just problems: which requirements relevant to this
   feature area are cited, and — by grepping the requirement docs for the plan's feature area — which
   related requirements exist but aren't mentioned in this plan at all. That's not
   automatically wrong (not every related requirement belongs in every plan) but it's
   worth surfacing. For feature plans, also check that every ID cited inline in a phase
   appears in the **Requirements traced** / **Constraints considered** sections, and
   vice versa.

6. **Change nothing.** This command produces a report. Whether to fix what it finds
   is the user's call, made in a separate step.
