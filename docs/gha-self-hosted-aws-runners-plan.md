# Plan: Move `build-osx.yml` from GitHub-Hosted to Self-Hosted (AWS) Runners

Status: **planning only — no code or configuration changed.**

## 0. Scope (confirmed with user, 2026-09-17)

- In scope: `.github/workflows/build-osx.yml`, currently running on GitHub-hosted
  `macos-26` runners.
- Out of scope: `.travis.yml` and everything it drives (Linux build/check,
  distcheck, srcdist, Rocky 8/9 Docker image builds, SonarCloud scans, OLFS
  trigger/deploy). Travis continues to own that pipeline unchanged.
- Runner compute model (persistent vs. ephemeral) is left open below; the plan
  weighs both and recommends one.

## 1. Current state

`build-osx.yml` ([.github/workflows/build-osx.yml](../.github/workflows/build-osx.yml)):

- Triggers on push to `master`/`main`/`*-test-deploy` and on PRs.
- `runs-on: ${{ matrix.runs-on }}` with a one-entry matrix, `macos-26` — a
  GitHub-hosted macOS runner.
- Installs system deps via Homebrew, pulls prebuilt `hyrax-dependencies` from
  `s3://opendap.github.actions.build/`, builds `libdap4` from source, then
  configures/builds/tests bes with autotools, and on failure uploads test logs
  to `s3://opendap.github.actions.test/` via
  [.github/scripts/upload-test-results.sh](../.github/scripts/upload-test-results.sh).
- AWS access is via two long-lived repo secrets, `AWS_GHA_BES_ID` /
  `AWS_GHA_BES_SECRET_KEY`.
- The repo has no existing Terraform/Packer/Ansible/CDK — any AWS-side runner
  infrastructure to support this is **greenfield**.

## 2. The requirement that dominates this plan: macOS on AWS is not ordinary EC2

This is the single most important constraint, so it goes first, ahead of the
generic self-hosted-runner considerations in §3.

AWS's only way to run macOS compute is **EC2 Mac Dedicated Hosts**
(`mac1.metal` Intel, `mac2.metal` M1, `mac2-m2.metal`, `mac2-m2pro.metal`).
This is fundamentally different from Linux EC2:

- **24-hour minimum host allocation.** This is an Apple macOS EULA/licensing
  term enforced by AWS, not a pricing tier choice: once you allocate a Mac
  Dedicated Host you cannot release it for 24 hours, or you're billed as if
  you had. This alone rules out the "spin up a fresh runner per job, tear it
  down when done" pattern that's standard practice for self-hosted Linux
  runners — per-job ephemeral hosts would mean paying for a full day of a
  Mac Dedicated Host for a build that takes ~20–40 minutes.
- **Bare metal only, one host = one instance.** A Mac Dedicated Host runs
  exactly one Mac instance at a time; you cannot bin-pack multiple concurrent
  jobs onto one host the way you can with Linux EC2 instance types.
- **Region/AZ capacity is more limited** than general-purpose EC2, and Mac
  instances are only available via Dedicated Hosts, not standard
  on-demand/spot instance launches.
- **AMI lifecycle is manual/custom.** There is no equivalent of
  `actions/runner-images`'s macOS image for AWS; a custom AMI (Homebrew,
  Xcode Command Line Tools, the packages `build-osx.yml` currently installs
  via `brew install bison ccache jpeg cppunit autoconf automake libgeotiff
  libtool groff openssl`, the AWS CLI, and the GitHub Actions runner agent)
  must be built and maintained by us.
- **Path differences.** GitHub-hosted `macos-26` runners are Apple Silicon
  with Homebrew under `/opt/homebrew`; a self-built AMI needs to match that
  layout (or the workflow's `LDFLAGS`/`CPPFLAGS`/`PATH` assumptions in
  `build-osx.yml:44-45,95-97` need updating) — this is a real compatibility
  risk item, not just an infra detail.

Net effect: the "ephemeral autoscaled runner" pattern that makes self-hosted
Linux/Windows runners both cost-effective and secure is **not directly
available** for this macOS workflow. The plan below treats that as a first-class
constraint rather than an afterthought.

## 3. Requirements

### 3.1 Functional
- Builds must run on Apple Silicon macOS (parity with developer machines —
  this is the documented reason the workflow exists, see
  [.github/workflows/build-osx.yml:1-2](../.github/workflows/build-osx.yml)).
- Must produce the same pass/fail signal on PRs and pushes as today (status
  checks, branch protection integration).
