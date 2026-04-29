# Module Configuration Key Map

This document maps the installed module-side `*.conf.in` templates under `modules/` to the C++ names that read those keys and to representative usage sites.

Scope:

- Included templates:
  - `modules/asciival/ascii.conf.in`
  - `modules/cmr_module/cmr.conf.in`
  - `modules/csv_handler/csv.conf.in`
  - `modules/debug_functions/debug_functions.conf.in`
  - `modules/dmrpp_module/dmrpp.conf.in`
  - `modules/fileout_covjson/focovjson.conf.in`
  - `modules/fileout_json/fojson.conf.in`
  - `modules/fileout_netcdf/fonc.conf.in`
  - `modules/fits_handler/fits.conf.in`
  - `modules/freeform_handler/ff.conf.in`
  - `modules/functions/functions.conf.in`
  - `modules/gateway_module/gateway.conf.in`
  - `modules/gdal_module/gdal.conf.in`
  - `modules/hdf4_handler/h4.conf.in`
  - `modules/hdf5_handler/h5.conf.in`
  - `modules/httpd_catalog_module/httpd_catalog.conf.in`
  - `modules/ncml_module/ncml.conf.in`
  - `modules/netcdf_handler/nc.conf.in`
  - `modules/s3_reader/s3.conf.in`
  - `modules/ugrid_functions/ugrid_functions.conf.in`
  - `modules/usage/usage.conf.in`
  - `modules/xml_data_handler/xml_data_handler.conf.in`
- Excluded on purpose: test-only configs, `modules/common`, `modules/data`, `modules/docs`, and the incomplete top-level `modules/ngap_module`
- "C++ name" means a macro, `const`/`constexpr`, or the dynamic string pattern used by the code

Notes:

- I used `docs/BES_Modules.md` to decide which `modules/` directories count as installed/runtime modules.
- Several module templates mostly contribute framework keys such as `BES.Include`, `BES.modules+=...`, `BES.module.<name>`, and `BES.Catalog.<catalog>.TypeMatch+=...`. Those are documented once below instead of repeated in every table.
- Some keys in the templates appear stale, mismatched, or only documented. Those are called out explicitly instead of being silently treated as live configuration.

## Common module patterns

These keys recur across most module configs and resolve through framework code rather than module-specific `*Names.h` headers.

| Config key pattern | C++ name / symbol | Representative usage | Notes |
| --- | --- | --- | --- |
| `BES.Include` | `BES_INCLUDE_KEY` in `dispatch/TheBESKeys.cc:52` and `dispatch/kvp_utils.cc:46` | `dispatch/TheBESKeys.cc`, `dispatch/kvp_utils.cc` | Used by almost every module config to pull in `dap.conf` first. |
| `BES.modules+=<module>` | raw string `"BES.modules"` | `dispatch/BESModuleApp.cc:93` | Appends one module name to the global module list. |
| `BES.module.<name>` | dynamic key pattern `"BES.module." + name` | `dispatch/BESModuleApp.cc:119` | Maps module names to shared libraries. |
| `BES.Catalog.<catalog>.TypeMatch` | dynamic key pattern `"BES.Catalog." + n + ".TypeMatch"` | `dispatch/BESCatalogUtils.cc:118` | Reader/catalog modules use this to map names to handlers. |
| `BES.Catalog.<catalog>.RootDirectory` | dynamic key pattern `"BES.Catalog." + n + ".RootDirectory"` | `dispatch/BESCatalogUtils.cc:79` | Some modules need this only because the framework insists on it. |
| `BES.Catalog.<catalog>.FollowSymLinks` | dynamic key pattern `"BES.Catalog." + n + ".FollowSymLinks"` | `dispatch/BESCatalogUtils.cc:156` | Usually optional. |
| `BES.Catalog.<catalog>.Include` | dynamic key pattern `"BES.Catalog." + n + ".Include"` | `dispatch/BESCatalogUtils.cc:107` | Usually optional. |
| `BES.Catalog.<catalog>.Exclude` | dynamic key pattern `"BES.Catalog." + n + ".Exclude"` | `dispatch/BESCatalogUtils.cc:98` | Usually optional. |
| `AllowedHosts` / `AllowedHosts+=...` | `ALLOWED_HOSTS_BES_KEY` in `http/AllowedHosts.h:39` | `http/AllowedHosts.cc:54`, `http/CurlUtils.cc:1151` | Shared remote-host allow-list used by several modules. |
| `Http.cache.effective.urls` | `HTTP_CACHE_EFFECTIVE_URLS_KEY` in `http/HttpNames.h:64` | `http/EffectiveUrlCache.cc:170` | Used directly by `dmrpp.conf.in`. |
| `Http.cache.effective.urls.skip.regex.pattern` | `HTTP_CACHE_EFFECTIVE_URLS_SKIP_REGEX_KEY` in `http/HttpNames.h:65` | `http/EffectiveUrlCache.cc:179` | Used directly by `dmrpp.conf.in`. |
| `DAP.GlobalMetadataStore.*` | `PATH_KEY`, `PREFIX_KEY`, `SIZE_KEY`, `LEDGER_KEY` in `dap/GlobalMetadataStore.cc:98-101` | `dap/GlobalMetadataStore.cc` | Referenced in `modules/netcdf_handler/nc.conf.in` comments because netCDF can optionally rely on the framework MDS. |

