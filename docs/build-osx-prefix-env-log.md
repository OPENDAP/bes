# build-osx prefix env log

## 2026-06-08 13:46:08 MDT

Prompt:

> Look at [build-osx.yml](.github/workflows/build-osx.yml) and tell me if the `export prefix="$HOME/install"` lines can be replaced with `env:` entries for `prefix`? If so, you may make those changes.

Assumptions and environment:

- Repository: `/Users/jhrg/src/opendap/hyrax/bes`
- Workflow inspected: `.github/workflows/build-osx.yml`
- The matrix uses GitHub-hosted macOS runner labels `macos-15` and `macos-26`.
- I ignored `retired/` per repository instructions.

Reasoning:

- The repeated Bash exports can be replaced by a job-level `env` entry because all affected steps run in the same job and only need the same install prefix.
- A YAML value of `prefix: "$HOME/install"` would not be equivalent. GitHub Actions passes `env:` values to the shell as environment values; Bash does not recursively expand `$HOME` inside `$prefix`.
- The cache paths and hyrax-dependencies extraction already assume the hosted macOS runner home directory. I used the concrete path `/Users/runner/install` so `$prefix` resolves to the same location as the previous `export prefix="$HOME/install"` on those runners.

Changes made:

- Added job-level `env.prefix: "/Users/runner/install"` in `.github/workflows/build-osx.yml`.
- Removed the four repeated `export prefix="$HOME/install"` lines from the workflow steps.

Validation run:

- `git diff --check -- .github/workflows/build-osx.yml`
- `ruby -e 'require "yaml"; YAML.load_file(".github/workflows/build-osx.yml"); puts "yaml ok"'`

Validation not run:

- I did not run the GitHub Actions job.
- I did not run `actionlint`; it is not installed in this environment.
- I did not run `make check`; this is a CI workflow-only change.

