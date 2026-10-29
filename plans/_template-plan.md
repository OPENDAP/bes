<!-- Shape for every plan written to plans/. /plan-feature follows this structure.
     This file itself is never a real plan — copy the shape, don't edit this one. -->

# Plan: {{FEATURE NAME}}

**Status:** Draft | In Review | Approved | In Progress | Done
**Created:** {{DATE}}

## Summary

One or two sentences: what this delivers and why now.

## Requirements traced

Every FR / NFR / UC this plan satisfies, listed explicitly — not just mentioned inline
in the phases below, so `/audit-plan` and a human skimming can see coverage at a
glance.

- FR-###, FR-### — ...
- NFR-### — ...
- UC-### — ...

## Constraints considered

Every IC that bears on this plan, and how the plan respects it (or, if it can't,
an explicit flag rather than a silent workaround).

- IC-### — ...

## Phases

### Phase 1 — {{name}}

**Goal:** ...
**Satisfies:** FR-###, UC-###

Steps:
1. ...
2. ...

**Risks:** ...

### Phase 2 — {{name}}

(repeat as needed — keep phases small enough to review and land independently)

## Open questions

Genuine unknowns — not filled with a plausible guess. Each should block a specific
phase until answered, or say why it doesn't.

- ...

## Out of scope

What this plan deliberately does not cover, so it isn't quietly assumed later.

- ...
