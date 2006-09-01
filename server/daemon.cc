// daemon.cc

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
 
// (c) COPYRIGHT University Corporation for Atmostpheric Research 2004-2005
// Please read the full copyright statement in the file COPYRIGHT_UCAR.
//
// Authors:
//      pwest       Patrick West <pwest@ucar.edu>
//      jgarcia     Jose Garcia <jgarcia@ucar.edu>

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <stdio.h>
#include <errno.h>

#include <fstream>
#include <iostream>
#include <string>

using std::ifstream ;
using std::ofstream ;
using std::cout ;
using std::endl ;
using std::cerr ;
using std::flush;
using std::string ;

#include "ServerExitConditions.h"
#include "config.h"

#define OPENDAP_SERVER_ROOT "OPENDAP_SERVER_ROOT"
#define OPENDAP_SERVER "/bes"
#define OPENDAP_SERVER_PID "bes.pid"

int  daemon_init() ;
int  mount_server( char ** ) ;
int  pr_exit( int status ) ;
void store_listener_id( int pid ) ;
bool load_names() ;

string NameProgram ;

// This two variables are set by load_names
string server_name ;
string file_for_listener ;

char **arguments = 0 ; 

int
main(int argc, char *argv[])
{
    NameProgram = argv[0] ;

    // Set the name of the listener and the file for the listenet pid
    if( !load_names() )
	return 1 ;

    if( !access( file_for_listener.c_str(), F_OK ) )
    {
	ifstream temp( file_for_listener.c_str() ) ;
	cout << NameProgram
	     << ": there seems to be a BES daemon already running at " ;
	char buf[500] ;
	temp.getline( buf, 500 ) ;
	cout << buf << endl ;
	temp.close() ;
	return 1 ;
    }

    arguments = new char *[argc+1] ;

    // Set arguments[0] to the name of the listener
    char temp_name[1024] ;
    strcpy( temp_name, server_name.c_str() ) ;
    arguments[0] = temp_name ;

    // Marshal the arguments to the listener from the command line 
    // arguments to the daemon
    for( int i = 1; i < argc; i++ )
    {
	arguments[i] = argv[i] ;
    }
    arguments[argc] = NULL ;

    daemon_init() ;

    int restart = mount_server( arguments ) ;
    if( restart == 2 )
    {
	cout << NameProgram
	     << ": server can not mount at first try (core dump). "
	     << "Please correct problems on the process manager "
	     << server_name << endl ;
	return 0 ;
    }
    while( restart )
    {
	sleep( 5 ) ;
	restart = mount_server( arguments ) ;
    }
    delete [] arguments; arguments = 0 ;

    if( !access( file_for_listener.c_str(), F_OK ) )
    {
	remove( file_for_listener.c_str() ) ;
    }

    return 0 ;
}

int
daemon_init()
{
    pid_t pid ;
    if( ( pid = fork() ) < 0 )
	return -1 ;
    else if( pid != 0 )
	exit( 0 ) ;
    setsid() ;
    return 0 ;
}

int
mount_server(char* *arguments)
{
    const char *perror_string = 0 ;
    pid_t pid ;
    int status ;
    if( ( pid = fork() ) < 0 )
    {
	cerr << NameProgram << ": fork error " ;
	perror_string = strerror( errno ) ;
	if( perror_string )
	    cerr << perror_string ;
	cerr << endl ;
	return 1 ;
    }
    else if( pid == 0 ) /* child process */
    {
	execvp( arguments[0], arguments ) ;
	cerr << NameProgram
	     << ": mounting listener, subprocess failed: " ;
	perror_string = strerror( errno ) ;
	if( perror_string )
	    cerr << perror_string ;
	cerr << endl ;
	exit( 1 ) ;
    }
    store_listener_id( pid ) ;
    if( ( pid = waitpid( pid, &status, 0 ) ) < 0 ) /* parent process */
    {
	cerr << NameProgram << ": waitpid error " ;
	perror_string = strerror( errno ) ;
	if( perror_string )
	    cerr << perror_string ;
	cerr << endl ;
	return 1 ;
    }
    int child_status = pr_exit( status ) ;
    return child_status ;
}

