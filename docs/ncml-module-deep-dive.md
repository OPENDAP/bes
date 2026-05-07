# NcML Module Deep Dive

## Scope

This document is a code-oriented deep dive on `modules/ncml_module`. It focuses on the implementation in that directory and ignores the test layout and test differences from other modules, per request.

Primary external behavior reference:

- Hyrax Guide, Appendix D: Aggregation, especially `10.D.1 The NcML Module` through `10.D.8 Grid Metadata Tutorial`: <https://opendap.github.io/hyrax_guide/Master_Hyrax_Guide.html#_aggregation>

Primary code reference:

- `modules/ncml_module`

## Executive Summary

The NcML module is not just a file reader. It is a virtual dataset transformation layer that sits inside BES and rewrites DDS/DataDDS/DMR responses by parsing an NcML document and applying mutations to an in-memory libdap model.

At a high level, it supports four broad jobs:

1. Wrap a local dataset and alter metadata.
2. Create synthetic variables and purely virtual datasets.
3. Remove or rename variables, dimensions, and attributes.
4. Build aggregated virtual datasets, primarily `union`, `joinNew`, and `joinExisting`.

The implementation is mature but uneven. The core parse and transformation model is coherent, and the aggregation code is substantially more capable than the old `README` suggests. At the same time, some public-facing behavior descriptions are stale, some attributes are parsed but not actually acted on, and a few configuration hooks appear dead or partially retired.

## Intended Behavior Versus Current Code

The Hyrax Guide describes the module as a subset of NcML 2.2 plus OPeNDAP extensions. The code matches that overall framing.

The clearest implementation-backed behaviors are:

- wrapping an underlying dataset with `<netcdf location="...">`,
- reading metadata from the wrapped dataset and then mutating it,
- creating virtual datasets with no wrapped source,
- adding new scalar, array, and structure variables,
- adding nested attribute containers and `OtherXML` attributes,
- directory-driven aggregation membership using `<scan>`,
- aggregation types `union`, `joinNew`, and `joinExisting`.

The code also makes several limits explicit:

- unsupported NcML attributes often cause parse errors instead of being ignored,
- `forecastModelRunCollection` and related FMRC forms are rejected,
- changing values of existing variables is rejected,
- aggregation ordering matters because aggregation is processed inline, not as a distinct preprocessing phase.

One important documentation drift is that the old `modules/ncml_module/README` still says only `union` and `joinNew` are implemented, while the current code has full `joinExisting` classes in the build and dispatch path.

## Module Entry Points

### BES module registration

`NCMLModule.cc` registers the module with BES by:

- installing `NCMLRequestHandler`,
- enabling the normal DAP service hooks,
- ensuring `catalog` catalog and container storage are available,
- registering debug support.

That means NcML files behave like normal BES containers once the type match routes a `.ncml` resource into this module.

### Request handling

`NCMLRequestHandler.cc` is the runtime entry point for actual requests. It registers builders for:

- DAS
- DDS
- DAP2 data
- DMR
- DAP4 data via the DMR path
- help
- version

The request handler does not parse NcML directly. Instead, it builds a `DDSLoader`, creates an `NCMLParser`, parses the NcML file, and then maps the result into the requested response type.

Important detail:

- DMR support is not native. The code first builds a DDS-like structure and then converts that into a DMR using `D4BaseTypeFactory` and `DMR::build_using_dds()`.

That is a pragmatic design, but it also means DAP4 behavior depends on the fidelity of the DDS-to-DMR conversion path, not on a DAP4-native NcML model.

## Core Architecture

### `NCMLParser`

`NCMLParser` is the central coordinator. It is a SAX-driven parser over the NcML XML document.

Its main responsibilities are:

- maintain the current dataset, variable, scope, and attribute table,
- lazily load wrapped datasets only when needed,
- instantiate concrete `NCMLElement` handlers,
- apply mutations directly to libdap DDS/DataDDS objects as parsing proceeds,
- hand back the transformed response object.

The parser is built around a few key ideas:

- `NCMLElement` subclasses implement each supported NcML element.
- A scope stack tracks dataset, variable, and attribute-container context.
- Attribute table access is lazy through `AttrTableLazyPtr`, so aggregations that do not need metadata do not automatically force all underlying DDS loads.
- Element processing is inline and stateful, not a separate parse-tree build followed by execution.

