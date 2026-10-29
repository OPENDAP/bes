---
description: First-time setup — interview the user and seed the requirement docs for this project
---

Set up this project's requirement docs for the first time. This is for a project that
either has no real content in `docs/requirements/` yet, or is having this template
folded into an existing codebase.

## Steps

1. **Look around first.** If there's existing code, read the top-level structure and
   any existing README/docs to form a rough picture — but treat it as *context for
   your questions*, not as a source of requirements to transcribe. Code shows what was
   built; it doesn't reliably say what was intended or why, and "the code does X" is
   not the same claim as "FR-001: the system must do X."

2. **Interview before writing anything**, in one message where possible, using
   structured questions where the answer is a choice:
   - What is this project, in a sentence or two? (→ `{{ONE_LINE_DESCRIPTION}}` in
     `CLAUDE.md`)
   - What are the 3-8 most important things it must do right now? (→ seed `FR-###`
     entries — don't try to exhaustively capture everything in one pass)
   - Any hard non-functional targets already known — performance, security,
     compliance? (→ seed `NFR-###` entries; skip if genuinely not decided yet)
   - Any hard constraints — things that must never be done, or technology/regulatory
     boundaries already fixed? (→ seed `IC-###` entries)
   - Language/stack, test command, branching convention — whatever a fresh Claude
     session would otherwise have to rediscover. (→ `{{CONVENTIONS}}` in `CLAUDE.md`)

   **Do not guess these from the code.** A codebase can suggest candidates worth
   asking about, but the requirement doc should reflect what the user confirms, not
   what looks plausible. Leave a field `TBD` rather than filling it with an inference.

3. **Fill in `CLAUDE.md`**: `{{PROJECT_NAME}}`, `{{ONE_LINE_DESCRIPTION}}`,
   `{{CONVENTIONS}}`.

4. **Seed the requirement docs** with what the interview produced, following the
   existing table/section formats. Delete the `_EXAMPLE_` rows/sections in each file
   as you go — leave them only in files where nothing real was added yet, so the
   shape is still visible.

5. **Do not write `plans/` or `docs/decisions/DECISIONS.md` content** — those fill in
   as work actually happens, not from a first interview.

6. **Report back** what was written and, just as importantly, what's still `TBD` or
   unasked — so the user can see the gaps rather than discover them later.