## Small modules with only loader keys

These module templates only contribute include/module-loader settings and no module-specific runtime keys in the config template:

- `modules/asciival/ascii.conf.in`
- `modules/debug_functions/debug_functions.conf.in`
- `modules/usage/usage.conf.in`
- `modules/ugrid_functions/ugrid_functions.conf.in`
- `modules/xml_data_handler/xml_data_handler.conf.in`

For those, the live configuration keys are just:

- `BES.Include`
- `BES.modules+=<module>`
- `BES.module.<module>`

## `modules/cmr_module/cmr.conf.in`

| Config key | C++ name / symbol | Representative usage | Notes |
| --- | --- | --- | --- |
| `BES.module.cmr` | dynamic `BES.module.<name>` | `dispatch/BESModuleApp.cc:119` | The module is loadable even if `BES.modules+=cmr` is left commented out. |
| `BES.modules+=cmr` | raw string `"BES.modules"` | `dispatch/BESModuleApp.cc:93` | Disabled by default in the template. |
| `CMR.Reference` | none found | no live lookup found | Looks like documentation only in the current tree. |
| `BES.Catalog.CMR.RootDirectory` | dynamic `BES.Catalog.<catalog>.RootDirectory` | `dispatch/BESCatalogUtils.cc:79` | Framework-required placeholder for catalog `CMR`. |
| `BES.Catalog.CMR.FollowSymLinks` | dynamic `BES.Catalog.<catalog>.FollowSymLinks` | `dispatch/BESCatalogUtils.cc:156` | Optional framework catalog key. |
| `BES.Catalog.CMR.Exclude` | dynamic `BES.Catalog.<catalog>.Exclude` | `dispatch/BESCatalogUtils.cc:98` | Optional framework catalog key. |
| `BES.Catalog.CMR.Include` | dynamic `BES.Catalog.<catalog>.Include` | `dispatch/BESCatalogUtils.cc:107` | Optional framework catalog key. |
| `BES.Catalog.CMR.TypeMatch+=...` | dynamic `BES.Catalog.<catalog>.TypeMatch` | `dispatch/BESCatalogUtils.cc:118` | Type map for the `CMR` catalog. |
| `CMR.Collections` / `CMR.Collections+=...` | `CMR_COLLECTIONS_KEY` in `modules/cmr_module/CmrNames.h:35` | `modules/cmr_module/CmrApi.cc:726`, `modules/cmr_module/CmrCatalog.cc:89` | Required; module throws if no collections are configured. |
| `CMR.Facets` | `CMR_FACETS_KEY` in `modules/cmr_module/CmrNames.h:36` | `modules/cmr_module/CmrCatalog.cc:96` | Required; module throws if no facets are configured. |
| `CMR.host.url` | `CMR_HOST_URL_KEY` in `modules/cmr_module/CmrNames.h:37` | `modules/cmr_module/CmrApi.cc:71` | Defaults to `https://cmr.earthdata.nasa.gov/` if omitted. |
| `AllowedHosts+=...` | `ALLOWED_HOSTS_BES_KEY` in `http/AllowedHosts.h:39` | `http/AllowedHosts.cc:54` | Needed because CMR endpoints must be on the global allow-list. |

## Reader modules using only `TypeMatch`

These templates have no module-specific keys beyond the loader keys and the dynamic catalog `TypeMatch` rule:

### `modules/csv_handler/csv.conf.in`

- `BES.Catalog.catalog.TypeMatch+=csv:...`

### `modules/fits_handler/fits.conf.in`

- `BES.Catalog.catalog.TypeMatch+=fits:...`

### `modules/freeform_handler/ff.conf.in`

- `BES.Catalog.catalog.TypeMatch+=ff:...`

In all three cases the key is resolved by the framework through `dispatch/BESCatalogUtils.cc:118`.

## `modules/dmrpp_module/dmrpp.conf.in`

