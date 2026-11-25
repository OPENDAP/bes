# DMR++ XML Format Specification

**Status:** Draft 0.1 (17 November 2026). For internal review. Not normative yet.

> **How this draft was written.** The element and attribute inventory comes from the 186 `.dmrpp` example documents in `modules/dmrpp_module/data/{buffer_chunk, deflate_twice, dmrpp, fill_value, h4_dmrpp, s3, string_arrays}` of the `bes` repository (working copy `bes-tmp`). Where the examples alone did not show what a construct *means*, I read the reference implementation in the same repository: the reader `modules/dmrpp_module/DMZ.cc`, the writer `DmrppCommon.cc` / `DMRpp.cc`, the build tool helpers `build_dmrpp_util.cc` and `vlsa_util.cc`, and the chunk decoder `Chunk.cc`. Statements based on code are marked **[impl]** with the file and function. Anything I could not determine is marked **TODO (James)**. Nothing here is meant to be invented. If a statement is not marked and is not from the DAP4 specification, it comes straight from the examples.

---

## 1. Introduction

### 1.1 Purpose

A DMR++ document describes a dataset so that a server can read the dataset's values directly from the original data file (for example, an HDF5, netCDF-4 or HDF4 file kept locally or in object storage). The server does not need the format library that wrote the file.

A DMR++ is a DAP4 DMR (Dataset Metadata Response) with extra XML elements and attributes. These additions say *where* each variable's bytes are in the source file and *how* they are encoded. They are:

- the byte offset and length of each chunk or block,
- the chunk shape and each chunk's position in the array,
- the compression and filter pipeline,
- the byte order and fill value,
- small data values carried in the document itself.

### 1.2 Relationship to the DAP4 DMR

The DMR is defined in the DAP4 specification, Section 1.5, *DMR Declarations*: <https://opendap.github.io/dap4-specification/DAP4.html#_dmr_declarations>. This document does not repeat that material. In a DMR++:

- Every DMR element (`Dataset`, `Group`, `Dimension`, `Enumeration`, variable elements such as `Int32`, `Float64`, `String`, `Structure`, plus `Dim`, `Map` and `Attribute`) keeps the meaning DAP4 gives it.
- The DMR++ additions are elements and attributes in the `dmrpp` XML namespace (Section 2).
- DMR++ elements go in two places. Some are attributes on the `Dataset` element (Section 4). The rest are child elements of *variable* elements, where they describe that variable's storage (Sections 5 to 9).

From here on, "variable element" means any DAP4 variable declaration element: an atomic type (`Byte`, `Int8`, `UInt8`, `Int16`, `UInt16`, `Int32`, `UInt32`, `Int64`, `UInt64`, `Float32`, `Float64`, `String`, `URI`), `Enum`, `Opaque`, `Structure` or `Sequence`. A variable is an *array* when it has one or more `Dim` children (DAP4 §1.5.13) and a *scalar* otherwise.

### 1.3 Conventions

The keywords MUST, MUST NOT, SHOULD and MAY appear only where the reference reader enforces the behavior (it raises an error or ignores the input). Everything else describes what the example documents show.

> **TODO (James):** Decide whether this specification is *normative*, so that the reader must be brought into line with it, or *descriptive* of the current reader. That choice affects how strongly Section 12 (known inconsistencies) should be worded.

---

## 2. Namespace

### 2.1 Namespace URI and prefix

DMR++ elements and attributes are in this namespace:

```
http://xml.opendap.org/dap/dmrpp/1.0.0#
```

It is declared on the `Dataset` element with the prefix `dmrpp`:

```xml
<Dataset xmlns="http://xml.opendap.org/ns/DAP/4.0#"
         xmlns:dmrpp="http://xml.opendap.org/dap/dmrpp/1.0.0#"
         ...>
```

The writer emits exactly this URI and prefix **[impl: `DmrppCommon.cc`, `DmrppCommon::d_dmrpp_ns`, `d_ns_prefix`; `DMRpp.cc`, `DMRpp::print_dmrpp`]**.

### 2.2 The prefix is significant to the reference reader

The reference reader does not process XML namespaces. It matches the literal qualified names `dmrpp:chunks`, `dmrpp:href` and so on **[impl: `DMZ.cc`, comment in `DMZ::process_dataset`: "This code does not use namespaces. By default, we assume the DMR++ elements all use the namespace prefix 'dmrpp'"]**. Two consequences follow:

- A document that binds the DMR++ namespace to any other prefix will not be read correctly.
- The reader does not check the namespace URI. It does not even check that a declaration exists.

**Producers MUST use the prefix `dmrpp`.**

> **TODO (James):** Should the specification require the exact namespace URI? Two examples (`fill_value/FValue.h5.dmrpp` and `fill_value/FValue_chunk.h5.dmrpp`) use `https://xml.opendap.org/dap/dmrpp/1.0.0#`, with `https`. They also use `https` for the DAP4 namespace. These documents work only because the reader ignores namespace URIs.

### 2.3 Unprefixed attributes on DMR++ elements

Attributes on `dmrpp:` elements are written *without* a prefix (for example `<dmrpp:chunk offset="…" nBytes="…"/>`). Under XML Namespaces, unprefixed attributes are in no namespace. The one attribute that sometimes carries the prefix on a DMR++ element is `dmrpp:trust` on `dmrpp:chunk` and `dmrpp:block`; the reader accepts both `trust` and `dmrpp:trust` there (§6.3).

### 2.4 Exception: the `v` element

The child of `dmrpp:vlsa` is written `<v>`, with no prefix (§9.2). Because the document's default namespace is the DAP4 namespace, a namespace-aware parser puts `v` in the DAP4 namespace, not the DMR++ namespace. The reader looks for the literal name `v` **[impl: `DmrppNames.h`, `DMRPP_VLSA_VALUE_ELEMENT "v"`]**.

> **TODO (James):** Is `v` intended to be in the DMR++ namespace? If so, should the writer emit `dmrpp:v`, or should the specification record the unprefixed form as correct?

---

## 3. Document overview

The skeleton below shows where DMR++ content goes inside a DMR. DMR elements are given only as context.

