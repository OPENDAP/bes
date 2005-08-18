// ServerApp.cc

#include <signal.h>
#include <unistd.h>

#include <iostream>

using std::cout ;
using std::cerr ;
using std::endl ;
using std::flush ;

#include "ServerApp.h"
#include "ServerExitConditions.h"
#include "TheDODSKeys.h"
#include "SocketListener.h"
#include "TcpSocket.h"
#include "UnixSocket.h"
#include "OPeNDAPServerHandler.h"
#include "PPTException.h"
#include "PPTServer.h"
#include "PPTException.h"
#include "SocketException.h"

ServerApp::ServerApp()
    : _portVal( 0 ),
      _gotPort( false ),
      _unixSocket( "" )
{
}

ServerApp::~ServerApp()
{
}

void
ServerApp::signalTerminate( int sig )
{
    if( sig == SIGINT )
    {
	cout << OPeNDAPApp::TheApplication()->appName() << ":" << getpid()
	     << ": got termination signal, exiting!" << endl ;
	exit( SERVER_EXIT_NORMAL_SHUTDOWN ) ;
    }
}

void
ServerApp::signalRestart( int sig )
{
    if( sig == SIGUSR1 )
    {
	cout << OPeNDAPApp::TheApplication()->appName() << ":" << getpid()
	     << ": got restart signal." << endl ;
	exit( SERVER_EXIT_RESTART ) ;
    }
}

void
ServerApp::showUsage()
{
    cout << OPeNDAPApp::TheApplication()->appName()
         << ": -d -v -p [PORT]" << endl ;
    cout << "-d set the server to debugging mode" << endl ;
    cout << "-v echos version and exit" << endl ;
    cout << "-p set port to PORT" << endl ;
    exit( 0 ) ;
}

void
ServerApp::showVersion()
{
  cout << OPeNDAPApp::TheApplication()->appName() << " version 2.0" << endl ;
  exit( 0 ) ;
}

int
ServerApp::initialize( int argc, char **argv )
{
    int retVal = OPeNDAPBaseApp::initialize( argc, argv ) ;
    if( retVal != 0 )
	return retVal ;

    cout << "Trying to register SIGINT ... " << flush ;
    if( signal( SIGINT, signalTerminate ) == SIG_ERR )
    {
	cerr << "FAILED: Can not register SIGINT signal handler" << endl ;
	exit( SERVER_EXIT_FATAL_CAN_NOT_START ) ;
    }
    else
	cout << "OK" << endl ;

    cout << "Trying to register SIGUSR1 ... " << flush ;
    if( signal( SIGUSR1, signalRestart ) == SIG_ERR )
    {
	cerr << "FAILED: Can not register SIGUSR1 signal handler" << endl ;
	exit( SERVER_EXIT_FATAL_CAN_NOT_START ) ;
    }
    else
	cout << "OK" << endl ;

    int c = 0 ;

    while( ( c = getopt( argc, argv, "dvp:" ) ) != EOF )
    {
	switch( c )
	{
	    case 'p':
		_portVal = atoi( optarg ) ;
		_gotPort = true ;
		break ;
	    case 'd':
		setDebug( true ) ;
		break ;
	    case 'v':
		showVersion() ;
		break ;
	    case '?':
		showUsage() ;
		break ;
	}
    }

    if( !_gotPort )
    {
	showUsage() ;
    }

    bool found = false ;
    string key = "DODS.ServerUnixSocket" ;
    _unixSocket = TheDODSKeys->get_key( key, found ) ;
    if( !found || _unixSocket == "" )
    {
	cout << "Unable to determine unix socket" << endl ;
	cout << "Please " << key << " in the opendap initialization file"
	     << endl ;
	exit( SERVER_EXIT_FATAL_CAN_NOT_START ) ;
    }

    return 0 ;
}

int
ServerApp::run()
{
    try
    {
	SocketListener listener ;

	TcpSocket ts( _portVal ) ;
	listener.listen( &ts ) ;

	UnixSocket us( _unixSocket ) ;
	listener.listen( &us ) ;

	OPeNDAPServerHandler handler ;

	PPTServer ps( &handler, &listener ) ;
	ps.initConnection() ;
	ps.closeConnection() ;
    }
    catch( PPTException &pe )
    {
	cerr << "caught PPTException" << endl ;
	cerr << pe.getMessage() << endl ;
	return 1 ;
    }
    catch( SocketException &se )
    {
	cerr << "caught SocketException" << endl ;
	cerr << se.getMessage() << endl ;
	return 1 ;
    }
    catch( ... )
    {
	cerr << "caught unknown exception" << endl ;
	return 1 ;
    }

    return 0 ;
}

int
main( int argc, char **argv )
{
    ServerApp app ;
    return app.main( argc, argv ) ;
}

