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

#include <signal.h>
#include <unistd.h> // for getpid
#include <grp.h>    // for getgrnam
#include <pwd.h>    // for getpwnam

#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>

using std::cout ;
using std::cerr ;
using std::endl ;
using std::ios ;
using std::ostringstream ;
using std::ofstream ;

#include "config.h"

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
#include "BESServerUtils.h"

#include "BESDefaultModule.h"
#include "BESXMLDefaultCommands.h"

static int session_id = 0;

ServerApp::ServerApp()
    : BESModuleApp(),
      _portVal( 0 ),
      _gotPort( false ),
      _unixSocket( "" ),
      _secure( false ),
      _mypid( 0 ),
      _ts( 0 ),
      _us( 0 ),
      _ps( 0 )
{
    _mypid = getpid() ;
}

ServerApp::~ServerApp()
{
}

// This is needed so that the master beslistner will get the exit status of
// all of the child beslisteners (preventing them from becoming zombies).
void
ServerApp::CatchSigChild( int sig )
{
    if( sig == SIGCHLD )
    {
	BESDEBUG("besdaemon", "beslistener: caught sig chld" << endl);
	int stat;
	pid_t pid = wait(&stat);
	BESDEBUG("besdaemon", "beslistener: child pid: " << pid << " exited with status: " << stat << endl);
    }
}

// If the HUP signal is sent to the master beslistener, it should exit and
// return a value indicating to the besdaemon that it should be restarted.
// This also has the side-affect of re-reading the configuration file.
void
ServerApp::CatchSigHup( int sig )
{
    if( sig == SIGHUP )
    {
	int pid = getpid();
	BESDEBUG("besdaemon", "beslisterner: " << pid << " caught SIGHUP." << endl);

	BESApp::TheApplication()->terminate( sig ) ;

	BESDEBUG("besdaemon", "beslisterner: " << pid << " past terminate (SIGHUP)." << endl);

	exit( SERVER_EXIT_RESTART ) ;
    }
}
// This is the default signal sent by 'kill'; when the master beslistener gets
// this signal it should stop. besdaemon should not try to start a new
// master beslistener.
void
ServerApp::CatchSigTerm( int sig )
{
    if( sig == SIGTERM )
    {
	int pid = getpid();
	BESDEBUG("besdaemon", "beslisterner: " << pid << " caught SIGTERM" << endl);

        BESApp::TheApplication()->terminate( sig ) ;

        BESDEBUG("besdaemon", "beslisterner: " << pid << " past terminate (SIGTERM)." << endl);

	exit( SERVER_EXIT_NORMAL_SHUTDOWN ) ;
    }
}

#if 0
// I don't think we should be catching this - if the users wnats to send INT
// to a process they should get the default behavior.
void
ServerApp::signalInterrupt( int sig )
{
    if( sig == SIGINT )
    {
	BESApp::TheApplication()->terminate( sig ) ;
	exit( SERVER_EXIT_NORMAL_SHUTDOWN ) ;
    }
}
#endif


