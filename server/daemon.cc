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
#include <cstring>
#include <cstdlib>
#include <cerrno>

using std::ifstream ;
using std::ofstream ;
using std::cout ;
using std::endl ;
using std::cerr ;
using std::flush;
using std::string ;

#include "config.h"
#include "ServerExitConditions.h"
#include "SocketListener.h"
#include "TcpSocket.h"
#include "UnixSocket.h"
#include "PPTServer.h"
#include "DaemonCommandHandler.h"
#include "BESServerUtils.h"
#include "BESScrub.h"
#include "BESError.h"
#include "BESDebug.h"
#include "TheBESKeys.h"

#define BES_SERVER "/beslistener"
#define BES_SERVER_PID "/bes.pid"
#define DAEMON_PORT_STR "BES.DaemonPort"

int  daemon_init() ;
int  start_master_beslistener( char **arguments ) ;
void stop_all_beslisteners( int sig ) ;
int  pr_exit( int status ) ;
void store_daemon_id( int pid ) ;
bool load_names( const string &install_dir, const string &pid_dir ) ;

string NameProgram ;

// This two variables are set by load_names
string server_name ;
string file_for_daemon ;
// This can be used to see if HUP or TERM has been sent to the master bes
int master_beslistener_status = -1;
int master_beslistener_pid = -1;	// This is also the process group id

char **arguments = 0 ;

/** Stop all of the listeners (both the master listener and all of the
 *  child listeners that actually process requests). A test version of this
 *  used the master beslistener's exit status to determine if the daemon
 *  should try to restart the master beslistener (the beslistener still
 *  returns a different value depending on which signal it gets, even though
 *  it's not used anymore).
 * @param sig
 */
void stop_all_beslisteners(int sig)
{
    BESDEBUG("besdaemon", "besdaemon: stopping listeners" << endl);

    // deregister sigchld handler
    sigignore(SIGCHLD);

    // Send 'sig' to all members of the process group with/of the master bes.
    // The master beslistener pid is the group id of all of the beslisteners.
    int status = killpg(master_beslistener_pid, sig);
    switch (status) {
    case EINVAL:
	cerr << "The sig argument is not a valid signal number." << endl;
	break;

    case EPERM:
	cerr << "The sending process is not the super-user and one or more of the target processes has an effective user ID different from that of the sending process."
	    << endl;
	break;

    case ESRCH:
	cerr << "No process can be found in the process group specified by the process group (" << master_beslistener_pid << ")." << endl;
	break;

    default: // No error
	break;
    }

    int pid;
    while ((pid = wait(&status)) > 0) {
	BESDEBUG("besdaemon", "besdaemon: caught listener: " << pid << " status: " << pr_exit(status) << endl);
	if (pid = master_beslistener_pid) {
	    BESDEBUG("besdaemon", "besdaemon: caught master beslistener: " << pid << " status: " << pr_exit(status) << endl);
	    master_beslistener_status = pr_exit(status);
	}
    }

    BESDEBUG("besdaemon", "besdaemon: done catching listeners" << endl);
}

/** Start the 'master beslistener' and wait for its exit status. That status
    value is passed to pr_exit() which then either returns 0, 1 or > 1. The
    value returned by pr_exit is the value returned by this function.

   @param arguments argumnet[0] is the full path to the beslistener; the
   remaining stuff is just a copy of the arguments passed to this daemon on
   startup.
   @return 0 for an error or the PID of the master beslistener
 */
int
start_master_beslistener(char **arguments)
{
    int pid;
    if( ( pid = fork() ) < 0 )
    {
	cerr << NameProgram << ": fork error " ;
	const char *perror_string = strerror( errno ) ;
	if( perror_string )
	    cerr << perror_string ;
	cerr << endl ;
	return 0 ;
    }
    else if( pid == 0 ) /* child process */
    {
	// This starts the beslistener (aka mbes) that is parent of the
	// beslisteners that actually serve data.
	cerr << "Starting: " << arguments[0] << endl;
	execvp( arguments[0], arguments ) ;
	cerr << NameProgram
	     << ": mounting listener, subprocess failed: " ;
	const char *perror_string = strerror( errno ) ;
	if( perror_string )
	    cerr << perror_string ;
	cerr << endl ;
	exit( 1 ) ; 	//NB: This exits from the child process.
    }

    // parent
    BESDEBUG("besdaemon", "besdaemon: master beslistener pid: " << pid << endl);

    return pid;
}

