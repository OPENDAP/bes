// daemon.cc

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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// You can contact University Corporation for Atmospheric Research at
// 3080 Center Green Drive, Boulder, CO 80301

// (c) COPYRIGHT University Corporation for Atmospheric Research 2004-2005
// Please read the full copyright statement in the file COPYRIGHT_UCAR.
//
// Authors:
//      pwest       Patrick West <pwest@ucar.edu>
//      jgarcia     Jose Garcia <jgarcia@ucar.edu>

#include <unistd.h>  // for getopt fork setsid execvp access geteuid
#include <sys/wait.h>  // for waitpid
#include <sys/types.h>
#include <sys/stat.h>  // for chmod
#include <ctype.h> // for isdigit
#include <signal.h>

#include <fstream>
#include <iostream>
#include <string>
#include <sstream>
#include <cstring>
#include <cstdlib>
#include <cerrno>

using std::ifstream;
using std::ofstream;
using std::cout;
using std::endl;
using std::cerr;
using std::flush;
using std::string;

#include "config.h"
#include "ServerExitConditions.h"
#include "SocketListener.h"
#include "TcpSocket.h"
#include "UnixSocket.h"
#include "PPTServer.h"
#include "BESModuleApp.h"
#include "DaemonCommandHandler.h"
#include "BESServerUtils.h"
#include "BESScrub.h"
#include "BESError.h"
#include "BESDebug.h"
#include "TheBESKeys.h"
#include "BESLog.h"
#include "BESDaemonConstants.h"

#define BES_SERVER "/beslistener"
#define BES_SERVER_PID "/bes.pid"
#define DAEMON_PORT_STR "BES.DaemonPort"
#define DAEMON_UNIX_SOCK_STR "BES.DaemonUnixSocket"

// These are called from DaemonCommandHandler
void block_signals();
void unblock_signals();
int start_master_beslistener();
bool stop_all_beslisteners(int sig);

static string daemon_name;

// This two variables are set by load_names
static string beslistener_path;
static string file_for_daemon_pid;
// This can be used to see if HUP or TERM has been sent to the master bes
int master_beslistener_status = BESLISTENER_STOPPED;

static int master_beslistener_pid = -1; // This is also the process group id

typedef map<string,string> arg_map;
static arg_map global_args;
static string debug_sink = "";

static TcpSocket *my_socket = 0;
static UnixSocket *unix_socket = 0;
static PPTServer *command_server = 0;

static string errno_str(const string &msg)
{
    ostringstream oss;
    oss << daemon_name << msg;
    const char *perror_string = strerror(errno);
    if (perror_string)
        oss << perror_string;
    oss << endl;
    return oss.str();
}

/** Evaluate the exit status returned to the besdaemon by the master
 beslistener and return 0, 1 or SERVER_EXIT_RESTART.

 @param status The status (exit value) of the child process.
 @return If the status indicates that the child process exited normally,
 return 0; abnormally, return 1; indicating restart needed, return the
 value of SERVER_EXIT_RESTART.
 */
static int pr_exit(int status)
{
    if (WIFEXITED( status )) {
        switch (WEXITSTATUS( status )) {
            case SERVER_EXIT_NORMAL_SHUTDOWN:
                return 0;

            case SERVER_EXIT_FATAL_CAN_NOT_START:
                cerr << daemon_name << ": server cannot start, exited with status " << WEXITSTATUS( status ) << endl;
                cerr << "Please check all error messages " << "and adjust server installation" << endl;
                return 1;

            case SERVER_EXIT_ABNORMAL_TERMINATION:
                cerr << daemon_name << ": abnormal server termination, exited with status " << WEXITSTATUS( status ) << endl;
                return 1;

            case SERVER_EXIT_RESTART:
                cerr << daemon_name << ": server has been requested to re-start." << endl;
                return SERVER_EXIT_RESTART;

            default:
                return 1;
        }
    }
    else if (WIFSIGNALED( status )) {
        cerr << daemon_name << ": abnormal server termination, signaled with signal number " << WTERMSIG( status ) << endl;
#ifdef WCOREDUMP
        if (WCOREDUMP( status )) {
            cerr << daemon_name << ": server dumped core." << endl;
            return 1;
        }
#endif
        return 1;
    }
    else if (WIFSTOPPED( status )) {
        cerr << daemon_name << ": abnormal server termination, stopped with signal number " << WSTOPSIG( status ) << endl;
        return 1;
    }

    return 0;
}

