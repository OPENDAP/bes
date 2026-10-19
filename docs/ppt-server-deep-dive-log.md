# PPT and Server Deep Dive: Work Log

## 2026-10-13

**Prompt:** "Do a deep dive on the PPT code and overall server organization of the BES.
See the code in the `server` and `ppt`."

**Output:** [`ppt-server-deep-dive.md`](ppt-server-deep-dive.md)

### What I did

- Read all of `ppt/*.{h,cc}` and `server/*.{h,cc}`, plus `besctl.in`,
  `ServerExitConditions.h`, `BESDaemonConstants.h`, the `server/` and `ppt/unit-tests`
  `Makefile.am` files, and the related keys in `dispatch/bes/bes.conf.in`.
- Followed the calls from these two directories into `dispatch/`: `BESApp` (to find
  where `_theApplication` is assigned, which it never is) and
  `BESInterface`/`BESUtil` (SIGALRM and timeout cancellation, to judge how likely
  partial writes are).
- Read the existing [`BES_PPT.md`](BES_PPT.md) and compared its token strings and
  examples with `PPTProtocolNames.h` and `PPTConnection.cc`.
- Ran live probes against the in-tree build (no rebuild; the binaries were from
  2026-10-09). The scratch config, start script, and Python PPT client are in the
  session scratchpad, not in the repository. The port was 19022. A separate installed
  BES (PID 8141) was running throughout and was not touched.

### Live probes and results (macOS arm64)

| Probe | Result |
|---|---|
| Handshake + `showVersion` | Got `PPTSERVER_CONNECTION_OK` and a 330-byte response. `lsof` on the child showed it holding `TCP *:19022 (LISTEN)` and fd 1 |
| SIGTERM to a child that has an open client | The child kept running |
| SIGTERM to the process group (`kill -TERM -<master>`) | The master exited. The child survived, re-parented to 1, and still LISTENing. Its client's request finished normally |
| Starting a new master while that child was alive | `Failed to bind TCP socket: 19022: Address already in use` |
| An idle client (connected, sent nothing), then a second client | The second client timed out after 5 s. No fork happened |
| SIGTERM to the master while an idle client was connected | No effect until the idle client closed |
| A client connected, then sent RST (`SO_LINGER{1,0}`) | The master exited with `Connection reset by peer`. The next connection was refused |
| The same RST under `besdaemon -n -i … -c …` | The daemon printed "server cannot start, exited with status 1" and exited |
| `besdaemon` with only `BES.ServerUnixSocket` set | The daemon blocked in the pipe read and ignored SIGTERM. It needed SIGKILL |
| The same run, with a 109-byte socket path | The socket was created at a truncated path (`…/scratchp`) |
| `beslistener -v` (in-tree wrapper, `.libs`, installed `../build/bin`) | SIGSEGV (exit 139) in all three |
| `besdaemon` with no `-i` | `cannot find ./beslistener`, because the daemon's name is hard-coded, not taken from `argv[0]` |

All scratch processes were killed afterward, and the stray truncated socket file was
removed.

### Reasoning notes

- I wanted to know which process does the handshake. `PPTServer::initConnection()`
  calls `welcomeClient()` before `_handler->handle()`, and `handle()` is the call that
  forks. So the handshake runs in the master. Everything in H1 and H2 follows from
  that, together with `SA_RESTART` and the single `catch` in `ServerApp::run()`.
- I first expected children to exit on SIGTERM. They inherit handlers that only set
  flags, and `BESServerHandler::execute()` never reads those flags. The live test
  confirmed this before I wrote it down.
- The inherited LISTEN socket explains the bind failure. Nothing in the fork path
  closes it, and `FD_CLOEXEC` is not used anywhere.
- I marked the Linux behavior as Inferred throughout. Everything was tested on macOS
  only.
- The partial-write concern (M2) is real in the code. Its likelihood is low, because
  `BESUtil` cancels the alarm before it sends and there are no other periodic signals
  in a child. I kept it at Medium for that reason.
- I did not edit `BES_PPT.md`. The corrections are listed in the deep-dive document
  instead, so the user can decide how to change a file that ships in the distribution.
- I did not add the new documents to `docs/Makefile.am` `EXTRA_DIST`. The other
  deep-dive and log files are not listed there either. They are working documents, not
  distributed documentation.

### Not done

- No code changes, no build, no `make check`.
- No Linux testing.
- Not tested: the `--with-libwrap` build, the secure mode (compiled out), and the HAI
  admin commands over the wire.
