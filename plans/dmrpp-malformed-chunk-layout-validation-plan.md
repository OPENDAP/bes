# Plan: Reject a DMR++ whose chunk layout would write outside the variable's buffer

**Status:** Draft
**Created:** 2026-10-30

## Summary

The DMR++ handler copies chunk data into a variable's value buffer without checking
where the copy lands. A malformed DMR++ can therefore make it write out of bounds, and
when compute threads are on (the default) that corrupts the heap shared by every
thread. This plan checks each chunk's layout against the variable before any data is
read, and adds a cheap guard where each chunk is copied. A bad layout then ends the
request with a `BESInternalError`, and nothing is written for that chunk.

## Requirements traced

- **FR-012** (Should, **Proposed — not yet approved**). DMR++ handler: when the chunk
  layout would place data outside the variable's value buffer, throw a
  `BESInternalError` and copy none of that chunk. This covers at least (a) a
  `chunkPositionInArray` beyond the dimension bounds and (b) a direct-IO offset plus
  size past the buffer. Every phase below cites it. Because FR-012 is `Proposed`, this
  plan should not move past `Draft` until FR-012 is `Approved`.

No other FR, NFR or UC applies. FR-001 to FR-011 cover the NGAP container. There are no
NFRs or UCs yet. FR-012 has no related use case (`TBD`).

## Constraints considered

- **IC:** `docs/constraints/implementation-constraints.md` has no entries (only the
  `_EXAMPLE_` row), so no IC bears on this plan and none is in tension with it.
- **ADR:** `docs/decisions/DECISIONS.md` has no accepted ADRs, so there is nothing to
  contradict.
- **CLAUDE.md conventions** (not IC entries, but binding):
  - C++14 only;
  - throw `BESError` subclasses by value;
  - never let an exception escape a thread. The guard in Phase 2 runs inside
    `std::async` tasks, whose futures carry the exception to the request thread, so this
    holds;
  - every new piece of logic needs a test, and `make check` and `make distcheck` must
    pass;
  - baselines change only through `--baselines=yes`.

## What the code does today

Read on 2026-10-30, commit `cee814573`.

- **Where chunks come from.** `DMZ::process_chunk()` (`DMZ.cc:2019`) reads `offset`,
  `nBytes`, `chunkPositionInArray` and `fm` from the DMR++. `Chunk::set_position_in_array`
  checks only the *syntax* of the position string. Nothing checks how many coordinates
  it has or their values. `DMZ::load_chunks()` (`DMZ.cc:2402`) is the only place that
  loads a variable's chunks, and it is the only point where the chunk list, the chunk
  shape (`get_chunk_dimension_sizes()`) and the array shape (`get_array_dims()`) are
  all known. It runs before the variable is read, so before any thread starts.
- **Case (a), unconstrained.** `DmrppArray::insert_chunk_unconstrained`
  (`DmrppArray.cc:1050`) clamps `end_element` to `thisDim.stop`. If
  `chunk_origin[dim] > stop`, the clamped `end_element` is smaller than
  `chunk_origin[dim]`, so `end_element - chunk_origin[dim]` wraps around (it is
  unsigned). That gives either a huge `memcpy` length or a huge recursion count, and
  both write past the array buffer.
- **Case (a), constrained.** `insert_chunk` (`:2081`) only sees chunks that
  `find_needed_chunks_simple` (`:1976`) has selected, so out-of-range chunks are
  usually filtered out first. It still indexes `chunk_origin[dim]` without checking.
- **Wrong number of coordinates (same family).** If a chunk's position has fewer
  coordinates than the array has dimensions, `chunk_origin[dim]` reads past the end of
  the vector, and an empty position (`[]` is rejected, but an absent attribute yields an
  empty vector) is worse. A chunk shape with more dimensions than the array has
  reaches `get_dimension(dim)` with `dim == ndims`. The bounds check there,
  `i > (dim_end() - dim_begin())` at `DmrppArray.cc:388-393`, is off by one, so it
  dereferences `dim_end()`.
- **Case (b), direct IO.** `insert_chunk_unconstrained_dio` (`:1099`) copies
  `chunk->get_size()` bytes to `get_buf() + direct_io_offset` with no check. The
  offsets are not read from the DMR++. `add_dio_var_storage_info_unconstrained`
  (`:3785`) and the subset version (`:3759-3778`) compute them as running sums of the
  `nBytes` of earlier chunks. The buffer is reserved from
  `get_var_chunks_storage_size()`, the sum of those same sizes. Fill-value chunks are
  added *after* the real chunks (`DMZ.cc:2370-2390`) and removed before the direct-IO
  read (`:3803-3804`). So **as written, (b) can't be reached through any DMR++**; the
  check for it guards against future changes to how offsets or the buffer are
  computed.

## Phases

### Phase 1 — Check each chunk's layout once, when the chunks are loaded

**Goal:** reject a malformed layout before any data moves and before any thread starts.
**Satisfies:** FR-012 (a), and the rank cases that make (a) unsafe.

Steps:
1. Add a helper, e.g. `DMZ::validate_chunk_layout(BaseType *btp)`. Call it in
   `DMZ::load_chunks()` right after `process_chunks()` succeeds, before fill-value
   chunks are added. Run it only when the variable is an `Array` and its chunk shape is
   not empty.
