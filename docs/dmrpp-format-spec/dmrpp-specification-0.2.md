# DMR++ XML Format: Descriptive Specification

**Status:** Draft 0.2, 19 November 2026. This is a *descriptive* document. It records what DMR++ documents contain today and how the Hyrax reader treats them. Its purpose is to guide improvements to the implementation. A normative specification will be written afterwards. Nothing in this draft should be read as a requirement on producers or readers.

**Changes from Draft 0.1**

- Added two new sources: the curated examples taken from `bes/modules/fileout_netcdf/data`, and the HDF4 MODIS file `MYD09.A2019003.2040.006.2019005020913.hdf.dmrpp`.
- Rewrote the linked-block chunk section (`LBChunk`, `LinkedBlockIndex`) using real examples. It is now §7.
- Documented `DIO="off"`, `dmrpp:block` inside `Structure`, the single-block "legacy" form, and `dmrpp:missingdata` on scalar `Byte` variables and on one-element arrays.
- Corrected the description of `structOffset`. It can be present or absent for packed structures, and the reader treats a missing `structOffset` as "packed".
- Added §13, an evaluation of `dmrpp.xsd`, and §14, implementation issues that a normative specification would need resolved.
- Dropped the normative wording (MUST and similar) from Draft 0.1. Resolved several open questions and added new ones (§15).

---

## 1. Introduction

### 1.1 Purpose of DMR++

A DMR++ document lets a server read a dataset's values directly from the original data file (HDF5, netCDF-4, HDF4, and so on), stored locally or in object storage, without using the format library that wrote the file.

A DMR++ is a DAP4 DMR (Dataset Metadata Response) with extra XML elements and attributes added. These additions record:

- where each variable's bytes are in the source file (byte offset and length of each chunk or block);
- how chunks fit together (chunk shape and each chunk's position in the array);
- how the bytes are encoded (filters, byte order, fill value);
- small values that are carried in the document itself.

### 1.2 Relationship to the DAP4 DMR

The DMR is defined in DAP4 §1.5, *DMR Declarations* (<https://opendap.github.io/dap4-specification/DAP4.html#_dmr_declarations>). This document does not repeat that material. The DMR elements `Dataset`, `Group`, `Dimension`, `Enumeration`, the variable elements, `Dim`, `Map` and `Attribute` keep their DAP4 meanings.

The DMR++ additions are in the `dmrpp` namespace (§2). They appear in two places:

- as attributes on `Dataset` (§4);
- as child elements of variable elements (§5–§10).

A *variable element* is any DAP4 variable declaration: one of the atomic types (`Byte`, `Int8`, `UInt8`, `Int16`, `UInt16`, `Int32`, `UInt32`, `Int64`, `UInt64`, `Float32`, `Float64`, `String`, `URI`), or `Enum`, `Opaque`, `Structure` or `Sequence`. A variable is an *array* if it has one or more `Dim` children, and a *scalar* otherwise.

### 1.3 Sources and how statements are marked

| Tag | Source |
|---|---|
| **[A]** | 186 `.dmrpp` files in `bes/modules/dmrpp_module/data/{buffer_chunk, deflate_twice, dmrpp, fill_value, h4_dmrpp, s3, string_arrays}`. |
| **[B]** | 129 `.dmrpp` files in `bes/modules/fileout_netcdf/data`. These were selected and summarized in `dmrpp_feature_examples.docx` by the lead DMR++ developer. The snippets in that document leave out some wrapper elements, so I checked the cited files on disk. |
| **[C]** | `MYD09.A2019003.2040.006.2019005020913.hdf.dmrpp`. This is an HDF4 MODIS Level 2 swath file that is not yet in the code base. |
| **[xsd]** | `dmrpp.xsd`, the DMR++ XML Schema (namespace `http://xml.opendap.org/dap/dmrpp/1.0.0#`, version 1.0). It is evaluated in §13. |
| **[impl]** | The reader and writer code in the `bes-tmp` working copy, mainly `DMZ.cc` (reader), `DmrppCommon.cc` and `DMRpp.cc` (writer), `build_dmrpp_util.cc`, `vlsa_util.cc`, `Chunk.cc` and `DmrppArray.cc`. |

"The reader" always means the Hyrax DMR++ reader (`DMZ.cc`) in this branch. Items that need an answer from the DMR++ developers are marked **QUESTION**. They are collected in §15.

---

## 2. Namespace

### 2.1 URI and prefix

All DMR++ documents in [A], [B] and [C] declare the namespace on `Dataset` with the prefix `dmrpp`:

```xml
<Dataset xmlns="http://xml.opendap.org/ns/DAP/4.0#"
         xmlns:dmrpp="http://xml.opendap.org/dap/dmrpp/1.0.0#" …>
```

The exceptions are listed in §14.

The writer always produces this URI and prefix **[impl: `DmrppCommon::d_dmrpp_ns`, `d_ns_prefix`]**. The schema uses the same URI as its target namespace **[xsd]**.

### 2.2 The reader matches the literal prefix

The reader does not process XML namespaces. It looks for the literal names `dmrpp:chunks`, `dmrpp:href` and so on, and it does not check the namespace URI **[impl: `DMZ::process_dataset` comment]**. The schema describes this as a "Hyrax deviation" on every element **[xsd]**. In practice:

- A document that binds the DMR++ namespace to a different prefix will not be read.
- A document that uses a different URI, or no namespace declaration at all, *will* be read.

### 2.3 Attributes on DMR++ elements have no prefix

Attributes on `dmrpp:` elements are written without a prefix, for example `<dmrpp:chunk offset="…" nBytes="…"/>`. This matches the schema's `attributeFormDefault="unqualified"` **[xsd]**. Attributes that DMR++ adds to `Dataset` (§4) are written with the prefix (`dmrpp:href`).

There is one mixed case. On `dmrpp:chunk` and `dmrpp:block`, the reader accepts both `trust` and `dmrpp:trust` **[impl: `DMZ::process_chunk`]**. When the server fills in placeholder URLs, it adds `dmrpp:trust="true"` to chunks (§4.5).

### 2.4 The `v` element has no prefix

`<v>` elements inside `dmrpp:vlsa` are written without a prefix in every example ([A], [B]). Because the document's default namespace is DAP4, a namespace-aware parser places `v` in the DAP4 namespace. The schema instead places `v` in the DMR++ namespace, because its `elementFormDefault="qualified"` setting requires `dmrpp:v` **[xsd]**. The reader looks for the literal name `v` **[impl: `DmrppNames.h`]**. This disagreement is listed as XSD-C7 in §13.

---

## 3. Document overview

```xml
<Dataset xmlns="http://xml.opendap.org/ns/DAP/4.0#"
         xmlns:dmrpp="http://xml.opendap.org/dap/dmrpp/1.0.0#"
         name="…" dapVersion="4.0" dmrVersion="1.0"
         dmrpp:href="…"  dmrpp:version="…"  dmrpp:trust="true"     ← §4
         dmrpp:s3="…"   dmrpp:s3credentials="…">                  ← §4.4

  <Float32 name="v1"> <Dim …/> <Attribute …/>                     (DAP4)
    <dmrpp:chunks compressionType="…" deflateLevel="…" fillValue="…"
                  byteOrder="…" structOffset="…" LBChunk="…" DIO="…">   ← §5
      <dmrpp:chunkDimensionSizes>…</dmrpp:chunkDimensionSizes>         ← §5.3
      <dmrpp:chunk offset nBytes chunkPositionInArray [href] [fm] [LinkedBlockIndex]/> ← §6, §7
      …  or  <dmrpp:block offset nBytes/> …                            ← §8
    </dmrpp:chunks>
  </Float32>

  <Int32 name="v2"> <dmrpp:chunk offset nBytes/> </Int32>        ← §6.5 (older form, no wrapper)
  <Int32 name="v3"> <dmrpp:chunks fillValue="0"/> <dmrpp:compact>…</dmrpp:compact> </Int32>  ← §9.1
  <Float64 name="v4"> <dmrpp:missingdata>…</dmrpp:missingdata> </Float64>                 ← §9.2
  <Structure name="v5"> … <dmrpp:chunks fillValue="0"/>
      <dmrpp:specialstructuredata>…</dmrpp:specialstructuredata> </Structure>               ← §9.3
  <String name="v6"> <Dim …/> <dmrpp:chunks>…</dmrpp:chunks>
      <dmrpp:FixedLengthStringArray string_length="N" pad="…"/> </String>                   ← §10.1
  <String name="v7"> <Dim …/> <dmrpp:vlsa> <v>…</v> <v c="2">…</v> <v s="1024">…</v> </dmrpp:vlsa> </String> ← §10.2
</Dataset>
```

### 3.1 Combinations of DMR++ children observed on a variable

| DMR++ children of a variable element | [A] | [B] | [C] |
|---|---|---|---|
| `dmrpp:chunks` | 1,982 | 249 | 46 |
| `dmrpp:chunks` + `dmrpp:FixedLengthStringArray` | 23 | 9 | – |
| `dmrpp:chunk` (no `dmrpp:chunks` wrapper) | 24 | 0 | – |
| `dmrpp:compact` | 10 | 0 | – |
| `dmrpp:chunks` + `dmrpp:compact` | 13 | 4 | – |
| `dmrpp:compact` + `dmrpp:FixedLengthStringArray` | 4 | 0 | – |
| `dmrpp:chunks` + `dmrpp:compact` + `dmrpp:FixedLengthStringArray` | 3 | 0 | – |
| `dmrpp:chunks` + `dmrpp:specialstructuredata` | 5 | 3 | – |
| `dmrpp:missingdata` | 2 | 29 | – |
| `dmrpp:vlsa` | 8 | 4 | – |

[A] counts exclude 3 files that are not well-formed XML (§14). All [B] and [C] files are well-formed.

In every example, the DMR++ children come after the variable's DAP4 children. The reader finds DMR++ children by name, so their position does not matter to it **[impl]**.

---

## 4. Extensions to `Dataset`

### 4.1 `dmrpp:href`

This is the default location of the data that every `dmrpp:chunk` and `dmrpp:block` refers to, unless the element has its own `href` (§6.4).

- The reader rejects a document that does not have it **[impl: `DMZ::process_dataset`]**. The schema marks it `use="required"` **[xsd]**.
- It appears on every [B] and [C] file, and on every [A] file except two (§14).
- Forms observed:
  - a path without a scheme, such as `data/MYD09….hdf` [C] or `/data/string_arrays/t_string_compact.h5` [A];
  - an absolute `https://` URL [A];
  - the placeholder `OPeNDAP_DMRpp_DATA_ACCESS_URL` [A] (see §4.5).
- A value without `://` is treated as a path under the BES default catalog root, even if it starts with `/`, and becomes a `file://` URL **[impl: `http/url_impl.cc`, `url::parse`]**.

### 4.2 `dmrpp:version`

This is the version of the DMR++ software that wrote the document. The schema's example is `3.21.1-1090`, which shows that a build-number suffix can appear **[xsd]**. Values observed:

- **[A]:** `3.20.10`, `3.20.13`, `3.21.0`, `3.21.1`, `3.22.0`, in 90 of 186 files.
- **[B]:** in 127 of 129 files.
- **[C]:** absent.

**How the reader uses it [impl: `DMZ::process_dataset`, `DmrppCommon::set_filter`]:** if `dmrpp:version` is absent, the reader treats the document as "old style". It discards the order and duplicates in `compressionType` and rebuilds the list as `shuffle deflate fletcher32`, keeping only the filters that are present. Values containing `lzw` or `tiffpred` are not rebuilt. As a result, a version-less document that uses `deflate deflate` would be decoded wrongly. In [C] this has no effect because only `deflate` is used.

### 4.3 `dmrpp:trust`

The reader treats the `dmrpp:href` URL as trusted only when the value is the literal string `true`. A trusted URL bypasses the `AllowedHosts` check **[impl]**.

The schema adds history and guidance **[xsd]**. The attribute was originally a hint that URLs taken from NASA's CMR need not be checked against an allow-list. Most users of DMR++ documents should ignore it, and software should not use it for access-control decisions. The schema gives it type `xs:boolean` with default `false`.

It appears in one [A] file and in no [B] or [C] file.

### 4.4 `dmrpp:s3` and `dmrpp:s3credentials`

The schema declares both as optional `Dataset` attributes **[xsd]**:

- `dmrpp:s3` is an `s3://` URI for the data, an alternative to `dmrpp:href`.
- `dmrpp:s3credentials` is the URL of a service that issues credentials for that S3 URI (for example, a DAAC `/s3credentials` endpoint).

Neither attribute appears in [A], [B] or [C]. The DMR++ reader ignores both. They are added by the server's placeholder substitution (§4.5).

**QUESTION:** Is any reader expected to use `dmrpp:s3` in place of `dmrpp:href`?

### 4.5 Placeholder URLs filled in by the server

When serving a document, the NGAP and S3 containers replace the text `href="OPeNDAP_DMRpp_DATA_ACCESS_URL"` with:

```
href="<data URL>" dmrpp:s3="<s3 URL>" dmrpp:s3credentials="<credentials URL>" dmrpp:trust="true"
```

They replace `href="OPeNDAP_DMRpp_MISSING_DATA_ACCESS_URL"` the same way, except that `_mvs.h5` is appended to each URL **[impl: `NgapOwnedContainer::get_daac_content_filters`; `S3Container.cc`]**. Because this is plain text replacement, the result depends on the exact spelling `href="…"`. The schema does not mention the placeholders.

### 4.6 Other `Dataset` content

- `xml:base` is passed through to the response [A, one file].
- The reader always sets the DMR version to `1.0`, whatever value the document gives.
- Recent files ([A], [B]) contain a DAP4 `Attribute` container named `build_dmrpp_metadata` that records provenance (`created`, `build_dmrpp`, `bes`, `libdap`, `configuration`, `invocation`). It is ordinary DMR content. [C] does not have it.

---

## 5. `dmrpp:chunks`

### 5.1 Role

`dmrpp:chunks` occurs at most once per variable element. It does three things:

- holds the storage attributes for the variable (§5.2);
- holds the chunk shape (§5.3);
- holds the data references (`dmrpp:chunk` §6/§7, or `dmrpp:block` §8).

It is used for both chunked and contiguous storage. It also appears with no children, carrying only `fillValue` and/or `byteOrder`. That empty form is observed next to `dmrpp:compact` and `dmrpp:specialstructuredata`, and for variables that are entirely fill values (§11). Note that the schema's description of this element does not match these uses (XSD-C3 in §13).

It is observed under every numeric atomic type, `Enum`, `String` and `Structure`, both scalar and array.

### 5.2 Attributes

| Attribute | [A] | [B] | [C] | Value |
|---|---|---|---|---|
| `compressionType` | 1,617 | 102 | 46 | filter list (§5.2.1) |
| `deflateLevel` | 24 | 92 | 46 | integer list (§5.2.2) |
| `fillValue` | 180 | 212 | 46 | text (§5.2.3) |
| `byteOrder` | 155 | 231 | 46 | `LE` or `BE` (§5.2.4) |
| `structOffset` | 25 | 5 | 0 | integer list (§5.2.5) |
| `LBChunk` | 0 | 0 | 6 | `true` (§7) |
| `DIO` | 0 | 3 | 0 | `off` (§5.2.6) |

These counts are per file attribute, mostly on `dmrpp:chunks`. The reader ignores attributes it does not recognize. The misspellings found in [A] are listed in §14.

#### 5.2.1 `compressionType`

A space-separated list of filters, in the order they were applied when the data was written. To decode, the reader undoes them in reverse order **[impl: `Chunk::filter_chunk`; xsd agrees]**. For version-less documents, see §4.2.

| Token | Reader behavior **[impl]** |
|---|---|
| `deflate` | zlib inflate. The token may be repeated, but repeats must be next to each other in the list, or the reader reports an error. |
| `shuffle` | Undoes HDF5 byte shuffling, using the element size. |
| `fletcher32` | Removes the 4-byte checksum at the end. The checksum itself is not verified (that check is disabled at compile time). |
| `lzw`, `tiffpred:<p>:<spp>:<epr>` | TIFF LZW decoding and the TIFF predictor (`p` is 2 or 3). Not used in any example. |

Values observed:

- **[A]:** `deflate`, `shuffle`, `shuffle deflate`, `deflate shuffle`, `deflate fletcher32`, `shuffle deflate fletcher32`, `deflate deflate`, `shuffle deflate deflate`, `fletcher32`, `fletcher32 shuffle deflate deflate`, `shuffle deflate deflate fletcher32`.
- **[B]:** `deflate` (71), `shuffle deflate` (13), `fletcher32 shuffle deflate` (8), `shuffle deflate deflate` (4), `deflate deflate` (3), `fletcher32 shuffle deflate deflate`, `shuffle deflate deflate fletcher32`, `shuffle deflate fletcher32`.
- **[C]:** `deflate` only.

The schema says chunks can override `compressionType` individually **[xsd]**. That is not possible in either the schema's own chunk definition or the reader (XSD-C6).

#### 5.2.2 `deflateLevel`

A space-separated list with one level per `deflate` token **[impl; xsd agrees]**. Single values observed: `1`, `4`, `6`, `8`, `9`.

Six [B] files (`*_2deflates.h5`) use two levels, `deflateLevel="4 1"`, with `compressionType="deflate deflate"` or `"shuffle deflate deflate"`. By the schema's rule ("list the levels for each of them in order"), the first level belongs to the first `deflate` in the list. The [A] files that use `deflate deflate` (`deflate_twice/`) have no `deflateLevel`.

The level is not needed to decode the data. The reader uses it only to decide whether *direct I/O* is possible, meaning chunks are copied unchanged into a netCDF-4 response. Direct I/O also requires all of the following **[impl: `DMZ::set_up_direct_io_flag_phase_1/2`]**:

- `deflate` appears in `compressionType`;
- `byteOrder="LE"`;
- the variable is an integer or floating-point array;
- the filter list does not end with `fletcher32`;
- `dmrpp:chunkDimensionSizes` is present;
- `DIO` is not `off`.

#### 5.2.3 `fillValue`

The value used for elements or whole chunks that are not stored (§11). How it is written depends on the variable type:

- **Numeric and `Enum`:** decimal text, written in various styles: `0`, `-28672`, `65535`, `-9999.9`, `0.000000` ([C], `Float32`), `-9999.000000`, `9.96921e+36`.
- **`String`:** the string itself. Values observed: empty, `a`, `aa`.
- **`Structure`:** one value per member, separated by spaces, in member order. Array members are expanded element by element. Examples: `-9999.9 32767 999.9` and `-9999.9 32767 32766 32765 999.9 899.9` [A, B].
- **Sentinels** **[impl: `flagged_as_unsupported_type`]**:
  - `unsupported-string` is accepted, and treated as an empty fill value, for scalar strings and fixed-length string arrays. Otherwise the reader reports an error.
  - `unsupported-variable-length-string`, `unsupported-array` and `unsupported-compound` always cause an error. None of these three appears in the examples.

The reader rejects `fillValue` on `URI`, `Sequence` and `Grid` variables. The schema describes `fillValue` only as a string "interpreted according to the data type" **[xsd]**.

#### 5.2.4 `byteOrder`

`LE` or `BE`. Any other non-empty value is an error. The reader swaps bytes when this differs from the host's byte order **[impl]**. `BE` appears in HDF4 sources ([A] `h4_dmrpp`, [B] HDF4 files, and every variable in [C]).

When `byteOrder` is absent:

- the schema says the default is `LE` **[xsd]**;
- the reader does no swapping, which effectively means "host order" **[impl]**.

These agree on little-endian hosts and differ on big-endian hosts (XSD-C13).

#### 5.2.5 `structOffset`

Used only with `Structure` variables (HDF5 compound types). It gives the byte offset of every member except the first, followed by the total size of one record in bytes **[impl: `obtain_structure_offset`; xsd agrees]**. Example: `Float64`, `Int16`, `Float32[2]` gives `8 12 24` [A].

**Present or absent.** `structOffset` appears for padded records (`8 12 16` for `Float64`, `Int16`, `Float32`). For packed records it is sometimes present and sometimes absent:

| File | Members | `dmrpp:version` | `structOffset` |
|---|---|---|---|
| [A] `fill_value/comp_scalar_udf_nfv_right_offset.h5.dmrpp` | `Float64`, `Int32`, `Float32` (packed) | 3.21.0 | `8 12 16` |
| [B] `comp_scalar_udf_nfv_right.h5.dmrpp` | `Float64`, `Int32`, `Float32` (packed) | 3.21.1 | absent |
| [xsd] example | `Int32`, `Int64`, `Float32` (packed) | – | `4 12 16` |

So the note in the curated examples ([B]), that `structOffset` "appears when members do not sit back to back", does not hold for every file. In this branch, the writer emits `structOffset` for every compound variable that is not stored as compact and has at least one chunk **[impl: `build_dmrpp_util.cc` near `process_string_in_structure`; `DmrppCommon::print_chunks_element`]**.

When `structOffset` is absent, the reader assumes the members are packed back to back. It uses the sum of the member widths as the record size **[impl: `DmrppArray.cc`, `DmrppStructure.cc`]**. A packed file decodes correctly either way.

**QUESTION:** Which writer, or which version of it, left out `structOffset` for the [B] file? Should `structOffset` be emitted always, or only when the record has padding?

#### 5.2.6 `DIO`

`DIO="off"` turns off direct I/O (§5.2.2) for this variable. The writer adds it only when `compressionType` is present **[impl]**. Observed in [B]: `two_arrays_16x16.h5_no_dio.dmrpp` (which is meant to be compared with `two_arrays_16x16.h5.dmrpp`) and `One_chunk_shuf_deflate.h5.disable_dio.dmrpp`.

```xml
<dmrpp:chunks compressionType="deflate" deflateLevel="1" fillValue="0" byteOrder="LE" DIO="off">
```

The reader recognizes only the value `off`. The schema's wording, "If not given, assume the value is false", should mean "direct I/O is not disabled" (XSD-C14).

### 5.3 `dmrpp:chunkDimensionSizes`

The size of one chunk in elements, written as space-separated integers, one per `Dim` and in the same order. Examples: `50 50`, `10 1354` [C], `40 5416` [C].

- It is present for chunked storage. It is absent for contiguous storage, which is a single reference with no `chunkPositionInArray`.
- For variables that are entirely fill values, it is sometimes present and sometimes absent (§11). The schema says it is "omitted" in that case, which the examples contradict (XSD-C5).
- The schema allows at most one occurrence. A schema comment says there may be more and that the reader uses the first **[xsd, internal inconsistency XSD-C12]**.

### 5.4 Content model, as observed

```
dmrpp:chunks ::= dmrpp:chunkDimensionSizes?  ( dmrpp:chunk* | dmrpp:block* )
```

`chunkDimensionSizes` comes first. `chunk` and `block` are never mixed in one `dmrpp:chunks` **[impl; xsd uses `xs:choice`]**.

---

## 6. `dmrpp:chunk`

### 6.1 Role and placement

A `dmrpp:chunk` refers to one run of stored bytes. Normally it is a child of `dmrpp:chunks`. Counts: 4,573 in [A], 5,496 in [B], 10,553 in [C].

In the older form, it is a direct child of the variable element (§6.5). There are 24 such cases in [A] and none in [B] or [C].

### 6.2 Attributes

| Attribute | Observed | Reader | Value |
|---|---|---|---|
| `offset` | all | required | byte offset in the file (unsigned 64-bit) |
| `nBytes` | all | required | stored (filtered) length in bytes |
| `chunkPositionInArray` | chunked storage | optional | element index of the chunk's first element (§6.3) |
| `href` | [A] 14, [B] 23 | optional | per-chunk data location (§6.4) |
| `trust` / `dmrpp:trust` | none in files; added by substitution | optional | `true` means the `href` is trusted |
| `fm` | none | optional | HDF5 filter mask (§6.6) |
| `LinkedBlockIndex` | [C] 2,032 | optional | position of this block within a linked-block chunk (§7) |

Attributes can appear in any order, with one exception: in practice `LinkedBlockIndex` must be the last attribute (§7.3).

### 6.3 `chunkPositionInArray`

The **element indices** of the chunk's first element, written as a bracketed list separated by commas with no spaces: `[0,50]`, `[1910,0]`. Each index is a multiple of the chunk size in that dimension. Every example in [A], [B] and [C] follows this rule. For instance, in [C] a 2030-row array divided into 10-row chunks has chunk positions `[0,0]`, `[10,0]`, …, `[2020,0]`.

The schema says instead that this attribute holds *chunk-space* indices (`[1,0,0,0]` for the second chunk) and that it is required **[xsd]**. Both statements disagree with the examples (XSD-C1 and XSD-C2).

The attribute is absent on references to contiguous data: 120 such chunks in [A] and 86 in [B].

Chunks can be listed in any order. Chunks at the edge of an array can extend past the array's bounds (see the `*_uneven*` files in [A]).

**QUESTION:** Is the stored size of an edge chunk always the full chunk shape?

### 6.4 Per-chunk `href`

When present, this overrides `Dataset/@dmrpp:href` **[impl; xsd agrees]**. Uses observed:

- One file per chunk, with `offset="0"` (`bstream_<md5>.bs`, [A]).
- *Missing-variables* companion files named `<granule>_mvs.h5` ([A] `grid_*_2d.h5`; [B] `grid_2_2d_sin.h5`). In [B] the same document uses both a `_mvs.h5` href and inline `dmrpp:missingdata`.

The writer in `DmrppCommon.cc` never writes a per-chunk `href`.

**QUESTION:** Which tool writes these, and what is the `_mvs.h5` file format?

### 6.5 Older form: `dmrpp:chunk` without a `dmrpp:chunks` wrapper

```xml
<Int16 name="d16_1"> <Dim size="2"/> … <dmrpp:chunk nBytes="4" offset="2216"/> </Int16>
```

The reader treats this as contiguous storage with no filters and no byte swapping **[impl]**. It appears only in older hand-written [A] files. Neither [B] nor [C] uses it, and the curated examples always place `dmrpp:chunk` inside `dmrpp:chunks`.

### 6.6 `fm` (filter mask)

The writer emits `fm` only for chunked storage, and only when the mask is non-zero. The value comes from the HDF5 per-chunk filter mask (`H5Dchunk_iter`) **[impl]**. The schema says it is a 32-bit mask that shows which filters in `compressionType` were actually applied, because filters can fail when data is written. It also says direct I/O needs the mask for the HDF5 chunk-write API **[xsd]**.

In HDF5, a *set* bit means the filter at that pipeline position was *skipped* for this chunk. The schema's wording does not say which way the bits work (XSD-C15).

The general decoding path does not use `fm`. The linked-block path discards it. Only direct I/O passes it on **[impl]**. No example contains `fm`.

---

## 7. Linked-block chunks: `LBChunk` and `LinkedBlockIndex`

### 7.1 Purpose

In HDF4, a chunk is normally stored as one contiguous piece. Some chunks, however, are stored as several *linked blocks* in different places in the file. Such a chunk must be put back together from its blocks before the filters are undone **[xsd, LBChunk documentation; impl]**. [C] is a real example.

### 7.2 Encoding, as observed in [C]

```xml
<dmrpp:chunks compressionType="deflate" deflateLevel="8" fillValue="60" byteOrder="BE" LBChunk="true">
    <dmrpp:chunkDimensionSizes>10 1354</dmrpp:chunkDimensionSizes>
    …
    <dmrpp:chunk offset="3209132"   nBytes="365" chunkPositionInArray="[1910,0]" LinkedBlockIndex="0"/>
    <dmrpp:chunk offset="355271302" nBytes="2"   chunkPositionInArray="[1910,0]" LinkedBlockIndex="1"/>
    …
</dmrpp:chunks>
```

(Variable `_1km_Atmospheric_Optical_Depth_Band_3`.)

- `LBChunk="true"` is set on `dmrpp:chunks` whenever at least one chunk of the variable is split into linked blocks. In [C], 6 of 46 variables have it.
- Each block of a split chunk is a separate `dmrpp:chunk` element. All of them have:
  - the same `chunkPositionInArray`;
  - their own `offset` and `nBytes`;
  - a `LinkedBlockIndex` counting `0, 1, …, n-1`.

  The blocks of one chunk are adjacent in the document and listed in index order.
- Chunks that are not split, in the same variable, have no `LinkedBlockIndex`. For example, in `_1km_Atmospheric_Optical_Depth_Band_3`, 202 of 203 chunks are ordinary and one is split into 2 blocks. In `_1km_Atmospheric_Optical_Depth_Band_8`, 4 of 203 chunks are split.
- Some variables have every chunk split. `_1km_water_vapor` has 21 chunks of 3 blocks and 182 chunks of 4 blocks.
- Blocks can be very small (`nBytes="2"`) and very far from block 0 in the file.
- In [C], `LinkedBlockIndex` values are 0 (817 times), 1 (817), 2 (216) and 3 (182). It is always the last attribute.

### 7.3 Reader behavior **[impl: `DMZ::process_chunks`, `add_mblock_index`, `process_multi_blocks_chunk`]**

When `LBChunk="true"`, the reader:

1. collects `(offset, nBytes)` for each run of blocks, where a run starts at `LinkedBlockIndex="0"`;
2. creates one logical chunk per run, at the shared `chunkPositionInArray`;
3. skips the elements whose `LinkedBlockIndex` is not 0.

Chunks that have no `LinkedBlockIndex` are handled as ordinary chunks.

The function that creates each logical chunk checks only the *last* attribute when looking for `LinkedBlockIndex`. Its fallback loop over all attributes is meant to handle other positions, but it tests the last attribute again instead of the current one. As a result, a `LinkedBlockIndex` that is not last would be treated as an ordinary chunk. This would leave the block lists out of step with the chunks and produce wrong data. See §14.

### 7.4 Comparison with `dmrpp:block`

`dmrpp:block` (§8) describes a whole *variable* stored as linked blocks. `LBChunk`/`LinkedBlockIndex` describe single *chunks* stored as linked blocks inside a chunked variable.

**QUESTION:** Should linked-block chunks use nested elements in the future (for example, `dmrpp:chunk` containing `dmrpp:block` children) instead of repeated `dmrpp:chunk` elements tied together by `LinkedBlockIndex`?

---

## 8. `dmrpp:block`

### 8.1 Role

A `dmrpp:block` is one block of an HDF4 variable whose data is stored as linked blocks that are not next to each other in the file. The reader joins the blocks in document order to form the variable's data **[impl]**. The schema adds that filters, if there are any, are applied to the joined result **[xsd]**.

### 8.2 Placement

`dmrpp:block` is a child of `dmrpp:chunks` and is never mixed with `dmrpp:chunk`. Observed under:

- `Int32` arrays (SDS): [A] and [B] `SDS_unlimited_2.hdf.dmrpp`, and [B] `sds_two_linked_blocks.hdf.legacy.dmrpp`;
- `Float32` arrays: [B] `vdata_linked_blocks.hdf.dmrpp`;
- `Structure` arrays (HDF4 Vdata): [B] `vdata_packed_linked_blocks.hdf.dmrpp` and `PointFile.hdf.dmrpp`.

```xml
<Structure name="…"> …
    <dmrpp:chunks byteOrder="BE">
        <dmrpp:block offset="294" nBytes="12"/>
        <dmrpp:block offset="495" nBytes="12"/>
    </dmrpp:chunks>
</Structure>
```

### 8.3 Attributes

`offset` and `nBytes` are required. `href` and `trust` (or `dmrpp:trust`) are optional and are not used in any example **[impl; xsd agrees]**.

### 8.4 The single-block "legacy" form

The writer now emits `dmrpp:block` only when there are two or more blocks. Older files can contain a single block, which happened when two adjacent linked blocks were merged. The reader still accepts this form **[impl comment]**. [B] `sds_two_linked_blocks.hdf.legacy.dmrpp` is an example:

```xml
<dmrpp:chunks byteOrder="BE"> <dmrpp:block offset="2776" nBytes="32"/> </dmrpp:chunks>
```

---

## 9. Inline data elements

These three elements hold a variable's values inside the DMR++ document as text. The reader copies the decoded bytes into memory as they are, with no byte swapping, even when `byteOrder` is present **[impl: `val2buf`]**. In every example, the decoded numeric data is little-endian.

**QUESTION:** Is inline binary data always little-endian, or is it in the producer's native byte order?

### 9.1 `dmrpp:compact`

Holds all of a variable's values when the source used HDF5 *compact* storage, in which the data sits in the object header and has no file offset. The content is Base64 of the raw bytes, *not* compressed.

| Variable | Content |
|---|---|
| Numeric or `Enum` | the values, row-major order |
| Scalar `String` | the bytes of the string |
| Fixed-length `String` array | the strings one after another, each `string_length` bytes long (needs a `dmrpp:FixedLengthStringArray` sibling) |

Variable-length string arrays in compact form are rejected. So are `Structure`, `Sequence` and `Grid` **[impl]**.

`dmrpp:compact` often appears next to an empty `dmrpp:chunks` that carries `fillValue`. The schema describes compact as being for "small variables where it is more efficient" and says that `byteOrder` affects how the data is read **[xsd]**. In fact, compact comes from HDF5 storage layout, and the reader ignores `byteOrder` for it (XSD-C18).

### 9.2 `dmrpp:missingdata`

Holds values that are *not stored* in the source file and were computed when the DMR++ was built. Examples:

- latitude and longitude for HDF-EOS2 grids ([A] `sinusoid_ll.hdf`, [B] `cea_ll.hdf` and others);
- index values for made-up dimensions (`FakeDim*`) in HDF5 files ([B] `t_cf_1dllzs2.h5`);
- scalar `Byte` variables in grid files ([B] `grid_2_2d_sin.h5`, value `cA==`, which decodes to one byte, 0x70).

The schema explains the purpose: these values normally come from APIs such as HDF-EOS2, which cannot be used against cloud object storage **[xsd]**.

Placement and content **[impl: `DMZ::process_missing_data`]**:

| Variable | Content | Observed |
|---|---|---|
| Array with more than one element | Base64 of the **zlib-compressed** raw bytes, row-major order | [A] `Float64` ×2; [B] `Float64` ×16, `Int32` ×5 |
| Array with exactly one element | Base64 of the raw bytes, not compressed | [B] `Int32` ×1 (`AAAAAA==`) |
| Scalar `Byte` | Base64 of the one byte | [B] ×7 |

No other variable types are accepted. The schema's first sentence for this element ("missing (i.e., fill values)") is wrong. Its second paragraph is correct (XSD-C8).

### 9.3 `dmrpp:specialstructuredata`

Holds all values of a `Structure` (HDF5 compound) that has `String` members. Such records cannot be read directly from the file. It is observed with an empty `<dmrpp:chunks fillValue="0"/>` sibling, on scalar and array structures. Examples in [B] include `compound_simple_scalar_memb_str_array.h5` (a member that is a `String` array) and `compound_array_fix_vlen_str.h5` (both fixed-length and variable-length string members).

The content is Base64 of a packed byte stream. For each structure element, and within it each member in order **[impl]**:

- **numeric member:** its raw bytes, with no separator;
- **`String` member:** the Base64 of the string, followed by `;`;
- **`String` array member:** each element as Base64 followed by `;`.

I decoded one example by hand: `gQQAAIXrVEJNakF5TXc9PTs=` contains Int32 1153 (LE), Float32 53.23 (LE), then `MjAyMw==;`, which is "2023" followed by the separator.

Only numeric and `String` members, scalar or array, are supported. Subsetting arrays of these structures is not supported **[impl]**. The schema says only that these structures need "special handling" and that storing the data inline is "more efficient" (XSD-C17).

---

## 10. String array descriptors

### 10.1 `dmrpp:FixedLengthStringArray`

An empty element on a `String` array. It says that each string has a fixed length and how it is padded. It appears together with `dmrpp:chunks`, with `dmrpp:compact`, or with both. Counts: [A] 30, [B] 9.

| Attribute | Observed values | Reader | Schema |
|---|---|---|---|
| `string_length` | 1, 2, 3, 7, 8, 27, 80 | parsed | required, positive integer |
| `pad` | `null_term`, `null_pad`, `space_pad` | also accepts `not_set`; any other value is an error | optional, default `not_set` |

The `pad` values match the HDF5 string-padding types. The reader trims each value according to the pad type **[impl: `DmrppArray::ingest_fixed_length_string`]**.

**QUESTION:** What are the exact trimming rules for each `pad` value?

### 10.2 `dmrpp:vlsa` and `v`

Holds the values of a variable-length string array inside the document. Counts: [A] 8, [B] 4. It is the only DMR++ child of its variable. Each `v` element, in row-major order, holds one value **[impl: `vlsa_util.cc`]**:

| Attribute of `v` | Meaning | Observed | In schema |
|---|---|---|---|
| `c` | run length: the value fills this many *consecutive* array elements; 1 if absent | [A] 5 | yes |
| `s` | the content is Base64 of a zlib-compressed string, and `s` is the uncompressed length in bytes. The writer compresses strings longer than 512 bytes. | [A] 2, [B] 1 | **no** (XSD-O1) |

```xml
<dmrpp:vlsa> <v>apple</v> <v>banana</v> <v c="2">cherry</v> <v c="3">giraffe</v> <v>hippopotamus</v> </dmrpp:vlsa>
<dmrpp:vlsa> <v s="1024">eJwVk0cSRCEI…</v> <v>palm tree</v> </dmrpp:vlsa>
```

**QUESTION:** Should it be an error if the counts do not add up to the number of array elements?

---

## 11. Fill values and chunks that are not stored

**[impl: `process_fill_value_chunks` and the code around it in `DMZ.cc`]**

1. **Some chunks missing.** If `chunkDimensionSizes` is present and there are fewer `dmrpp:chunk` elements than the number of chunks needed to cover the array, each missing chunk is filled with `fillValue`. Examples: [B] `FValue_chunks_partial_fill_compress.h5` is a 4×4 array with 2×2 chunks, of which only `[0,0]` and `[2,2]` are stored, and the rest is `-99`. [A] `buffer_chunk/simple.dmrpp` is another.
2. **No chunks at all.** If `dmrpp:chunks` contains no `dmrpp:chunk`, the whole variable is fill values. This happens both with `chunkDimensionSizes` present ([B] `comp_array2_udf_fv_chunk.h5`; [A] `nc4_unlimited_0.h5`) and with it absent ([B] `comp_scalar_udf_fv.h5`, a scalar `Structure` with `fillValue="-9999.9 32767 999.9"`).
3. **Structures.** Fill chunks for a `Structure` are built from the record size given by `structOffset` (if present) and the fill values listed for each member.

The schema states that "no `chunk` elements" means contiguous storage. The reader and the examples instead treat it as "all fill values" (XSD-C4).

**QUESTION:** What should happen when chunks are missing and there is no `fillValue`?

---

## 12. Data-reference resolution (summary)

For each `dmrpp:chunk` or `dmrpp:block`:

1. Use the element's `href` if it has one; otherwise use `Dataset/@dmrpp:href`.
2. If the value has no scheme, treat it as a path under the catalog root (`file://`).
3. The URL is trusted if `trust` or `dmrpp:trust` on the element, or `dmrpp:trust` on `Dataset` (when the dataset href is used), is literally `true`.
4. Read `nBytes` bytes starting at `offset`. For a linked-block chunk (§7), read every block in order and join them. For `dmrpp:block` (§8), join all blocks of the variable.
5. Undo the `compressionType` filters in reverse order.
6. Swap bytes if `byteOrder` differs from the host's.
7. Place the result at `chunkPositionInArray` (element indices), or at the origin if there is none.

---

## 13. Evaluation of `dmrpp.xsd`

The schema is a separate, importable definition of the DMR++ vocabulary. It declares global elements (`chunks`, `chunkDimensionSizes`, `chunk`, `block`, `FixedLengthStringArray`, `vlsa`, `compact`, `missingdata`, `specialstructuredata`) and global attributes for `Dataset` (`href`, `trust`, `version`, `s3`, `s3credentials`). Placing these within the DMR is left to the main DMR schema.

> **Update:** Schema version 1.1 (19 November 2026) applies the changes suggested in §13.5. It also restricts `DIO` to `off` and `deflateLevel` to 0–9, and adds a filter vocabulary for `compressionType`. The namespace URI is unchanged. Every well-formed example in [A], [B] and [C] validates against 1.1, except for the [A] problems listed in §14.2. The evaluation below is of version 1.0.

Each item below is classified as **consistent**, **new** (information not in Draft 0.1), **old or incomplete** (behind the documents or the reader), or **contradictory**.

### 13.1 Consistent with the documents and the reader

- Namespace URI. Element names and attribute names (apart from the gaps in §13.3).
- `attributeFormDefault="unqualified"` (§2.3).
- The "Hyrax deviation" notes that the reader needs the literal `dmrpp:` prefix.
- `dmrpp:href` is required. Chunk and block `href` override it.
- `ByteOrder` takes the values `LE` and `BE`.
- `compressionType` lists filters in the order they were applied; decoding reverses that order.
- `deflateLevel` has one level per `deflate`.
- `structOffset` omits the first member and ends with the total record size.
- `offset` and `nBytes` are required, unsigned 64-bit.
- `chunkPositionInArray` *lexical* form: `\[[0-9]+(,[0-9]+)*\]`.
- `chunkDimensionSizes` is optional, then `chunk+` or `block+` (or neither).
- `FixedLengthStringArray`: `string_length` is required; `pad` is one of four values, including `not_set`.
- `v@c` is a run-length count.
- `compact`, `missingdata` and `specialstructuredata` contain Base64.
- `LBChunk`, `LinkedBlockIndex` and `DIO` exist as described, and match [B] and [C].

### 13.2 New information supplied by the schema (now used in this draft)

| ID | Item | Used in |
|---|---|---|
| N1 | History of `dmrpp:trust` (CMR-supplied URLs); advice not to use it for access control; default `false` | §4.3 |
| N2 | `dmrpp:version` is the DMR++ software version and can carry a build suffix (`3.21.1-1090`) | §4.2 |
| N3 | Meaning of `dmrpp:s3` and `dmrpp:s3credentials` | §4.4 |
| N4 | Blocks are joined *before* filters are undone | §8.1 |
| N5 | Why `LBChunk` exists (HDF4 chunks that are not atomic) | §7.1 |
| N6 | `fm` exists because write-time filters can fail, and direct I/O needs it | §6.6 |
| N7 | `byteOrder` defaults to `LE` | §5.2.4 |
| N8 | Why `missingdata` exists (values computed by HDF-EOS2-style APIs) | §9.2 |

### 13.3 Old or incomplete (the schema lags the documents or the reader)

| ID | Gap |
|---|---|
| O1 | No `s` attribute on `v`, so [A] `t_big_vl_str_dset*.h5` and [B] `t_big_vl_str_dset2.h5` would fail validation. The zlib+Base64 encoding of `v` is not described. |
| O2 | The internal formats are not described: zlib compression of `missingdata` for arrays with more than one element, the `;`-separated nested-Base64 stream of `specialstructuredata`, and the rows-of-strings layout of fixed-length strings in `compact`. |
| O3 | `fillValue` syntax is not described: Structure lists and the `unsupported-*` sentinels (§5.2.3). |
| O4 | The chunk type allows only an unprefixed `trust`. Placeholder substitution writes `dmrpp:trust` on chunks, which would not validate (no `xs:anyAttribute`). |
| O5 | `dmrpp:s3`, `dmrpp:s3credentials` and `dmrpp:trust="true"` are injected into `dmrpp:chunk` by placeholder substitution, but the chunk type does not allow them. The placeholder tokens themselves are not mentioned. |
| O6 | Behaviors not mentioned: an absent `structOffset` means packed; `LinkedBlockIndex` blocks share `chunkPositionInArray` and appear in order; single-`block` files still occur; `dmrpp:chunk` directly under a variable (older [A] files). |
| O7 | The filter vocabulary (`deflate`, `shuffle`, `fletcher32`, `lzw`, `tiffpred:…`) is not listed. `compressionType` is just `xs:string`. |

### 13.4 Contradictions

| ID | The schema says | The evidence says |
|---|---|---|
| C1 | `chunkPositionInArray` is a *chunk-space* index (`[1,0,0,0]` for the second chunk). | Every example uses *element* indices. In [C], 10-row chunks of a 2030-row array are at `[0,0]` … `[2020,0]`; `[1910,0]` is element row 1910, not chunk 1910. |
| C2 | `chunkPositionInArray` is `use="required"` (a schema comment raises doubt). | It is absent on contiguous-data chunks: 120 in [A], 86 in [B]. |
| C3 | If `dmrpp:chunks` is present, the variable is chunked; if absent, it is contiguous. | `dmrpp:chunks` also wraps contiguous data (no `chunkDimensionSizes`, one `chunk`). It is empty when used with `compact` or `specialstructuredata`. Contiguous data in newer files is never written without it. |
| C4 | No `chunk` elements means contiguous storage. | No `chunk` elements means all fill values (§11). |
| C5 | `chunkDimensionSizes` is omitted for a variable that is all fill values. | Both forms occur (§11, item 2). |
| C6 | Chunks can override `compressionType` individually. | `ChunkType` has no such attribute, and the reader has no per-chunk filters. |
| C7 | `v` is in the DMR++ namespace (`elementFormDefault="qualified"`). | Every document writes an unprefixed `<v>`, which falls in the DAP4 namespace. |
| C8 | `missingdata` holds values that are "missing (i.e., fill values)". | It holds computed values (lat/lon, index values). The schema's own second paragraph says so. |
| C9 | `trust` and `LBChunk` are `xs:boolean`, so `1` and `0` are valid. | The reader recognizes only the literal `true`. |
| C10 | `FixedLengthStringArray` marks a "variable-length array of fixed-length strings" and is used "in conjunction with the dmrpp:chunk element". | The arrays have fixed shapes. The element is also used with `dmrpp:compact` only. |
| C11 | The `structOffset` example (`Int32`, `Int64`, `Float32` gives `4 12 16`) describes a packed record. | This is not a contradiction of meaning, but it conflicts with the [B] note that packed records have no `structOffset` (§5.2.5). |
| C12 | A comment says there can be several `chunkDimensionSizes`. | The schema allows at most one (an internal inconsistency). |
| C13 | Missing `byteOrder` means `LE`. | The reader does no swapping (host order). The two differ only on big-endian hosts. |
| C14 | `DIO`: "If not given, assume the value is false." | `DIO` is not a boolean. When it is absent, direct I/O is allowed. |
| C15 | `fm`: each bit shows whether a filter "was applied". | In HDF5 a set bit means the filter was *skipped*. The direction should be stated. |
| C16 | `LBChunk`: "the `LinkedBlockIndex` attribute on each block". | `LinkedBlockIndex` is on `dmrpp:chunk` elements, and only on chunks that are actually split (§7.2). |
| C17 | `specialstructuredata` exists because inline storage is "more efficient". | It is used for compounds with string members, which cannot be read directly from the file. |
| C18 | Interpretation of `compact` depends on byte order. | The reader ignores `byteOrder` for inline data (§9). |

### 13.5 Suggested schema changes (for discussion)

These fix the contradictions without changing any existing document:

- C1, C2: redefine `chunkPositionInArray` as element indices, and make it optional (required only when `chunkDimensionSizes` is present).
- C3, C4, C5: rewrite the documentation for `chunks`, `chunk` and `chunkDimensionSizes`.
- C6: remove the per-chunk override sentence.
- O1: add `s` to `v`.
- C7: either declare `v` unqualified (local `form="unqualified"`), or change the writer to emit `dmrpp:v`.
- O4, O5: allow `dmrpp:trust` (and the injected `dmrpp:s3*`) on `chunk` and `block`, or document that the substituted output is not meant to be validated.
- C8, C10, C14, C15, C16, C17: correct the wording.

---

## 14. Implementation and corpus issues

These are listed for the implementation work that will come before a normative specification.

### 14.1 Reader

| Issue | Effect |
|---|---|
| `process_multi_blocks_chunk` only finds `LinkedBlockIndex` when it is the last attribute. The fallback loop tests `LBI_attr.name()` instead of `attr.name()`. | If `LinkedBlockIndex` is anywhere else, data is silently wrong (§7.3). [C] is not affected because it always writes the attribute last. |
| The `fm` filter mask is dropped for linked-block chunks and ignored in normal decoding. | A chunk whose write-time filter failed would be decoded wrongly. |
| Inline data (`compact`, `missingdata`, `specialstructuredata`) is never byte-swapped. | Wrong values on big-endian hosts, or if a producer writes big-endian inline data. |
| A missing `byteOrder` means host order, but the schema says `LE`. | Same risk as above. |
| Namespaces are ignored and the literal `dmrpp:` prefix is required. | Valid XML with another prefix fails; invalid XML (no declaration) is accepted. |
| Version-less documents have their filter list reordered and duplicates removed (§4.2). | A version-less `deflate deflate` or `fletcher32 shuffle …` pipeline would decode wrongly. [C] has no version. |
| `trust` and `LBChunk` recognize only the literal `true`. | Schema-valid `1` is ignored. |

### 14.2 Corpus [A] (dmrpp_module test data)

| Files | Issue |
|---|---|
| `dmrpp/a3_local_twoD.h5`, 8 × `dmrpp/chunked_{gzipped,shufzip}_*.h5`, 2 × `s3/AIRS.2015.12.3*.dmrpp` | `deflate_level` written instead of `deflateLevel` (1,563 times). The reader ignores it, so direct I/O is never chosen for these files. |
| `dmrpp/nc4_group_atomic.h5` | `byteOrde="LE"` (misspelled) on 4 `dmrpp:chunks`. |
| `dmrpp/a2_local_twoD.h5`, `dmrpp/a3_local_twoD.h5` | No `Dataset/@dmrpp:href`, which the reader requires. |
| `dmrpp/h4mapper_testsds_splice.dmrpp` | Uses the `dmrpp:` prefix without declaring it (not namespace-well-formed). |
| `h4_dmrpp/SDS_simple.hdf`, `h4_dmrpp/SDS_simple_comp.hdf` | A stray line of text before the XML declaration (not well-formed). |
| `fill_value/FValue.h5`, `fill_value/FValue_chunk.h5` | `https://` in both namespace URIs. |
| `dmrpp/coads_climatology.dmrpp` | Commented-out `dmrpp:byteStream` (`uuid`, `md5`), which appears nowhere else. |

[B] has none of these problems: all 129 files are well-formed and use the standard namespaces. [C] is well-formed but has no `dmrpp:version` and no `build_dmrpp_metadata`.

---

## 15. Open questions

Resolved since Draft 0.1:

- Whether this document is normative or descriptive: it is descriptive.
- An example of `LBChunk`: now provided by [C] (§7).
- The meaning of `fm`: partly answered by the schema; the bit direction is still open (Q9).
- The scope of `s3` and `s3credentials`: they are declared in the schema (Q3 remains).

Still open:

1. Should the namespace URI be required, and is the `https` form allowed (§2.2)?
2. Should the `vlsa` value element be `v` or `dmrpp:v` (§2.4, C7)?
3. Is any reader expected to use `dmrpp:s3` and `dmrpp:s3credentials` (§4.4)?
4. Should version-less documents keep the legacy filter reordering (§4.2)?
5. Are the placeholder tokens part of the format or a deployment detail (§4.5)?
6. What is the canonical text form of numeric `fillValue` (§5.2.3)?
7. What does a missing `byteOrder` mean: `LE` (schema) or host order (reader) (§5.2.4, C13)?
8. Should `structOffset` always be written, or only for padded records? Why does [B] `comp_scalar_udf_nfv_right.h5` not have it (§5.2.5)?
9. What is the bit direction of `fm`, and must decoders honor it (§6.6, C15)?
10. Is the stored size of an edge chunk always the full chunk shape (§6.3)?
11. Which tool writes per-chunk `href`, and what is the `_mvs.h5` format (§6.4)?
12. Is the older form, `dmrpp:chunk` without a wrapper, deprecated (§6.5)?
13. Should linked-block chunks be redesigned, for example with nested `dmrpp:block` (§7.4)?
14. Is inline binary data always little-endian (§9)?
15. Is `specialstructuredata` always packed with no alignment (§9.3)?
16. What are the exact trimming rules for each `pad` value (§10.1)?
17. Must the `vlsa` run lengths add up to the number of elements (§10.2)?
18. What happens when chunks are missing and there is no `fillValue` (§11)?
19. Is `dmrpp:byteStream` historical (§14.2)?
20. Are the schema changes in §13.5, as made in schema version 1.1, acceptable? In particular, is it right to accept both an unprefixed `v` and `dmrpp:v` (C7)?

---

## Appendix A. Vocabulary summary

### A.1 Elements

| Element | Parent | Content | [A] | [B] | [C] |
|---|---|---|---|---|---|
| `dmrpp:chunks` | variable | `chunkDimensionSizes?`, then `chunk*` or `block*` | 2,030 | 265 | 46 |
| `dmrpp:chunkDimensionSizes` | `dmrpp:chunks` | integers separated by spaces | 1,890 | 143 | 46 |
| `dmrpp:chunk` | `dmrpp:chunks`; variable (older form) | empty | 4,601 | 5,496 | 10,553 |
| `dmrpp:block` | `dmrpp:chunks` | empty | 3 | 10 | 0 |
| `dmrpp:compact` | variable | Base64 | 30 | 4 | 0 |
| `dmrpp:missingdata` | array variable or scalar `Byte` | Base64 (zlib if more than one element) | 2 | 29 | 0 |
| `dmrpp:specialstructuredata` | `Structure` | Base64 (§9.3) | 5 | 3 | 0 |
| `dmrpp:FixedLengthStringArray` | `String` array | empty | 30 | 9 | 0 |
| `dmrpp:vlsa` | `String` array | `v+` | 8 | 4 | 0 |
| `v` (no prefix) | `dmrpp:vlsa` | string, or zlib+Base64 if `s` is present | 37 | 22 | 0 |

Counts are text-search counts across all files in each corpus.

### A.2 Attributes

| Attribute | On | [A] | [B] | [C] | Reader | Schema |
|---|---|---|---|---|---|---|
| `dmrpp:href` | `Dataset` | 184 | 129 | 1 | required | required |
| `dmrpp:version` | `Dataset` | 90 | 127 | 0 | yes | yes |
| `dmrpp:trust` | `Dataset` | 1 | 0 | 0 | yes | yes |
| `dmrpp:s3`, `dmrpp:s3credentials` | `Dataset` (and chunks after substitution) | 0 | 0 | 0 | ignored | `Dataset` only |
| `compressionType` | `chunks` | 1,617 | 102 | 46 | yes | yes |
| `deflateLevel` | `chunks` | 24 | 92 | 46 | yes | yes |
| `fillValue` | `chunks` | 180 | 212 | 46 | yes | yes |
| `byteOrder` | `chunks` | 155 | 231 | 46 | yes | yes |
| `structOffset` | `chunks` | 25 | 5 | 0 | yes | yes |
| `LBChunk` | `chunks` | 0 | 0 | 6 | yes | yes |
| `DIO` | `chunks` | 0 | 3 | 0 | yes | yes |
| `offset`, `nBytes` | `chunk`, `block` | all | all | all | required | required |
| `chunkPositionInArray` | `chunk` | 4,477 | 5,410 | 10,553 | optional | required (C2) |
| `href` | `chunk`, `block` | 14 | 23 | 0 | yes | yes |
| `trust` / `dmrpp:trust` | `chunk`, `block` | 0 | 0 | 0 | yes | `trust` only |
| `fm` | `chunk` | 0 | 0 | 0 | yes | yes |
| `LinkedBlockIndex` | `chunk` | 0 | 0 | 2,032 | yes (must be last) | yes |
| `string_length`, `pad` | `FixedLengthStringArray` | 30 | 9 | 0 | yes | yes |
| `c` | `v` | 5 | 0 | 0 | yes | yes |
| `s` | `v` | 2 | 1 | 0 | yes | **no** (O1) |
| `deflate_level` (misspelling) | `chunks` | 1,563 | 0 | 0 | no | no |
| `byteOrde` (misspelling) | `chunks` | 4 | 0 | 0 | no | no |

## Appendix B. Example index

| Feature | Recommended example |
|---|---|
| Typical chunked + deflate | [B] `two_arrays_16x16.h5.dmrpp` |
| `DIO="off"` | [B] `two_arrays_16x16.h5_no_dio.dmrpp` |
| `fletcher32 shuffle deflate deflate` | [A] or [B] `SDS_fle_shuf_2def.h5.dmrpp` |
| Partially stored (fill) chunks | [B] `FValue_chunks_partial_fill_compress.h5.dmrpp` |
| Whole variable is fill (chunked / scalar) | [B] `comp_array2_udf_fv_chunk.h5.dmrpp` / `comp_scalar_udf_fv.h5.dmrpp` |
| Compact | [A] or [B] `compact_example.h5.dmrpp` |
| `missingdata`: array / one-element array / scalar `Byte` | [B] `cea_ll.hdf.dmrpp` / `t_cf_1dllzs2.h5.dmrpp` / `grid_2_2d_sin.h5.dmrpp` |
| Per-chunk `href` to `_mvs.h5` | [B] `grid_2_2d_sin.h5.dmrpp` |
| `vlsa`, plain / compressed (`s`) / run-length (`c`) | [A] `utf8_vlen_str.h5` / `t_big_vl_str_dset2.h5` / `string_arrays/vl_str_dup_array.h5` |
| Fixed-length string array | [B] `chunked_string_array_c.h5.dmrpp` |
| `specialstructuredata` | [B] `compound_simple_scalar_memb_str_array.h5.dmrpp` |
| `structOffset`, padded / packed with / packed without | [B] `comp_scalar_udf_nfv.h5` / [A] `fill_value/comp_scalar_udf_nfv_right_offset.h5` / [B] `comp_scalar_udf_nfv_right.h5` |
| `dmrpp:block`, SDS / Vdata `Structure` / single legacy block | [A] or [B] `SDS_unlimited_2.hdf` / [B] `vdata_packed_linked_blocks.hdf` / [B] `sds_two_linked_blocks.hdf.legacy` |
| `LBChunk` + `LinkedBlockIndex` | [C] `MYD09.A2019003.2040.006.2019005020913.hdf.dmrpp` |
| Older unwrapped `dmrpp:chunk` | [A] `dmrpp/d_int.h5.dmrpp` |