void CatchSigChild(int signal)
{
    if (signal == SIGCHLD) {
	int pid = wait(&master_beslistener_status);
	BESDEBUG("besdaemon",  "besdaemon: caught master beslistener (" << pid << ") status: " << pr_exit(master_beslistener_status) << endl);
	// Decode and record the exit status.
	master_beslistener_status = pr_exit(master_beslistener_status);
    }
}

#if 1
// When the daemon gets the HUP signal, it forwards that onto each beslistener.
// They then all exit, returning the 'restart' code so that the daemon knows
// to restart the master beslistener. I wrote this for testing before I had
// the command interpreter running.
void CatchSigHup(int signal)
{
    if (signal == SIGHUP) {
	BESDEBUG("besdaemon", "caught SIGHUP in besdaemon." << endl);
	BESDEBUG("besdaemon", "sending SIGHUP to the process group: " << master_beslistener_pid << endl);

	stop_all_beslisteners(SIGHUP);
    }
}
#endif

// When TERM (the default for 'kill') is sent to this process, send it also
// to each beslistener. This will cause the beslisteners to all exit with a zero
// value (the code for 'do not restart').
void CatchSigTerm(int signal)
{
    if (signal == SIGTERM) {
	BESDEBUG("besdaemon", "besdaemon: caught SIGTERM." << endl);
	BESDEBUG("besdaemon", "besdaemon: sending SIGTERM to the process group: " << master_beslistener_pid << endl);

	// Stop all of the beslisteners
	stop_all_beslisteners(SIGTERM);

	// Once all the child exit status values are read, exit the daemon
	exit(0);
    }
}

/** Start the daemon command interpreter.
 *
 * @param handler
 * @return 1 if the interpreter exited and the daemon should stop, 0 if the
 * interpreter was not started.
 */
int start_command_processor(DaemonCommandHandler &handler)
{
    TcpSocket *socket = 0;
    UnixSocket *unix_socket = 0;
    PPTServer *command_server = 0;

    try {
	SocketListener listener;

	string port_str;
	bool port_found;
	int port = 0;
	TheBESKeys::TheKeys()->get_value( DAEMON_PORT_STR, port_str, port_found ) ;
	if (port_found) {
	    char *ptr;
	    port = strtol(port_str.c_str(), &ptr, 10);
	    if (port == 0) {
		cerr << "Invalid port number for daemon command interface: " << port_str << endl;
		exit(1);
	    }
	}

	if (port) {
	    BESDEBUG("besdaemon", "besdaemon: starting command interface on port: " << port << endl);
	    socket = new TcpSocket(port);
	    listener.listen(socket);
	}

	string usock_str;
	bool usock_found;
	TheBESKeys::TheKeys()->get_value( "BES.DaemonUnixSocket", usock_str, usock_found ) ;

	if (!usock_str.empty()) {
	    BESDEBUG("besdaemon", "besdaemon: starting command interface on socket: " << usock_str << endl);
	    unix_socket = new UnixSocket(usock_str);
	    listener.listen(unix_socket);
	}

	if (!port_found && !usock_found) {
	    BESDEBUG("besdaemon", "Neither a port nor a unix socket was set for the daemon command interface." << endl);
	    return 0;
	}

	command_server = new PPTServer(&handler, &listener, /*is_secure*/false);
	command_server->initConnection();

	delete command_server; command_server = 0;
	delete socket; socket = 0;
	delete unix_socket; unix_socket = 0;

	// When/if the command interpreter exits, stop the all listeners.
	stop_all_beslisteners(SIGTERM);
	return 1;
    }
    catch (BESError &se) {
	cerr << "daemon: " << se.get_message() << endl;
	delete command_server; command_server = 0;
	delete socket; socket = 0;
	delete unix_socket; unix_socket = 0;

	stop_all_beslisteners(SIGTERM);
	return 1;
    }
    catch (...) {
	cerr << "daemon: " << "caught unknown exception" << endl;
	delete command_server; command_server = 0;
	delete socket; socket = 0;
	delete unix_socket; unix_socket = 0;

	stop_all_beslisteners(SIGTERM);
	return 1;
    }
}


