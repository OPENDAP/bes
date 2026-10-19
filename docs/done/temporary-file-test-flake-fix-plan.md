# TemporaryFileTest Flake Fix Plan

## 2026-05-18 14:14:40 MDT

Prompt:

> Write a plan to implement the Recommended fix in [temporary-file-test-flake-diagnosis.md](docs/temporary-file-test-flake-diagnosis.md)

Assumptions and scope:

- Repository: `/Users/jhrg/src/opendap/hyrax/bes`
- Target file: `dap/unit-tests/TemporaryFileTest.cc`
- This plan is test-only. It should not change `dap/TempFile.cc`, installed behavior, protocol behavior, or runtime configuration.
- The primary failure context is the Travis `distcheck` job, which runs `make distcheck -j16 GZIP_ENV=--fast`.
- The implementation should keep the test intent unchanged: prove that `TempFile` removes temporary files on normal destruction, exception unwinding, and SIGPIPE-triggered process exit.

## Goals

1. Remove timing-based parent/child synchronization from `sigpipe_test()` and `multifile_sigpipe_test()`.
2. Make the shared filename handoff large enough and explicitly NUL-terminated.
3. Check system call failures so future CI logs identify the real fault.
4. Fix incorrect `munmap()` cleanup in the multi-file SIGPIPE test.
5. Use correct `waitpid()` status predicates for signal termination.

## Implementation Steps

1. Add small test-local helpers in `TemporaryFileTest.cc`.

   Suggested helpers:

   - A constant filename buffer size, for example `static constexpr size_t TEMP_NAME_BUFFER_SIZE = 4096;`.
   - If MAX_PATH is not too large, use that and not 4096.
   - A shared state structure:

     ```c++
     struct SharedTempName {
         volatile sig_atomic_t ready;
         char name[TEMP_NAME_BUFFER_SIZE];
     };
     ```

   - A helper to allocate shared memory with `mmap()` and assert that it did not return `MAP_FAILED`.
   - A helper to copy a `std::string` into the shared buffer using bounded copy and explicit trailing NUL.
   - A helper to wait for readiness with a bounded polling loop.
   - A helper to assert SIGPIPE child termination using `WIFSIGNALED(status)` and `WTERMSIG(status)`.

   Keep these helpers private to the test class or in an unnamed namespace near the test. Avoid adding production APIs.

2. Replace `sleep(1)` synchronization.

   In `sigpipe_test()`:

   - Parent maps one `SharedTempName`.
   - Child creates the temp file, copies the full returned path into shared memory, then sets `ready = 1`.
   - Parent waits until `ready == 1` before checking `file_present()` and sending SIGPIPE.
   - The wait should have a timeout so a child setup failure becomes a clear test failure instead of a hang.

   In `multifile_sigpipe_test()`:

   - Parent maps an array of three `SharedTempName` objects, or maps three individual `SharedTempName` objects.
   - Child sets each entry's `ready` flag after copying that file's full path.
   - Parent waits for all three entries before sending SIGPIPE.

   A pipe-based handshake would also work, but a shared-memory ready flag keeps the current test structure closest to the existing code.

3. Fix shared-memory filename storage.

   - Stop sizing mappings with `TEMP_FILE_PREFIX.size() + 1`; that only fits `"tmpf_test"`, not the absolute path returned by `TempFile::create()`.
   - Use the fixed buffer size for each full path.
   - If a path is too long for the buffer, fail the child-side assertion clearly before signalling readiness.
   - Replace `tmp_file_name.copy(...)` with a bounded copy that writes the trailing `'\0'`.

4. Check system call results.

   Add assertions around:

   - `mmap()` return values.
   - `fork()` result, already partially checked.
   - `kill(pid, SIGPIPE)`.
   - `waitpid(pid, &status, 0)`.
   - `munmap()` return values.

   If the child fails before readiness, the parent should still clean up by waiting for the child where possible.

5. Fix `munmap()` cleanup.

   - In `sigpipe_test()`, unmap the one mapped `SharedTempName`.
   - In `multifile_sigpipe_test()`, unmap the actual mapping or each actual mapped object.
   - Do not call `munmap(glob_name, name_size)` when `glob_name` is a stack array of pointers.

6. Replace raw wait status checks.

   Change checks like:

   ```c++
   status == SIGPIPE
   ```

   to checks equivalent to:

   ```c++
   WIFSIGNALED(status) && WTERMSIG(status) == SIGPIPE
   ```

   Include the raw status in failure messages for diagnostics.

7. Keep edits narrowly scoped.

   - Do not refactor unrelated tests.
   - Do not change `TempFile` signal handling in this pass unless the test fix exposes a production bug.
   - Do not regenerate autotools outputs; `TemporaryFileTest.cc` source-only edits should not require it.

## Validation Plan