void
ServerApp::set_group_id()
{
#if !defined(OS2) && !defined(TPF)
    // OS/2 and TPF don't support groups.

    // get group id or name from BES configuration file
    // If BES.Group begins with # then it is a group id,
    // else it is a group name and look up the id.
    BESDEBUG( "server", "beslisterner: Setting group id ... " << endl ) ;
    bool found = false ;
    string key = "BES.Group" ;
    string group_str ;
    try
    {
	TheBESKeys::TheKeys()->get_value( key, group_str, found ) ;
    }
    catch( BESError &e )
    {
	BESDEBUG( "server", "beslisterner: FAILED" << endl ) ;
	string err = string("FAILED: ") + e.get_message() ;
	cerr << err << endl ;
	(*BESLog::TheLog()) << err << endl ;
	exit( SERVER_EXIT_FATAL_CAN_NOT_START ) ;
    }
    if( !found || group_str.empty() )
    {
	BESDEBUG( "server", "beslisterner: FAILED" << endl ) ;
	string err = "FAILED: Group not specified in BES configuration file" ;
	cerr << err << endl ;
	(*BESLog::TheLog()) << err << endl ;
	exit( SERVER_EXIT_FATAL_CAN_NOT_START ) ;
    }
    BESDEBUG( "server", "to " << group_str << " ... " << endl ) ;

    gid_t new_gid = 0 ;
    if( group_str[0] == '#' )
    {
	// group id starts with a #, so is a group id
	const char *group_c = group_str.c_str() ;
	group_c++ ;
	new_gid = atoi( group_c ) ;
    }
    else
    {
	// specified group is a group name
	struct group *ent ;
	ent = getgrnam( group_str.c_str() ) ;
	if( !ent )
	{
	    BESDEBUG( "server", "beslisterner: FAILED" << endl ) ;
	    string err = (string)"FAILED: Group " + group_str
			 + " does not exist" ;
	    cerr << err << endl ;
	    (*BESLog::TheLog()) << err << endl ;
	    exit( SERVER_EXIT_FATAL_CAN_NOT_START ) ;
	}
	new_gid = ent->gr_gid ;
    }

    if( new_gid < 1 )
    {
	BESDEBUG( "server", "beslisterner: FAILED" << endl ) ;
	ostringstream err ;
	err << "FAILED: Group id " << new_gid
	    << " not a valid group id for BES" ;
	cerr << err.str() << endl ;
	(*BESLog::TheLog()) << err.str() << endl ;
	exit( SERVER_EXIT_FATAL_CAN_NOT_START ) ;
    }

    BESDEBUG( "server", "to id " << new_gid << " ... " << endl ) ;
    if( setgid( new_gid ) == -1 )
    {
	BESDEBUG( "server", "beslisterner: FAILED" << endl ) ;
	ostringstream err ;
	err << "FAILED: unable to set the group id to " << new_gid ;
	cerr << err.str() << endl ;
	(*BESLog::TheLog()) << err.str() << endl ;
	exit( SERVER_EXIT_FATAL_CAN_NOT_START ) ;
    }

    BESDEBUG( "server", "OK" << endl ) ;
#else
    BESDEBUG( "server", "beslisterner: Groups not supported in this OS" << endl ) ;
#endif
}

void
ServerApp::set_user_id()
{
    BESDEBUG( "server", "beslisterner: Setting user id ... " << endl ) ;

    // Get user name or id from the BES configuration file.
    // If the BES.User value begins with # then it is a user
    // id, else it is a user name and need to look up the
    // user id.
    bool found = false ;
    string key = "BES.User" ;
    string user_str ;
    try
    {
	TheBESKeys::TheKeys()->get_value( key, user_str, found ) ;
    }
    catch( BESError &e )
    {
	BESDEBUG( "server", "beslisterner: FAILED" << endl ) ;
	string err = (string)"FAILED: " + e.get_message() ;
	cerr << err << endl ;
	(*BESLog::TheLog()) << err << endl ;
	exit( SERVER_EXIT_FATAL_CAN_NOT_START ) ;
    }
    if( !found || user_str.empty() )
    {
	BESDEBUG( "server", "beslisterner: FAILED" << endl ) ;
	string err = (string)"FAILED: User not specified in BES config file" ;
	cerr << err << endl ;
	(*BESLog::TheLog()) << err << endl ;
	exit( SERVER_EXIT_FATAL_CAN_NOT_START ) ;
    }
    BESDEBUG( "server", "to " << user_str << " ... " << endl ) ;

    uid_t new_id = 0 ;
    if( user_str[0] == '#' )
    {
	const char *user_str_c = user_str.c_str() ;
	user_str_c++ ;
	new_id = atoi( user_str_c ) ;
    }
    else
    {
	struct passwd *ent ;
	ent = getpwnam( user_str.c_str() ) ;
	if( !ent )
	{
	    BESDEBUG( "server", "beslisterner: FAILED" << endl ) ;
	    string err = (string)"FAILED: Bad user name specified: "
			 + user_str ;
	    cerr << err << endl ;
	    (*BESLog::TheLog()) << err << endl ;
	    exit( SERVER_EXIT_FATAL_CAN_NOT_START ) ;
	}
	new_id = ent->pw_uid ;
    }

    // new user id cannot be root (0)
    if( !new_id )
    {
	BESDEBUG( "server", "beslisterner: FAILED" << endl ) ;
	string err = (string)"FAILED: BES cannot run as root" ;
	cerr << err << endl ;
	(*BESLog::TheLog()) << err << endl ;
	exit( SERVER_EXIT_FATAL_CAN_NOT_START ) ;
    }

    BESDEBUG( "server", "to " << new_id << " ... " << endl ) ;
    if( setuid( new_id ) == -1 )
    {
	BESDEBUG( "server", "beslisterner: FAILED" << endl ) ;
	ostringstream err ;
	err << "FAILED: Unable to set user id to " << new_id ;
	cerr << err.str() << endl ;
	(*BESLog::TheLog()) << err.str() << endl ;
	exit( SERVER_EXIT_FATAL_CAN_NOT_START ) ;
    }
}

