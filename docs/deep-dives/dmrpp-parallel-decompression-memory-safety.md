# DMR++ parallel decompression: can threads corrupt memory?

- **Date:** 2026-10-30
- **Commit:** cee814573
- **Question:** Parallel decompression is on by default in `dmrpp_module`
  (`DmrppRequestHandler::d_use_compute_threads = true`). There seems to be a problem
  where threads overwrite memory and corrupt a pointer. How likely is that?
- **Files:** `modules/dmrpp_module/DmrppArray.cc`, `SuperChunk.cc`, `Chunk.cc`
  (plus `Chunk.h`, `SuperChunk.h`, `DmrppNames.h`)

## Short answer

The compute-thread design, as written, has **no inter-thread data race on a pointer
when the DMR++ is well formed.** Each thread works on one `Chunk` that no other thread
touches. Threads write to disjoint parts of the array's buffer. The main thread is
blocked while they run.

The most credible way to get pointer or heap corruption is a **double `delete[]` in
`Chunk::filter_chunk()` / `inflate()`**. Threading does not cause it, but with many
threads running, the damaged heap shows up later and in unrelated places. The trigger
is narrow: a deflate stream must decompress to more than the expected chunk size and
then fail. With good data that is rare. The double-deflate (GHRSST-style) path is the
one most likely to reach it. Overall likelihood: **low, but real and not tested.**

## How the parallel path is built

Transfer and decompression are two separate layers of concurrency.

1. **Transfer (`DmrppArray.cc:788-965`, `read_super_chunks_concurrent_curl_multi`).**
   This is one thread running a libcurl multi loop. `prepare_super_chunk_transfer()`
   (`:682`) allocates each SuperChunk's buffer. It slices that buffer into the child
   `Chunk`s (`map_chunks_to_buffer`, ownership `false`). It also creates a temporary
   `Chunk` that libcurl writes into through `chunk_write_data` (`Chunk.cc:192`). When a
   transfer finishes, `finish_super_chunk_transfer` calls `send_off(sc)`, which runs
   **synchronously** on the main thread. libcurl does no I/O outside
   `curl_multi_perform()`, so no other transfer writes into a buffer while
   decompression runs. The older `std::async` transfer version is `#if 0`'d out.
2. **Decompression (`SuperChunk.cc:292-484`, `process_chunks_*_concurrent`).**
   `SuperChunk::read_*_curl_multi()` starts one `std::async` task per child `Chunk`,
   up to `d_max_compute_threads`. It waits for all of them before it returns. Each task
   runs `process_one_chunk*` (`SuperChunk.cc:77-154`), which does three things:
   - `read_chunk()` returns at once because `d_is_read` is already set.
   - `filter_chunk()` (`Chunk.cc:801`) runs inflate, shuffle, fletcher32, lzw or
     tiffpred. It replaces the chunk's slice of the SuperChunk buffer with a new
     buffer that the chunk owns. The old slice is not deleted, because ownership was
     `false`.
   - `insert_chunk` / `insert_chunk_unconstrained` / `insert_chunk_unconstrained_dio`
     copy the result into `array->get_buf()` (or `d_structure_array_buf`). That buffer
     was sized before any thread started.

### State the threads share

| State | Access from threads | Safe? |
|---|---|---|
| Each `Chunk` (`d_read_buffer`, sizes, flags) | Only its own task | Yes. One task per chunk, and each chunk is in one SuperChunk |
| SuperChunk `d_read_buffer` | Read-only (each chunk reads its own slice) | Yes |
| Array value buffer (`get_buf()`) | `memcpy` to the region computed from the chunk's position | Yes, **if** chunk positions don't overlap |
| `DmrppArray` getters (`get_filters()`, `get_dimension()`, `get_chunk_dimension_sizes()`, `var()->type()`) | Read-only | Yes |
| `chunk_processing_thread_counter` | `atomic_uint`. Incremented under a mutex, decremented without one | Yes (atomic) |
| `BESDEBUG` output | Writes to a shared ostream | **No, when `dmrpp:3` debugging is on** |

## Findings

### F1: Double `delete[]` when `inflate()` grows its buffer and then fails (most likely source of heap corruption)

`inflate()` (`Chunk.cc:321-388`) takes `char **destp`. When the output fills the
buffer before the stream ends (`avail_out == 0` with `Z_OK`), it allocates a buffer
twice as large and calls **`delete[]` on the original `*destp` buffer**
(`Chunk.cc:364-377`). It does not update `*destp` until it returns successfully
(`:382`).

If a later `inflate(&z_strm, …)` call fails (`Z_DATA_ERROR`, `Z_BUF_ERROR`, …),
`inflate()` throws at `:359` and `*destp` still points at the freed buffer. Then the
`catch` blocks in `filter_chunk()` free it a second time:

- single deflate: `delete[] dest_deflate;` (`Chunk.cc:926-929`)
- multiple deflate: `delete[] dest_deflate; delete[] tmp_dest;` (`Chunk.cc:900-904`)

The newly allocated buffer also leaks.

A double free corrupts the allocator's free lists. With several decompression
threads sharing one heap, the crash usually comes later and somewhere unrelated,
which looks like "a thread overwrote a pointer."

**How likely it is.** The bug needs both of these:

1. the decompressed output is larger than `chunk_size` (`chunk_size * elem_width`
   bytes), so the buffer gets reallocated, and
2. the stream is then rejected by zlib.

With correct data and a correct DMR++ chunk size, step 1 does not happen for a single
deflate. The GHRSST double-deflate case is the known way to reach the reallocation
(see the comment at `Chunk.cc:362-363`, KY 2022-08-03). Step 2 needs corrupt or
misdescribed bytes. A wrong size in the DMR++, or an object that changed under its
DMR++, would do it. Short reads are already caught by byte-count checks, so a plain
truncated transfer won't trigger it.

The bug is independent of threading. Serial mode has it too. Threading only makes the
symptom harder to trace. No unit test covers "reallocate, then fail": `ChunkTest`'s
corrupt-stream test fails before any reallocation.

### F2: No bounds checks in the insert routines (needs a malformed DMR++)

- `insert_chunk_unconstrained` (`DmrppArray.cc:1050`): if a chunk's
  `position_in_array` is past the dimension's `stop`, then
  `end_element - chunk_origin[dim]` wraps around (it is unsigned). `memcpy` then gets
  a huge length, or the recursion runs a huge number of times. Either one writes past
  the array buffer.
- `insert_chunk_unconstrained_dio` (`:1099`) copies `chunk->get_size()` bytes to
  `get_buf() + direct_io_offset` without checking the result against the buffer size
  that `reserve_value_capacity_ll_byte(get_var_chunks_storage_size())` set.
- If two chunks have the same or overlapping positions, two threads write the same
  bytes at once. That race is on data, not on pointers.

All of these need a bad DMR++. They are out-of-bounds writes, and in threaded mode
they can hit another thread's buffer or heap metadata.

### F3: `BESDEBUG` from compute threads

`process_one_chunk*`, `filter_chunk` and `get_next_future` all log through `BESDEBUG`.
When the `dmrpp:3` or `dmrpp` context is on and output goes to a file stream, several
threads write to one `std::ostream` with no lock. That is a data race and can corrupt
the stream's internal buffer. It does not apply to production unless debug logging is
on, but it is worth knowing if the corruption showed up while debugging was enabled.

### F4: Thread-counter leak, which hangs rather than corrupts (side note)

`start_one_chunk_compute_thread` (`SuperChunk.cc:219-220`) increments the global
counter **before** calling `std::async`. If `std::async` throws `std::system_error`
(the system can't create a thread), the count is never given back. The other two
start variants increment after the call.

Also, the `catch (...)` in `process_chunks_*_concurrent` waits for the outstanding
futures but does not decrement the counter for them. It is only reset to 0 when the
exception came from `get_next_future`. A count leaked this way stays for the life of
the `beslistener`. Once the leaked count reaches `d_max_compute_threads`,
`process_chunks_concurrent` spins forever: the queue is not empty, `futures` is
empty, and no thread can start. This produces a hang, not memory corruption.

## Requirements cross-check

No FR, NFR, UC or IC covers the DMR++ handler, thread safety, or decompression. The
only DMR++ entries (FR-007/008/011) are about finding NGAP URLs. No ADR covers the
compute-thread design. **To verify:** whether "parallel decompression is on by
default and must be memory-safe" should be written down as an NFR.

## Follow-ups (2026-10-30)

- F1 → **BUG-005**. A standalone ASan build confirmed the double free.
  `DmrppArray::inflate_simple()` has the same stale `*destp`.
- F2 → **FR-012** (Proposed).
- F3 → **BUG-006**. Correction to F3 above: builds made without `--enable-developer`
  define `NDEBUG`, which compiles `BESDEBUG` out, so production builds can't hit this.
- F4 → **BUG-007**.

## Suggested next steps

- F1 → `/fix-bug`. It needs a regression test: a deflate stream larger than
  `chunk_size` with corrupted bytes after the point where the buffer fills, run under
  ASan (`--enable-asan`).
- F2 → `/fix-bug` or `/new-requirement`, depending on whether we commit to rejecting
  a malformed DMR++ cleanly instead of crashing.
- F4 → `/fix-bug` (small).
- To confirm in production: get a core or an ASan build from a failing request, and
  check whether the dataset uses `deflate deflate` or a deflate whose output exceeds
  the chunk size. If it does, F1 is the prime suspect.
