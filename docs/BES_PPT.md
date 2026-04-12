# OPeNDAP PPT/TfTP Programmer's Manual

National Center for Atmospheric Research (NCAR)  
High Altitude Observatory (HAO)  
Boulder, CO.

This manual will guide you through using the OPeNDAP PPT/TfTP library. It contains information about the different macros needed to communicate between a standalone client and server.

Patrick West  
`pwest@hao.ucar.edu`

## Overview

The OPeNDAP back-end server uses a communication protocol called PPT (Point to Point Transport), or TfTP, involving handshaking between the client and server. The protocol tokens are defined as macros in `ppt/PPTProtocolNames.h`. For example, to use the client handshake token you would write:

```
PPT_CLIENT_TESTING_CONNECTION
```

## 1. Making a connection to the server

When a client wishes to communicate with a server it first sends the message: `PPT_CLIENT_TESTING_CONNECTION`.

Upon receiving this initialization request the server responds with the message `PPT_SERVER_CONNECTION_OK`. If the server is configured for secure mode it instead responds with `PPT_SERVER_AUTHENTICATE` and proceeds with the secure-port handshake.

If the handshake token is not recognized, the server sends a descriptive error string and closes the socket; it does not send `PPT_PROTOCOL_UNDEFINED` in this path.

## 2. Sending messages to and receiving responses from the server

Once a connection has been established, the client sends requests to and receives responses from the server using a chunked framing protocol implemented in `ppt/PPTConnection.cc`.

Each message is sent as one or more chunks, each prefixed by a 7-hex-digit length and a type byte:

- `x` indicates a chunk of extensions in the form `name[=value];...`.
- `d` indicates a chunk of data.

The message ends when a zero-length data chunk (`0000000d`) is sent. This is how the receiver knows the request or response is complete.

The token `PPT_COMPLETE_DATA_TRANSMISSION` exists as a macro, but its string value is `"PPT_COMPLETE_DATA_TRANSMITION"` and it is only used during the secure-port authentication exchange, not as the general message terminator.

### Example message flow (non-secure)

This illustrates a simple request/response exchange. Each chunk is sent over the socket exactly as shown.

Client -> Server (handshake):

```
PPT_CLIENT_TESTING_CONNECTION
```

Server -> Client (handshake response):

```
PPT_SERVER_CONNECTION_OK
```

Client -> Server (one request payload chunk + end marker):

```
000000Cd<request-bytes>
0000000d
```

Server -> Client (one response payload chunk + end marker):

```
0000012d<response-bytes>
0000000d
```

Notes:

- The 7 hex digits are the byte length of the following payload.
- `d` marks data chunks. Extensions, when present, use `x` chunks before the data.
- A zero-length data chunk (`0000000d`) terminates the message.

Example with extensions (client -> server):

```
0000019xstatus=PPT_EXIT_NOW;trace=on;
0000000d
```

Notes:

- The extensions chunk length (`0000019`) is the hex length of `status=PPT_EXIT_NOW;trace=on;`.
- Extensions are `name[=value];` pairs. Empty values are allowed but `=` is omitted.
- In practice, extensions are most commonly used for the `status` field.

### Protocol tokens (glossary)

- `PPT_CLIENT_TESTING_CONNECTION`: client handshake token.
- `PPT_SERVER_CONNECTION_OK`: server handshake response when not using secure mode.
- `PPT_SERVER_AUTHENTICATE`: server handshake response when secure mode is enabled.
- `PPT_CLIENT_REQUEST_AUTHPORT`: client request for secure auth port (secure mode only).
- `PPT_PROTOCOL_UNDEFINED`: defined token; not currently used in the handshake failure path.
- `PPT_COMPLETE_DATA_TRANSMISSION`: macro whose string value is `"PPT_COMPLETE_DATA_TRANSMITION"`; used only in the secure auth-port exchange.
- `PPT_EXIT_NOW`: status token used in extensions to signal shutdown; EOF is also treated as exit.

### Sequence chart (non-secure)

```
Client                                               Server
  |                                                    |
  |  PPT_CLIENT_TESTING_CONNECTION                     |
  |--------------------------------------------------->|
  |                                                    |
  |  PPT_SERVER_CONNECTION_OK                          |
  |<---------------------------------------------------|
  |                                                    |
  |  000000Cd<request-bytes>                           |
  |  0000000d                                          |
  |--------------------------------------------------->|
  |                                                    |
  |  0000012d<response-bytes>                          |
  |  0000000d                                          |
  |<---------------------------------------------------|
  |                                                    |
  |  0000019xstatus=PPT_EXIT_NOW;                      |
  |  0000000d                                          |
  |--------------------------------------------------->|
  |                                                    |
  |  (socket closed)                                   |
  |                                                    |
```

### Sequence chart (secure mode, not currently used)

This reflects the optional secure-auth port flow. In the current codebase the OpenSSL path is disabled, so treat this as reference only.

Reference: the secure branches in `ppt/PPTClient.cc` and `ppt/PPTServer.cc` are guarded by `#if defined HAVE_OPENSSL && defined NOTTHERE`, so they are compiled out in current builds.

```
Client                                               Server
  |                                                    |
  |  PPT_CLIENT_TESTING_CONNECTION                     |
  |--------------------------------------------------->|
  |                                                    |
  |  PPT_SERVER_AUTHENTICATE                           |
  |<---------------------------------------------------|
  |                                                    |
  |  PPT_CLIENT_REQUEST_AUTHPORT                       |
  |--------------------------------------------------->|
  |                                                    |
  |  <secure-port-number>PPT_COMPLETE_DATA_TRANSMISSION|
  |<---------------------------------------------------|
  |                                                    |
  |  (SSL auth on secure port)                         |
  |                                                    |
```

## 3. Ending a connection

When the client is ready to end the connection or the server is exiting, it signals `PPT_EXIT_NOW` via an extension (`status=PPT_EXIT_NOW`) and then closes the connection. An EOF while reading is also treated as an exit condition, and the receiver should clean up the connection.
