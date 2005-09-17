// OPeNDAPServerHandler.cc

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
    if( TheDODSKeys )
    {
	bool found = false ;
	_method = TheDODSKeys->get_key( "DODS.ProcessManagerMethod", found ) ;
    }
    if( _method != "multiple" && _method != "single" )
    {
	cerr << "Unable to determine method to handle clients, "
	     << "single or multiple as defined by DODS.ProcessManagerMethod"
	     << endl ;
	exit( SERVER_EXIT_FATAL_CAN_NOT_START ) ;
    }
}

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
		case DODS_PARSER_ERROR:
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