void block_signals()
{
    sigset_t set;
    sigemptyset (&set);
    sigaddset(&set, SIGCHLD);
    sigaddset(&set, SIGHUP);
    sigaddset(&set, SIGTERM);

    if (sigprocmask(SIG_BLOCK, &set, 0) < 0) {
        cerr << errno_str(": sigprocmask error, blocking signals in stop_all_beslisteners ");
    }
}

void unblock_signals()
{
    sigset_t set;
    sigemptyset (&set);
    sigaddset(&set, SIGCHLD);
    sigaddset(&set, SIGHUP);
    sigaddset(&set, SIGTERM);

    if (sigprocmask(SIG_UNBLOCK, &set, 0) < 0) {
        cerr << errno_str(": sigprocmask error unblocking signals in stop_all_beslisteners ");
    }
}
/** Stop all of the listeners (both the master listener and all of the
 *  child listeners that actually process requests). A test version of this
 *  used the master beslistener's exit status to determine if the daemon
 *  should try to restart the master beslistener. That feature is retained
 *  so that the daemon can start without listening on the BESDaemonPort, thus
 *  eliminating a security issue for sites without a firewall.
 *
 *  Uses the master bestistener's process group to send 'sig' to all of the
 *  beslisteners.
 *
 * @param sig Signal to send to all beslisteners.
 * @return true if the master beslistener status was caught, false otherwise
 */
bool stop_all_beslisteners(int sig)
{
    BESDEBUG("besdaemon", "besdaemon: stopping listeners" << endl);

    block_signals();
    BESDEBUG("besdaemon", "besdaemon: blocking signals " << endl);

    BESDEBUG("besdaemon", "besdaemon: master_beslistener_pid " << master_beslistener_pid << endl);
    // Send 'sig' to all members of the process group with/of the master bes.
    // The master beslistener pid is the group id of all of the beslisteners.
    int status = killpg(master_beslistener_pid, sig);
    switch (status) {
        case EINVAL:
            cerr << "The sig argument is not a valid signal number." << endl;
            break;

        case EPERM:
            cerr << "The sending process is not the super-user and one or more of the target processes has an effective user ID different from that of the sending process." << endl;
            break;

        case ESRCH:
            cerr << "No process can be found in the process group specified by the process group (" << master_beslistener_pid << ")." << endl;
            break;

        default: // No error
            break;
    }

    bool mbes_status_caught = false;
    int pid;
    while ((pid = wait(&status)) > 0) {
        BESDEBUG("besdaemon", "besdaemon: caught listener: " << pid << " raw status: " << status << endl);
        if (pid == master_beslistener_pid) {
            master_beslistener_status = pr_exit(status);
            mbes_status_caught = true;
            BESDEBUG("besdaemon", "besdaemon: caught master beslistener: " << pid << " status: " << master_beslistener_status << endl);
        }
    }

    BESDEBUG("besdaemon", "besdaemon: done catching listeners (last pid:" << pid << ")" << endl);

    unblock_signals();
    BESDEBUG("besdaemon", "besdaemon: unblocking signals " << endl);
    return mbes_status_caught;
}

/** Update the arguments passed to the master beslistener so that changes in
 *  the debug/log contexts set in the besdaemon will also be set in the
 *  beslisteners.
 *
 *  @note The arguments are held in a static global variable - use this
 *  function to alter that array.
 */
