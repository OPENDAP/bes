<!-- This file is auto-loaded by Claude Code at the start of every session in this
     repo. It is a CONTRACT, not a log. If you're an editor here to update it, keep
     it short — it earns its place by being read every session, not by being complete. -->

# CLAUDE.md — `bes`

**What this project is:** This is the Back End Server for Hyrax, an OPeNDAP data server.

1. **OLFS** (front end, Java, separate repo: github.com/opendap/olfs). Implements the DAP2/DAP4 web API and talks to the BES over PPT.
2. **BES framework** (this repo). A Unix daemon (`besdaemon` → `beslistener`) that parses BES XML requests, dispatches them to plugin modules, and streams responses back.
3. **BES modules** (this repo, `modules/`). Data handlers (netCDF, HDF4/5, DMR++, GDAL, FITS, CSV, FreeForm, NcML, ...) and response builders (fileout_netcdf, fileout_json, fileout_covjson, asciival, ...).

Most DAP2/DAP4 protocol code comes from **libdap4** (github.com/opendap/libdap4), an external dependency. Third-party libraries come from **hyrax-dependencies**.

This repo plans work from written requirements, not from conversation memory or
assumption. Requirements, use cases, and constraints live in `docs/`. Plans that
Claude produces live in `plans/`. Every plan traces back to written IDs: a feature
plan cites FR/NFR/UC/IC IDs, a bugfix plan cites a `BUG-###`, a task plan cites a
`TASK-###` (bugfix and task plans may legitimately cite no FR/NFR/UC). If a *feature*
request has no requirement backing it, that's a signal to add one — not to infer one
silently.

## Read before planning anything

| File / folder | Holds |
|---|---|
| `docs/requirements/functional-requirements.md` | What the system must do — `FR-###` |
| `docs/requirements/non-functional-requirements.md` | Quality attributes & targets — `NFR-###` |
| `docs/requirements/use-cases.md` | Actor-driven flows — `UC-###` |
| `docs/constraints/implementation-constraints.md` | Hard boundaries on *how* — `IC-###` |
| `docs/decisions/DECISIONS.md` | Why past calls were made — `ADR-###` |
| `docs/bugs/BUG-LOG.md` | Deviations from intended behavior, report through root cause — `BUG-###` |
| `docs/tasks/TASK-LOG.md` | Scoped maintenance/engineering work not tied to a feature or bug — `TASK-###` |
| `docs/deep-dives/` | Persisted findings from code investigations, one file per question |
| `plans/` | Output: one plan file per feature or bug fix, each citing the IDs above |

