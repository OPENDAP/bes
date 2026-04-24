# Framework Configuration Key Map

This document maps the installed framework-side `*.conf.in` templates to the C++ names that read those keys and to representative usage sites.

Scope:

- Included templates: `dispatch/bes/bes.conf.in`, `dap/dap.conf.in`, `dapreader/dapreader.conf.in`, `http/http.conf.in`
- Search coverage for consumers: `dispatch/`, `server/`, `ppt/`, `xmlcommand/`, `http/`, `standalone/`, `cmdln/`, `dap/`, `dapreader/`, and `modules/`
- Excluded on purpose: test-only `bes.conf.in` files, module-specific config templates as primary sources, and retired code except where a key is only referenced there
- "C++ name" means a macro, `const`/`constexpr`, or the dynamic string pattern used by the code

Notes:

- Several keys are read indirectly. For example, `BES.Help.*` and `DAP.Help.*` are loaded through the prefix key `BES.Help` or `DAP.Help` and then a suffix is added by the info renderer.
- Some keys appear to be stale or only documented in the template. Those are called out explicitly instead of being silently "mapped."
- There are no additional installed framework `*.conf.in` templates under `server/`, `ppt/`, `standalone/`, or `cmdln/`, but those directories were searched for consumers of the framework-owned keys.
- I also searched `modules/` because a number of framework-owned keys such as `AllowedHosts`, `BES.Catalog.catalog.RootDirectory`, `BES.Data.RootDirectory`, and `Http.cache.effective.urls*` are used there.

## `dispatch/bes/bes.conf.in`

