# PPT and Server Deep Dive: How the BES Accepts, Runs, and Stops Work

## Scope

This covers `ppt/` (sockets, the PPT wire protocol, and the stream buffer that sends
responses) and `server/` (`besdaemon`, the master and child `beslistener` processes,
the daemon admin command interface, and `besctl`). It also covers the parts of
`dispatch/` that these two directories call directly (`BESApp`, `BESInterface`
timeouts).

Findings are marked:

- **Verified**: checked with a live test on macOS (arm64, Darwin 25.5) using the
  in-tree `--enable-developer` build, or with a direct code trace.
- **Inferred**: follows from reading the code. Not tested. This includes every claim
  about Linux behavior.

The live tests ran a scratch `beslistener` and `besdaemon` on port 19022, with a
minimal `bes.conf` in the session scratchpad and Python socket clients. Nothing in the
repository was changed, and the installed BES that was running at the time was not
touched.

## Summary

The architecture is simple and has held up for a long time. The daemon supervises a
master listener. The master forks one child per client connection. Each child runs
requests over a chunked protocol until the client says it is done.

The weak point is the master listener. It does the PPT handshake itself, before it
forks, using a blocking read with no timeout. Any error in that step ends the master
process. That leads to the two most serious findings:

1. **One TCP connection that connects and then resets takes down the whole BES**
   (Verified). The master's `read()` fails with `ECONNRESET` and throws. The exception
   leaves `ServerApp::run()`, so the master exits with status 1. `besdaemon` reads that
   status as "server cannot start" and exits too. Some TCP health checks and port
   scanners close connections this way.
2. **One TCP connection that connects and sends nothing stops the BES from serving
   anyone else** (Verified). While the master waits for that handshake, it cannot
   accept, fork, reap children, or act on SIGTERM. In the test, a second client timed
   out, and SIGTERM had no effect until the idle client went away.

Other significant findings:

3. **Child listeners ignore SIGTERM and SIGHUP, and they keep the listening socket
   open** (Verified). After a stop or restart, children that have open OLFS connections
   keep running with the old configuration. While they run, a new master cannot bind
   the port (`Address already in use`). On a SIGHUP restart, `besdaemon` then gives up
   and exits.
4. **A config with only a Unix socket hangs `besdaemon` at startup** (Verified). The
   master writes its "running" status to the daemon only when it opens a TCP port. The
   daemon blocks forever in `read()` and ignores SIGTERM.
5. **`beslistener -v`, `-h`, and the "no port configured" path crash with SIGSEGV**
   (Verified for `-v`, including the installed build). `BESApp::_theApplication` is
   never assigned.
6. **Response writes ignore the return value of `write()`** (Inferred). A short write
   would corrupt the chunk stream without any error being raised.