int
ServerApp::initialize( int argc, char **argv )
{
    int c = 0 ;
    bool needhelp = false ;
    string dashi ;
    string dashc ;

    // If you change the getopt statement below, be sure to make the
    // corresponding change in daemon.cc and besctl.in
    while( ( c = getopt( argc, argv, "hvsd:c:p:u:i:r:" ) ) != EOF )
    {
	switch( c )
	{
	    case 'i':
		dashi = optarg ;
		break ;
	    case 'c':
		dashc = optarg ;
		break ;
	    case 'r':
		break ; // we can ignore the /var/run directory option here
	    case 'p':
		_portVal = atoi( optarg ) ;
		_gotPort = true ;
		break ;
	    case 'u':
		_unixSocket = optarg ;
		break ;
	    case 'd':
		BESDebug::SetUp( optarg ) ;
		break ;
	    case 'v':
		BESServerUtils::show_version( BESApp::TheApplication()->appName() ) ;
		break ;
	    case 's':
		_secure = true ;
		break ;
	    case 'h':
	    case '?':
	    default:
		needhelp = true ;
		break ;
	}
    }

    // before we can do any processing, log any messages, initialize any
    // modules, do anything, we need to determine where the BES
    // configuration file lives. From here we get the name of the log
    // file, group and user id, and information that the modules will
    // need to run properly.

    // If the -c option was passed, set the config file name in TheBESKeys
    if( !dashc.empty() )
    {
	TheBESKeys::ConfigFile = dashc ;
    }

    // If the -c option was not passed, but the -i option
    // was passed, then use the -i option to construct
    // the path to the config file
    if( dashc.empty() && !dashi.empty() )
    {
	if( dashi[dashi.length()-1] != '/' )
	{
	    dashi += '/' ;
	}
	string conf_file = dashi + "etc/bes/bes.conf" ;
	TheBESKeys::ConfigFile = conf_file ;
    }

    // Now that we have the configuration information, we can log to the
    // BES log file if there are errors in starting up, etc...

    uid_t curr_euid = geteuid() ;
#ifndef BES_DEVELOPER
    // must be root to run this app and to set user id and group id later
    if( curr_euid )
    {
	string err = "FAILED: Must be root to run BES" ;
	cerr << err << endl ;
	(*BESLog::TheLog()) << err << endl ;
	exit( SERVER_EXIT_FATAL_CAN_NOT_START ) ;
    }
#else
    cerr << "Developer Mode: not testing if BES is run by root" << endl ;
#endif

    // register the two debug context for the server and ppt. The
    // Default Module will register the bes context.
    BESDebug::Register( "server" ) ;
    BESDebug::Register( "ppt" ) ;

    // Before we can load modules, start writing to the BES log
    // file, etc... we need to run as the proper user. Set the user
    // id and the group id to what is specified in the BES
    // configuration file
    if( curr_euid == 0 )
    {
#ifdef BES_DEVELOPER
	cerr << "Developer Mode: Running as root - setting group and user ids"
	     << endl ;
#endif
	set_group_id() ;
	set_user_id() ;
    }
    else
    {
	cerr << "Developer Mode: Not setting group or user ids" << endl ;
    }

    // Because we are now running as the user specified in the
    // configuraiton file, we won't be able to listen on system ports.
    // If this is a problem, we may need to move this code above setting
    // the user and group ids.
    bool found = false ;
    string port_key = "BES.ServerPort" ;
    if( !_gotPort )
    {
	string sPort ;
	try
	{
	    TheBESKeys::TheKeys()->get_value( port_key, sPort, found ) ;
	}
	catch( BESError &e )
	{
	    BESDEBUG( "server", "beslisterner: FAILED" << endl ) ;
	    string err = (string)"FAILED: " + e.get_message() ;
	    cerr << err << endl ;
	    (*BESLog::TheLog()) << err << endl ;
	    exit( SERVER_EXIT_FATAL_CAN_NOT_START ) ;
	}
	if( found )
	{
	    _portVal = atoi( sPort.c_str() ) ;
	    if( _portVal != 0 )
	    {
		_gotPort = true ;
	    }
	}
    }

    found = false ;
    string socket_key = "BES.ServerUnixSocket" ;
    if( _unixSocket == "" )
    {
	try
	{
	    TheBESKeys::TheKeys()->get_value( socket_key, _unixSocket, found ) ;
	}
	catch( BESError &e )
	{
	    BESDEBUG( "server", "beslisterner: FAILED" << endl ) ;
	    string err = (string)"FAILED: " + e.get_message() ;
	    cerr << err << endl ;
	    (*BESLog::TheLog()) << err << endl ;
	    exit( SERVER_EXIT_FATAL_CAN_NOT_START ) ;
	}
    }

    if( !_gotPort && _unixSocket == "" )
    {
	string msg = "Must specify a tcp port or a unix socket or both\n" ;
	msg += "Please specify on the command line with -p <port>" ;
	msg += " and/or -u <unix_socket>\n" ;
	msg += "Or specify in the bes configuration file with "
	    + port_key + " and/or " + socket_key + "\n" ;
	cout << endl << msg ;
	(*BESLog::TheLog()) << msg << endl ;
	BESServerUtils::show_usage( BESApp::TheApplication()->appName() ) ;
    }

    found = false ;
    if( _secure == false )
    {
	string key = "BES.ServerSecure" ;
	string isSecure ;
	try
	{
	    TheBESKeys::TheKeys()->get_value( key, isSecure, found ) ;
	}
	catch( BESError &e )
	{
	    BESDEBUG( "server", "beslisterner: FAILED" << endl ) ;
	    string err = (string)"FAILED: " + e.get_message() ;
	    cerr << err << endl ;
	    (*BESLog::TheLog()) << err << endl ;
	    exit( SERVER_EXIT_FATAL_CAN_NOT_START ) ;
	}
	if( isSecure == "Yes" || isSecure == "YES" || isSecure == "yes" )
	{
	    _secure = true ;
	}
    }

    BESDEBUG( "server", "beslisterner: Registering signal SIGCHLD ... " << endl ) ;
    if( signal( SIGCHLD, CatchSigChild ) == SIG_ERR )
    {
	BESDEBUG( "server", "beslisterner: FAILED" << endl ) ;
	string err = "FAILED: cannot register SIGCHLD signal handler" ;
	cerr << err << endl ;
	(*BESLog::TheLog()) << err << endl ;
	exit( SERVER_EXIT_FATAL_CAN_NOT_START ) ;
    }
    BESDEBUG( "server", "OK" << endl ) ;

#if 0
    BESDEBUG( "server", "beslisterner: Registering signal SIGINT ... " << endl ) ;
    if( signal( SIGINT, signalInterrupt ) == SIG_ERR )
    {
	BESDEBUG( "server", "FAILED" << endl ) ;
	string err = "FAILED: cannot register SIGINT signal handler" ;
	cerr << err << endl ;
	(*BESLog::TheLog()) << err << endl ;
	exit( SERVER_EXIT_FATAL_CAN_NOT_START ) ;
    }
    BESDEBUG( "server", "OK" << endl ) ;
#endif

    BESDEBUG( "server", "beslisterner: Registering signal SIGHUP ... " << endl ) ;
    if( signal( SIGHUP, CatchSigHup ) == SIG_ERR )
    {
	BESDEBUG( "server", "beslisterner: FAILED" << endl ) ;
	string err = "FAILED: cannot register SIGHUP signal handler" ;
	cerr << err << endl ;
	(*BESLog::TheLog()) << err << endl ;
	exit( SERVER_EXIT_FATAL_CAN_NOT_START ) ;
    }
    BESDEBUG( "server", "OK" << endl ) ;

    BESDEBUG( "server", "beslisterner: Registering signal SIGTERM ... " << endl ) ;
    if( signal( SIGTERM, CatchSigTerm ) == SIG_ERR )
    {
	BESDEBUG( "server", "beslisterner: FAILED" << endl ) ;
	string err = "FAILED: cannot register SIGTERM signal handler" ;
	cerr << err << endl ;
	(*BESLog::TheLog()) << err << endl ;
	exit( SERVER_EXIT_FATAL_CAN_NOT_START ) ;
    }
    BESDEBUG( "server", "OK" << endl ) ;

    BESDEBUG( "server", "beslisterner: initializing default module ... "
			<< endl ) ;
    BESDefaultModule::initialize( argc, argv ) ;
    BESDEBUG( "server", "beslisterner: done initializing default module"
			<< endl ) ;

    BESDEBUG( "server", "beslisterner: initializing default commands ... "
			<< endl ) ;
    BESXMLDefaultCommands::initialize( argc, argv ) ;
    BESDEBUG( "server", "beslisterner: done initializing default commands"
			<< endl ) ;

    // This will load and initialize all of the modules
    BESDEBUG( "server", "beslisterner: initializing loaded modules ... "
			<< endl ) ;
    int ret = BESModuleApp::initialize( argc, argv ) ;
    BESDEBUG( "server", "beslisterner: done initializing loaded modules"
			<< endl ) ;

    BESDEBUG( "server", "beslisterner: initialized settings:" << *this ) ;

    if( needhelp )
    {
	BESServerUtils::show_usage( BESApp::TheApplication()->appName() ) ;
    }

    // This sets the process group to be ID of this process. All children
    // will get this GID. Then use killpg() to send a signal to this process
    // and all of the children.
    session_id = setsid();
    BESDEBUG("besdaemon", "beslisterner: The master beslistener session id (group id): " << session_id << endl);

    return ret ;
}