2. For the variable, require that the chunk shape has as many dimensions as the array,
   and that no chunk-shape entry is 0.
3. For each chunk, require:
   - the position has as many coordinates as the array has dimensions;
   - for every dimension `d`, `position[d] < array_shape[d]`.
4. On the first failure, throw `BESInternalError`. Name the variable, the chunk's
   position as written in the DMR++, and the failed rule, for example
   `chunkPositionInArray [0,400] is outside dimension 1 (size 360) of variable 'sst'`.
5. Mark the chunks as loaded only after the check passes, so a failed load isn't
   cached as good.

**Tests:**
- `DMZTest`: small DMR++ fixtures, one per rule (position too large, too few
  coordinates, chunk shape with a different rank). Each must throw
  `BESInternalError`, and a good fixture must load without error.
- One Autotest case in `modules/dmrpp_module/tests`:
  - copy an existing chunked fixture's `.dmrpp`;
  - change one `chunkPositionInArray` to a value out of range;
  - check that a `dap` request returns the error response, with a baseline made through
    `./testsuite --baselines=yes`.

  Add the new fixture to `EXTRA_DIST`.

**Risks:**
- A DMR++ in the wild whose layout has always been technically bad but read fine,
  because the bad chunk was never copied, for example a constrained request that
  skipped it. Those requests will now fail. This is intended under FR-012, but it
  changes behavior; see Open questions.
- The cost is O(chunks × dimensions) per variable, once per load, which is small next
  to parsing the XML.

### Phase 2 — Guard each chunk at the point where it is copied

**Goal:** cover chunk lists that don't come through `DMZ` (tests, `add_chunk` callers,
future code), and case (b).
**Satisfies:** FR-012 (a) and (b).

Steps:
1. *(Prerequisite, tracked separately as BUG-008.)* The off-by-one in
   `DmrppArray::get_dimension()` (`i >` should be `i >=` at `DmrppArray.cc:389`) is
   fixed under `plans/bugfix-get-dimension-off-by-one-plan.md`. This plan relies on
   that fix and doesn't repeat it.
2. In `insert_chunk_unconstrained`, before copying anything (at `dim == 0`, or in
   `process_one_chunk_unconstrained` before it calls the function), check:
   - the position has the right number of coordinates;
   - `position[d] <= dimension(d).stop` for every `d`.

   If either fails, throw `BESInternalError`. Checking first means nothing is
   partly copied, which FR-012's "copies none of that chunk" requires.
3. In `insert_chunk_unconstrained_dio`, check
   `direct_io_offset + size <= the buffer size reserved for the variable`, and throw
   `BESInternalError` if not. Find out how `libdap::Vector` exposes its byte capacity
   for that comparison; see Open questions.
4. In `insert_chunk` (constrained), check the number of coordinates once at `dim == 0`.

**Tests:**
- `DmrppArrayTest`: build an array with a chunk added by hand at an out-of-range
  position, or with the wrong number of coordinates. `insert_chunk_unconstrained` must
  throw and leave the destination buffer unchanged (check a canary byte pattern).
- `DmrppArrayTest`: build a direct-IO chunk whose offset plus size is past the reserved
  buffer. `insert_chunk_unconstrained_dio` must throw.
- `get_dimension(ndims)` must throw.
- Run the module's `make check` on a `--enable-asan` build. It must stay clean.

**Risks:**
- These checks run once per chunk, inside compute threads. Each is O(dimensions), which
  is negligible. A throw reaches the request thread through `get_next_future`, which
  resets the thread counter, so this doesn't add to BUG-007's leak. The leak happens
  only when the exception comes from outside `get_next_future`.

## Open questions

- **Approval of FR-012.** It is `Proposed`. Blocks moving this plan past `Draft`.
- **Positions not aligned to the chunk grid** (`position[d] % chunk_shape[d] != 0`).
  These don't write out of bounds, because the copy clamps to the array. They do
  produce wrong values, and `get_chunk_map` / `process_fill_value_chunks` assume
  positions on the grid. Should Phase 1 reject them? That goes beyond FR-012's wording,
  so it would need FR-012 extended or a new FR. Doesn't block Phase 1 as written.
- **Reads past the end of a chunk.** If an unfiltered chunk's `nBytes` is smaller than
  its chunk shape × element size, the copy *reads* past the chunk's slice of the
  SuperChunk buffer. FR-012 covers writes only. Should a related requirement cover
  reads? Doesn't block this plan.
- **Behavior change for existing DMR++ files.** Is there a corpus (e.g. the NGAP
  regression granules) to run Phase 1 against before release, to find layouts that are
  technically bad but have worked until now? This affects release timing, not the
  design.
- **Byte capacity of the buffer (Phase 2, step 3).** Which call gives the bytes
  reserved for the buffer (libdap `Vector`'s capacity × element width, or a size the
  handler records itself when it reserves)? Answer this by reading libdap4 before
  coding step 3.

## Out of scope

- BUG-005 (inflate double free), BUG-006 (`BESDEBUG` race) and BUG-007 (thread-counter
  leak). Each has its own bugfix plan.
- Checking non-chunk parts of the DMR++: attributes, compact data, linked blocks
  (`process_block`), or variable-length string arrays.
- Checking a DMR++ against the data file it describes (offsets past the end of the
  object, checksums).
- Merging the three copies of the insert code, or the three
  `process_chunks_*_concurrent` copies (a TASK).
