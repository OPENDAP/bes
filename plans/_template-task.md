<!-- Shape for task plans written to plans/task-<slug>-plan.md by /plan-task.
     This file itself is never a real plan — copy the shape, don't edit this one.
     Deliberately not the same shape as _template-plan.md: a task isn't new capability
     tied to FR/NFR/UC. Deliberately not the same shape as _template-bugfix.md:
     nothing is necessarily broken. What a task shares with a bugfix plan is that
     scope and completeness matter more than phased delivery. -->

# Task Plan: {{TASK TITLE}}

**Task:** TASK-###
**Status:** Draft | In Review | Approved | In Progress | Done
**Created:** {{DATE}}

## Summary

One or two sentences: what this changes and why.

## Motivation

Why this, why now. A task doesn't need an FR/NFR/UC behind it the way a feature
does — but if there's a related requirement, constraint, or a pattern of bugs that
motivated it, say so.

## Related requirements / constraints

Optional — leave `None` if this task exists independent of the requirement docs
(e.g. retrofitting tests onto code that predates them).

- FR-### / NFR-### / IC-### — how it relates

## Scope

**In scope:** exactly what's included — specific enough that "is this file part of
the task" has an obvious answer.

**Out of scope:** what looks related but isn't included, so it doesn't get folded in
mid-task or assumed covered when it isn't.

## Approach

Steps:
1. ...
2. ...

For anything mechanical (removing every instance of X, updating every dependency in
a set), include how completeness is being checked — a `grep` count before and after,
a list enumerated up front and checked off — not just "done when it looks done."

## Verification

How to confirm the task is actually complete and nothing else broke — full test
suite, build across the platforms this project targets, a diff review of every
touched file. For test-coverage tasks, what coverage looks like before and after.

## Risks

What could go wrong or regress as a side effect of a change that touches many
files or dependencies at once.

## Open questions

Genuine unknowns — not filled with a plausible guess. Each should block the task
until answered, or say why it doesn't.

- ...