| Config key | C++ name / symbol | Representative usage | Notes |
| --- | --- | --- | --- |
| `SupportEmail` | none found | no live BES lookup found; only docs/comments in `dispatch/bes/site.conf.proto:41` | Looks like a site/OLFS-facing setting, not a framework key read by BES code. |
| `BES.ServerAdministrator` and `BES.ServerAdministrator+=...` | `SERVER_ADMINISTRATOR_KEY` in `dispatch/ServerAdministrator.h:34` | `dispatch/ServerAdministrator.cc` | Map-valued key; the `+=` entries populate `organization`, `street`, `city`, `region`, `postalCode`, `country`, `telephone`, `website`. |
| `BES.User` | raw string `"BES.User"` | `server/daemon.cc:841` | Read by daemon startup code, not by `dispatch/`. |
| `BES.Group` | raw string `"BES.Group"` | `server/daemon.cc:757` | Read by daemon startup code, not by `dispatch/`. |
| `BES.LogName` | raw string `"BES.LogName"` | `dispatch/BESLog.cc:98`; also `server/DaemonCommandHandler.cc:159` | Primary log file path. |
| `BES.LogVerbose` | raw string `"BES.LogVerbose"` | `dispatch/BESLog.cc:149` | Enables verbose logging. |
| `BES.LogUnixTime` | raw string `"BES.LogUnixTime"` | `dispatch/BESLog.cc:155` | Switches log timestamps to Unix time. |
| `BES.LogTimeLocal` | raw string `"BES.LogTimeLocal"` and `LOCAL_TIME_KEY` in `dap/GlobalMetadataStore.cc:102` | `dispatch/BESLog.cc:118`, `dap/GlobalMetadataStore.cc:370` | Shared by normal logging and MDS ledger timestamps. |
| `BES.Catalog.catalog.RootDirectory` | dynamic key pattern `"BES.Catalog." + n + ".RootDirectory"` | `dispatch/BESCatalogUtils.cc:79`; also `http/RemoteResource.cc:53`, `dap/BESStoredDapResultCache.cc:86`, `modules/ncml_module/DirectoryUtil.cc:427`, `modules/dmrpp_module/ngap_container/NgapOwnedContainer.cc:121` | `catalog` is the catalog name used in the template. |
| `BES.Catalog.catalog.FollowSymLinks` | dynamic key pattern `"BES.Catalog." + n + ".FollowSymLinks"` | `dispatch/BESCatalogUtils.cc:156` | `catalog` is the catalog name used in the template. |
| `BES.UncompressCache.dir` | `BESUncompressCache::DIR_KEY` in `dispatch/BESUncompressCache.cc:40` | `dispatch/BESUncompressCache.cc:68` | |
| `BES.UncompressCache.prefix` | `BESUncompressCache::PREFIX_KEY` in `dispatch/BESUncompressCache.cc:41` | `dispatch/BESUncompressCache.cc:83` | |
| `BES.UncompressCache.size` | `BESUncompressCache::SIZE_KEY` in `dispatch/BESUncompressCache.cc:42` | `dispatch/BESUncompressCache.cc:52` | |
| `BES.TimeOutInSeconds` | `BES_TIMEOUT_KEY` in `dispatch/BESInterface.cc:91` | `dispatch/BESInterface.cc` | Template comment is accurate: front-end context can override this. |
| `BES.CancelTimeoutOnSend` | raw string constant `BES_KEY_TIMEOUT_CANCEL` | `dispatch/BESUtil.cc:82`, `dap/BESDapResponseBuilder.cc:130` | Shared between framework utility code and DAP response code. |
| `BES.AnnotationServiceURL` | raw string constant `annotation_service_url` | `dispatch/BESResponseHandler.cc:48` | Used when building DAP responses with annotation metadata. |
| `AllowedHosts` | `ALLOWED_HOSTS_BES_KEY` in `http/AllowedHosts.h:39` | `http/AllowedHosts.cc:54`, `http/CurlUtils.cc:1151`, `modules/gateway_module/GatewayContainer.cc:74`, `modules/dmrpp_module/CurlHandlePool.cc:309` | Controls remote URL allow-listing across framework and modules. |
| `BES.Data.RootDirectory` | raw string `"BES.Data.RootDirectory"` and `BES_DATA_ROOT` macro in `dap/BESStoredDapResultCache.cc:85` | `dispatch/BESContainerStorageVolatile.cc:74`, `dap/BESStoredDapResultCache.cc`, `modules/ncml_module/DirectoryUtil.cc:429` | Standalone-BES data root; also used by some modules when no catalog root is available. |
| `BES.FollowSymLinks` | raw string `"BES.FollowSymLinks"` | `dispatch/BESContainerStorageVolatile.cc:83` | Standalone-BES symlink policy. |
| `BES.ServerPort` | raw string `"BES.ServerPort"` | `server/ServerApp.cc:343` | Read by listener startup code. |
| `BES.ServerIP` | raw string `"BES.ServerIP"` | `server/ServerApp.cc:364` | Read by listener startup code. |
| `BES.DaemonPort` | `DAEMON_PORT_STR` in `server/daemon.cc:72` | `server/daemon.cc` | Admin interface port. |
| `BES.ServerSecure` | raw string `"BES.ServerSecure"` | `server/ServerApp.cc:407` | TLS enablement gate. |
| `BES.ServerSecurePort` | raw string `"BES.ServerSecurePort"` | `ppt/PPTServer.cc:117` | Used by PPT secure transport code. |
| `BES.ServerCertFile` | raw string `"BES.ServerCertFile"` | `ppt/PPTServer.cc:95` | PPT secure transport. |
| `BES.ServerCertAuthFile` | raw string `"BES.ServerCertAuthFile"` | `ppt/PPTServer.cc:102` | PPT secure transport. |
| `BES.ServerKeyFile` | raw string `"BES.ServerKeyFile"` | `ppt/PPTServer.cc:109` | PPT secure transport. |
| `BES.ClientCertFile` | raw string `"BES.ClientCertFile"` | `ppt/PPTClient.cc:83` | PPT secure transport. |
| `BES.ClientCertAuthFile` | raw string `"BES.ClientCertAuthFile"` | `ppt/PPTClient.cc:91` | PPT secure transport. |
| `BES.ClientKeyFile` | raw string `"BES.ClientKeyFile"` | `ppt/PPTClient.cc:99` | PPT secure transport. |
| `BES.Help.TXT` | prefix key `BES.Help` plus `.TXT` suffix | `dispatch/BESHelpResponseHandler.cc:80`, `dispatch/BESTextInfo.cc:187` | Used for text help. |
| `BES.Help.HTML` | prefix key `BES.Help` plus `.HTML` suffix | `dispatch/BESHelpResponseHandler.cc:80`, `dispatch/BESHTMLInfo.cc:212` | Used for HTML help. |
| `BES.Help.XML` | no direct live suffix reader found | `dispatch/BESHelpResponseHandler.cc:80`, `dispatch/BESXMLInfo.cc:449` | `BESXMLInfo` currently appends `.HTML`, not `.XML`, so this key looks unused in current framework code. |
| `BES.Info.Buffered` | no current live lookup found | none found in non-test framework sources | The base class supports a buffered-key constructor in `dispatch/BESInfo.cc:70`, but the active builders use default constructors. This looks stale in the current path. |
| `BES.Info.Type` | raw string `"BES.Info.Type"` | `dispatch/BESInfoList.cc:84` | Selects text/html/xml info builder. |
| `BES.Container.Persistence` | raw string `"BES.Container.Persistence"` | `dispatch/BESContainerStorageList.cc:161` | Controls `strict` vs `nice` behavior. |
| `BES.SetSockRecvSize` | raw string `"BES.SetSockRecvSize"` | `ppt/TcpSocket.cc:390` | Not used in `dispatch/`; used by PPT transport. |
| `BES.SockRecvSize` | raw string `"BES.SockRecvSize"` | `ppt/TcpSocket.cc:400` | Not used in `dispatch/`; used by PPT transport. |
| `BES.SetSockSendSize` | raw string `"BES.SetSockSendSize"` | `ppt/TcpSocket.cc:451` | Not used in `dispatch/`; used by PPT transport. |
| `BES.SockSendSize` | raw string `"BES.SockSendSize"` | `ppt/TcpSocket.cc:462` | Not used in `dispatch/`; used by PPT transport. |
| `BES.Memory.GlobalArea.EmergencyPoolSize` | no live framework symbol found | `retired/dispatch/BESMemoryGlobalArea.cc:100` | Only found in retired code. |
| `BES.Memory.GlobalArea.MaximumHeapSize` | no live framework symbol found | no live non-retired lookup found | The family appears to be effectively retired. |
| `BES.Memory.GlobalArea.Verbose` | no live framework symbol found | `retired/dispatch/BESMemoryGlobalArea.cc:101` | Only found in retired code. |
| `BES.Memory.GlobalArea.ControlHeap` | no live framework symbol found | `retired/dispatch/BESMemoryGlobalArea.cc:102` | Only found in retired code. |
| `BES.ProcessManagerMethod` | raw string `"BES.ProcessManagerMethod"` | `server/BESServerHandler.cc:74` | Server process model selection. |
| `BES.DefaultResponseMethod` | no live framework symbol found | `retired/apache/BESApacheInterface.cc:328` | Only found in retired Apache code. |
| `BES.modules` | raw string `"BES.modules"` | `dispatch/BESModuleApp.cc:93` | Module list. |
| `BES.module.<name>` | dynamic key pattern `"BES.module." + name` | `dispatch/BESModuleApp.cc:119` | Covers `BES.module.dap`, `BES.module.dapcmd`, `BES.module.reader`, etc. |
| `BES.Include` | `BES_INCLUDE_KEY` in `dispatch/TheBESKeys.cc:52` and `dispatch/kvp_utils.cc:46` | `dispatch/TheBESKeys.cc`, `dispatch/kvp_utils.cc`, `server/DaemonCommandHandler.cc:139` | Include/ingest other config files. |