- Must retain the existing S3 integration points (`hyrax-dependencies`
  fetch, failure-log upload) or deliberately replace them.

### 3.2 Security (self-hosted runners are a different threat model than
GitHub-hosted)
- **Fork PRs must not be able to run on self-hosted runners.** GitHub's own
  guidance: a self-hosted runner on a repo that accepts PRs from forks lets
  an attacker submit a PR that executes arbitrary code on your infrastructure.
  `build-osx.yml` currently triggers on `pull_request` with no fork
  restriction — this must be tightened (e.g., `pull_request_target` with
  path/label gating, requiring maintainer approval to run workflows for
  first-time/external contributors, or restricting the self-hosted job to
  `push` and same-repo PRs only) before switching off GitHub-hosted runners.
- **Credential model.** Prefer an IAM role attached to the EC2 Mac instance
  (instance profile) over the current static `AWS_GHA_BES_ID`/`AWS_GHA_BES_SECRET_KEY`
  repo secrets. Since the runner now *is* AWS infrastructure, this removes a
  class of long-lived credentials entirely. Scope the role narrowly: read on
  `s3://opendap.github.actions.build/*`, write on
  `s3://opendap.github.actions.test/*`, nothing else.
- **Runner isolation between jobs.** Given the 24-hour host constraint, a
  literally-ephemeral (one job, then destroyed) runner is off the table for
  cost reasons (§2). The mitigation is process-level, not host-level: run the
  GitHub Actions runner in `--ephemeral` mode (deregisters after one job) with
  a `launchd`-managed supervisor that immediately re-registers a fresh runner
  process, and wipe the work directory between jobs. This doesn't isolate at
  the VM level like disposable cloud runners do, but bounds blast radius to
  "one job's process tree," not "long-lived shared state across jobs."
- **Network egress control.** Restrict outbound to what's needed: GitHub
  (Actions service, api.github.com), the two `opendap.github.actions.*` S3
  buckets, Homebrew's CDN, and the opendap/libdap4 GitHub repo. A security
  group plus (optionally) an egress proxy/allowlist is more defensible than
  open outbound.
- **Physical/tenancy note specific to Mac instances:** AWS enforces host
  isolation per customer for Mac Dedicated Hosts (a host is never
  reused for a different customer without an Apple-mandated wipe/cooldown),
  but that's an AWS-side control, not something we configure.

### 3.3 Cost
- Do not assume numbers we don't have. Before committing to an architecture,
  pull actual GitHub Actions usage data for this workflow (Settings → Billing
  → Actions, or the API) to get real monthly minutes consumed on `macos-26`
  runners (GitHub bills macOS minutes at a 10x multiplier vs. Linux — see
  GitHub's runner pricing page for the current multiplier and per-minute
  rate) and compare against:
  - EC2 Mac Dedicated Host on-demand hourly rate (check current AWS Mac
    instance pricing for the target family — `mac2.metal` for Apple Silicon
    parity) × (24h minimum allocation × number of allocation cycles/month
    under the reservation strategy chosen in §4), or
  - A Dedicated Host Reservation (1-year/3-year) if usage is continuous
    enough to justify it — AWS offers reservation discounts on Mac Dedicated
    Hosts, but reservations require committing to always-on capacity.
- Cost only wins if utilization is high enough to absorb the 24-hour minimum,
  or if we accept a persistent host and compare it to a monthly reservation
  cost, not a per-job cost. This needs real numbers before a go/no-go
  decision, not an estimate here.

### 3.4 Operational / maintenance
- AMI build-and-patch pipeline (Xcode CLT updates, security patches, Homebrew
  package version drift) — someone owns this on a recurring basis; there is
  no GitHub-maintained image to fall back on.
- Health monitoring: unlike GitHub-hosted runners, a dead or hung self-hosted
  runner fails silently until a job queues against it. Needs CloudWatch
  alarms/health checks and paging, or PRs simply hang.
- Runner registration token lifecycle: self-hosted runner registration
  tokens expire quickly; automate registration via the GitHub App/PAT +
  `actions/runner` API flow, not a manually pasted token.
- Capacity: with one Mac Dedicated Host, concurrent PR builds queue instead
  of running in parallel (GitHub-hosted matrix jobs run in parallel across
  ephemeral runners by default). If PR concurrency matters, that means
  multiple Dedicated Hosts, multiplying the 24-hour-minimum cost.