| Config key | C++ name / symbol | Representative usage | Notes |
| --- | --- | --- | --- |
| `BES.Include=fonc.conf` | `BES_INCLUDE_KEY` in `dispatch/TheBESKeys.cc:52` | `dispatch/TheBESKeys.cc` | Added specifically so DMR++ code can see `FONc.ClassicModel`. |
| `BES.Catalog.catalog.TypeMatch+=dmrpp:...` | dynamic `BES.Catalog.<catalog>.TypeMatch` | `dispatch/BESCatalogUtils.cc:118` | Registers `.dmrpp` handling. |
| `DMRPP.UseParallelTransfers` | `DMRPP_USE_TRANSFER_THREADS_KEY` in `modules/dmrpp_module/DmrppNames.h:44` | module name macro definition confirmed; active read is in DMR++ runtime code | Present as a commented optional override. |
| `DMRPP.MaxParallelTransfers` | `DMRPP_MAX_TRANSFER_THREADS_KEY` in `modules/dmrpp_module/DmrppNames.h:45` | module name macro definition confirmed; active read is in DMR++ runtime code | Present as a commented optional override. |
| `DMRPP.UseObjectCache` | `DMRPP_USE_OBJECT_CACHE_KEY` in `modules/dmrpp_module/DmrppNames.h:40` | module name macro definition confirmed; active read is in DMR++ runtime code | Controls object-memory caches. |
| `DMRPP.ObjectCacheEntries` | `DMRPP_OBJECT_CACHE_ENTRIES_KEY` in `modules/dmrpp_module/DmrppNames.h:41` | module name macro definition confirmed; active read is in DMR++ runtime code | Cache entry count. |
| `DMRPP.ObjectCachePurgeLevel` | `DMRPP_OBJECT_CACHE_PURGE_LEVEL_KEY` in `modules/dmrpp_module/DmrppNames.h:42` | module name macro definition confirmed; active read is in DMR++ runtime code | Cache purge fraction. |
| `DMRPP.Elide.Unsupported` | `ELIDE_UNSUPPORTED_KEY` in `modules/dmrpp_module/DMZ.cc:103` | `modules/dmrpp_module/DMZ.cc` | Commented optional override. |
| `DMRPP.DisableDirectIO` | `DMRPP_DISABLE_DIRECT_IO` in `modules/dmrpp_module/DmrppNames.h:51` | `modules/dmrpp_module/DmrppNames.h`, plus DMR++ request path | Commented optional override. |
| `DMRPP.UseBufferChunk` | `DMRPP_USE_BUFFER_CHUNK` in `modules/dmrpp_module/DmrppNames.h:52` | `modules/dmrpp_module/DmrppNames.h`, plus DMR++ request path | Commented optional override. |
| `CredentialsManager.config` | `CATALOG_MANAGER_CREDENTIALS` in `http/CredentialsManager.h:38` | `http/CredentialsManager.cc:269` | DMR++ comments are accurate: this is actually owned by shared HTTP credential code. |
| `Http.cache.effective.urls` | `HTTP_CACHE_EFFECTIVE_URLS_KEY` in `http/HttpNames.h:64` | `http/EffectiveUrlCache.cc:170` | Shared HTTP setting, enabled here because DMR++ frequently hits signed URLs. |
| `Http.cache.effective.urls.skip.regex.pattern` | `HTTP_CACHE_EFFECTIVE_URLS_SKIP_REGEX_KEY` in `http/HttpNames.h:65` | `http/EffectiveUrlCache.cc:179` | Shared HTTP setting. |

## `modules/fileout_covjson/focovjson.conf.in`

| Config key | C++ name / symbol | Representative usage | Notes |
| --- | --- | --- | --- |
| `FoCovJson.Tempdir` | raw string `"FoCovJson.Tempdir"` | `modules/fileout_covjson/FoDapCovJsonTransmitter.cc:95` | Temp directory for intermediate output. |
| `FoCovJson.Reference` | raw string `"FoCovJson.Reference"` | `modules/fileout_covjson/FoCovJsonRequestHandler.cc:99` | Used by help/info response path. |
| `FoCovJson.MAY_IGNORE_Z_AXIS` | raw string `"FoCovJson.MAY_IGNORE_Z_AXIS"` | `modules/fileout_covjson/FoCovJsonRequestHandler.cc:60` | Optional boolean override. |
| `FoCovJson.SIMPLE_GEO` | raw string `"FoCovJson.SIMPLE_GEO"` | `modules/fileout_covjson/FoCovJsonRequestHandler.cc:63` | Optional boolean override. |

## `modules/fileout_json/fojson.conf.in`

| Config key | C++ name / symbol | Representative usage | Notes |
| --- | --- | --- | --- |
| `FoJson.Tempdir` | raw string `"FoJson.Tempdir"` | `modules/fileout_json/FoDapJsonTransmitter.cc:90`, `modules/fileout_json/FoInstanceJsonTransmitter.cc:87` | Temp directory for JSON serializers. |
| `FoJson.Reference` | raw string `"FoJson.Reference"` | `modules/fileout_json/FoJsonRequestHandler.cc:82` | Used by help/info response path. |

## `modules/fileout_netcdf/fonc.conf.in`

| Config key | C++ name / symbol | Representative usage | Notes |
| --- | --- | --- | --- |
| `FONc.Tempdir` | `FONC_TEMP_DIR_KEY` in `modules/fileout_netcdf/FONcNames.h:9` | `modules/fileout_netcdf/FONcTransmitter.cc` | Temp directory for intermediate netCDF output. |
| `FONc.Reference` | raw string `"FONc.Reference"` | `modules/fileout_netcdf/FONcRequestHandler.cc:127` | Used by help/info response path. |
| `FONc.UseCompression` | `FONC_USE_COMP_KEY` in `modules/fileout_netcdf/FONcNames.h:18` | names header plus fileout netCDF transform path | |
| `FONc.ChunkSize` | `FONC_CHUNK_SIZE_KEY` in `modules/fileout_netcdf/FONcNames.h:24` | names header plus fileout netCDF transform path | |
| `FONc.ClassicModel` | `FONC_CLASSIC_MODEL_KEY` in `modules/fileout_netcdf/FONcNames.h:32` | fileout netCDF code; also `modules/dmrpp_module/DmrppRequestHandler.cc:186` | Shared with DMR++ direct-IO decision logic. |
| `FONc.RequestMaxSizeKB` | `FONC_REQUEST_MAX_SIZE_KB_KEY` in `modules/fileout_netcdf/FONcNames.h:41` | names header plus request/transform path | |
| `FONc.ReduceDim` | `FONC_REDUCE_DIM_KEY` in `modules/fileout_netcdf/FONcNames.h:35` | names header plus transform path | |
| `FONc.NoGlobalAttrs` | `FONC_NO_GLOBAL_ATTRS_KEY` in `modules/fileout_netcdf/FONcNames.h:38` | names header plus transform path | Commented optional override in the template. |
| `FONc.UseShuffle` | `FONC_USE_SHUFFLE_KEY` in `modules/fileout_netcdf/FONcNames.h:21` | names header plus transform path | Commented optional override in the template. |
| `FONc.NC3ClassicFormat` | `FONC_NC3_CLASSIC_FORMAT_KEY` in `modules/fileout_netcdf/FONcNames.h:44` | names header plus transform path | Commented optional override in the template. |

