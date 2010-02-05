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

void
ServerApp::signalTerminate( int sig )
{
    if( sig == SIGTERM )
    {
        BESApp::TheApplication()->terminate( sig ) ;
	exit( SERVER_EXIT_NORMAL_SHUTDOWN ) ;
    }
}

void
ServerApp::signalInterrupt( int sig )
{
    if( sig == SIGINT )
    {
	BESApp::TheApplication()->terminate( sig ) ;
	exit( SERVER_EXIT_NORMAL_SHUTDOWN ) ;
    }
}

void
ServerApp::signalRestart( int sig )
{
    if( sig == SIGUSR1 )
    {
	BESApp::TheApplication()->terminate( sig ) ;
	exit( SERVER_EXIT_RESTART ) ;
    }
}

void
ServerApp::set_group_id()
{
#if !defined(OS2) && !defined(TPF)
    // OS/2 and TPF don't support groups.

    // get group id or name from BES configuration file
    // If BES.Group begins with # then it is a group id,
    // else it is a group name and look up the id.
    BESDEBUG( "server", "ServerApp: Setting group id ... " << endl ) ;
    bool found = false ;
    string key = "BES.Group" ;
    string group_str ;
    try
    {
	TheBESKeys::TheKeys()->get_value( key, group_str, found ) ;
    }
    catch( BESError &e )
    {
	BESDEBUG( "server", "FAILED" << endl ) ;
	string err = string("FAILED: ") + e.get_message() ;
	cerr << err << endl ;
	(*BESLog::TheLog()) << err << endl ;
	exit( SERVER_EXIT_FATAL_CAN_NOT_START ) ;
    }
    if( !found || group_str.empty() )
    {
	BESDEBUG( "server", "FAILED" << endl ) ;
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
	    BESDEBUG( "server", "FAILED" << endl ) ;
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
	BESDEBUG( "server", "FAILED" << endl ) ;
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
	BESDEBUG( "server", "FAILED" << endl ) ;
	ostringstream err ;
	err << "FAILED: unable to set the group id to " << new_gid ;
	cerr << err.str() << endl ;
	(*BESLog::TheLog()) << err.str() << endl ;
	exit( SERVER_EXIT_FATAL_CAN_NOT_START ) ;
    }

    BESDEBUG( "server", "OK" << endl ) ;
#else
    BESDEBUG( "server", "ServerApp: Groups not supported in this OS" << endl ) ;
#endif
}

