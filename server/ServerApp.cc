// ServerApp.cc

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2004-2009 University Corporation for Atmospheric Research
// Author: Patrick West <pwest@ucar.edu> and Jose Garcia <jgarcia@ucar.edu>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//
// You can contact University Corporation for Atmospheric Research at
// 3080 Center Green Drive, Boulder, CO 80301

// (c) COPYRIGHT University Corporation for Atmospheric Research 2004-2005
// Please read the full copyright statement in the file COPYRIGHT_UCAR.
//
// Authors:
//      pwest       Patrick West <pwest@ucar.edu>
//      jgarcia     Jose Garcia <jgarcia@ucar.edu>

#include "config.h"

#include <unistd.h>
#include <csignal>
#include <sys/wait.h> // for wait

#include <iostream>
#include <exception>
#include <sstream>

#include <cstring>
#include <cstdlib>
#include <cerrno>

#include <libxml/xmlmemory.h>

#include "ServerApp.h"
#include "ServerExitConditions.h"
#include "TheBESKeys.h"
#include "BESLog.h"
#include "SocketListener.h"
#include "TcpSocket.h"
#include "UnixSocket.h"
#include "BESServerHandler.h"
#include "BESError.h"
#include "PPTServer.h"
#include "BESMemoryManager.h"
#include "BESDebug.h"
#include "BESCatalogUtils.h"
#include "BESUtil.h"
#include "BESServerUtils.h"
#include "BESIndent.h"

#include "BESDefaultModule.h"
#include "BESXMLDefaultCommands.h"
#include "BESDaemonConstants.h"

using namespace std;

static int session_id = 0;

// These are set to 1 by their respective handlers and then processed in the
// signal processing loop.
static volatile sig_atomic_t sigchild = 0;
static volatile sig_atomic_t sigpipe = 0;
static volatile sig_atomic_t sigterm = 0;
static volatile sig_atomic_t sighup = 0;

// Set in ServerApp::initialize().
// Added jhrg 9/22/15
static volatile int master_listener_pid = -1;

#if 1
static string bes_exit_message(int cpid, int stat)
{
    ostringstream oss;
    oss << "beslistener child pid: " << cpid;
    if (WIFEXITED(stat)) { // exited via exit()?
        oss << " exited with status: " << WEXITSTATUS(stat);
    }
    else if (WIFSIGNALED(stat)) { // exited via a signal?
        oss << " exited with signal: " << WTERMSIG(stat);
#ifdef WCOREDUMP
        if (WCOREDUMP(stat)) oss << " and a core dump!";
#endif
    }
    else {
        oss << " exited, but I have no clue as to why";
    }

    return oss.str();
}
#endif

// These two functions duplicate code in daemon.cc
static void block_signals()
{
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGCHLD);
    sigaddset(&set, SIGHUP);
    sigaddset(&set, SIGTERM);
    sigaddset(&set, SIGPIPE);

    if (sigprocmask(SIG_BLOCK, &set, nullptr) < 0) {
        throw BESInternalError(string("sigprocmask error: ") + strerror(errno) + " while trying to block signals.",
            __FILE__, __LINE__);
    }
}

static void unblock_signals()
{
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGCHLD);
    sigaddset(&set, SIGHUP);
    sigaddset(&set, SIGTERM);
    sigaddset(&set, SIGPIPE);

    if (sigprocmask(SIG_UNBLOCK, &set, nullptr) < 0) {
        throw BESInternalError(string("sigprocmask error: ") + strerror(errno) + " while trying to unblock signals.",
            __FILE__, __LINE__);
    }
}

// I moved the signal handlers here so that signal processing would be simpler
// and no library calls would be made to functions that are not 'asynch safe'.
// This was the fix for ticket 2025 and friends (the zombie process problem).
// jhrg 3/3/14

// This is needed so that the master bes listener will get the exit status of
// all the child bes listeners (preventing them from becoming zombies).
static void catch_sig_child(int sig)
{
    if (sig == SIGCHLD) {
        sigchild = 1;
    }
}