### 3.5 Compatibility / correctness
- Homebrew prefix, Xcode CLT version, and preinstalled tool versions on a
  custom AMI will drift from GitHub's `macos-26` image over time; the
  `hyrax-dependencies-${matrix.runs-on}.tgz` S3 artifact naming
  (`build-osx.yml:84`) is keyed to the GitHub-hosted label and will need a
  parallel artifact (or the same one, if binary-compatible) for the
  self-hosted image.
- `ccache` cache and `libdap` install cache currently use
  `actions/cache@v4` (`build-osx.yml:62-68,103-112`), which works the same
  on self-hosted runners as long as the runner has network access to the
  Actions cache service — no change needed there, but worth calling out as a
  dependency on GitHub connectivity even after self-hosting the compute.

## 4. Architecture options

### Option A — Single persistent EC2 Mac Dedicated Host, long-running runner
Allocate one `mac2.metal` Dedicated Host, run one GitHub Actions
self-hosted runner as a `launchd` service continuously (in `--ephemeral`
mode with auto-relaunch, per §3.2).
- **Pros:** Simplest to build and reason about; sidesteps the 24-hour
  reallocation question entirely (host never gets released); lowest
  engineering effort.
- **Cons:** No burst capacity (PRs queue serially); host is a single point of
  failure; cost is a flat monthly/hourly rate regardless of utilization —
  only worth it if utilization is reasonably high or CI cadence tolerates
  queuing.

### Option B — Scheduled allocation/release cycles
Allocate the Dedicated Host at start of business hours, release it after
(at least) 24 hours, driven by a scheduled Lambda/EventBridge job, to save
money during nights/weekends.
- **Pros:** Can reduce cost vs. Option A if there are predictable quiet
  periods longer than the 24-hour minimum (e.g., host down all weekend).
- **Cons:** Because of the 24-hour minimum, this only saves money in fairly
  coarse-grained windows (effectively "off for whole days"), not a normal
  business-hours on/off cycle — a host released Friday evening can't be
  reallocated Monday morning without eating another 24-hour charge; the
  savings are real but modest, and there's real engineering effort
  (automation, and handling a PR that lands while the host is down) for that
  modest saving.

### Option C — Hybrid: keep GitHub-hosted as primary, self-hosted as
supplemental/scheduled
Keep `build-osx.yml` on GitHub-hosted `macos-26` for normal PR/push CI;
add a separate scheduled or manually-triggered workflow (e.g., nightly) that
runs on the self-hosted AWS Mac runner, as a lower-stakes way to validate the
self-hosted path and get partial cost/control benefit without committing PR
CI to it.
- **Pros:** Lowest risk; validates the self-hosted stack under real load
  before cutting PR-blocking CI over to it; easy rollback (nothing to roll
  back — GitHub-hosted path never removed).
- **Cons:** Doesn't fully satisfy "switch CI to AWS-hosted runners" if the
  goal is to stop paying for/depending on GitHub-hosted macOS minutes
  entirely; is a stepping stone, not an end state.

### Recommendation
Given the 24-hour minimum host constraint (§2) makes true ephemeral/ per-job
self-hosted runners impractical for this workflow, **Option A (single
persistent host) preceded by a burn-in period run as Option C** is the
pragmatic path:
1. Stand up the self-hosted Mac runner and validate it via a non-blocking
   scheduled/manual workflow (Option C) for a couple of weeks.
2. Once AMI parity, security gating (fork-PR restriction), and monitoring are
   proven out, cut `build-osx.yml`'s matrix over to the self-hosted label and
   retire the GitHub-hosted entry.
3. Revisit Option B only if real usage data (§3.3) shows sustained multi-day
   quiet windows worth exploiting.

## 5. Migration steps (once the above is approved)

1. **Data gathering:** pull actual GitHub Actions macOS-minute usage for this
   workflow; get current EC2 Mac Dedicated Host pricing for the target
   region; produce a real cost comparison (§3.3). This is a go/no-go gate.
2. **AWS account/networking setup:** VPC/subnet, security group with the
   restricted egress from §3.2, IAM role for the instance profile scoped to
   the two `opendap.github.actions.*` buckets.
3. **Allocate a Mac Dedicated Host + launch instance**, install Xcode CLT,
   Homebrew (matching `/opt/homebrew` layout), the packages listed in
   `build-osx.yml:71-80`, AWS CLI, and the GitHub Actions runner agent; script
   this build (Packer or a plain shell provisioning script checked into
   `docs/` or a new `aws/` directory) so it's reproducible, not a one-off
   manual setup.
