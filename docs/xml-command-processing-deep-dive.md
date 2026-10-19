# XML Command Processing Deep Dive: How the BES Reads and Runs a Request

## Scope

This covers the path a request takes after the PPT layer has read it off the socket.
That path is `BESServerHandler::execute()` (or `StandAloneClient::executeCommand()`),
then `BESInterface`, then `BESXMLInterface`, then the `BESXMLCommand` subclasses in
`xmlcommand/`, then the `BESResponseHandler`s, `BESRequestHandler`s, and
`BESTransmitter`s in `dispatch/` and `dap/`.

Sockets, PPT framing, and process management are in
[`ppt-server-deep-dive.md`](ppt-server-deep-dive.md). How error documents are built is
in [`besinfo-deep-dive.md`](besinfo-deep-dive.md). This document does not repeat them.

Findings are marked:

- **Verified (live)**: reproduced with the in-tree `besstandalone` (macOS arm64,
  `--enable-developer` build from 2026-10-09), using `xmlcommand/tests/bes.conf` and
  `.bescmd` files in the session scratchpad.
- **Verified (trace)**: confirmed by reading the code along the full path.
- **Inferred**: follows from the code. Not tested.

Nothing in the repository was changed.

## Summary

The design is a template method with a command factory. `BESInterface::execute_request()`
fixes the order of the steps: build the plan, start the timer, execute the plan. The
`finish()` call afterwards writes any error document and cleans up.
`BESXMLInterface` parses the XML document with libxml2. It turns each child of
`<request>` into a `BESXMLCommand`, using a static name→builder map. Each command gets
its own copy of the request's `BESDataHandlerInterface` (DHI). The commands then run in
document order. Each one runs `prep_request()`, then its `BESResponseHandler::execute()`,
then `transmit()`.

The structure works, and most of the problems are in the details:

1. **The "only one command may return a response" rule can be bypassed**
   (Verified, live). The check overwrites its flag instead of accumulating it. Any
   command without a response between two `get`s resets it. One request then streams
   two responses back to back.
2. **State from one request leaks into later requests in the same `beslistener`
   child** (Verified, trace). Contexts are never cleared. Containers and definitions
   last for the life of the process, and so does the libxml2 generic error handler.
   That handler points at a stack variable that no longer exists.
3. **The cleanup and logging steps after an error act on the wrong command, or on
   cleared state** (Verified, live, for the log). The verbose log says
   `completed` for a request that failed. `end_request()` releases containers only for
   whichever command `d_dhi_ptr` happens to point at.