```xml
<Dataset xmlns="http://xml.opendap.org/ns/DAP/4.0#"
         xmlns:dmrpp="http://xml.opendap.org/dap/dmrpp/1.0.0#"
         name="…" dapVersion="4.0" dmrVersion="1.0"
         dmrpp:href="…"            ← §4.1 (required)
         dmrpp:version="…"         ← §4.2 (optional)
         dmrpp:trust="true">       ← §4.3 (optional)

  <Dimension …/>  <Group …> … </Group>   (DAP4)

  <Float32 name="v1">                    (DAP4 variable element)
    <Dim …/>  <Attribute …/>             (DAP4)
    <dmrpp:chunks …>                     ← §5
      <dmrpp:chunkDimensionSizes>…</dmrpp:chunkDimensionSizes>  ← §5.3
      <dmrpp:chunk offset nBytes chunkPositionInArray [href]/> ← §6
      …   or   <dmrpp:block offset nBytes/> …                   ← §7
    </dmrpp:chunks>
  </Float32>

  <Int32 name="v2">
    <dmrpp:chunk offset nBytes/>         ← §6.5 (contiguous, without dmrpp:chunks)
  </Int32>

  <Int32 name="v3">
    <dmrpp:chunks fillValue="0"/>
    <dmrpp:compact>BASE64</dmrpp:compact>                       ← §8.1
  </Int32>

  <Float64 name="v4"> <dmrpp:missingdata>BASE64</dmrpp:missingdata> </Float64>  ← §8.2
  <Structure name="v5"> … <dmrpp:specialstructuredata>BASE64</…> </Structure>  ← §8.3

  <String name="v6"> <Dim …/>
    <dmrpp:chunks …> … </dmrpp:chunks>
    <dmrpp:FixedLengthStringArray string_length="N" pad="…"/>   ← §9.1
  </String>

  <String name="v7"> <Dim …/>
    <dmrpp:vlsa> <v>…</v> <v c="2">…</v> <v s="1024">…</v> </dmrpp:vlsa>  ← §9.2
  </String>
</Dataset>
```

### 3.1 DMR++ children of a variable element

The examples contain these combinations of DMR++ child elements under one variable element (counts are variable elements across the 183 well-formed examples):

| DMR++ children of the variable element | Variable types seen | Count |
|---|---|---|
| `dmrpp:chunks` | all numeric types, `Enum`, `String`, `Structure`; scalars and arrays | 1,982 |
| `dmrpp:chunks`, `dmrpp:FixedLengthStringArray` | `String` arrays | 23 |
| `dmrpp:chunk` (no `dmrpp:chunks`) | `Int16`, `Int32`, `Float32`, `Float64`; scalars and arrays | 24 |
| `dmrpp:compact` | `Int8`, `Int32`, `Int64`, `Float32`, `Float64`, `String` | 10 |
| `dmrpp:chunks`, `dmrpp:compact` | `Byte`, `Int8`, `Int32`, `Int64`, `UInt64`, `Float32`, `String` | 13 |
| `dmrpp:compact`, `dmrpp:FixedLengthStringArray` | `String` arrays | 4 |
| `dmrpp:chunks`, `dmrpp:compact`, `dmrpp:FixedLengthStringArray` | `String` arrays | 3 |
| `dmrpp:chunks`, `dmrpp:specialstructuredata` | `Structure` (scalar and array) | 5 |
| `dmrpp:missingdata` | `Float64` arrays | 2 |
| `dmrpp:vlsa` | `String` arrays | 8 |

In the examples, the DMR++ children always come after the variable's DAP4 children (`Dim`, `Map`, `Attribute`, and member variables of a `Structure`). The reader looks children up by name, so it does not depend on this order **[impl: `DMZ.cc`, `var_node.child("dmrpp:chunks")` etc.]**. Inside `dmrpp:chunks` the order does matter (§5.4).

> **TODO (James):** Should the specification *require* DMR++ children to come after the DAP4 children?

---

## 4. Extensions to the `Dataset` element

The `Dataset` element (DAP4 §1.5.7) takes these additional attributes.

### 4.1 `dmrpp:href` (required)

The location of the data file that all `dmrpp:chunk`, `dmrpp:block` and similar references point into, unless a reference has its own `href` (§6.4).

- **The reader requires it** and raises an error if it is missing ("DMR++ XML dataset element dmrpp:href is missing") **[impl: `DMZ::process_dataset`]**.
- **Value forms seen in the examples:**
  - A path with no URL scheme, relative or absolute: `data/dmrpp/t_int_scalar.h5` or `/data/string_arrays/t_string_compact.h5`.
  - An absolute `https://` URL: `https://s3.amazonaws.com/cloudydap/airs/AIRS_partial.h5`.
  - The placeholder token `OPeNDAP_DMRpp_DATA_ACCESS_URL` (3 examples). See §4.4.
- **How a value without a scheme is resolved [impl: `http/url_impl.cc`, `url::parse`]:** the value is treated as a path *inside the BES default catalog's root directory* (`BES.Catalog.catalog.RootDirectory`) even if it starts with `/`. It is then turned into a `file://` URL. Values containing `://` are used as given.

> **Note.** Two examples, `dmrpp/a2_local_twoD.h5.dmrpp` and `dmrpp/a3_local_twoD.h5.dmrpp`, have no `dmrpp:href` on `Dataset`. Instead, every `dmrpp:chunk` carries its own `href`. The current reader would reject both documents.
>
> **TODO (James):** Are these two documents still valid DMR++, or are they test artifacts? Should the specification allow `dmrpp:href` to be left out when every data reference has its own `href`?

### 4.2 `dmrpp:version` (optional)

The version of the software that wrote the DMR++. The writer sets it from its compile-time version string **[impl: `build_dmrpp_util.cc`, `dmrpp->set_version(CVER)`]**. Values seen: `3.20.10`, `3.20.13`, `3.21.0`, `3.21.1`, `3.22.0`. This attribute appears in 90 of the 186 examples.

**How it changes interpretation [impl: `DMZ::process_dataset`; `DmrppCommon::set_filter`]:** when `dmrpp:version` is *missing*, the reader treats the document as an "old style" DMR++. It then ignores the order of the tokens in `compressionType` (§5.2.1) and rebuilds the filter list in the fixed order `shuffle deflate fletcher32`, keeping only the filters that appear. Duplicate tokens are dropped in the rebuild. The exception is when the value contains `lzw` or `tiffpred`; then it is kept as written. When `dmrpp:version` is present, `compressionType` is used exactly as written.