4. **Register the runner** with a label set (e.g. `[self-hosted, macos,
   arm64]`) via automated registration-token retrieval, run it as a
   `launchd` service in ephemeral/auto-relaunch mode.
5. **Add a non-blocking scheduled/manual workflow** targeting the new label
   to validate the pipeline end-to-end (dependency fetch, libdap build,
   bes build/test, failure-log upload) before touching `build-osx.yml`'s PR
   path.
6. **Tighten trigger security** on `build-osx.yml` (fork-PR gating) before
   any self-hosted runner is reachable from `pull_request` events.
7. **Cut over:** change `build-osx.yml`'s matrix `runs-on` entry from
   `macos-26` to the self-hosted label; keep the GitHub-hosted entry in the
   matrix in parallel for a burn-in window (both run, compare results) before
   removing it.
8. **Monitoring/alerting:** CloudWatch alarm or equivalent on instance/runner
   health; document the on-call/ownership for AMI patching and host
   maintenance.
9. **Decommission GitHub-hosted entry** from the matrix once burn-in is
   clean, and remove/rotate the now-unnecessary `AWS_GHA_BES_ID`/
   `AWS_GHA_BES_SECRET_KEY` repo secrets if the instance-profile credential
   path fully replaces them.

## 6. Explicit non-goals of this plan

- Does not touch `.travis.yml` or the Linux/distcheck/Docker/Sonar pipeline.
- Does not change any workflow, script, or infrastructure file — this is a
  planning document only, per the request.
- Does not pick a final architecture option for you — §4's recommendation is
  a starting point; the cost-data gate in step 1 of §5 could change it.

## 7. Open questions for the team before implementation starts

- Who owns AWS billing/account access for a new EC2 Mac Dedicated Host
  (this is real, non-trivial spend even in the cheapest option)?
- What's the acceptable PR-queue latency if only one Mac Dedicated Host
  exists (serial builds) vs. paying for a second host for parallelism?
- Is there an existing AWS account/VPC this should live in, or does it need
  a new one, and does that account already have Mac Dedicated Host quota
  (this may require an AWS support quota-increase request, which is not
  instant)?

## Appendix A: Comparison with commercial third-party CI runner services

The main plan assumes "hosts we own" means we operate the compute ourselves
(self-hosted on our own AWS account). There's a third path worth naming
explicitly: paying a commercial vendor for CI compute that's faster and/or
cheaper than GitHub-hosted, without operating any infrastructure ourselves.
This appendix compares that path against the self-hosted-on-AWS plan above.
It does not change the recommendation in §4 — it's here because "self-host
vs. pay a vendor for speed" is a real fork in the road the team should decide
on deliberately, not by default.

### A.1 A constraint that applies to every option in this appendix

Apple's license terms require macOS to run on Apple hardware. That's true for
us (§2: it's why AWS's answer is Mac *Dedicated Hosts*, not ordinary VMs), and
it's equally true for any third-party vendor: a commercial "fast macOS runner"
service is still, physically, real Mac hardware somewhere (their own
colocated Mac minis/Studios/Mac Pros, or — like us — EC2 Mac Dedicated
Hosts they operate). So "faster" from a macOS CI vendor comes from things
like: better fleet utilization/warm pools (they amortize the 24-hour AWS
minimum, or fixed hardware costs, across many customers), Apple Silicon
generation/spec choices, faster storage, and dependency/build caching — not
from some macOS-specific trick unavailable to us. That's a real advantage
(their business is CI throughput; ours is bes), but it's an operational and
cost advantage, not a hardware loophole.

### A.2 Vendor categories relevant to a macOS GitHub Actions workflow

- **Blacksmith** (blacksmith.sh). Positioned as a drop-in replacement for
  GitHub-hosted runners (change the `runs-on` label, keep the workflow
  otherwise unchanged), emphasizing bare-metal speed and built-in
  dependency/Docker-layer caching. **Caveat — verify before relying on this:**
  Blacksmith's publicly marketed product has been Linux (x86_64/arm64)
  focused; I could not confirm current macOS runner support or macOS pricing
  from what I know, and I'm not going to guess. If macOS support matters for
  this decision, confirm directly with Blacksmith rather than assuming their
  Linux story carries over — this is the kind of vendor-capability claim that
  changes fast and shouldn't be taken from a stale mental model.
