# BES Modules Deep Dive

This document describes the function of the directories under `modules/`. Some are loadable BES modules, some are shared test/support directories, and a few appear vestigial or only partially present in this checkout.

## How modules fit the framework

A typical BES module does some combination of the following during `initialize(modname)`:

- register a `BESRequestHandler` so the framework can build DAS, DDS, DMR, data, help, or version responses for a container type,
- register a `BESTransmitter` so `return as ...` can serialize a response in a non-default format,
- register one or more XML commands or response handlers,
- register catalogs or container storage backends,
- register server-side functions with libdap,
- advertise formats or services.

In practice the module tree breaks down into five groups:

- reader modules that map an external format or remote resource into DAP responses,
- writer/transformer modules that serialize DAP responses as some other format,
- function modules that add server-side computation,
- catalog/storage modules that add discovery or alternate container backends,
- support-only directories used by tests, data, or packaging.

## Reader modules

### `asciival`

Adds ASCII and CSV-style data value output for DAP responses. Historically this was a standalone asciival tool; in BES it registers transmitters that turn already-built DAP data responses into ASCII-oriented output.

### `csv_handler`

Reads CSV files and exposes them through Hyrax as DAP datasets. It registers a request handler and a file-based catalog/container path for CSV content.

### `dmrpp_module`

Reads DMR++ sidecar metadata and serves data by byte-range access rather than by linking directly to HDF5 for each read. This is the module behind Hyrax's efficient cloud/object-store access pattern.

Key responsibilities:

- build DMR, DAP4 data, DAS, DDS, and DAP2 data responses from DMR++ metadata,
- use curl-based transfer machinery and optional concurrency,
- support `return as dmrpp`,
- install NGAP-owned container storage under `dmrpp_module/ngap_container/`.

This is one of the most strategically important modules in the tree because it is the bridge between Hyrax's DAP abstractions and cloud-native byte-range retrieval.

### `fits_handler`

Reads FITS scientific data files and exposes them through DAP responses. It behaves like the classic file-format handlers: request handler plus file-based catalog integration.

### `freeform_handler`

Reads datasets described by FreeForm/FFND format descriptions. It supports binary, ASCII, and dBASE-style sources and can map them into arrays or sequences depending on the `.fmt` description.

### `gdal_module`

Combines the older GDAL reader and writer code into one module to avoid duplicate GDAL linkage.

On the reader side it serves geospatial raster formats such as:

- GeoTIFF,
- GRIB,
- JPEG2000.

On the writer side it also supports geospatial output formats via transmitters.

### `hdf4_handler`

Reads HDF4 and HDF-EOS2 content, including CF-oriented remapping logic needed for many NASA products. It is heavily specialized around scientific metadata normalization, coordinate handling, and DAP response generation for HDF4-family sources.

### `hdf5_handler`

Reads HDF5 and HDF-EOS5 content and contains substantial logic for both direct DAP mapping and CF-friendly remapping. It also supports direct DMR generation and features aimed at large NASA archives and netCDF-4-like HDF5 content.

### `ncml_module`

Treats NcML documents as virtual dataset definitions.

Its role is broader than simple file reading:

- add or alter metadata,
- rename or remove variables,
- add synthetic variables,
- wrap another dataset,
- build aggregated virtual datasets such as union and joinNew,
- optionally cache aggregations.

This module is effectively a dataset transformation layer inside BES.

### `netcdf_handler`

Reads netCDF and netCDF-4 files and maps them into DAP responses. This is one of the core production handlers for local scientific datasets.

### `xml_data_handler`

Builds XML output containing data values. It is intended for highly constrained data-value inspection rather than bulk delivery and is useful for workflows that want machine-readable value snippets.

## Writer and transformation modules

### `fileout_covjson`

Adds `return as covjson` output. It serializes supported coverage-style DAP responses into CoverageJSON, especially for CF-style grids, points, point series, and vertical profiles.

### `fileout_json`

Adds JSON-oriented output formats:

- `return as json`,
- `return as ijson`.

This is a serializer module, not a reader.

### `fileout_netcdf`

Adds NetCDF file output from DAP data responses.

It works by:

- asking the existing response object to fully read data,
- transforming that in-memory DAP view into a netCDF file,
- streaming the file back.

It supports NetCDF3 and NetCDF4-style returns and is one of the canonical examples of BES's separation between response building and response transmission.

### `usage`

Adds non-DAP informational responses such as usage/help-oriented outputs. Historically this package grouped human-facing handlers like usage, ASCII, and HTML-form style responses.

### `w10n_handler`

Adds `return as w10n` JSON output and related path-info behavior. In practice this is a specialized JSON-facing module for a particular API/view of dataset information rather than a raw format reader.

## Function modules

These modules do not primarily read a file format. They register libdap server-side functions that can be used inside constraint expressions or related DAP processing.

### `debug_functions`

Registers testing and fault-injection functions such as:

- `abort`,
- `sleep`,
- `sumUntil`,
- `error`.

This is useful for exercising timeout, signal, and error paths in the framework.

### `functions`

Registers general-purpose server-side functions, including:

- grid and geospatial functions,
- scaling and masking helpers,
- array construction helpers,
- tabular and range functions,
- ROI and bounding-box operations,
- identity/version helpers,
- optional STARE and GDAL-backed functions when available.

This module extends libdap computation rather than storage.

### `ugrid_functions`

Registers functions for subsetting unstructured-grid datasets that follow UGRID conventions. This is computation on top of an existing dataset, not a standalone file reader.

## Catalog and storage modules

These modules extend discovery, indirection, or container resolution more than they extend file-format decoding.

### `cmr_module`

Adds a catalog backed by NASA CMR. Depending on configuration, it can also install container storage used to turn catalog results into accessible containers. Its focus is remote dataset discovery and access mediation rather than direct file parsing.

### `gateway_module`

Provides a gateway to remote resources and services. It is used when BES should act as a controlled proxy or adapter for external HTTP resources. It also contributes path-info behavior and allowed-host/proxy controls.

### `httpd_catalog_module`

Adds the `RemoteResources` catalog and lets BES browse HTTP/HTTPS-backed collections as if they were catalog nodes. This is catalog/discovery infrastructure, not a decoder by itself.

### `s3_reader`

Adds an S3-backed request handler and container storage. Its job is to make S3 objects look like BES containers that other framework paths can work with.

### `dmrpp_module/ngap_container`

Not a top-level directory, but important enough to call out separately. This code provides the NGAP-owned container and storage implementation used by the DMR++ module for NGAP-specific access patterns and caching behavior.

## Support, test, and non-plugin directories

### `common`

Shared test/build support used by module test suites. This is not a runtime plugin.

### `data`

Regression and sample datasets used by module and system tests. Not a loadable module.

### `docs`

Placeholder/documentation directory under `modules/`. It does not contain a loadable module in this checkout.

### `ngap_module`

This directory appears vestigial or incomplete in the current checkout. It contains generated dependency files and test scaffolding, but the live NGAP implementation used by the codebase is under `modules/dmrpp_module/ngap_container/`.

## Module-by-module summary table

| Directory | Role |
| --- | --- |
| `asciival` | ASCII/CSV-style transmitter for DAP data values |
| `cmr_module` | NASA CMR-backed catalog and optional container access |
| `common` | shared test/build support |
| `csv_handler` | CSV reader |
| `data` | test data |
| `debug_functions` | fault-injection and debug server-side functions |
| `dmrpp_module` | DMR++ reader, range-access execution, DMR++ transmitter |
| `docs` | support/docs directory |
| `fileout_covjson` | CoverageJSON serializer |
| `fileout_json` | JSON and instance-JSON serializers |
| `fileout_netcdf` | NetCDF file serializer |
| `fits_handler` | FITS reader |
| `freeform_handler` | FreeForm/FFND-based reader |
| `functions` | general DAP server-side functions |
| `gateway_module` | remote gateway/proxy access module |
| `gdal_module` | geospatial raster reader plus geospatial output writers |
| `hdf4_handler` | HDF4/HDF-EOS2 reader |
| `hdf5_handler` | HDF5/HDF-EOS5 reader |
| `httpd_catalog_module` | HTTP-backed remote catalog |
| `ncml_module` | virtual dataset and aggregation layer |
| `netcdf_handler` | netCDF/netCDF-4 reader |
| `ngap_module` | incomplete/legacy top-level directory in this checkout |
| `s3_reader` | S3-backed container/storage reader |
| `ugrid_functions` | UGRID subsetting functions |
| `usage` | usage/help-style non-DAP outputs |
| `w10n_handler` | specialized JSON-oriented output/path-info module |
| `xml_data_handler` | XML value-output handler |

## Patterns across the module tree

Three recurring patterns show up across the modules:

- Reader modules register a request handler and often a file or remote container backend.
- Writer modules usually register a request handler for help/version plus one or more named transmitters.
- Function modules mostly register libdap server-side functions and do little or no container work.

That consistency is useful when reading unfamiliar code:

- if a module adds a `BESRequestHandler`, it probably contributes new dataset-reading behavior;
- if it adds a `BESTransmitter`, it probably contributes a new `return as ...` format;
- if it adds libdap functions, it is extending computation rather than I/O;
- if it adds catalogs or container storage, it is extending discovery or access indirection.

## Practical takeaway

From an operational point of view, the most central production modules in this tree are:

- `netcdf_handler`,
- `hdf4_handler`,
- `hdf5_handler`,
- `dmrpp_module`,
- `fileout_netcdf`,
- `functions`,
- `gateway_module`,
- `s3_reader`,
- `cmr_module`,
- `ncml_module`.

The rest either add alternate views/serializations, specialized APIs, or support the test and development workflow around those core paths.
