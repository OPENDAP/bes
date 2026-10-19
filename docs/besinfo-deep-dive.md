# BESInfo Deep Dive: How BES Errors Are Built and Returned

## Scope

This covers `dispatch/BESInfo.{h,cc}`, its subclasses (`BESXMLInfo`, `BESTextInfo`,
`BESHTMLInfo`, `BESVersionInfo`), the factory `BESInfoList`, and the path an error
takes from a `throw` to bytes on the PPT socket: `BESInterface`, `BESXMLInterface`,
`BESServerHandler`, `BESError`, and `http::HttpError`.

Findings are marked **Verified** (checked with a test or a direct code trace) or
**Inferred** (follows from reading the code, not tested).

## Summary

`BESInfo` does two jobs:

1. It is the `BESResponseObject` for small informational responses (`showVersion`,
   `showHelp`, `showContainers`, catalog listings, `showBesKey`, and others).
2. It is the error document. When a request fails, `BESInterface::handleException()`
   builds a `BESInfo`, writes the `BESError` into it, and `BESInterface::finish()`
   writes it to the client stream.

The happy path works and has test coverage. The main weaknesses are:

- Error formatting can itself throw, and nothing catches that. In a server child
  process the result is that the child exits and the OLFS gets a dropped connection
  with no error document. Invalid UTF-8 in any error string is enough to cause this.
- `HttpError` sends the unfiltered post-redirect URL, which can include AWS signed URL
  credentials, in both `<Message>` and `<redirect_url>`. This defeats
  `filter_aws_url()`.
- The `errors` context (`<setContext name="errors">xml</setContext>`) has no effect
  because of a case-conversion bug.
- If an error happens after transmission has started, the client gets a mix of
  partial data and the error document.

## Class Structure

```
BESObj
 └─ BESResponseObject
     └─ BESInfo (abstract: add_tag, add_space, add_break, transmit are pure virtual)
         ├─ BESXMLInfo      (libxml2 xmlTextWriter, always buffered)
         ├─ BESTextInfo     (indented "name: value" text)
         ├─ BESHTMLInfo     (HTML)
         └─ BESVersionInfo  (decorator; holds a BESInfo* from BESInfoList and forwards to it)
```

### State in `BESInfo`

| Member | Meaning |
|---|---|
| `_strm` | Output stream. When buffered, it is an owned `ostringstream`. When unbuffered, it is a caller-supplied stream. |
| `_strm_owned` | Whether the destructor deletes `_strm`. |
| `_buffered` | If true, output is accumulated and written by `print()`. If false, `add_data()` writes straight to `_strm`. |
| `_tags` | A stack used only to check that `begin_tag`/`end_tag` calls are balanced. `end_response()` throws if the stack is not empty. |
| `_response_name` | Name of the response element, for example `showVersion`. For errors it is the action name, or `BES`. |

### Constructors and buffering

- `BESInfo()` is always buffered and owns an `ostringstream`.
- `BESInfo(key, strm, owned)` reads a BES key. A value of `true` or `yes` (in three
  capitalizations each) means buffered. Any other value means unbuffered on `strm`, and
  `strm` must not be null. When it chooses buffered and `owned` is true, it deletes the
  passed stream.

How the subclasses use this:

- `BESXMLInfo` has only the default constructor, so it is **always buffered**.
  `BES.Info.Buffered` has no effect on it.
- `BESTextInfo` and `BESHTMLInfo` have both constructors, but their registered builders
  (`BuildTextInfo`, `BuildHTMLInfo`) call the default constructor. So every `BESInfo`
  that comes from `BESInfoList` is buffered.
- The shipped `bes.conf.in` sets `BES.Info.Buffered=no` and `BES.Info.Type=xml`. In
  practice, `BES.Info.Buffered` is a no-op for the server.

### `BESXMLInfo` specifics

- `begin_response()` creates an `xmlBuffer` and `xmlTextWriter`, writes
  `<?xml version="1.0" encoding="ISO-8859-1"?>`, then
  `<response xmlns="http://xml.opendap.org/ns/bes/1.0#" reqID=... reqUUID=...>`, then
  `<{response_name}>`.
- `add_tag()` writes one complete element. `begin_tag()` and `end_tag()` open and close
  an element. Text written with `add_space()` or `add_break()` goes to the
  `ostringstream` first and is flushed into the element by `end_tag()`. Text written
  with `add_data()` goes directly to the writer.
