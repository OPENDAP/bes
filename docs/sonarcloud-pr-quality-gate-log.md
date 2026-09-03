# SonarCloud PR quality gate log

## 2026-09-01 (date of change; conversation started 2026-08-28)

Prompt:

> I used SonarScan as part of a CICD process. When the CICD process is run on
> a github PR branch the process tests the sonarscan Quality Gate. However,
> it appears to be testing the result from the master/main branch, not the
> PR's branch. How do I fix that? [...] Since I do this three times in the
> .travis.yml, maybe a fix in a bash script would be best?

Assumptions and environment:

- Repository: `/Users/jhrg/src/opendap/hyrax/bes`
- Files inspected: `.travis.yml` (the three `scan` stage jobs, lines ~253-288
  before this change), `sonar-bes-framework.properties`,
  `sonar-bes-modules-1.properties`, `sonar-bes-hdf-handlers.properties`.
- No build or CI run was performed. `bash -n` was run against the new script
  to check syntax only; nothing was executed against SonarCloud or Travis.

Diagnosis:

- Two separate bugs combined to produce the reported symptom:
  1. `sonar-scanner` was invoked with no `sonar.pullrequest.*` (or
     `sonar.branch.name`) properties in any of the three
     `sonar-bes-*.properties` files or in `.travis.yml`. Travis is not one of
     the CI systems sonar-scanner auto-detects PR context for, so without
     those properties every analysis — PR or not — is recorded against the
     project's main branch in SonarCloud. This is worse than just "reading
     the wrong badge": a PR build was overwriting master's own analysis
     history with the PR's code.
  2. The `curl` calls against
     `api/project_badges/quality_gate?project=<key>` passed no `branch=` or
     `pullRequest=` query parameter, so they always returned the badge for
     the project's default (main) branch.
- The top-level `stage: scan` condition `if: branch = master` is not itself
  the bug: for Travis pull_request builds, `$TRAVIS_BRANCH` is the PR's
  *target* branch, so a PR into master satisfies `branch = master` and the
  scan stage runs on PR builds too, which is the desired behavior once (1)
  and (2) are fixed. The `branches: only:` allowlist near the top of the
  file also does not block pull_request builds — Travis only applies that
  filter to push-triggered builds.

Fix:

- Added `travis/sonar_scan_and_gate.sh`, taking `<sonar-project-key>
  <sonar-properties-file>` as arguments. It:
  - runs the existing configure/build-wrapper/make sequence,
  - checks `$TRAVIS_PULL_REQUEST`; when set (not `"false"`), passes
    `-Dsonar.pullrequest.key`, `-Dsonar.pullrequest.branch`, and
    `-Dsonar.pullrequest.base` (from `$TRAVIS_PULL_REQUEST`,
    `$TRAVIS_PULL_REQUEST_BRANCH`, `$TRAVIS_BRANCH`) to `sonar-scanner`,
  - queries the quality gate badge with a matching `pullRequest=` or
    `branch=` parameter so it reads the same context that was just
    analyzed.
- Replaced the duplicated inline script blocks in the three `scan` stage
  jobs in `.travis.yml` (`scan bes`, `scan bes-modules-1`,
  `scan bes-hdf-handlers`) with calls to this script, keeping each job's
  `export BES_BUILD=...` line and the `opendap-bes-submodules` naming
  comment in place.
- Did not modify the `sonar-bes-*.properties` files, the `stages:`
  conditions, or the `branches:` allowlist — none of those needed to
  change for this fix.

Validation performed:

- `bash -n travis/sonar_scan_and_gate.sh` — syntax check only.
- Not run: an actual Travis build (PR or master), a live sonar-scanner
  invocation, or a call against the SonarCloud API. This should be
  validated by pushing a branch and opening a PR, or by re-running Travis
  on an existing PR, and confirming in the SonarCloud UI that a
  PR-specific analysis (not a main-branch analysis) is created and that
  the `curl` step reports that PR's own quality gate.

## 2026-09-01 (follow-up)

Prompt:

> In travis/sonar_scan_and_gate.sh the SVG badge is used to test for the
> pass/fail status of the scan. Is this equivalent to using the json
> output? [...]

Then, after being asked to choose between switching to the JSON
`api/qualitygates/project_status` endpoint, leaving the SVG badge check as
is, or dropping the redundant check entirely:

> Since the second test (on line 73) is not needed, remove it and add a
> comment that while we used to test the SVG badge text, we dropped that
> because it's not reliable and that the .properties files ensure the scan
> will fail if the scan does not pass the quality gate.

Diagnosis:

- The `project_badges/quality_gate` endpoint returns an SVG meant for
  README display badges; the pass/fail text is rendered into SVG markup
  with no documented, stable contract, and badge responses can be served
  from a cache, so a grep against it right after analysis is not
  guaranteed to reflect the just-computed result. The documented endpoint
  for CI automation is `api/qualitygates/project_status`, which returns a
  JSON `status` field (`OK`/`ERROR`/`NONE`).
- Separately, every `sonar-bes-*.properties` file already sets
  `sonar.qualitygate.wait=true`, so `sonar-scanner` itself blocks on the
  quality gate and exits non-zero on failure. Combined with `set -e` in
  `sonar_scan_and_gate.sh`, the script already fails at the
  `sonar-scanner` line before the curl/grep step is ever reached in the
  failure case — the SVG check was dead code in the one case it was meant
  to catch.

Fix:

- Removed the `curl`/`grep` SVG badge check and the `gate_status`/`exit`
  lines from `travis/sonar_scan_and_gate.sh`, per the user's explicit
  choice not to switch to the JSON endpoint but to drop the redundant
  check outright.
- Left a comment in its place explaining why the SVG check was removed
  and that `sonar.qualitygate.wait=true` already fails the build via
  `sonar-scanner`'s own exit code.
- Updated the file's header comment to describe the current behavior
  (sonar-scanner fails the build on its own) instead of the removed
  "poll the badge" step.
- The script's own exit status is now whatever the last command
  (`sonar-scanner`, via `set -e`, or the final `loggy` call on success)
  leaves it as; no explicit `exit $?` was needed.

Validation performed:

- `bash -n travis/sonar_scan_and_gate.sh` — syntax check only. No Travis
  build or SonarCloud call was run.

## 2026-09-01 (unit test for the PR-vs-branch decision)

Prompt:

> Help me test this script. Is there a way I can write a unit test, or
> something functionally equivalent to a unit test for this script? [...]

Followed by "Do number 1." — extract the PR-vs-branch decision logic into
its own function and unit test that function directly, rather than
end-to-end exercising the whole script (which needs autoreconf, a real
`./configure`, `build-wrapper-linux-x86-64`, `sonar-scanner`, and network
access to SonarCloud).

Environment note: this machine has no `bats`, `shellcheck`, or `shunit2`
installed, and `/bin/bash` here is 3.2.57 (macOS default; no Homebrew bash
present) — no namerefs (`local -n`, which needs bash 4.3+). The test script
was written to run under bash 3.2 and was actually executed locally to
confirm that.

Changes:

- Refactored `travis/sonar_scan_and_gate.sh`:
  - Extracted the `sonar.pullrequest.*` option construction into
    `build_sonar_pr_opts()`, which sets the global array `sonar_pr_opts`
    from `$TRAVIS_PULL_REQUEST`/`$TRAVIS_PULL_REQUEST_BRANCH`/
    `$TRAVIS_BRANCH`.
  - Moved the rest of the script (arg validation, autoreconf/configure/
    build-wrapper/sonar-scanner) into a `main()` function.
  - Added a `[[ "${BASH_SOURCE[0]}" == "${0}" ]]` guard at the bottom so
    `main` only runs when the file is executed directly; sourcing the file
    (as the test script does) makes `build_sonar_pr_opts` and `main`
    available as callable functions without triggering a real build.
  - `main`'s argument-validation failure now uses `return 1` (it's a
    function) instead of `exit 1`; the direct-execution guard captures
    that with `main "$@"; exit $?`, so the script's exit-code behavior
    when run as `./sonar_scan_and_gate.sh` is unchanged.