That last point matters. The guide explicitly notes that Hyrax does not process `<aggregation>` before other elements, and the code reflects that. Ordering is therefore part of the effective language semantics.

### `DDSLoader`

`DDSLoader` is the bridge back into BES. It temporarily hijacks an existing `BESDataHandlerInterface`, swaps in a synthetic container for the wrapped dataset, runs the normal underlying request handler, and restores the original BES state afterward.

This is a clever reuse of the existing BES dispatch pipeline, but it has consequences:

- NcML depends on the underlying handler being correctly configured for every wrapped source type.
- Nested loads are operationally coupled to BES container storage and handler registration.
- A large amount of exception-safety code exists just to restore hijacked request state cleanly.

## Main Data Model

### `NetcdfElement`

`NetcdfElement` represents a `<netcdf>` block. In practice this is the unit of lexical scope for:

- wrapped dataset identity,
- local dimensions,
- current aggregation child,
- deferred validation of newly created variables.

It can work in three modes:

- root dataset that borrows the BES response object,
- nested aggregation member that owns its own response object,
- purely virtual dataset with no `location`.

The class lazily loads the wrapped dataset on first DDS access.

### `NCMLElement` subclasses

The module implements most behavior through element-specific classes:

- `ReadMetadataElement`
- `ExplicitElement`
- `DimensionElement`
- `VariableElement`
- `ValuesElement`
- `AttributeElement`
- `RemoveElement`
- `AggregationElement`
- `ScanElement`
- `VariableAggElement`

This keeps the parser core readable, but it spreads semantics across many files. For maintenance, you usually need both `NCMLParser` and the element class open at the same time.

## Element Semantics in the Code

### `<readMetadata>`

`ReadMetadataElement` is mostly a directive marker. It enforces that only one metadata directive is used per dataset. The actual metadata load still happens lazily through DDS access.

### `<explicit>`

`ExplicitElement` clears inherited metadata from the current dataset DDS. This is effectively a "start from a blank metadata view" operation on an otherwise wrapped dataset.

### `<dimension>`

`DimensionElement` creates name-to-length mappings used for array shape resolution and aggregation dimension handling.

Notable constraints in code:

- dimensions are only supported as direct children of `<netcdf>`,
- unsupported NcML attributes such as `isUnlimited`, `isShared`, and `isVariableLength` are treated as errors,
- rename support exists through `orgName`, but it works by rewriting array dimension names in variables already present in the current DDS.

### `<variable>` and `<values>`

`VariableElement` handles four distinct cases:

- refer to an existing variable,
- create a new variable,
- rename an existing variable using `orgName`,
- descend into a constructor variable such as a `Structure`.

`ValuesElement` fills new variables with explicit or autogenerated values. One hard-coded restriction is important:

- the module refuses to change the values of an existing variable.

For newly created variables, value assignment may be deferred until `</netcdf>` closes. That is how placeholder variables for aggregation coordinate axes are allowed to exist temporarily without immediate data.

### `<attribute>`

`AttributeElement` is one of the richer handlers.

It supports:

- add/modify/rename atomic attributes,
- nested attribute containers via `type="Structure"`,
- DAP2 scalar and vector types,
- `OtherXML`, which captures arbitrary XML and injects it into the DDX-style attribute model.

`OtherXML` is handled by a dedicated proxy SAX parser in `OtherXMLParser.*`. That is a real extension beyond basic NcML and is clearly intentional, not accidental.

### `<remove>`

`RemoveElement` can remove:

- attributes,
- variables,
- dimensions.

Dimension removal is more invasive than a metadata-only operation because it may rename array dimensions to empty names or remove matching top-level variables and then scrub array dimension names across the current DDS.

## Aggregation Architecture

### High-level shape

`AggregationElement` owns the aggregation model for a dataset. It stores:

- aggregation type,
- aggregation dimension name,
- explicit child datasets,
- `scan` elements,
- optional `variableAgg` declarations.

Processing happens when the `<aggregation>` element closes, not when the parent dataset closes. Additional post-processing occurs again when the enclosing `<netcdf>` closes via `processParentDatasetComplete()`, mainly for grid/map cleanup.