int
ServerApp::run()
{
    try
    {
	BESDEBUG( "server", "beslisterner: initializing memory pool ... "
			    << endl ) ;
	BESMemoryManager::initialize_memory_pool() ;
	BESDEBUG( "server", "OK" << endl ) ;

	SocketListener listener ;

	if( _portVal )
	{
	    _ts = new TcpSocket( _portVal ) ;
	    listener.listen( _ts ) ;
	    BESDEBUG( "server", "beslisterner: listening on port ("
				<< _portVal << ")" << endl ) ;
	    int status = 4; // ***
	    BESDEBUG( "server", "beslisterner: about to write status (4)" << endl ) ;
	    int res = write(4, &status, sizeof(status)); // ***
	    BESDEBUG( "server", "beslisterner: wrote status (" << res << ")" << endl ) ;
	}

	if( !_unixSocket.empty() )
	{
	    _us = new UnixSocket( _unixSocket ) ;
	    listener.listen( _us ) ;
	    BESDEBUG( "server", "beslisterner: listening on unix socket ("
				<< _unixSocket << ")" << endl ) ;
	}

	BESServerHandler handler ;

	_ps = new PPTServer( &handler, &listener, _secure ) ;
	_ps->initConnection() ;
    }
    catch (BESError &se) {
	cerr << se.get_message() << endl;
	(*BESLog::TheLog()) << se.get_message() << endl;
	int status = SERVER_EXIT_FATAL_CAN_NOT_START; // ***
	write(4, &status, sizeof(status)); // ***
	return 1;
    }
    catch (...) {
	cerr << "caught unknown exception" << endl;
	(*BESLog::TheLog()) << "caught unknown exception initializing sockets" << endl;
	int status = SERVER_EXIT_FATAL_CAN_NOT_START; // ***
	write(4, &status, sizeof(status)); // ***
	return 1;
    }

    return 0 ;
}

