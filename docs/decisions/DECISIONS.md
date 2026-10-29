<!-- CONTRACT: WHY things are the way they are. Never a log of what happened (that's
     git history / plans/) — only calls that could plausibly have gone another way,
     with the reasoning captured so it isn't re-litigated from scratch in six months. -->

# Decisions

Convention: `ADR-###`, sequential, never renumbered. A superseded decision stays in
the file with `Status: Superseded by ADR-0XX` — the reasoning that led somewhere wrong
is often as useful later as the reasoning that led somewhere right.

---

## ADR-EXAMPLE — Use a time-limited link, not a one-time code, for password reset

*(delete this section once you've seen the shape)*

**Date:** YYYY-MM-DD
**Status:** Accepted

**Context:** Password reset needed a mechanism that works from any device, including
one the user isn't currently signed in on.

**Decision:** Emailed link with a signed, expiring token, rather than a numeric code
entered back into the original device.

**Alternatives considered:**
- Numeric code — simpler to implement, but assumes the user is on the same device
  they requested from, which broke for a chunk of support tickets historically.

**Consequences:** Reset flow now depends on transactional email deliverability, which
becomes a new failure mode to monitor (see IC-EXAMPLE if a related constraint exists).

**Related:** FR-EXAMPLE, UC-EXAMPLE

---
