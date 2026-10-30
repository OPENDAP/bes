# Bugfix Plan: `inflate()` frees the caller's buffer when it grows it, and `filter_chunk()` frees it again when inflate fails

**Bug:** BUG-005
**Status:** Draft
**Created:** 2026-10-30

## Summary

`inflate()` in `Chunk.cc` can replace its output buffer with a larger one. When it
does, it frees the caller's buffer but doesn't tell the caller. If zlib then reports an
error, the `catch` blocks in `Chunk::filter_chunk()` free that buffer a second time,
which corrupts the heap. The fix keeps the caller's pointer current whenever the buffer
is replaced.

## Root cause

`inflate(char **destp, …)` (`modules/dmrpp_module/Chunk.cc:321-388`):

- The function keeps its own pointer, `outbuf = *destp`. When the output fills the
  buffer before the stream ends (`Z_OK` with `avail_out == 0`), it allocates a buffer
  twice as large, copies the data, and calls `delete[] outbuf`. That frees the caller's
  buffer (`:364-377`).
- `*destp` is updated only after a successful finish (`:382`).
- If a later `inflate(&z_strm, …)` call returns an error, the function throws at `:359`.
  `*destp` still holds the freed address, and the new buffer is lost.

The callers then free the stale pointer again:

- single deflate: `catch (...) { delete[] dest_deflate; throw; }` (`Chunk.cc:926-929`)
- multiple deflate: `catch (...) { delete[] dest_deflate; delete[] tmp_dest; throw; }`
  (`Chunk.cc:900-904`). On the first pass `dest_deflate` is stale. On later passes
  `tmp_dest` is the input buffer and is still valid.

Confirmed outside the tree on 2026-10-30. A copy of the loop and the single-deflate
`catch` was built with `-fsanitize=address`. The input was a 4096-byte stream, deflated,
with its last 8 bytes removed, decoded with a 1024-byte expected chunk. ASan reported
`attempting double-free`. The first free was in the growth branch; the second was in the
caller's `catch`.

`DmrppArray::inflate_simple()` (`DmrppArray.cc:1848-1937`) is a copy of the same loop
with the same stale `*destp`. Its two linked-block callers (`DmrppArray.cc:1576`,
`:1650`) have no `catch`, so they don't double-free. On error, though, they leak the new
buffer. For linked blocks the starting `dest_len` is the *compressed* size, so the
growth branch runs on almost every call.

In threaded mode this runs inside a compute thread (`SuperChunk.cc:89` →
`filter_chunk`). The damaged allocator state usually crashes some other thread later,
which looks like "a thread overwrote a pointer." The bug is not a race. Serial mode has
it too.

## Related requirements

None. No FR, NFR or IC covers decompression or memory safety in the DMR++ handler. The
expected behavior is the handler's own design: a bad stream is reported by throwing a
`BESError`. The violation is that the error path corrupts the heap.

## Fix approach

Narrow fix in both copies of the loop:

1. In `Chunk.cc` `inflate()`, set `*destp = outbuf;` right after `outbuf = new_outbuf;`
   in the growth branch. The caller's pointer then always names the one live buffer:
   - its `catch` frees exactly that buffer (no double free, no leak);
   - on success, the `*destp = outbuf;` at the end has no effect.
2. Make the same change in `DmrppArray::inflate_simple()`.

Leave the multiple-deflate bookkeeping in `filter_chunk()` as it is. With `*destp`
current, `dest_deflate` and `tmp_dest` are distinct live buffers whenever the `catch`
runs, so freeing both is correct.

The broader fix is not part of this plan:

- replace `char **` and manual `delete[]` with `std::vector<char>` or
  `std::unique_ptr<char[]>`;
- merge the two copies of the inflate loop.

That is a rewrite of the ownership pattern, which CLAUDE.md says is a `TASK`. Doing it
here would also make this fix harder to review.

## Regression risk

- `filter_chunk()` handles every deflate-compressed chunk, so every compressed DMR++
  read goes through the changed function.
- On the success path, only the order of one assignment changes. The returned size and
  the final buffer are the same.
- `inflate_simple()` is used only for linked blocks
  (`read_linked_blocks*`).
- Existing coverage: `ChunkTest` (`filter_chunk` with deflate, shuffle and fletcher32,
  plus the lzw/tiffpred error tests) and the dmrpp module's Autotest baselines. None of
  them reaches the growth branch followed by an error.

## Verification

1. Add `ChunkTest::filter_chunk_deflate_grow_then_fail_test`:
   - make a chunk whose read buffer is a zlib stream of `4 * N` bytes with the last
     8 bytes removed;
   - call `filter_chunk("deflate", N, 1)`;
   - expect a `BESError` to be thrown.

   On a `--enable-asan` build, ASan must report a double free before the fix and stay
   clean after it. Without ASan, macOS's allocator usually aborts with "pointer being
   freed was not allocated".
2. Add the same case for `"deflate deflate"`, failing on the first pass.
3. If a test seam for `DmrppArray` is practical, add a third case for
   `inflate_simple()`: a truncated stream must throw and must not leak under ASan's
   leak checker. If it isn't practical, note that in the PR.
4. `make check` in `modules/dmrpp_module`, then `make distcheck`.

## Out of scope

- The leak in the linked-block callers of `inflate_simple()`, which have no `catch`.
  Fix step 2 makes the leak smaller (the stale pointer is gone), but the callers still
  leak `dest_deflate` when the function throws. Log it as its own BUG, or fold it into
  the `TASK` that switches to RAII.
- Moving these buffers to RAII and merging the two inflate copies (a TASK).
- F2 from the deep dive (insert routines without bounds checks). It is tracked as a
  new requirement, not as part of this bug.