## `modules/functions/functions.conf.in`

| Config key | C++ name / symbol | Representative usage | Notes |
| --- | --- | --- | --- |
| `FUNCTIONS.stareStoragePath` | `STARE_STORAGE_PATH_KEY` in `modules/functions/stare/StareFunctions.h:34` | `modules/functions/DapFunctions.cc:114` | Optional STARE support path. |
| `FUNCTIONS.stareSidecarSuffix` | `STARE_SIDECAR_SUFFIX_KEY` in `modules/functions/stare/StareFunctions.h:35` | `modules/functions/DapFunctions.cc:115` | Optional STARE sidecar suffix. |

## `modules/gateway_module/gateway.conf.in`

| Config key | C++ name / symbol | Representative usage | Notes |
| --- | --- | --- | --- |
| `AllowedHosts` / `AllowedHosts+=...` | `ALLOWED_HOSTS_BES_KEY` in `http/AllowedHosts.h:39` | `http/AllowedHosts.cc:54` | Live shared setting; the gateway comments here match the shared HTTP allow-list behavior. |
| `Gateway.MimeTypes` / `Gateway.MimeTypes+=...` | none found | no live lookup found | The current shared HTTP code uses `Http.MimeTypes` (`http/HttpNames.h:37`, `http/HttpUtils.cc:77`), not `Gateway.MimeTypes`. This template key looks stale. |
| `Gateway.ProxyHost` | none found | no live lookup found | Current proxy code uses `Http.ProxyHost` in `http/HttpNames.h:39` and `http/ProxyConfig.cc:47`. |
| `Gateway.ProxyPort` | none found | no live lookup found | Current proxy code uses `Http.ProxyPort`. |
| `Gateway.ProxyUser` | none found | no live lookup found | Current proxy code uses `Http.ProxyUser`. |
| `Gateway.ProxyPassword` | none found | no live lookup found | Current proxy code uses `Http.ProxyPassword`. |
| `Gateway.ProxyUserPW` | none found | no live lookup found | Current proxy code uses `Http.ProxyUserPW`. |
| `Gateway.ProxyAuthType` | none found | no live lookup found | `http/ProxyConfig.cc:123` still mentions `Gateway.ProxyAuthType` in a debug message, but the actual lookup uses `Http.ProxyAuthType`. |
| `Gateway.Cache.dir` | none found | no live lookup found | No active reader found in the current tree. |
| `Gateway.Cache.prefix` | none found | no live lookup found | No active reader found in the current tree. |
| `Gateway.Cache.size` | none found | no live lookup found | No active reader found in the current tree. |

## `modules/gdal_module/gdal.conf.in`

| Config key | C++ name / symbol | Representative usage | Notes |
| --- | --- | --- | --- |
| `BES.Catalog.catalog.TypeMatch+=gdal:...` | dynamic `BES.Catalog.<catalog>.TypeMatch` | `dispatch/BESCatalogUtils.cc:118` | Registers reader-side GDAL file patterns. |
| `FONg.Tempdir` | raw string `"FONg.Tempdir"` | `modules/gdal_module/writer/GeoTiffTransmitter.cc:94` | Live for GeoTIFF writer path. |
| `FONg.Reference` | none found | no live lookup found | Looks documented but unused in current code. |
| `FONg.default_gcs` | none found | no live lookup found for this exact spelling | There is a likely template/code mismatch: `GeoTiffTransmitter.cc:108` reads `FONg.Default_GCS`, not `FONg.default_gcs`. |
| `FONg.GeoTiff.band.type.byte` | raw string `"FONg.GeoTiff.band.type.byte"` | `modules/gdal_module/writer/FONgTransform.cc:345` | Live boolean override. |

## `modules/hdf4_handler/h4.conf.in`