// If the HUP signal is sent to the master beslistener, it should exit and
// return a value indicating to the besdaemon that it should be restarted.
// This also has the side-affect of re-reading the configuration file.
static void catch_sig_hup(int sig)
{
    if (sig == SIGHUP) {
        sighup = 1;
    }
}

static void catch_sig_pipe(int sig)
{
    if (sig == SIGPIPE) {
        // When a child listener catches SIGPIPE it is because of a
        // failure on one of its I/O connections - file I/O or, more
        // likely, network I/O. I have found that C++ ostream objects
        // seem to 'hide' sigpipe so that a child listener will run
        // for some time after the client has dropped the
        // connection. Whether this is from buffering or some other
        // problem, the situation happens when either the remote
        // client to exits (e.g., curl) or when Tomcat is stopped
        // using SIGTERM. So, even though the normal behavior for a
        // Unix daemon is to look at error codes from write(), etc.,
        // and exit based on those, this code exits whenever the child
        // listener catches SIGPIPE. However, if this is the Master
        // listener, allow the processing loop to handle this signal
        // and do not exit.  jhrg 9/22/15
        if (getpid() != master_listener_pid) {
            INFO_LOG("Child listener (PID: " << getpid() << ") caught SIGPIPE (master listener PID: "
                << master_listener_pid << "). Child listener Exiting." << endl);

            // cleanup code here; only the Master listener should run the code
            // in ServerApp::terminate(); do nothing for cleanup for a child
            // listener. jhrg 9/22/15

            // Note that exit() is not safe for use in a signal
            // handler, so we fall back to the default behavior, which
            // is to exit.
            signal(sig, SIG_DFL);
            raise(sig);
        }
        else {
            INFO_LOG("Master listener (PID: " << getpid() << ") caught SIGPIPE." << endl);

            sigpipe = 1;
        }
    }
}

// This is the default signal sent by 'kill'; when the master beslistener gets
// this signal it should stop. besdaemon should not try to start a new
// master beslistener.
static void catch_sig_term(int sig)
{
    if (sig == SIGTERM) {
        sigterm = 1;
    }
}

/** Register the signal handlers. This registers handlers for HUP, TERM and
 *  CHLD. For each, if this OS supports restarting 'slow' system calls, enable
 *  that. For the TERM and HUP handlers, block SIGCHLD for the duration of
 *  the handler (we call stop_all_beslisteners() in those handlers and that
 *  function uses wait() to collect the exit status of the child processes).
 *  This ensure that our signal handlers (TERM and HUP) don't themselves get
 *  interrupted.
 */
static void register_signal_handlers()
{
    struct sigaction act{};
    sigemptyset(&act.sa_mask);
    sigaddset(&act.sa_mask, SIGCHLD);
    sigaddset(&act.sa_mask, SIGPIPE);
    sigaddset(&act.sa_mask, SIGTERM);
    sigaddset(&act.sa_mask, SIGHUP);
    act.sa_flags = 0;
#ifdef SA_RESTART
    BESDEBUG("beslistener", "beslistener: setting restart for sigchld." << endl);
    act.sa_flags |= SA_RESTART;
#endif

    BESDEBUG("beslistener", "beslistener: Registering signal handlers ... " << endl);

    act.sa_handler = catch_sig_child;
    if (sigaction(SIGCHLD, &act, 0))
        throw BESInternalFatalError("Could not register a handler to catch beslistener child process status.", __FILE__,
        __LINE__);

    act.sa_handler = catch_sig_pipe;
    if (sigaction(SIGPIPE, &act, 0) < 0)
        throw BESInternalFatalError("Could not register a handler to catch beslistener pipe signal.", __FILE__,
        __LINE__);

    act.sa_handler = catch_sig_term;
    if (sigaction(SIGTERM, &act, 0) < 0)
        throw BESInternalFatalError("Could not register a handler to catch beslistener terminate signal.", __FILE__,
        __LINE__);

    act.sa_handler = catch_sig_hup;
    if (sigaction(SIGHUP, &act, 0) < 0)
        throw BESInternalFatalError("Could not register a handler to catch beslistener hup signal.", __FILE__,
        __LINE__);

    BESDEBUG("beslistener", "beslistener: OK" << endl);
}