> **TODO (James):** Please confirm that `dmrpp:version` is meant to be the BES/`build_dmrpp` release version and not a version of the DMR++ format. Is the "missing version means legacy filter order" rule something the specification should keep?

### 4.3 `dmrpp:trust` (optional)

A boolean flag. When the value is exactly `true`, the URL in `dmrpp:href` is marked *trusted*. Trusted URLs skip the server's `AllowedHosts` check **[impl: `DMZ::process_dataset`; `http/AllowedHosts.cc`: `isAllowed = candidate_url.is_trusted() || check(...)`]**. Any other value is treated as false. This attribute appears in one example (`dmrpp/trust_test.dmrpp`).

### 4.4 Placeholder hrefs and server-side substitution

Some DMR++ documents carry placeholder tokens instead of real data URLs. The server fills them in when it serves the document:

| Token | Where it is seen | Meaning **[impl]** |
|---|---|---|
| `OPeNDAP_DMRpp_DATA_ACCESS_URL` | `Dataset/@dmrpp:href` (3 examples) | Replaced with the data URL of the granule. |
| `OPeNDAP_DMRpp_MISSING_DATA_ACCESS_URL` | not seen in the examples | Replaced with the data URL plus the suffix `_mvs.h5`. This is a companion "missing variables" file (compare the chunk `href` values ending in `.h5_mvs.h5` in §6.4). |

**[impl: `modules/dmrpp_module/ngap_container/NgapOwnedContainer.cc`, `get_daac_content_filters`; `modules/s3_reader/S3Container.cc`]** The substitution is plain text replacement of the string `href="<TOKEN>"`. The replacement text also adds the attributes `dmrpp:s3="…"`, `dmrpp:s3credentials="…"` and `dmrpp:trust="true"`. As a result, a document served this way can contain `dmrpp:s3` and `dmrpp:s3credentials`. These attributes appear in no example, and the DMR++ reader ignores them.

> **TODO (James):** Should the placeholder tokens, and the `dmrpp:s3` and `dmrpp:s3credentials` attributes, be part of the specification? Or are they a deployment detail of the NGAP and S3 containers?

### 4.5 Other `Dataset` attributes seen

`name`, `dapVersion`, `dmrVersion` and the namespace declarations are DAP4. One example (`string_arrays/t_string_compact.h5.dmrpp`) has `xml:base`. The reader passes it through as the request's XML base **[impl: `DMZ::process_dataset`]**. The reader always sets the DMR version to `1.0`, whatever value the document gives **[impl: same function]**. One example has `dmrVersion="removed"` (`dmrpp/utf8_vlen_str.h5.dmrpp`).

### 4.6 Other `Dataset`-level content (not in the `dmrpp` namespace)

Recent examples include an ordinary DAP4 `Attribute` container named `build_dmrpp_metadata` directly under `Dataset`. It holds `String` attributes such as `created`, `build_dmrpp`, `bes`, `libdap`, `configuration` and `invocation`. This is provenance written by `build_dmrpp`. It is ordinary DMR content and is not part of the DMR++ vocabulary.

> **TODO (James):** Should the `build_dmrpp_metadata` attribute container be described in this specification, for example as recommended provenance?

---

## 5. `dmrpp:chunks`

### 5.1 Purpose and placement

`dmrpp:chunks` describes how one variable's data is stored in the data file. It appears **at most once**, as a child of a variable element. It is seen under all numeric atomic types, `Enum`, `String` and `Structure`, both scalar and array.

The element has three roles:

1. It carries per-variable storage properties as attributes (§5.2).
2. It contains the chunk shape (§5.3).
3. It contains the list of data references (`dmrpp:chunk`, §6, or `dmrpp:block`, §7).

It may also be **empty** (no children). In that case it carries only attributes such as `fillValue` and `byteOrder`. This form is seen next to `dmrpp:compact` and `dmrpp:specialstructuredata`, and for variables made entirely of fill values (§10).

### 5.2 Attributes

| Attribute | Seen in examples | Value |
|---|---|---|
| `compressionType` | yes (1,617) | Space-separated list of filter names (§5.2.1) |
| `deflateLevel` | yes (24) | Space-separated list of integers (§5.2.2) |
| `fillValue` | yes (180) | Text (§5.2.3) |
| `byteOrder` | yes (155) | `LE` or `BE` (§5.2.4) |
| `structOffset` | yes (25) | Space-separated list of integers (§5.2.5) |
| `LBChunk` | **no** | `true` (§5.2.6) |
| `DIO` | **no** | `off` (§5.2.7) |

All attributes are optional. The reader ignores attributes it does not recognize. Two misspelled attributes appear in the examples (§12).

#### 5.2.1 `compressionType`

The filters applied to each chunk's bytes. The value is a list of tokens separated by single spaces. The tokens are listed in the **order the filters were applied when the data was written**. When reading, the filters are undone in **reverse** order **[impl: `Chunk::filter_chunk` loops over the tokens with `rbegin()`/`rend()`]**. Example: `shuffle deflate` means the data was shuffled and then deflated, so the reader inflates first and then unshuffles. (For documents without `dmrpp:version`, see §4.2.)

Values seen in the examples:

```
shuffle deflate          deflate                deflate shuffle
shuffle                  deflate fletcher32     shuffle deflate fletcher32
deflate deflate          shuffle deflate deflate
fletcher32               fletcher32 shuffle deflate deflate
shuffle deflate deflate fletcher32
```

Tokens seen, and how the reader handles them **[impl: `Chunk::filter_chunk`]**:

| Token | Meaning |
|---|---|
| `deflate` | zlib/deflate compression. It may appear more than once (data deflated more than once). **Repeated `deflate` tokens MUST be next to each other**; otherwise the reader raises "The deflate filters must be adjacent to each other". |
| `shuffle` | HDF5 byte shuffle. It is undone using the variable's element size. |
| `fletcher32` | A Fletcher-32 checksum appended to the chunk. The reader removes the last 4 bytes of the chunk. Checking the checksum is turned off at compile time (`ACTUALLY_USE_FLETCHER32_CHECKSUM`). |

The reader in this branch also accepts two tokens that appear in **no example**: `lzw` (TIFF LZW) and `tiffpred:<predictor>:<samples-per-pixel>:<elements-per-row>`, a TIFF predictor where the predictor is `2` or `3`. For example, `tiffpred:2:3:1536`.