| Config key | C++ name / symbol | Representative usage | Notes |
| --- | --- | --- | --- |
| `BES.Catalog.catalog.TypeMatch+=h4:...` | dynamic `BES.Catalog.<catalog>.TypeMatch` | `dispatch/BESCatalogUtils.cc:118` | Registers HDF4/HDF-EOS2 files. |
| `HDF4.CacheDir` | none found | no live lookup found | Present in template but I did not find an active code reader. |
| `H4.EnableDirectDMR` | raw string `"H4.EnableDirectDMR"` | `modules/hdf4_handler/HDF4RequestHandler.cc:164-165` | Optional direct-DMR path. |
| `H4.EnableCF` | raw string `"H4.EnableCF"` | `modules/hdf4_handler/HDF4RequestHandler.cc:166` | CF-vs-generic behavior switch. |
| `H4.EnablePassFileID` | raw string `"H4.EnablePassFileID"` | `modules/hdf4_handler/HDF4RequestHandler.cc:170` | |
| `H4.DisableStructMetaAttr` | raw string `"H4.DisableStructMetaAttr"` | `modules/hdf4_handler/HDF4RequestHandler.cc:171` | |
| `H4.EnableSpecialEOS` | raw string `"H4.EnableSpecialEOS"` | `modules/hdf4_handler/HDF4RequestHandler.cc:172` | |
| `H4.DisableScaleOffsetComp` | raw string `"H4.DisableScaleOffsetComp"` | `modules/hdf4_handler/HDF4RequestHandler.cc:173`, `modules/hdf4_handler/HDFCFUtil.h:133` | |
| `H4.DisableECSMetaDataMin` | raw string `"H4.DisableECSMetaDataMin"` | `modules/hdf4_handler/HDF4RequestHandler.cc:174` | Read in several metadata paths. |
| `H4.DisableECSMetaDataAll` | raw string `"H4.DisableECSMetaDataAll"` | `modules/hdf4_handler/HDF4RequestHandler.cc:175` | Read in several metadata paths. |
| `H4.EnableEOSGeoCacheFile` | raw string `"H4.EnableEOSGeoCacheFile"` | `modules/hdf4_handler/HDF4RequestHandler.cc:178`, `modules/hdf4_handler/HDFEOS2ArrayGridGeoField.cc:137` | |
| `H4.EnableDataCacheFile` | raw string `"H4.EnableDataCacheFile"` | `modules/hdf4_handler/HDF4RequestHandler.cc:179`, `modules/hdf4_handler/HDFSPArray_RealField.cc:48` | |
| `HDF4.Cache.latlon.path` | `BESH4Cache::PATH_KEY` in `modules/hdf4_handler/BESH4MCache.cc:27` | `modules/hdf4_handler/HDF4RequestHandler.cc:199` | |
| `HDF4.Cache.latlon.prefix` | `BESH4Cache::PREFIX_KEY` in `modules/hdf4_handler/BESH4MCache.cc:28` | `modules/hdf4_handler/HDF4RequestHandler.cc:200` | |
| `HDF4.Cache.latlon.size` | `BESH4Cache::SIZE_KEY` in `modules/hdf4_handler/BESH4MCache.cc:29` | `modules/hdf4_handler/HDF4RequestHandler.cc:202` | |
| `H4.EnableMetaDataCacheFile` | raw string `"H4.EnableMetaDataCacheFile"` | `modules/hdf4_handler/HDF4RequestHandler.cc:180` | |
| `H4.Cache.metada.path` | none found | no live lookup found | Template typo; current code reads `H4.Cache.metadata.path`. |
| `H4.Cache.metadata.path` | raw string `"H4.Cache.metadata.path"` | `modules/hdf4_handler/HDF4RequestHandler.cc:208`, `modules/hdf4_handler/hdfdesc.cc:2970` | Live metadata-cache path. |
| `H4.EnableHybridVdata` | raw string `"H4.EnableHybridVdata"` | `modules/hdf4_handler/HDF4RequestHandler.cc:183` | |
| `H4.EnableCERESVdata` | raw string `"H4.EnableCERESVdata"` | `modules/hdf4_handler/HDF4RequestHandler.cc:184`, `modules/hdf4_handler/HDFCFUtil.cc:2737` | |
| `H4.EnableVdata_to_Attr` | raw string `"H4.EnableVdata_to_Attr"` | `modules/hdf4_handler/HDF4RequestHandler.cc:185` | |
| `H4.EnableVdataDescAttr` | raw string `"H4.EnableVdataDescAttr"` | `modules/hdf4_handler/HDF4RequestHandler.cc:186`, `modules/hdf4_handler/HDFCFUtil.cc:2723` | |
| `H4.DisableVdataNameclashingCheck` | raw string `"H4.DisableVdataNameclashingCheck"` | `modules/hdf4_handler/HDF4RequestHandler.cc:187` | |
| `H4.EnableVgroupAttr` | raw string `"H4.EnableVgroupAttr"` | `modules/hdf4_handler/HDF4RequestHandler.cc:188` | |
| `H4.EnableCheckMODISGeoFile` | raw string `"H4.EnableCheckMODISGeoFile"` | `modules/hdf4_handler/HDF4RequestHandler.cc:191`, `modules/hdf4_handler/HDFCFUtil.cc:609` | |
| `H4.EnableSwathGridAttr` | raw string `"H4.EnableSwathGridAttr"` | `modules/hdf4_handler/HDF4RequestHandler.cc:192` | |
| `H4.EnableCERESMERRAShortName` | raw string `"H4.EnableCERESMERRAShortName"` | `modules/hdf4_handler/HDF4RequestHandler.cc:193`, `modules/hdf4_handler/HDFCFUtil.cc:2676` | |
| `H4.EnableCheckScaleOffsetType` | raw string `"H4.EnableCheckScaleOffsetType"` | `modules/hdf4_handler/HDF4RequestHandler.cc:194`, `modules/hdf4_handler/hdfdesc.cc` | |
| `H4.DisableSwathDimMap` | raw string `"H4.DisableSwathDimMap"` | `modules/hdf4_handler/HDF4RequestHandler.cc:196` | |

