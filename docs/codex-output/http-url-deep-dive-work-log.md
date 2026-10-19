# HTTP URL Deep Dive Work Log

## 2026-06-11 17:16:40 MDT

Query/prompt:

> Perform a deep dive on the code in `http/`. Can this be simplified? Is there redundancy that can be eliminated? How effective is the `url` class defined by `http/url_impl.h` and `http/url_impl.cc`? What would be the effect of replacing that with a set of functions? To complete this analysis, make sure that you check modules that use this library in addition to the BES framework itself. Modules that likely use this include `modules/dmrpp_module/` and `modules/cmr_module/`.

Plan:

1. Inventory `http/` sources and build integration.
2. Trace `http::url` and `url_impl.h` usage across BES, AWS support code, CMR, DMR++, and other modules.
3. Inspect `url_impl` behavior, tests, caches, `RemoteResource`, `AllowedHosts`, curl helpers, and module call sites.
4. Identify simplification opportunities, redundancy, bugs, and migration risk.
5. Write the markdown analysis and this work log in `docs/`.

Reasoning and observations:

- I confirmed the repo and docs instructions. `docs/AGENTS.md` requires keeping `docs/Makefile.am` current for docs changes.
- I ignored `retired/` and avoided treating data fixtures as source code.
- I used `rg` to find `http::url`, `url_impl.h`, `RemoteResource`, curl helpers, and URL encoding/building call sites.
- I read `http/url_impl.h`, `http/url_impl.cc`, `HttpUrlTest`, `AllowedHosts`, `RemoteResource`, `CurlUtils`, `EffectiveUrl`, `EffectiveUrlCache`, `CredentialsManager`, `awsv4`, `HttpUtils`, CMR call sites, DMR++ `DMZ`, `Chunk`, `DmrppCommon`, `CurlHandlePool`, NGAP code, and other module users.
- I found that `http::url` is not just a parser. It also performs catalog-relative path conversion, carries trust, and implements AWS/CloudFront expiration policy.
- I found that CMR uses `http::url` shallowly through `RemoteResource`, while DMR++ carries it through chunk and curl handle lifetimes.
- I found concrete bug candidates in `CredentialsManager::get(const string&)` longest-prefix matching and NGAP `.dmrpp` suffix removal.
- I concluded that replacing `http::url` with loose functions would move state into scattered parameters. A smaller value type plus focused helper functions is the safer simplification.

Validation:

- Analysis only. I did not run `./configure`, `make`, `make check`, unit tests, `make distcheck`, Docker builds, or network-backed tests.
- Commands used were source-inspection commands such as `find`, `date`, `rg`, `sed`, `nl`, and `git status --short`.
