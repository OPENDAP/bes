---
description: Log a bug (or resume an existing one), investigate root cause read-only, and draft a bugfix plan
argument-hint: <description of the bug, or an existing BUG-### ID>
---

Handle a bug: $ARGUMENTS

## Steps

1. **Resolve new vs. existing.** If `$ARGUMENTS` is a `BUG-###` ID already present in
   `docs/bugs/BUG-LOG.md`, read that entry and resume from wherever it left off (skip
   to step 6 if root cause is already filled in, otherwise step 5; never repeat step 4 —
   the bug already has an ID). If `$ARGUMENTS` is
   empty, ask for a short description. Otherwise treat it as a new bug report.

2. **For a new bug, interview for concrete reproduction — don't accept a vague
   symptom as a bug entry:**
   - Title (short)
   - Severity: Critical / High / Medium / Low — ask, don't infer from tone
   - Reproduction steps: concrete, numbered, specific enough that someone else could
     follow them
   - Expected behavior vs. actual behavior

   If reproduction steps aren't pinned down yet, say so and help narrow them down
   before assigning an ID — a symptom report without steps to reproduce isn't
   actionable yet.

3. **Search `docs/requirements/` for a related FR/NFR/UC.** If you find one the bug
   plausibly violates, propose it and confirm rather than asserting it silently — the
   match might be wrong, or there may be no documented requirement at all (which is
   worth naming explicitly: the "correct" behavior was never written down).

4. **Assign the next `BUG-###` ID** (read `docs/bugs/BUG-LOG.md`, find the highest
   existing number, increment) and append a new section following the existing
   format, `Status: Open` (logged with concrete reproduction steps; root cause not yet
   looked at).

5. **Investigate root cause, read-only.** First set the entry's status to
   `Investigating`. Trace the actual code path involved. For
   anything beyond a narrow, localized bug — where understanding it means mapping a
   whole subsystem — use `/deep-dive` first and come back with findings rather than
   trying to hold a large investigation inline here. Fill in the `Root cause` field
   with what was actually found in the code. If it isn't conclusive, say what's been
   ruled out and what's still open — don't present a guess as the answer.

6. **Draft the fix plan** using the shape in `plans/_template-bugfix.md`: root cause,
   related requirements, fix approach (and why that scope, not narrower or broader),
   regression risk, and verification steps built from the reproduction steps in
   `docs/bugs/BUG-LOG.md`. Write it to `plans/bugfix-<slug>-plan.md`. Confirm before
   overwriting an existing file.

7. **Update the bug's status to `Fix Planned`** in `docs/bugs/BUG-LOG.md` and link the
   plan file. **Never set `Status: Fixed` yourself** — that's a claim about verified
   deployed behavior; leave it for the user to confirm once the fix actually lands.

8. **Report back:** the `BUG-###` ID, the related requirement(s) if any, root cause
   found (or what's still unknown), and where the fix plan was written.

Do not start implementing the fix. This command produces a logged bug and a plan for
review.