> **TODO (James):** Are `lzw` and `tiffpred:…` meant for this version of the specification? They look like recent work (GeoTIFF support?).

If `compressionType` is missing, the chunks are not filtered.

#### 5.2.2 `deflateLevel`

The deflate compression level or levels, as a space-separated list of integers. The reader takes the list to have one entry per `deflate` token **[impl: `DMZ::process_chunks`; `DmrppCommon::print_chunks_element` writes one level per deflate]**. Values seen: `1`, `4`, `9` (always one value in the examples).

This attribute is **not needed to decode** the data. The reader uses it only to decide whether the variable can use "direct I/O", which passes chunks through unchanged to a netCDF-4 response **[impl: `DMZ::set_up_direct_io_flag_phase_1/2`]**. Direct I/O also requires that `compressionType` contains `deflate`, that `byteOrder="LE"`, that the variable is an integer or floating-point array, and that the filter list does not end with `fletcher32`.

#### 5.2.3 `fillValue`

The value used for array elements, or for whole chunks, that have no stored data (§10). How the text is formatted depends on the variable type:

- **Numeric and `Enum` variables:** a decimal number. Seen: `0`, `-99`, `-999`, `-2147483647`, `255`, `65535`, `9999`, `-9999`, `-9999.9`, `-9999.000000`, `-99999.000000`, `9.96921e+36`. The number format is not fixed: both `%f`-style and `%g`-style text occur.
- **`String` variables:** the literal string. Seen: the empty string, `a`, `aa`.
- **`Structure` variables:** one value for each numeric member, in member order, separated by single spaces. Array members are expanded element by element. For example, `-9999.9 32767 32766 32765 999.9 899.9` **[impl: `build_dmrpp_util.cc`, `get_compound_base_fill_value_as_string` caller]**.
- **Sentinel values** that mark an unsupported variable **[impl: `DMZ.cc`, `flagged_as_unsupported_type`]**:

  | Value | Reader behavior |
  |---|---|
  | `unsupported-string` | An older, ambiguous marker. It is accepted, and treated as an empty fill value, when the `String` variable is a scalar or has a `dmrpp:FixedLengthStringArray` sibling. Otherwise the reader raises an error. Seen in 22 examples. |
  | `unsupported-variable-length-string` | The reader raises an error (variable not readable). Not seen. |
  | `unsupported-array` | The reader raises an error. Not seen. |
  | `unsupported-compound` | The reader raises an error. Not seen. |

The reader does not allow `fillValue` on `URI`, `Sequence` or DAP2 `Grid` variables **[impl: `DMZ::process_chunks`]**.

> **TODO (James):** Please confirm the intended text encoding of numeric fill values. Should they round-trip exactly to the stored binary value?

#### 5.2.4 `byteOrder`

The byte order of the stored values: `LE` (little-endian) or `BE` (big-endian). Any other non-empty value is an error **[impl: `DmrppCommon::ingest_byte_order`]**. The reader swaps bytes when this order differs from the host's. It applies to every `dmrpp:chunk` and `dmrpp:block` of the variable.

`BE` appears mostly in the HDF4 examples (`h4_dmrpp/`).

If `byteOrder` is missing (it often is in older examples, and for `String` and some `Structure` variables), the reader does no byte swapping.

> **TODO (James):** What should a reader assume when `byteOrder` is missing for a multi-byte numeric type? Is leaving it out an error, or does it mean "native/irrelevant"?

#### 5.2.5 `structOffset`

Used only for `Structure` variables (HDF5 compound types). It lists the byte offset of every member *except the first*, followed by the **total size in bytes of one structure element** **[impl: `build_dmrpp_util.cc`, `obtain_structure_offset`]**. This lets the reader unpack padded (non-packed) records.

Example (`dmrpp/compound_two_chunks_naligned.h5.dmrpp`):

```xml
<Structure name="comp_array">
    <Float64 name="a_name"/>         <!-- offset 0 (implicit) -->
    <Int16   name="b_name"/>         <!-- offset 8 -->
    <Float32 name="c_name"> <Dim size="2"/> </Float32>  <!-- offset 12 -->
    <Dim size="4"/>
    <dmrpp:chunks fillValue="0" byteOrder="LE" structOffset="8 12 24">   <!-- record size 24 -->
        <dmrpp:chunkDimensionSizes>2</dmrpp:chunkDimensionSizes>
        <dmrpp:chunk offset="3496" nBytes="48" chunkPositionInArray="[0]"/>
        <dmrpp:chunk offset="3544" nBytes="48" chunkPositionInArray="[2]"/>
    </dmrpp:chunks>
</Structure>
```

The writer emits `structOffset` only when the variable has at least one data reference **[impl: `DmrppCommon::print_chunks_element`]**.

#### 5.2.6 `LBChunk` (not in examples)

When `true`, each chunk of this variable is made of several HDF4 *linked blocks*. Each block is listed as its own `dmrpp:chunk` carrying a `LinkedBlockIndex` attribute (§6.3). The reader joins the consecutive `dmrpp:chunk` elements that start with `LinkedBlockIndex="0"` into one logical chunk **[impl: `DMZ::process_chunks`, `add_mblock_index`, `process_multi_blocks_chunk`]**.

> **TODO (James):** None of the examples show this form. Please provide or point to an example so this section can be checked.

#### 5.2.7 `DIO` (not in examples)

When `off`, direct I/O (§5.2.2) is turned off for this variable. The writer emits it only when `compressionType` is present **[impl: `DmrppCommon::print_chunks_element`; `DMZ::set_up_direct_io_flag_phase_2`]**.

### 5.3 `dmrpp:chunkDimensionSizes`

The shape of one chunk in array elements, as a list of non-negative integers separated by single spaces. There is one integer per array dimension, in the same order as the variable's `Dim` elements. Examples: `50 50`, `1 90 180`, `4`.

- It appears at most once per `dmrpp:chunks`. If there are more, the reader uses only the first in some code paths **[impl: `DMZ::process_cds_node` vs. `set_up_direct_io_flag_phase_2`]**.
- It is present when the variable uses **chunked** storage. It is missing for **contiguous** storage (a single data reference covering the whole variable). That includes scalars.
- The element has text content only. It has no attributes.

### 5.4 Content model and order