Read the requirement and constraint docs **in full** before drafting or revising a
plan — don't rely on what they said earlier in the session, they may have changed.
Also check `docs/decisions/DECISIONS.md` for any accepted ADR the design would
contradict (flag it the way you'd flag an IC tension), and look in `docs/deep-dives/`
for an existing note on the area before investigating it again.

- Ignore `_EXAMPLE_` / `*-EXAMPLE` placeholder entries — they show the format; they
  are not requirements.
- Don't plan against a `Deprecated` entry; follow `Superseded by` to its replacement.
  Call out any `Proposed` entry as not yet confirmed rather than treating it as settled.

## ID conventions

- IDs are `PREFIX-###`, zero-padded to 3 digits, assigned sequentially, **never reused
  or renumbered**. If a requirement is dropped, mark it `Status: Superseded by FR-0XX`
  or `Status: Deprecated` — don't delete the row. History is part of why traceability
  works.
- Prefixes: `FR` functional requirement, `NFR` non-functional requirement, `UC` use
  case, `IC` implementation constraint, `ADR` architecture/design decision, `BUG` a
  logged deviation from intended behavior, `TASK` a scoped maintenance/engineering
  task that is neither new capability nor a correction.
- Every plan phase or major step should cite the ID(s) it satisfies, e.g. `(FR-012,
  UC-003)`. A step with no citation is either scope creep or an uncaptured
  requirement — flag it, don't quietly do it.
- `BUG-###` is a different kind of ID from the rest: it doesn't describe intended
  behavior, it describes a *violation* of it (against an existing FR/NFR/UC where one
  exists, or against an undocumented assumption when it doesn't). Don't force a bug
  into a feature plan's shape — see `plans/_template-bugfix.md`.
- `TASK-###` is a third kind of ID: neither new capability like an FR/NFR/UC nor a
  correction like a BUG — scoped engineering work (dependency bumps, removing dead
  code or stale compile-time directives, retrofitting tests onto code that predates
  them) that doesn't need requirement backing to be legitimate. Don't force it into
  a feature or bugfix plan's shape — see `plans/_template-task.md`.

## The core discipline: don't invent

- If you can't find a requirement backing a feature request, say so and offer to run
  `/new-requirement` rather than writing a plausible-sounding requirement yourself.
  A plan built on an invented requirement looks identical to one built on a real one
  until it's too late to matter.
- If a field is unknown during an interview (priority, target metric, actor), leave
  it as `TBD` rather than guessing. A gap is honest; a plausible guess reads as fact
  later.
- If a proposed plan would violate an implementation constraint (`IC-###`), **say so
  explicitly** and ask how to proceed. Don't silently comply with the constraint by
  redesigning around it without flagging the tension, and don't silently ignore it.

## Filling in templates and logs

When you create a plan or log entry from a template: set `Status` to the single
starting value (`Draft` for plans, `Open` for bug and task log entries) — never copy the `A | B | C`
option list; set `Created` / dates to today's actual date (run `date +%F` if you
don't have it — never guess); and leave genuinely unknown fields `TBD`.

## Custom commands available in this repo

| Command | Does |
|---|---|
| `/init-planning` | First-time setup: interviews you and seeds the requirement docs for an existing or new codebase |
| `/plan-feature <name or description>` | Reads the requirement docs, drafts a phased implementation plan in `plans/`, citing IDs throughout |
| `/fix-bug <description or BUG-###>` | Logs a bug (or resumes one), investigates root cause read-only, drafts a bugfix plan |
| `/deep-dive <area or question>` | Read-only investigation of existing code; checks it against the requirement docs and saves findings |
| `/new-requirement` | Interviews you to add a new FR / NFR / UC / IC entry with the next sequential ID |
| `/plan-task <description or TASK-###>` | Logs a maintenance/engineering task (or resumes one) and drafts a plan for it |
| `/trace <ID>` | Reports everywhere an ID is referenced — docs, plans, code — and flags orphaned requirements |
| `/audit-plan <plan file>` | Checks a plan's ID citations against the requirement docs; reports gaps and coverage, changes nothing |

The `requirements-reviewer` subagent (`.claude/agents/`) reviews the requirement and
constraint docs for vague, untestable, conflicting, or dangling entries. Use it when
asked to review or tighten those docs — it reports findings and never edits.

## Conventions — C++ (Autoconf / Automake / Libtool, Autotest + CppUnit)

**Stack:** C++14 on Linux and macOS, built with Autoconf, Automake, and Libtool, tested
with CppUnit (unit tests) and Autotest (integration and baseline tests), all driven by
`make check`. Compilers/platforms: GCC 9+ and Clang 12+ on Linux; Apple Clang on macOS
(arm64 + x86_64). Bump past C++14 only after checking every target compiler supports it.

Detailed rules live in `.claude/rules/build-autotools.md` (`configure.ac`, `Makefile.am`,
`conf/`, `*.in`) and `.claude/rules/testing.md` (`unit-tests/`, `tests/`, `*.at`,
baselines); they load when you work on matching files.

### Build & test essentials

- `make check` from the top of the build tree runs every test; `make distcheck` must
  pass. A test that `make check` doesn't run doesn't exist.
- Never commit generated files (`configure`, `Makefile.in`, `config.h`, `testsuite`,
  `package.m4`, `atlocal`, ...); regenerate with `autoreconf -fiv`.
- The `-std=` flag and build modes come from configure (`--enable-developer`,
  `--enable-asan`, `--enable-coverage`). Don't hardcode `-std=` or assign `CPPFLAGS` /
  `CXXFLAGS` / `LDFLAGS` in a `Makefile.am`.
- Baselines change only via `./testsuite --baselines=yes`; never hand-edit one.
- New logic isn't done without a test. A bug fix isn't done without a regression test
  that fails before the fix and passes after.
- `clang-format` (`.clang-format` at the repo root) is the formatting source of truth.

### Language & style

- C++14 only: no `std::optional`, `std::variant`, `std::string_view`,
  `std::filesystem`, structured bindings, or `if constexpr`. Needing one is an ADR, not
  a quiet workaround.
- Errors: BES code throws the `BESError` subclasses (`BESInternalError`,
  `BESSyntaxUserError`, `BESNotFoundError`, `BESForbiddenError`, ...; `BESError`
  derives from `std::exception`); libdap code throws `libdap::Error`. Throw by value,
  catch by `const&`. Never let an exception escape a callback invoked from C (libcurl,
  libxml2, HDF5, ...) or a thread. Don't use exceptions for routine not-found or
  end-of-input.
- New code uses RAII and `std::unique_ptr` for owned memory, `std::string` over
  `char*`, and no `strcpy` / `sprintf` / `gets`. Existing code and libdap APIs (e.g.
  `add_var_nocopy`) pass ownership through raw pointers: follow the API's ownership
  contract and don't rewrite surrounding ownership patterns in passing — that's a
  `TASK-###`.
- `<cstdint>` fixed-width types for file and wire formats; never read or write binary
  data by casting a struct or pointer.
- Headers are self-contained and never contain `using namespace`.

### Portability (Linux & macOS)

- Standard C++ and POSIX only. Anything available on just one OS (`<endian.h>`,
  `<malloc.h>`, `strlcpy`, `qsort_r`, `memrchr`, GNU-vs-XSI `strerror_r`, ...) goes
  behind a small wrapper in one file, with `#if defined(__linux__)` /
  `#if defined(__APPLE__)` branches (or an Autoconf `HAVE_FOO` check), each exercised by
  a test.
- Plain `char` signedness differs across platforms; use `signed char`, `unsigned char`,
  or `uint8_t` where it matters.
- No GNU-only compiler extensions (statement expressions, VLAs, bare `__attribute__`).
- macOS filesystems are usually case-insensitive and Linux's aren't: `#include` paths
  and file names must match case exactly.

### Goals, not yet enforced

`-Wextra` in developer builds and an opt-in `--enable-werror`; a committed
`.clang-tidy` that runs clean; a Doxygen comment on every public function in an
installed header. None of these is in place yet — don't enforce them or add the
tooling unless asked.