ServerApp::ServerApp() : BESModuleApp()
{
    d_pid = getpid();
}

int ServerApp::initialize(int argc, char **argv)
{
    int c = 0;
    bool needhelp = false;
    string dashi;
    string dashc;
    string dashd;

    // If you change the getopt statement below, be sure to make the
    // corresponding change in daemon.cc and besctl.in
    while ((c = getopt(argc, argv, "hvsd:c:p:u:i:r:H:")) != -1) {
        switch (c) {
        case 'i':
            dashi = optarg;
            break;
        case 'c':
            dashc = optarg;
            break;
        case 'r':
            break; // we can ignore the /var/run directory option here
        case 'p':
            d_port = atoi(optarg);
            d_got_port = true;
            break;
        case 'H':
            d_ip_value = optarg;
            d_got_ip = true;
            break;
        case 'u':
            d_unix_socket_value = optarg;
            break;
        case 'd':
            dashd = optarg;
            break;
        case 'v':
            BESServerUtils::show_version(BESApp::TheApplication()->appName());
            break;
        case 's':
            d_is_secure = true;
            break;
        case 'h':
        case '?':
        default:
            needhelp = true;
            break;
        }
    }

    // before we can do any processing, log any messages, initialize any
    // modules, do anything, we need to determine where the BES
    // configuration file lives. From here we get the name of the log
    // file, group and user id, and information that the modules will
    // need to run properly.

    // If the -c option was passed, set the config file name in TheBESKeys
    if (!dashc.empty()) {
        TheBESKeys::ConfigFile = dashc;
    }

    // If the -c option was not passed, but the -i option
    // was passed, then use the -i option to construct
    // the path to the config file
    if (dashc.empty() && !dashi.empty()) {
        BESUtil::trim_if_trailing_slash(dashi);
        string conf_file = dashi + "etc/bes/bes.conf";
        TheBESKeys::ConfigFile = conf_file;
    }

    if (!dashd.empty()) BESDebug::SetUp(dashd);

    // register the two debug context for the server and ppt. The
    // Default Module will register the bes context.
    BESDebug::Register("server");
    BESDebug::Register("ppt");

    // Because we are now running as the user specified in the
    // configuration file, we won't be able to listen on system ports.
    // If this is a problem, we may need to move this code above setting
    // the user and group ids.
    bool found = false;
    string port_key = "BES.ServerPort";
    if (!d_got_port) {
        string sPort;
        try {
            TheBESKeys::TheKeys()->get_value(port_key, sPort, found);
        }
        catch (BESError &e) {
            string err = string("FAILED: ") + e.get_message();
            cerr << err << endl;
            ERROR_LOG(err << endl);
            exit(SERVER_EXIT_FATAL_CANNOT_START);
        }
        if (found) {
            d_port = atoi(sPort.c_str());
            if (d_port != 0) {
                d_got_port = true;
            }
        }
    }

    found = false;
    string ip_key = "BES.ServerIP";
    if (!d_got_ip) {
        try {
            TheBESKeys::TheKeys()->get_value(ip_key, d_ip_value, found);
        }
        catch (BESError &e) {
            string err = string("FAILED: ") + e.get_message();
            cerr << err << endl;
            ERROR_LOG(err << endl);
            exit(SERVER_EXIT_FATAL_CANNOT_START);
        }

        if (found) {
            d_got_ip = true;
        }
    }

    found = false;
    string socket_key = "BES.ServerUnixSocket";
    if (d_unix_socket_value.empty()) {
        try {
            TheBESKeys::TheKeys()->get_value(socket_key, d_unix_socket_value, found);
        }
        catch (BESError &e) {
            string err = string("FAILED: ") + e.get_message();
            cerr << err << endl;
            ERROR_LOG(err << endl);
            exit(SERVER_EXIT_FATAL_CANNOT_START);
        }
    }

    if (!d_got_port && d_unix_socket_value.empty()) {
        string msg = "Must specify a tcp port or a unix socket or both\n";
        msg += "Please specify on the command line with -p <port>";
        msg += " and/or -u <unix_socket>\n";
        msg += "Or specify in the bes configuration file with " + port_key + " and/or " + socket_key + "\n";
        cout << endl << msg;
        ERROR_LOG(msg << endl);
        BESServerUtils::show_usage(BESApp::TheApplication()->appName());
    }

    found = false;
    if (!d_is_secure) {
        string key = "BES.ServerSecure";
        string isSecure;
        try {
            TheBESKeys::TheKeys()->get_value(key, isSecure, found);
        }
        catch (BESError &e) {
            string err = string("FAILED: ") + e.get_message();
            cerr << err << endl;
            ERROR_LOG(err << endl);
            exit(SERVER_EXIT_FATAL_CANNOT_START);
        }
        if (isSecure == "Yes" || isSecure == "YES" || isSecure == "yes") {
            d_is_secure = true;
        }
    }

    BESDEBUG("beslistener", "beslistener: initializing default module ... " << endl);
    BESDefaultModule::initialize(argc, argv);
    BESDEBUG("beslistener", "beslistener: done initializing default module" << endl);

    BESDEBUG("beslistener", "beslistener: initializing default commands ... " << endl);
    BESXMLDefaultCommands::initialize(argc, argv);
    BESDEBUG("beslistener", "beslistener: done initializing default commands" << endl);

    // This will load and initialize all the modules
    BESDEBUG("beslistener", "beslistener: initializing loaded modules ... " << endl);
    int ret = BESModuleApp::initialize(argc, argv);
    BESDEBUG("beslistener", "beslistener: done initializing loaded modules" << endl);

    BESDEBUG("beslistener", "beslistener: initialized settings:" << *this);

    if (needhelp) {
        BESServerUtils::show_usage(BESApp::TheApplication()->appName());
    }

    // This sets the process group to be ID of this process. All children
    // will get this GID. Then use killpg() to send a signal to this process
    // and all the children.
    session_id = setsid();
    BESDEBUG("beslistener", "beslistener: The master beslistener session id (group id): " << session_id << endl);

    master_listener_pid = getpid();
    BESDEBUG("beslistener", "beslistener: The master beslistener Process id: " << master_listener_pid << endl);

    return ret;
}