- `end_response()` closes both elements, ends the document, copies the buffer into
  `_doc`, and frees the libxml2 objects. `print()` and `transmit()` call
  `end_response()` themselves if it has not run yet.
- Every libxml2 failure calls `cleanup()` and throws `BESInternalError`.
- The comments in `end_response()` have the two `EndElement` calls in the wrong order.
  The first call closes the specific response element and the second closes
  `<response>`. This has no effect on behavior.

## How Errors Flow

### 1. Something throws

`BESXMLInterface::build_data_request_plan()` and `execute_data_request_plan()` run
inside the `try` block in `BESInterface::execute_request()`
(`dispatch/BESInterface.cc:367-422`). Every exception type ends up in
`handleException()`:

| Caught | Passed to `handleException` as |
|---|---|
| `BESError&` (including `HttpError`, `BESDapError`, and others) | itself (polymorphic, by reference) |
| `std::bad_alloc` | `BESInternalFatalError` |
| `std::exception` | `BESInternalError` |
| `...` | `BESInternalError` |

### 2. `BESInterface::handleException()` builds the document

`dispatch/BESInterface.cc:230-259`:

1. Choose the `BESInfo` type. If the `errors` context equals `"xml"`, use
   `new BESXMLInfo()`. Otherwise use `BESInfoList::TheList()->build_info()`, which reads
   `BES.Info.Type` (default `txt`).
2. `log_error(e)` writes the error to the BES log.
3. Get the admin email from `bes::ServerAdministrator`. If that fails or returns an
   empty string, use `support@opendap.org`.
4. `begin_response(action_name or "BES")`, then `add_exception(e, admin)`, then
   `end_response()`.
5. Return `e.get_bes_error_type()` as the request status.

The result is stored in `dhi.error_info`, a raw owning pointer.

Because `BESXMLInterface::execute_data_request_plan()` reassigns `d_dhi_ptr` to each
command's own DHI, an error during execution attaches to that command's DHI. An error
during plan building attaches to the interface's DHI. `finish()` reads `d_dhi_ptr`, so
both cases are handled.

### 3. `BESInfo::add_exception()` defines the format

`dispatch/BESInfo.cc:218-236`. With `BES.Info.Type=xml`, the output is:

```xml
<?xml version="1.0" encoding="ISO-8859-1"?>
<response reqID="..." reqUUID="..." xmlns="http://xml.opendap.org/ns/bes/1.0#">
    <getDODS>                         <!-- dhi.action_name, or "BES" -->
        <BESError>
            <Type>3</Type>            <!-- BES_*_ERROR code, see BESError.h -->
            <Message>...</Message>
            <Administrator>support@opendap.org</Administrator>
            <!-- error.add_my_error_details_to(*this) inserts subclass details here -->
            <Location>
                <File>...</File>
                <Line>...</Line>
            </Location>
        </BESError>
    </getDODS>
</response>
```

Type codes are defined in `dispatch/BESError.h`: 1 internal, 2 internal fatal, 3
syntax/user, 4 forbidden, 5 not found, 6 timeout, 7 HTTP.

`BESError::add_my_error_details_to(BESInfo&)` is the only extension point, and only
`http::HttpError` overrides it. It adds `curl_code`, `http_status`, `origin_url`,
`redirect_url`, `<response_headers><header>...</header></response_headers>`, and
`response_body`. These elements appear between `<Administrator>` and `<Location>`, so
the element order of an HTTP error differs from other errors. An OLFS parser that reads
elements by position would break on this. I did not check the OLFS code.

The `<File>` and `<Line>` elements always contain source paths, in every build mode.
`BES.DoNotLogSourceFilenames` only affects the log (`log_error`), not the response. The
test suites remove these elements with `sed` (`cmdln/tests/testsuite.at:157`,
`modules/common/handler_tests_macros.m4:801-812`).

### 4. `BESServerHandler::execute()` puts it on the wire

`server/BESServerHandler.cc:194-260`:

- `status == 0`: call `cmd.finish(0)`, then `fds.finish()`.
- `status != 0`:
  1. Send the PPT extension chunk `status=error;`, plus `exit=true;` when the status is
     `BES_INTERNAL_FATAL_ERROR`.
  2. `cmd.finish(status)` runs `error_info->print(d_strm)`, which writes the buffered
     document to the `PPTStreamBuf` stream, then deletes `error_info`.
  3. `fds.finish()` sends the last chunk.
  4. On fatal errors, exit the child. On internal errors, exit only if
     `BES.ExitOnInternalError=true`. Other errors keep the connection open.

