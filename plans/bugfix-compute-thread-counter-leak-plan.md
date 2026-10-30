# Bugfix Plan: The DMR++ compute-thread counter leaks on error paths, and `process_chunks_*_concurrent` can then spin forever

**Bug:** BUG-007
**Status:** Draft
**Created:** 2026-10-30

## Summary

The global `chunk_processing_thread_counter` in `SuperChunk.cc` is not decremented on
some error paths. The leaked count stays for the life of the `beslistener`. Once it
reaches `d_max_compute_threads`, no new compute thread can start, and the scheduling
loop in `process_chunks_*_concurrent` spins forever. The fix makes the counting
exception-safe and turns "no thread can ever start" into an error instead of a hang.

## Root cause

All in `modules/dmrpp_module/SuperChunk.cc`:

1. **The count goes up before the thread exists.** `start_one_chunk_compute_thread`
   (`:212-227`) runs `chunk_processing_thread_counter++` *before*
   `futures.push_back(std::async(...))` (`:219-220`). If `std::async` throws
   `std::system_error` (no thread can be created, e.g. `EAGAIN` near `ulimit -u`), or
   `push_back` throws `bad_alloc`, the count stays raised. The two `_unconstrained`
   variants (`:245-246`, `:260-261`) increment afterwards and are fine.
2. **The `catch` drains futures without decrementing.** In each of
   `process_chunks_concurrent` (`:340-349`), `process_chunks_unconstrained_concurrent`
   (`:417-426`) and `process_chunks_unconstrained_concurrent_dio` (`:474-483`), the
   `catch (...)` calls `get()` and `pop_back()` on every outstanding future but never
   decrements the counter. If one of those `get()` calls throws, the `catch` stops
   early. The remaining `std::async` futures still wait for their threads in their
   destructors, but those threads aren't counted down either.

   If the exception came from `get_next_future`, the counter was already set to 0
   (`DmrppArray.cc:134-135`), so nothing leaks. It leaks when the exception comes from
   anywhere else in the loop: `new one_chunk_args`, `std::async`, or `push_back`.
3. **The loop that hangs.** With the counter at the limit and `futures` empty, every
   pass does this:
   - skips `get_next_future`;
   - `start_one_chunk_*` returns `false`;
   - `chunks` is not empty, so `done` stays `false`.

   That is an endless busy loop (`:300-339` and the two copies). Because the counter is
   a global and the `beslistener` serves many requests, one bad request can hang every
   later DMR++ request on that listener that uses compute threads.

The counter is changed only from the thread running the request. Compute threads never
touch it. So this is about exception safety, not a race.

## Related requirements

None. Nothing documents what should happen when thread creation fails.

## Fix approach

Keep the change inside `SuperChunk.cc`:

1. **Count only threads that started.** In `start_one_chunk_compute_thread`, move
   `chunk_processing_thread_counter++` after the `push_back`, as in the other two
   variants.
2. **Decrement while draining.** In each of the three `catch (...)` blocks, decrement
   the counter once per future removed. Wrap each `get()` in its own
   `try { … } catch (...) {}`, so one failing thread doesn't stop the drain. After the
   loop, rethrow the original exception.
3. **Fail instead of hanging.** In each of the three scheduling loops, after the inner
   start loop: if no thread started, `futures` is empty, and `chunks` is not empty,
   throw `BESInternalError`. The message should name the counter value and
   `d_max_compute_threads`, and the counter should be reset to 0 first so the next
   request starts clean. A loop that can make no progress should end the request with
   an error, not take the listener down.

Why not more: the three functions are nearly identical, and the comment at `:278-287`
admits it. Merging them, or replacing the global counter with a per-call RAII token, is
the right long-term cleanup, but it is a `TASK`. This plan fixes the leak and the hang
in the code as it stands.

## Regression risk

- Every DMR++ read with `d_use_compute_threads=true` goes through these loops, and that
  is the default.
- On the success path, only the order of `++` and `push_back` changes in one function.
  The draining and the new guard run only on error or exhaustion.
- The guard in step 3 can't fire by mistake on the success path: when `futures` is
  empty, the counter should be 0, which is below the limit (minimum 8,
  `DmrppRequestHandler.cc:126,208`). So the start succeeds.
- Existing coverage: `CurlHandlePoolTest::process_one_chunk_threaded_test_*` drives
  `process_chunks_concurrent` with a `MockDmrppArray` (`CurlHandlePoolTest.cc:217-221`).
  No test covers an error path.

## Verification

1. **What the hang looks like (unit test).** In `CurlHandlePoolTest`:
   - set `chunk_processing_thread_counter = DmrppRequestHandler::d_max_compute_threads`;
   - call `process_chunks_concurrent` with one chunk that uses a fill value, so there is
     no network I/O.

   Before the fix the call never returns, so run it under a watchdog: `std::async` plus
   `wait_for(5s)` that fails the test on timeout. After the fix it throws
   `BESInternalError` and the counter is back to 0.
2. **Leak accounting (unit test).**
   - Make a chunk whose processing throws (e.g. a corrupt deflate chunk).
   - Arrange for a second exception to come from outside `get_next_future`. If the code
     has no seam for that, add a small one: a test-only hook that makes
     `start_one_chunk_compute_thread` throw on its Nth call.
   - Check that the counter is 0 after the exception leaves `process_chunks_concurrent`.
3. **System level (manual, Linux).** Run a `besstandalone` DMR++ request on a dataset
   with many chunks under a low `ulimit -u`. Before the fix this can hang. After it, the
   request fails with a `BESInternalError`, and a later request on the same process
   works.
4. `make check` in `modules/dmrpp_module`, then `make distcheck`.

## Out of scope

- Merging the three `process_chunks_*_concurrent` copies, or moving to an RAII thread
  token (a TASK).
- The `get_next_future` design itself: polling with `wait_for`, and clearing all
  futures on the first error.
- BUG-005 (inflate double free), BUG-006 (`BESDEBUG` race), and F2 (bounds checks,
  tracked as a new requirement).