char **update_beslistener_args()
{
    // string contexts = BESDebug::GetOptionsString();

    char **arguments = new char*[global_args.size() * 2 + 1];

    // Marshal the arguments to the listener from the command line
    // arguments to the daemon
    arguments[0] = strdup(global_args["beslistener"].c_str());

    int i = 1;
    arg_map::iterator it;
    for (it = global_args.begin() ; it != global_args.end(); ++it) {
        BESDEBUG("besdaemon", "besdaemon; global_args " << (*it).first << " => " << (*it).second << endl);
        // Build the complete command line args for the beslistener, with
        // special case code for -d and to omit the 'beslistener' line
        // since it's already set in arguments[0].
        if ((*it).first == "-d") {
            arguments[i++] = strdup("-d");
            // This is where the current debug/log settings are grabbed and
            // used to build the correct '-d' option value for the new
            // beslistener.
            string debug_opts = debug_sink + "," + BESDebug::GetOptionsString();
            arguments[i++] = strdup(debug_opts.c_str());
            // ***
            // cerr << "setting -d value to: " << debug_opts << endl;
        }
        else if ((*it).first != "beslistener") {
            arguments[i++] = strdup((*it).first.c_str());
            arguments[i++] = strdup((*it).second.c_str());
        }
    }
    arguments[i] = 0;       // terminal null

    // ***
    // for (int j = 0; j < i; ++j)
        // cerr << "arguments[" << j << "]: " << arguments[j] << endl;

    return arguments;
}

/** Start the 'master beslistener' and return its PID. This function also
 sets the global 'master_beslistener_pid' so that other code in this file
 (like the signal handlers) can have access to it. It starts the beslistener
 using the global 'arguments' that is a copy of the arguments passed to this
 daemon.

 @note The globals 'arguments' and 'master_beslistener_pid' are used here
 (but not passed in) so that they can be kept local to this file. A poor-man's
 version of encapsulation.

 @return 0 for an error or the PID of the master beslistener
 */
int start_master_beslistener()
{
    // The only certain way to know that the beslistener master has started is
    // to pass back its status once it is initialized. Use a pipe for that.
    int fd[2];
    if (pipe(fd) < 0) {
        cerr << errno_str(": pipe error ");
        return 0;
    }

    int pid;
    if ((pid = fork()) < 0) {
        cerr << errno_str(": fork error ");
        return 0;
    }
    else if (pid == 0) { // child process  (the master beslistener)
        // See 'int ServerApp::run()' for the place where the program exec'd
        // below writes the pid value to the pipe.

        close(fd[0]); // Close the read end of the pipe in the child

        // dup2 so we know the FD to write to in the child
        if (dup2(fd[1], BESLISTENER_PIPE_FD) != BESLISTENER_PIPE_FD) {
            cerr << errno_str(": dup2 error ");
            return 0;
        }

        // We don't have to free this because this is a different process
        // than the parent.
        char **arguments = update_beslistener_args();

        BESDEBUG("besdaemon", "Starting: " << arguments[0] << endl);
        // ***
        // cerr << "Starting: " << arguments[0] << endl;

        // Close the socket for the besdaemon here. This keeps if from being
        // passed into the master beslistener and then entering the state
        // CLOSE_WAIT once the besdaemon's client closes it's end.
        if (command_server)
            command_server->closeConnection();

        // This is where beslistener - the master listener - is started
        execvp(arguments[0], arguments);

        // if we are still here, it's an error...
        cerr << errno_str(": mounting listener, subprocess failed: ");
        exit(1); //NB: This exits from the child process.
    }

    // parent process (the besdaemon)

    // The daemon records the pid of the master beslistener, but only does so
    // when that process writes its status to the pipe 'fd'.

    close(fd[1]); // close the write end of the pipe in the parent.

    BESDEBUG("besdaemon", "besdaemon: master beslistener pid: " << pid << endl);

    // Read the status from the child (beslistener).
    int beslistener_start_status;
    if (read(fd[0], &beslistener_start_status, sizeof(beslistener_start_status)) < 0) {
        cerr << "Could not read master beslistener status; the master pid was not changed." << endl;
        close(fd[0]);
        return 0;
    }
    else if (beslistener_start_status != BESLISTENER_RUNNING) {
        cerr << "Could not read master beslistener status; the master pid was not changed." << endl;
        close(fd[0]);
        return 0;
    }
    else {
        BESDEBUG("besdaemon", "besdaemon: master beslistener start status: " << beslistener_start_status << endl);
        // Setting master_beslistener_pid here and not forcing callers to use the
        // return value means that this global can be local to this file.
        master_beslistener_pid = pid;
        master_beslistener_status = BESLISTENER_RUNNING;
    }

    close(fd[0]);
    return pid;
}