`BESXMLInterface::transmit_data()` has a branch that sends `error_info` through the
transmitter (`xmlcommand/BESXMLInterface.cc:400-409`). In normal flow this branch never
runs, because `error_info` is set only by `handleException()`, which runs after the
command loop exits. The only other code that touches `error_info` is
`server/test/TestException.cc`.

## Defects and Risks

Ordered by impact on deployed servers.

### D1. Error formatting can throw, and nothing catches it (Verified)

`handleException()` is called from inside `catch` blocks in `execute_request()`. If
`handleException()` throws, the new exception leaves `execute_request()`. It is not
caught by `BESServerHandler::execute()`. It reaches the `catch (BESError&)` in
`ServerApp::run()` (`server/ServerApp.cc:546`), which logs it and returns
`SERVER_EXIT_FATAL_CANNOT_START`, so the child exits. No `status=error` extension and no
error document are sent. The OLFS sees the connection close in the middle of a request.

Ways `handleException()` can throw:

- **Invalid UTF-8 in any string written to `BESXMLInfo`.** libxml2's text writer
  expects UTF-8 input. The document encoding is ISO-8859-1, and the encoder fails on
  invalid input. I tested this with the system libxml2 2.9.13 on macOS and a small C
  program that copies `BESXMLInfo`'s calls:

  | Input to `xmlTextWriterWriteString` | Result |
  |---|---|
  | `hello <&> world` | Correct, escaped |
  | Valid UTF-8 `café €` | Correct (`é` as a Latin-1 byte, `€` as `&#8364;`) |
  | `bad \xff\xfe bytes` | `xmlTextWriterEndDocument` returns -1, "I/O error : encoder error" |
  | PNG magic `\x89PNG...` | -1 |
  | Control bytes `\x01\x02` | rc OK, but the output contains raw `^A^B`, which is **not well-formed XML 1.0** |

  When the writer returns -1, `BESXMLInfo` throws `BESInternalError`. Strings that can
  realistically contain invalid UTF-8 include `HttpError::d_response_body` (an arbitrary
  origin response body, possibly binary or gzip), response headers, Latin-1 file paths
  in messages, and messages from third-party libraries. I did not check which libxml2
  version the hyrax-dependencies build links against. Behavior could differ, but the
  encoder failure is long-standing libxml2 behavior.
- **`BES.Info.Type` set to an unregistered value** (for example `XML` or `json`).
  `build_info()` returns `nullptr` and `handleException()` dereferences it, so the child
  crashes with SIGSEGV instead of throwing. This needs a configuration error to happen.
- Any other libxml2 writer failure, or unbalanced tags from a future
  `add_my_error_details_to` override.

Suggested fix: make `handleException()` unable to fail. Wrap the build in
`try/catch(...)`, and on failure build a minimal, hand-escaped fallback document.
Separately, sanitize strings before they reach `xmlTextWriterWriteString`: replace
invalid UTF-8 sequences and drop control characters that XML 1.0 does not allow.

### D2. `HttpError` sends AWS signed-URL credentials to the client (Verified by code trace)

`filter_aws_url()` exists to remove `X-Amz-*` query parameters from URLs before they
appear in errors. In `process_http_code_helper()` (`http/CurlUtils.cc:846-890`), the
caller deliberately passes the unfiltered effective URL because retry logic needs it
(`http/CurlUtils.cc:1005-1008`). That URL then goes into:

- `<Message>`: `"The response from " << last_accessed_url << ...` (line 854), for all
  4xx and non-retryable 5xx responses.
- `<redirect_url>`, through `HttpError::add_my_error_details_to()`.

The path at `CurlUtils.cc:1716` also passes `redirect_url_str`, the complete response
headers, and the complete body without filtering.

The common case is an S3 403 after a TEA/EDL redirect. The BESError then contains
`X-Amz-Credential`, `X-Amz-Security-Token`, and `X-Amz-Signature`. These are
short-lived but valid until they expire. The values also go to the BES log through
`log_error()`. How the OLFS shows `<Message>` to end users is outside this repository.