Based on the examples and the reader **[impl: `DMZ::process_chunks`; comment "A 'dmrpp:chunks' node has a chunkDimensionSizes node and then one or more chunks nodes, and they have to be in that order"]**:

```
dmrpp:chunks ::= dmrpp:chunkDimensionSizes?  ( dmrpp:chunk* | dmrpp:block* )
```

- `dmrpp:chunkDimensionSizes`, if present, comes first.
- `dmrpp:chunk` and `dmrpp:block` **MUST NOT** both appear under one `dmrpp:chunks` **[impl: comment "They are mutual exclusive"]**.
- If there are no data references, all values are fill values (§10).

---

## 6. `dmrpp:chunk`

### 6.1 Purpose

A reference to one run of bytes in a data file: the stored, and possibly filtered, bytes of one chunk.

### 6.2 Placement

- Normally a child of `dmrpp:chunks` (4,573 occurrences in the examples).
- It may also be a **direct child of a variable element** with no `dmrpp:chunks` element (24 occurrences). See §6.5.

### 6.3 Attributes

| Attribute | Required | Seen | Value |
|---|---|---|---|
| `offset` | **yes** | yes | Byte offset of the chunk in the data file. A non-negative integer (unsigned 64-bit in the reader). |
| `nBytes` | **yes** | yes | Length of the stored (filtered) chunk in bytes. A non-negative integer (unsigned 64-bit). |
| `chunkPositionInArray` | for chunked storage | yes | Element index of the chunk's first element (§6.3.1). |
| `href` | no | yes (14) | Overrides `Dataset/@dmrpp:href` for this chunk (§6.4). |
| `trust` or `dmrpp:trust` | no | **no** | `true` marks this chunk's `href` as trusted (as §4.3). |
| `fm` | no | **no** | Filter mask (§6.3.2). |
| `LinkedBlockIndex` | no | **no** | Order of this block within a linked-block chunk (§5.2.6). |

The reader raises an error if `offset` or `nBytes` is missing ("Both size and offset are required for a chunk node") **[impl: `DMZ::process_chunk`]**. In the examples, attributes appear in any order (for example `nBytes` before `offset`).

#### 6.3.1 `chunkPositionInArray`

The position of the chunk in the variable's array, written as **element indices** of the chunk's first element. The format is a bracketed, comma-separated list with no spaces and one integer per dimension: `[0]`, `[0,50]`, `[12,90,180]`. Each index is a multiple of the matching value in `dmrpp:chunkDimensionSizes`.

In the examples, `chunkPositionInArray` is missing exactly when the parent `dmrpp:chunks` has no `dmrpp:chunkDimensionSizes` (contiguous storage), and when `dmrpp:chunk` is a direct child of the variable (§6.5).

Chunks may be listed in any order. The order in the document does not need to match the order in the file. See `buffer_chunk/simple.dmrpp`, where the `offset` values do not increase.

Chunks at the edge of an array may extend past the array bounds. Examples whose names contain `uneven` cover this case (e.g., `dmrpp/chunked_twoD_uneven.h5.dmrpp`).

> **TODO (James):** Please confirm the rule for edge chunks. Is the stored chunk always the full `chunkDimensionSizes` shape, as in HDF5?

#### 6.3.2 `fm` (not in examples)

The writer emits `fm` only for chunked storage, and only when the value is non-zero. The value is the HDF5 per-chunk *filter mask* from `H5Dchunk_iter` **[impl: `DmrppCommon::print_chunks_element`; `build_dmrpp_util.cc`, `chunk_cb`]**. The reader stores it, and the direct I/O path passes it on **[impl: `DmrppArray.cc`]**. The general chunk decoding path does not use it.

> **TODO (James):** Please confirm the meaning of `fm` (a bit set means "filter *n* was skipped for this chunk", as in HDF5). Should the decoder honor it?

### 6.4 Per-chunk `href`

When present, `href` gives the file this chunk is read from. It is resolved the same way as `dmrpp:href` (§4.1). Uses seen in the examples:

- Data held in one-file-per-chunk "byte stream" files (`data/dmrpp/bstream_<md5>.bs`, with `offset="0"`), in `dmrpp/a3_local_twoD.h5.dmrpp`.
- Chunks taken from a *missing-variables* companion file whose name ends in `_mvs.h5` (`dmrpp/grid_1_2d.h5.dmrpp`, `dmrpp/grid_2_2d.h5.dmrpp`). In these documents, most variables use the `Dataset` `href`, and a few use the `_mvs.h5` file.

The attribute is written with no prefix (`href`), not `dmrpp:href`. The DMR++ writer in `DmrppCommon.cc` never emits a per-chunk `href`, so these values come from other tools or processes.

> **TODO (James):** Which tool writes per-chunk `href` values (e.g., the tool that merges `_mvs.h5` content)? Please also describe the `_mvs.h5` "missing variables" file, which this specification can only mention.

### 6.5 `dmrpp:chunk` as a direct child of a variable (contiguous form)

A variable element may contain a single `dmrpp:chunk` with no `dmrpp:chunks` wrapper:

```xml
<Int16 name="d16_1">
  <Dim size="2"/>
  …
  <dmrpp:chunk nBytes="4" offset="2216"/>
</Int16>
```

The reader treats this as contiguous storage: one data reference for the whole variable, using the default byte order and no filters **[impl: `DMZ.cc`, "Contiguous data" branch near `var_node.child("dmrpp:chunk")`]**. This form appears in older, hand-written examples (`dmrpp/d_int.h5.dmrpp`, `dmrpp/coads_climatology.dmrpp`, `dmrpp/http_*.dmrpp`, and the `s3/MERRA2_*` files). Newer writer output always wraps chunks in `dmrpp:chunks`.

> **TODO (James):** Is the direct-child form deprecated? If not, should a producer that uses it be able to state `byteOrder` some other way?

---

## 7. `dmrpp:block`

### 7.1 Purpose

A reference to one *linked block* of an HDF4 variable whose data is split into several non-adjacent byte runs. The variable's data is the **concatenation of all its blocks in document order** **[impl: `DMZ::process_block`, `block_count`; `DmrppCommon::print_chunks_element`]**.

### 7.2 Placement and attributes

A child of `dmrpp:chunks`. It cannot be mixed with `dmrpp:chunk` (§5.4). It appears in one example, `h4_dmrpp/SDS_unlimited_2.hdf.dmrpp`:

```xml
<dmrpp:chunks byteOrder="BE">
    <dmrpp:block offset="2776" nBytes="400"/>
    <dmrpp:block offset="3580" nBytes="40"/>
    <dmrpp:block offset="4426" nBytes="40"/>
</dmrpp:chunks>
```

| Attribute | Required | Seen | Value |
|---|---|---|---|
| `offset` | **yes** | yes | Byte offset in the data file |
| `nBytes` | **yes** | yes | Length in bytes |
| `href` | no | no | Overrides `Dataset/@dmrpp:href` |
| `trust` or `dmrpp:trust` | no | no | As in §6.3 |

The writer emits `dmrpp:block` only when there is more than one block. The reader also accepts a single block, because some older DMR++ files contain one **[impl: comment in `DMZ::process_chunks`]**.

---

## 8. Inline data elements

These elements carry a variable's values inside the DMR++ itself, encoded as text. In every case the reader does no byte swapping on the decoded bytes. It copies them straight into the variable's memory buffer **[impl: `val2buf` calls in `DMZ::process_compact`, `process_missing_data`, `process_special_structure_data_internal`]**. In the examples, the decoded numeric data is little-endian.

> **TODO (James):** What is the byte order of inline binary data? Is it fixed as little-endian, or is it "whatever the producing host used"? Is it governed by `dmrpp:chunks/@byteOrder` when present? The reader currently ignores `byteOrder` for inline data.

### 8.1 `dmrpp:compact`

**Purpose.** It holds all of a variable's values when the source used HDF5 *compact* storage, which keeps the raw data inside the object header so there is no offset to point to.

**Placement.** A child of a numeric, `Enum` or `String` variable element, scalar or array. Seen under `Byte`, `Int8`, `Int32`, `Int64`, `UInt64`, `Float32`, `Float64` and `String`. The reader rejects it under `Structure`, `Sequence` and `Grid` **[impl: `DMZ::process_compact`]**. It may come with an empty `dmrpp:chunks` that carries `fillValue` and/or `byteOrder`, and, for string arrays, with a `dmrpp:FixedLengthStringArray`.

**Attributes.** None.

**Content.** Base64 text of the variable's raw bytes. It is *not* compressed.

- Numeric and `Enum`: the values in row-major order, `width × N` bytes.
- Scalar `String`: the bytes of the string. For example, `QW4gSERGNSBzY2FsYXIgc3RyaW5n` decodes to `An HDF5 scalar string`.
- `String` array: only fixed-length string arrays are supported. They need a `dmrpp:FixedLengthStringArray` sibling (§9.1), and the content is the strings stored one after another, each `string_length` bytes. Compact variable-length string arrays make the reader raise "Variable Length Strings are not yet supported".

### 8.2 `dmrpp:missingdata`

**Purpose.** It holds the values of a variable whose data is *not stored* in the source file and was computed when the DMR++ was built. In the example, these are latitude and longitude arrays for an HDF-EOS2 grid (`h4_dmrpp/sinusoid_ll.hdf.dmrpp`).

**Placement.** A child of an array variable element, or of a scalar `Byte` **[impl: `DMZ::process_missing_data`]**. Seen under `Float64` arrays (2 occurrences).

**Attributes.** None.

**Content [impl: `DMZ::process_missing_data`]:**

- Array with more than one element: Base64 text of the **zlib-compressed** raw bytes of all elements, in row-major order.
- Array with exactly one element, or scalar `Byte`: Base64 text of the raw bytes, not compressed.

I checked this by hand: the first `missingdata` value in `sinusoid_ll.hdf.dmrpp` decodes and inflates to 128 bytes, which is 16 little-endian `Float64` values for a 4 × 4 grid.

### 8.3 `dmrpp:specialstructuredata`

**Purpose.** It holds all values of a `Structure` (an HDF5 compound) that has string members. Such values cannot be read from the file as fixed-size records **[impl: `DMZ::process_special_structure_data`]**.

**Placement.** A child of a `Structure` element, scalar or array. The examples pair it with an empty `<dmrpp:chunks fillValue="0"/>` sibling. Members must be numeric or `String` scalars or arrays. `URI`, `Enum`, `Opaque` and nested constructor types are rejected **[impl: `supported_special_structure_type_internal`]**. Subsetting (constraining) an array of such structures is not supported.

**Attributes.** None.

**Content.** Base64 text of a byte stream. For each structure element in row-major order, and then for each member in declaration order:

- **Numeric scalar or array member:** the raw bytes of the value(s), `width × count` bytes, with no separator.
- **`String` member:** the string's own **Base64** encoding (nested Base64), followed by a `;` (0x3B) terminator.
- **`String` array member:** each element as nested Base64 followed by `;`.

I checked this by hand. `gQQAAIXrVEJNakF5TXc9PTs=` (from `dmrpp/compound_simple_scalar_vlen_str.h5.dmrpp`, members `Int32 Orbit`, `Float32 Temperature`, `String year`) decodes to these bytes: `81 04 00 00` (Int32 1153, LE), then `85 EB 54 42` (Float32 53.23, LE), then `MjAyMw==;`, which is Base64 for `2023` followed by the terminator.

> **TODO (James):** The reader skips padding in this stream, so members are packed with no alignment. Please confirm that this is the intended layout.

---

## 9. String array descriptors

### 9.1 `dmrpp:FixedLengthStringArray`

**Purpose.** It marks a `String` array whose source values are fixed-length strings, and gives the length and padding scheme so the reader can cut the stored bytes into separate strings.

**Placement.** A child of a `String` array element. It is an empty element. It comes with `dmrpp:chunks` (chunked or contiguous), with `dmrpp:compact`, or with both. Seen 30 times.

**Attributes** (both seen in every occurrence):

| Attribute | Value |
|---|---|
| `string_length` | Length of each string in bytes, as a positive integer. Seen: `1`, `2`, `3`, `7`, `27`, `80`. |
| `pad` | The padding scheme. Seen: `null_term`, `null_pad`, `space_pad`. The reader also accepts `not_set` and raises an error for anything else **[impl: `DmrppArray.cc`, `str_to_pad_type`]**. |

The pad values follow the HDF5 string padding types:

- `null_term`: the string ends at the first NUL.
- `null_pad`: padded with NULs.
- `space_pad`: padded with spaces.