// NB: when this method returns, the return value is passed to the ServerApp::terminate()
// method. Look at BESApp.cc to see how BESApp::main() is written.
int ServerApp::run()
{
    try {
        BESDEBUG("beslistener", "beslistener: initializing memory pool ... " << endl);
        BESMemoryManager::initialize_memory_pool();
        BESDEBUG("beslistener", "OK" << endl);

        SocketListener listener;
        if (d_port) {
            if (!d_ip_value.empty())
                d_tcp_socket = new TcpSocket(d_ip_value, d_port);
            else
                d_tcp_socket = new TcpSocket(d_port);

            listener.listen(d_tcp_socket);

            BESDEBUG("beslistener", "beslistener: listening on port (" << d_port << ")" << endl);

            // Write to stdout works because the besdaemon is listening on the
            // other end of a pipe where the pipe fd[1] has been dup2'd to
            // stdout. See daemon.cc:start_master_beslistener.
            // NB MASTER_TO_DAEMON_PIPE_FD is 1 (stdout)
            int status = BESLISTENER_RUNNING;
            long res = write(MASTER_TO_DAEMON_PIPE_FD, &status, sizeof(status));

            if (res == -1) {
                ERROR_LOG("Master listener could not send status to daemon: " << strerror(errno) << endl);
                ::exit(SERVER_EXIT_FATAL_CANNOT_START);
            }
        }

        if (!d_unix_socket_value.empty()) {
            d_unix_socket = new UnixSocket(d_unix_socket_value);
            listener.listen(d_unix_socket);
            BESDEBUG("beslistener", "beslistener: listening on unix socket (" << d_unix_socket_value << ")" << endl);
        }

        BESServerHandler handler;

        d_ppt_server = new PPTServer(&handler, &listener, d_is_secure);

        register_signal_handlers();

        // Loop forever, processing signals and running the code in PPTServer::initConnection().
        // NB: The code in initConnection() used to loop forever, but I moved that out to here
        // so the signal handlers could be in this class. The PPTServer::initConnection() method
        // is also used by daemon.cc but this class (ServerApp; the beslistener) and the besdaemon
        // need to do different things for the signals like HUP and TERM, so they cannot share
        // the signal processing code. One fix for the problem described in ticket 2025 was to
        // move the signal handlers into PPTServer. Changing how the 'forever' loops are organized
        // and keeping the signal processing code here (and in daemon.cc) is another solution that
        // preserves the correct behavior of the besdaemon, too. jhrg 3/5/14
        while (true) {
            block_signals();

            if (sigterm | sighup | sigchild | sigpipe) {
                int stat;
                pid_t cpid;
                while ((cpid = wait4(0 /*any child in the process group*/, &stat, WNOHANG, 0/*no rusage*/)) > 0) {
                    d_ppt_server->decr_num_children();
                    if (sigpipe) {
                        INFO_LOG("Master listener caught SISPIPE from child: " << cpid << endl);
                    }
                    INFO_LOG(bes_exit_message(cpid, stat) << "; num children: " << d_ppt_server->get_num_children() << endl);
                    BESDEBUG("ppt2", bes_exit_message(cpid, stat) << "; num children: " << d_ppt_server->get_num_children() << endl);
                }
            }

            if (sighup) {
                BESDEBUG("ppt2", "Master listener caught SIGHUP, exiting with SERVER_EXIT_RESTART" << endl);

                INFO_LOG("Master listener caught SIGHUP, exiting with SERVER_EXIT_RESTART" << endl);

#if 0
                d_ppt_server->closeConnection();
                close(BESLISTENER_PIPE_FD);
#endif
                return SERVER_EXIT_RESTART;
#if 0
                ::exit(SERVER_EXIT_RESTART);
#endif
            }

            if (sigterm) {
                BESDEBUG("ppt2", "Master listener caught SIGTERM, exiting with SERVER_NORMAL_SHUTDOWN" << endl);

                INFO_LOG("Master listener caught SIGTERM, exiting with SERVER_NORMAL_SHUTDOWN" << endl);

#if 0
                d_ppt_server->closeConnection();
                close(BESLISTENER_PIPE_FD);
#endif
                return SERVER_EXIT_NORMAL_SHUTDOWN;
#if 0
                ::exit(SERVER_EXIT_NORMAL_SHUTDOWN);
#endif
            }

            sigchild = 0;   // Only reset this signal, all others cause an exit/restart
            unblock_signals();

            // This is where the 'child listener' is started. This method will call
            // BESServerHandler::handle(...) that will, in turn, fork. The child process
            // becomes the 'child listener' that actually processes a request.
            //
            // This call blocks, using select(), until a client asks for another beslistener.
            d_ppt_server->initConnection();
        }
#if 0
        d_ppt_server->closeConnection();
#endif
    }
    catch (BESError &se) {
        BESDEBUG("beslistener", "beslistener: caught BESError (" << se.get_message() << ")" << endl);

        ERROR_LOG(se.get_message() << endl);
#if 0
        int status = SERVER_EXIT_FATAL_CANNOT_START;
        write(MASTER_TO_DAEMON_PIPE_FD, &status, sizeof(status));
        close(MASTER_TO_DAEMON_PIPE_FD);
#endif
        return SERVER_EXIT_FATAL_CANNOT_START;
    }
    catch (...) {
        ERROR_LOG("caught unknown exception initializing sockets" << endl);
#if 0
        int status = SERVER_EXIT_FATAL_CANNOT_START;
        write(MASTER_TO_DAEMON_PIPE_FD, &status, sizeof(status));
        close(MASTER_TO_DAEMON_PIPE_FD);
#endif
        return SERVER_EXIT_FATAL_CANNOT_START;
    }

#if 0
    close(BESLISTENER_PIPE_FD);
    return 0;
#endif
}