int
ServerApp::terminate( int sig )
{
    pid_t apppid = getpid() ;
    if( apppid == _mypid )
    {
	if( _ps )
	{
	    _ps->closeConnection() ;
	    delete _ps ;
	}
	if( _ts )
	{
	    _ts->close() ;
	    delete _ts ;
	}
	if( _us )
	{
	    _us->close() ;
	    delete _us ;
	}

	// Do this in the reverse order that it was initialized. So
	// terminate the loaded modules first, then the default
	// commands, then the default module.
	BESDEBUG( "server", "beslisterner: terminating loaded modules ...  "
	                    << endl ) ;
	BESModuleApp::terminate( sig ) ;
	BESDEBUG( "server", "beslisterner: done terminating loaded modules"
	                    << endl ) ;

	BESDEBUG( "server", "beslisterner: terminating default commands ...  "
			    << endl ) ;
	BESXMLDefaultCommands::terminate( ) ;
	BESDEBUG( "server", "beslisterner: done terminating default commands ...  "
			    << endl ) ;

	BESDEBUG( "server", "beslisterner: terminating default module ... "
			    << endl ) ;
	BESDefaultModule::terminate( ) ;
	BESDEBUG( "server", "beslisterner: done terminating default module ... "
			    << endl ) ;
    }
    return sig ;
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance
 *
 * @param strm C++ i/o stream to dump the information to
 */
void
ServerApp::dump( ostream &strm ) const
{
    strm << BESIndent::LMarg << "ServerApp::dump - ("
			     << (void *)this << ")" << endl ;
    BESIndent::Indent() ;
    strm << BESIndent::LMarg << "got port? " << _gotPort << endl ;
    strm << BESIndent::LMarg << "port: " << _portVal << endl ;
    strm << BESIndent::LMarg << "unix socket: " << _unixSocket << endl ;
    strm << BESIndent::LMarg << "is secure? " << _secure << endl ;
    strm << BESIndent::LMarg << "pid: " << _mypid << endl ;
    if( _ts )
    {
	strm << BESIndent::LMarg << "tcp socket:" << endl ;
	BESIndent::Indent() ;
	_ts->dump( strm ) ;
	BESIndent::UnIndent() ;
    }
    else
    {
	strm << BESIndent::LMarg << "tcp socket: null" << endl ;
    }
    if( _us )
    {
	strm << BESIndent::LMarg << "unix socket:" << endl ;
	BESIndent::Indent() ;
	_us->dump( strm ) ;
	BESIndent::UnIndent() ;
    }
    else
    {
	strm << BESIndent::LMarg << "unix socket: null" << endl ;
    }
    if( _ps )
    {
	strm << BESIndent::LMarg << "ppt server:" << endl ;
	BESIndent::Indent() ;
	_ps->dump( strm ) ;
	BESIndent::UnIndent() ;
    }
    else
    {
	strm << BESIndent::LMarg << "ppt server: null" << endl ;
    }
    BESModuleApp::dump( strm ) ;
    BESIndent::UnIndent() ;
}

int
main( int argc, char **argv )
{
    try
    {
	ServerApp app ;
	return app.main( argc, argv ) ;
    }
    catch( BESError &e )
    {
	cerr << "Caught unhandled exception: " << endl ;
	cerr << e.get_message() << endl ;
	return 1 ;
    }
    catch( ... )
    {
	cerr << "Caught unhandled, unknown exception" << endl ;
	return 1 ;
    }
    return 0 ;
}

