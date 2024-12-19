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

#include <unistd.h>  // for getopt fork setsid execvp access geteuid

#include <grp.h>    // for getgrnam
#include <pwd.h>    // for getpwnam

#include <sys/wait.h>  // for waitpid
#include <sys/stat.h>  // for chmod
#include <cctype> // for isdigit
#include <csignal>

#include <iostream>
#include <string>
#include <sstream>
#include <cstring>
#include <cstdlib>
#include <cerrno>
#include <map>
#include <vector>

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
#include "BESUtil.h"

#define BES_SERVER "/beslistener"
#define BES_SERVER_PID "/bes.pid"
#define DAEMON_PORT_STR "BES.DaemonPort"
#define DAEMON_UNIX_SOCK_STR "BES.DaemonUnixSocket"

using namespace std;

// Defined in setgroups.c
extern "C" int set_sups(int target_sups_size, gid_t* target_sups_list);

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
volatile int master_beslistener_status = BESLISTENER_STOPPED;
#if 0
volatile int num_children = 0;
#endif
static volatile int master_beslistener_pid = -1; // This is also the process group id

typedef map<string, string> arg_map;
static arg_map global_args;
static string debug_sink;

static TcpSocket *my_socket = nullptr;
static UnixSocket *unix_socket = nullptr;
static PPTServer *command_server = nullptr;

// These are set to 1 by their respective handlers and then processed in the
// signal processing loop. jhrg 3/5/14
static volatile sig_atomic_t sigchild = 0;
static volatile sig_atomic_t sigterm = 0;
static volatile sig_atomic_t sighup = 0;

static string errno_str(const string &msg)
{
    ostringstream oss;
    oss << daemon_name << msg;
    const char *perror_string = strerror(errno);
    if (perror_string) oss << perror_string;
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
    if (WIFEXITED(status)) {
        switch (WEXITSTATUS(status)) {
        case SERVER_EXIT_NORMAL_SHUTDOWN:
            return 0;

        case SERVER_EXIT_FATAL_CANNOT_START:
            cerr << daemon_name << ": server cannot start, exited with status " << WEXITSTATUS(status) << endl;
            cerr << "Please check all error messages " << "and adjust server installation" << endl;
            return 1;

        case SERVER_EXIT_ABNORMAL_TERMINATION:
            cerr << daemon_name << ": abnormal server termination, exited with status " << WEXITSTATUS(status) << endl;
            return 1;

        case SERVER_EXIT_RESTART:
            cerr << daemon_name << ": server has been requested to re-start." << endl;
            return SERVER_EXIT_RESTART;

        default:
            return 1;
        }
    }
    else if (WIFSIGNALED(status)) {
        cerr << daemon_name << ": abnormal server termination, signaled with signal number " << WTERMSIG(status)
            << endl;
#ifdef WCOREDUMP
        if (WCOREDUMP(status)) {
            cerr << daemon_name << ": server dumped core." << endl;
            return 1;
        }
#endif
        return 1;
    }
    else if (WIFSTOPPED(status)) {
        cerr << daemon_name << ": abnormal server termination, stopped with signal number " << WSTOPSIG(status) << endl;
        return 1;
    }

    return 0;
}

/** For code that must use signals to stop and start the master listener,
 * block signals being delivered to this process. The code must call the
 * companion unblock_signals() when it is done.
 */
void block_signals()
{
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGCHLD);
    sigaddset(&set, SIGHUP);
    sigaddset(&set, SIGTERM);

    if (sigprocmask(SIG_BLOCK, &set, nullptr) < 0) {
        cerr << errno_str(": sigprocmask error, blocking signals in stop_all_beslisteners ");
    }
}

