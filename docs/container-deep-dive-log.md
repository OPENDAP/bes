# Container Deep Dive: Work Log

## 2026-10-15

**Prompt:** "perform a deep dive on the BESContainer code and its child classes. How do
they function, what do that do and where do they interact with the overall framework.
Pay particular attention to the NgapOwnedContainer.h and the BESFileContainer.h. Make
sure to include the BESFileContainerStorage.h and BESContainerStorage.h and
BESContainerStorageList.h classes."

**Output:** [`container-deep-dive.md`](container-deep-dive.md)

### What I did

- **Read the core code in full.** In `dispatch/`:
  - `BESContainer`, `BESFileContainer`;
  - `BESContainerStorage`, `BESContainerStorageVolatile`, `BESFileContainerStorage`, `BESContainerStorageList`;
  - the parts of `BESUncompressManager3::uncompress()` and `BESFileLockingCache::unlock_and_close()` that `BESFileContainer` depends on.

  In `modules/dmrpp_module/`:
  - `NgapOwnedContainer.{h,cc}` and `NgapOwnedContainerStorage.{h,cc}`;
  - `NgapNames.h`, `ngap.conf.in`, `MemoryCache.h`;
  - the `initialize()` and `open_cache_info()` methods in `dispatch/FileCache.h`;
  - the dmrpp call sites: `DmrppRequestHandler.cc:295-360`, `ReturnAsDmrppTransmitter.cc`, and `DmrppModule::initialize/terminate`.
- **Delegated two read-only surveys to subagents:**
  1. the S3, Gateway, CMR and HttpdCatalog containers and their stores, plus `BESContainerStorageFile`;
  2. the framework call sites: XML commands, `BESDefine`, the DHI, `access()` and `release()` callers, module registration, tests and existing docs.
- **Checked the subagents' key claims myself:**
  - `BESInterface::end_request`, `BESRequestHandlerList::execute_current`, `BESSetContainerResponseHandler::execute`, `~BESDefine`, `BESDefaultModule.cc:99-100`, and `bes.conf.in:173`;
  - the S3 trusted-URL path (`S3Container.cc:143-170`) and the AllowedHosts trusted short-circuit (`AllowedHosts.cc:161`);
  - `curl::http_get(string)`, which has no AllowedHosts check;
  - `GatewayContainer::_duplicate`.
- **Ran the existing unit tests** on the in-tree build:
  - `dispatch/unit-tests/containerT`: OK, 7 tests;
  - `ngap_container/unit-tests/NgapOwnedContainerTest`: OK, 30 tests.
- **Wrote a scratch probe** (in the session scratchpad, not the repo) that calls `FileCache::initialize()` 50 times and counts open file descriptors: 3 before, 53 after. This confirms that each `NgapOwnedContainer` construction with `NGAP.UseDMRppCache=true` leaks one descriptor.

### Corrections to subagent output

- **Double `access()` lock leak: false.** One subagent suspected that calling `access()` twice on a compressed `BESFileContainer` leaks a read lock. `unlock_and_close()` loops over all of the process's descriptors for that file, so one `release()` drops both locks. I reported the narrower real issue instead: shared locks between two containers on the same file.
- **"The dmrpp library is dlclosed": false.** One subagent assumed this when assessing the orphaned `"ngap"` store. There is no `dlclose` in `dispatch/`, `server/` or `standalone/`, so the store's destructor code is still mapped at exit.
- **"The DMR name becomes 'as-string'": partly wrong.** `DMZ.cc:304` overwrites the name from `<Dataset name>`. Only the DMR's filename stays `"as-string"`.
- **`NGAP.UseOPeNDAPBucket` defaults: the subagent was only partly right.** The subagent said it defaults to true. That is the code default; the shipped `ngap.conf.in` sets it to false. The document states both.

### Reasoning notes

- **Why the fd leak ranks highest.** It happens on the normal NGAP request path, with the shipped configuration, once per request, in long-lived `beslistener` children. The other bugs are either latent (the size compounding, `_duplicate`), affect only debug output, or need a non-default setting.
- **Why the security findings are framed as decisions.** Fixing either one (the S3 trusted URL, the NGAP `source/` path) could break deployments that rely on fetching from arbitrary hosts, so the document lists them as needing a decision rather than as straightforward fixes.
- **No code changes.** The request was for analysis.

### Not done

- No integration (autotest) runs.
- No live NGAP, CMR or S3 requests.
- No Linux testing.
- I did not build a reproducer for the `parts[1]` out-of-bounds read or for the `BESFileContainerStorage` empty-name undefined behavior. Both come from a code trace.