int
main(int argc, char *argv[])
{
#ifndef BES_DEVELOPER
    // must be root to run this app and to set user id and group id later
    uid_t curr_euid = geteuid() ;
    if( curr_euid )
    {
	cerr << "FAILED: Must be root to run BES" << endl ;
	exit( SERVER_EXIT_FATAL_CAN_NOT_START ) ;
    }
#else
    cerr << "Developer Mode: not testing if BES is run by root" << endl ;
#endif

    NameProgram = argv[0] ;

    string install_dir ;
    string pid_dir ;

    // If you change the getopt statement below, be sure to make the
    // corresponding change in ServerApp.cc and besctl.in
    int c = 0 ;

    // there are 16 arguments allowed to the daemon, including the program
    // name. 3 options do not have arguments and 6 have arguments
    if( argc > 16 )
    {
	// the show_usage method exits
	BESServerUtils::show_usage( NameProgram ) ;
    }

    // Most of the argument precessing is just for vetting the arguments
    // that will be passed onto the beslistener(s), but we do grab some info
    string config_file = "";
    // argv[0] is the name of the program, so start num_args at 1
    unsigned short num_args = 1 ;
    while( ( c = getopt( argc, argv, "hvsd:c:p:u:i:r:" ) ) != EOF )
    {
	switch( c )
	{
	    case 'v': // version
		BESServerUtils::show_version( NameProgram ) ;
		break ;
	    case '?': // unknown option
	    case 'h': // help
		BESServerUtils::show_usage( NameProgram ) ;
		break ;
	    case 'i': // BES install directory
		install_dir = optarg ;
		if( BESScrub::pathname_ok( install_dir, true ) == false )
		{
		    cout << "The specified install directory (-i option) "
		         << "is incorrectly formatted. Must be less than "
			 << "255 characters and include the characters "
			 << "[0-9A-z_./-]" << endl ;
		    return 1 ;
		}
		num_args+=2 ;
		break ;
	    case 's': // secure server
		num_args++ ;
		break ;
	    case 'r': // where to write the pid file
	    {
		pid_dir = optarg ;
		if( BESScrub::pathname_ok( pid_dir, true ) == false )
		{
		    cout << "The specified state directory (-r option) "
		         << "is incorrectly formatted. Must be less than "
			 << "255 characters and include the characters "
			 << "[0-9A-z_./-]" << endl ;
		    return 1 ;
		}
		num_args+=2 ;
	    }
	    break ;
	    case 'c': // configuration file
	    {
		config_file = optarg ;
		if( BESScrub::pathname_ok( config_file, true ) == false )
		{
		    cout << "The specified configuration file (-c option) "
		         << "is incorrectly formatted. Must be less than "
			 << "255 characters and include the characters "
			 << "[0-9A-z_./-]" << endl ;
		    return 1 ;
		}
		num_args+=2 ;
	    }
	    break ;
	    case 'u': // unix socket
	    {
		string check_path = optarg ;
		if( BESScrub::pathname_ok( check_path, true ) == false )
		{
		    cout << "The specified unix socket (-u option) "
		         << "is incorrectly formatted. Must be less than "
			 << "255 characters and include the characters "
			 << "[0-9A-z_./-]" << endl ;
		    return 1 ;
		}
		num_args+=2 ;
	    }
	    break ;
	    case 'p': // TCP port
	    {
		string port_num = optarg ;
		for( unsigned int i = 0; i < port_num.length(); i++ )
		{
		    if( !isdigit( port_num[i] ) )
		    {
			cout << "The specified port contains non-digit "
			     << "characters: " << port_num << endl ;
			return 1 ;
		    }
		}
		num_args+=2 ;
	    }
	    break ;
	    case 'd': // debug
	    {
		string check_arg = optarg ;
		if( BESScrub::command_line_arg_ok( check_arg ) == false )
		{
		    cout << "The specified debug options \"" << check_arg
		         << "\" contains invalid characters" << endl ;
		    return 1 ;
		}
		BESDebug::SetUp( optarg ) ;
		num_args+=2 ;
	    }
	    break ;
	    default:
		BESServerUtils::show_usage( NameProgram ) ;
		break ;
	}
    }
    // if the number of arguments is greater than the number of allowed arguments
    // then extra arguments were passed that aren't options. Show usage and
    // exit.
    if( argc > num_args )
    {
	cout << NameProgram
	     << ": too many arguments passed to the BES" ;
	BESServerUtils::show_usage( NameProgram ) ;
    }

    if( pid_dir.empty() )
    {
	pid_dir = install_dir ;
    }

#if 1
    // If the -c option was passed, set the config file name in TheBESKeys
    if( !config_file.empty() )
    {
	TheBESKeys::ConfigFile = config_file ;
    }

    // If the -c option was not passed, but the -i option
    // was passed, then use the -i option to construct
    // the path to the config file
    if( install_dir.empty() && !install_dir.empty() )
    {
	if( install_dir[install_dir.length()-1] != '/' )
	{
	    install_dir += '/' ;
	}
	string conf_file = install_dir + "etc/bes/bes.conf" ;
	TheBESKeys::ConfigFile = conf_file ;
    }
#endif

    // Set the name of the listener and the file for the listener pid
    if( !load_names( install_dir, pid_dir ) )
	return 1 ;

    // make sure the array size is not too big
    if( BESScrub::size_ok( sizeof( char *), num_args+1 ) == false )
    {
	cout << NameProgram
	     << ": too many arguments passed to the BES" ;
	BESServerUtils::show_usage( NameProgram ) ;
    }

    arguments = new char *[num_args+1] ;

    // Set arguments[0] to the name of the listener
    string::size_type len = server_name.length() ;
    char temp_name[len + 1] ;
    server_name.copy( temp_name, len ) ;
    temp_name[len] = '\0' ;
    arguments[0] = temp_name ;

    // Marshal the arguments to the listener from the command line
    // arguments to the daemon
    for( int i = 1; i < num_args; i++ )
    {
	arguments[i] = argv[i] ;
    }
    arguments[num_args] = NULL ;

    if( !access( file_for_daemon.c_str(), F_OK ) )
    {
	ifstream temp( file_for_daemon.c_str() ) ;
	cout << NameProgram
	     << ": there seems to be a BES daemon already running at " ;
	char buf[500] ;
	temp.getline( buf, 500 ) ;
	cout << buf << endl ;
	temp.close() ;
	return 1 ;
    }

    daemon_init() ;

    if (signal(SIGCHLD, CatchSigChild) < 0) {
	cerr << "Could not register a handler to catch beslistener status." << endl;
	exit(1);
    }

    if (signal(SIGTERM, CatchSigTerm) < 0) {
	cerr << "Could not register a handler to catch the terminate signal." << endl;
	exit(1);
    }

#if 1

    // broken
    if (signal(SIGHUP, CatchSigHup) < 0) {
	cerr << "Could not register a handler to catch the hang-up signal." << endl;
	exit(1);
    }
#endif

    // master_beslistener_pid is global so that the signal handlers can use it
    master_beslistener_pid = start_master_beslistener( arguments ) ;
    if( master_beslistener_pid == 0 )			// Error
    {
	cerr << NameProgram
	     << ": server cannot mount at first try (core dump). "
	     << "Please correct problems on the process manager "
	     << server_name << endl ;
	return master_beslistener_pid ;
    }

    store_daemon_id( getpid() ) ;

    BESDEBUG("besdaemon", "besdaemon: master_beslistener_pid: " << master_beslistener_pid << endl);

    DaemonCommandHandler handler;
    int status = start_command_processor(handler);
    if (status == 0) {
	while (true) {
	    sleep(5);
	    // When master_beslistener_status is not -1, we've caught sigchld
	    // from the master beslistener. Should we restart? Look at the
	    // value of the status.
	    BESDEBUG("besdaemon", "besdaemon: master_beslistener_status: " << master_beslistener_status << endl);
	    if (master_beslistener_status == SERVER_EXIT_RESTART) {
		master_beslistener_status = -1;
		master_beslistener_pid = start_master_beslistener(arguments);
	    }
	}
    }

    BESDEBUG("besdaemon", "besdaemon: past the command processor start" << endl);

    delete [] arguments; arguments = 0 ;

    if( !access( file_for_daemon.c_str(), F_OK ) )
    {
	(void)remove( file_for_daemon.c_str() ) ;
    }

    return status ;
}