## `dap/dap.conf.in`

| Config key | C++ name / symbol | Representative usage | Notes |
| --- | --- | --- | --- |
| `BES.modules+=dap,dapcmd` | raw string `"BES.modules"` | `dispatch/BESModuleApp.cc:93` | Appends DAP modules to the global module list. |
| `BES.module.dap` | dynamic `BES.module.<name>` | `dispatch/BESModuleApp.cc:119` | Loaded when `name == "dap"`. |
| `BES.module.dapcmd` | dynamic `BES.module.<name>` | `dispatch/BESModuleApp.cc:119` | Loaded when `name == "dapcmd"`. |
| `BES.Catalog.Default` | raw string `"BES.Catalog.Default"` | `dispatch/BESCatalogList.cc:79` | Default catalog name. |
| `BES.Catalog.catalog.TypeMatch` | dynamic key pattern `"BES.Catalog." + n + ".TypeMatch"` | `dispatch/BESCatalogUtils.cc:118` | Used to map filenames to handlers. The blank assignment shown in comments is significant because it clears prior values. |
| `BES.Catalog.catalog.Include` | dynamic key pattern `"BES.Catalog." + n + ".Include"` | `dispatch/BESCatalogUtils.cc:107` | Include regex list. |
| `BES.Catalog.catalog.Exclude` | dynamic key pattern `"BES.Catalog." + n + ".Exclude"` | `dispatch/BESCatalogUtils.cc:98` | Exclude regex list. |
| `DAP.Help.TXT` | prefix key `DAP.Help` plus `.TXT` suffix | `dap/BESDapRequestHandler.cc:61`, `dispatch/BESTextInfo.cc:187` | Used for text help. |
| `DAP.Help.HTML` | prefix key `DAP.Help` plus `.HTML` suffix | `dap/BESDapRequestHandler.cc:61`, `dispatch/BESHTMLInfo.cc:212` | Used for HTML help. |
| `DAP.Help.XML` | no direct live suffix reader found | `dap/BESDapRequestHandler.cc:61`, `dispatch/BESXMLInfo.cc:449` | Same issue as `BES.Help.XML`: current XML info code uses the `.HTML` file. |
| `DAP.GlobalMetadataStore.path` | `PATH_KEY` in `dap/GlobalMetadataStore.cc:98` | `dap/GlobalMetadataStore.cc:267` | Comment says unset/null disables MDS. The same framework MDS key family is also referenced by module code such as `modules/dmrpp_module/DmrppMetadataStore.h`. |
| `DAP.GlobalMetadataStore.prefix` | `PREFIX_KEY` in `dap/GlobalMetadataStore.cc:99` | `dap/GlobalMetadataStore.cc:251` | The same framework MDS key family is also referenced by module code such as `modules/dmrpp_module/DmrppMetadataStore.h`. |
| `DAP.GlobalMetadataStore.size` | `SIZE_KEY` in `dap/GlobalMetadataStore.cc:100` | `dap/GlobalMetadataStore.cc:236` | The same framework MDS key family is also referenced by module code such as `modules/dmrpp_module/DmrppMetadataStore.h`. |
| `DAP.GlobalMetadataStore.ledger` | `LEDGER_KEY` in `dap/GlobalMetadataStore.cc:101` | `dap/GlobalMetadataStore.cc:370` | The same framework MDS key family is also referenced by module code such as `modules/dmrpp_module/DmrppMetadataStore.h`. |
| `DAP.Use.Dmrpp` | `USE_DMRPP_KEY` in `dap/BESDapNames.h:100` | `dap/BESDataResponseHandler.cc:58`, `dap/BESDap4ResponseHandler.cc:46` | Enables DMR++ fast path when metadata store has a DMR++. |
| `DAP.FunctionResponseCache.path` | `BESDapFunctionResponseCache::PATH_KEY` in `dap/BESDapFunctionResponseCache.cc:94` | `dap/BESDapFunctionResponseCache.cc:137` | Empty/undefined disables cache. |
| `DAP.FunctionResponseCache.prefix` | `BESDapFunctionResponseCache::PREFIX_KEY` in `dap/BESDapFunctionResponseCache.cc:95` | `dap/BESDapFunctionResponseCache.cc:121` | |
| `DAP.FunctionResponseCache.size` | `BESDapFunctionResponseCache::SIZE_KEY` in `dap/BESDapFunctionResponseCache.cc:96` | `dap/BESDapFunctionResponseCache.cc:106` | |
| `DAP.StoredResultsCache.subdir` | `DAP_STORED_RESULTS_CACHE_SUBDIR_KEY` in `dap/BESStoredDapResultCache.h:36` | `dap/BESStoredDapResultCache.cc:127` | |
| `DAP.StoredResultsCache.prefix` | `DAP_STORED_RESULTS_CACHE_PREFIX_KEY` in `dap/BESStoredDapResultCache.h:37` and `BESStoredDapResultCache::PREFIX_KEY` in `dap/BESStoredDapResultCache.cc:99` | `dap/BESStoredDapResultCache.cc:151` | |
| `DAP.StoredResultsCache.size` | `DAP_STORED_RESULTS_CACHE_SIZE_KEY` in `dap/BESStoredDapResultCache.h:38` and `BESStoredDapResultCache::SIZE_KEY` in `dap/BESStoredDapResultCache.cc:100` | `dap/BESStoredDapResultCache.cc:108` | |
| `DAP.Async.StyleSheet.Ref` | none found | no live lookup found outside the template | This appears undocumented-in-code or stale in the current tree. |
| `BES.MaxVariableSize.bytes` | `BES_KEYS_MAX_VAR_SIZE_KEY` in `dap/DapUtils.cc:57` | `dap/DapUtils.cc:479` | Also overridden by request context key `max_variable_size`. |
| `BES.MaxResponseSize.bytes` | `BES_KEYS_MAX_RESPONSE_SIZE_KEY` in `dap/DapUtils.cc:56` | `dap/DapUtils.cc:453` | Also overridden by request context key `max_response_size`. |