Run focused validation first:

```sh
./TemporaryFileTest
```

from the `dap/unit-tests` build directory.

Then stress the previously flaky cases:

```sh
./TemporaryFileTest sigpipe_test
./TemporaryFileTest multifile_sigpipe_test
```

Repeat those commands in a loop if practical, because the original bug is intermittent.

Run the local unit-test target:

```sh
make check
```

from `dap/unit-tests` or an appropriate build directory.

Broaden validation if the build environment is already configured:

```sh
make check
```

from the top-level build directory.

For the CI-specific failure mode, run:

```sh
make distcheck -j16 GZIP_ENV=--fast
```

This is the best validation for the Travis failure, but it depends on the local autotools build being configured with the expected `prefix`, `--with-dependencies="$prefix/deps"`, and `--enable-developer` setup.

## Risks and Review Notes

- The shared ready flag is acceptable for this test because the parent only needs a simple child-to-parent readiness signal before `kill()`. If reviewers are uncomfortable with polling shared memory, use a pipe instead.
- Keep the timeout long enough for loaded CI hosts but short enough that failures do not stall the suite. A few seconds should be sufficient; record the chosen value in the test failure message.
- `TempFile::sigpipe_handler()` calls non-async-signal-safe operations. That is existing production behavior and outside this test-only plan, but the test continues to exercise that behavior.
- If the fixed test still flakes after deterministic synchronization, re-open the production signal handling path in `dap/TempFile.cc` as the next suspect.

## 2026-05-18 14:36:47 MDT

Prompt:

> Implement step one of the plan in [temporary-file-test-flake-fix-plan.md](docs/temporary-file-test-flake-fix-plan.md)

Implementation log:

- Added test-local helper state and functions to `dap/unit-tests/TemporaryFileTest.cc`.
- Used `PATH_MAX` for `TEMP_NAME_BUFFER_SIZE` when available, with a 4096-byte fallback.
- Added helpers for shared-memory allocation, bounded filename copying with explicit NUL termination, bounded readiness polling, and SIGPIPE wait-status assertions.
- Kept the existing SIGPIPE tests unchanged for this step; the new helpers are not wired into the test flow until the later implementation steps.

## 2026-05-18 14:46:36 MDT

Prompt:

> Move on to step 2

Implementation log:

- Replaced the parent-side `sleep(1)` readiness waits in `sigpipe_test()` and `multifile_sigpipe_test()` with `wait_for_temp_name()`.
- Changed the child processes to publish each temporary filename through `copy_temp_name()` and then set the shared `ready` flag.
- Added timeout failure handling so a child setup failure produces a clear test failure instead of relying on CI timing.
- Updated the shared-memory pointer types in both tests from raw filename buffers to `SharedTempName` mappings.

## 2026-05-18 14:49:56 MDT

Prompt:

> Proceed to step 3

Implementation log:

- Confirmed the old undersized shared-memory filename storage has been removed from `sigpipe_test()` and `multifile_sigpipe_test()`.
- Confirmed the tests no longer allocate filename buffers with `TEMP_FILE_PREFIX.size() + 1`.
- Confirmed all child-to-parent filename publication now uses `copy_temp_name()`, which bounds the copy to `TEMP_NAME_BUFFER_SIZE - 1` and writes an explicit trailing NUL byte.
- Confirmed path-length failure happens before setting the shared `ready` flag.
- No additional source edits were needed for step 3 because the storage changes were necessarily introduced while wiring step 2.

## 2026-05-18 14:52:20 MDT

Prompt:

> Proceed to step 4

Implementation log:

- Added test-local helpers to check `kill()`, `waitpid()`, and `munmap()` results with captured `errno` in failure messages.
- Replaced unchecked SIGPIPE and SIGKILL calls in the parent branches with `signal_child()`.
- Replaced direct parent `waitpid()` calls with `wait_for_child()`, which checks both failure and unexpected child pid.
- Replaced direct parent `munmap()` calls with `unmap_shared_temp_name()`.
- Improved the existing fork checks to include `errno` in the assertion message.
- Left the raw SIGPIPE wait-status predicate unchanged for step 6.

## 2026-05-18 14:56:03 MDT

Prompt:

> Do the reaiing step (5 - 7)

Implementation log:

- Confirmed `sigpipe_test()` unmaps the single `SharedTempName` mapping.
- Confirmed `multifile_sigpipe_test()` unmaps each mapped `SharedTempName` individually.
- Replaced raw `status == SIGPIPE` assertions with `assert_sigpipe_exit(status)`, which uses `WIFSIGNALED(status)` and `WTERMSIG(status) == SIGPIPE`.
- Kept the source edits scoped to `dap/unit-tests/TemporaryFileTest.cc`; no production code, autotools files, generated files, or installed/runtime configuration were changed.