The existing protocol document, [`BES_PPT.md`](BES_PPT.md), shows the wrong strings
for the handshake tokens. See [Corrections to BES_PPT.md](#corrections-to-bes_pptmd).

## Process Architecture

```
            besctl / hyraxctl (shell)
                   │  starts; stops with kill -TERM <daemon pid>
                   ▼
            besdaemon  (daemon.cc)            optional admin port: BES.DaemonPort
                   │  fork + execvp, pipe on fd 1   (DaemonCommandHandler, "HAI")
                   ▼
     master beslistener  (ServerApp.cc)       BES.ServerPort / BES.ServerUnixSocket
       setsid(); group leader                  SocketListener + PPTServer
                   │  accept → handshake → fork (BESServerHandler::handle)
        ┌──────────┼──────────┐
        ▼          ▼          ▼
     child      child      child     one per OLFS connection; loops over requests;
   beslistener beslistener  ...      exits on PPT_EXIT_NOW, EOF, fatal error, SIGPIPE
```

### Binaries and libraries

| Artifact | Sources | Role |
|---|---|---|
| `libbes_ppt.la` | `ppt/*.cc` | Sockets, PPT framing, `PPTServer`, `PPTClient`, `PPTStreamBuf` |
| `besdaemon` | `daemon.cc`, `DaemonCommandHandler.cc`, `BESXMLWriter.cc`, `setgroups.c`, `BESServerUtils.cc` | Root-started supervisor. Drops privileges, starts and restarts the master, and runs the optional admin interface |
| `beslistener` | `ServerApp.cc`, `BESServerHandler.cc`, `BESServerUtils.cc` | Master listener, and (after `fork()`) the child listeners |
| `besctl`, `hyraxctl` | `besctl.in`, `hyraxctl.in` | Shell wrappers. `stop` sends SIGTERM to the daemon PID from `bes.pid` |
| `bescmdln` | `cmdln/` | Test and admin client. Uses `PPTClient` |

`server/test/` (`TestModule`, `TestCommand`, and so on) is not in any `SUBDIRS` and is
not built.

### Class structure (`ppt/`)

```
BESObj
 ├─ Socket (abstract; fd, connected/listening flags, peer ip/port)
 │   ├─ TcpSocket    (connect/listen, SO_RCVBUF/SO_SNDBUF from bes.conf, optional libwrap)
 │   └─ UnixSocket   (connect binds a temp client path in cwd; listen unlinks then binds)
 ├─ SocketListener   (map fd → Socket*, select() with 120 s timeout, accept())
 ├─ ServerHandler (abstract: handle(Connection*))
 │   ├─ BESServerHandler      (server/; fork per connection, request loop)
 │   └─ DaemonCommandHandler  (server/; one admin command per connection)
 └─ Connection (abstract; Socket* _mySock, ostream* _out)
     └─ PPTConnection        (chunk framing: send/sendExtensions/sendExit/receive)
         ├─ PPTServer        (accept + handshake + hand off to ServerHandler)
         └─ PPTClient        (connect + handshake; used by bescmdln)

std::streambuf
 └─ PPTStreamBuf             (response path: buffer → "%07xd" + data chunks; finish() → "0000000d")
```

`PPTMarkFinder` and `SocketUtilities::ltoa` are leftovers. `PPTMarkFinder` has no
callers. `SocketUtilities` builds the temporary client path for `UnixSocket::connect`.

### Exit codes and status values

| Value | Name | Who sets it | How it is read |
|---|---|---|---|
| 0 | `SERVER_EXIT_NORMAL_SHUTDOWN` | master on SIGTERM | daemon: stop |
| 1 | `SERVER_EXIT_FATAL_CANNOT_START` | master on **any** `BESError` from `run()`, including runtime socket errors | daemon: "server cannot start", then exits |
| 2 | `SERVER_EXIT_ABNORMAL_TERMINATION` | child on `BES_INTERNAL_FATAL_ERROR` (and on `BES_INTERNAL_ERROR` if `BES.ExitOnInternalError=true`) | master logs it |
| 3 | `SERVER_EXIT_RESTART` | master on SIGHUP | daemon restarts the master |
| 4 | `BESLISTENER_RUNNING` | master writes this int to fd 1 (the pipe to the daemon) after the TCP listen | daemon startup handshake |
| 6 | `CHILD_SUBPROCESS_READY` | child on `PPT_EXIT_NOW` or EOF | master logs it |

Children whose request loop throws (for example, `ECONNRESET` while reading) unwind
into `ServerApp::run()`'s `catch` in the child process. They exit with 1, so the master
log shows "exited with status: 1" for what is an ordinary client disconnect
(Verified: seen in the test log).

## Startup Sequence

1. `besctl start` runs `besdaemon` with the options the user passed.
2. `besdaemon` (`daemon.cc:main`) checks that it runs as root (skipped in developer
   builds). It checks the options with `BESScrub`, finds `bin/beslistener`, refuses to
   start if `bes.pid` exists, forks to the background (unless `-n`), and writes
   `bes.pid`. As root, it then calls `setgid(BES.Group)`, drops supplementary groups,
   and calls `setuid(BES.User)`. It loads modules only to learn the debug contexts.
3. `start_master_beslistener()` creates a pipe, forks, `dup2`s the write end onto fd 1,
   and `execvp`s `beslistener` with the daemon's options plus a `-d` built from the
   current debug settings. The daemon then blocks in `read()` on the pipe
   (`daemon.cc:382`) until it gets one `int`.
4. `ServerApp::initialize` reads the port, IP, Unix socket, and secure settings,
   initializes `BESDefaultModule`, `BESXMLDefaultCommands`, and every module in
   `BES.modules`, and then calls `setsid()` (`ServerApp.cc:445`). The master becomes a
   process-group leader, so `killpg()` can reach every child.
5. `ServerApp::run` listens on TCP and writes `BESLISTENER_RUNNING` to fd 1
   (`ServerApp.cc:477`). It then listens on the Unix socket, builds `PPTServer`, and
   registers handlers for SIGCHLD, SIGPIPE, SIGTERM, and SIGHUP with `SA_RESTART`.
6. The daemon gets status 4, records the master PID, and either runs the admin command
   loop (`BES.DaemonPort` or `BES.DaemonUnixSocket` set) or waits in `pause()` for
   signals.

## The Master Listener Loop

`ServerApp::run()` (`ServerApp.cc:~505-545`):

```
loop:
    block SIGCHLD/HUP/TERM/PIPE
    if any flag: reap children with wait4(0, WNOHANG)      // decrement child count
    if sighup:  return 3
    if sigterm: return 0
    unblock signals
    d_ppt_server->initConnection()
        SocketListener::accept()        select(listen fds, 120 s); returns null on EINTR/timeout
        Socket::allowConnection()       libwrap (only with --with-libwrap)
        PPTServer::welcomeClient()      BLOCKING read(64) of handshake, in the master
        BESServerHandler::handle()      fork(); child → execute(); parent returns
        _mySock->close()                parent closes its copy of the accepted fd
```

Signals are handled only between calls to `initConnection()`. `select()` returns early
on EINTR. The handshake `read()` does not, because the handlers use `SA_RESTART`.

Every `Socket*` that `accept()` returns is allocated with `new`. `PPTServer` overwrites
`_mySock` each time and never deletes the old one, so the master leaks one small object
per connection (Inferred).

## The Child Listener Request Loop

`BESServerHandler::execute()` (`BESServerHandler.cc:~122-251`) runs in the child after
`fork()`:

1. It wraps the accepted fd in a `PPTStreamBuf` with a buffer of `SO_SNDBUF − 15`
   bytes (65535 − 15 for Unix sockets). It then makes an `ostream` over that buffer.
2. It calls `connection->receive(extensions, &ss)` until the zero-length data chunk
   arrives. Extension chunks go into `extensions`, and data chunks are appended to
   `ss`.
3. If `extensions["status"] == "PPT_EXIT_NOW"`, the child closes the socket and calls
   `exit(6)`. EOF is turned into the same status by `PPTConnection::receive`.
4. Otherwise it builds `BESXMLInterface(cmd, &ostream)` and calls
   `execute_request()`.
   - On success it calls `finish()` and then `fds.finish()`. The response goes out as
     data chunks, then `0000000d`.
   - On error it first writes an extension chunk `status=error` (with `exit=true` for
     fatal errors) **directly to the socket**. It then calls `finish()` (the error
     document) and `fds.finish()`. It exits on fatal errors, and on internal errors if
     `BES.ExitOnInternalError` is set.

Notes:

- The `extensions` map is declared outside the loop and never cleared (`:132`). An
  extension the client sends once stays set for every later request on that
  connection. Today this matters only if a client sends `status` with a value other
  than the exit token.
- The error extension skips the `PPTStreamBuf` buffer. If response data was already
  buffered when the error happened, the order on the wire is: extension chunk, then
  the buffered data, then the error document. If data was already flushed, the client
  gets data, then the extension, then the error document. See
  [`besinfo-deep-dive.md`](besinfo-deep-dive.md) for the error-document side of this.
- No size limit applies to an incoming request. The OLFS is trusted, so this is low
  risk.

## PPT Wire Protocol

### Handshake (raw bytes, no framing)

| Direction | Macro | **Bytes on the wire** |
|---|---|---|
| client → server | `PPT_CLIENT_TESTING_CONNECTION` | `PPTCLIENT_TESTING_CONNECTION` |
| server → client | `PPT_SERVER_CONNECTION_OK` | `PPTSERVER_CONNECTION_OK` |
| server → client (secure, compiled out) | `PPT_SERVER_AUTHENTICATE` | `PPTSERVER_AUTHENTICATE` |
| client → server (secure, compiled out) | `PPT_CLIENT_REQUEST_AUTHPORT` | `PPTCLIENT_REQUEST_AUTHPORT` |
| server → client on bad token | none | `PPT cannot negotiate, client started the connection with <bytes>`, then close |

Verified: a probe sent `PPTCLIENT_TESTING_CONNECTION` and got
`PPTSERVER_CONNECTION_OK`.

Each side does a single `read()` of up to 64 bytes and compares it to the expected
string exactly (`PPTServer.cc:187-199`). This works only because the OLFS waits for the
reply before it sends anything else. If the token is split across TCP segments, or the
first request chunk arrives in the same `read()`, the handshake fails (Inferred).

### Framing (after the handshake)

```
message     = *ext-chunk *data-chunk last-chunk
ext-chunk   = 7HEXDIG "x" 1*( name [ "=" value ] ";" )
data-chunk  = 7HEXDIG "d" <length bytes>
last-chunk  = "0000000d"
```

- The length is lowercase hex, zero-padded to 7 digits, so the most a chunk can carry
  is 0xFFFFFFF bytes (about 268 MB). `setw(7)` sets only a minimum width, so a
  `BES.SockSendSize` above that limit would produce an 8-digit header and break
  framing (Inferred. The default is 65535).
- The parser does not check the hex digits. A header such as `zzzzzzzd` parses as
  length 0, which is read as the end of the message (`PPTConnection.cc:295`). Garbage
  is accepted silently.
- Extensions: a value may contain `=` but not `;`. An empty extension chunk throws.
- `PPTConnection::receive(ostream&, len)` calls itself once per `read()`
  (`PPTConnection.cc:364`). A large chunk delivered in small reads gives deep recursion
  in unoptimized builds (Inferred).
- `sendExit()` sends `…xstatus=PPT_EXIT_NOW;` + `0000000d` + `0000000d`. The second
  terminator is extra. Receivers exit after the first one, so it causes no harm.

### Response path: `PPTStreamBuf`

`sync()` writes the 8-byte header and the payload as two separate `write()` calls. It
does not check either return value (`PPTStreamBuf.cc:76-86`). `finish()` sends
`sync()` and then `0000000d`.

- A short write (a blocking socket interrupted by a signal partway through a large
  write) or a failed write (`-1`, which is also added to `count`) is not detected. The
  peer then gets a header whose length does not match the bytes that follow (Inferred.
  The BES cancels its SIGALRM timeout before it sends, so this is unlikely today, but
  nothing guards against it).
- `Socket::send()` (used for extensions and handshake) also does a single `write()` and
  treats only `-1` as an error. It copies the whole buffer through `substr()` first
  (`Socket.cc:92`).
- `TCP_NODELAY` is never set anywhere in the repository. With a small header write, a
  large payload write, and a small trailer write, Nagle's algorithm combined with
  delayed ACK can add latency at the end of each response (Inferred. Not measured).

## Daemon Admin Interface ("HAI")

This interface is enabled only when `BES.DaemonPort` or `BES.DaemonUnixSocket` is set.
Both are commented out in the shipped `bes.conf.in`. `DaemonCommandHandler::handle`
reads one PPT message per connection. The message is an XML `<BesAdminCmd>` with one
or more of:

| Command | Effect |
|---|---|
| `StopNow` | `killpg(SIGTERM)` and reap the master |
| `Start` | start a new master |
| `Exit` | stop all, then `exit(0)` the daemon |
| `GetConfig` | return the contents of `bes.conf` and every `BES.Include` file |
| `SetConfig module="name"` | overwrite that file (the first write backs it up to `name.<pid>`) |
| `TailLog lines="N"` | return the last N lines of `BES.LogName` |
| `GetLogContexts` / `SetLogContext` | list or change the BESDebug contexts passed to the next master |

The interface has **no authentication**. Anyone who can reach the port can stop the
server, read the config, and rewrite any config file the daemon knows about. That
includes `BES.modules` and the module library paths, which run with the BES user's
privileges at the next start. The daemon source notes this as a concern for sites
without a firewall. Leaving it off by default is the right choice. Any site that turns
it on must restrict access to it with a firewall.

Smaller defects:

- `SetConfig` with an unknown `module` uses `d_pathnames[module]`, which adds an empty
  entry to the map. It then writes `.tmp` in the daemon's working directory and fails.
  After that, every `GetConfig` fails on the empty path (`DaemonCommandHandler.cc:569`,
  Inferred).
- `TailLog` with a negative `lines` value is converted to a very large unsigned number
  (`:593`, Inferred).
- `daemon.cc` defines `volatile int master_beslistener_status`, but
  `DaemonCommandHandler.cc:69` declares it `extern int`. The types do not match, which
  is undefined behavior in principle (Inferred).

## Shutdown and Restart

- **besctl stop**: sends `kill -TERM` to the daemon. The daemon calls
  `killpg(master_pgid, SIGTERM)`, waits for its own child (only the master), removes
  `bes.pid`, and exits.
- **SIGHUP to the daemon**: sends `killpg(SIGHUP)`, waits for the master to exit with
  status 3, and starts a new master. If the new master fails to start, the daemon sends
  `killpg(SIGTERM)` and calls `exit(1)`.
- **Children**: they inherit the master's handlers. Those handlers only set flags, and
  nothing in `BESServerHandler::execute()` reads the flags. SIGTERM and SIGHUP
  therefore have **no effect** on a child. A child exits only on `PPT_EXIT_NOW`, EOF,
  a fatal error, SIGPIPE (its handler restores the default action and re-raises), or
  SIGKILL.

The children also inherit, and never close, the master's listening sockets and fd 1
(the pipe to the daemon). Verified with `lsof` on a child: fd 3 was
`TCP *:19022 (LISTEN)`, and fd 1 was the master's stdout.

Consequences (Verified on macOS):

1. After SIGTERM to the process group, the master exited, but the child kept running.
   It was re-parented to PID 1, still held `*:19022 (LISTEN)`, and finished its
   client's request normally.
2. A new master started while that child was alive failed with
   `ERROR: Failed to bind TCP socket: 19022: Address already in use`.

In production the OLFS keeps BES connections open, so after `besctl restart` or a
SIGHUP, old children can keep serving with the old configuration. They can also keep
the new master from binding the port. On the SIGHUP path, that failure makes
`besdaemon` exit. Linux also refuses `bind()` with `SO_REUSEADDR` while another socket
is listening on the same address and port (Inferred, not tested on Linux). Also, while
an orphaned child holds the listening socket, new TCP connections finish the kernel
handshake but are never accepted, so clients hang instead of being refused (Inferred).

`besctl restart` runs `if test !$BES_RUNNING` (`besctl.in:110`). That test is always
true, so `start` runs even when `stop` failed.

## Configuration Keys Used by These Directories

| Key | Default in `bes.conf.in` | Used by |
|---|---|---|
| `BES.ServerPort` | 10022 | master. Command-line `-p` overrides it |
| `BES.ServerIP` | unset (INADDR_ANY) | master bind address. `-H` overrides it |
| `BES.ServerUnixSocket` | unset | master. `-u` overrides it |
| `BES.ServerSecure`, `BES.Server*File`, `BES.ServerSecurePort` | unset | secure mode. The OpenSSL path is compiled out (`#if … && defined NOTTHERE`), so `-s` makes every connection throw |
| `BES.ProcessManagerMethod` | `multiple` | `single` runs the request loop in the master (debug only; the master exits when the client leaves) |
| `BES.SetSockRecvSize` / `BES.SockRecvSize` | No / 65535 | `TcpSocket` |
| `BES.SetSockSendSize` / `BES.SockSendSize` | No / 65535 | `TcpSocket`. Also sets the PPT chunk size, through `getSendChunkSize()` |
| `BES.ExitOnInternalError` | false | child exits on `BES_INTERNAL_ERROR` |
| `BES.DaemonPort` / `BES.DaemonUnixSocket` | unset | HAI admin interface |
| `BES.User` / `BES.Group` | required when run as root | `besdaemon` drops privileges |
| `BES.LogName` | required | daemon passes it to the master as the `-d` sink |

`TcpSocket::listen()` refuses any port that has a name in `/etc/services`
(`getnameinfo`). Port 10022 is not named on common systems. A site that picks a port
that is named there gets a startup error.

## Findings

These are ordered by operational impact. The references point to the current tree.

### High

**H1. A client reset during the handshake kills the master, and then the daemon.**
Verified. `PPTServer::initConnection()` runs `welcomeClient()` in the master
(`PPTServer.cc:136-148`). `Socket::receive()` throws `BESInternalError` when `read()`
returns -1 with `ECONNRESET`. Nothing catches the exception below
`ServerApp::run()`'s `catch (BESError&)` (`ServerApp.cc:546`), which returns
`SERVER_EXIT_FATAL_CANNOT_START`. Test results: one Python client that connected, set
`SO_LINGER{1,0}`, and closed caused these log lines:
`Socket::receive: socket failure, reading on stream socket: Connection reset by peer`.
The next client got `Connection refused`. Under `besdaemon -n`, the daemon printed
`besdaemon: server cannot start, exited with status 1` and exited.
The same path is taken for any other exception in `initConnection()`: `accept()`
errors other than EINTR (`SocketListener.cc:148`, for example `EMFILE` or
`ECONNABORTED`), `fork()` failure, and `send()` failing on the "cannot negotiate"
reply.

**H2. An idle connection blocks the master and makes it ignore SIGTERM.**
Verified. The handshake `read()` (`PPTServer.cc:187`) blocks with no timeout, and
`SA_RESTART` makes it resume after signals. Test results: with one idle connection
open, a second client's handshake timed out after 5 s, and no child was forked.
SIGTERM sent to the master had no effect for as long as the idle client stayed
connected. The master exited only after that client closed. The comment at that line
explains why the earlier non-blocking read was dropped (tickets #823 and others). The
fix needs a bounded wait, such as `poll()` with a timeout or `SO_RCVTIMEO`, not a
non-blocking read. A simpler alternative is to move the handshake into the child after
`fork()`.

**H3. Children ignore SIGTERM and SIGHUP and hold the listening socket.**
Verified. See [Shutdown and Restart](#shutdown-and-restart). The minimal fix is in the
child branch of `BESServerHandler::handle()` (`BESServerHandler.cc:117`). It should
close the listening fds (or they should be opened with `FD_CLOEXEC` and closed after
the fork) and reset SIGTERM and SIGHUP to `SIG_DFL`, or decide on an explicit
graceful-drain policy. Either choice changes shutdown behavior that the OLFS sees, so
it needs a decision, not just a patch.

**H4. Unix-socket-only configuration hangs `besdaemon`.**
Verified. `ServerApp::run()` writes `BESLISTENER_RUNNING` only inside `if (d_port)`
(`ServerApp.cc:463-484`). The daemon's `read()` (`daemon.cc:382`) blocks while the
master keeps the write end open as fd 1. Test results: with
`BES.ServerUnixSocket` set and no `BES.ServerPort`, the daemon and the master both
stayed running. SIGTERM to the daemon had no effect. The daemon had to be killed with
SIGKILL. If the read returns 0 (EOF), `beslistener_start_status` is used without being
initialized (`daemon.cc:381`).

### Medium

**M1. `beslistener -v`, `-h`, and the "must specify a port" path crash (SIGSEGV).**
Verified for `-v` (exit 139) on both the in-tree and the installed binaries.
`BESApp::_theApplication` is initialized to `nullptr` (`BESApp.cc:40`) and never
assigned. `ServerApp.cc:298` (and the `show_usage` calls) dereference it.
`besctl help` runs `beslistener -h` (`besctl.in:353`), so it crashes too (Inferred from the same code path).

**M2. Response and handshake writes ignore partial writes.**
Inferred. See [Response path](#response-path-pptstreambuf). Both `PPTStreamBuf::sync()`
and `Socket::send()` need a loop that writes until all bytes are sent.

**M3. A Unix socket path longer than 103 bytes is silently truncated.**
Verified. `UnixSocket::listen()` uses `strncpy(..., 103)` (`UnixSocket.cc:168`) with
no length check, unlike `connect()`, which has one. With a 109-byte path, the socket
was created at the truncated name `…/scratchp`. The configured path did not exist, and
clients failed with `ENOENT`. The limit is 104 on macOS and 108 on Linux.

**M4. `TcpSocket::listen()` builds its address from uninitialized memory.**
Inferred. `struct sockaddr_in server;` is not zeroed (`TcpSocket.cc:290`; the
`= {}` is commented out), so `sin_zero` (and `sin_len` on BSD and macOS) hold garbage.
`inet_pton()` returns 0 for an invalid `BES.ServerIP`, but only `< 0` is checked
(`:295`), so a typo binds to an undefined address instead of failing.

**M5. `TcpSocket::connect()` (client side) treats a connect timeout as success.**
Inferred. `select()` returning 0 after 5 s is not handled. `SO_ERROR` is still 0 while
the connection is pending, so `_connected` is set to true (`TcpSocket.cc:~200-245`).
The method also resolves the host twice (`gethostbyname` and `getaddrinfo`), connects
with the `gethostbyname` result, and leaks the fd on every error path, because
`Socket::close()` closes only when `_connected` is true. This affects `bescmdln` and
tests, not the server.

**M6. `Socket::close()` does not close listening sockets.**
Verified by code trace. It checks only `_connected` (`Socket.cc:81`). A listening
socket has `_connected == false`, so `close()` does nothing to it. In practice the
process exits soon afterward, so the effect is limited to the daemon's
`start_command_processor` cleanup.

### Low

- `PPTConnection::readBufferNonBlocking()` writes progress digits to `std::cout`
  (`PPTConnection.cc:447`). It is used only by `PPTClient`. In a server process,
  stdout is the pipe to the daemon. It also compares `revents == POLLIN` exactly, so
  `POLLIN|POLLHUP` is ignored until the timeout.
- `SocketListener::accept()` uses `struct sockaddr from`, which is too small for
  `sockaddr_un` and `sockaddr_in6` (`SocketListener.cc:136`). Peer addresses on Unix
  sockets log as "UNKNOWN FAMILY".
- `TcpSocket::listen()` passes `sizeof(service_name)` (the size of `std::string`, not
  `NI_MAXSERV`) to `getnameinfo` (`TcpSocket.cc:314`). The listen backlog is hard-coded
  to 5 (`:367`). The master forks quickly, so this matters only during bursts.
- `Socket::receive()` writes an `ERROR_LOG` line on every `EINTR` or `EAGAIN` and
  retries (`Socket.cc:113`). On a non-blocking fd, that becomes a busy loop that fills
  the log.
- The libwrap path (`--with-libwrap`) calls `fromhost()` with no argument, and its
  deny condition is `paranoid && hosts_access()`, which is the reverse of the intended
  logic (`TcpSocket.cc:600-602`). This path probably does not compile, and it is not
  built by default (Inferred).
- `daemon.cc:stop_all_beslisteners()` switches on the return value of `killpg()`
  instead of `errno`, so its error messages never print (`daemon.cc:227`).
- `daemon.cc:load_names()` derives the listener path from `daemon_name`, which is the
  literal `"besdaemon"` (`daemon.cc:941`), not `argv[0]`. Without `-i`, the daemon
  always looks for `./beslistener` (Verified).
- `-i <dir>` without `-c` builds `<dir-without-trailing-slash>etc/bes/bes.conf`
  (`daemon.cc:1082`, `ServerApp.cc:327`). The slash between the two parts is missing.
  `besctl` always passes `-c`, which hides this.
- `pause()` in the daemon's no-admin-port loop (`daemon.cc:1219`) and the
  unblock-then-`select()` in the master (`ServerApp.cc:536-543`) both race with signal
  delivery. The worst case delays shutdown by up to 120 s in the master. `sigsuspend`
  or `pselect` would close the race.

## Corrections to BES_PPT.md

| BES_PPT.md says | The code does |
|---|---|
| The client sends `PPT_CLIENT_TESTING_CONNECTION`, and the server answers `PPT_SERVER_CONNECTION_OK` | Those are macro names. The bytes are `PPTCLIENT_TESTING_CONNECTION` and `PPTSERVER_CONNECTION_OK` (no underscore after `PPT`). The sequence charts show the macro names as if they were the bytes |
| The extensions example `0000019xstatus=PPT_EXIT_NOW;trace=on;` | `status=PPT_EXIT_NOW;trace=on;` is 29 bytes, so the header is `000001dx` (0x19 = 25 does not match either way). Also, the sender sorts extensions by name (it uses `std::map`) |
| `0000012d<response-bytes>` "12" | Lengths are hex. The example is consistent only if it means 18 bytes |
| The grammar in `PPTConnection.cc` says `chunk-size = 8HEX` | It is 7 hex digits plus a 1-byte type |

[`BES_PPT.md`](BES_PPT.md) was not edited. It is listed in `docs/Makefile.am`
`EXTRA_DIST`, so it ships with the distribution.

## Test Coverage

- `ppt/unit-tests` (`connT`, `sbT`, `extT`) cover chunk parsing, the stream buffer,
  and extension parsing against a mock socket. They run under `make check` when
  cppunit is available.
- Nothing tests the handshake, the master loop, fork and exit behavior, signal
  handling, or the daemon. Every High finding above is outside the automated tests.
  The probes used for this document (a Python client that sends handshake, request,
  idle connection, and RST cases to a scratch `beslistener`) could become the start of
  a small integration test. Running it needs only the in-tree build and a free port.

## Suggested Order of Work

These are small, reviewable changes, ordered by impact for the effort.

1. Catch `BESError` per connection inside the master loop (`ServerApp::run()` around
   `initConnection()`, or inside `PPTServer::initConnection()`): log it, close the
   accepted socket, and continue. Fixes H1. Low risk.
2. Put a timeout on the handshake read with `poll()` (a few seconds). Fixes H2. Moving
   the handshake into the child is cleaner, but it changes which process logs the
   handshake errors.
3. Write `BESLISTENER_RUNNING` after all listeners are open, whether TCP or Unix.
   Initialize `beslistener_start_status` and treat a short read as failure. Fixes H4.
4. Assign `_theApplication = this` in the `BESApp` constructor. Fixes M1.
5. Loop on short writes in `PPTStreamBuf::sync()` and `Socket::send()`. Fixes M2.
6. Decide what children should do on stop and restart (exit now, or drain, with a time
   limit). Then close the inherited listening fds in the child. Fixes H3. This changes
   behavior that the OLFS sees and needs coordination.
7. Add a length check in `UnixSocket::listen()`, and zero the address and check
   `inet_pton() == 1` in `TcpSocket::listen()`. Fixes M3 and M4.