/** See block_signals() */
void unblock_signals()
{
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGCHLD);
    sigaddset(&set, SIGHUP);
    sigaddset(&set, SIGTERM);

    if (sigprocmask(SIG_UNBLOCK, &set, nullptr) < 0) {
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

    BESDEBUG("besdaemon", "besdaemon: master_beslistener_pid " << master_beslistener_pid << endl);
    // Send 'sig' to all members of the process group with/of the master bes.
    // The master beslistener pid is the group id of all the beslisteners.
    int status = killpg(master_beslistener_pid, sig);
    switch (status) {
    case EINVAL:
        cerr << "The sig argument is not a valid signal number." << endl;
        break;

    case EPERM:
        cerr
            << "The sending process is not the super-user and one or more of the target processes has an effective user ID different from that of the sending process."
            << endl;
        break;

    case ESRCH:
        cerr << "No process can be found in the process group specified by the process group ("
            << master_beslistener_pid << ")." << endl;
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
            BESDEBUG("besdaemon",
                "besdaemon: caught master beslistener: " << pid << " status: " << master_beslistener_status << endl);
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
    char **arguments = new char*[global_args.size() * 2 + 1];

    // Marshal the arguments to the listener from the command line
    // arguments to the daemon
    arguments[0] = strdup(global_args["beslistener"].c_str());

    int i = 1;
    arg_map::iterator it;
    for (it = global_args.begin(); it != global_args.end(); ++it) {
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
        }
        else if ((*it).first != "beslistener") {
            arguments[i++] = strdup((*it).first.c_str());
            arguments[i++] = strdup((*it).second.c_str());
        }
    }
    arguments[i] = nullptr;       // terminal null

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
    int pipefd[2];
    if (pipe(pipefd) < 0) {
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

        close(pipefd[0]); // Close the read end of the pipe in the child

        // dup2 so we know the FD to write to in the child (the beslistener).
        // BESLISTENER_PIPE_FD is '1' which is stdout; since beslistener is a
        // daemon process both stdin and out have been closed so these descriptors
        // are available. Using higher numbers can cause problems (see ticket
        // 1783). jhrg 7/15/11
        if (dup2(pipefd[1], MASTER_TO_DAEMON_PIPE_FD) != MASTER_TO_DAEMON_PIPE_FD) {
            cerr << errno_str(": dup2 error ");
            return 0;
        }

        // We don't have to free this because this is a different process
        // than the parent.
        char **arguments = update_beslistener_args();

        BESDEBUG("besdaemon", "Starting: " << arguments[0] << endl);

        // Close the socket for the besdaemon here. This keeps it from being
        // passed into the master beslistener and then entering the state
        // CLOSE_WAIT once the besdaemon's client closes its end.
        if (command_server) command_server->closeConnection();

        // This is where beslistener - the master listener - is started
        execvp(arguments[0], arguments);

        // if we are still here, it's an error...
        cerr << errno_str(": mounting listener, subprocess failed: ");
        exit(1); //NB: This exits from the child process.
    }

    // parent process (the besdaemon)

    // The daemon records the pid of the master beslistener, but only does so
    // when that process writes its status to the pipe 'fd'.

    close(pipefd[1]); // close the write end of the pipe in the parent.

    BESDEBUG("besdaemon", "besdaemon: master beslistener pid: " << pid << endl);

    // Read the status from the child (beslistener).
    int beslistener_start_status;
    long status = read(pipefd[0], &beslistener_start_status, sizeof(beslistener_start_status));

    if (status < 0) {
        cerr << "Could not read master beslistener status; the master pid was not changed." << endl;
        close(pipefd[0]);
        return 0;
    }
    else if (beslistener_start_status != BESLISTENER_RUNNING) {
        cerr << "The beslistener status is not 'BESLISTENER_RUNNING' (it is '" << beslistener_start_status
            << "')  the master pid was not changed." << endl;
        close(pipefd[0]);
        return 0;
    }
    else {
        BESDEBUG("besdaemon", "besdaemon: master beslistener start status: " << beslistener_start_status << endl);
        // Setting master_beslistener_pid here and not forcing callers to use the
        // return value means that this global can be local to this file.
        master_beslistener_pid = pid;
        master_beslistener_status = BESLISTENER_RUNNING;
    }

    close(pipefd[0]);
    return pid;
}

/** Clean up resources allocated by main(). This is called both by main() and
 *  by the SIGTERM signal handler.
 */
static void cleanup_resources()
{
    // TOCTOU error. Since the code ignores the error code from
    // remove(), we might as well drop the test. We could test for an
    // error and print a warning to the log... jhrg 10/23/15
#if 0
    if (!access(file_for_daemon_pid.c_str(), F_OK)) {
        (void) remove(file_for_daemon_pid.c_str());
    }
#endif

    (void) remove(file_for_daemon_pid.c_str());
}