## `modules/hdf5_handler/h5.conf.in`

| Config key | C++ name / symbol | Representative usage | Notes |
| --- | --- | --- | --- |
| `BES.Catalog.catalog.TypeMatch+=h5:...` | dynamic `BES.Catalog.<catalog>.TypeMatch` | `dispatch/BESCatalogUtils.cc:118` | Registers HDF5/HDF-EOS5 files. |
| `H5.EnableCF` | raw string `"H5.EnableCF"` | `modules/hdf5_handler/HDF5RequestHandler.cc:255-258` | Central CF-vs-generic switch. |
| `H5.MetaDataMemCacheEntries` | raw string `"H5.MetaDataMemCacheEntries"` | `modules/hdf5_handler/HDF5RequestHandler.cc:232` | |
| `H5.CachePurgeLevel` | raw string `"H5.CachePurgeLevel"` | `modules/hdf5_handler/HDF5RequestHandler.cc:235` | |
| `H5.EnableCFDMR` | raw string `"H5.EnableCFDMR"` | `modules/hdf5_handler/HDF5RequestHandler.cc:347-350` | |
| `H5.EnableCoorattrAddPath` | raw string `"H5.EnableCoorattrAddPath"` | `modules/hdf5_handler/HDF5RequestHandler.cc:339-342` | |
| `H5.ForceFlattenNDCoorAttr` | raw string `"H5.ForceFlattenNDCoorAttr"` | `modules/hdf5_handler/HDF5RequestHandler.cc:319-322` | |
| `H5.EnableDropLongString` | raw string `"H5.EnableDropLongString"` | `modules/hdf5_handler/HDF5RequestHandler.cc:303-306`, `modules/hdf5_handler/HDF5CF.cc:2824` | |
| `H5.EnableAddPathAttrs` | raw string `"H5.EnableAddPathAttrs"` | `modules/hdf5_handler/HDF5RequestHandler.cc:298-301` | |
| `H5.EnableFillValueCheck` | raw string `"H5.EnableFillValueCheck"` | `modules/hdf5_handler/HDF5RequestHandler.cc:308-311` | |
| `H5.EnableDAP4Coverage` | raw string `"H5.EnableDAP4Coverage"` | `modules/hdf5_handler/HDF5RequestHandler.cc:352-355` | |
| `H5.EnableCheckNameClashing` | raw string `"H5.EnableCheckNameClashing"` | `modules/hdf5_handler/HDF5RequestHandler.cc:293-296` | |
| `H5.NoZeroSizeFullnameAttr` | raw string `"H5.NoZeroSizeFullnameAttr"` | `modules/hdf5_handler/HDF5RequestHandler.cc:334-337` | |
| `H5.EscapeUTF8Attr` | raw string `"H5.EscapeUTF8Attr"` | `modules/hdf5_handler/HDF5RequestHandler.cc:362`, `modules/hdf5_handler/h5commoncfdap.cc:616` | |
| `H5.EnableDiskMetaDataCache` | raw string `"H5.EnableDiskMetaDataCache"` | `modules/hdf5_handler/HDF5RequestHandler.cc:399-402` | |
| `H5.EnableEOSGeoCacheFile` | raw string `"H5.EnableEOSGeoCacheFile"` | `modules/hdf5_handler/HDF5RequestHandler.cc:410-413` | |
| `H5.Cache.latlon.path` | raw string `"H5.Cache.latlon.path"` | `modules/hdf5_handler/HDF5RequestHandler.cc:415` | |
| `H5.Cache.latlon.prefix` | raw string `"H5.Cache.latlon.prefix"` | `modules/hdf5_handler/HDF5RequestHandler.cc:416` | |
| `H5.Cache.latlon.size` | raw string `"H5.Cache.latlon.size"` | `modules/hdf5_handler/HDF5RequestHandler.cc:414` | |
| `H5.EnableDiskDataCache` | raw string `"H5.EnableDiskDataCache"` | `modules/hdf5_handler/HDF5RequestHandler.cc:378-381` | |
| `H5.DiskCacheDataPath` | `HDF5DiskCache::PATH_KEY` in `modules/hdf5_handler/HDF5DiskCache.cc:23` | `modules/hdf5_handler/HDF5RequestHandler.cc:383` | |
| `H5.DiskCacheFilePrefix` | `HDF5DiskCache::PREFIX_KEY` in `modules/hdf5_handler/HDF5DiskCache.cc:24` | `modules/hdf5_handler/HDF5RequestHandler.cc:384` | |
| `H5.DiskCacheSize` | `HDF5DiskCache::SIZE_KEY` in `modules/hdf5_handler/HDF5DiskCache.cc:25` | `modules/hdf5_handler/HDF5RequestHandler.cc:385` | |
| `H5.DiskCacheComp` | raw string `"H5.DiskCacheComp"` | `modules/hdf5_handler/HDF5RequestHandler.cc:387-390` | |
| `H5.DiskCacheFloatOnlyComp` | raw string `"H5.DiskCacheFloatOnlyComp"` | `modules/hdf5_handler/HDF5RequestHandler.cc:392-395` | |
| `H5.DiskCacheCompThreshold` | raw string `"H5.DiskCacheCompThreshold"` | `modules/hdf5_handler/HDF5RequestHandler.cc:396` | |
| `H5.DiskCacheCompVarSize` | raw string `"H5.DiskCacheCompVarSize"` | `modules/hdf5_handler/HDF5RequestHandler.cc:397` | |
| `H5.RmConventionAttrPath` | raw string `"H5.RmConventionAttrPath"` | `modules/hdf5_handler/HDF5RequestHandler.cc:324-327` | |
| `H5.KeepVarLeadingUnderscore` | raw string `"H5.KeepVarLeadingUnderscore"` | `modules/hdf5_handler/HDF5RequestHandler.cc:288-291` | |
| `H5.EnablePassFileID` | raw string `"H5.EnablePassFileID"` | `modules/hdf5_handler/HDF5RequestHandler.cc:273-276` | |
| `H5.DisableStructMetaAttr` | raw string `"H5.DisableStructMetaAttr"` | `modules/hdf5_handler/HDF5RequestHandler.cc:278-281`, `modules/hdf5_handler/heos5cfdap.cc:837` | |
| `H5.DisableECSMetaAttr` | raw string `"H5.DisableECSMetaAttr"` | `modules/hdf5_handler/HDF5RequestHandler.cc:283-286` | |
| `H5.CheckIgnoreObj` | raw string `"H5.CheckIgnoreObj"` | `modules/hdf5_handler/HDF5RequestHandler.cc:314-317` | |
| `H5.EnableDMR64bitInt` | raw string `"H5.EnableDMR64bitInt"` | `modules/hdf5_handler/HDF5RequestHandler.cc:329-332` | |
| `H5.LargeDataMemCacheEntries` | raw string `"H5.LargeDataMemCacheEntries"` | `modules/hdf5_handler/HDF5RequestHandler.cc:233` | |
| `H5.LargeDataMemCacheConfig` | raw string `"H5.LargeDataMemCacheConfig"` | `modules/hdf5_handler/HDF5RequestHandler.cc:440-443` | |
| `H5.DataCachePath` | raw string `"H5.DataCachePath"` | `modules/hdf5_handler/HDF5RequestHandler.cc:1736` | |
| `H5.LargeDataMemCacheFileName` | raw string `"H5.LargeDataMemCacheFileName"` | `modules/hdf5_handler/HDF5RequestHandler.cc:1739` | |
| `H5.SmallDataMemCacheEntries` | raw string `"H5.SmallDataMemCacheEntries"` | `modules/hdf5_handler/HDF5RequestHandler.cc:234` | |
| `H5.STPEastFileName` | raw string `"H5.STPEastFileName"` | `modules/hdf5_handler/HDF5RequestHandler.cc:370` | |
| `H5.STPNorthFileName` | raw string `"H5.STPNorthFileName"` | `modules/hdf5_handler/HDF5RequestHandler.cc:371` | |
| `H5.DefaultHandleDimension` | raw string `"H5.DefaultHandleDimension"` | `modules/hdf5_handler/HDF5RequestHandler.cc:261-264` | |
| `H5.DefaultDAP4AddUnlimitedDimension` | raw string `"H5.DefaultDAP4AddUnlimitedDimension"` | `modules/hdf5_handler/HDF5RequestHandler.cc:266-269` | Commented optional override in the template. |

