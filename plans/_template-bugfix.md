<!-- Shape for bugfix plans written to plans/bugfix-<slug>-plan.md by /fix-bug.
     This file itself is never a real plan — copy the shape, don't edit this one.
     Deliberately not the same shape as _template-plan.md: a bug fix is corrective,
     not incremental delivery, so it doesn't need multiple phases in most cases. -->

# Bugfix Plan: {{BUG TITLE}}

**Bug:** BUG-###
**Status:** Draft | In Review | Approved | In Progress | Done
**Created:** {{DATE}}

## Summary

One or two sentences: what's broken and what this fix changes.

## Root cause

What's actually wrong, as established by reading the code — not inferred from
symptoms alone. If the investigation didn't conclusively find it, say what's been
ruled out and what's still open, rather than presenting a best guess as settled.

## Related requirements

What this bug violates, if anything is documented — or note explicitly if the
correct behavior was never written down anywhere, which is itself worth fixing via
`/new-requirement` alongside the code fix.

- FR-### / NFR-### / UC-### — how it's violated

## Fix approach

What changes, and why this is the right level of fix — a narrow patch, or a fix that
also addresses the underlying design gap that let the bug happen. If narrow, say why
the broader fix isn't happening now (out of scope, needs its own plan, etc.) rather
than leaving that judgment call implicit.

## Regression risk

What else touches the code being changed, and what could break. What existing tests
(if any) cover this path already.

## Verification

How to confirm the fix actually works — ideally the original reproduction steps from
`docs/bugs/BUG-LOG.md`, re-run and expected to now pass, plus a note on what test (new
or existing) should guard against regression.

## Out of scope

What this fix deliberately does not address (e.g. a related but distinct bug, or a
deeper redesign) — so it isn't assumed to be covered.