// Note that SIGCHLD, SIGTERM and SIGHUP are blocked while in these three
// signal handlers below.

static void catch_sig_child(int signal)
{
    if (signal == SIGCHLD) {
        sigchild = 1;
    }
}

static void catch_sig_hup(int signal)
{
    if (signal == SIGHUP) {
        sighup = 1;
    }
}

static void catch_sig_term(int signal)
{
    if (signal == SIGTERM) {
        sigterm = 1;
    }
}

static void process_signals()
{
    block_signals();

    // Process SIGCHLD. This is used to detect if the HUP signal was sent to the
    // master listener and it has returned SERVER_EXIT_RESTART by recording
    // that value in the global 'master_beslistener_status'. Other code needs
    // to test that (static) global to see if the beslistener should be restarted.
    if (sigchild) {
        int status;
        int pid = wait(&status);

        // Decode and record the exit status, but only if it really is the
        // master beslistener this daemon is using. If two or more Start commands
        // are sent in a row, a master beslistener will start, fail to bind to
        // the port (because another master beslstener is already bound to it)
        // and exit. We don't want to record that second process's exit status here.
        if (pid == master_beslistener_pid) master_beslistener_status = pr_exit(status);

        sigchild = 0;
    }

    // The two following signals implement a simple stop/restart behavior
    // for the daemon. The TERM signal (which is the default for the 'kill'
    // command) is used to stop the entire server, including the besdaemon. The HUP
    // signal is used to stop all beslisteners and then restart the master
    // beslistener, forcing a re-read of the config file. Note that the daemon
    // does not re-read the config file.

    // When the daemon gets the HUP signal, it forwards that to each beslistener.
    // They then all exit, returning the 'restart' code so that the daemon knows
    // to restart the master beslistener.
    if (sighup) {
        // restart the beslistener(s); read their exit status
        stop_all_beslisteners(SIGHUP);

        // FIXME jhrg 3/5/14
        if (start_master_beslistener() == 0) {
            cerr << "Could not restart the master beslistener." << endl;
            stop_all_beslisteners(SIGTERM);
            cleanup_resources();
            exit(1);
        }

        sighup = 0;
    }

    // When TERM (the default for 'kill') is sent to this process, send it also
    // to each beslistener. This will cause the beslisteners to all exit with a zero
    // value (the code for 'do not restart').
    if (sigterm) {
        // Stop all the beslistener(s); read their exit status
        stop_all_beslisteners(SIGTERM);

        // FIXME jhrg 3/5/14
        cleanup_resources();
        // Once all the child exit status values are read, exit the daemon
        exit(0);
    }

    unblock_signals();
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
    BESDEBUG("besdaemon", "besdaemon: Starting command processor." << endl);

    try {
        SocketListener listener;

        string port_str;
        bool port_found;
        int port = 0;
        TheBESKeys::TheKeys()->get_value(DAEMON_PORT_STR, port_str, port_found);
        if (port_found) {
            port = std::stoi(port_str, nullptr, 10);
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
        while (true) {
            process_signals();

            command_server->initConnection();
        }
    }
    catch (BESError &se) {
        cerr << "daemon: " << se.get_message() << endl;
    }
    catch (...) {
        cerr << "daemon: " << "caught unknown exception" << endl;
    }

    // Once the handler exits, close sockets and free memory
    command_server->closeConnection();

    delete command_server;
    command_server = nullptr;

    // delete closes the sockets
    delete my_socket;
    my_socket = nullptr;
    delete unix_socket;
    unix_socket = nullptr;

    // When/if the command interpreter exits, stop the all listeners.
    stop_all_beslisteners(SIGTERM);

    return 1;
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

    // block child, term and hup in the handlers
    sigemptyset(&act.sa_mask);
    sigaddset(&act.sa_mask, SIGCHLD);
    sigaddset(&act.sa_mask, SIGTERM);
    sigaddset(&act.sa_mask, SIGHUP);
    act.sa_flags = 0;
#ifdef SA_RESTART
    BESDEBUG("besdaemon", "besdaemon: setting restart for sigchld." << endl);
    act.sa_flags |= SA_RESTART;
#endif

    act.sa_handler = catch_sig_child;
    if (sigaction(SIGCHLD, &act, 0)) {
        cerr << "Could not register a handler to catch beslistener status." << endl;
        exit(1);
    }

    act.sa_handler = catch_sig_term;
    if (sigaction(SIGTERM, &act, 0) < 0) {
        cerr << "Could not register a handler to catch the terminate signal." << endl;
        exit(1);
    }

    act.sa_handler = catch_sig_hup;
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
 * the global 'file_for_daemon_pid'. This value is used by the shell script
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
        f << pid << endl;
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
            // Added jhrg 2/9/12 ... and removed 1/31/19. The special dir breaks
            // systemctl/systemd on CentOS 7. We might be able to tweak things so
            // it would work, but I'm switching back to what other daemons do. jhrg
            // file_for_daemon_pid = install_dir + "/var/run/bes";

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
                    // Added jhrg 2/9/12. See about 1/31/19 jhrg
                    // file_for_daemon_pid = root + "/var/run/bes";
                }
            }
            else {
                if (file_for_daemon_pid.empty()) {
                    file_for_daemon_pid = beslistener_path;
                }
            }
        }
    }

    if (beslistener_path.empty()) {
        beslistener_path = ".";
        if (file_for_daemon_pid.empty()) {
            file_for_daemon_pid = "./run";
        }
    }

    beslistener_path += BES_SERVER;
    file_for_daemon_pid += BES_SERVER_PID;

    if (access(beslistener_path.c_str(), F_OK) != 0) {
        cerr << daemon_name << ": cannot find " << beslistener_path << endl
            << "Please either pass -i <install_dir> on the command line." << endl;
        return false;
    }

    // Record the name for use when building the arg list for the beslistener
    global_args["beslistener"] = beslistener_path;

    return true;
}