// The BESApp::main() method will call terminate() with the return value of
// run(). The return value from terminate() is the return value the BESApp::main().
int ServerApp::terminate(int status)
{
    pid_t apppid = getpid();
    // is this the parent process - the master beslistener?
    if (apppid == d_pid) {
        // I don't understand the following comment. jhrg 3/23/22
        // These are all safe to call in a signal handler
        if (d_ppt_server) {
            d_ppt_server->closeConnection();
            delete d_ppt_server;
        }
        if (d_tcp_socket) {
            d_tcp_socket->close();
            delete d_tcp_socket;
        }
        if (d_unix_socket) {
            d_unix_socket->close();
            delete d_unix_socket;
        }

        // Do this in the reverse order that it was initialized. So
        // terminate the loaded modules first, then the default
        // commands, then the default module.

        // These are not safe to call in a signal handler
        BESDEBUG("beslistener", "beslistener: terminating loaded modules ...  " << endl);
        BESModuleApp::terminate(status);
        BESDEBUG("beslistener", "beslistener: done terminating loaded modules" << endl);

        BESDEBUG("beslistener", "beslistener: terminating default commands ...  " << endl);
        BESXMLDefaultCommands::terminate();
        BESDEBUG("beslistener", "beslistener: done terminating default commands ...  " << endl);

        BESDEBUG("beslistener", "beslistener: terminating default module ... " << endl);
        BESDefaultModule::terminate();
        BESDEBUG("beslistener", "beslistener: done terminating default module ... " << endl);

        xmlCleanupParser();

        // Only the master listener and the daemon know about this communication channel.
        // Once we send the status back to the daemon, close the pipe.
        write(MASTER_TO_DAEMON_PIPE_FD, &status, sizeof(status));
        close(MASTER_TO_DAEMON_PIPE_FD);
    }

    return status;
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance
 *
 * @param strm C++ i/o stream to dump the information to
 */
void ServerApp::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << "ServerApp::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();
    strm << BESIndent::LMarg << "got IP? " << d_got_ip << endl;
    strm << BESIndent::LMarg << "IP: " << d_ip_value << endl;
    strm << BESIndent::LMarg << "got port? " << d_got_port << endl;
    strm << BESIndent::LMarg << "port: " << d_port << endl;
    strm << BESIndent::LMarg << "unix socket: " << d_unix_socket_value << endl;
    strm << BESIndent::LMarg << "is secure? " << d_is_secure << endl;
    strm << BESIndent::LMarg << "pid: " << d_pid << endl;
    if (d_tcp_socket) {
        strm << BESIndent::LMarg << "tcp socket:" << endl;
        BESIndent::Indent();
        d_tcp_socket->dump(strm);
        BESIndent::UnIndent();
    }
    else {
        strm << BESIndent::LMarg << "tcp socket: null" << endl;
    }
    if (d_unix_socket) {
        strm << BESIndent::LMarg << "unix socket:" << endl;
        BESIndent::Indent();
        d_unix_socket->dump(strm);
        BESIndent::UnIndent();
    }
    else {
        strm << BESIndent::LMarg << "unix socket: null" << endl;
    }
    if (d_ppt_server) {
        strm << BESIndent::LMarg << "ppt server:" << endl;
        BESIndent::Indent();
        d_ppt_server->dump(strm);
        BESIndent::UnIndent();
    }
    else {
        strm << BESIndent::LMarg << "ppt server: null" << endl;
    }
    BESModuleApp::dump(strm);
    BESIndent::UnIndent();
}

int main(int argc, char **argv)
{
    try {
        ServerApp app;
        return app.main(argc, argv);
    }
    catch (BESError &e) {
        cerr << "Caught unhandled exception: " << endl;
        cerr << e.get_message() << endl;
        return 1;
    }
    catch (const std::exception &e) {
        cerr << "Caught unhandled standard exception: " << endl;
        cerr << e.what() << endl;
        return 1;
    }
    catch (...) {
        cerr << "Caught unhandled, unknown exception" << endl;
        return 1;
    }
}