### Supported aggregation types

The code supports:

- `union`
- `joinNew`
- `joinExisting`

The code explicitly rejects:

- `forecastModelRunCollection`
- `forecastModelSingleRunCollection`

### `union`

`processUnion()`:

- merges dimensions across member datasets,
- loads member DDS objects,
- unions attributes and top-level variables into the parent dataset DDS,
- preserves "first one wins" behavior for variable and attribute name collisions.

This is metadata-heavy and necessarily eager. There is no meaningful lazy path for union metadata assembly because the output structure depends on the complete member set.

### `joinNew`

`processJoinNew()`:

- expands `scan` elements into concrete child datasets,
- merges dimensions,
- creates a new outer dimension named by `dimName`,
- uses the first dataset as the template DDS,
- creates aggregation variables for the declared `variableAgg` names,
- unions non-aggregated variables from the template dataset.

The actual data path is lazy. Aggregated arrays and grids are represented by subclasses such as:

- `ArrayAggregateOnOuterDimension`
- `GridAggregateOnOuterDimension`

Those classes aggregate member data at read/serialize time rather than materializing the entire dataset during parse.

### `joinExisting`

`joinExisting` is more complex than `joinNew` because the joined dimension already exists in the member datasets and the final output dimension length is the sum of the member lengths.

The code path does the following:

- expands `scan` membership if needed,
- builds an `AggMemberDataset` list,
- determines member dimension sizes using one of:
  - explicit `ncoords`,
  - dimension cache files,
  - slow DDS inspection,
- creates the post-aggregation dimension in the output scope,
- decides which variables to aggregate, either from explicit `variableAgg` declarations or by scanning the template DDS,
- builds join-existing aggregation variable wrappers,
- unions required non-aggregated variables, typically coordinate variables.

The actual data aggregation is delegated to:

- `ArrayJoinExistingAggregation`
- `GridJoinExistingAggregation`

This is the strongest evidence that `joinExisting` is not an abandoned stub. It has dedicated runtime classes, dimension caching support, and selection logic.

### `scan`

`ScanElement` turns a directory listing into explicit `<netcdf>` members.

Supported filters and behaviors in the code include:

- `location`
- `suffix`
- `regExp`
- `subdirs`
- `olderThan`
- `dateFormatMark`
- `ncoords`

It uses ICU date formatting support to derive time-like `coordValue` strings when `dateFormatMark` is present, and it sorts results:

- by location if no date extraction is used,
- by generated `coordValue` if date extraction is used.

This is one of the more operationally important parts of the module because it turns static NcML into evolving aggregations over on-disk collections.

### Dimension caching

`AggMemberDatasetDimensionCache` exists specifically to avoid repeated expensive metadata loads for `joinExisting`.

The idea is simple:

- on first use, inspect member datasets and cache dimension sizes,
- on later use, read the small cache files instead of reloading every member DDS.

That matches the guide's discussion of the NcML dimension cache and is one of the few performance-focused pieces in the module.

## Lazy Versus Eager Behavior

A useful mental model is:

- parse-time edits are mostly eager mutations to a DDS/DataDDS model,
- wrapped source loading is lazy,
- union metadata assembly is effectively eager,
- join aggregation data assembly is lazy until read/serialize.

This split explains several implementation decisions:

- `NetcdfElement` delays `loadLocation()`,
- `AttrTableLazyPtr` delays metadata table access,
- `AggMemberDatasetUsingLocationRef` delays member dataset loading,
- aggregation array/grid subclasses override `read()` and `serialize()`.

This is a good fit for large aggregations, but it also means failures can be delayed from parse time to read time.

## DAP2 and DAP4 Behavior

For DAP2-style requests, the module works directly with DDS/DataDDS and then synthesizes DAS output from the transformed DDS.

For DAP4:

- the request handler still builds a DDS-like structure first,
- then converts it to DMR,
- then applies DAP4 constraints/functions through the BES response object.

Two consequences follow:

1. The module is fundamentally DDS-centric, even when serving DMR/DAP4.
2. Any mismatch between DDS semantics and DMR semantics will show up here as conversion risk, not as an isolated DAP4 parser issue.

