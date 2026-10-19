# XML Command Processing Deep Dive: Work Log

## 2026-10-13

**Prompt:** "Perform a deep dive on the code in `xmlcommnd` and `dispatch` that read and
process requests to teh BES. Likely entry points are @dispatch/BESInterface.h and its
implementation @dispatch/BESInterface.cc, although the code in `server` (e.g.,
`BESServerHandler::execute(Connection *connection`) is probably where the commands enter
into the BES command processing stack. See @xmlcommand/BESXMLInterface.h and
@xmlcommand/BESXMLInterface.cc."

**Output:** [`xml-command-processing-deep-dive.md`](xml-command-processing-deep-dive.md)

### What I read

- `server/BESServerHandler.cc` and `standalone/StandAloneClient.cc`. These are the only
  two callers of `BESXMLInterface`.
- `dispatch/BESInterface.{h,cc}` and `xmlcommand/BESXMLInterface.{h,cc}`.
- `xmlcommand/BESXMLCommand.{h,cc}`, `BESXMLUtils.cc`, `BESXMLDefaultCommands.cc`,
  `BESXMLDapCommandModule.cc`, and the parsers for `get`, `define`, `setContainer`,
  `setContext`, and `setContexts`. I also read `NullResponseHandler.h`,
  `SetContextsResponseHandler.cc`, and every command's `has_response()`.
- In `dispatch/`: `BESDataHandlerInterface` (`clone`/`clean`),
  `BESResponseHandlerList::find_handler`, `BESRequestHandlerList::execute_current`,
  `BESTransmitter::send_response`, the define, setContainer, and setContext response
  handlers, `BESDefine` ownership, `BESContainerStorageVolatile::look_for`,
  `TheBESKeys::load_dynamic_config`, `RequestServiceTimer`, and `BESDataNames.h`
  (`XML_ERRORS`).
- `dap/BESDDSResponseHandler.cc`, as an example of `execute()` and `transmit()`.
- The existing `ppt-server-deep-dive.md` and `besinfo-deep-dive.md`, so I would not
  repeat their findings. The `errors` context bug and the mixed stream after a
  mid-transmit error were already there. I cross-referenced them instead of reporting
  them as new.

### Reasoning notes

- I followed `d_dhi_ptr` through each method. It is the one pointer the base class
  uses, and the subclass moves it in three places: `execute_data_request_plan`,
  `log_status`, and `clean`. That is where the F3 problems come from.
- The `has_response` assignment looked wrong on first read (`=` where it should
  accumulate with OR). I checked it live instead of trusting the read.
- I checked container ownership on purpose. `look_for()` returns a duplicate, and
  `BESDefine` deletes its containers, so the define→get path has no double free. I
  recorded that as a non-finding in the flow description.
- `load_dynamic_config` looked important until I found `DYNAMIC_CONFIG_ENABLED 0`.
  I reported it as dead code.
- For F4 (the dangling libxml2 handler), I did not try to trigger a generic-channel
  libxml2 error after parsing. It stays marked Inferred.

### Live probes (`besstandalone`, `xmlcommand/tests/bes.conf`, files in the scratchpad)

| Probe | Result |
|---|---|
| `get das`, `setContainer`, `define`, `get dds` | A full DAS and then a full DDS in one output. No error. (F1) |
| `get das`, `get dds` next to each other | Rejected: "Commands with multiple responses not supported." |
| The first probe, with the second `get` using a `.das` container | A DAS, then a `<getDDS><BESError>` appended to it |
| `setContext errors=xml` with `BES.Info.Type=txt` | The error came back as text (F5, besinfo D3) |
| `BES.LogVerbose=yes`, `get` on a missing definition | The log said `... completed` after the `ERROR!` line (F3) |
| `<dap4constraint bogus="x">` as the first child of `<container>` | Rejected |
| The same attribute, after `<constraint></constraint>` | Accepted (F6) |
| `<setContext>` without a name | Syntax error returned. The leak (F7) was not measured |

`besstandalone` exited 0 for all of these, including the error cases.

### Not done

- No `make check`, no ASan or valgrind, and no requests over PPT through
  `beslistener`. No code changes.