Suggested fix: filter the URL when it is put into the message and when the exception is
constructed. Keep the unfiltered URL only for the retry decision.

### D3. The `errors` context has no effect (Verified by code trace)

```cpp
static inline void downcase(string &s) {
    transform(..., [](int c) { return std::toupper(c); });   // upper-cases
}
...
downcase(context);
if (found && context == XML_ERRORS)   // XML_ERRORS is "xml"
```

The context value is converted to upper case and then compared with lowercase `"xml"`,
so the comparison is always false. Several `.bescmd` tests send
`<setContext name="errors">xml</setContext>`, but they pass only because
`BES.Info.Type=xml` in their configuration. The production result is the same because
the shipped default is also `xml`. A site with `BES.Info.Type=txt` would send text
errors even when the OLFS asks for XML.

Fix: use `tolower` (and rename nothing, or rename the helper accurately). This is a
behavior change for sites that use non-XML info types. The risk is low but it should be
noted.

### D4. Errors after transmission has started produce a mixed stream (Inferred)

If a transmitter has already written part of a response to `my_ostrm` and then throws,
two things follow:

- Bytes already flushed as `d` chunks have reached the OLFS as data.
- `sendExtensions()` writes the `status=error` chunk directly to the socket
  (`ppt/PPTConnection.cc:162-181`). Bytes still in the `PPTStreamBuf` buffer are flushed
  later, after the extension and before the error XML, because `print()` writes into
  the same stream.

The OLFS then receives `status=error` followed by partial payload and the XML document,
which will not parse as XML. Per the comment at `BESXMLInterface.cc:388-391`, this is a
known limitation of the design. The buffer-ordering detail is inferred from the code and
not tested. A smaller fix is to discard unflushed `PPTStreamBuf` contents before
`sendExtensions()`. That needs care, because data already sent cannot be taken back.

### D5. Ownership of `error_info` is a raw, shallow-copied pointer (Inferred, latent)

`BESDataHandlerInterface::clone()` copies `error_info` by pointer
(`dispatch/BESDataHandlerInterface.cc:103`), and `finish()` deletes it through
`d_dhi_ptr`. This is safe today, because command DHIs are cloned while `error_info` is
null and only one DHI ever has it set. A future change that clones after an error, or
calls `handleException()` twice, would leak or double-free. `handleException()` also
does not check whether `error_info` is already set before overwriting it.

### D6. Smaller issues in `BESInfo.cc`

- `add_data_from_file()` (`BESInfo.cc:195-202`):
  - A line of 4095 or more characters sets `failbit`. After that `getline` never sets
    `eof`, so `while (!ifs.eof())` loops forever. (Inferred from standard `istream`
    behavior.)
  - A final line without a trailing newline is dropped, because `eof` is set on the
    same read that returns it.
  - `errno` is read after the `ifstream` constructor, so the reported error is usually
    correct, but nothing guarantees it.

  This is used for help text, not errors, but a hang in a child is serious.
- `print()` casts `_strm` with a C-style cast to `ostringstream*`. This is correct only
  because buffered implies an owned `ostringstream`. A `static_cast` with that invariant
  stated would be clearer.
- `begin_response()` uses the action name as an XML element name without checking it.
  Today the names are internal constants, so this is not exploitable.

## Test Coverage

| Covered | Where |
|---|---|
| `BESInfoList` builder registration and basic Text/HTML/XML output | `dispatch/unit-tests/infoT.cc` |
| `HttpError` details through `BESXMLInfo::add_exception` | `http/unit-tests/HttpErrorTest.cc:199-281` |
| End-to-end error XML for many handlers | About 68 `*.baseline` files that contain `<BESError>` |

Not covered:

- `handleException()` failure (D1). A unit test should pass an `HttpError` with
  `response_body = "\xff\xfe"` through `BESXMLInfo` and require a well-formed document,
  not an exception.
- The `errors` context with `BES.Info.Type=txt` (D3).
- URL filtering in `HttpError` messages and details (D2).
- Errors that happen after streaming has started (D4).

## Suggested Order of Work

1. D2: filter credentials. Small diff, security impact.
2. D1: make `handleException()` unable to fail and sanitize text. Small diff, with
   regression tests.
3. D3: one-character fix. Note the possible behavior change for non-XML sites.
4. D6: `add_data_from_file` loop. Small diff.
5. D4: needs design discussion. It affects how the OLFS interprets the PPT stream.