/** Clean up resources allocated by main(). This is called both by main() and
 *  by the SIGTERM signal handler.
 */
static void cleanup_resources()
{
    //delete[] arguments;
    //arguments = 0;

    if (!access(file_for_daemon_pid.c_str(), F_OK)) {
        (void) remove(file_for_daemon_pid.c_str());
    }
}

// Note that SIGCHLD, SIGTERM and SIGHUP are blocked while in these three
// signal handlers below.

// Catch SIGCHLD. This is used to detect if the HUP signal was sent to the
// beslistener(s) and they have returned SERVER_EXIT_RESTART by recording
// that value in the global 'master_beslistener_status'. Other code needs
// to test that (static) global to see if the beslistener should be restarted.
static void CatchSigChild(int signal)
{
    if (signal == SIGCHLD) {
        int status;
        int pid = wait(&status);

        BESDEBUG("besdaemon", "besdaemon: SIGCHLD: caught master beslistener (" << pid << ") status: " << pr_exit(status) << endl);
        // Decode and record the exit status, but only if it really is the
        // master beslistener this daemon is using. If two or more Start commands
        // are sent in a row, a master beslistener will start, fail to bind to
        // the port (because another master beslstener is already bound to it)
        // and exit. We don't want to record that second process's exit status here.
        if (pid == master_beslistener_pid)
            master_beslistener_status = pr_exit(status);
    }
}

// The two following signal handlers implement a simple stop/restart behavior
// for the daemon. The TERM signal (which is the default for the 'kill'
// command) is used to stop the entire server, including the besdaemon. The HUP
// signal is used to stop all beslisteners and then restart the master
// beslistener, forcing a re-read of the config file. Note that the daemon
// does not re-read the config file.

// When the daemon gets the HUP signal, it forwards that onto each beslistener.
// They then all exit, returning the 'restart' code so that the daemon knows
// to restart the master beslistener.
//
// Block sigchld when in this function.
static void CatchSigHup(int signal)
{
    if (signal == SIGHUP) {
        BESDEBUG("besdaemon", "besdaemon: caught SIGHUP in besdaemon." << endl);
        BESDEBUG("besdaemon", "besdaemon: sending SIGHUP to the process group: " << master_beslistener_pid << endl);

        // restart the beslistener(s); read their exit status
        stop_all_beslisteners(SIGHUP);

        if (start_master_beslistener() == 0) {
            cerr << "Could not restart the master beslistener." << endl;
            stop_all_beslisteners(SIGTERM);
            cleanup_resources();
            exit(1);
        }
    }
}

// When TERM (the default for 'kill') is sent to this process, send it also
// to each beslistener. This will cause the beslisteners to all exit with a zero
// value (the code for 'do not restart').
//
// Block sigchld when in this function.
static void CatchSigTerm(int signal)
{
    if (signal == SIGTERM) {
        BESDEBUG("besdaemon", "besdaemon: caught SIGTERM." << endl);
        BESDEBUG("besdaemon", "besdaemon: sending SIGTERM to the process group: " << master_beslistener_pid << endl);

        // Stop all of the beslistener(s); read their exit status
        stop_all_beslisteners(SIGTERM);

        cleanup_resources();

        // Once all the child exit status values are read, exit the daemon
        exit(0);
    }
}

/** Start the daemon command interpreter. This runs forever, until the daemon
 * is supposed to exit _unless_ the bes.conf file is configured to have the
 * command interpreter shut off. In that case other code in main() is used to
 * implement a simpler system where SIGTERM and SIGHUP are used to stop and
 * restart the daemon/beslisteners.
 *
 * @param handler This implements the ServerHandler instance used by the
 * daemon.
 * @return 1 if the interpreter exited and the daemon should stop, 0 if the
 * interpreter was not started.
 */