## `modules/httpd_catalog_module/httpd_catalog.conf.in`

| Config key | C++ name / symbol | Representative usage | Notes |
| --- | --- | --- | --- |
| `BES.modules+=httpd` | raw string `"BES.modules"` | `dispatch/BESModuleApp.cc:93` | Disabled by default in the template. |
| `BES.module.httpd` | dynamic `BES.module.<name>` | `dispatch/BESModuleApp.cc:119` | |
| `Httpd_Catalog.Reference` | none found | no live lookup found | Looks like documentation only in the current tree. |
| `BES.Catalog.RemoteResources.RootDirectory` | dynamic `BES.Catalog.<catalog>.RootDirectory` | `dispatch/BESCatalogUtils.cc:79` | Template says the value is not used by the module, only required by the framework. |
| `BES.Catalog.RemoteResources.TypeMatch` | dynamic `BES.Catalog.<catalog>.TypeMatch` | `dispatch/BESCatalogUtils.cc:118` | Template says the value is not used by the module, only required by the framework. |
| `BES.Catalog.RemoteResources.FollowSymLinks` | dynamic `BES.Catalog.<catalog>.FollowSymLinks` | `dispatch/BESCatalogUtils.cc:156` | Optional framework catalog key. |
| `BES.Catalog.RemoteResources.Exclude` | dynamic `BES.Catalog.<catalog>.Exclude` | `dispatch/BESCatalogUtils.cc:98` | Optional framework catalog key. |
| `BES.Catalog.RemoteResources.Include` | dynamic `BES.Catalog.<catalog>.Include` | `dispatch/BESCatalogUtils.cc:107` | Optional framework catalog key. |
| `Httpd_Catalog.Collections` | `HTTPD_CATALOG_COLLECTIONS` in `modules/httpd_catalog_module/HttpdCatalogNames.h:35` | `modules/httpd_catalog_module/HttpdCatalog.cc:88` | Core live config for the virtual remote collections. |
| `AllowedHosts+=...` | `ALLOWED_HOSTS_BES_KEY` in `http/AllowedHosts.h:39` | `http/AllowedHosts.cc:54` | Required because remote collection origins must be allowed globally. |

