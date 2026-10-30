# Bugfix Plan: `DmrppArray::get_dimension()` bounds check is off by one

**Bug:** BUG-008
**Status:** Draft
**Created:** 2026-10-30

## Summary

`DmrppArray::get_dimension(i)` accepts `i == ndims`, so it reads one element past the
dimension list instead of throwing. The fix changes the check from `>` to `>=`, makes
the error message state the index and the rank, and adds a CppUnit regression test.

## Root cause

`modules/dmrpp_module/DmrppArray.cc:388-393`:

```c++
if (i > (dim_end() - dim_begin())) {
    throw BESInternalError("get_dimension: i > (dim_end() - dim_begin())", ...);
}
return *(dim_begin() + i);
```

Valid indices are `0 .. ndims-1`. With `i == ndims` the check passes and the function
dereferences `dim_end()`.

The `insert_chunk*` recursions (`:1058`, `:2091`, `:2355`, `:4146`, `:4227`) reach
`i == ndims` when a DMR++'s `chunkDimensionSizes` has more entries than the array has
dimensions. They recurse to `chunk_shape.size() - 1`, and neither
`DmrppCommon::parse_chunk_dimension_sizes()` nor `DMZ` checks that rank. The details
are in `docs/bugs/BUG-LOG.md` under BUG-008.

## Related requirements

- None documented. The correct behavior is the function's own guard: throw
  `BESInternalError` for an out-of-range index.
- FR-012 (Proposed) is the feature that validates malformed chunk layouts. Its plan,
  `plans/dmrpp-malformed-chunk-layout-validation-plan.md`, relies on this guard as a
  backstop, and its Step 1 now points here.

## Fix approach

1. In `DmrppArray::get_dimension()` (`DmrppArray.cc:389`), change `i >` to `i >=`.
   Compare against `dimensions()`, or cast the iterator difference, so the
   signed/unsigned comparison is explicit. Make the message say what failed, e.g.
   `"get_dimension: index " + to_string(i) + " is out of range for an array of rank " + to_string(ndims)`.
2. No other code changes.

Why this scope: the defect is one comparison. The broader gap is that nothing checks
that the chunk rank matches the array rank before the copy recursion starts. That is
validation of malformed DMR++ input, which belongs to FR-012 and its plan. It is not
added here because FR-012 is still `Proposed`. After this fix, a rank mismatch at
least ends in a clean `BESInternalError` from the first out-of-range
`get_dimension()` call, instead of undefined behavior. Even so, `chunk_origin[dim]`
and `chunk_shape[dim]` indexing in the same functions is still unchecked. That is
FR-012 territory (see Out of scope).

## Regression risk

- Very low. The only change is that an index which used to read out of bounds now
  throws. Valid indices `0 .. ndims-1` behave exactly as before.
- Callers: the five `insert_chunk*` sites above, all in `DmrppArray.cc`. No other file
  calls `DmrppArray::get_dimension()` (it is declared at `DmrppArray.h:118`).
- If any well-formed DMR++ in the test suites did reach `get_dimension(ndims)`, it
  would now fail instead of silently reading garbage. That would be a latent bug
  surfacing, not a regression, and the dmrpp_module `make check` run will show it.
- Existing coverage: `unit-tests/DmrppArrayTest.cc` does not call `get_dimension()`
  directly. The dmrpp_module Autotest suites exercise valid indices through the
  `insert_chunk*` paths.

## Verification

1. Add a CppUnit test to `modules/dmrpp_module/unit-tests/DmrppArrayTest.cc`, following
   the existing `DmrppArray tiat("foo", new libdap::Byte("foo")); tiat.append_dim(...)`
   pattern:
   - build a 2-D `DmrppArray` (dims of size 3 and 4);
   - `get_dimension(0)` and `get_dimension(1)` return the expected sizes;
   - `CPPUNIT_ASSERT_THROW(tiat.get_dimension(2), BESInternalError)`;
   - `CPPUNIT_ASSERT_THROW(tiat.get_dimension(3), BESInternalError)`, which also
     covered the old code.

   Register it in `CPPUNIT_TEST_SUITE`. It must fail before the fix and pass after.
   Before the fix, `get_dimension(2)` doesn't throw, which shows up as an assertion
   failure, or as an ASan report on an `--enable-asan` build.
2. Repro step 2 from BUG-008 (a 1-D variable whose DMR++ has
   `chunkDimensionSizes="10 10"`) should now end in `BESInternalError` instead of
   undefined behavior. This is a manual check. Adding it as a baseline test belongs
   with FR-012's malformed-DMR++ tests, because the remaining unchecked `chunk_origin`
   indexing can still misbehave first on some paths.
3. `make check` in `modules/dmrpp_module` passes, and `make distcheck` passes.

## Out of scope

- Checking that the chunk rank matches the array rank, that `chunkPositionInArray`
  has the right length, and that bounds hold on `chunk_origin[dim]` and
  `chunk_shape[dim]`. That is FR-012 (Proposed) and
  `plans/dmrpp-malformed-chunk-layout-validation-plan.md`.
- Direct-IO buffer-overrun checks (FR-012 case (b)).
