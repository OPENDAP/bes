# DMR++ Refactor Plan

## Scope
- Refactor `DmrppArray` and `SuperChunk` (`.h` and `.cc`) to reduce duplication.
- Extract and isolate Direct I/O (DIO) behavior from general flow.
- Remove support for parallel data transfers controlled by:
  - `DmrppRequestHandler::d_use_transfer_threads`
  - `DmrppRequestHandler::d_max_transfer_threads`
- Keep compute-thread behavior (`d_use_compute_threads`) as-is.

## 1. Baseline and Guardrails
- Confirm current tests pass before changes (module unit tests and dmrpp autotests).
- Keep behavior unchanged for:
  - constrained vs unconstrained reads
  - DIO vs non-DIO data correctness
  - fill-value chunks
  - linked-block and buffer-chunk code paths

## 2. Remove Transfer-Thread Configuration Surface
- In `modules/dmrpp_module/DmrppRequestHandler.h`:
  - Remove static members:
    - `d_use_transfer_threads`
    - `d_max_transfer_threads`
- In `modules/dmrpp_module/DmrppRequestHandler.cc`:
  - Remove static definitions for those members.
  - Remove key reads/logging for:
    - `DMRPP.UseParallelTransfers`
    - `DMRPP.MaxParallelTransfers`
  - Remove libcurl Multi API warning block tied to transfer-thread enablement.
- In `modules/dmrpp_module/DmrppNames.h`:
  - Remove:
    - `DMRPP_USE_TRANSFER_THREADS_KEY`
    - `DMRPP_MAX_TRANSFER_THREADS_KEY`
- Evaluate removal of contiguous-transfer threshold config if it is only used for transfer-thread splitting.

## 3. Remove Transfer-Thread Runtime Code in `DmrppArray`
- In `modules/dmrpp_module/DmrppArray.h`:
  - Remove transfer-thread-only argument structs and declarations (e.g., `one_super_chunk_args` if no longer used).
- In `modules/dmrpp_module/DmrppArray.cc`:
  - Remove transfer-thread globals and helpers:
    - transfer-thread mutex/counter
    - `one_super_chunk_*transfer_thread*` wrappers
    - `start_super_chunk_*transfer_thread*` helpers
    - `read_super_chunks_*concurrent*` functions
  - In all read entry points with serial vs transfer-thread branching, delete concurrent branch and keep serial flow only:
    - `read_chunks()`
    - `read_chunks_unconstrained()`
    - `read_chunks_dio_unconstrained()`
    - `read_chunks_dio_constrained()`
    - `read_linked_blocks()`
    - `read_contiguous()` (remove child-chunk parallel transfer split path)
  - Remove transfer-thread debug/timer strings and dead comments.

## 4. Extract DIO Policy in `SuperChunk`
- In `modules/dmrpp_module/SuperChunk.h`:
  - Replace duplicated DIO/non-DIO entry points with a shared policy-based internal path (e.g., `IoMode { Normal, Direct }`).
- In `modules/dmrpp_module/SuperChunk.cc`:
  - Merge duplicated retrieval functions:
    - `retrieve_data()`
    - `retrieve_data_dio()`
  - Keep small, explicit policy branches only where behavior differs (fill-value handling, profiling labels).
  - Merge duplicated unconstrained processing functions:
    - `process_child_chunks_unconstrained()`
    - `read_unconstrained_dio()`
  - Keep compute-thread parallelism intact; only remove duplication and DIO branching sprawl.

## 5. Extract Shared SuperChunk-Building in `DmrppArray`
- Introduce shared helpers for building `queue<shared_ptr<SuperChunk>>`:
  - all chunks
  - needed subset (constraint-based)
  - precomputed DIO subset (`dio_subset_chunks_needed`)
- Use one helper pattern for:
  - superchunk creation
  - chunk add/fallback-to-new-superchunk
  - common error handling
- Apply this to:
  - constrained/unconstrained read paths
  - DIO/non-DIO variants
  - linked-block variants where applicable

## 6. Consolidate DIO/Non-DIO Chunk Processing in `DmrppArray`
- Unify chunk processing wrappers where possible:
  - `process_one_chunk_unconstrained()`
  - `process_one_chunk_unconstrained_dio()`
- Keep insertion functions separate initially if needed for safety:
  - `insert_chunk_unconstrained(...)`
  - `insert_chunk_unconstrained_dio(...)`
- Dispatch through a single shared driver with an insert policy/callback.

## 7. Tests and Cleanup
- Update unit tests that currently force transfer threads to `true`:
  - `modules/dmrpp_module/unit-tests/DmrppArrayTest.cc`
  - `modules/dmrpp_module/unit-tests/SuperChunkTest.cc`
- Remove stale expectations or config references to removed transfer-thread keys.
- Run:
  - dmrpp unit tests
  - relevant autotests (including DIO and constrained/unconstrained coverage)
- Ensure no references remain to removed symbols/keys.

## 8. Recommended Execution Order
1. Remove transfer-thread config symbols and key wiring (`DmrppRequestHandler*`, `DmrppNames.h`).
2. Remove transfer-thread runtime code in `DmrppArray`.
3. Refactor `SuperChunk` DIO/non-DIO duplication into shared policy-based internals.
4. Refactor `DmrppArray` superchunk building/execution duplication.
5. Update tests and run full validation.

## Expected Outcome
- No support for parallel data transfers remains.
- DIO behavior is isolated and explicit, not cloned across multiple methods.
- Reduced duplication in both `DmrppArray` and `SuperChunk`.
- Lower maintenance cost with unchanged external read semantics.