static int start_command_processor(DaemonCommandHandler &handler)
{
#if 0
    // These are now global so that start_master_beslistener() can close them
    // in the child process (which will become the master beslistener if all
    // goes well) before exec'ing the beslistener.
    TcpSocket *socket = 0;
    UnixSocket *unix_socket = 0;
    PPTServer *command_server = 0;
#endif

    BESDEBUG("besdaemon", "besdaemon: Starting command processor." << endl);

    try {
        SocketListener listener;

        string port_str;
        bool port_found;
        int port = 0;
        TheBESKeys::TheKeys()->get_value(DAEMON_PORT_STR, port_str, port_found);
        if (port_found) {
            char *ptr;
            port = strtol(port_str.c_str(), &ptr, 10);
            if (port == 0) {
                cerr << "Invalid port number for daemon command interface: " << port_str << endl;
                exit(1);
            }
        }

        if (port) {
            BESDEBUG("besdaemon", "besdaemon: listening on port: " << port << endl);
            my_socket = new TcpSocket(port);
            listener.listen(my_socket);
        }

        string usock_str;
        bool usock_found;
        TheBESKeys::TheKeys()->get_value(DAEMON_UNIX_SOCK_STR, usock_str, usock_found);

        if (!usock_str.empty()) {
            BESDEBUG("besdaemon", "besdaemon: listening on unix socket: " << usock_str << endl);
            unix_socket = new UnixSocket(usock_str);
            listener.listen(unix_socket);
        }

        if (!port_found && !usock_found) {
            BESDEBUG("besdaemon", "Neither a port nor a unix socket was set for the daemon command interface." << endl);
            return 0;
        }

        BESDEBUG("besdaemon", "besdaemon: starting command interface on port: " << port << endl);
        command_server = new PPTServer(&handler, &listener, /*is_secure*/false);

        // Once initialized, 'handler' loops until it's told to exit.
        command_server->initConnection();

        // Once the handler exits, close sockets and free memory
        command_server->closeConnection();
        delete command_server;
        command_server = 0;

        // delete closes the sockets
        delete my_socket;
        my_socket = 0;
        delete unix_socket;
        unix_socket = 0;

        // When/if the command interpreter exits, stop the all listeners.
        block_signals();
        stop_all_beslisteners(SIGTERM);
        unblock_signals();

        return 1;
    }
    catch (BESError &se) {
        cerr << "daemon: " << se.get_message() << endl;
        delete command_server;
        command_server = 0;
        delete my_socket;
        my_socket = 0;
        delete unix_socket;
        unix_socket = 0;

        block_signals();
        stop_all_beslisteners(SIGTERM);
        unblock_signals();
        return 1;
    }
    catch (...) {
        cerr << "daemon: " << "caught unknown exception" << endl;
        delete command_server;
        command_server = 0;
        delete my_socket;
        my_socket = 0;
        delete unix_socket;
        unix_socket = 0;

        block_signals();
        stop_all_beslisteners(SIGTERM);
        unblock_signals();
        return 1;
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
    struct sigaction act;

    // block chld, term and hup in the handlers
    sigemptyset(&act.sa_mask);
    sigaddset(&act.sa_mask, SIGCHLD);
    sigaddset(&act.sa_mask, SIGTERM);
    sigaddset(&act.sa_mask, SIGHUP);
    act.sa_flags = 0;
#ifdef SA_RESTART
    BESDEBUG("besdaemon" , "besdaemon: setting restart for sigchld." << endl);
    act.sa_flags |= SA_RESTART;
#endif

    act.sa_handler = CatchSigChild;
    if (sigaction(SIGCHLD, &act, 0)) {
        cerr << "Could not register a handler to catch beslistener status." << endl;
        exit(1);
    }

    act.sa_handler = CatchSigTerm;
    if (sigaction(SIGTERM, &act, 0) < 0) {
        cerr << "Could not register a handler to catch the terminate signal." << endl;
        exit(1);
    }

    act.sa_handler = CatchSigHup;
    if (sigaction(SIGHUP, &act, 0) < 0) {
        cerr << "Could not register a handler to catch the hang-up signal." << endl;
        exit(1);
    }
}

/** Make this process a daemon (a process with ppid of 1) and a session
 * leader. Note that code in the beslistener (ServerApp.cc) makes the master
 * beslistener a session leader too.
 *
 * @return -1 if the initial fork() call fails; 0 otherwise.
 */
static int daemon_init()
{
    pid_t pid;
    if ((pid = fork()) < 0) // error
        return -1;
    else if (pid != 0) // parent exits
        exit(0);
    setsid(); // child establishes its own process group
    return 0;
}

/** Given a value for the daemon's pid, store that in the file referenced by
 * the global 'file_for_daemon_pid'. This value is used by the shell scrip
 * 'besctl' to stop the daemon.
 *
 * @param pid The process ID of this daemon.
 */
static void store_daemon_id(int pid)
{
    ofstream f(file_for_daemon_pid.c_str());
    if (!f) {
        cerr << errno_str(": unable to create pid file " + file_for_daemon_pid + ": ");
    }
    else {
        f << "PID: " << pid << " UID: " << getuid() << endl;
        f.close();
        mode_t new_mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
        (void) chmod(file_for_daemon_pid.c_str(), new_mode);
    }
}

/** Given the installation directory and pid-file directory - either or both
 * may be empty - build the path to the pid file (stored in the global
 * file_for_daemon_pid and server name (stored in the global 'beslistener_path'). The
 * server name is really the full path to the beslistener.
 * @param install_dir Installation directory path
 * @param pid_dir pid file pathname
 * @return true if the server executable is found, false otherwise.
 */
static bool load_names(const string &install_dir, const string &pid_dir)
{
    string bindir = "/bin";
    if (!pid_dir.empty()) {
        file_for_daemon_pid = pid_dir;
    }

    if (!install_dir.empty()) {
        beslistener_path = install_dir;
        beslistener_path += bindir;
        if (file_for_daemon_pid.empty()) {
            file_for_daemon_pid = install_dir + "/var/run";
        }
    }
    else {
        string prog = daemon_name;
        string::size_type slash = prog.find_last_of('/');
        if (slash != string::npos) {
            beslistener_path = prog.substr(0, slash);
            slash = prog.find_last_of('/');
            if (slash != string::npos) {
                string root = prog.substr(0, slash);
                if (file_for_daemon_pid.empty()) {
                    file_for_daemon_pid = root + "/var/run";
                }
            }
            else {
                if (file_for_daemon_pid.empty()) {
                    file_for_daemon_pid = beslistener_path;
                }
            }
        }
    }

    if (beslistener_path == "") {
        beslistener_path = ".";
        if (file_for_daemon_pid.empty()) {
            file_for_daemon_pid = "./run";
        }
    }

    beslistener_path += BES_SERVER;
    file_for_daemon_pid += BES_SERVER_PID;

    if (access(beslistener_path.c_str(), F_OK) != 0) {
        cerr << daemon_name << ": cannot find " << beslistener_path << endl << "Please either pass -i <install_dir> on the command line." << endl;
        return false;
    }

    // Record the name for use when building the arg list for the beslistener
    global_args["beslistener"] = beslistener_path;

    return true;
}

/** Run the daemon.

 */
int main(int argc, char *argv[])
{
#ifndef BES_DEVELOPER
    // must be root to run this app and to set user id and group id later
    uid_t curr_euid = geteuid();
    if (curr_euid) {
        cerr << "FAILED: Must be root to run BES" << endl;
        exit(SERVER_EXIT_FATAL_CAN_NOT_START);
    }
#else
    cerr << "Developer Mode: not testing if BES is run by root" << endl;
#endif

    daemon_name = argv[0];

    string install_dir;
    string pid_dir;

    // there are 16 arguments allowed to the daemon, including the program
    // name. 3 options do not have arguments and 6 have arguments
    if (argc > 16) {
        // the show_usage method exits
        BESServerUtils::show_usage(daemon_name);
    }

    // Most of the argument processing is just for vetting the arguments
    // that will be passed onto the beslistener(s), but we do grab some info
    string config_file = "";
    // argv[0] is the name of the program, so start num_args at 1
    unsigned short num_args = 1;
    // If you change the getopt statement below, be sure to make the
    // corresponding change in ServerApp.cc and besctl.in
    int c = 0;
    while ((c = getopt(argc, argv, "hvsd:c:p:u:i:r:")) != EOF) {
        switch (c) {
            case 'v': // version
                BESServerUtils::show_version(daemon_name);
                break;
            case '?': // unknown option
            case 'h': // help
                BESServerUtils::show_usage(daemon_name);
                break;
            case 'i': // BES install directory
                install_dir = optarg;
                if (BESScrub::pathname_ok(install_dir, true) == false) {
                    cout << "The specified install directory (-i option) " << "is incorrectly formatted. Must be less than " << "255 characters and include the characters " << "[0-9A-z_./-]" << endl;
                    return 1;
                }
                global_args["-i"] = install_dir;
                num_args += 2;
                break;
            case 's': // secure server
                global_args["-s"] = "";
                num_args++;
                break;
            case 'r': // where to write the pid file
                pid_dir = optarg;
                if (BESScrub::pathname_ok(pid_dir, true) == false) {
                    cout << "The specified state directory (-r option) " << "is incorrectly formatted. Must be less than " << "255 characters and include the characters " << "[0-9A-z_./-]" << endl;
                    return 1;
                }
                global_args["-r"] = pid_dir;
                num_args += 2;
                break;
            case 'c': // configuration file
                config_file = optarg;
                if (BESScrub::pathname_ok(config_file, true) == false) {
                    cout << "The specified configuration file (-c option) " << "is incorrectly formatted. Must be less than " << "255 characters and include the characters " << "[0-9A-z_./-]" << endl;
                    return 1;
                }
                global_args["-c"] = config_file;
                num_args += 2;
                break;
            case 'u': // unix socket
            {
                string check_path = optarg;
                if (BESScrub::pathname_ok(check_path, true) == false) {
                    cout << "The specified unix socket (-u option) " << "is incorrectly formatted. Must be less than " << "255 characters and include the characters " << "[0-9A-z_./-]" << endl;
                    return 1;
                }
                global_args["-u"] = check_path;
                num_args += 2;
                break;
            }
            case 'p': // TCP port
            {
                string port_num = optarg;
                for (unsigned int i = 0; i < port_num.length(); i++) {
                    if (!isdigit(port_num[i])) {
                        cout << "The specified port contains non-digit " << "characters: " << port_num << endl;
                        return 1;
                    }
                }
                global_args["-p"] = port_num;
                num_args += 2;
            }
                break;
            case 'd': // debug
            {
                string check_arg = optarg;
                if (BESScrub::command_line_arg_ok(check_arg) == false) {
                    cout << "The specified debug options \"" << check_arg << "\" contains invalid characters" << endl;
                    return 1;
                }
                BESDebug::SetUp(check_arg);
                global_args["-d"] = check_arg;
                debug_sink = check_arg.substr(0, check_arg.find(','));
                num_args += 2;
                break;
            }
            default:
                BESServerUtils::show_usage(daemon_name);
                break;
        }
    }

    // if the number of arguments is greater than the number of allowed arguments
    // then extra arguments were passed that aren't options. Show usage and
    // exit.
    if (argc > num_args) {
        cout << daemon_name << ": too many arguments passed to the BES";
        BESServerUtils::show_usage(daemon_name);
    }

    if (pid_dir.empty()) {
        pid_dir = install_dir;
    }

    // If the -c option was passed, set the config file name in TheBESKeys
    if (!config_file.empty()) {
        TheBESKeys::ConfigFile = config_file;
    }

    // If the -c option was not passed, but the -i option
    // was passed, then use the -i option to construct
    // the path to the config file
    if (install_dir.empty() && !install_dir.empty()) {
        if (install_dir[install_dir.length() - 1] != '/') {
            install_dir += '/';
        }
        string conf_file = install_dir + "etc/bes/bes.conf";
        TheBESKeys::ConfigFile = conf_file;
    }

    // Set the name of the listener and the file for the listener pid
    if (!load_names(install_dir, pid_dir))
        return 1;

    if (!access(file_for_daemon_pid.c_str(), F_OK)) {
        ifstream temp(file_for_daemon_pid.c_str());
        cout << daemon_name << ": there seems to be a BES daemon already running at ";
        char buf[500];
        temp.getline(buf, 500);
        cout << buf << endl;
        temp.close();
        return 1;
    }

    daemon_init();

    register_signal_handlers();

    // Load the modules in the conf file(s) so that the debug (log) contexts
    // will be available to the BESDebug singleton so we can tell the OLFS/HAI
    // about them. Then Register the 'besdaemon' context.
    BESModuleApp app;
    if (app.initialize(argc, argv) != 0) {
        cerr << "Could not initialize the modules to get the log contexts." << endl;
    }
    BESDebug::Register( "besdaemon" ) ;

    // These are from the beslistener - they are valid contexts but are not
    // registered by a module. See ServerApp.cc
    BESDebug::Register("server");
    BESDebug::Register("ppt");

    // The stuff in global_args is used whenever a call to start_master_beslistener()
    // is made, so any time the BESDebug contexts are changed, a change to the
    // global_args will change the way the the beslistener is started. In fact,
    // it's not limited to the debug stuff, but that's we're using it for now.
    // jhrg 6/16/11

    // The -d option was not given; add one setting up a default log sink using
    // the log file from the bes.conf file or the name "LOG".
    if (global_args.count("-d") == 0)
    {
        bool found = false;
        // string log_file_name;
        TheBESKeys::TheKeys()->get_value("BES.LogName", debug_sink, found);
        if (!found)
        {
            // This is a crude fallback that avoids a value without any name
            // for a log file (which would be a syntax error).
            global_args["-d"] = "LOG," + BESDebug::GetOptionsString();
        }
        else
        {
            // I use false for the 'created' flag so that subsequent changes to the
            // debug stream won't do odd things like delete the ostream pointer.
            // Note that the beslistener has to recognize that "LOG" means to use
            // the bes.log file for a debug/log sink
            BESDebug::SetStrm(BESLog::TheLog()->get_log_ostream(), false) ;

            global_args["-d"] = debug_sink + "," + BESDebug::GetOptionsString();
        }
    }
    // The option was given; use the token read from the options for the sink
    // so that the beslistener will open the correct thing.
    else
    {
        global_args["-d"] = debug_sink + "," + BESDebug::GetOptionsString();
    }

    // master_beslistener_pid is global so that the signal handlers can use it;
    // it is actually assigned a value in start_master_beslistener but it's
    // assigned here to make it clearer what's going on.
    master_beslistener_pid = start_master_beslistener();
    if (master_beslistener_pid == 0) {
        cerr << daemon_name << ": server cannot mount at first try (core dump). " << "Please correct problems on the process manager " << beslistener_path << endl;
        return master_beslistener_pid;
    }

    store_daemon_id(getpid());

    BESDEBUG("besdaemon", "besdaemon: master_beslistener_pid: " << master_beslistener_pid << endl);

    // start_command_processor() does not return unless all commands have been
    // processed and the daemon has been told to exit (status == 1) or the
    // bes.conf file was set so that the processor never starts (status == 0).
    DaemonCommandHandler handler(TheBESKeys::ConfigFile);
    int status = start_command_processor(handler);

    // if the command processor does not start, drop into this loop which
    // implements the simple restart-on-HUP behavior of the daemon.
    if (status == 0) {
        bool done = false;
        while (!done) {
            pause();
            BESDEBUG("besdaemon", "besdaemon: master_beslistener_status: " << master_beslistener_status << endl);
            if (master_beslistener_status == BESLISTENER_RESTART) {
                master_beslistener_status = BESLISTENER_STOPPED;
                // master_beslistener_pid = start_master_beslistener();
                start_master_beslistener();
            }
            // If the satus is not 'restart' and not running, then exit loop
            else if (master_beslistener_status != BESLISTENER_RUNNING) {
                done = true;
            }
        }
    }

    BESDEBUG("besdaemon", "besdaemon: past the command processor start" << endl);

    cleanup_resources();

    return status;
}

