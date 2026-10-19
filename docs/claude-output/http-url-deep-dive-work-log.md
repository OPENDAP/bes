# Work Log: `http/` Deep Dive Analysis

## 2026-06-12

### Query/Prompt

> Perform a deep dive on the code in `http/`. Can this be simplified? Is there redundancy that can be eliminated? How effective is the `url` class defined by `url_impl.h` and `url_impl.cc`? What would be the effect of replacing that with a set of functions? To complete this analysis, make sure that you check modules that use this library in addition to the BES framework itself. Modules that likely use this include `dmrpp_module` and `cmr_module`.

---

### Approach

The request was open-ended exploration: no single file to read, no existing bug to trace. I delegated to an Explore subagent to do the broad read across `http/`, `modules/dmrpp_module/`, and `modules/cmr_module/` without consuming the main context window with raw file contents. The agent was asked to:

1. Enumerate and fully read all `.h` and `.cc` files in `http/`.
2. Grep for `url` class usages across the three domains.
3. Count method call frequencies to separate heavily-used API surface from dead weight.
4. Identify explicit `TODO` comments and redundant overload sets.
5. Assess the degree of real runtime polymorphism in the `url`/`EffectiveUrl` hierarchy.

No assumptions were made about what the analysis would find before the agent ran.

---

### Findings and Reasoning

**On the `url` class design:**

The class had 15 virtual methods but only one subclass (`EffectiveUrl`), which overrode exactly three of them. That immediately flags the virtual declarations as vestigial rather than actively used. The deleted assignment operator and the TODO comments on shared_ptr constructors confirmed the class is mid-migration from heap-allocated shared ownership toward value semantics.

The method frequency count was the most diagnostic step. Five methods (`str`, `path`, `host`, `protocol`, `query`) account for >90% of production call sites. The remaining methods — `query_parameter_values_size`, `set_ingest_time`, `get_url_no_query` — are near-zero in production. That asymmetry argues against retaining a full class hierarchy for what is essentially a struct plus a parse function.

**On redundancy:**

The TODO comments (dated 2/20/25 with jhrg initials) explicitly mark the `shared_ptr` overloads in `awsv4`, `CurlUtils`, and `AllowedHosts` for removal. Four separate classes each carry a paired `shared_ptr`/value overload set. This is the mechanical residue of an in-progress migration the author started but did not finish. The correct action is to complete that migration before considering any larger refactor.

**On replacing the class with free functions:**

The encapsulation argument for a class is weaker when: (a) all methods are simple getters or predicates, (b) no invariant needs to be enforced at mutation time (this class is immutable after construction so there is no mutation), and (c) the only subclass overrides a single predicate. All three conditions hold here. A `ParsedUrl` struct plus free functions produces the same semantics with less machinery.

The `EffectiveUrl` subclass collapses cleanly: it adds only `response_headers` storage and overrides `is_expired()` to read from those headers. A `response_headers` field on `ParsedUrl` plus a free `is_expired()` that checks both KVPs and headers (dispatching on whether headers are populated) handles the same logic without inheritance.

**On scope of change:**

~150 call sites sounds large but they are almost entirely the same substitution repeated: `->str()` → `.raw`, `->path()` → `.path`, etc. The only structural change is `Chunk` in `dmrpp_module`, which has 8+ constructors taking `shared_ptr<http::url>` — those need to become value constructors. That is mechanical but fiddly.

The recommended two-PR sequence (finish existing TODOs first, then convert to struct) reduces risk by validating the simpler cleanup independently before tackling the larger rename.

---

### What Was Not Checked

- `gateway_module`, `httpd_catalog_module`, `s3_reader` were noted as consumers but not read in depth. The Explore agent confirmed each has 1–3 `url` creation sites, all using the string constructor and passing to `RemoteResource`. They are low-risk for the proposed refactoring but should be verified before any PR lands.
- The unit test suite in `http/tests/` was surveyed for method usage but not audited for coverage gaps.
- Runtime behavior was not tested. The analysis is static only.

---

### Output

- Analysis document: `docs/http-url-deep-dive.md`
- This work log: `docs/http-url-deep-dive-work-log.md`