void
ServerApp::set_user_id()
{
    BESDEBUG( "server", "ServerApp: Setting user id ... " << endl ) ;

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
	BESDEBUG( "server", "FAILED" << endl ) ;
	string err = (string)"FAILED: " + e.get_message() ;
	cerr << err << endl ;
	(*BESLog::TheLog()) << err << endl ;
	exit( SERVER_EXIT_FATAL_CAN_NOT_START ) ;
    }
    if( !found || user_str.empty() )
    {
	BESDEBUG( "server", "FAILED" << endl ) ;
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
	    BESDEBUG( "server", "FAILED" << endl ) ;
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
	BESDEBUG( "server", "FAILED" << endl ) ;
	string err = (string)"FAILED: BES cannot run as root" ;
	cerr << err << endl ;
	(*BESLog::TheLog()) << err << endl ;
	exit( SERVER_EXIT_FATAL_CAN_NOT_START ) ;
    }

    BESDEBUG( "server", "to " << new_id << " ... " << endl ) ;
    if( setuid( new_id ) == -1 )
    {
	BESDEBUG( "server", "FAILED" << endl ) ;
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

    // If the -c optiion was passed, set the config file name in TheBESKeys
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
    cout << "Developer Mode: not testing if BES is run by root" << endl ;
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
	cout << "Developer Mode: Running as root - setting group and user ids"
	     << endl ;
#endif
	set_group_id() ;
	set_user_id() ;
    }
    else
    {
	cout << "Developer Mode: Not setting group or user ids" << endl ;
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
	    BESDEBUG( "server", "FAILED" << endl ) ;
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
	    BESDEBUG( "server", "FAILED" << endl ) ;
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
	    BESDEBUG( "server", "FAILED" << endl ) ;
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

    BESDEBUG( "server", "ServerApp: Registering signal SIGTERM ... " << endl ) ;
    if( signal( SIGTERM, signalTerminate ) == SIG_ERR )
    {
	BESDEBUG( "server", "FAILED" << endl ) ;
	string err = "FAILED: cannot register SIGTERM signal handler" ;
	cerr << err << endl ;
	(*BESLog::TheLog()) << err << endl ;
	exit( SERVER_EXIT_FATAL_CAN_NOT_START ) ;
    }
    BESDEBUG( "server", "OK" << endl ) ;

    BESDEBUG( "server", "ServerApp: Registering signal SIGINT ... " << endl ) ;
    if( signal( SIGINT, signalInterrupt ) == SIG_ERR )
    {
	BESDEBUG( "server", "FAILED" << endl ) ;
	string err = "FAILED: cannot register SIGINT signal handler" ;
	cerr << err << endl ;
	(*BESLog::TheLog()) << err << endl ;
	exit( SERVER_EXIT_FATAL_CAN_NOT_START ) ;
    }
    BESDEBUG( "server", "OK" << endl ) ;

    BESDEBUG( "server", "ServerApp: Registering signal SIGUSR1 ... " << endl ) ;
    if( signal( SIGUSR1, signalRestart ) == SIG_ERR )
    {
	BESDEBUG( "server", "FAILED" << endl ) ;
	string err = "FAILED: cannot register SIGUSR1 signal handler" ;
	cerr << err << endl ;
	(*BESLog::TheLog()) << err << endl ;
	exit( SERVER_EXIT_FATAL_CAN_NOT_START ) ;
    }
    BESDEBUG( "server", "OK" << endl ) ;

    BESDEBUG( "server", "ServerApp: initializing default module ... "
			<< endl ) ;
    BESDefaultModule::initialize( argc, argv ) ;
    BESDEBUG( "server", "OK" << endl ) ;

    BESDEBUG( "server", "ServerApp: initializing default commands ... "
			<< endl ) ;
    BESXMLDefaultCommands::initialize( argc, argv ) ;
    BESDEBUG( "server", "OK" << endl ) ;

    // This will load and initialize all of the modules
    int ret = BESModuleApp::initialize( argc, argv ) ;

    BESDEBUG( "server", "ServerApp: initialized settings:" << *this ) ;

    if( needhelp )
    {
	BESServerUtils::show_usage( BESApp::TheApplication()->appName() ) ;
    }

    return ret ;
}

int
ServerApp::run()
{
    try
    {
	BESDEBUG( "server", "ServerApp: initializing memory pool ... "
			    << endl ) ;
	BESMemoryManager::initialize_memory_pool() ;
	BESDEBUG( "server", "OK" << endl ) ;

	SocketListener listener ;

	if( _portVal )
	{
	    _ts = new TcpSocket( _portVal ) ;
	    listener.listen( _ts ) ;
	    BESDEBUG( "server", "ServerApp: listening on port ("
				<< _portVal << ")" << endl ) ;
	}

	if( !_unixSocket.empty() )
	{
	    cerr << "unixSocket = \"" << _unixSocket << "\"" << endl ;
	    _us = new UnixSocket( _unixSocket ) ;
	    listener.listen( _us ) ;
	    BESDEBUG( "server", "ServerApp: listening on unix socket ("
				<< _unixSocket << ")" << endl ) ;
	}

	BESServerHandler handler ;

	_ps = new PPTServer( &handler, &listener, _secure ) ;
	_ps->initConnection() ;
    }
    catch( BESError &se )
    {
	cerr << se.get_message() << endl ;
	(*BESLog::TheLog()) << se.get_message() << endl ;
	return 1 ;
    }
    catch( ... )
    {
	cerr << "caught unknown exception" << endl ;
	(*BESLog::TheLog()) << "caught unknown exception initializing sockets"
			    << endl ;
	return 1 ;
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

	BESDEBUG( "server", "ServerApp: terminating default module ... "
			    << endl ) ;
	BESDefaultModule::terminate( ) ;
	BESDEBUG( "server", "OK" << endl ) ;

	BESDEBUG( "server", "ServerApp: terminating default commands ...  "
			    << endl ) ;
	BESXMLDefaultCommands::terminate( ) ;
	BESDEBUG( "server", "OK" << endl ) ;

	BESModuleApp::terminate( sig ) ;
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

