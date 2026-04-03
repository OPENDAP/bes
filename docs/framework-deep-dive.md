# BES Framework Deep Dive

This document focuses on the BES framework itself: the code that boots the server, accepts a request, parses BES XML, builds an execution plan, dispatches work to handlers, and transmits a response. It intentionally stops short of the format-specific modules in `modules/`.

## What the framework is

At framework level, BES is a plugin host plus a request execution engine.

- The plugin host loads modules named in `BES.modules`, initializes them, and lets them register request handlers, response handlers, catalogs, container stores, services, and transmitters.
- The execution engine accepts a BES XML request, converts it into one or more command objects, builds a `BESDataHandlerInterface` for each command, executes the bound response handler, and streams the result back to the caller.

The main framework directories involved in that flow are:

- `dispatch/`: core registries, request state, default framework services, container and definition storage, error handling, logging, and plugin loading.
- `server/`: the daemon/listener application and the per-connection execution loop.
- `ppt/`: the socket protocol layer used between OLFS and BES.
- `xmlcommand/`: XML parsing and the command builders that turn `<request>` documents into executable plan objects.
- `http/`: shared HTTP client infrastructure used by framework and modules for remote resource access.
- `standalone/`: an in-process execution path used for testing and command-line work without PPT/network transport.

## Startup and bootstrap

### 1. Process startup

`server/ServerApp.cc` is the main listener process.

At startup it:

- resolves the BES config file location,
- reads listener configuration such as TCP port, bind address, Unix socket, and secure mode,
- initializes the default framework module,
- initializes the default XML command set,
- loads all configured shared-object modules through `BESModuleApp`,
- creates listening sockets,
- constructs a `PPTServer`,
- enters the signal-aware accept loop.

`dispatch/BESModuleApp.cc` is the generic module loader used by both `server` and `standalone`.

Its job is:

- read `BES.modules`,
- map module names to shared libraries via `BES.module.<name>`,
- load each module through the plugin factory,
- call each module's `initialize(modname)`,
- later call `terminate(modname)` in reverse order.

### 2. Default framework registration

Before optional modules load, `dispatch/BESDefaultModule.cc` seeds the framework with the base services needed to process any request.

It registers:

- default response handlers such as help, version, status, service, stream, container, definition, catalog, and context operations,
- the basic transmitter,
- info builders for text, HTML, and XML error/info output,
- default definition storage backends,
- the core `bes` debug context.

Separately, `xmlcommand/BESXMLDefaultCommands.cc` registers the XML commands that the framework itself understands, including:

- `show`,
- `get`,
- `setContext` and `setContexts`,
- `setContainer`,
- `define`,
- container and definition deletion commands,
- path-info and BES-key inspection helpers.

Together these two initializers create the base grammar and base runtime needed before data-format modules are loaded.

## Connection handling

### 3. PPT transport

`ppt/PPTServer.cc` owns the protocol-level handshake on top of the socket listener.

Its responsibilities are:

- accept a socket,
- validate that the client is speaking PPT,
- optionally negotiate secure mode,
- wrap the socket as a connection,
- hand the connection to the server-side request handler.

The framework keeps PPT separate from request execution. The transport knows how to exchange chunks and extensions; it does not know how to interpret BES XML.

### 4. Listener and worker model

`server/BESServerHandler.cc` is the bridge from PPT connection to request execution.

It supports two process models controlled by `BES.ProcessManagerMethod`:

- `single`: one process handles requests directly.
- `multiple`: the listener forks a child per client connection.

Inside the worker loop, BES:

- receives PPT chunks until a full command arrives,
- handles protocol exit messages,
- wraps the connection output stream in `PPTStreamBuf`,
- creates a `BESXMLInterface`,
- calls `execute_request(from)`,
- calls `finish(status)`,
- emits error extensions if needed,
- either keeps serving the same client or exits on fatal conditions.

This separation matters: the server layer manages sockets, process lifetime, and protocol status; the interface layer manages request semantics.

## The execution model

### 5. `BESInterface` as the request execution shell

`dispatch/BESInterface.cc` is the generic execution shell used by `BESXMLInterface`.

It provides the common lifecycle:

1. attach the output stream to the request state,
2. set request metadata like source and PID,
3. select the basic transmitter,
4. build the request plan,
5. set request timeout,
6. execute the plan,
7. clear timeout,
8. convert exceptions into error responses,
9. finish logging and cleanup.

Its central request-state object is `dispatch/BESDataHandlerInterface.h`.

That object carries:

- the active `BESResponseHandler`,
- the container list and current container cursor,
- action and action-name strings,
- a generic string map for request metadata,
- the output stream,
- the response object reachable through the response handler,
- any generated `BESInfo` error object.

The important design choice is that BES passes around one mutable execution context instead of large numbers of specialized parameter types.

### 6. Building the plan from XML

`xmlcommand/BESXMLInterface.cc` is where the framework turns BES XML into executable work.

`build_data_request_plan()`:

- parses the request XML with libxml2,
- validates that the root element is `<request>`,
- extracts request identifiers and logging metadata,
- iterates over each child command element,
- looks up a command builder in the XML command registry,
- creates a `BESXMLCommand`,
- asks the command to parse its node into a command-specific `BESDataHandlerInterface`,
- validates any requested transmitter,
- appends the command to an ordered command list.

Two details are especially important:

- BES supports a request document with multiple commands, but only one command in the request may produce a response.
- `setContext` is executed immediately while the plan is being built because later commands may depend on those context values.

