# Bugfix Plan: `BESDEBUG` writes to the debug stream from several threads without a lock

**Bug:** BUG-006
**Status:** Draft
**Created:** 2026-10-30

## Summary

In developer builds, the DMR++ compute threads call `BESDEBUG`. The macro writes to the
one shared debug `std::ostream` with no lock, which is a data race. This fix makes each
`BESDEBUG` line a single write made while holding one process-wide mutex.

## Root cause

`BESDEBUG` (`dispatch/BESDebug.h:66-70`) expands to a chain of `<<` operations straight
onto `*BESDebug::GetStrm()`. Nothing serializes access.

The DMR++ compute threads log while running:

- `process_one_chunk*` (`SuperChunk.cc:79-153`)
- `filter_chunk` (`Chunk.cc:969-1002`)
- `get_next_future` (`DmrppArray.cc:121-157`)

So when the `dmrpp` / `dmrpp:3` contexts are on, several threads write to the same
`std::ostream` at once. The standard doesn't define what happens then. Interleaved text
is the usual result, and corrupting the stream's buffer is possible.

`BESDebug.h:47` already declares a `static std::mutex bes_debug_log_mutex;`. It is
unused. Because it is `static` in a header, each translation unit gets its own copy, so
it could not serialize anything across files even if it were used.

**Scope of exposure.** Without `--enable-developer`, `configure.ac` defines `NDEBUG`
(`configure.ac:171`, `:328`). That makes `BESDEBUG` an empty macro (`BESDebug.h:51`),
so production builds don't have this race. Builds made with `--enable-developer` have
it whenever a dmrpp debug context is on.

## Related requirements

None. Nothing documents what the debug log must guarantee.

## Fix approach

1. Make the mutex one per process. Remove the `static` header variable and replace it
   with a function that returns a function-local static, for example
   `static std::mutex &BESDebug::log_mutex()`. Define it in `BESDebug.cc`.
2. Change the `BESDEBUG` macro so it:
   - formats the whole line, including `get_debug_log_line_prefix()` and `y`, into a
     local `std::ostringstream` *outside* the lock;
   - takes a `std::lock_guard` on `BESDebug::log_mutex()` and writes the finished
     string to `*GetStrm()` in one call.

   Formatting outside the lock avoids deadlock when evaluating `y` itself calls
   something that logs. It also keeps the lock held only briefly.
3. If `BESDebug::SetStrm()` swaps or deletes the stream, take the same lock there, so
   no write can run while the stream is being replaced.

Why at this level: the race is in the shared macro, not in the dmrpp module. Other
modules that log from threads have the same problem. Removing `BESDEBUG` calls from the
compute threads would only hide it.

## Regression risk

- Every module includes `BESDebug.h`, so this is a wide rebuild. The only change in
  behavior is one extra string build and one mutex lock for each debug line that is
  actually printed. Production (`NDEBUG`) builds compile the macro out, so they are
  unchanged.
- `BESISDEBUG` and code that writes to `BESDebug::GetStrm()` directly (outside the
  macro) are not covered. Grep for direct uses and list them in the PR.
- Existing coverage: `dispatch/unit-tests/debugT.cc`.

## Verification

1. Add a case to `debugT.cc`:
   - set the debug stream to an `ostringstream` and turn on a test context;
   - start 8 threads, each writing 2,000 `BESDEBUG` lines tagged `t<i>-<j>`;
   - join them, split the output into lines, and check that there are exactly 16,000
     lines and that each one ends with exactly one complete tag.

   Before the fix this should fail (garbled or missing lines), or crash. Because it is
   a race, the failure is likely but not certain; say so in the test comment. After
   the fix it must pass every time.
2. Optionally, build with `-fsanitize=thread` and run that test. It should report a
   race before the fix and none after. There is no configure switch for TSan, so this
   is a manual check.
3. `make check` from the top of the build tree, then `make distcheck`.

## Out of scope

- Any thread-safety work in `BESDebug::Set` / `IsSet` (the context map). The contexts
  are set while the request is parsed, before any compute thread starts.
- The other DMR++ findings: BUG-005, BUG-007, and F2 (tracked as a new requirement).