/** Make this process a daemon (a process with ppid of 1) and a session
 * leader.
 * @return -1 if the initial fork() call fails; 0 otherwise.
 */
int
daemon_init()
{
    pid_t pid ;
    if( ( pid = fork() ) < 0 )	// error
	return -1 ;
    else if( pid != 0 )		// parent exits
	exit( 0 ) ;
    setsid() ;			// child establishes its own process group
    return 0 ;
}

/** Evaluate the exit status returned to the besdaemon by the master
    beslistener and return 0, 1 or SERVER_EXIT_RESTART.

    @param status The status (value) of the child process.
    @return If the status indicates that the child process exited normally,
    return 0; abnormally, return 1; indicating restart needed, return the
    value of SERVER_EXIT_RESTART.
 */
int
pr_exit(int status)
{
    if (WIFEXITED( status )) {
	switch (WEXITSTATUS( status )) {
	case SERVER_EXIT_NORMAL_SHUTDOWN:
	    return 0;

	case SERVER_EXIT_FATAL_CAN_NOT_START:
	    cerr << NameProgram << ": server cannot start, exited with status " << WEXITSTATUS( status ) << endl;
	    cerr << "Please check all error messages " << "and adjust server installation" << endl;
	    return 1;

	case SERVER_EXIT_ABNORMAL_TERMINATION:
	    cerr << NameProgram << ": abnormal server termination, exited with status " << WEXITSTATUS( status )
		    << endl;
	    return 1;

	case SERVER_EXIT_RESTART:
	    cout << NameProgram << ": server has been requested to re-start." << endl;
	    return SERVER_EXIT_RESTART;

	default:
	    return 1;
	}
    }
    else if (WIFSIGNALED( status )) {
	cerr << NameProgram << ": abnormal server termination, signaled with signal number " << WTERMSIG( status )
		<< endl;
#ifdef WCOREDUMP
	if (WCOREDUMP( status )) {
	    cerr << NameProgram << ": server dumped core." << endl;
	    return 1;
	}
#endif
	return 1;
    }
    else if (WIFSTOPPED( status )) {
	cerr << NameProgram << ": abnormal server termination, stopped with signal number " << WSTOPSIG( status )
		<< endl;
	return 1;
    }

    return 0;
}

