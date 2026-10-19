# TemporaryFileTest Intermittent Failure Diagnosis

## 2026-05-18 14:10:14 MDT

Prompt:

> Sometimes, the test @dap/init-tests/TemporaryFileTest.cc fails during a CICD build. Most oftem this is a failure in the Travis CI build when ir i running the 'make distcheck' build. Casn you disgnose this intermittent failure?

Assumptions and environment:

- Repository: `/Users/jhrg/src/opendap/hyrax/bes`
- Active file appears to be `dap/unit-tests/TemporaryFileTest.cc`; the prompt path `dap/init-tests/TemporaryFileTest.cc` appears to be a typo.
- Primary CI case inspected: `.travis.yml` `distcheck` job, which runs `make distcheck -j16 GZIP_ENV=--fast`.
- No local build of `TemporaryFileTest` was present at diagnosis time, so this is a source-level diagnosis rather than a local reproduction.
- I did not run `autoreconf`, `configure`, `make`, `make check`, or `make distcheck`.

Findings:

1. The likely intermittent failure is in the two fork/SIGPIPE tests, `sigpipe_test()` and `multifile_sigpipe_test()`.
2. Both tests use `sleep(1)` in the parent as a readiness protocol before reading the file names from child-written shared memory. Under `make distcheck -j16`, a loaded CI worker can plausibly delay the child long enough that the parent reads before the child has created the temporary file and copied the name.
3. The shared memory buffers are sized incorrectly. The tests allocate `TEMP_FILE_PREFIX.size() + 1`, but the child copies the full absolute temporary file path returned by `TempFile::create()`. In a distcheck build that path can be long. The current code relies on the OS mapping at least a page, and on the zero-filled bytes after the copied string to act as a terminator. That is undefined relative to the requested object size and makes the test fragile.
4. `std::string::copy()` does not append a trailing NUL byte. The parent immediately constructs `std::string(glob_name)` and passes `glob_name` to `stat()`, so it requires NUL termination that the copy call does not guarantee.
5. `multifile_sigpipe_test()` calls `munmap(glob_name, name_size)`, passing the address of the local array rather than each mapped pointer. That is a test bug, even though it probably happens after the assertions that usually fail.
6. The wait status checks compare the raw `waitpid()` status directly to `SIGPIPE`. The robust form is `WIFSIGNALED(status)` plus `WTERMSIG(status) == SIGPIPE`. The current check may work on Linux for simple signal exits, but it is not the portable or diagnosable way to test this behavior.

Conclusion:

The flake is most likely test-induced, not evidence that `mkstemp()` or `TempFile` RAII cleanup is intermittently broken. The parent process is racing the child and the shared-memory filename handoff is undersized and not explicitly NUL-terminated. `make distcheck -j16` increases the chance of hitting the race because the distcheck tree is building and testing under higher load.

Recommended fix:

- Replace `sleep(1)` readiness with a small shared-memory state flag, a pipe, or another deterministic parent/child handshake.
- Allocate shared memory large enough for the full path, for example `PATH_MAX` or a fixed test buffer such as 4096 bytes, and always NUL-terminate after copying.
- Check `mmap()`, `munmap()`, `kill()`, and `waitpid()` return values.
- In `multifile_sigpipe_test()`, unmap each `glob_name[i]` individually.
- Replace raw wait status comparison with `WIFSIGNALED(status)` and `WTERMSIG(status) == SIGPIPE`.

