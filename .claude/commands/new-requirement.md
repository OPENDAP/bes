---
description: Interview the user to add a new FR / NFR / UC / IC entry with the next sequential ID
argument-hint: (optional) FR | NFR | UC | IC
---

Add a new requirement, use case, or constraint entry.

## Steps

1. **Determine the type.** If `$ARGUMENTS` names one of `FR`, `NFR`, `UC`, `IC`
   unambiguously, use it. Otherwise ask which of the four this is, in one message.

2. **Compute the next ID.** Read the relevant doc (`docs/requirements/functional-
   requirements.md`, `non-functional-requirements.md`, `use-cases.md`, or
   `docs/constraints/implementation-constraints.md`) and find the highest existing
   number for that prefix. The new ID is one higher, zero-padded to 3 digits (ignore `_EXAMPLE_`
   placeholders; with no real entries yet, the first ID is 001). Never
   reuse a number that was deprecated or superseded.

3. **Check for an existing entry that already covers this** (if so, say so and ask
   whether to extend it or add a new one), then **interview for the required fields —
   do not guess any of them:**
   - `FR`: the requirement statement (single testable sentence), priority
     (Must/Should/Could), related use case IDs if any known yet, status (suggest
     `Proposed`; don't mark `Approved` unless the user says so).
   - `NFR`: category, the requirement, a measurable target, priority, status (as for `FR`).
   - `UC`: title, actor(s), trigger, preconditions, main flow steps, alternate/
     exception flows, postconditions, related FR/NFR IDs.
   - `IC`: category, the constraint itself, rationale, impact on design.

   If the user doesn't know a field yet (no target metric decided, priority not set),
   write `TBD` in that field rather than filling it with something plausible. TBD is
   honest and greppable; a guess reads as settled fact to the next session.

4. **Show the drafted entry before writing it**, formatted to match the existing rows/
   sections exactly (same column order, same heading structure for use cases).

5. **On confirmation, append it** to the correct file, preserving the existing table
   or section format. Do not touch other entries (one exception: if you introduced a new category, add
   it to the category list at the top of that doc, which the docs say to keep in sync).

6. **Report the new ID back**, and mention `/trace <ID>` and `/plan-feature` as the
   next useful commands now that it exists.