void
store_daemon_id( int pid )
{
    const char *perror_string = 0 ;
    ofstream f( file_for_daemon.c_str() ) ;
    if( !f )
    {
	cerr << NameProgram << ": unable to create pid file "
	     << file_for_daemon << ": " ;
	perror_string = strerror( errno ) ;
	if( perror_string )
	    cerr << perror_string ;
	cerr << " ... Continuing" << endl ;
	cerr << endl ;
    }
    else
    {
	f << "PID: " << pid << " UID: " << getuid() << endl ;
	f.close() ;
	mode_t new_mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH ;
	(void)chmod( file_for_daemon.c_str(), new_mode ) ;
    }
}

bool
load_names( const string &install_dir, const string &pid_dir )
{
    string bindir = "/bin";
    if( !pid_dir.empty() )
    {
	file_for_daemon = pid_dir ;
    }

    if( !install_dir.empty() )
    {
	server_name = install_dir ;
	server_name += bindir ;
	if( file_for_daemon.empty() )
	{
	    file_for_daemon = install_dir + "/var/run" ;
	}
    }
    else
    {
	string prog = NameProgram ;
	string::size_type slash = prog.find_last_of( '/' ) ;
	if( slash != string::npos )
	{
	    server_name = prog.substr( 0, slash ) ;
	    slash = prog.find_last_of( '/' ) ;
	    if( slash != string::npos )
	    {
		string root = prog.substr( 0, slash ) ;
		if( file_for_daemon.empty() )
		{
		    file_for_daemon = root + "/var/run" ;
		}
	    }
	    else
	    {
		if( file_for_daemon.empty() )
		{
		    file_for_daemon = server_name ;
		}
	    }
	}
    }

    if( server_name == "" )
    {
	server_name = "." ;
	if( file_for_daemon.empty() )
	{
	    file_for_daemon = "./run" ;
	}
    }

    server_name += BES_SERVER ;
    file_for_daemon += BES_SERVER_PID ;

    if( access( server_name.c_str(), F_OK ) != 0 )
    {
	cerr << NameProgram
	     << ": cannot start." << server_name << endl
	     << "Please either pass -i <install_dir> on the command line."
	     << endl ;
	return false ;
    }
    return true ;
}

