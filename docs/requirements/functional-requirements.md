<!-- CONTRACT: what the system must DO. One row per requirement. IDs are permanent —
     never renumbered or reused. Dropped requirements get Status: Deprecated /
     Superseded, not deleted. Delete the _EXAMPLE_ row once you've seen the shape. -->

# Functional Requirements

Convention: `FR-###`, zero-padded, assigned sequentially by next-highest-number.
Priority: `Must` / `Should` / `Could` (MoSCoW). Status: `Proposed` / `Approved` /
`Implemented` / `Deprecated` / `Superseded by FR-0XX`.

| ID | Requirement | Priority | Related Use Cases | Status |
|---|---|---|---|---|
| _EXAMPLE_ | Users can reset a forgotten password via an emailed, time-limited link | Must | UC-EXAMPLE | _delete this row_ |
| FR-001 | NGAP container: a REST path of the form `/collections/<collection-concept-id>[/<label>]/granules/<granule-ur>` is resolved by querying CMR with `collection_concept_id=<collection-concept-id>&granule_ur=<granule-ur>`; the optional `<label>` segment is ignored | Must | TBD | Implemented |
| FR-002 | NGAP container: a legacy REST path of the form `/providers/<provider>/collections/<entry-title>/granules/<granule-ur>` is resolved by querying CMR with `provider`, `entry_title`, and `granule_ur` | Must | TBD | Implemented |
| FR-003 | NGAP container: a legacy REST path of the form `/providers/<provider>/concepts/<collection-concept-id>/granules/<granule-ur>` is resolved by querying CMR with `provider`, `collection_concept_id`, and `granule_ur` | Must | TBD | Implemented |
| FR-004 | NGAP container: a percent-encoded legacy REST path produces the same CMR query as its unencoded form (values are not double-encoded) | Must | TBD | Implemented |
| FR-005 | NGAP container: a REST path that matches none of the forms in FR-001–FR-003 is rejected with a syntax (user) error | Should | TBD | Implemented |
| FR-006 | NGAP container: a CMR response with zero hits for the REST path is reported as a not-found error | Must | TBD | Implemented |
| FR-007 | NGAP container: the https data URL is the first `RelatedUrls` entry with `Type` `GET DATA`, no `Subtype`, and a URL that starts with `http://` or `https://` and does not end in `.xml` or `.dmrpp` | Must | TBD | Approved |
| FR-008 | NGAP container: the S3 data URL is the first `RelatedUrls` entry with `Type` `GET DATA`, no `Subtype`, and a URL that starts with `s3://` and does not end in `.xml` or `.dmrpp` | Must | TBD | Approved |
| FR-009 | NGAP container: the s3credentials (TEA) URL is the first `RelatedUrls` entry with `Type` `VIEW RELATED INFORMATION` and a URL that starts with `http://` or `https://` and ends in `s3credentials` | Must | TBD | Implemented |
| FR-010 | NGAP container: a granule with no https data URL is reported as an error; a missing S3 or s3credentials URL does not fail the request | Must | TBD | Implemented |
| FR-011 | NGAP container: the DMR++ for a granule is located by appending `.dmrpp` to its selected https data URL (FR-007) and, when present, to its S3 data URL (FR-008) | Must | TBD | Implemented |
| FR-012 | DMR++ handler: when the chunk layout in a DMR++ would place data outside the variable's value buffer, the handler throws a `BESInternalError` and copies none of that chunk. At minimum this covers (a) a chunk `chunkPositionInArray` beyond the variable's dimension bounds (`insert_chunk_unconstrained`) and (b) a direct-IO chunk whose offset plus size exceeds the variable's buffer (`insert_chunk_unconstrained_dio`) | Should | TBD | Proposed |

<!-- Add new rows via /new-requirement, or by hand — keep the table format. Each
     requirement should be a single testable statement, not a paragraph. If it needs
     a paragraph, it's probably a use case (docs/requirements/use-cases.md) with this
     as one of its acceptance criteria. -->
