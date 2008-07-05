// BESServerHandler.cc

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2004,2005 University Corporation for Atmospheric Research
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

#include <unistd.h>    // for getpid fork sleep
#include <sys/types.h>
#include <sys/socket.h>
#include <signal.h>
#include <sys/wait.h>  // for waitpid

#include <cstring>
#include <cstdlib>
#include <cerrno>
#include <sstream>
#include <iostream>

using std::ostringstream ;
using std::cout ;
using std::endl ;
using std::cerr ;
using std::flush ;

#include "BESServerHandler.h"
#include "Connection.h"
#include "Socket.h"
#include "BESCmdInterface.h"
#include "TheBESKeys.h"
#include "BESInternalError.h"
#include "ServerExitConditions.h"
#include "BESUtil.h"
#include "PPTStreamBuf.h"
#include "PPTProtocol.h"
#include "BESDebug.h"
#include "BESStopWatch.h"

BESServerHandler::BESServerHandler()
{
    bool found = false ;
    _method = TheBESKeys::TheKeys()->get_key( "BES.ProcessManagerMethod", found ) ;
    if( _method != "multiple" && _method != "single" )
    {
	cerr << "Unable to determine method to handle clients, "
	     << "single or multiple as defined by BES.ProcessManagerMethod"
	     << endl ;
	exit( SERVER_EXIT_FATAL_CAN_NOT_START ) ;
    }
}

// *** I'm not sure that we need to fork twice. jhrg 11/14/05
// The reason that we fork twice is explained in Advanced Programming in the
// Unit Environment by W. Richard Stevens. In the 'multiple' case we don't
// want to leave any zombie processes.
void
BESServerHandler::handle( Connection *c )
{
    if(_method=="single")
    {
	execute( c ) ;
    }
    else
    {
	int main_process = getpid() ;
	pid_t pid ;
	if( ( pid = fork() ) < 0 )
	{
	    string error( "fork error" ) ;
	    const char* error_info = strerror( errno ) ;
	    if( error_info )
		error += " " + (string)error_info ;
	    throw BESInternalError( error, __FILE__, __LINE__ ) ;
	}
	else if( pid == 0 ) /* child process */
	{
	    pid_t pid1 ;
	    // we fork twice so we do not have zombie children
	    if( ( pid1 = fork() ) < 0 )
	    {
		// we must send a signal of inmediate termination to the
		// main server 
		kill( main_process, 9 ) ;
		perror( "fork error" ) ;
		exit( SERVER_EXIT_CHILD_SUBPROCESS_ABNORMAL_TERMINATION ) ;
	    } 
	    else if( pid1 == 0 ) /* child of the child */
	    {
		execute( c ) ;
	    }
	    sleep( 1 ) ;
	    c->closeConnection() ;
	    exit( SERVER_EXIT_CHILD_SUBPROCESS_NORMAL_TERMINATION ) ;
	}
	if( waitpid( pid, NULL, 0 ) != pid )
	{
	    string error( "waitpid error" ) ;
	    const char *error_info = strerror( errno ) ;
	    if( error_info )
		error += " " + (string)error_info ;
	    throw BESInternalError( error, __FILE__, __LINE__ ) ;
	} 
	c->closeConnection() ;
    }
}

void
BESServerHandler::execute( Connection *c )
{
    ostringstream strm ;
    string ip = c->getSocket()->getIp() ;
    strm << "ip " << ip << ", port " << c->getSocket()->getPort() ;
    string from = strm.str() ;

    map<string,string> extensions ;

    for(;;)
    {
	ostringstream ss ;

	bool done = false ;
	while( !done )
	    done = c->receive( extensions, &ss ) ;

	if( extensions["status"] == c->exit() )
	{
	    c->closeConnection() ;
	    exit( CHILD_SUBPROCESS_READY ) ;
	}

	string cmd_str = BESUtil::www2id( ss.str(), "%", "%20" ) ;
	BESDEBUG( "server", "BESServerHandler::execute - command = " << cmd_str << endl )

	BESStopWatch *sw = 0 ;
	if( BESISDEBUG( "timing" ) )
	{
	    sw = new BESStopWatch() ;
	    sw->start() ;
	}

	int descript = c->getSocket()->getSocketDescriptor() ;
	/*
	unsigned int sockbufsize = 0 ;
	int size = sizeof(int) ;
	int err = getsockopt( descript, SOL_SOCKET, SO_RCVBUF,
			      (void *)&sockbufsize, (socklen_t*)&size) ;
	cerr << "The size of the receive buffer is " << sockbufsize << endl ;
	err = getsockopt( descript, SOL_SOCKET, SO_SNDBUF,
			  (void *)&sockbufsize, (socklen_t*)&size ) ;
	cerr << "The size of the send buffer is " << sockbufsize << endl ;
	*/

	unsigned int bufsize = c->getSendChunkSize() ;
	PPTStreamBuf fds( descript, bufsize ) ;
	std::streambuf *holder ;
	holder = cout.rdbuf() ;
	cout.rdbuf( &fds ) ;

	BESCmdInterface cmd( cmd_str, &cout ) ;
	int status = cmd.execute_request( from ) ;

	if( status == 0 )
	{
	    BESDEBUG( "server", "BESServerHandler::execute - executed successfully" << endl )
	    fds.finish() ;
	    cout.rdbuf( holder ) ;

	    if( BESISDEBUG( "timing" ) )
	    {
		if( sw && sw->stop() )
		{
		    BESDEBUG( "timing",
			      "BESServerHandler::execute - executed in "
			      << sw->seconds() << " seconds and "
			      << sw->microseconds() << " microseconds"
			      << endl )
		}
		else
		{
		    BESDEBUG( "timing", \
			  "BESServerHandler::execute - no timing available" )
		}
	    }
	}
	else
	{
	    // an error has occurred.
	    BESDEBUG( "server", "BESServerHandler::execute - error occurred" << endl )

	    // flush what we have in the stream to the client
	    cout << flush ;

	    // Send the extension status=error to the client so that it can reset.
	    map<string,string> extensions ;
	    extensions["status"] = "error" ;
	    c->sendExtensions( extensions ) ;

	    // transmit the error message. finish_with_error will transmit
	    // the error
	    cmd.finish_with_error( status ) ;

	    // we are finished, send the last chunk
	    fds.finish() ;

	    // reset the streams buffer
	    cout.rdbuf( holder ) ;

	    switch (status)
	    {
		case BES_INTERNAL_FATAL_ERROR:
		    {
			cout << "BES server " << getpid()
			     << ": Status not OK, dispatcher returned value "
			     << status << endl ;
			//string toSend = "FATAL ERROR: server must exit!" ;
			//c->send( toSend ) ;
			c->sendExit() ;
			c->closeConnection() ;
			exit( CHILD_SUBPROCESS_READY ) ;
		    }
		    break;
		case BES_INTERNAL_ERROR: 
		case BES_SYNTAX_USER_ERROR:
		case BES_FORBIDDEN_ERROR:
		case BES_NOT_FOUND_ERROR:
		default:
		    break;
	    }
	}
    }
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance
 *
 * @param strm C++ i/o stream to dump the information to
 */
void
BESServerHandler::dump( ostream &strm ) const
{
    strm << BESIndent::LMarg << "BESServerHandler::dump - ("
			     << (void *)this << ")" << endl ;
    BESIndent::Indent() ;
    strm << BESIndent::LMarg << "server method: " << _method << endl ;
    BESIndent::UnIndent() ;
}