Values are trimmed according to the pad type **[impl: `DmrppArray::ingest_fixed_length_string`]**.

> **TODO (James):** Please confirm the exact trimming rules for each `pad` value.

The header comment in `DmrppArray.h` calls this element `dmrpp:fStringArray`. The reader and all the examples use `dmrpp:FixedLengthStringArray`.

### 9.2 `dmrpp:vlsa` and `v`

**Purpose.** "Variable-length string array." It holds the values of a `String` array whose source values are variable-length strings, written inline in the DMR++ because they cannot be read in place.

**Placement.** A child of a `String` array element. It is not seen with any other DMR++ sibling (8 occurrences). The reader requires the variable to be an array of `String` or `URI` **[impl: `DMZ::process_vlsa`]**.

**Attributes of `dmrpp:vlsa`.** None.

**Content.** One or more `v` elements (no prefix; see §2.4), in row-major element order.

**`v` element.** Its text content is one string value. Attributes **[impl: `vlsa_util.cc`, `write_value`, `read_value`, `read`; `DmrppNames.h`]**:

| Attribute | Meaning |
|---|---|
| `c` | Repeat count. The value is used for this many *consecutive* array elements (run-length encoding). Missing means 1. The writer emits it only when the count is greater than 1. |
| `s` | Present when the content is **compressed**. It gives the uncompressed length of the string in bytes. When `s` is present, the content is Base64 text of the zlib-compressed string. The writer compresses values longer than 512 bytes (`VLSA_VALUE_COMPRESSION_THRESHOLD`). The reader raises an error if the inflated size is not `s`. |

Examples (`string_arrays/vl_str_dup_array.h5.dmrpp`, `string_arrays/t_big_vl_str_dset2.h5.dmrpp`):

```xml
<dmrpp:vlsa>
    <v>apple</v>
    <v>banana</v>
    <v c="2">cherry</v>      <!-- elements 2 and 3 -->
    <v c="3">giraffe</v>     <!-- elements 4, 5, 6 -->
    <v>hippopotamus</v>
</dmrpp:vlsa>

<dmrpp:vlsa>
    <v s="1024">eJwVk0cSRCEI…</v>   <!-- 1024-byte string, zlib + Base64 -->
    <v>palm tree</v>
</dmrpp:vlsa>
```

`c` and `s` may appear together (`<v c="2" s="1024">…</v>`). The total of all repeat counts should equal the number of array elements.

> **TODO (James):** Should a reader treat a total that differs from the element count as an error?

---

## 10. Fill values and chunks that are not stored

**[impl: `DMZ.cc`, the chunk-processing block that calls `process_fill_value_chunks`]**

1. **Partly stored arrays.** Suppose `dmrpp:chunkDimensionSizes` is present, and the number of `dmrpp:chunk` elements is smaller than the number of chunks needed to tile the array. Then every chunk position with no `dmrpp:chunk` is filled with `fillValue`. Example: `buffer_chunk/simple.dmrpp` lists 7 of the 8 chunks of a 16-element `Byte` array. Chunk `[14]` is missing and is filled with `0`.
2. **No stored data.** If `dmrpp:chunks` has no `dmrpp:chunk` children, the whole variable is fill values. This applies whether or not `dmrpp:chunkDimensionSizes` is present (e.g., `fill_value/FValue_*.h5.dmrpp`, `dmrpp/nc4_unlimited_0.h5.dmrpp`).
3. For `Structure` variables, fill chunks are built with `structOffset` (the last value is the record size) and the per-member fill values (§5.2.3).

> **TODO (James):** What should happen when chunks are missing and there is no `fillValue`? The writer seems to always emit `fillValue` in that case. Should the specification require it?

---

## 11. Data reference resolution summary

For each `dmrpp:chunk` or `dmrpp:block`:

1. The URL is the element's `href` if present, otherwise `Dataset/@dmrpp:href`.
2. A value without `://` is a path under the BES catalog root and becomes a `file://` URL (§4.1).
3. The URL is trusted if the element has `trust="true"` or `dmrpp:trust="true"`. For a chunk with no `href` of its own, it is trusted if `Dataset/@dmrpp:trust="true"`.
4. Read `nBytes` bytes starting at byte `offset`.
5. Undo the `compressionType` filters in reverse order (§5.2.1).
6. Swap bytes according to `byteOrder` (§5.2.4).
7. Place the decoded values at `chunkPositionInArray` within the array (§6.3.1). Blocks are instead concatenated (§7).

---

## 12. Inconsistencies found in the example corpus

These are reported so you can decide whether to fix the examples or record the variants. None of them is endorsed by this specification.

| File | Issue |
|---|---|
| `dmrpp/a3_local_twoD.h5.dmrpp`, `dmrpp/chunked_gzipped_*.h5.dmrpp` and `dmrpp/chunked_shufzip_*.h5.dmrpp` (9 files), `s3/AIRS.2015.12.3*.dmrpp` (2 files, 777 each) | Use `deflate_level` instead of `deflateLevel` on `dmrpp:chunks` (1,563 occurrences in total). The reader recognizes only `deflateLevel`, so these values are ignored. This does not affect decoding (§5.2.2). |
| `dmrpp/nc4_group_atomic.h5.dmrpp` | `byteOrde="LE"` (misspelled) on 4 `dmrpp:chunks`. Ignored by the reader, so these variables get no byte order. |
| `dmrpp/a2_local_twoD.h5.dmrpp`, `dmrpp/a3_local_twoD.h5.dmrpp` | No `Dataset/@dmrpp:href`, which the reader requires (§4.1). |
| `dmrpp/h4mapper_testsds_splice.dmrpp` | Uses the `dmrpp:` prefix with no `xmlns:dmrpp` declaration, so it is not namespace-well-formed XML. The reference reader accepts it anyway (§2.2). |
| `h4_dmrpp/SDS_simple.hdf.dmrpp`, `h4_dmrpp/SDS_simple_comp.hdf.dmrpp` | The first line is the stray text `number of blocks for contiguous storage is: 1`, before the XML declaration. These files are not well-formed XML. |
| `fill_value/FValue.h5.dmrpp`, `fill_value/FValue_chunk.h5.dmrpp` | Use `https://` in both the DAP4 and the DMR++ namespace URIs (§2.2). |
| `dmrpp/coads_climatology.dmrpp` | Contains a commented-out `<dmrpp:byteStream offset="0" nBytes="1440" uuid="…" md5="…"/>`. This element appears nowhere else, and the reader does not recognize it. |

