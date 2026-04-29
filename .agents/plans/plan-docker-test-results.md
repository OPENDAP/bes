# Plan: Save `make check` Results in Docker Build

## Goal
Capture and persist the `make check` test logs from the Docker build, reusing the same log packaging approach used by Travis in `travis/upload-test-results.sh`.

## Scope Notes
- Keep changes narrowly scoped to the Dockerfile test section.
- Mirror the Travis log selection and tarball naming style.
- Avoid altering test behavior or BES runtime configuration.

## Steps
1. Identify the exact `make check` block in `Dockerfile` and confirm where to hook in log capture after tests complete (including the `el9` skip path).
2. Reuse the Travis packaging command to create a tarball of `*.log` and `*site_map.txt` (excluding `timing`) in a deterministic location such as `/tmp`.
3. Add a Dockerfile step to persist the tarball in the build stage (optionally with an `ARG`/`ENV` for the filename) and, if desired, copy it into the final image for later retrieval. Use a build context to save the tarball to a location that is configured in the travis/build-rhel-docker.sh script.
4. Modify the .travis.yml or the travis/upload-test-results.sh script to copy this to the S3 bucket used for test results.
5. Document the change in a short comment near the Dockerfile test block so future readers know the tarball mirrors the Travis upload contents.

## Validation
- Build the Docker image and confirm the tarball is produced when `make check` runs (and absent or empty when tests are skipped).
- Spot-check that the tarball contains the expected log files.
