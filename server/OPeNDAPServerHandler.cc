// OPeNDAPServerHandler.cc

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

#include "OPeNDAPServerHandler.h"
#include "Connection.h"
#include "Socket.h"
#include "OPeNDAPCmdInterface.h"
#include "TheDODSKeys.h"
#include "DODSBasicException.h"
#include "ServerExitConditions.h"
#include "DODSStatusReturn.h"

OPeNDAPServerHandler::OPeNDAPServerHandler()
{
    bool found = false ;
    _method = TheDODSKeys::TheKeys()->get_key( "DODS.ProcessManagerMethod", found ) ;
    if( _method != "multiple" && _method != "single" )
    {
	cerr << "Unable to determine method to handle clients, "
	     << "single or multiple as defined by DODS.ProcessManagerMethod"
	     << endl ;
	exit( SERVER_EXIT_FATAL_CAN_NOT_START ) ;
    }
}

// *** I'm not sure that we need to fork twice. jhrg 11/14/05
void
OPeNDAPServerHandler::handle( Connection *c )
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
	    throw DODSBasicException( error ) ;
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
	    throw DODSBasicException( error ) ;
	} 
	c->closeConnection() ;
    }
}

void
OPeNDAPServerHandler::execute( Connection *c )
{
    for(;;)
    {
	ostringstream ss ;

	bool isDone = c->receive( &ss ) ;

	if( isDone )
	{
	    return ;
	}

	int holder = dup( STDOUT_FILENO ) ;
	dup2( c->getSocket()->getSocketDescriptor(), STDOUT_FILENO ) ;
	
	OPeNDAPCmdInterface cmd( ss.str() ) ;
	int status = cmd.execute_request() ;

	fflush( stdout ) ;
	dup2( holder, STDOUT_FILENO ) ;
	close( holder ) ;

	if( status == DODS_EXECUTED_OK )
	{
	    c->send( "" ) ;
	}
	else
	{
	    switch (status)
	    {
		case DODS_TERMINATE_IMMEDIATE:
		    {
			cout << "DODS server " << getpid()
			     << ": Status not OK, dispatcher returned value "
			     << status << endl ;
			string toSend = "FATAL ERROR: server must exit!" ;
			c->send( toSend ) ;
			c->sendExit() ;
			exit( CHILD_SUBPROCESS_READY ) ;
		    }
		    break;
		case DODS_DATA_HANDLER_FAILURE:
		    {
			cout << "DODS server " << getpid()
			     << ": Status not OK, dispatcher returned value "
			     << status << endl ;
			string toSend = "Data Handler Error: server my exit!" ;
			c->send( toSend ) ;
			c->sendExit() ;
			exit( CHILD_SUBPROCESS_READY ) ;
		    }
		    break;
		case DODS_REQUEST_INCORRECT: 
		case DODS_MEMORY_EXCEPTION:
#if 0
		case DODS_MYSQL_CONNECTION_FAILURE: 
		case DODS_MYSQL_BAD_QUERY:
#endif
		case DODS_CONTAINER_PERSISTENCE_ERROR:
		case DODS_INITIALIZATION_FILE_PROBLEM:
		case DODS_LOG_FILE_PROBLEM:
		case DODS_AGGREGATION_EXCEPTION:
		case OPeNDAP_FAILED_TO_EXECUTE_COMMIT_COMMAND:
		default:
		    {
			c->send( "" ) ;
		    }
		    break;
	    }
	}
    }
}