In effect, `xmlcommand/` is the planning stage of the framework.

### 7. Executing the plan

`BESXMLInterface::execute_data_request_plan()` walks the planned command list.

For each command it:

- runs `prep_request()`,
- swaps `d_dhi_ptr` to that command's `BESDataHandlerInterface`,
- logs the command,
- calls the command's bound response handler `execute()`,
- checks timeout status,
- calls `transmit_data()`.

The response handler is where a command becomes real work. For a `get` command, that usually means constructing a response object and asking the request-handler registry to populate it from one or more containers.

## Registries and dispatch points

The framework is registry-driven. Modules do not patch a switch statement; they populate shared registries.

### Request handlers

`dispatch/BESRequestHandlerList.cc` maps a container type or module name to a `BESRequestHandler`.

Its key behaviors are:

- `add_handler()` and `remove_handler()` for module registration,
- `execute_each()` to iterate all containers in the current request,
- `execute_current()` to resolve the current container type, find the matching request handler, and invoke the method for the current action,
- `execute_all()` for service-wide operations like help/version.

The most important call is `execute_current()`:

- it ensures the container is accessible,
- determines container type,
- finds the matching request handler,
- finds the method for the requested action,
- invokes that method to populate the response object.

That is the framework's main data dispatch pivot.

### Response handlers

`BESResponseHandlerList` is the second dispatch layer. XML commands bind to response-handler builders, and those response handlers know how to execute a command using the current `BESDataHandlerInterface`.

Examples include:

- container management,
- definition management,
- show/version/help/status,
- DAP response builders.

### Transmitters

`dispatch/BESReturnManager.cc` maps `return as ...` names to `BESTransmitter` instances.

This keeps response construction separate from response serialization.

Examples:

- the basic transmitter for standard BES output,
- DAP transmitters,
- file-out transmitters such as NetCDF, JSON, CoverageJSON, GeoTIFF, and DMR++.

When `BESXMLInterface::transmit_data()` sees `RETURN_CMD`, it resolves the named transmitter and lets the response handler transmit through it.

### Other registries

The rest of `dispatch/` provides shared framework state:

- `BESCatalogList`: named catalogs and browsing backends.
- `BESContainerStorageList`: named container stores and persistence layers.
- `BESDefinitionStorageList`: stores named definitions.
- `BESInfoList`: builders for human-readable or machine-readable info/error output.
- `BESServiceRegistry`: advertised services and formats.
- `BESContextManager`: request-scoped or command-scoped key/value context.

These registries are why modules can integrate cleanly without touching the server loop.

## Timeout, error, and cleanup behavior

### Timeouts

`BESInterface` sets a per-request timeout from either:

- the `bes_timeout` context, or
- `BES.TimeOutInSeconds`.

It starts `RequestServiceTimer` before execution and checks for timeout expiration before transmission. Some transmitters may disable the timeout once streaming has started.

### Errors

Any `BESError`, C++ exception, or allocation failure is converted into a framework error response by `BESInterface::handleException()`.

That path:

- logs the error,
- chooses XML or non-XML error formatting depending on context,
- embeds server administrator contact information,
- stores the error response in the current `BESDataHandlerInterface`.

Then `finish()` prints/transmits the error info using the already-selected output stream.

### Cleanup

At the end of a request:

- `BESXMLInterface::clean()` deletes command objects and lets each command clean its DHI,
- `BESInterface::end_request()` releases any containers used by the request,
- the server loop decides whether to continue or terminate the worker based on status.

## `standalone/` and why it exists

`standalone/StandAloneClient.cc` uses the same `BESXMLInterface` path as the daemon, but skips PPT and sockets entirely.

That makes it useful for:

- regression tests,
- local debugging,
- generating baseline outputs,
- validating XML command behavior without the daemon stack.

Architecturally, `standalone/` proves that the execution engine is decoupled from the network protocol. The same planning and dispatch code can run in-process.

## `http/` in the framework

`http/` is not the request planner, but it is framework infrastructure because many modules depend on it for remote access.

It provides:

- remote resource access abstractions,
- URL handling,
- allowed-host checks,
- proxy configuration,
- credential management,
- HTTP and curl utilities,
- AWS SigV4 support.

This code sits below modules such as gateway, DMR++, S3-related access, and remote catalog implementations.

## End-to-end request path

The framework flow is:

```text
ServerApp
  -> PPTServer
  -> BESServerHandler
  -> BESXMLInterface
  -> build_data_request_plan()
  -> response_handler->execute()
  -> BESRequestHandlerList / other registries
  -> transmitter
  -> output stream / PPT chunks
```

For an ordinary `get` request, that usually expands to:

```text
<request> XML
  -> XML command object
  -> BESDataHandlerInterface
  -> response handler chosen by action
  -> request handler chosen by container type
  -> response object populated
  -> transmitter chosen by return format
  -> bytes written to client
```

## Architectural takeaway

The BES framework is less a monolithic server than a layered runtime:

- `server/` and `ppt/` handle process and transport concerns.
- `xmlcommand/` handles parsing and planning.
- `dispatch/` owns the registries and the shared execution contract.
- `http/` provides reusable remote-access primitives.
- `standalone/` reuses the same engine without the daemon protocol.

That layering is what lets BES mix and match:

- different request commands,
- different data readers,
- different output formats,
- different container sources,
- different transport environments.

The framework's core abstraction is: build a `BESDataHandlerInterface`, resolve handlers from registries, and let plugins do the format-specific work.