static void set_group_id()
{
#if !defined(OS2) && !defined(TPF)
    // OS/2 and TPF don't support groups.

    // get group id or name from BES configuration file
    // If BES.Group begins with # then it is a group id,
    // else it is a group name and look up the id.
    BESDEBUG("server", "beslistener: Setting group id ... " << endl);
    bool found = false;
    string key = "BES.Group";
    string group_str;
    try {
        TheBESKeys::TheKeys()->get_value(key, group_str, found);
    }
    catch (BESError &e) {
        BESDEBUG("server", "beslistener: FAILED" << endl);
        string err = string("FAILED: ") + e.get_message();
        cerr << err << endl;
        ERROR_LOG(err);
        exit(SERVER_EXIT_FATAL_CANNOT_START);
    }

    if (!found || group_str.empty()) {
        BESDEBUG("server", "beslistener: FAILED" << endl);
        string err = "FAILED: Group not specified in BES configuration file";
        cerr << err << endl;
        ERROR_LOG(err);
        exit(SERVER_EXIT_FATAL_CANNOT_START);
    }
    BESDEBUG("server", "to " << group_str << " ... " << endl);

    gid_t new_gid = 0;
    if (group_str[0] == '#') {
        // group id starts with a #, so is a group id
        const char *group_c = group_str.c_str();
        group_c++;
        new_gid = atoi(group_c);
    }
    else {
        // specified group is a group name
#if 0
        struct group *ent;
        // FIXME replace getgrname() and getpwnam() with the _r versions. jhrg 8/11/21
        ent = getgrnam(group_str.c_str());
#endif
        struct group in;
        struct group *result = nullptr;
        vector<char> buffer(1024);
        int rc = getgrnam_r(group_str.c_str(), &in, buffer.data(), buffer.size(), &result);
        if (rc != 0 || result == nullptr) {
            BESDEBUG("server", "beslistener: FAILED" << endl);
            string err = string( "FAILED: Group ") + group_str + " does not exist (" + strerror(errno) + ").";
            cerr << err << endl;
            ERROR_LOG(err);
            exit(SERVER_EXIT_FATAL_CANNOT_START);
        }
        new_gid = result->gr_gid;
    }

    if (new_gid < 1) {
        BESDEBUG("server", "beslistener: FAILED" << endl);
        ostringstream err;
        err << "FAILED: Group id " << new_gid << " not a valid group id for BES";
        cerr << err.str() << endl;
        ERROR_LOG(err.str());
        exit(SERVER_EXIT_FATAL_CANNOT_START);
    }

    BESDEBUG("server", "to id " << new_gid << " ... " << endl);
    if (setgid(new_gid) == -1) {
        BESDEBUG("server", "beslistener: FAILED" << endl);
        ostringstream err;
        err << "FAILED: unable to set the group id to " << new_gid;
        cerr << err.str() << endl;
        ERROR_LOG(err.str());
        exit(SERVER_EXIT_FATAL_CANNOT_START);
    }

    BESDEBUG("server", "OK" << endl);
#else
    BESDEBUG( "server", "beslistener: Groups not supported in this OS" << endl );
#endif
}

