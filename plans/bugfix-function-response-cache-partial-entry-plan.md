# Bugfix Plan: Function-response cache keeps a partial entry when the server function throws

**Bug:** BUG-003
**Status:** Draft
**Created:** 2026-10-29

## Summary

When a server function throws while `BESDapFunctionResponseCache` is building a cache entry,
the partial file (just the resource-ID line) stays on disk. Every later identical request then
fails with "DDX response document parse failed" instead of the function's own error. The fix
removes the partial file before the exception propagates.

## Root cause

See the BUG-003 entry in `docs/bugs/BUG-LOG.md`. In brief:
- `write_dataset_to_cache` (`dap/BESDapFunctionResponseCache.cc:549-617`) writes the resource
  ID before it evaluates the function.
- Its `catch (...)` (lines 608–613) unlocks the file but doesn't delete it.
- `load_from_cache` (lines 417–477) treats a matching first line as a hit and hands the empty
  remainder to the DDX parser.

## Related requirements

- **None.** No FR/NFR covers the DAP function-response cache. The violated assumption is that
  a failed request leaves nothing in the cache, so repeating it gives the same result. If this
  should be written down, run `/new-requirement`. This fix doesn't depend on it.

## Fix approach

1. **Remove the partial file in the `catch (...)` block** (line 608). After
   `cache_file_ostream.close()` and before `unlock_and_close(cache_file_name)`:
   - Call `unlink(cache_file_name.c_str())`, while the process still holds the exclusive lock
     from `create_and_lock`.
   - If `unlink` fails, write an `ERROR_LOG` line naming the file and `errno`. Don't throw:
     the original exception must still be the one that propagates. Throwing inside a `catch`
     that then rethrows would replace it.
2. **Why `unlink`, not `BESFileLockingCache::purge_file`.** `purge_file`
   (`dispatch/BESFileLockingCache.cc`) subtracts the file's size from the cache-info total.
   The partial file's size was never added, because `update_cache_info` runs only on the
   success path (line 603). Using `purge_file` would drive the recorded cache size too low.
3. **Comment.** Add a short comment at the `unlink` that cites BUG-003 and explains why the
   file must not survive a failed evaluation.

**Why this scope.** The defect is the missing cleanup on one error path, and this fix adds
it.
- **A broader change was considered and left out.** That change would make `load_from_cache`
  treat an entry it can't parse as a miss: purge it and rebuild. It would also heal entries
  that already exist on deployed servers. But it changes the cache's read-side behavior for
  every corrupt-entry case, not only this one, and it needs its own locking analysis, because
  a reader holds only a shared lock. See Out of scope.

## Regression risk

- **Success path.** It's untouched. The existing `FunctionResponseCacheTest` cases
  (`cache_a_response`, `cache_and_read_a_response`, `cache_and_read_a_response2`) cover it
  and must still pass.
- **Concurrency.** Suppose process A is writing the entry and its function throws, while
  process B is in `get_or_cache_dataset`'s "2nd try" `load_from_cache` (lines 390–391),
  blocked in `get_read_lock` on the same file.
  - If B opened the file before A's `unlink`, B still gets a shared lock on the unlinked
    inode once A unlocks. B reads the resource-ID line and hits the same parse error,
    once, for that one request.
  - After the `unlink`, no new request can find the partial file.
  - This is a much smaller window than today's "forever". Closing it completely is part of
    the read-side change in Out of scope.
- **Existing poisoned entries.** Partial files already on deployed servers (for example, the
  two in `/tmp/hyrax_func` on the dev machine) stay until size-based purge or manual removal.
  The release notes should tell operators to clear `DAP.FunctionResponseCache.path` after
  upgrading.
- **Other callers.** `write_dataset_to_cache` is private. Its only caller is
  `get_or_cache_dataset` (line 385).

## Verification

1. **Regression unit test** in `dap/unit-tests/FunctionResponseCacheTest.cc`, named
   `failed_function_leaves_no_cache_entry`:
   - Call `cache->get_or_cache_dataset(test_dds, "test()")`. `test()` with no argument throws
     `libdap::Error` "test(name) requires one argument." (`dap/unit-tests/TestFunction.h:31-32`).
     Assert that the message contains "requires one argument".
   - Call it a second time and assert the same message. Before the fix, the second call
     throws `DDXParseFailed` "The DDX response document parse failed…". Both are
     `libdap::Error`, so assert on the message, not the type.
   - Assert that the test's cache directory holds no `func_*` entry for that resource ID.
     Use the directory and prefix the fixture's `setUp` configures; don't hardcode `/tmp`.
   - Register the test in the suite. Run it against the unfixed code once, confirm the
     second-call assertion fails, and record that in the PR.
2. `make check` in `dap/unit-tests`, then from the top of the build tree.
3. **BUG-003 reproduction against a real server.** `cmdln/tests` talks to the *installed*
   server, so build, install, and restart that BES with the fix. Then:
   - Clear `/tmp/hyrax_func`.
   - In `cmdln/tests`, run `./testsuite 48 51` twice in a row. Both runs pass.
   - The cache directory has no new entry whose content is only the resource-ID line.
4. `make distcheck`. `cmdln/tests` 48 and 51 should pass. Test 61 also needs BUG-004's fix.

## Out of scope

- **Making `load_from_cache` self-healing** (treat an entry it can't parse as a miss, then
  purge and rebuild). This would also cover the remaining concurrency window and entries that
  already exist. It needs its own bug or task.
- **Making `cmdln/tests` hermetic** (its own `besdaemon` and cache directory instead of the
  installed server). That's how BUG-003 surfaced, but it's a separate piece of test
  infrastructure work.
- **BUG-004** (`cmdln/tests/dmrpp/` missing from `EXTRA_DIST`).
- **Other `BESFileLockingCache` subclasses** that write an entry's header before producing its
  content. They haven't been checked for the same pattern.