- Added `travis/sonar_scan_and_gate_test.sh`, which sources
  `sonar_scan_and_gate.sh` and checks, with no real build or network
  access:
  1. a plain branch build (`TRAVIS_PULL_REQUEST=false`) produces an empty
     `sonar_pr_opts`;
  2. a PR build produces the three `sonar.pullrequest.key/branch/base`
     options, built from the PR number, source branch, and target branch,
     in order — this is the exact behavior that was missing before this
     script existed;
  3. a PR build targeting a non-master branch uses that branch (not a
     hardcoded `"master"`) as `sonar.pullrequest.base`;
  4. `main` run with zero or one arguments returns 1 without attempting
     any real work.
  - Written to run under bash 3.2: no namerefs, a single array-comparison
    helper (`assert_sonar_pr_opts_eq`) hardcoded to the one array under
    test rather than a generic by-name array-passing helper.

Validation performed:

- `bash -n` on both `travis/sonar_scan_and_gate.sh` and
  `travis/sonar_scan_and_gate_test.sh`.
- Actually ran `bash travis/sonar_scan_and_gate_test.sh` locally (bash
  3.2.57) — all 5 assertions pass, exit code 0.
- Actually ran `./travis/sonar_scan_and_gate.sh` directly with no
  arguments to confirm the direct-execution guard still logs the usage
  error and exits 1 (rather than only being reachable via `main` when
  sourced).
- Not run: an actual Travis build, a live `sonar-scanner` invocation, or
  a call against the SonarCloud API.

## 2026-09-02 (deliberate vulnerability to smoke-test the PR gate)

Prompt:

> Is there a way I can introduce a change to the current branch of this
> (bes) repo so that it will fail the sonarscan quality gate? I'm thinking
> of a clear security issue. Clearly, I will need this to be simple and
> easy to remove. It would be best if this was an isolated program.

Goal: exercise the fix above end-to-end — confirm that a PR build's
SonarCloud quality gate reflects the PR's own code (not master's) by
deliberately introducing new code with an obvious vulnerability that
should fail the gate, then removing it once confirmed.

Changes:

- Added `dispatch/sonar_gate_smoke_test.cc`: a small, standalone `main()`
  with an unbounded `strcpy()` into a fixed-size stack buffer (CWE-120 /
  CWE-787). Chosen over alternatives (e.g. `system()` command injection,
  a hardcoded credential) because buffer overflow is one of SonarSource's
  advertised C/C++ `VULNERABILITY`-type detections (not a Security
  Hotspot needing manual review), so it should reliably drop the Security
  Rating on new code below `A` and fail the default "Sonar way" gate. It
  depends on nothing from BES (`<cstring>` only), so it can't affect any
  real library or its symbols.
- Added a `noinst_PROGRAMS` stanza to `dispatch/Makefile.am` building it
  as its own program (not folded into `libbes_dispatch_la_SOURCES`), so
  it's isolated from every real target. `dispatch/` was chosen because
  it's one of the directories `sonar-bes-framework.properties` scans
  (`sonar.sources=dap,dapreader,dispatch,http,ppt,server,xmlcommand`),
  the project whose "scan bes" job this whole change has been about.
  `noinst_PROGRAMS` still builds under plain `make` (just isn't
  installed), so `build-wrapper-linux-x86-64` will capture its
  compilation for `sonar-scanner` to analyze.
- The file and the Makefile.am stanza are both marked TEMPORARY in
  comments, to be deleted before merging.

Validation performed:

- `make -C dispatch sonar_gate_smoke_test` in the existing local dev
  build tree (`/Users/jhrg/src/opendap/hyrax/bes`, configured in-tree,
  `prefix=../build`) — automake's maintainer-mode rules regenerated
  `dispatch/Makefile` from the edited `Makefile.am` automatically and the
  program compiled and linked cleanly. This only confirms it builds
  locally; it does not confirm SonarCloud actually classifies this as a
  gate-failing vulnerability, which can only be confirmed by pushing a PR
  and watching the "scan bes" Travis job / SonarCloud UI.
- Removed the resulting local build artifacts (`dispatch/sonar_gate_smoke_test`
  binary, its `.o`, and the `.deps` entry) after verifying — they were
  untracked and not covered by `.gitignore` (which only ignores `*.o`,
  not extensionless program binaries), so left in place they could have
  been swept up by a broad `git add`.
- Not run: an actual Travis build, `sonar-scanner`, or the SonarCloud
  API — this still needs to be verified on a real PR.

To remove once confirmed: delete `dispatch/sonar_gate_smoke_test.cc` and
the `noinst_PROGRAMS`/`sonar_gate_smoke_test_SOURCES` lines added to
`dispatch/Makefile.am`.