## `dapreader/dapreader.conf.in`

| Config key | C++ name / symbol | Representative usage | Notes |
| --- | --- | --- | --- |
| `BES.Include=dap.conf` | `BES_INCLUDE_KEY` in `dispatch/TheBESKeys.cc:52` | `dispatch/TheBESKeys.cc`, `dispatch/kvp_utils.cc` | Forces DAP config to load first. |
| `BES.modules+=reader` | raw string `"BES.modules"` | `dispatch/BESModuleApp.cc:93` | Adds the reader module to the global module list. |
| `BES.module.reader` | dynamic `BES.module.<name>` | `dispatch/BESModuleApp.cc:119` | Loaded when `name == "reader"`. |
| `BES.Catalog.catalog.TypeMatch+=reader:...` | dynamic key pattern `"BES.Catalog." + n + ".TypeMatch"` | `dispatch/BESCatalogUtils.cc:118` | Extends the catalog type map for DAP-reader file extensions. |
| `DR.UseTestTypes` | raw string `"DR.UseTestTypes"` | `dapreader/DapRequestHandler.cc:111` | No `*Names.h` symbol; read directly. |
| `DR.UseSeriesValues` | raw string `"DR.UseSeriesValues"` | `dapreader/DapRequestHandler.cc:112` | No `*Names.h` symbol; read directly. |

