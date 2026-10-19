# Work Log: Self-Hosted AWS Runners for GitHub Actions

## 2026-09-17 12:00 MDT

**Prompt:** "I would like to know how to switch our github workflows for CICD from
hosts managed by GitHub to hosts we own that run in the AWS Cloud. Also address
what requirements are important for this problem? Write a plan, but change no
code."

**Reasoning:**

1. Inspected the repo's actual CI surface before assuming scope:
   - `.travis.yml` (312 lines) runs the primary Linux pipeline: build/check,
     distcheck, srcdist, two Rocky Linux Docker image builds (el8/el9), three
     SonarCloud scans, and an OLFS trigger/deploy stage. This is Travis-CI's
     own hosted infrastructure, not GitHub Actions, and is out of scope for a
     "GitHub workflows" migration.
   - `.github/workflows/build-osx.yml` is the *only* GitHub Actions workflow
     in the repo. It builds/tests bes on GitHub-hosted `macos-26` runners,
     using Homebrew for system deps, S3-hosted prebuilt hyrax-dependencies,
     a from-source libdap4 build, ccache, and an S3 log-upload step on
     failure (`.github/scripts/upload-test-results.sh`).
   - No existing Terraform/Packer/Ansible/CDK in the repo — any AWS-side
     runner infrastructure is greenfield.
2. Because "switch our GitHub workflows" could mean just `build-osx.yml`, or
   could imply also moving the Travis pipeline onto GitHub Actions with
   self-hosted runners, I asked the user to confirm scope rather than assume.
   Answer: scope is `build-osx.yml` only; Travis stays as-is. Also asked
   whether to commit to a runner compute model (persistent vs. ephemeral) or
   have the plan weigh options; user chose to have the plan weigh options.
3. This scoping matters a lot in practice: `build-osx.yml` runs on macOS, and
   AWS's only self-hosted-capable macOS compute is EC2 Mac Dedicated Hosts
   (`mac1.metal` Intel / `mac2.metal` Apple Silicon M1 / `mac2-m2.metal` /
   `mac2-m2pro.metal`), which carry a 24-hour minimum host allocation period
   imposed by Apple's licensing terms — this is a hard, non-negotiable AWS/Apple
   constraint that shapes cost and autoscaling design far more than the
   Linux-runner case would.
4. No code or configuration was changed. This task is planning-only per the
   user's explicit instruction ("write a plan, but change no code").

**Output:** `docs/gha-self-hosted-aws-runners-plan.md`

## 2026-09-17 12:14 MDT

**Prompt:** "Add to this plan a comparison between self-hosting and commercial
third-party machines that cost money but can run jobs faster. For example,
compare using hosts from Blacksmith. Add this as a new section to the plan,
maybe an appendix if that's appropriate."

**Reasoning:**

1. Added "Appendix A: Comparison with commercial third-party CI runner
   services" to the end of the plan document, rather than inserting it
   mid-document — it's a decision that competes with, rather than extends,
   the "we own the hosts" framing the main plan is built around, so it reads
   better as a clearly separated alternative-path comparison.
2. On Blacksmith specifically: I do not have confident, current information
   that Blacksmith supports macOS runners — their publicly marketed product
   has been Linux (x86_64/arm64) focused. Per the "do not make up data"
   instruction, I flagged this explicitly as unconfirmed rather than either
   asserting they do support macOS or asserting they don't, and told the user
   to verify directly before relying on it.
3. Named the more directly relevant vendor category for a *macOS* GitHub
   Actions workflow: Mac-specific CI cloud vendors (MacStadium/Orka, Cirrus
   Labs' Cirrus Runners), since Apple's licensing requires macOS to run on
   Apple hardware — the same constraint that makes AWS's self-host answer
   "Mac Dedicated Hosts" rather than ordinary EC2 also means any third-party
   speed advantage comes from fleet utilization/caching, not a hardware
   loophole.
4. Also named GitHub's own paid "larger runner" tier as the lowest-effort
   option, since it's not obvious the user's actual goal is ownership per se
   vs. simply wanting faster/cheaper CI — made that distinction explicit in
   the "how this should change the decision" closing subsection rather than
   assuming the appendix should just reinforce the self-hosted recommendation
   from the main plan.
5. Deliberately avoided stating current vendor pricing or benchmarked speed
   numbers for any of Blacksmith/MacStadium/Cirrus/GitHub's larger runners,
   since I don't have verified current figures; recommended getting a real
   quote and running this repo's actual build/test suite as a benchmark
   before deciding, and tying that back to the same cost-data gate already
   required in §5 step 1 of the main plan.
6. No code or configuration changed; this remains planning-only.