- **Mac-specific CI cloud vendors** (e.g., MacStadium/Orka, Cirrus
  Labs' "Cirrus Runners"). These exist specifically because of the
  Apple-hardware constraint in A.1 — their whole business is owning/managing
  Mac fleets (their own hardware or EC2 Mac instances) and renting CI capacity
  on them, with a GitHub Actions integration (self-hosted runner registration
  or an app-based integration) so workflow files stay mostly unchanged. This
  category is the more directly comparable one for `build-osx.yml`
  specifically, more so than Blacksmith's Linux-first product.
- **GitHub's own "larger runner" tier.** Worth naming even though it's not
  third-party: GitHub sells bigger/faster macOS-hosted runners (more
  CPU/RAM) as a paid upgrade within GitHub Actions itself, no new vendor
  relationship, no new secrets/IAM, no new trigger-security work. This is the
  lowest-effort lever if the actual goal is "make CI faster" rather than
  "stop depending on GitHub-managed compute."

Any of these should be re-verified for current macOS availability, pricing,
and GitHub Actions integration mechanism at decision time — vendor offerings
in this space have moved quickly and I'm not going to state current prices
here without a source.

### A.3 Comparison

| Dimension | Status quo (GitHub-hosted `macos-26`) | GitHub larger/faster runner tier | Self-hosted AWS EC2 Mac Dedicated Host (§4) | Commercial Mac CI vendor (MacStadium/Cirrus-style) | Blacksmith-style generic runner service |
|---|---|---|---|---|---|
| Who operates the hardware | GitHub | GitHub | Us | Vendor | Vendor (macOS support unconfirmed, see A.2) |
| Satisfies "hosts we own" | No | No | Yes | No | No |
| Setup/engineering effort | None (current state) | Low (config change + billing) | High (AMI, IAM, networking, monitoring, security gating — all of §3–5) | Low–medium (vendor onboarding, runner registration, workflow label change) | Low (mostly a `runs-on` label swap), *if* macOS is actually supported |
| Ongoing ops burden | None | None | Ours: patching, health monitoring, capacity, on-call (§3.4) | Vendor's | Vendor's |
| Cost model | Per-minute, 10x macOS multiplier, no minimums | Per-minute, higher rate than standard, no minimums | 24-hour Dedicated Host minimum per allocation (§2); cost-effective only at sustained utilization or with a reservation | Per-minute or subscription; vendor absorbs the 24h AWS minimum across their customer base, so pricing can undercut both GitHub and DIY self-hosting | Unconfirmed for macOS |
| Speed vs. GitHub-hosted | Baseline | Faster (more CPU/RAM) | Comparable or faster depending on instance choice/idle-runner warm start | Vendors typically market meaningful speedups (dedicated hardware, caching) — verify with a real benchmark on this codebase, don't take vendor marketing numbers as-is | Unconfirmed for macOS |
| Security/isolation model | GitHub-managed, ephemeral per job | Same as status quo | Ours to build (§3.2) — real work, real risk if done poorly | Vendor-managed; still needs the same fork-PR trigger gating on our workflow side regardless of who hosts the runner | Same caveat |
| Vendor/platform lock-in | Low (GitHub Actions is already the platform) | Low | None beyond AWS | Medium (vendor-specific runner registration/tooling) | Medium |
| Fits the original ask ("hosts we own in AWS") | No | No | Yes | No | No |

### A.4 How this should change the decision

- If the actual goal is strictly "we own the hosts, in AWS" (as scoped at the
  start of this plan), the vendor options in this appendix don't satisfy that
  goal by definition, regardless of price or speed — they're presented here
  for completeness and because "own vs. rent" is a decision worth making
  consciously, not because they're a substitute recommendation.
- If the underlying goal is closer to "CI is slow/expensive/too dependent on
  GitHub, fix that" and ownership isn't actually a hard requirement, this
  appendix suggests checking GitHub's own larger-runner tier and a Mac-focused
  CI vendor (MacStadium/Cirrus-style, not Blacksmith, given A.2's caveat)
  *before* investing in §5's self-hosted build-out — they plausibly get most
  of the speed/cost benefit with a fraction of the engineering and ongoing
  operational cost, at the price of not owning the hardware.
- Either way, the fork-PR trigger-security work in §3.2 is not optional and
  not specific to self-hosting — it applies the moment `build-osx.yml` runs
  on any runner GitHub doesn't manage itself, vendor or ours.
- Recommended next step if this path is worth pursuing: get a real quote and
  a real benchmark (run this repo's actual build/test suite) from one
  Mac-focused vendor, and compare that measured wall-clock time and quoted
  price against the cost-data gate already required in §5 step 1. Don't
  decide on marketing claims from either side.
