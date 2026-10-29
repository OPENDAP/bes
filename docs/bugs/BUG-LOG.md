<!-- CONTRACT: deviations from intended behavior, tracked from report through root
     cause to fix. Not a duplicate of docs/requirements/ — a bug means something
     already documented (or implicitly assumed) isn't holding true in practice.
     Delete the BUG-EXAMPLE section once you've seen the shape. -->

# Bug Log

Convention: `BUG-###`, sequential, never renumbered. Status: `Open` → `Investigating`
→ `Fix Planned` → `Fixed` → (or `Won't Fix` / `Duplicate of BUG-0XX`). `Open` means
logged with reproduction steps but root cause not yet looked at (`/fix-bug` sets it
at step 4 and moves to `Investigating` at step 5; an entry logged by hand starts here
too). **Only the user sets a bug to `Fixed`** — that's a claim about verified
deployed behavior, not something to self-report from having written a fix plan.

---

## BUG-EXAMPLE — Password reset link accepted after expiry

*(delete this section once you've seen the shape)*

**Severity:** High
**Status:** Fix Planned
**Reported:** YYYY-MM-DD
**Related requirements:** FR-EXAMPLE, UC-EXAMPLE (violates the "time-limited" part of both)
**Fix plan:** `plans/bugfix-reset-link-expiry-plan.md`

**Reproduction steps:**
1. Request a password reset
2. Wait past the stated expiry window (currently 1 hour)
3. Follow the link anyway

**Expected behavior:** Link is rejected with an "expired, request a new one" message
(per FR-EXAMPLE).

**Actual behavior:** Link still succeeds and resets the password.

**Root cause:** Expiry is checked against token creation time using the token's own
embedded claim, but that claim is client-suppliable and never verified against issue
time recorded server-side. *(Fill this in during investigation — leave `TBD` until
actually found in the code, not guessed from symptoms.)*

---

<!-- Add new bugs via /fix-bug, or by hand — keep the section format: metadata lines,
     then Reproduction / Expected / Actual / Root Cause. A bug with no reproduction
     steps yet is a symptom report, not a bug entry — get concrete steps before
     assigning an ID if at all possible. -->