static void set_user_id()
{
    BESDEBUG("server", "beslistener: Setting user id ... " << endl);

    // Get user name or id from the BES configuration file.
    // If the BES.User value begins with # then it is a user
    // id, else it is a user name and need to look up the
    // user id.
    bool found = false;
    string key = "BES.User";
    string user_str;
    try {
        TheBESKeys::TheKeys()->get_value(key, user_str, found);
    }
    catch (BESError &e) {
        BESDEBUG("server", "beslistener: FAILED" << endl);
        string err =  "FAILED: " + e.get_message();
        cerr << err << endl;
        ERROR_LOG(err);
        exit(SERVER_EXIT_FATAL_CANNOT_START);
    }

    if (!found || user_str.empty()) {
        BESDEBUG("server", "beslistener: FAILED" << endl);
        auto err = "FAILED: User not specified in BES config file";
        cerr << err << endl;
        ERROR_LOG(err);
        exit(SERVER_EXIT_FATAL_CANNOT_START);
    }
    BESDEBUG("server", "to " << user_str << " ... " << endl);

    uid_t new_id = 0;
    if (user_str[0] == '#') {
        const char *user_str_c = user_str.c_str();
        user_str_c++;
        new_id = atoi(user_str_c);
    }
    else {
#if 0
        struct passwd *ent;
        ent = getpwnam(user_str.c_str());
#endif

        struct passwd in;
        struct passwd *result = nullptr;
        vector<char> buffer(1024);
        int rc = getpwnam_r(user_str.c_str(), &in, buffer.data(), buffer.size(), &result);
        if (rc != 0 || result == nullptr) {
            BESDEBUG("server", "beslistener: FAILED" << endl);
            string err = (string) "FAILED: Bad user name specified: " + user_str + "(" + strerror(errno) + ").";
            cerr << err << endl;
            ERROR_LOG(err);
            exit(SERVER_EXIT_FATAL_CANNOT_START);
        }
        new_id = result->pw_uid;
    }

    // new user id cannot be root (0)
    if (!new_id) {
        BESDEBUG("server", "beslistener: FAILED" << endl);
        auto err = (string) "FAILED: BES cannot run as root";
        cerr << err << endl;
        ERROR_LOG(err);
        exit(SERVER_EXIT_FATAL_CANNOT_START);
    }

    // Right before we relinquish root, remove any 'supplementary groups'
    //int set_sups(const int target_sups_size, const gid_t* const target_sups_list)
    vector<gid_t> groups(1);
    groups.at(0) = getegid();
    if (set_sups(groups.size(), groups.data()) == -1) {
        BESDEBUG("server", "beslistener: FAILED" << endl);
        ostringstream err;
        err << "FAILED: Unable to relinquish supplementary groups (" << new_id << ")";
        cerr << err.str() << endl;
        ERROR_LOG(err.str());
        exit(SERVER_EXIT_FATAL_CANNOT_START);
    }

    BESDEBUG("server", "to " << new_id << " ... " << endl);
    if (setuid(new_id) == -1) {
        BESDEBUG("server", "beslistener: FAILED" << endl);
        ostringstream err;
        err << "FAILED: Unable to set user id to " << new_id;
        cerr << err.str() << endl;
        ERROR_LOG(err.str());
        exit(SERVER_EXIT_FATAL_CANNOT_START);
    }

    BESDEBUG("server", "OK" << endl);
}

/**
 * Run the daemon.
 */