## `http/http.conf.in`

| Config key | C++ name / symbol | Representative usage | Notes |
| --- | --- | --- | --- |
| `Http.UserAgent` | `HTTP_USER_AGENT_KEY` in `http/HttpNames.h:34` | `http/CurlUtils.cc:1360` | Commented out in template, but live in code. |
| `Http.RemoteResource.TmpDir` | `REMOTE_RESOURCE_TMP_DIR_KEY` in `http/HttpNames.h:82` | `http/RemoteResource.cc:143` | Commented out in template, but live in code. |
| `Http.RemoteResource.TmpFile.Delete` | `REMOTE_RESOURCE_DELETE_TMP_FILE` in `http/HttpNames.h:83` | `http/RemoteResource.cc:159` | Commented out in template, but live in code. |
| `Http.Cookies.File` | `HTTP_COOKIES_FILE_KEY` in `http/HttpNames.h:54` | `http/CurlUtils.cc:1310` | Cookie-jar base path. |
| `Http.No.Retry.Regex` | `HTTP_NO_RETRY_URL_REGEX_KEY` in `http/HttpNames.h:63` | `http/CurlUtils.cc:726` | Multi-valued regex list. |
| `Http.cache.effective.urls` | `HTTP_CACHE_EFFECTIVE_URLS_KEY` in `http/HttpNames.h:64` | `http/EffectiveUrlCache.cc:170` | Enables effective-URL caching; also set directly by some module configs such as `modules/dmrpp_module/dmrpp.conf.in`. |
| `Http.cache.effective.urls.skip.regex.pattern` | `HTTP_CACHE_EFFECTIVE_URLS_SKIP_REGEX_KEY` in `http/HttpNames.h:65` | `http/EffectiveUrlCache.cc:179` | Skip-pattern for effective-URL caching; also set directly by some module configs such as `modules/dmrpp_module/dmrpp.conf.in`. |
| `Http.MimeTypes` and `Http.MimeTypes += ...` | `HTTP_MIMELIST_KEY` in `http/HttpNames.h:37` | `http/HttpUtils.cc:77` | Maps handler names to MIME types. |
| `Http.ProxyProtocol` | `HTTP_PROXYPROTOCOL_KEY` in `http/HttpNames.h:38` | `http/ProxyConfig.cc:70` | Optional; defaults to `http`. |
| `Http.ProxyHost` | `HTTP_PROXYHOST_KEY` in `http/HttpNames.h:39` | `http/ProxyConfig.cc:47` | Presence of host enables proxy config. |
| `Http.ProxyPort` | `HTTP_PROXYPORT_KEY` in `http/HttpNames.h:40` | `http/ProxyConfig.cc:55` | Optional in config, but required in practice if host is set. |
| `Http.ProxyAuthType` | `HTTP_PROXYAUTHTYPE_KEY` in `http/HttpNames.h:41` | `http/ProxyConfig.cc:106` | Recognizes `basic`, `digest`, `ntlm`. |
| `Http.ProxyUser` | `HTTP_PROXYUSER_KEY` in `http/HttpNames.h:42` | `http/ProxyConfig.cc:79` | |
| `Http.ProxyPassword` | `HTTP_PROXYPASSWORD_KEY` in `http/HttpNames.h:43` | `http/ProxyConfig.cc:88` | |
| `Http.ProxyUserPW` | `HTTP_PROXYUSERPW_KEY` in `http/HttpNames.h:44` | `http/ProxyConfig.cc:97` | |
| `Http.NoProxy` | `HTTP_NO_PROXY_REGEX_KEY` in `http/HttpNames.h:45` | `http/ProxyConfig.cc:133` | Regex for bypassing the configured proxy. |
| `Http.UseInternalCache` | none found | no live lookup found outside the template | This appears stale or not yet implemented in the current tree. |

