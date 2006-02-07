// ClientMain.cc

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2004,2005 University Corporation for Atmospheric Research
// Author: Patrick West <pwest@ucar.org>
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
 
// (c) COPYRIGHT University Corporation for Atmostpheric Research 2004-2005
// Please read the full copyright statement in the file COPYRIGHT_UCAR.
//
// Authors:
//      pwest       Patrick West <pwest@ucar.edu>

#include <unistd.h>
#include <signal.h>

#include <iostream>
#include <string>
#include <fstream>

using std::cout ;
using std::cerr ;
using std::endl ;
using std::flush ;
using std::string ;
using std::ofstream ;

#include "CmdApp.h"
#include "CmdClient.h"
#include "PPTException.h"

CmdApp::CmdApp()
    : _client( 0 ),
      _hostStr( "" ),
      _unixStr( "" ),
      _portVal( 0 ),
      _outputStrm( 0 ),
      _inputStrm( 0 ),
      _createdInputStrm( false )
{
}

CmdApp::~CmdApp()
{
    if( _client )
    {
	delete _client ;
	_client = 0 ;
    }
}

void
CmdApp::showVersion()
{
    cout << appName() << ": version 2.0" << endl ;
}

void
CmdApp::showUsage( )
{
    cout << endl ;
    cout << appName() << ": the following flags are available:" << endl ;
    cout << "    -h <host> - specifies a host for TCP/IP connection" << endl ;
    cout << "    -p <port> - specifies a port for TCP/IP connection" << endl ;
    cout << "The flag -u specifies the name of a Unix socket for connecting to the server." << endl ;
    cout << "    -x <command> - specifies a command for the server to execute" << endl ;
    cout << "    -i <inputFile> - specifies a file name for a sequence of input commands" << endl ;
    cout << "    -f <outputFile> - specifies a file name to output the results of the input" << endl ;
    cout << "    -t <timeoutVal> - specifies an optional timeout value in seconds" << endl ;
    cout << "    -d - sets the optional debug flag for the client session" << endl ;
}

void
CmdApp::signalCannotConnect( int sig )
{
    if( sig == SIGCONT )
    {
	CmdApp *app = dynamic_cast<CmdApp *>(OPeNDAPApp::TheApplication()) ;
	if( app )
	{
	    CmdClient *client = app->client() ;
	    if( client && !client->isConnected() )
	    {
		cout << OPeNDAPApp::TheApplication()->appName()
		     << ": No response, server may be down or "
		     << "busy with another incoming connection. exiting!\n" ;
		exit( 1 ) ;
	    }
	}
    }
}

void
CmdApp::signalInterrupt( int sig )
{
    if( sig == SIGINT )
    {
	cout << OPeNDAPApp::TheApplication()->appName()
	     << ": Please type exit to terminate the session" << endl ;
    }
    if( signal( SIGINT, CmdApp::signalInterrupt ) == SIG_ERR )
    {
	cerr << OPeNDAPApp::TheApplication()->appName()
	     << ": Could not re-register signal\n" ;
    }
}

void
CmdApp::signalTerminate( int sig )
{
    if( sig == SIGTERM )
    {
	cout << OPeNDAPApp::TheApplication()->appName()
	     << ": Please type exit to terminate the session" << endl ;
    }
    if( signal( SIGTERM, CmdApp::signalTerminate ) == SIG_ERR )
    {
	cerr << OPeNDAPApp::TheApplication()->appName()
	     << ": Could not re-register signal\n" ;
    }
}

void
CmdApp::signalBrokenPipe( int sig )
{
    if( sig == SIGPIPE )
    {
	cout << OPeNDAPApp::TheApplication()->appName()
	     << ": got a broken pipe, server may be down or the port invalid."
	     << endl
	     << "Please check parameters and try again" << endl ;
	CmdApp *app = dynamic_cast<CmdApp *>(OPeNDAPApp::TheApplication()) ;
	if( app )
	{
	    CmdClient *client = app->client() ;
	    if( client )
	    {
		client->brokenPipe() ;
	    }
	}
	exit( 1 ) ;
    }
}

void
CmdApp::registerSignals()
{
    // Registering SIGCONT for connection unblocking
    if( signal( SIGCONT, signalCannotConnect ) == SIG_ERR )
    {
	cerr << appName() << ": Registering signal SIGCONT ... " << flush ;
	cerr << "FAILED" << endl ;
	exit( 1 ) ;
    }

    // Registering SIGINT to disable Ctrl-C from the user in order to avoid
    // server instability
    if( signal( SIGINT, signalInterrupt ) == SIG_ERR )
    {
	cerr << appName() << ": Registering signal SIGINT ... " << flush ;
	cerr << "FAILED" << endl ;
	exit( 1 ) ;
    }

    // Registering SIGTERM to disable kill from the user in order to avoid
    // server instability
    if( signal( SIGTERM, signalTerminate ) == SIG_ERR )
    {
	cerr << appName() << ": Registering signal SIGINT ... " << flush ;
	cerr << "FAILED" << endl ;
	exit( 1 ) ;
    }

    // Registering SIGPIE for broken pipes managment.
    // Registering SIGPIE for broken pipes managment.
    if( signal( SIGPIPE, CmdApp::signalBrokenPipe ) == SIG_ERR )
    {
	cerr << appName() << ": Registering signal SIGPIPE ... " << flush ;
	cerr << "FAILED" << endl ;
	exit( 1 ) ;
    }
}