> **TODO (James):** Was `dmrpp:byteStream` (with `uuid` and `md5` attributes) an early design that was dropped? Should the specification mention it as historical?

---

## 13. Open questions (collected)

1. Is this specification normative or descriptive (§1.3)?
2. Should the namespace URI be required, and should the `https` variant be allowed (§2.2)?
3. Should the `vlsa` value element be `v` or `dmrpp:v` (§2.4)?
4. Must DMR++ children come after DAP4 children (§3.1)?
5. Are documents without `Dataset/@dmrpp:href` (per-chunk `href` only) valid (§4.1)?
6. Is `dmrpp:version` the BES/`build_dmrpp` version, and should the legacy filter-order rule stay (§4.2)?
7. Are the placeholder tokens and the `dmrpp:s3` and `dmrpp:s3credentials` attributes in scope (§4.4)?
8. Should the `build_dmrpp_metadata` provenance container be described (§4.6)?
9. Are `lzw` and `tiffpred:…` filters in scope for this version (§5.2.1)?
10. What is the canonical text form of numeric `fillValue` (§5.2.3)?
11. What does a missing `byteOrder` mean (§5.2.4)?
12. Can you provide an example of `LBChunk` and `LinkedBlockIndex` (§5.2.6)?
13. Edge chunks: is the stored shape always the full chunk shape (§6.3.1)?
14. What does `fm` mean, and must decoders honor it (§6.3.2)?
15. Which tool writes per-chunk `href`, and what is the `_mvs.h5` file (§6.4)?
16. Is the direct-child `dmrpp:chunk` form deprecated (§6.5)?
17. What is the byte order of inline binary data (§8)?
18. Is `specialstructuredata` packed with no alignment (§8.3)?
19. What are the exact trimming rules for each `pad` value (§9.1)?
20. Is a `vlsa` count that differs from the element count an error (§9.2)?
21. Is `fillValue` required when some chunks are not stored (§10)?
22. Is `dmrpp:byteStream` historical (§12)?

---

## Appendix A. Vocabulary summary

Counts are occurrences across all 186 example files (text search). "Reader" means `DMZ.cc` in this branch.

### A.1 Elements

| Element | Parent(s) | Content | Occurrences |
|---|---|---|---|
| `dmrpp:chunks` | variable element | `chunkDimensionSizes?`, then `chunk*` or `block*` | 2,030 |
| `dmrpp:chunkDimensionSizes` | `dmrpp:chunks` | text: space-separated integers | 1,890 |
| `dmrpp:chunk` | `dmrpp:chunks`; variable element (contiguous form) | empty | 4,601 |
| `dmrpp:block` | `dmrpp:chunks` | empty | 3 |
| `dmrpp:compact` | variable element | text: Base64 | 30 |
| `dmrpp:missingdata` | array variable element (or scalar `Byte`) | text: Base64 (zlib if more than one element) | 2 |
| `dmrpp:specialstructuredata` | `Structure` | text: Base64 (packed stream, §8.3) | 5 |
| `dmrpp:FixedLengthStringArray` | `String` array | empty | 30 |
| `dmrpp:vlsa` | `String` array | `v+` | 8 |
| `v` (unprefixed) | `dmrpp:vlsa` | text: string, or Base64 of zlib if `s` is set | 37 |

### A.2 Attributes

| Attribute | On | Seen | Reader recognizes |
|---|---|---|---|
| `dmrpp:href` | `Dataset` | 184 | yes (required) |
| `dmrpp:version` | `Dataset` | 90 | yes |
| `dmrpp:trust` | `Dataset` | 1 | yes |
| `dmrpp:s3`, `dmrpp:s3credentials` | `Dataset`, `dmrpp:chunk` (after substitution) | 0 | no (ignored) |
| `compressionType` | `dmrpp:chunks` | 1,617 | yes |
| `deflateLevel` | `dmrpp:chunks` | 24 | yes |
| `fillValue` | `dmrpp:chunks` | 180 | yes |
| `byteOrder` | `dmrpp:chunks` | 155 | yes |
| `structOffset` | `dmrpp:chunks` | 25 | yes |
| `LBChunk` | `dmrpp:chunks` | 0 | yes |
| `DIO` | `dmrpp:chunks` | 0 | yes |
| `offset` | `dmrpp:chunk`, `dmrpp:block` | all | yes (required) |
| `nBytes` | `dmrpp:chunk`, `dmrpp:block` | all | yes (required) |
| `chunkPositionInArray` | `dmrpp:chunk` | 4,477 | yes |
| `href` | `dmrpp:chunk`, `dmrpp:block` | 14 (chunk only) | yes |
| `trust` / `dmrpp:trust` | `dmrpp:chunk`, `dmrpp:block` | 0 | yes |
| `fm` | `dmrpp:chunk` | 0 | yes |
| `LinkedBlockIndex` | `dmrpp:chunk` | 0 | yes |
| `string_length` | `dmrpp:FixedLengthStringArray` | 30 | yes |
| `pad` | `dmrpp:FixedLengthStringArray` | 30 | yes |
| `c` | `v` | 5 | yes |
| `s` | `v` | 2 | yes |
| `deflate_level` | `dmrpp:chunks` | 1,563 | **no** (see §12) |
| `byteOrde` | `dmrpp:chunks` | 4 | **no** (misspelling, §12) |

## Appendix B. Example corpus

| Directory | `.dmrpp` files | Main topics |
|---|---|---|
| `buffer_chunk` | 9 | Raw `.bin` sources, out-of-order chunks, fill chunks |
| `deflate_twice` | 5 | `deflate deflate` pipelines |
| `dmrpp` | 105 | General coverage: chunking, filters, compound types, enums, compact, contiguous, hrefs, trust |
| `fill_value` | 32 | `fillValue` for numeric, string and compound types; unstored chunks |
| `h4_dmrpp` | 13 | HDF4 sources: `BE` data, linked blocks, `missingdata` |
| `s3` | 5 | Absolute `https://` hrefs to S3 objects (AIRS, MERRA-2) |
| `string_arrays` | 17 | Fixed-length and variable-length string arrays, `vlsa`, compact strings |
