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

#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>

#include <sstream>
#include <iostream>

using std::ostringstream ;
using std::cout ;
using std::endl ;
using std::cerr ;

#include "BESServerHandler.h"
#include "Connection.h"
#include "Socket.h"
#include "BESCmdInterface.h"
#include "TheBESKeys.h"
#include "BESException.h"
#include "ServerExitConditions.h"
#include "BESStatusReturn.h"
#include "BESUtil.h"

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
	    throw BESException( error, __FILE__, __LINE__ ) ;
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
	    throw BESException( error, __FILE__, __LINE__ ) ;
	} 
	c->closeConnection() ;
    }
}

void
BESServerHandler::execute( Connection *c )
{
    ostringstream strm ;
    strm << "ip " << c->getSocket()->getIp() << ", port " << c->getSocket()->getPort() ;
    string from = strm.str() ;

    for(;;)
    {
	ostringstream ss ;

	bool isDone = c->receive( &ss ) ;

	if( isDone )
	{
	    c->closeConnection() ;
	    exit( CHILD_SUBPROCESS_READY ) ;
	}

	int holder = dup( STDOUT_FILENO ) ;
	dup2( c->getSocket()->getSocketDescriptor(), STDOUT_FILENO ) ;
	
	BESCmdInterface cmd( BESUtil::www2id( ss.str(), "%", "%20" ) ) ;
	int status = cmd.execute_request( from ) ;

	fflush( stdout ) ;
	dup2( holder, STDOUT_FILENO ) ;
	close( holder ) ;

	if( status == BES_EXECUTED_OK )
	{
	    c->send( "" ) ;
	}
	else
	{
	    switch (status)
	    {
		case BES_TERMINATE_IMMEDIATE:
		    {
			cout << "BES server " << getpid()
			     << ": Status not OK, dispatcher returned value "
			     << status << endl ;
			//string toSend = "FATAL ERROR: server must exit!" ;
			//c->send( toSend ) ;
			c->send( "" ) ;
			c->sendExit() ;
			c->closeConnection() ;
			exit( CHILD_SUBPROCESS_READY ) ;
		    }
		    break;
		case BES_DATA_HANDLER_FAILURE:
		    {
			cout << "BES server " << getpid()
			     << ": Status not OK, dispatcher returned value "
			     << status << endl ;
			//string toSend = "Data Handler Error: server my exit!" ;
			//c->send( toSend ) ;
			c->send( "" ) ;
			c->sendExit() ;
			c->closeConnection() ;
			exit( CHILD_SUBPROCESS_READY ) ;
		    }
		    break;
		case BES_REQUEST_INCORRECT: 
		case BES_MEMORY_EXCEPTION:
		case BES_CONTAINER_PERSISTENCE_ERROR:
		case BES_INITIALIZATION_FILE_PROBLEM:
		case BES_LOG_FILE_PROBLEM:
		case BES_AGGREGATION_EXCEPTION:
		case BES_FAILED_TO_EXECUTE_COMMIT_COMMAND:
		default:
		    {
			c->send( "" ) ;
		    }
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