## Configuration and Runtime Dependencies

The module depends on more than just its own code.

Runtime prerequisites include:

- BES core request/response infrastructure,
- libdap DDS/DataDDS/DMR classes,
- libxml2 SAX parsing,
- ICU for `scan@dateFormatMark`,
- the underlying handlers for wrapped datasets such as netCDF, HDF4, HDF5, DMR++, or gateway-backed resources,
- BES container storage named `catalog`, and sometimes `gateway`.

Configured knobs visible in code or shipped config include:

- `NCML.GlobalAttributesContainerName`
- `NCML.DimensionCache.directory`
- `NCML.DimensionCache.prefix`
- `NCML.DimensionCache.size`

There is also dead or partially retired configuration history:

- `NCML.TempDirectory` is referenced only inside disabled `#if 0` code in `NCMLModule.cc`.

## Notable Drift, Gaps, and Risks

### Stale documentation inside the module

The old `README` materially understates current capability by describing only `union` and `joinNew`, while the codebase contains active `joinExisting` support.

### Parsed-but-unused attributes

`AggregationElement` stores `recheckEvery`, but the current implementation does not appear to use it to trigger re-scan or refresh behavior.

### Unsupported attributes are hard failures

Several schema attributes are accepted syntactically and then rejected semantically:

- `netcdf@enhance`
- `netcdf@addRecords`
- `netcdf@fmrcDefinition`
- various `dimension` flags

That keeps behavior explicit, but it also means compatibility with broader NcML examples is narrower than the element names alone suggest.

### Local-only intent versus gateway-aware code

Older documentation says this module only handles local datasets. The implementation in `DDSLoader.cc` now checks for `http://` and `https://` locations and routes them through `gateway` container storage if available.

That means one of two things is true:

- remote wrapping is now intentionally supported through gateway-backed handlers, or
- this is an opportunistic implementation path that outgrew the original documentation but was never fully documented as supported behavior.

Either way, the code and the older prose no longer agree.

### DMR is derived, not native

The DAP4 path depends on converting a DDS-based transformed view into DMR. That is maintainable, but it is not the same as having a first-class DAP4-native internal model.

### Custom ownership model increases maintenance cost

The module uses:

- custom ref-counted objects (`RCObject`, `RCPtr`, `WeakRCPtr`),
- manual `ref()` / `unref()` patterns,
- raw pointers mixed with `unique_ptr`,
- BES-owned response objects and temporarily hijacked DHI state.

This was understandable in the period the code was written, but it raises the cost of safe change, especially around aggregation cloning and nested dataset lifetimes.

### Ordering-sensitive semantics

Because aggregation is processed inline, authoring order in NcML can affect behavior. That is documented, but it still makes the module harder to reason about than a normalize-then-execute pipeline would.

## Practical Reading Order for Future Work

If you need to change behavior in this module, the shortest useful reading order is:

1. `NCMLRequestHandler.cc`
2. `NCMLParser.h` and `NCMLParser.cc`
3. `NetcdfElement.*`
4. `VariableElement.*`, `AttributeElement.*`, `ValuesElement.*`, `RemoveElement.*`, `DimensionElement.*`
5. `AggregationElement.*`
6. `ArrayAggregationBase.*`, `GridAggregationBase.*`, and the concrete join classes
7. `DDSLoader.*`
8. `AggregationUtil.*`

For performance work, focus on:

- `DDSLoader.*`
- `AggMemberDataset*`
- `ScanElement.*`
- `AggMemberDatasetDimensionCache.*`
- aggregation `read()` and `serialize()` methods

For behavioral work, focus on:

- `NCMLParser.*`
- the relevant `NCMLElement` subclass
- `NCMLUtil.*`

## Bottom Line

`modules/ncml_module` is a DDS-centric virtual dataset engine embedded in BES. It uses NcML as a control language, but the real design center is "load an existing BES-readable dataset, mutate its libdap representation, and optionally replace selected variables with lazy aggregation objects."

That design has held up reasonably well. The code is more capable than some of its local documentation suggests, especially around `joinExisting`, `scan`, and lazy aggregation reads. The main weaknesses are implementation drift, older ownership patterns, and the fact that some advertised schema surface is deliberately unsupported or only partially wired through.