int
CmdApp::initialize( int argc, char **argv )
{
    int retVal = OPeNDAPBaseApp::initialize( argc, argv ) ;
    if( retVal != 0 )
	return retVal ;

    registerSignals() ;

    string portStr = "" ;
    string outputStr = "" ;
    string inputStr = "" ;
    string timeoutStr = "" ;

    bool badUsage = false ;

    int c ;

    while( ( c = getopt( argc, argv, "dvh:p:t:u:x:f:i:" ) ) != EOF )
    {
	switch( c )
	{
	    case 't':
		timeoutStr = optarg ;
		break ;
	    case 'h':
		_hostStr = optarg ;
		break ;
	    case 'd':
		setDebug( true ) ;
		break ;
	    case 'v':
		showVersion() ;
		break ;
	    case 'p':
		portStr = optarg ;
		break ;  
	    case 'u':
		_unixStr = optarg ;
		break ;
	    case 'x':
		_cmd = optarg ;
		break ;
	    case 'f':
		outputStr = optarg ;
		break ;
	    case 'i':
		inputStr = optarg ;
		break ;
	    case '?':
		showUsage() ;
		break ;
	}
    }
    if( _hostStr == "" && _unixStr == "" )
    {
	cerr << "host and port or unix socket must be specified" << endl ;
	badUsage = true ;
    }

    if( _hostStr != "" && _unixStr != "" )
    {
	cerr << "must specify either a host and port or a unix socket" << endl ;
	badUsage = true ;
    }

    if( portStr != "" && _unixStr != "" )
    {
	cerr << "must specify either a host and port or a unix socket" << endl ;
	badUsage = true ;
    }

    if( _hostStr != "" )
    {
	if( portStr == "" )
	{
	    cout << "port must be specified when specifying a host" << endl ;
	    badUsage = true ;
	}
	else
	{
	    _portVal = atoi( portStr.c_str() ) ;
	}
    }

    if( timeoutStr != "" )
    {
	_timeoutVal = atoi( timeoutStr.c_str() ) ;
    }

    if( outputStr != "" )
    {
	if( _cmd == "" && inputStr == "" )
	{
	    cerr << "When specifying an output file you must either "
	         << "specify a command or an input file"
		 << endl ;
	    badUsage = true ;
	}
	else if( _cmd != "" && inputStr != "" )
	{
	    cerr << "You must specify either a command or an input file on "
	         << "the command line, not both"
		 << endl ;
	    badUsage = true ;
	}
    }

    if( badUsage == true )
    {
	showUsage( ) ;
	return 1 ;
    }

    if( outputStr != "" )
    {
	_outputStrm = new ofstream( outputStr.c_str() ) ;
	if( !(*_outputStrm) )
	{
	    cerr << "could not open the output file " << outputStr << endl ;
	    badUsage = true ;
	}
    }

    if( inputStr != "" )
    {
	_inputStrm = new ifstream( inputStr.c_str() ) ;
	if( !(*_inputStrm) )
	{
	    cerr << "could not open the input file " << inputStr << endl ;
	    badUsage = true ;
	}
	_createdInputStrm = true ;
    }

    if( badUsage == true )
    {
	showUsage( ) ;
	return 1 ;
    }

    return 0 ;
}

int
CmdApp::run()
{
    try
    {
	_client = new CmdClient( ) ;
	if( _hostStr != "" )
	{
	    _client->startClient( _hostStr, _portVal ) ;
	}
	else
	{
	    _client->startClient( _unixStr ) ;
	}

	if( _outputStrm )
	{
	    _client->setOutput( _outputStrm, true ) ;
	}
	else
	{
	    _client->setOutput( &cout, false ) ;
	}
    }
    catch( PPTException &e )
    {
	cerr << "error starting the client" << endl ;
	cerr << e.getMessage() << endl ;
	exit( 1 ) ;
    }

    try
    {
	if( _cmd != "" )
	{
	    cout << "executeCommand" << endl ;
	    _client->executeCommand( _cmd ) ;
	}
	else if( _inputStrm )
	{
	    cout << "executeCommands" << endl ;
	    _client->executeCommands( *_inputStrm ) ;
	}
	else
	{
	    _client->interact() ;
	}
    }
    catch( PPTException &e )
    {
	cerr << "error processing commands" << endl ;
	cerr << e.getMessage() << endl ;
    }

    try
    {
	if( _client )
	{
	    _client->shutdownClient() ;
	}
	if( _createdInputStrm )
	{
	    _inputStrm->close() ;
	    delete _inputStrm ;
	    _inputStrm = 0 ;
	}
    }
    catch( PPTException &e )
    {
	cerr << "error closing the client" << endl ;
	cerr << e.getMessage() << endl ;
	return 1 ;
    }

    return 0 ;
}

int
main( int argc, char **argv )
{
    CmdApp app ;
    return app.main( argc, argv ) ;
}

