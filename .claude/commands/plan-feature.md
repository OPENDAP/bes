---
description: Draft an implementation plan for a feature, citing the FR/NFR/UC/IC IDs it satisfies
argument-hint: <feature name or short description>
---

Draft an implementation plan for: $ARGUMENTS

If `$ARGUMENTS` is empty, ask for a short feature name or description and stop.

## Steps

1. **Read every file in `docs/requirements/` and `docs/constraints/` in full**, right
   now — don't rely on what they said earlier in this session. They may have changed.
   Also read `docs/decisions/DECISIONS.md`: a plan that reverses an accepted ADR must
   say so explicitly. Skip `_EXAMPLE_` / `*-EXAMPLE` placeholder entries.

2. **Identify which existing `FR-###` / `NFR-###` / `UC-###` entries this feature
   is actually backed by.** Don't use `Deprecated` entries; follow `Superseded by` to the
   replacement; flag `Proposed` entries as unconfirmed. If none apply — the feature isn't traceable to anything
   written down — stop and say so plainly. Offer to run `/new-requirement` first.
   Do not invent a plausible-sounding requirement and plan against it; a plan built on
   an invented requirement is indistinguishable from a real one until someone checks.

3. **Identify every `IC-###` that constrains this design space.** Read them, not just
   their IDs — a constraint you skim past is one you'll violate by accident.

4. **Think through the approach before writing anything.** Prefer plan mode for this —
   research the existing code relevant to the feature (read-only) so the plan reflects
   what's actually there, not an assumption about it. If the relevant area is
   unfamiliar or large enough to need real mapping before you can plan against it, run
   `/deep-dive` on it first and come back with findings rather than researching inline
   under time pressure to produce a plan.

5. **Draft the plan using the shape in `plans/_template-plan.md`.** Concretely:
   - Every phase cites the FR/NFR/UC IDs it satisfies.
   - The "Constraints considered" section cites every relevant IC and states how the
     plan respects it.
   - If the plan would violate a constraint, **say so explicitly in the plan** — a
     flagged tension the user can decide on, not a silent workaround and not a silent
     violation.
   - Open questions are genuine unknowns, not gaps papered over with a guess.

6. **Write the plan to `plans/<slug>-plan.md`**, where `<slug>` is the feature name in
   kebab-case. (If you're in plan mode, present the plan and write the file once plan
   mode is exited or approved — don't try to write it from inside plan mode.) If that file already exists, ask before overwriting — don't clobber a
   plan that might be mid-review.

7. **Summarize back to the user:**
   - Which FR/NFR/UC IDs the plan covers.
   - Any relevant requirements you found that the plan does *not* address, and why.
   - Any flagged constraint conflicts.
   - Any open questions that block a phase.

Do not start implementing. This command produces a plan file for review, nothing else.