int main(int argc, char *argv[])
{
    uid_t curr_euid = geteuid();

#ifndef BES_DEVELOPER
    // must be root to run this app and to set user id and group id later
    if (curr_euid) {
        cerr << "FAILED: Must be root to run BES" << endl;
        exit(SERVER_EXIT_FATAL_CANNOT_START);
    }
#else
    cerr << "Developer Mode: Not testing if BES is run by root" << endl;
#endif

    daemon_name = "besdaemon";

    string install_dir;
    string pid_dir;

    bool become_daemon = true;

    // there are 16 arguments allowed to the daemon, including the program
    // name. 3 options do not have arguments and 6 have arguments
    if (argc > 16) {
        // the show_usage method exits
        BESServerUtils::show_usage(daemon_name);
    }

    try {
        // Most of the argument processing is just for vetting the arguments
        // that will be passed onto the beslistener(s), but we do grab some info
        string config_file;
        // argv[0] is the name of the program, so start num_args at 1
        unsigned short num_args = 1;

        // If you change the getopt statement below, be sure to make the
        // corresponding change in ServerApp.cc and besctl.in
        int c = 0;
        while ((c = getopt(argc, argv, "hvsd:c:p:u:i:r:n")) != -1) {
            switch (c) {
            case 'v': // version
                BESServerUtils::show_version(daemon_name);
                break;
            case '?': // unknown option
            case 'h': // help
                BESServerUtils::show_usage(daemon_name);
               break;
            case 'n': // no-daemon (Do Not Become A daemon process)
                become_daemon=false;
                cerr << "Running in foreground!" << endl;
                num_args++;
                break;
            case 'i': // BES install directory
                install_dir = optarg;
                if (!BESScrub::pathname_ok(install_dir, true)) {
                    cout << "The specified install directory (-i option) "
                        << "is incorrectly formatted. Must be less than "
                        << "255 characters and include the characters " << "[0-9A-z_./-]" << endl;
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
                if (!BESScrub::pathname_ok(pid_dir, true)) {
                    cout << "The specified state directory (-r option) "
                        << "is incorrectly formatted. Must be less than "
                        << "255 characters and include the characters " << "[0-9A-z_./-]" << endl;
                    return 1;
                }
                global_args["-r"] = pid_dir;
                num_args += 2;
                break;
            case 'c': // configuration file
                config_file = optarg;
                if (!BESScrub::pathname_ok(config_file, true)) {
                    cout << "The specified configuration file (-c option) "
                        << "is incorrectly formatted. Must be less than "
                        << "255 characters and include the characters " << "[0-9A-z_./-]" << endl;
                    return 1;
                }
                global_args["-c"] = config_file;
                num_args += 2;
                break;
            case 'u': // unix socket
            {
                string check_path = optarg;
                if (!BESScrub::pathname_ok(check_path, true)) {
                    cout << "The specified unix socket (-u option) " << "is incorrectly formatted. Must be less than "
                        << "255 characters and include the characters " << "[0-9A-z_./-]" << endl;
                    return 1;
                }
                global_args["-u"] = check_path;
                num_args += 2;
                break;
            }
            case 'p': // TCP port
            {
                string port_num = optarg;
                for (unsigned int i = 0; i < port_num.size(); i++) {
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
                if (!BESScrub::command_line_arg_ok(check_arg)) {
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
        if (config_file.empty() && !install_dir.empty()) {
            BESUtil::trim_if_trailing_slash(install_dir);
            string conf_file = install_dir + "etc/bes/bes.conf";
            TheBESKeys::ConfigFile = conf_file;
        }
    }
    catch (BESError &e) {
        // (*BESLog::TheLog())
        // BESLog::TheLog throws exceptions...
        cerr << "Caught BES Error while processing the daemon's options: " << e.get_message() << endl;
        return 1;
    }
    catch (const std::exception &e) {
        cerr << "Caught C++ error while processing the daemon's options: " << e.what() << endl;
        return 2;
    }
    catch (...) {
        cerr << "Caught unknown error while processing the daemon's options." << endl;
        return 3;
    }

    try {
        // Set the name of the listener and the file for the listener pid
        if (!load_names(install_dir, pid_dir)) return 1;

        if (!access(file_for_daemon_pid.c_str(), F_OK)) {
            ifstream temp(file_for_daemon_pid.c_str());
            cout << daemon_name << ": there seems to be a BES daemon already running at ";
            char buf[500];
            temp.getline(buf, 500);
            cout << buf << endl;
            temp.close();
            return 1;
        }

        if(become_daemon){
            daemon_init();
        }

        store_daemon_id(getpid());

        if (curr_euid == 0) {
#ifdef BES_DEVELOPER
            cerr << "Developer Mode: Running as root - setting group and user ids" << endl;
#endif
            set_group_id();
            set_user_id();
        }
        else {
            cerr << "Developer Mode: Not setting group or user ids" << endl;
        }

        register_signal_handlers();

        // Load the modules in the conf file(s) so that the debug (log) contexts
        // will be available to the BESDebug singleton so we can tell the OLFS/HAI
        // about them. Then Register the 'besdaemon' context.
        BESModuleApp app;
        if (app.initialize(argc, argv) != 0) {
            cerr << "Could not initialize the modules to get the log contexts." << endl;
        }
        BESDebug::Register("besdaemon");

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
        if (global_args.count("-d") == 0) {
            bool found = false;
            TheBESKeys::TheKeys()->get_value("BES.LogName", debug_sink, found);
            if (!found) {
                // This is a crude fallback that avoids a value without any name
                // for a log file (which would be a syntax error).
                global_args["-d"] = "cerr," + BESDebug::GetOptionsString();
            }
            else {
                // I use false for the 'created' flag so that subsequent changes to the
                // debug stream won't do odd things like delete the ostream pointer.
                // Note that the beslistener has to recognize that "LOG" means to use
                // the bes.log file for a debug/log sink
                BESDebug::SetStrm(BESLog::TheLog()->get_log_ostream(), false);

                global_args["-d"] = debug_sink + "," + BESDebug::GetOptionsString();
            }
        }
        // The option was given; use the token read from the options for the sink
        // so that the beslistener will open the correct thing.
        else {
            global_args["-d"] = debug_sink + "," + BESDebug::GetOptionsString();
        }

        // master_beslistener_pid is global so that the signal handlers can use it;
        // it is actually assigned a value in start_master_beslistener but it's
        // assigned here to make it clearer what's going on.
        master_beslistener_pid = start_master_beslistener();
        if (master_beslistener_pid == 0) {
            cerr << daemon_name << ": server cannot mount at first try (core dump). "
                << "Please correct problems on the process manager " << beslistener_path << endl;
            return master_beslistener_pid;
        }

        BESDEBUG("besdaemon", "besdaemon: master_beslistener_pid: " << master_beslistener_pid << endl);
    }
    catch (BESError &e) {
        cerr << "Caught BES Error during initialization: " << e.get_message() << endl;
        return 1;
    }
    catch (const std::exception &e) {
        cerr << "Caught C++ error during initialization: " << e.what() << endl;
        return 2;
    }
    catch (...) {
        cerr << "Caught unknown error during initialization." << endl;
        return 3;
    }

    int status = 0;
    try {
        // start_command_processor() does not return unless all commands have been
        // processed and the daemon has been told to exit (status == 1) or the
        // bes.conf file was set so that the processor never starts (status == 0).
        DaemonCommandHandler handler(TheBESKeys::ConfigFile);
        status = start_command_processor(handler);

        // if the command processor does not start, drop into this loop which
        // implements the simple restart-on-HUP behavior of the daemon.
        if (status == 0) {
            bool done = false;
            while (!done) {
                pause();

                process_signals();

                BESDEBUG("besdaemon", "besdaemon: master_beslistener_status: " << master_beslistener_status << endl);
                if (master_beslistener_status == BESLISTENER_RESTART) {
                    master_beslistener_status = BESLISTENER_STOPPED;
                    // master_beslistener_pid = start_master_beslistener();
                    start_master_beslistener();
                }
                // If the status is not 'restart' and not running, then exit loop
                else if (master_beslistener_status != BESLISTENER_RUNNING) {
                    done = true;
                }
            }
        }
    }
    catch (BESError &e) {
        status = 1;
        // (*BESLog::TheLog())
        // BESLog::TheLog throws exceptions...
        cerr << "Caught BES Error while starting the command handler: " << e.get_message() << endl;
    }
    catch (const std::exception &e) {
        status = 2;
        cerr << "Caught C++ error while starting the command handler: " << e.what() << endl;
    }
    catch (...) {
        status = 3;
        cerr << "Caught unknown error while starting the command handler." << endl;
    }

    BESDEBUG("besdaemon", "besdaemon: past the command processor start" << endl);

    cleanup_resources();

    return status;
}