## `modules/ncml_module/ncml.conf.in`

| Config key | C++ name / symbol | Representative usage | Notes |
| --- | --- | --- | --- |
| `BES.Catalog.catalog.TypeMatch+=ncml:...` | dynamic `BES.Catalog.<catalog>.TypeMatch` | `dispatch/BESCatalogUtils.cc:118` | Registers `.ncml` files. |
| `NCML.DimensionCache.directory` | `AggMemberDatasetDimensionCache::CACHE_DIR_KEY` in `modules/ncml_module/AggMemberDatasetDimensionCache.cc:34` | `modules/ncml_module/AggMemberDatasetDimensionCache.cc:65` | |
| `NCML.DimensionCache.prefix` | `AggMemberDatasetDimensionCache::PREFIX_KEY` in `modules/ncml_module/AggMemberDatasetDimensionCache.cc:35` | `modules/ncml_module/AggMemberDatasetDimensionCache.cc:86` | |
| `NCML.DimensionCache.size` | `AggMemberDatasetDimensionCache::SIZE_KEY` in `modules/ncml_module/AggMemberDatasetDimensionCache.cc:36` | `modules/ncml_module/AggMemberDatasetDimensionCache.cc:42` | |
| `NCML.DimensionCache.maxDimensions` | `MAX_DIMENSION_COUNT_KEY` in `modules/ncml_module/AggMemberDatasetWithDimensionCacheBase.cc:62` | `modules/ncml_module/AggMemberDatasetWithDimensionCacheBase.cc:262` | Optional cap; defaults to 100 if not set. |

## `modules/netcdf_handler/nc.conf.in`

| Config key | C++ name / symbol | Representative usage | Notes |
| --- | --- | --- | --- |
| `BES.Catalog.catalog.TypeMatch+=nc:...` | dynamic `BES.Catalog.<catalog>.TypeMatch` | `dispatch/BESCatalogUtils.cc:118` | Registers netCDF/netCDF-4 files. |
| `NC.ShowSharedDimensions` | raw string `"NC.ShowSharedDimensions"` | `modules/netcdf_handler/NCRequestHandler.cc:134` | |
| `NC.PromoteByteToShort` | raw string `"NC.PromoteByteToShort"` | `modules/netcdf_handler/NCRequestHandler.cc:166`, `modules/netcdf_handler/NCArray.cc:212` | |
| `NC.CacheEntries` | raw string `"NC.CacheEntries"` | `modules/netcdf_handler/NCRequestHandler.cc:179` | |
| `NC.CachePurgeLevel` | raw string `"NC.CachePurgeLevel"` | `modules/netcdf_handler/NCRequestHandler.cc:181` | Commented optional override in the template. |
| `NC.UseMDS` | raw string `"NC.UseMDS"` | `modules/netcdf_handler/NCRequestHandler.cc:178` | If enabled, depends on framework `DAP.GlobalMetadataStore.*` keys. |

## `modules/s3_reader/s3.conf.in`

| Config key | C++ name / symbol | Representative usage | Notes |
| --- | --- | --- | --- |
| `s3.inject_data_urls` | none found | no live lookup found | Present in the installed template, but I did not find an active code reader in the current tree. |

## Gaps, mismatches, and likely-stale keys

These keys are present in installed module templates, but I could not tie them to a live code lookup:

- `CMR.Reference`
- `FONg.Reference`
- `Gateway.MimeTypes`
- `Gateway.ProxyHost`
- `Gateway.ProxyPort`
- `Gateway.ProxyUser`
- `Gateway.ProxyPassword`
- `Gateway.ProxyUserPW`
- `Gateway.ProxyAuthType`
- `Gateway.Cache.dir`
- `Gateway.Cache.prefix`
- `Gateway.Cache.size`
- `HDF4.CacheDir`
- `Httpd_Catalog.Reference`
- `s3.inject_data_urls`

These keys look actively mismatched or malformed in the current templates:

- `FONg.default_gcs`
  - The template uses `FONg.default_gcs`, but `modules/gdal_module/writer/GeoTiffTransmitter.cc:108` reads `FONg.Default_GCS`.
- `H4.Cache.metada.path`
  - This appears to be a typo in the commented example. Live code reads `H4.Cache.metadata.path` in `modules/hdf4_handler/HDF4RequestHandler.cc:208`.
- `KEY`
  - The parser will see `KEY = ...` in explanatory comments inside `modules/hdf4_handler/h4.conf.in`, but that is documentation text, not a real runtime key family.