int
pr_exit(int status)
{
    if( WIFEXITED( status ) )
    {
	int status_to_be_returned = SERVER_EXIT_UNDEFINED_STATE ;
	switch( WEXITSTATUS( status ) )
	{
	    case SERVER_EXIT_NORMAL_SHUTDOWN:
		status_to_be_returned = 0 ;
		break ;
	    case SERVER_EXIT_FATAL_CAN_NOT_START:
		{
		    cerr << NameProgram
		         << ": server can not start, exited with status "
			 << WEXITSTATUS( status ) << endl ;
		    cerr << "Please check all error messages "
		         << "and adjust server installation" << endl ;
		    status_to_be_returned = 0 ;
		}
		break;
	    case SERVER_EXIT_ABNORMAL_TERMINATION:
		{
		    cerr << NameProgram
		         << ": abnormal server termination, exited with status "
			 << WEXITSTATUS( status ) << endl ;
		    status_to_be_returned = 1 ;
		}
		break;
	    case SERVER_EXIT_RESTART:
		{
		    cout << NameProgram
		         << ": server has been requested to re-start." << endl ;
		    status_to_be_returned = 1 ;
		}
		break;
	    default:
		status_to_be_returned = 1 ;
		break;
	}

	return status_to_be_returned;
    }
    else if( WIFSIGNALED( status ) )
    {
	cerr << NameProgram
	     << ": abnormal server termination, signaled with signal number "
	     << WTERMSIG( status ) << endl ;
#ifdef WCOREDUMP
	if( WCOREDUMP( status ) ) 
	{
	    cerr << NameProgram << ": server dumped core." << endl ;
	    return 2 ;
	}
#endif
	return 1;
    }
    else if( WIFSTOPPED( status ) )
    {
	cerr << NameProgram
	     << ": abnormal server termination, stopped with signal number "
	     << WSTOPSIG( status ) << endl ;
	return 1 ;
    }
    return 0 ;
}

void
store_listener_id( int pid )
{
    const char *perror_string = 0 ;
    ofstream f( file_for_listener.c_str() ) ;
    if( !f )
    {
	cerr << NameProgram << ": unable to create pid file "
	     << file_for_listener << ": " ;
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
    }
}

bool
load_names()
{
    char *xdap_root = 0 ;
    string bindir = "/bin";
    xdap_root = getenv( OPENDAP_SERVER_ROOT ) ;
    if( xdap_root )
    {
	server_name = xdap_root ;
        server_name += bindir ;
	file_for_listener = xdap_root ;
    }
    else
    {
	// OPENDAP_SERVER_ROOT is not set, attemp to obtain current
	// working directory
	/*
	char t_buf[1024] ;
	if( getcwd( t_buf, sizeof( t_buf ) ) )
	{
	    cout << NameProgram << ": using current working directory "
	         << t_buf << endl ;
	    server_name = t_buf ;
	    file_for_listener = t_buf ;
	}
	*/
	server_name = BES_BIN_DIR ;
	file_for_listener = BES_STATE_DIR ;
    }
    if( server_name == "" )
    {
	server_name = "." ;
        server_name += bindir ;
	file_for_listener = "." ;
    }

    server_name += OPENDAP_SERVER ;
    file_for_listener += "/run/" ;
    file_for_listener += OPENDAP_SERVER_PID ;

    if( access( server_name.c_str(), F_OK ) != 0 )
    {
	cerr << NameProgram
	     << ": can not start." << server_name << endl
	     << "Please set environment variable "
	     << OPENDAP_SERVER_ROOT << " to the location of your listener "
	     << endl ;
	return false ;
    }
    return true ;
}

