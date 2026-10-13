# Work log: "Service Chaining" error message limited to NGAP 50x errors

## 2026-10-08

### Prompts

1. "Plan changes to the 'Service chaining' error message in the http/EffectiveUrlCache.cc, http/RemoteResource.cc,
   modules/dmrpp_module/CurlHandlePool.cc, and modules/dmrpp_module/ngap_container/NgapApi.cc classes. This message
   should only be sent when code in the NgapOwnedContainer object makes a call to an external service and that
   service returns a 50x error. If there are other places in the BES where an HttpError object might be thrown
   because a call to a web resource returned a 500 error, call those out in the plan."
2. Follow-up: "I'd like to modify the plan so that those 500 errors that are chunk reads and redirect lookups that
   run later in dmrpp_module _do_ count as service chaining errors, but only for NGAP requests."

### Problem

Four `catch (http::HttpError &)` blocks added "Hyrax encountered a Service Chaining Error ..." to every `HttpError`,
for any HTTP status and any container type. A 404 from the gateway module, or a 403 on a chunk read for a catalog
DMR++, was reported as a TEA/service-chaining failure. `RemoteResource` is not used by `NgapOwnedContainer` at all.

### Decisions

- **NGAP-request signal.** `NgapOwnedContainer::access()` resets the container type to `"dmrpp"`, so the type string
  cannot be used. `DmrppRequestHandler` already uses `dynamic_cast<ngap::NgapOwnedContainer *>`. Each data-building
  handler (`dap_build_dmr`, `dap_build_dap4data`, `dap_build_dap2data`, `dap_build_dds`, `dap_build_das`) now records
  the result in a per-process `std::atomic<bool>` (`ngap::set_ngap_request()`). Data is read after these handlers
  return, during transmit, so the flag has to outlive the handler call. Because it is reset on every request, a stale
  value cannot leak into the next request.
  - Rejected: a BESContext, because contexts persist across requests on one connection.
  - Rejected: `url::is_trusted()`, because a local DMR++ with `dmrpp:trust="true"` would be misreported as NGAP.
- **Where the helper lives.** The helper is in `libngap` (`ngap_container/NgapServiceChaining.{h,cc}`). libngap is
  a separate convenience library linked into `libdmrpp_module`, so the NGAP code does not depend on dmrpp core.
  `build_dmrpp` and `get_dmrpp` link libngap but never set the flag, so it stays false for them.
- **Generic `http/` code** (`EffectiveUrlCache`, `RemoteResource`) has no NGAP wording now. It uses a neutral
  "Error while attempting to ..." prefix.
- **Parallel SuperChunk path** (`DmrppArray.cc`, curl_multi): this path throws `BESInternalError`, not `HttpError`, and
  used to drop the HTTP status. The status is now kept in `MyCurlMultiTransfer::last_http_code`, added to the message
  as "Last HTTP status: N", and used for the 5xx test. The exception type was not changed, because switching to
  `HttpError` would change what the OLFS reports. That is left as a possible follow-up.

### Where the message is now added (only when the status is 500–599)

| Site | Condition |
| --- | --- |
| `NgapApi.cc` CMR lookup | 5xx (only runs for NGAP) |
| `NgapOwnedContainer::dmrpp_read_from_opendap_bucket()` (new try/catch) | 5xx |
| `NgapOwnedContainer::dmrpp_read_from_daac_bucket()` | 5xx |
| `dmrpp_easy_handle::read_data()` (`CurlHandlePool.cc`, serial chunk reads) | NGAP request && 5xx |
| `Chunk::get_data_url()` (redirect via `EffectiveUrlCache`; new try/catch) | NGAP request && 5xx |
| `DmrppArray.cc` parallel SuperChunk reads (both throw sites) | NGAP request && 5xx |

### Other 50x sources (not changed)

- `http/CurlUtils.cc:888` throws `HttpError` for 422/500/502/503/504 when the URL is not retryable. `:1110` throws
  when retries run out. `:1716` throws in `get_redirect_url()` for any non-3xx status.
- Gap: `process_http_code_helper()`'s `default:` branch throws `BESInternalError` for 501 and 505–511. Those
  codes will not get the message on the serial path or from NgapOwnedContainer's own calls. The parallel path
  does handle them, because it tests `last_http_code` directly.
- `aws/SignedUrlCache.cc:331` (TEA s3credentials) catches `HttpError`, logs it with INFO_LOG, and returns `nullptr`.
  Nothing is thrown.
- RemoteResource users (gateway, cmr, httpd_catalog, s3_reader) now report 50x errors without the Service Chaining
  text, which is intended.
- `debug_functions/DebugFunctions.cc:422` throws a test `HttpError` on purpose.

### Files changed

- `http/EffectiveUrlCache.cc`, `http/RemoteResource.cc`
- `modules/dmrpp_module/ngap_container/NgapServiceChaining.{h,cc}` (new), `ngap_container/Makefile.am`
- `modules/dmrpp_module/ngap_container/NgapApi.cc`, `NgapOwnedContainer.cc`
- `modules/dmrpp_module/DmrppRequestHandler.cc`, `CurlHandlePool.cc`, `Chunk.cc`, `DmrppArray.cc`
- `modules/dmrpp_module/ngap_container/unit-tests/NgapServiceChainingTest.cc` (new), `unit-tests/Makefile.am`

`ChangeLog` was not edited. It is generated from commit messages.

### Validation run

- Changing `Makefile.am` made configure run again. The prefix `bin` directory had to be on `PATH`
  (`/Users/jhrg/src/opendap/hyrax/build/bin`) so that `dap-config` could be found.
- `make -j20` in `http/` and `modules/dmrpp_module/`: success, with no new warnings in the changed files.
- `make check` in `modules/dmrpp_module/ngap_container/unit-tests`: 4/4 pass, including the new
  `NgapServiceChainingTest`. It covers the 5xx boundaries, the flag, and the message text for both `HttpError` and
  `BESInternalError`.
- `make check` in `modules/dmrpp_module/unit-tests` (9/9) and `http/unit-tests` (9/9): pass.
- `make check` in `modules/dmrpp_module/tests`: all suites pass (330 + 2 + 20 + 17 + 5). No baselines changed or
  were regenerated.

### Not run

- No test injects a fake 5xx into the chunk-read, redirect, or parallel paths. The new branches there were
  checked by review and compilation only.
- End-to-end NGAP 50x from CMR, TEA, or S3: this needs NGAP and EDL access.
- No full top-level `make check` and no `make distcheck`. The new source and header files are listed in
  `Makefile.am`, so they should be included in the distribution.