## Gaps and likely-stale keys

These keys are present in the installed templates, but I could not tie them to a live framework lookup path:

- `SupportEmail`
- `BES.Help.XML`
- `BES.Info.Buffered`
- `DAP.Help.XML`
- `DAP.Async.StyleSheet.Ref`
- `Http.UseInternalCache`

These keys are present in the installed framework template, but their only code references are outside the active framework path or in retired code:

- `BES.Memory.GlobalArea.EmergencyPoolSize`
- `BES.Memory.GlobalArea.MaximumHeapSize`
- `BES.Memory.GlobalArea.Verbose`
- `BES.Memory.GlobalArea.ControlHeap`
- `BES.DefaultResponseMethod`

## Additional Search Coverage

The current report now reflects searches across:

- framework/config owners: `dispatch/`, `dap/`, `dapreader/`, `http/`
- framework runtime dirs from `docs/BES_Framework.md`: `server/`, `ppt/`, `xmlcommand/`, `standalone/`
- additional adjacent dirs checked for completeness: `cmdln/`, `modules/`

What changed relative to the earlier version:

- `server/` and `ppt/` are now part of the explicit search scope instead of being treated as incidental one-off references.
- `modules/` is now part of the explicit search scope for framework-owned keys, because several of those keys are consumed by module code even though the primary templates still live outside `modules/`.
- `standalone/` and `cmdln/` were checked; I did not find additional live readers for the installed framework template keys there beyond `standalone/StandAloneApp.cc` using `TheBESKeys::ConfigFile` to select the config file path.