4. **Several validation and lifetime bugs in the command parsers** (Verified, live, for
   `<define>`). These are in [Findings](#findings).

`d_dhi_ptr` is the main source of fragility. It is a single protected pointer. It starts
at the request's base DHI, and three different methods then move it to point into
command objects. Every later step depends on where the previous step left it.

## The Call Chain

```
server/BESServerHandler::execute(Connection*)          standalone/StandAloneClient::executeCommand()
  loop: PPTConnection::receive() -> cmd_str                read .bescmd file -> cmd
  BESXMLInterface cmd(cmd_str, &ppt_ostream)               new BESXMLInterface(cmd, _strm)
        │                                                        │
        └──────────────────────┬─────────────────────────────────┘
                               ▼
  BESInterface::execute_request(from)                    dispatch/BESInterface.cc:337
    d_dhi_ptr->set_output_stream, data[REQUEST_FROM], data[SERVER_PID]
    d_transmitter = BASIC_TRANSMITTER
    try {
      build_data_request_plan()        ── BESXMLInterface: parse XML, build commands
      set_bes_timeout()                ── context "bes_timeout" or key BES.TimeOutInSeconds
      RequestServiceTimer::start()
      execute_data_request_plan()      ── BESXMLInterface: for each command:
                                            prep_request(); response_handler->execute();
                                            throw_if_timeout_expired(); transmit_data()
      clear_bes_timeout()
    } catch (...) { status = handleException(e, *d_dhi_ptr) }   // builds dhi.error_info
  BESInterface::finish(status)                            dispatch/BESInterface.cc:433
    print error_info to the stream, log_status(), end_request()  // release containers
  ~BESXMLInterface -> clean()                             // delete commands + response handlers
```

The two callers differ in one way. On error, `BESServerHandler` sends a
`status=error` PPT extension before it calls `finish()`
([BESServerHandler.cc:200-213](../server/BESServerHandler.cc#L200-L213)). It then exits
the child if the error is fatal. `StandAloneClient` calls `finish()` the same way in
both cases, and it exits only on `BES_INTERNAL_FATAL_ERROR`. `besstandalone` returns 0
for user and internal errors. This was seen in every probe below.

There is no other user of `BESInterface`. `BESXMLInterface` is its only subclass, and
these two call sites are its only callers. `transmit_data()`, `log_status()`, and
`clean()` are pure virtual in the base, but only the subclass calls them.

## Step by Step

### 1. `BESInterface::execute_request()`

[BESInterface.cc:337-425](../dispatch/BESInterface.cc#L337-L425)

- It sets three DHI values: the output stream, `REQUEST_FROM`, and `SERVER_PID`. These
  go into the base DHI *before* the commands are built, so every command's copy of the
  DHI gets them.
- It resets `d_transmitter` to the basic transmitter for every request.
  `BESXMLInterface::transmit_data()` replaces it when a command has a `returnAs`.
- The timer starts only after the plan is built. That lets a `<setContext
  name="bes_timeout">` in the same document take effect. Parsing is not timed.
- The exception handlers map `bad_alloc` to `BESInternalFatalError`, which makes the
  server child exit. They map other `std::exception`s and `...` to
  `BESInternalError`.
- `clear_bes_timeout()` runs only on success. After an error the timer is still
  armed. That is harmless because `RequestServiceTimer` is a deadline that code checks,
  not a signal, and the next request restarts it.

### 2. `BESXMLInterface::build_data_request_plan()`

[BESXMLInterface.cc:78-283](../xmlcommand/BESXMLInterface.cc#L78-L283)

1. It installs `BESXMLUtils::XMLErrorFunc` as the libxml2 *generic* error handler. The
   handler's context is the address of a local `vector<string> parse_errors` (`:96-97`).
2. It parses with `xmlReadMemory(..., XML_PARSE_NONET)`. On failure it throws
   `BESSyntaxUserError` with the collected messages.
3. The root element must be named `request`, and it must not have a text value. The
   namespace is not checked, so `<bes:request>` and `<request>` both work.
4. It reads the request-id attributes, `reqID` and `reqUUID`, into the base DHI. It
   makes a UUID if there is none, and calls `BESLog::set_request_id(reqID-reqUUID)`. If
   `reqID` is missing, the id becomes the literal
   `"BESXMLInterface::build_data_request_plan() - NoRequestIdDetected"`. That value is
   also the one sent downstream.
5. It walks the element children of `<request>`. For each one:
   - **`setContainer` special case** (`:186-193`): it calls
     `TheBESKeys::load_dynamic_config(value)`. `DYNAMIC_CONFIG_ENABLED` is `0`
     ([TheBESKeys.h:49](../dispatch/TheBESKeys.h#L49)), so this does nothing.
   - It looks up the builder with `BESXMLCommand::find_command(name)` and calls
     `bldr(d_xml_interface_dhi)`. The `BESXMLCommand` constructor copies the base DHI
     with `make_copy()`, which is a shallow copy of every field. Unknown names throw
     `BESSyntaxUserError("Unable to find command for X")`.
   - **`setContext` special case** (`:211-230`): it parses the command and runs its
     response handler *right away*, during parsing. That calls
     `BESContextManager::set_context()`. Then it deletes the command. It is never added
     to `d_xml_cmd_list`.
   - Every other command is pushed onto `d_xml_cmd_list`. The code then checks the
     "one response" rule and calls `parse_request(node)`. Finally it checks that the
     `RETURN_CMD` transmitter exists, so a bad `returnAs` fails before any work is
     done.
6. It calls `xmlFreeDoc` and `xmlCleanupParser()` on both the normal and the exception
   path.

`parse_request()` in each command fills that command's DHI. It sets `action`, `data[...]`,
and `d_cmd_log_info`. It then calls `BESXMLCommand::set_response()`, which does two
things:

- It calls `BESResponseHandlerList::find_handler(action)`. This is a factory: it
  creates a **new** response handler object on every call.
- It copies the log text into `data[LOG_INFO]`.

If no handler is registered for the action, it throws `BESSyntaxUserError`. So an
unknown `get type="xyz"` fails during parsing.

### 3. `BESXMLInterface::execute_data_request_plan()`

[BESXMLInterface.cc:351-381](../xmlcommand/BESXMLInterface.cc#L351-L381)

For each command, in document order:

1. `prep_request()`. `define` looks up its containers here. That allows `define` to
   follow `setContainer` in the same document. `get` looks up its definition here and
   copies that definition's container pointers into its DHI.
2. `d_dhi_ptr = &cmd->get_xmlcmd_dhi()`. From here on, the base class works on this
   command's DHI.
3. `log_the_command()`. This writes the request-log line, but only for `get.*`
   actions. It reads `containers.begin()` without checking that the list is non-empty.
   That is safe today only because `prep_request()` for `get` always adds at least one
   container or throws.
4. `response_handler->execute(dhi)`. For DAP responses (for example
   [BESDDSResponseHandler](../dap/BESDDSResponseHandler.cc)), this builds the response
   object, sometimes from the metadata store. Otherwise it calls
   `BESRequestHandlerList::execute_each(dhi)`. That iterates over the DHI's containers.
   For each one it calls `container->access()`, finds the `BESRequestHandler` for the
   container's type, finds the method for `dhi.action`, and calls it. This is where the
   data handler modules (netCDF, HDF5, DMR++, ...) run.
5. `RequestServiceTimer::throw_if_timeout_expired()`. This is the last point at which
   a timeout gives the client a clean error.
6. `transmit_data()`. It uses the `RETURN_CMD` transmitter if one is set, and the basic
   transmitter otherwise. It calls `response_handler->transmit(transmitter, dhi)`, which
   calls `transmitter->send_response(service_name, response_object, dhi)`.

`transmit_data()` has a branch for `d_dhi_ptr->error_info`. Nothing sets `error_info`
before this point on the normal path, because exceptions skip `transmit_data()`. The
branch is effectively dead.

### 4. `finish()`, `log_status()`, `end_request()`, `clean()`

- `finish()` prints `d_dhi_ptr->error_info` and **deletes it**. Then it calls
  `log_status()` and `end_request()`. Any exception from those two is logged and
  swallowed.
- `log_status()` writes a line only in verbose mode. It moves `d_dhi_ptr` to each
  command in turn, and reports `failed` if `error_info` is set. See finding F3.
- `end_request()` calls `release()` on the containers in the current `d_dhi_ptr`.
  For a file container, `release()` unlocks the uncompress cache file.
- `clean()` runs from the destructor. It calls `dhi.clean()` on each command, which
  deletes the response handler, and then deletes the command.

## Command Registry

The commands are registered in `BESXMLDefaultCommands::initialize()`. It is called
directly from `ServerApp.cc:428` and `StandAloneApp.cc:181`, not through `BES.modules`.
Some more commands come from `BESXMLDapCommandModule`, which is the
`libdap_xml_module.so` module.

| Element | Command class | `action` | Response? | When it takes effect |
|---|---|---|---|---|
| `setContext` | `BESXMLSetContextCommand` | `set.context` | no | **during parsing** |
| `setContexts` | `XMLSetContextsCommand` | `null` | no | **during parsing** (`parse_request` calls `set_context`, because `USE_CONTEXTS_RESPONSE_HANDLER` is 0) |
| `setContainer` | `BESXMLSetContainerCommand` | `set.container` | no | execution |
| `define` | `BESXMLDefineCommand` | `define` | no | execution (`prep_request` + handler) |
| `get` | `BESXMLGetCommand` | `get.<type>` (lowercased) | yes | execution |
| `deleteContainer(s)`, `deleteDefinition(s)` | `BESXMLDelete*Command` | `delete.*` | no | execution |
| `showContext`, `showDefinitions`, `showContainers`, `showHelp`, `showVersion`, `showStatus`, `showServiceDescriptions` | `BESXMLShowCommand` | `show.<x>` | yes | execution |
| `showError` | `BESXMLShowErrorCommand` | | no | execution |
| `showPathInfo`, `showBesKey` | `ShowPathInfoCommand`, `ShowBesKeyCommand` | | yes | execution |
| `showCatalog`, `showNode`, `buildSiteMap` | from `BESXMLDapCommandModule` | | yes | execution |

`BESXMLGetCommand` can hand a request to a sub-command registered as `get.<type>`. No
such command is registered anywhere, so this path is never used. Note that
`~BESXMLGetCommand` does not delete `_sub_cmd`.

### A typical OLFS request

```xml
<request reqID="..." reqUUID="...">
  <setContext name="xdap_accept">3.2</setContext>      <!-- applied while parsing -->
  <setContext name="bes_timeout">300</setContext>       <!-- applied while parsing; read before the timer starts -->
  <setContainer name="c" space="catalog">/data/x.nc</setContainer>   <!-- executes 1st -->
  <define name="d"><container name="c"><constraint>u</constraint></container></define> <!-- 2nd -->
  <get type="dods" definition="d"/>                      <!-- 3rd; the only one that returns a response -->
</request>
```

Containers flow like this:

1. `setContainer` stores a container in a `BESContainerStorage`.
2. In `define`, `prep_request()` calls `look_for()`, which returns a **duplicate** of
   the stored container. It then sets the CE on that duplicate.
   `BESDefineResponseHandler` moves the duplicates into a new `BESDefine`, which owns
   and later deletes them. It stores the `BESDefine` in `BESDefinitionStorage`,
   replacing any older definition with the same name.
3. In `get`, `prep_request()` copies the `BESDefine`'s container pointers into the
   command's DHI. They are borrowed, not owned.

## State That Lasts Across Requests

A `beslistener` child serves every request on one OLFS connection. These things are
process-wide and are not reset between requests:

| State | Where | Reset? |
|---|---|---|
| Contexts (`errors`, `bes_timeout`, `olfsLog`, `xml:base`, `transmit_protocol`, and module-specific ones) | `BESContextManager` singleton | **Never.** `unset_context()` exists but nothing in production calls it. |
| Containers | `BESContainerStorageList` | Only when replaced by name, or by `deleteContainer` |
| Definitions | `BESDefinitionStorageList` | Only when replaced by name, or by `deleteDefinition` |
| libxml2 generic error handler | libxml2 global | **Never restored.** It points at a dead stack frame (F4). |
| BES log request id | `BESLog` | Set during parsing. If parsing fails before the root element is read, the error is logged under the **previous** request's id. |
| PPT `extensions` map | `BESServerHandler::execute` | Never cleared (see the PPT deep dive) |

This works today because the OLFS sends the same full set of contexts with every
request. A request that leaves out a context gets the previous request's value. For
example, a missing `bes_timeout` gives the previous timeout, not the
`BES.TimeOutInSeconds` default. A missing `olfsLog` puts the previous request's OLFS
log fields into this request's log line. (Inferred.)

## Findings

### High

**F1. The one-response rule can be bypassed (Verified, live).**
[BESXMLInterface.cc:236-240](../xmlcommand/BESXMLInterface.cc#L236-L240)

```cpp
bool cmd_has_response = current_cmd->has_response();
if (has_response && cmd_has_response) throw ...;
has_response = cmd_has_response;          // should be: has_response = has_response || cmd_has_response
```

Probe `get das` → `setContainer` → `define` → `get dds` in one document. Result:
`besstandalone` wrote a full DAS and then a full DDS to one stream, with no error.
With the two `get`s next to each other, the request is correctly rejected with
"Commands with multiple responses not supported." Over PPT, the client would get two
responses joined in one chunked message. When the second command fails, the first
response has already been streamed, and the error document is appended to it. The
probe showed that too, with a DAS followed by a `<getDDS><BESError>` document. The fix
is one line. The OLFS never sends such a document, so the risk is to other clients and
to correctness.

**F2. Contexts and other state persist between requests on a connection (Verified,
trace).** See the table above. Nothing in this code isolates one request from
the next. A long-lived OLFS connection that sends a context once, such as a debugging
`bes_timeout=0`, keeps it for every later request on that connection.
Request-scoped contexts would fix it: clear them in `execute_request()` or at the
start of `build_data_request_plan()`. That changes behavior for any client that
relies on contexts persisting, so check the OLFS first.

### Medium

**F3. Logging and cleanup after an error act on the wrong DHI (Verified, live, for
the log).**

- `finish()` deletes and nulls `error_info` *before* it calls `log_status()`. As a
  result, `log_status()` never reports `failed`. The probe was `get` on a missing
  definition with `BES.LogVerbose=yes`. The log said
  `standalone [get das for nosuchdef;] completed` right after the `ERROR!` line.
- In verbose mode, `log_status()` leaves `d_dhi_ptr` on the *last* command. Then
  `end_request()` releases only that command's containers. If a command that is not
  last fails after it adds containers, those containers are never released, and any
  uncompress-cache lock they hold stays held. In non-verbose mode `d_dhi_ptr` stays on
  the failed command, so the two modes behave differently. (Inferred. In practice
  `get` is last.)
- `error_info` is set on whichever DHI `d_dhi_ptr` points at when the exception is
  caught. That can be the base DHI or a command's DHI. Because `make_copy()` shallow
  copies `error_info` and `response_handler`, this is safe only because both are null
  when the commands are built. See besinfo D5.

**F4. libxml2 is left with a handler that points into a dead stack frame
(Inferred).** [BESXMLInterface.cc:96-97](../xmlcommand/BESXMLInterface.cc#L96-L97)
installs `XMLErrorFunc` with `&parse_errors`, a local variable, and never restores the
default. After `build_data_request_plan()` returns, any libxml2 code in the same
process that reports through the generic error channel will `push_back` onto freed
stack memory. That includes modules, libdap's SAX parsers, and `BESXMLInfo`'s writer.
`xmlCleanupParser()` is not documented to reset the handler.
`DaemonCommandHandler.cc:377` does the same thing. Fix: call
`xmlSetGenericErrorFunc(nullptr, nullptr)` on every exit path, or use a
`static thread_local` vector. Separately, libxml2 documents `xmlCleanupParser()` as a
process-exit call that is unsafe while other threads use libxml2. Here it runs on
every request.

**F5. The `errors=xml` context has no effect (Verified, live).** This is already
reported as besinfo D3. `downcase()` actually calls `toupper`, so the context value is
compared as `"XML"` against `"xml"`. Probe: with `BES.Info.Type=txt` and
`<setContext name="errors">xml</setContext>`, the error came back as text.

### Low

**F6. The attribute check in `<define>` looks at the wrong map (Verified, live).**
[BESXMLDefineCommand.cc:290-293](../xmlcommand/BESXMLDefineCommand.cc#L290-L293) clears
`props` and reads the next child into `props`. It should use `child_props`. So
`child_props` keeps the *first* child's attributes. `<dap4constraint bogus="x">` is
rejected when it is the first child of `<container>`, and accepted when it follows
`<constraint/>`. Unknown child elements of `<container>` are silently ignored, but at
the `<define>` level they are rejected.

**F7. A `setContext` command leaks when its parse fails (Inferred).** In the special
case, `delete current_cmd` comes after `parse_request()`. If the parse throws, as it
does for a missing name or value (seen in the probe), the command object is never
deleted. In `define`'s `prep_request()`, the containers it has already duplicated are
leaked if a later container is not found. The DHI does not own them.

**F8. `find_command()` adds entries to the registry (Verified, trace).**
`BESXMLCommand::factory[cmd_str]` uses `operator[]`, so every unknown element name a
client sends adds a null entry to the static map. The map is per process, and the
OLFS is trusted, so the effect is small.

**F9. Dead or misleading code (Verified, trace).**
- `load_dynamic_config()` on every `setContainer` does nothing. It also runs during
  parsing, not execution, and with several `setContainer`s the last one would win.
- The `error_info` branch of `transmit_data()` never runs.
- `get.<type>` sub-commands are never registered.
- `XMLSetContextsCommand` logs `"set contexts for "` with an empty name list. The list
  is filled only when `USE_CONTEXTS_RESPONSE_HANDLER` is set.
- `BESXMLDefaultCommands::terminate()` does not remove `showError`, `showServiceDescriptions`,
  `deleteDefinitions`, `showPathInfo`, `showBesKey`, or the `show.besKey` handler.
  `BESXMLDapCommandModule::terminate()` does not remove `showNode`. This matters only
  at shutdown.
- The default request id is a C++ `prolog` string with spaces in it. That string is
  also what gets sent to downstream services.

**F10. `setContext` and `setContexts` act during parsing, everything else during
execution (Verified, trace).** This is deliberate, so that `bes_timeout` and `errors`
are set before the timer starts and before any error is formatted. It has a
consequence: if a later element fails to parse, the contexts are already set, and they
stay set (F2). The order inside a document is also misleading. A `setContext` placed
*after* a `get` still applies to that `get`.

## Test Coverage

- `xmlcommand/unit-tests/XmlInterfaceTest.cc` and `buildT`/`propsT` test parsing and
  the command factory. Nothing tests the multi-response rule with a command in
  between, `log_status()`, or state carried across requests.
- `xmlcommand/tests/` (autotest) covers `setContexts`, `showBesKey`, `showNode`, and
  `buildSiteMap`. Every test runs one request per `besstandalone` process, so state
  that persists across requests (F2) cannot show up there.
- Possible regression tests:
  1. A `.bescmd` with `get` / `setContainer` / `get` that should fail with the
     multiple-responses error (F1).
  2. A `define` with an attribute on a non-first child that should fail (F6).
  3. A unit test that runs two `BESXMLInterface`s in one process and checks that
     contexts do not carry over (F2, once a behavior is chosen).

## Suggested Order of Work

1. Fix F1 (one line) and add test 1.
2. Fix F4: reset the libxml2 generic error handler on every exit path, in both
   `BESXMLInterface` and `DaemonCommandHandler`.
3. Fix F3: move `log_status()` before `error_info` is deleted, and make
   `end_request()` release containers for every command in `d_xml_cmd_list`.
4. Fix F5 (besinfo D3) and F6. Both are small.
5. Decide on F2 together with the OLFS team. Request-scoped contexts are the right
   model, but they change the protocol's behavior.
6. Remove the dead code in F9, later, in a separate change.

## Validation Done

- Ran `besstandalone` live with six probe documents. They covered F1 (two variants),
  F3 (verbose log), F5, F6 (two variants), and F7 (the error path only, not the leak
  itself).
- I did not run `make check`, ASan, or valgrind. I did not test anything over PPT
  through `beslistener`. F2, F4, and the container-release part of F3 come from
  reading the code.
